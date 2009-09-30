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
#include <vector>

// core components
#include "fp_log.h"
#include "fp_ipc.h"
#include "fp_config.h"
#include "fp_fastcgi.h"
#include "fp_handler.h"

namespace fp {    
    // worker state code 
    enum w_code_e {
        W_NRDY,     // worker not yet ready
        W_FINE,     // worker ready
        W_IRLD,     // worker in reload now
        W_BUSY,     // worker is busy and unable to serv conn
        W_FAIL,     // worker failure, probably we must terminate
        W_TERM      // worker terminated (alredy should be dead)
    };
    
    // master state code
    enum m_code_e {
        M_NRDY,     // master not yet ready
        M_FINE,     // master ready
        M_SKIP,     // master wants worker to skip connections 
        M_DRLD,     // master wants worker to reload code
        M_TERM      // master wants worker to terminate
    };
    
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
