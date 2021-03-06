/*
 *  fp_fastcgi.cpp
 *  fastPy
 *
 *  Created by Alexandr Kutuzov on 04.09.09.
 *  Copyright 2009 White-label ltd. All rights reserved.
 *
 */

#include "fp_fastcgi.h"

namespace fp {
    // some static stuff
    int fastcgi::fd;

    fastcgi::fastcgi() {
        FCGX_Init();
    }
    
    fastcgi::~fastcgi() {
    }
    
    int fastcgi::openSock(char *socket) {
        fd = FCGX_OpenSocket(socket, 5000);

        if (fd < 0) {
            return -1;
        }
        
        return 0;
    }
    
    int fastcgi::initRequest(FCGX_Request *request) {

        FCGX_InitRequest(request, fd, 0);

        return 0;
    }

    int fastcgi::acceptRequest(FCGX_Request *request) {
        int rc = FCGX_Accept_r(request);
        
        return rc;
    }

    int fastcgi::error500(FCGX_Request *request, std::string output) {
        FCGX_FPrintF(request->out,
                     "Status: 500 Internal Server Error\r\n"
                     "Content-type: text/html\r\n"
                     "\r\n"
                     "<title>Internal server error</title>"
                     "<h1>%s</h1>", output.c_str());
        return 0;
    }

    int fastcgi::error418(FCGX_Request *request, std::string output) {
        FCGX_FPrintF(request->out,
                     "Status: 418 I'm a teapot\r\n"
                     "Content-type: text/html\r\n"
                     "\r\n"
                     "<title>Cofee machine not ready</title>"
                     "<h1>%s</h1>", output.c_str());
        return 0;
    }
    
    int fastcgi::writeResponse(FCGX_Request *request, char* output) {
        FCGX_PutStr(output, strlen(output), request->out);
        return 0;
    }        
    
    int fastcgi::finishRequest(FCGX_Request *request) {
        FCGX_Finish_r(request);
        return 0;
    }

}
