/*
 *  fp_fastcgi.h
 *  fastPy
 *
 *  Created by Alexandr Kutuzov on 04.09.09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef FP_FASTCGI_H
#define FP_FASTCGI_H

#include <pthread.h>
#include <fcgiapp.h>
#include <fcgio.h>

#include "fp_config.h"

namespace fp {

    class fastcgi: public config {
    public:
        fastcgi();
        ~fastcgi();
        
        int openSock(char *socket);
        int initRequest(FCGX_Request &request);
        int acceptRequest(FCGX_Request &request);
        int sendResponse(FCGX_Request &request);
        int finishRequest(FCGX_Request &request);
        
    private:
        static int fd;
        static bool inited;
        static pthread_mutex_t accept_mutex;

    };
}

#endif
