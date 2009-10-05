/*
 *  fastjs.cpp
 *  fastPy
 *
 *  Created by Alexandr Kutuzov on 02.09.09.
 *  Copyright 2009 White-label ltd. All rights reserved.
 *
 */

#include "fastpy.h"

namespace fp {
    // static members
    int fastPy::msig = 0;

    /*!
        @method     fastPy::fastPy()
        @abstract   class constructor
        @discussion class constructor, nothing else
    */

    fastPy::fastPy() {
        config_f = NULL;
        sock_f = NULL;
    }
    
    /*!
        @method     fastPy::~fastPy()
        @abstract   class destructor
        @discussion do nothing
    */

    fastPy::~fastPy() {
        
    }
    
    /*!
        @method     fastPy::go(int argc, char **argv)
        @abstract   entry point
        @discussion parsing args, starting app
    */

    int fastPy::go(int argc, char **argv) {
        int c;
        
        app_name = argv[0];
        
        while ((c = getopt (argc, argv, "dhvc:s:")) != -1) {
            switch (c) {
                case 'd':
                    detach = true;
                    break;                    
                case 'v':
                    if (verbose) debug = true;
                    verbose = true;
                    break;
                case 'c':
                    config_f = optarg;
                    break;
                case 's':
                    sock_f = optarg;
                    break;                    
                case 'h':
                default:
                    usage();
                    return 255;
            }        
        }
        
        if (config_f == NULL || sock_f == NULL) {
            ts_cout("Config and socket is required");
            usage();
            return 254;
        }
        
        if (readConf(config_f) < 0) {
            return 253;
        }
        
        fcgi = new fastcgi;
        
        // opening socket
        if (fcgi->openSock(sock_f) < 0) {
            logError("master", LOG_ERROR, "Unable to open socket");
            return 250;
        }
        
        if (detach) {
            int d_rc = detachProc();
            if (d_rc > 0) {
                // write pid file here
                return 0;
            } else if (d_rc < 0) {
                return 249;
            }
        } 
        
        for (int i=0; i < conf.workers_cnt; i++) {
            startChild();
        }
        
        if (masterLoop() < 0) {
            return 248;
        }
        
        return 0;
    }

    /*!
        @method     fastPy::startChild()
        @abstract   create child process
        @discussion create child process return zero on success
    */

    int fastPy::startChild() {
        int fpid = fork();
        
        if (fpid == 0) {
            // removing shm zones and master structs
            std::map<int,child_t>::iterator c_it;
            
            for (c_it = childrens.begin(); c_it != childrens.end(); c_it++) {
                child_t *c = &(*c_it).second;
                // closing but not deleting
                c->cipc.freeSHM();
            }
            
            childrens.clear();
            
            // starting worker
            worker w(fcgi);

            w.startWorker(); // we not checking exit codes because waitWorker is important
            w.waitWorker();
            
            exit(0);
        } else if (fpid > 0){
            child_t c;
            
            int e = c.cipc.initSHM(fpid, true);
            
            if (e < 0) {
                logError("master", LOG_ERROR, "shm inialization failed with: %d", e);
                return -1;
            }
            
            c.dead = false;
            childrens[fpid] = c;
            return 0;
        } else {
            return -1;
        }
    }
    
    /*!
        @method     fastPy::masterLoop()
        @abstract   worker scheduller
        @discussion master loop and scheduller
    */
    
