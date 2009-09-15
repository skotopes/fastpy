/*
 *  fp_fastcgi.cpp
 *  fastPy
 *
 *  Created by Alexandr Kutuzov on 04.09.09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "fp_fastcgi.h"

namespace fp {
    // some static stuff
    bool fastcgi::inited = false;
    int fastcgi::fd;

    fastcgi::fastcgi() {
        // initialize accept mutex 
        if (!inited) {
            // initialize fastcgi environment and open socket
            FCGX_Init();
        }
    }
    
    fastcgi::~fastcgi() {
    }
    
    int fastcgi::openSock(char *socket) {
        fd = FCGX_OpenSocket(socket, 5000);
        
        return 0;
    }

    int fastcgi::initAcceptMutex() {
        pthread_mutex_init(accept_mutex, NULL);
    }
    
    int fastcgi::initRequest(FCGX_Request *request) {

        FCGX_InitRequest(request, fd, 0);

        return 0;
    }

    int fastcgi::acceptRequest(FCGX_Request *request) {
        int rc;
        
        if (cnf.accept_mt) pthread_mutex_lock(accept_mutex);
        rc = FCGX_Accept_r(request);
        if (cnf.accept_mt) pthread_mutex_unlock(accept_mutex);
        
        return rc;
    }

    int fastcgi::error500(FCGX_Request *request, std::string output) {
        FCGX_FPrintF(request->out,
                     "Content-type: text/html\r\n"
                     "\r\n"
                     "<title>Internal server error</title>"
                     "<h1>%s</h1>", output.c_str());
        return 0;
    }    

    int fastcgi::writeResponse(FCGX_Request *request, char *output) {
        FCGX_PutStr(output, strlen(output), request->out);
        return 0;
    }        
    
    int fastcgi::finishRequest(FCGX_Request *request) {
        FCGX_Finish_r(request);
        return 0;
    }

}
