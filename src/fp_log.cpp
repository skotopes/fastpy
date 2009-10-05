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
    pid_t log::pid = 0;
    std::ofstream log::a_log_f;
    std::ofstream log::e_log_f;
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
    
    int log::logError(std::string mod, logerr_e level, std::string s, ...) {
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
        
        va_list ap;
        char *fmt = (char*)s.c_str();
        char *s_va; int d_va;
        
        va_start(ap, s);
        while (*fmt) {
            if (*fmt == '%') {
                switch(char ft = *++fmt) {
                    case 's':
                        s_va = va_arg(ap, char *);
                        ss << s_va;
                        break;
                    case 'd':
                        d_va = va_arg(ap, int);
                        ss << d_va;
                        break;
                    default:
                        ss << "<unknown type: %" << ft << ">";
                        break;
                }
            } else {
                ss << *fmt;
            }
            
            fmt++;
        }
        
        va_end(ap);

        if (!config::detach) ts_cout(ss.str());
        
        return 0;
    }

}
