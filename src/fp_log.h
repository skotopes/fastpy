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
#include <iostream>
#include <fstream>

#include "fp_core.h"

namespace fp {
    
    enum logerr_e {
        LOG_SKIP,
        LOG_DEBUG,
        LOG_WARN,
        LOG_ERROR
    };
    
    class log: public core {        
    public:
        log();
        ~log();

        int openLogs(std::string a_log, std::string e_log);
        
        int logError(std::string s);
        int logError(std::string mod, logerr_e level, std::string s);
        
    private:
        static pthread_mutex_t log_mutex;
        static std::ofstream access_log_f;
        static std::ofstream error_log_f;
    };
}

#endif
