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
        pyengine *py;
        fastcgi *fcgi;
        
        pthread_mutex_t accept_mutex;
        pthread_attr_t attr;
        pthread_t scheduler_thread;
        std::vector<pthread_t> threads;
        
        static bool able_to_work;
        static bool able_to_die;

        static int t_count;
        static int wpid;
        static int csig;
        static ipc wipc;

        int acceptor();
        int scheduler();
        
        static void *workerThread(void *data);
        static void *schedulerThread(void *data);
        static void sigHandler(int sig_type);
    };
};

#endif
