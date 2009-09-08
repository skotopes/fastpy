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
    
    struct module_t {
        PyObject *pModule;
        PyObject *pFunc;
    };

    struct start_response_t {
        PyObject_HEAD
        FCGX_Request *r;
        fastcgi *f;
    };
    
    class handler: public config {
    public:
        handler(fastcgi *f);
        ~handler();
        
        int createThreadState(thread_t &t);
        int proceedRequest(thread_t &t, FCGX_Request &r);
        int deleteThreadState(thread_t &t);
        
    private:
        fastcgi *fcgi;
        PyThreadState *mainThreadState;
        vhost_t v;

        int initModule(module_t &m);
        int runModule(module_t &m, FCGX_Request &r);
        int releaseModule(module_t &m);
        
        int setPyEnv();
        
        int initArgs(PyObject *dict);
        int releaseArgs(PyObject *dict);
        
    };    
    
}

#endif
