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
    
    log::log() {
        
    }
    
    log::~log() {
        
    }
    
    int log::logError(std::string s) {
        std::cout << s << std::endl;
        return 0;
    }
}
