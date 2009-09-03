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
// fastcgi
#include <fcgiapp.h>
#include <fcgio.h>
// Python
#include <Python.h>
// core components
#include "fp_log.h"
#include "fp_config.h"

namespace fp {
    class worker: public log, public config {

    public:
        worker(char *socket);
        ~worker();
    
        int acceptor();
        int startWorker();
        int waitWorker();
        
        bool acceptLock();
        bool acceptUnlock();
        
    private:
        int fd;
        pthread_attr_t attr;
        pthread_mutex_t accept_mutex;
        std::vector<pthread_t> threads;

        PyThreadState *mainThreadState;
        
        static void *workerThread(void *data);
    };
}

#endif
