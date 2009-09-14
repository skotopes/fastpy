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
#include <sstream>

#include "fp_config.h"
#include "fp_fastcgi.h"

namespace fp {

    /* ================================ python */
    
    struct thread_t {
        PyInterpreterState * mainInterpreterState;
        PyThreadState * workerThreadState;
        long tc_number;
        bool in_use;
    };
        
    struct StartResponseObject {
        PyObject_HEAD
        FCGX_Request *r;
        fastcgi *f;
    };    
            
    // python engine and manipulators only
    class pyengine: config {
    public:
        pyengine();
        ~pyengine();

        // thread state switchers
        int createThreadState(thread_t &t);
        int switchAndLockTC(thread_t &t);
        int nullAndUnlockTC(thread_t &t);
        int deleteThreadState(thread_t &t);
        
        // helpers
        int setPath();
        
        // wsgi callback module routines
        int initCallback();
        inline bool isCallbackReady() {return cbr_flag;};
        inline PyObject *getCallback() {return pFunc;};
        int releaseCallback();

        // Additional pyobjects routines
        StartResponseObject *newSRObject();
        
    private:
        static PyThreadState *mainThreadState;
        static long tc_allocated;

        // wsgi call back
        static bool cbr_flag;
        static PyObject *pModule;
        static PyObject *pFunc;
        
        // start_response
        static PyTypeObject StartResponseType;
        static PyObject *startResponse(PyObject *self, PyObject *args, PyObject *kw);
    };
    
    /* ================================ handler */

    class handler: public config {
    public:
        handler(fastcgi *f, FCGX_Request *r);
        ~handler();

        int proceedRequest();
        
    private:
        pyengine *py;
        fastcgi *fcgi;
        FCGX_Request *req;
        
        thread_t t;
        StartResponseObject *sro;
        
        int runModule();
                
        int initArgs(PyObject *dict);
        int releaseArgs(PyObject *dict);
        
    };    
    
}

#endif
