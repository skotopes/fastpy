/*
 *  fp_worker.h
 *  fastPy
 *
 *  Created by Alexandr Kutuzov on 02.09.09.
 *  Copyright 2009 White-label ltd. All rights reserved.
 *
 */


#ifndef FP_WORKER_H
#define FP_WORKER_H

// posix threads and vector
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <vector>

// core components
#include "fp_log.h"
#include "fp_ipc.h"
#include "fp_config.h"
#include "fp_fastcgi.h"
#include "fp_handler.h"

namespace fp {    
    
    // worker class 
    class worker: public log, public config {
    public:
        worker(fastcgi *fc);
        ~worker();
    
        int startWorker();
        int waitWorker();
        
    private:
        static pyengine *py;
        static fastcgi *fcgi;
        static int wpid;
        static ipc wipc;

        static bool working;
        static bool terminating;
        static int threads_busy;
        static int threads_total;
        
        static pthread_mutex_t serialize_mutex;
        static pthread_t scheduler_thread;
        
        pthread_attr_t attr;
        std::vector<pthread_t> threads;
        
        int acceptor();
        int scheduler();
        
        static void *workerThread(void *data);
        static void *schedulerThread(void *data);
        static void sigHandler(int sig_type);
    };
}

#endif
