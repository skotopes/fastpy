/*
 *  fastjs.cpp
 *  fastJs
 *
 *  Created by Alexandr Kutuzov on 02.09.09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "fastpy.h"

namespace fp {
    
    fastJs::fastJs() {
        config_f = NULL;
        sock_f = NULL;
    }
    
    fastJs::~fastJs() {
        
    }
    
    int fastJs::go(int argc, char **argv) {
        int c;
        while ((c = getopt (argc, argv, "hvc:s:")) != -1) {
            switch (c) {
                case 'v':
                    if (verbose) {
                        debug = true;
                    }
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
    
    int fastJs::runFPy() {
        // opening socket
        if (fcgi.openSock(sock_f) < 0) {
            return 250;
        }

        // forking and working
        for (int i=0; i < cnf.workers_cnt; i++) {
            createChild();
        }
        
        return 0;
    }
    

    int fastJs::createChild() {
        int fpid = fork();
        
        if (fpid == 0) {
            // some where in kenya(it`s our childrens)
            worker w(fcgi);
            
            w.startWorker();
            w.waitWorker();
            
            exit(0);                
        } else if (fpid > 0){
            // lions and tigers
            // simplu returning back
        } else {
            return -1;
        }
        
        return 0;
    }
    
    int fastJs::yesMaster() {
        
        while (true) {
            
        }
        
        return 0;
    }
    
    void fastJs::usage() {
        std::cout << "Usage:"<< std::endl
            << "fastjs OPTIONS"<< std::endl
            << std::endl
            << "Options:"<< std::endl
            << "-s [unix_socket]"<< std::endl
            << "-c [config_file]"<< std::endl
            << "-h \t\t- help"<< std::endl
            << "-v \t\t- verbose output(more v - more verbose)"<< std::endl;
    }
}

using namespace fp;

int main(int argc, char *argv[]){   
    fastJs fps;    
    return fps.go(argc, argv);
}
