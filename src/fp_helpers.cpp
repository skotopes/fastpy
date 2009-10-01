/*
 *  fp_helpers.cpp
 *  fastPy
 *
 *  Created by Alexandr Kutuzov on 30.09.09.
 *  Copyright 2009 White-label ltd. All rights reserved.
 *
 */

#include "fp_helpers.h"

namespace fp {

    void trim(std::string& str) {
        size_t startpos = str.find_first_not_of(" \t");
        size_t endpos = str.find_last_not_of(" \t");
        
        if(( std::string::npos == startpos ) || ( std::string::npos == endpos)) {
            str ="";
        } else {
            str = str.substr( startpos, endpos-startpos+1 );
        }
    }
    
    int toInt(std::string str) {
        return atoi(str.c_str());
    }
    
    bool toBool(std::string str) {
        if (str.compare("on") == 0) {
            return true;
        } else if (str.compare("true") == 0) {
            return true;
        }
        return false;
    }
    
    // thread safe cout
    static pthread_mutex_t cout_mutex = PTHREAD_MUTEX_INITIALIZER;
    static pthread_mutex_t cerr_mutex = PTHREAD_MUTEX_INITIALIZER;
    
    int ts_cout(std::string str) {
        pthread_mutex_lock(&cout_mutex);
        std::cout << str << std::endl;
        pthread_mutex_unlock(&cout_mutex);
        return 0;
    }

    int ts_cerr(std::string str) {
        pthread_mutex_lock(&cerr_mutex);
        std::cerr << str << std::endl;
        pthread_mutex_unlock(&cerr_mutex);
        return 0;
    }
    
}
