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

#include <iostream>
#include "fp_core.h"

namespace fp {
    class log: public core {
        
    public:
        log();
        ~log();

        int logError(std::string s);
        int openLogs(std::string a_log, std::string e_log);
        
    private:
        static bool log_opened;
    };
}

#endif
