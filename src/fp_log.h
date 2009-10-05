/*
 *  fp_log.h
 *  fastPy
 *
 *  Created by Alexandr Kutuzov on 02.09.09.
 *  Copyright 2009 White-label ltd. All rights reserved.
 *
 */

#ifndef FP_LOG_H
#define FP_LOG_H

#include <pthread.h>
#include <sys/types.h>
#include <stdarg.h>
#include <iostream>
#include <sstream>
#include <fstream>

#include "fp_config.h"
#include "fp_helpers.h"

namespace fp {
    
    enum logerr_e {
        LOG_SKIP,
        LOG_DEBUG,
        LOG_WARN,
        LOG_ERROR
    };
    
    class log {
    public:
        log();
        virtual ~log();

        int openLogs(std::string a_log, std::string e_log);
        
        static int logAccess(std::string mod, std::string s);
        static int logError(std::string mod, logerr_e level, std::string s, ...);
        
    private:
        static pid_t pid;
        static pthread_mutex_t log_mutex;
        static std::ofstream a_log_f;
        static std::ofstream e_log_f;
    };
}

#endif
