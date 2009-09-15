/*
 *  fp_core.h
 *  fastJs
 *
 *  Created by Alexandr Kutuzov on 02.09.09.
 *  Copyright 2009 White-label ltd. All rights reserved.
 *
 */

#ifndef FP_CORE_H
#define FP_CORE_H

#include <iostream>
#include <sstream>
#include <stdlib.h>

namespace fp {
    class core {
    public:
        void trim(std::string& str);
        void printLine(std::string str, ...);
        int toInt(std::string str);
        
    private:
    protected:
        static bool verbose;
        static bool debug;
    };
}

#endif
