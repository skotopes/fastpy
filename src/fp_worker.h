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
#include <vector>
// core components
#include "fp_log.h"
#include "fp_config.h"
#include "fp_fastcgi.h"
#include "fp_handler.h"

namespace fp {    
        
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
        std::vector<pthread_t> threads;
        static bool able_to_work;

        int acceptor();
        
        static void *workerThread(void *data);
        static void sigHandler(int sig_type);
    };
}

#endif
