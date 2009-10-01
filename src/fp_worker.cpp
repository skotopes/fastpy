/*
 *  fp_worker.cpp
 *  fastPy
 *
 *  Created by Alexandr Kutuzov on 02.09.09.
 *  Copyright 2009 White-label ltd. All rights reserved.
 *
 */

#include "fp_worker.h"

namespace fp {
    
    bool worker::able_to_work = true;
    int worker::t_count = 0;
    int worker::wpid = 0;
    int worker::csig = 0;
    ipc worker::wipc;
    
    worker::worker(fastcgi *fc) {
        logError("worker", LOG_DEBUG, "starting worker");
        wpid = getpid();
        fcgi = fc;
        py = new pyengine();
        
        // init joinable threads
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        pthread_mutex_init(&accept_mutex, NULL);

        signal(SIGHUP, sigHandler);
        signal(SIGINT, sigHandler);
        signal(SIGABRT, sigHandler);
        signal(SIGTERM, sigHandler);
        logError("worker", LOG_DEBUG, "worker initialization compleate");        
    }
    
    worker::~worker() {

    }
    
    int worker::startWorker() {
        // init types
        logError("worker", LOG_DEBUG, "initing types and python environment");
        if (wipc.initSHM(wpid,true) < 0) {
            logError("worker", LOG_DEBUG, "unable to initialize shm, probably system limit exceeded");
            return -1;
        }
        
        if (py->typeInit() < 0) {
            logError("worker", LOG_ERROR, "type initialization error");
            return -1;
        }
        
        if (py->setPath() < 0) {
            logError("worker", LOG_ERROR, "path change error");
            return -1;
        } 
        
        if (py->initCallback() < 0) {
            logError("worker", LOG_ERROR, "callback initialization error");
            return -1;
        }              
        
        if (conf.threads_cnt > 0) {
            logError("worker", LOG_DEBUG, "starting threads");
            threads.reserve(conf.threads_cnt + 1);
            
            // starting worker threads
            for (int i=0;i < conf.threads_cnt;i++) {
                pthread_t thread;
                
                int ec = pthread_create(&thread, &attr, worker::workerThread, (void*)this);
                
                if (ec) {
                    return -2;
                }

                t_count++;
                threads.push_back(thread);
            }

            // starting schedule thread            
            int ec = pthread_create(&scheduler_thread, &attr, worker::schedulerThread, (void*)this);
            
            if (ec) {
                return -2;
            }
                        
            logError("worker", LOG_DEBUG, "threads started");
        } else {
            logError("worker", LOG_ERROR, "thread count less or equal to 0, can`t continue");
            return -3;
        }
        
        return 0;
    }
    
    /*!
        @method     int worker::waitWorker()
        @abstract   wait worker threads 
        @discussion wait worker threads and set shm w_state to W_TERM
    */

    int worker::waitWorker() {
        std::vector<pthread_t>::iterator t_it;
        
        for (t_it = threads.begin(); t_it < threads.end(); t_it++) {
            pthread_join((*t_it), NULL);
        }
        
        pthread_join(scheduler_thread, NULL);

        py->releaseCallback();

        // Let`s inform master about our death
        wipc.lock();
        wipc.shm->w_code = W_TERM;
        wipc.unlock();
        
        wipc.closeMQ();
        logError("worker", LOG_DEBUG, "worker process is finished");

        return 0;
    }

    /*!
        @method     int worker::acceptor()
        @abstract   acceptor routine
        @discussion acceptor routine: accept loop and some more stuff
    */

    int worker::acceptor() {
        FCGX_Request *r;
        handler *h;
        int rc=0, ec=0;
        
        // Init request and handler
        r = new FCGX_Request;
        fcgi->initRequest(r);
        h = new handler(fcgi, r);

        // accept loop
        while (able_to_work) {
            
            pthread_mutex_lock(&accept_mutex);
            
            if (!able_to_work) {
                pthread_mutex_unlock(&accept_mutex);
                break;
            }
            
            // accepting connection
            rc = fcgi->acceptRequest(r);
            
            pthread_mutex_unlock(&accept_mutex);
            
            // in case if some shit happens
            if (rc < 0) {
                logError("acceptor", LOG_ERROR, "unable to accept, will try to continue ec: " + rc);
                continue;
            }
            
            // procced request with handler
            ec = h->proceedRequest();
            
            if (ec < 0) {
                std::stringstream s;
                s << "error while proceeding request: " << ec;
                logError("acceptor", LOG_ERROR, s.str());
            }
        }
        
        delete h;
        
        return 0;
    }
    
    /*!
        @method     int worker::scheduler()
        @abstract   scheduller routine
        @discussion scheduller routine and main loop
    */

    int worker::scheduler() {
        bool able_to_die = false;

        // sched service loop (normal work) 
        do {
            wipc.lock();
            switch (wipc.shm->m_code) {
                case M_NRDY:
                    break;
                case M_FINE:
                    wipc.shm->signal = csig;
                    break;
                case M_SKIP:
                    break;
                case M_DRLD:
                    py->reloadCallback();
                    wipc.shm->m_code = M_FINE;
                    logError("worker", LOG_WARN, "reloading app code");
                    break;
                case M_TERM:
                    able_to_work = false;
                    break;
                default:
                    break;
            }
            wipc.unlock();
            
            usleep(250000);
        } while (able_to_work);

        // sched service loop (terminating everything)
        int to = 20;
        do {
            if (t_count == 0) able_to_die = true;
            
            if (to == 0) {
                wipc.lock();
                wipc.shm->w_code = W_TERM;
                wipc.unlock();
                
                wipc.closeMQ();
                logError("worker", LOG_DEBUG, "terminating by timeout");
                exit(0);
            }
            
            to--;

            usleep(100000);
        } while (!able_to_die);
        
        return 0;
    }
    
    /*!
        @method     void *worker::workerThread(void *data)
        @abstract   run worker thread
        @discussion static callback for pthread_create, worker thread only
    */

    void *worker::workerThread(void *data) {
        worker *w = (worker*) data;
        w->acceptor();
        t_count--;
        pthread_exit(NULL);
    }

    /*!
        @method     void *worker::schedulerThread(void *data)
        @abstract   run scheduller thread 
        @discussion static callback for pthread_create, scheduller thread only
    */

    void *worker::schedulerThread(void *data) {
        worker *w = (worker*) data;
        w->scheduler();
        pthread_exit(NULL);
    }
    
    /*!
        @method     void worker::sigHandler(int sig_type)
        @abstract   signal handler
        @discussion worker signal handler
    */

    void worker::sigHandler(int sig_type) {
        csig = sig_type;
    }
}
