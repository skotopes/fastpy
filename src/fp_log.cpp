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
    bool log::log_opened = false;
    
    log::log() {
        
    }
    
    log::~log() {
        
    }
    
    int log::logError(std::string s) {
        std::cout << s << std::endl;
        return 0;
    }
}
