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
    class worker: public log, public config {

    public:
        worker(fastcgi &fc);
        ~worker();
    
        int acceptor();
        int startWorker();
        int waitWorker();
        
        bool acceptLock();
        bool acceptUnlock();
        
    private:
        fastcgi *fcgi;
        pthread_attr_t attr;
        std::vector<pthread_t> threads;

        PyThreadState *mainThreadState;
        
        static void *workerThread(void *data);
    };
}

#endif
