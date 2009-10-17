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

        if (changeID() < 0) {
            logError("master", LOG_ERROR, "Unable start with user: %s and group: %s", conf.user.c_str(), conf.group.c_str());
            return 252;
        }
        
        if (chdir(wsgi.base_dir.c_str()) < 0) {
            logError("master", LOG_ERROR, "Unable to change working directory to <%s>, check ownership", wsgi.base_dir.c_str());
            return 251;
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
        int ec;
        ipc_sem s;
        static int sem_id = 99;     // random number here
        sem_id++;                   // choosed by roll dice
        
        if ((ec = s.initSEM(sem_id)) < 0) {
            logError("master", LOG_ERROR, "unable to init startup semaphore: %d", ec);
            return -1;            
        }

        s.lock();
        
        int fpid = fork();
        
        if (fpid == 0) {            
            s.lock();
            
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
            
            s.unlock();
            s.freeSEM(true);
            
            ec = w.startWorker();
            
            if (ec < 0) {
                logError("master", LOG_ERROR, "unable to start worker: %d", ec);
                w.nowaitWorker();
            } else {
                w.waitWorker();
            }
            
            exit(0);
        } else if (fpid > 0){
            child_t c;

            if ((ec = c.cipc.initSHM(fpid, true)) < 0) {
                logError("master", LOG_ERROR, "shm inialization failed with: %d", ec);
                return -1;
            }
            
            c.dead = false;
            childrens[fpid] = c;

            s.unlock();
            s.freeSEM();
            
            return 0;
        } else {
            return -1;
        }
    }
    
    /*!
        @method     fastPy::masterLoop()
        @abstract   worker scheduler
        @discussion master loop and scheduler
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
                    logError("master", LOG_WARN, "starting new child");
                    startChild();
                    break;
                }
                
                if (c->cipc.shm->timestamp != 0 && (c->cipc.shm->timestamp + 10) < time(NULL)) {
                    logError("master", LOG_WARN, "child ts is timed out, terminating child");
                    c->cipc.freeSHM(true);
                    childrens.erase(c_it);
                    startChild();
                    break;
                }
                                
                switch (c->cipc.shm->w_code) {
                    case W_TERM:
                        c->dead = true;
                        break;
                    case W_FAIL:
                        working = false;
                        break;
                    default:
                        break;
                }
            }
            
            // checking our own state
            if (msig != 0) {
                switch (msig) {
                    case SIGHUP:
                        logError("master", LOG_WARN, "restarting workers pool");
                        for (c_it = childrens.begin(); c_it != childrens.end(); c_it++) {
                            child_t *c = &(*c_it).second;
                            c->cipc.shm->m_code = M_TERM;
                        }
                        
                        msig = 0;
                        
                        break;
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
            
            c->cipc.shm->m_code = M_TERM;
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
                
                if (c->cipc.shm->timestamp != 0 && (c->cipc.shm->timestamp + 10) < time(NULL)) {
                    logError("master", LOG_DEBUG, "looks like child is already dead");
                    c->cipc.freeSHM(true);
                    childrens.erase(c_it);
                    break;
                }

                if (c->cipc.shm->w_code == W_TERM || c->cipc.shm->w_code == W_FAIL) {
                    c->dead = true;
                }
            }
            
            if (childrens.size() == 0) {
                break;
            }

            usleep(10000);
        }
        
        logError("master", LOG_WARN, "childs are dead, terminating master");
        
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
        @method     fastPy::changeID()
        @abstract   change uid and gid
        @discussion get uid and git from named and set them if we can
    */

    int fastPy::changeID() {
        passwd *pd;
        group *gr;
        uid_t cr_uid, cr_gid;
        uid_t to_uid, to_gid;

        cr_uid = geteuid();
        cr_gid = getegid();
        
        if ((pd = getpwnam(conf.user.c_str())) == NULL) {
            logError("master", LOG_ERROR, "username {%s} does not exist", conf.user.c_str());
            return -1;
        }
        
        to_uid = pd->pw_uid;
        
        if ((gr = getgrnam(conf.group.c_str())) == NULL) {
            logError("master", LOG_ERROR, "group {%s} does not exist", conf.group.c_str());
            return -2;
        }
        
        to_gid = gr->gr_gid;
        
        if (cr_gid != to_gid) {
            if (cr_uid != 0) {
                logError("master", LOG_ERROR, "unable to set gid: %d because i`m not root", to_gid);
                return -4;
            }
            
            if (setgid(to_gid) == -1) {
                logError("master", LOG_ERROR, "unable to set gid: %d", to_gid);
                return -5;
            }
            
            if (initgroups(pd->pw_name, to_gid) == -1) {
                logError("master", LOG_ERROR, "initgroups(%s, %d) failed", pd->pw_name, to_gid);
                return -3;
            }            
        }        
                
        if (cr_uid != to_uid) {
            if (cr_uid != 0) {
                logError("master", LOG_ERROR, "unable to set uid: %d because i`m not root", to_uid);
                return -4;
            }
            
            if (setuid(to_uid) == -1) {
                logError("master", LOG_ERROR, "unable to set uid: %d", to_uid);
                return -4;
            }
        }
        
        return 0;
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
