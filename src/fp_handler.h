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

    // helper structures 
    struct thread_t {
        PyInterpreterState * mainInterpreterState;
        PyThreadState * workerThreadState;
        long tc_number;
        bool in_use;
    };
    
    struct module_t {
        PyObject *pModule;
        PyObject *pFunc;
    };
    
    // python engine and manipulators only
    class pyengine: config {
    public:
        pyengine();
        ~pyengine();

        int createThreadState(thread_t &t); // init py ts
        int switchAndLockTC(thread_t &t); // begin exec
        int nullAndUnlockTC(thread_t &t); // end exec
        int deleteThreadState(thread_t &t); // remove py ts
        
    private:
        static PyThreadState *mainThreadState;
        static long tc_allocated;
    };
    
    // handler and pyprocessing
    class handler: public config {
    public:
        handler(fastcgi *f, FCGX_Request *r);
        ~handler();

        int proceedRequest();
        
    private:
        pyengine *py;
        fastcgi *fcgi;
        FCGX_Request *req;
        
        vhost_t v;
        thread_t t;
        
        static bool inited;

        int initModule(module_t &m);
        int runModule(module_t &m);
        int releaseModule(module_t &m);
        
        int setPyEnv();
        
        int initArgs(PyObject *dict);
        int releaseArgs(PyObject *dict);
        
    };    
    
}

#endif
