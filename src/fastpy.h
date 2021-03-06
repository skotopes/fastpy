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
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <grp.h>

#include "fp_log.h"
#include "fp_ipc.h"
#include "fp_config.h"
#include "fp_fastcgi.h"
#include "fp_worker.h"

namespace fp {
    
    struct child_t {
        ipc_shm cipc;
        bool dead;
    };
    
    class fastPy: public config, public log {
    public: 
        fastPy();
        ~fastPy();
        
        int go(int argc, char **argv);
        
    private:
        fastcgi *fcgi;
        char *config_f;
        char *sock_f;
        static int msig;
        
        std::map<pid_t,child_t> childrens;

        int startChild();
        int masterLoop();
        int detachProc();
        int changeID();
        
        void usage();
        static void sig_handler(int s);
    };
    
}

#endif
