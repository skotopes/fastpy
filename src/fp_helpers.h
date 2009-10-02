/*
 *  fp_helpers.h
 *  fastPy
 *
 *  Created by Alexandr Kutuzov on 30.09.09.
 *  Copyright 2009 White-label ltd. All rights reserved.
 *
 */
#ifndef FP_HELPERS_H
#define FP_HELPERS_H

#include <iostream>
#include <sstream>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

namespace fp {    
    // standart helpers
    void trim(std::string& str);
    int toInt(std::string str);
    bool toBool(std::string str);
    
    int ts_cout(std::string str);
    int ts_cerr(std::string str);

}

#endif