    int fastPy::masterLoop() {
        std::map<int,child_t>::iterator c_it;

        // registering signal handler
        signal(SIGHUP, sig_handler);
        signal(SIGINT, sig_handler);
        signal(SIGTERM, sig_handler);
        signal(SIGUSR1, sig_handler);
        signal(SIGUSR2, sig_handler);

        bool working = true, terminating = true;

        logError("master", LOG_DEBUG, "schedulling workers");
        // main schedule loop
        while (working) {
            // iterating throw the map
            for (c_it = childrens.begin(); c_it != childrens.end(); c_it++) {
                child_t *c = &(*c_it).second;
                
                if (c->dead) {
                    c->cipc.freeSHM(true);
                    childrens.erase(c_it);
                    startChild();
                    break;
                }
                
                c->cipc.lock();
                
                if (c->cipc.shm->timestamp != 0 && (c->cipc.shm->timestamp + 10) < time(NULL)) {
                    logError("master", LOG_DEBUG, "child ts is timed out, terminating child");
                    c->cipc.freeSHM(true);
                    childrens.erase(c_it);
                    startChild();
                    break;
                }
                
                switch (c->cipc.shm->w_code) {
                    case W_TERM:
                        c->dead = true;
                        break;
                    default:
                        break;
                }
                
                c->cipc.unlock();
            }
            
            // checking our own state
            if (msig != 0) {
                switch (msig) {
                    case SIGTERM:
                    case SIGINT:
                        working = false;
                        break;
                    default:
                        break;
                }
            }

            usleep(200000);
        }
        
        logError("master", LOG_WARN, "going to shutdown");

        for (c_it = childrens.begin(); c_it != childrens.end(); c_it++) {
            child_t *c = &(*c_it).second;
            
            c->cipc.lock();
            c->cipc.shm->m_code = M_TERM;
            c->cipc.unlock();
        }
        
        // main terminate loop
        while (terminating) {            
            for (c_it = childrens.begin(); c_it != childrens.end(); c_it++) {
                child_t *c = &(*c_it).second;

                if (c->dead) {
                    c->cipc.freeSHM(true);
                    childrens.erase(c_it);
                    break;
                }
                
                // TODO: probably lock is not required here
                c->cipc.lock();

                if (c->cipc.shm->w_code == W_TERM) {
                    c->dead = true;
                }
                
                c->cipc.unlock();
            }
            
            if (childrens.size() == 0) {
                break;
            }

            usleep(10000);
        }
        
        logError("master", LOG_WARN, "terminating master");
        
        return 0;
    }

    /*!
        @method     fastPy::detachProc()
        @abstract   detach from console
        @discussion service routine, probably should be moved out of here
    */

    int fastPy::detachProc() {
        pid_t fpid = fork();
        
        if (fpid == 0) {
            // some where in kenya(it`s our childrens)
            int fd = open("/dev/null", O_RDWR);
            
            if (fd == -1) {
                logError("master", LOG_ERROR, "Unable to open /dev/null");
                return -1;
            }
            
            if (dup2(fd, STDIN_FILENO) == -1) {
                logError("master", LOG_ERROR, "Unable to close stdin");
                return -1;
            }
            
            if (dup2(fd, STDOUT_FILENO) == -1) {
                logError("master", LOG_ERROR, "Unable to close stdout");
                return -1;
            }
            
            if (fd > STDERR_FILENO) {
                if (close(fd) == -1) {
                    logError("master", LOG_ERROR, "Unable to close stderr");
                    return -1;
                }
            }
            
            return 0;
        } else if (fpid > 0) {
            return fpid;
        } else {
            logError("master", LOG_ERROR, "Unable to fork");
            return -2;
        }
    }
    
    /*!
        @method     fastPy::usage()
        @abstract   usage information
        @discussion usage information (non thread safe)
    */

    void fastPy::usage() {
        std::cout << "Usage:" << "fastpy OPTIONS"<< std::endl
            << std::endl
            << "Options:"<< std::endl
            << "-s [unix_socket]"<< std::endl
            << "-c [config_file]"<< std::endl
            << "-d \t\t- detach from console"<< std::endl
            << "-v \t\t- verbose output(-vv - for debug messages)"<< std::endl
            << "-h \t\t- help"<< std::endl;
    }
    
    /*!
        @method     fastPy::sig_handler(int s)
        @abstract   signal handler
        @discussion 
    */

    void fastPy::sig_handler(int s) {
        msig = s;
    }
}

using namespace fp;

int main(int argc, char *argv[]){
    fastPy fps;    
    return fps.go(argc, argv);
}
