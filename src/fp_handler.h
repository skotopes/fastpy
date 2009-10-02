/*
 *  fp_handler.h
 *  fastPy
 *
 *  Created by Alexandr Kutuzov on 06.09.09.
 *  Copyright 2009 White-label ltd. All rights reserved.
 *
 */

#ifndef FP_HANDLER_H
#define FP_HANDLER_H

#include <Python.h>
#include <structmember.h>
#include <sstream>

#include "fp_config.h"
#include "fp_fastcgi.h"

/* 
 * fast stream code based on Cody Pisto python-fastcgi
 * mostly it`s old python file object code, 
 * TODO: MUST BE REPLACED IN FUTURE
 */

#define BUF(v) PyString_AS_STRING((PyStringObject *)v)

#if BUFSIZ < 8192
    #define SMALLCHUNK 8192
#else
    #define SMALLCHUNK BUFSIZ
#endif

#if SIZEOF_INT < 4
    #define BIGCHUNK  (512 * 32)
#else
    #define BIGCHUNK  (512 * 1024)
#endif

#define fcgi_Stream_zero(_s) \
    if (_s != NULL) { \
    _s->s = NULL; \
}

#define fcgi_Stream_Check() \
    if (self->s == NULL || *(self->s) == NULL) { \
        PyErr_SetString(PyExc_ValueError, "I/O operation on closed file"); \
    return NULL; \
}

namespace fp {

    /* ================================ python */
    
    struct thread_t {
        PyInterpreterState * mainInterpreterState;
        PyThreadState * workerThreadState;
        long tc_number;
        bool in_use;
    };
    
    struct headers_t {
        std::string status_code;
        std::string headers_set;
        bool is_filled;
        bool is_sended;
    };
    
    struct StartResponseObject {
        PyObject_HEAD
        headers_t *h;
    };    

    struct FastStreamObject {
        PyObject_HEAD
        FCGX_Stream **s;
    };    
    
    // python engine and manipulators only
    class pyengine: config {
    public:
        pyengine();
        ~pyengine();

        // thread state switchers
        int createThreadState(thread_t &t);
        int switchAndLockTC(thread_t &t);
        int serviceLockTC();
        int serviceUnlockTC();
        int nullAndUnlockTC(thread_t &t);
        int deleteThreadState(thread_t &t);
        
        // helpers
        int typeInit();
        int setPath();
        
        // wsgi callback module routines
        int initCallback();
        int reloadCallback();
        inline bool isCallbackReady() {return cbr_flag;};
        inline PyObject *getCallback() {return pFunc;};
        int releaseCallback();

        // start response object routines
        StartResponseObject *newSRObject();

        // fast stream object routines
        FastStreamObject *newFSObject();
        
    private:
        static PyThreadState *mainThreadState;
        static PyThreadState *serviceThreadState;
        static long tc_allocated;

        // wsgi call back
        static bool cbr_flag;
        static PyObject *pModule;
        static PyObject *pFunc;
        
        // start_response
        static PyTypeObject StartResponseType;
        static PyObject *startResponse(PyObject *self, PyObject *args, PyObject *kw);
        
        // fast_stream
        static PyTypeObject FastStreamType;
        static PyMemberDef FastStreamMembers[];
        static PyMethodDef FastStreamMethods[];
        static size_t new_buffersize(size_t currentsize);
        static PyObject *fcgi_Stream_read(FastStreamObject *self, PyObject *args);
        static PyObject *get_line(FastStreamObject *self, long bytesrequested);
        static PyObject *fcgi_Stream_readline(FastStreamObject *self, PyObject *args);
        static PyObject *fcgi_Stream_readlines(FastStreamObject *self, PyObject *args);
        static PyObject *fcgi_Stream_getiter(FastStreamObject *self);
        static PyObject *fcgi_Stream_iternext(FastStreamObject *self);
        static PyObject *fcgi_Stream_write(FastStreamObject *self, PyObject *args);
        static PyObject *fcgi_Stream_writelines(FastStreamObject *self, PyObject *seq);
        static PyObject *fcgi_Stream_flush(FastStreamObject *self);
        static void fcgi_Stream_dealloc(FastStreamObject *self);
        static PyObject *fcgi_Stream_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
    };
    
    /* ================================ handler */

    class handler: public config {
    public:
        handler(fastcgi *f, FCGX_Request *r);
        virtual ~handler();

        int proceedRequest();
        
    private:
        pyengine *py;
        fastcgi *fcgi;
        FCGX_Request *req;
        
        thread_t t;
        StartResponseObject *pSro;
        FastStreamObject *pInput;
        FastStreamObject *pErrors;
        PyObject *pCallback;
        
        int runModule();

        int sendHeaders(headers_t &h);
        int initArgs(PyObject *dict);
        int releaseArgs(PyObject *dict);
        
    };    
    
}

#endif
