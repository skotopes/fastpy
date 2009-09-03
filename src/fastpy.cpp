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
        
        if (cnf.read(config_f) < 0) {
            return 253;
        }
        
        int ec = runFPy();

        return ec;
    }
    
    int fastJs::runFPy() {
        
        worker w(sock_f);
        
        w.startWorker();
        w.waitWorker();
        
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
