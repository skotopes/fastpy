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
            // forking and working
            for (int i=0; i < cnf.workers_cnt; i++) {
                createChild();
            }
        } else {
            worker w(fcgi);
            w.startWorker();
            w.waitWorker();            
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
            
        } else {
            return -1;
        }
        
        return 0;
    }
    
    int fastPy::yesMaster() {
        bool do_exit = false;
        
        // registering signal handler
        signal(SIGHUP, sig_handler);
        signal(SIGINT, sig_handler);
        signal(SIGTERM, sig_handler);
        signal(SIGUSR1, sig_handler);
        signal(SIGUSR2, sig_handler);        
        
        while (!do_exit) {
            
        }
        
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
