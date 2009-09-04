/*
 *  fp_worker.h
 *  fastJs
 *
 *  Created by Alexandr Kutuzov on 02.09.09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */


#ifndef FP_WORKER_H
#define FP_WORKER_H

// posix threads and vector
#include <pthread.h>
#include <vector>
// Python
#include <Python.h>
// core components
#include "fp_log.h"
#include "fp_config.h"
#include "fp_fastcgi.h"

namespace fp {    
    struct handler_t {
        bool is_ok;
        PyObject *pScript;
        PyObject *pModule;
        PyObject *pFunc;
    };
    
    class worker: public log, public config {
    public:
        worker(fastcgi &fc);
        ~worker();
    
        int startWorker();
        int waitWorker();
        
    private:
        fastcgi *fcgi;
        pthread_attr_t attr;
        std::vector<pthread_t> threads;
        PyThreadState *mainThreadState;
        vhost_t def_vhost;

        int acceptor();
        int initHandler(handler_t &h);
        int runHandler(handler_t &h, std::string &output);
        int releaseHandler(handler_t &h);
        
        static void *workerThread(void *data);
    };
}

#endif
