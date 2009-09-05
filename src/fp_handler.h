/*
 *  fp_handler.h
 *  fastPy
 *
 *  Created by Alexandr Kutuzov on 06.09.09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef FP_HANDLER_H
#define FP_HANDLER_H

#include <Python.h>

#include "fp_config.h"
#include "fp_fastcgi.h"

namespace fp {

    struct thread_t {
        PyInterpreterState * mainInterpreterState;
        PyThreadState * workerThreadState;
        
    };
    
    struct handler_t {
        bool is_ok;
        PyObject *pScript;
        PyObject *pModule;
        PyObject *pFunc;
    };
    
    class handler: public config {
    public:
        handler(fastcgi *f);
        ~handler();
        
        int createThreadState(thread_t &t);
        int deleteThreadState(thread_t &t);
        
        int proceedRequest(thread_t &t, FCGX_Request &r, vhost_t &v);
        
    private:
        fastcgi *fcgi;
        PyThreadState *mainThreadState;

        int initHandler(handler_t &h, vhost_t &v);
        int runHandler(handler_t &h, std::string &output);
        int releaseHandler(handler_t &h);
        
        
        int initEnviron(PyObject *dict);
        int releaseEnviron(PyObject *dict);
        
    };    
    
}

#endif
