/*
 *  fp_core.cpp
 *  fastJs
 *
 *  Created by Alexandr Kutuzov on 02.09.09.
 *  Copyright 2009 White-label ltd. All rights reserved.
 *
 */

#include "fp_core.h"

namespace fp {
    bool core::verbose = false;
    bool core::debug = false;
    
    void core::trim(std::string& str) {
        size_t startpos = str.find_first_not_of(" \t");
        size_t endpos = str.find_last_not_of(" \t");
        
        if(( std::string::npos == startpos ) || ( std::string::npos == endpos)) {
            str ="";
        } else {
            str = str.substr( startpos, endpos-startpos+1 );
        }
    }
    
    void core::printLine(std::string str, ...) {
        std::stringstream ss;
        
        ss << str << std::endl;

        std::cout << ss.str();
    }
    
    int core::toInt(std::string str) {
        return atoi(str.c_str());
    }
}
