/*
 *  fastjs.h
 *  fastJs
 *
 *  Created by Alexandr Kutuzov on 02.09.09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef FASTPY_H
#define FASTPY_H

#include <iostream>
#include <unistd.h>
#include "fp_config.h"
#include "fp_log.h"
#include "fp_fastcgi.h"
#include "fp_worker.h"

namespace fp {
    
    class fastJs: config, log {
    public: 
        fastJs();
        ~fastJs();
        
        int go(int argc, char **argv);
        
    private:
        char *config_f;
        char *sock_f;
        bool detach;

        fastcgi *fcgi;

        int runFPy();
        int createChild();
        int yesMaster();
        
        void usage();
    };
    
}

#endif
