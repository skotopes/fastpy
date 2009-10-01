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
    bool fastPy::able_to_work = true;
    bool fastPy::csig_new = false;
    int fastPy::csig_cnt = 0;
    int fastPy::csig = 0;

    fastPy::fastPy() {
        config_f = NULL;
        sock_f = NULL;
    }
    
    fastPy::~fastPy() {
        
    }
    
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
        
        int ec = runFPy();

        return ec;
    }
    
    int fastPy::runFPy() {
        fcgi = new fastcgi;
        
        // opening socket
        if (fcgi->openSock(sock_f) < 0) {
            return 250;
        }
        
        if (detach) {
            int fpid = fork();
            
            if (fpid == 0) {
                // some where in kenya(it`s our childrens)
                int fd = open("/dev/null", O_RDWR);
                
                if (fd == -1) {
                    logError("master", LOG_ERROR, "Unable to open /dev/null");
                    return 252;
                }
                
                if (dup2(fd, STDIN_FILENO) == -1) {
                    logError("master", LOG_ERROR, "Unable to close stdin");
                    return 252;
                }
                
                if (dup2(fd, STDOUT_FILENO) == -1) {
                    logError("master", LOG_ERROR, "Unable to close stdout");
                    return 252;
                }
                                
                if (fd > STDERR_FILENO) {
                    if (close(fd) == -1) {
                    logError("master", LOG_ERROR, "Unable to close stderr");
                        return 252;
                    }
                }
            } else if (fpid > 0) {
                return 0;
            } else {
                return -1;
            }
        } 

        // forking and working
        for (int i=0; i < conf.workers_cnt; i++) {
            createChild();
        }
        
        if (yesMaster() < 0) {
            return 251;
        }

        return 0;
    }
    

    int fastPy::createChild() {
        int fpid = fork();
        
        if (fpid == 0) {
            // some where in kenya(it`s our childrens)
            worker w(fcgi);

            // we not checking exit codes because waitWorker is important
            w.startWorker();
            w.waitWorker();
            
            exit(0);                
        } else if (fpid > 0){
            // lions and tigers
            child_t c;
            
            c.terminated = false;
            
            int e = c.cipc.initSHM(fpid, true);
            
            if (e < 0) {
                logError("master", LOG_ERROR, "shm inialization failed");
                return -1;
            }
                        
            childrens[fpid] = c;
        } else {
            return -1;
        }
        
        return 0;
    }
    
    int fastPy::yesMaster() {
        bool able_to_die = false;
        std::map<int,child_t>::iterator c_it;

        // registering signal handler
        signal(SIGHUP, sig_handler);
        signal(SIGINT, sig_handler);
        signal(SIGTERM, sig_handler);
        signal(SIGUSR1, sig_handler);
        signal(SIGUSR2, sig_handler);

        while (able_to_work) {
            
            if (csig_new) {
                csig_new = false;
                csig_cnt += conf.workers_cnt;
            }
            
            for (c_it = childrens.begin(); c_it != childrens.end(); c_it++) {
                int c_pid = (*c_it).first;
                child_t *c = &(*c_it).second;

                // if child is terminated
                if (c->terminated) {
                    c->cipc.closeMQ(true);
                    childrens.erase(c_it);
                    break;
                }
                
                c->cipc.lock();
                if (csig_cnt != 0 && csig == SIGUSR1) {
                    c->cipc.shm->m_code = M_DRLD;
                    csig_cnt --;
                }
                
                switch (c->cipc.shm->w_code) {
                    case W_NRDY:
                        break;
                    case W_FINE:
                        break;
                    case W_IRLD:
                        break;
                    case W_BUSY:
                        break;
                    case W_FAIL:
                        break;
                    case W_TERM:
                        break;
                    default:
                        break;
                }
                c->cipc.unlock();
            }
            
            if (!csig_new) csig = 0;
            usleep(100000);
        }

        logError("master", LOG_DEBUG, "terminating workers");
        // terminating processes
        while (!able_to_die) {
            for (c_it = childrens.begin(); c_it != childrens.end(); c_it++) {
                child_t *c = &(*c_it).second;
                
                // if child is terminated
                if (c->terminated) {
                    c->cipc.closeMQ(true);
                    childrens.erase(c_it);
                    break;
                }
                
                c->cipc.lock();
                c->cipc.shm->m_code = M_TERM;
                if (c->cipc.shm->w_code == W_TERM) c->terminated = true;
                c->cipc.unlock();
            }
            
            if (childrens.size() == 0) {
                able_to_die = true;
            }
            
            usleep(100000);
        }

        logError("master", LOG_DEBUG, "terminating master");
        return 0;
    }
    
    void fastPy::usage() {
        std::cout << "Usage:"<< std::endl
            << "fastpy OPTIONS"<< std::endl
            << std::endl
            << "Options:"<< std::endl
            << "-s [unix_socket]"<< std::endl
            << "-c [config_file]"<< std::endl
            << "-d \t\t- detach from console"<< std::endl
            << "-h \t\t- help"<< std::endl
            << "-v \t\t- verbose output(more v - more verbose)"<< std::endl;
    }

    void fastPy::sig_handler(int s) {
        if (s == SIGTERM || s == SIGINT) able_to_work = false;
        csig = s;
        csig_new = true;
    }
}

using namespace fp;

int main(int argc, char *argv[]){
    fastPy fps;    
    return fps.go(argc, argv);
}
