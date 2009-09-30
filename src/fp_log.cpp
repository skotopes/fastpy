/*
 *  fp_log.cpp
 *  fastPy
 *
 *  Created by Alexandr Kutuzov on 02.09.09.
 *  Copyright 2009 White-label ltd. All rights reserved.
 *
 */

#include "fp_log.h"

namespace fp {
    std::ofstream log::access_log_f;
    std::ofstream log::error_log_f;
    pthread_mutex_t log::log_mutex = PTHREAD_MUTEX_INITIALIZER;
    
    log::log() {
        pid = getpid();
    }
    
    log::~log() {
        
    }
    
    int log::logAccess(std::string mod, std::string s) {
        std::stringstream ss;
        
        time_t t = time(NULL);
        struct tm *dt = localtime(&t);
        char dtr[80];
        
        strftime (dtr,80,"[%d/%b/%Y:%X %Z] ",dt);
        ss << dtr << pid << ":"<< mod << " "<< s;
        
        ts_cout(ss.str());
        
        return 0;
    }
    
    int log::logError(std::string mod, logerr_e level, std::string s) {
        std::stringstream ss;
        
        time_t t = time(NULL);
        struct tm *dt = localtime(&t);
        char dtr[80];
        
        strftime (dtr,80,"[%d/%b/%Y:%X %Z] ",dt);
        ss << dtr << pid << ":"<< mod;
        
        if (level == LOG_DEBUG) {
            if (!config::debug) return 0;
            ss << " debug: ";
        } else if (level == LOG_WARN) {
            if (!config::verbose) return 0;
            ss << " warning: ";
        } else if (level == LOG_ERROR) {
            ss << " error: ";
        }
        
        ss << s;
        
        if (!config::detach) ts_cout(ss.str());
        
        return 0;
    }

}
