/*
 *  fp_fastcgi.h
 *  fastPy
 *
 *  Created by Alexandr Kutuzov on 04.09.09.
 *  Copyright 2009 White-label ltd. All rights reserved.
 *
 */

#ifndef FP_FASTCGI_H
#define FP_FASTCGI_H

#include <pthread.h>
#include <fcgiapp.h>
#include <fcgio.h>
#include <string.h>

#include "fp_config.h"

namespace fp {

    class fastcgi: public config {
    public:
        fastcgi();
        ~fastcgi();
        
        int openSock(char *socket);
        int initRequest(FCGX_Request *request);
        int acceptRequest(FCGX_Request *request);
        int writeResponse(FCGX_Request *request, std::string output);
        int writeResponse(FCGX_Request *request, char* output);
        int finishRequest(FCGX_Request *request);
        
        int error500(FCGX_Request *request, std::string output);
        int error418(FCGX_Request *request, std::string output);
        
    private:
        static int fd;
        static bool inited;
    };
}

#endif
