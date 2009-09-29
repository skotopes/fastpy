/*
 *  fastjs.h
 *  fastPy
 *
 *  Created by Alexandr Kutuzov on 02.09.09.
 *  Copyright 2009 White-label ltd. All rights reserved.
 *
 */

#ifndef FASTPY_H
#define FASTPY_H

#include <iostream>
#include <map>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>

#include "fp_log.h"
#include "fp_ipc.h"
#include "fp_config.h"
#include "fp_fastcgi.h"
#include "fp_worker.h"

namespace fp {
    
    struct child_t {
        ipc cipc;
        time_t timestamp;
    };
    
    class fastPy: config, log {
    public: 
        fastPy();
        ~fastPy();
        
        int go(int argc, char **argv);
        
    private:
        char *config_f;
        char *sock_f;
        bool detach;
        std::map<int,child_t> childrens;
        
        fastcgi *fcgi;
        
 
        int runFPy();
        int createChild();
        int yesMaster();
        
        void usage();
        static void sig_handler(int s);
    };
    
}

#endif
