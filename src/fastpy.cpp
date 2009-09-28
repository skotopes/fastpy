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
    
    fastPy::fastPy() {
        config_f = NULL;
        sock_f = NULL;
        detach = false;
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
            std::cout << "Config and socket is required" << std::endl;
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
                    logError("Unable to open /dev/null");
                    return 252;
                }
                
                if (dup2(fd, STDIN_FILENO) == -1) {
                    logError("Unable to close stdin");
                    return 252;
                }
                
                if (dup2(fd, STDOUT_FILENO) == -1) {
                    logError("Unable to close stdout");
                    return 252;
                }
                                
                if (fd > STDERR_FILENO) {
                    if (close(fd) == -1) {
                        logError("Unable to close stderr");
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
            
            w.startWorker();
            w.waitWorker();
            
            exit(0);                
        } else if (fpid > 0){
            // lions and tigers
            child_t c;
            
            int e = c.cipc.initMQ(fpid, true);
            
            if (e <0) {
                std::cout << "master mq init error: "<< e << " en: " << errno << std::endl;
            }
            
            c.conn_served = 0;
            c.conn_failed = 0;
            
            childrens[fpid] = c;
        } else {
            return -1;
        }
        
        return 0;
    }
    
    int fastPy::yesMaster() {
        bool able_to_die = false;
        
        // registering signal handler
        signal(SIGHUP, sig_handler);
        signal(SIGINT, sig_handler);
        signal(SIGTERM, sig_handler);
        signal(SIGUSR1, sig_handler);
        signal(SIGUSR2, sig_handler);        
        
        do {
            std::map<int,child_t>::iterator c_it;
            
            for (c_it = childrens.begin(); c_it != childrens.end(); c_it++) {
                wdata_t m;
                int c_pid = (*c_it).first;
                child_t c = (*c_it).second;
                
                c.cipc.readData(m);
                
                std::cout << "pid: " << c_pid << " ts: " << m.timestamp << std::endl;
                sleep(1);
            }
            
            sleep(5);
        } while (!able_to_die);

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
        std::cout << "Got signal" << s << std::endl;
        exit(1);
    }
    
}

using namespace fp;

int main(int argc, char *argv[]){   
    fastPy fps;    
    return fps.go(argc, argv);
}
