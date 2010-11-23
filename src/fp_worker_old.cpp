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

    // static members 
    pyengine *worker::py        = NULL;
    fastcgi *worker::fcgi       = NULL;
    int worker::wpid            = 0;
    ipc_shm worker::wipc;

    bool worker::working        = true;
    bool worker::terminating    = true;
    int worker::threads_busy    = 0;
    int worker::threads_total   = 0;

    pthread_mutex_t worker::serialize_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_t worker::scheduler_thread;
    
    /*!
        @method     worker::worker(fastcgi *fc)
        @abstract   class consrtuctor
        @discussion 
    */

    worker::worker(fastcgi *fc) {
        fcgi = fc;

        if (conf.threads_cnt == 0) {
            py = new pyengine(false);
        } else {
            py = new pyengine(true);    
        }
        
        wpid = getpid();
        // init joinable threads
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

        signal(SIGHUP, sigHandler);
        signal(SIGINT, sigHandler);
        signal(SIGABRT, sigHandler);
        signal(SIGTERM, sigHandler);
    }

    /*!
        @method     worker::~worker()
        @abstract   class destructor
        @discussion class destructor, do nothing
    */

    worker::~worker() {
    }
    
    int worker::startWorker() {
        int ec;
        // initializing context
        logError("worker", LOG_DEBUG, "service initing shared memory");

        if ((ec = wipc.initSHM(wpid)) < 0) {
            logError("worker", LOG_ERROR, "unable to attach to shm zone, error code: %d", ec);
            return -1;
        }
        
        logError("worker", LOG_DEBUG, "initing python types");

        if ((ec = py->typeInit()) < 0) {
            logError("worker", LOG_ERROR, "python type initialization error: %d", ec);
            return -1;
        }
        
        logError("worker", LOG_DEBUG, "initing python path");
        
        if ((ec = py->setPath()) < 0) {
            logError("worker", LOG_ERROR, "python path change error: %d", ec);
            return -1;
        } 

        logError("worker", LOG_DEBUG, "initing python callback");
        
        if ((ec = py->initModule()) < 0) {
            logError("worker", LOG_ERROR, "python callback initialization error: %d", ec);
            return -1;
        }

        logError("worker", LOG_DEBUG, "environment initialization finished");
        
        if (conf.threads_cnt > 0) {
            logError("worker", LOG_DEBUG, "starting threads");
            threads.reserve(conf.threads_cnt);
            
            // starting worker threads
            for (int i=0;i < conf.threads_cnt;i++) {
                pthread_t thread;
                
                ec = pthread_create(&thread, &attr, worker::workerThread, (void*)this);
                
                if (ec) {
                    return -2;
                }

                threads_total++;
                threads.push_back(thread);
            }

            ec = pthread_create(&scheduler_thread, &attr, worker::schedulerThread, (void*)this);
            
            if (ec) {
                return -3;
            }            
            
            logError("worker", LOG_DEBUG, "threads started");
        } else if (conf.threads_cnt == 0) {
            logError("worker", LOG_WARN, "thread count = 0, it means that multi-thread logic will be switched off");
            pthread_t thread;
            
            ec = pthread_create(&thread, &attr, worker::workerThread, (void*)this);
            
            if (ec) {
                return -2;
            }
            
            threads_total++;
            threads.push_back(thread);            

            ec = pthread_create(&scheduler_thread, &attr, worker::schedulerThread, (void*)this);
            
            if (ec) {
                return -3;
            }            
            
            logError("worker", LOG_DEBUG, "threads started");
        } else {
            logError("worker", LOG_ERROR, "thread count cannot be less then 0, can`t continue");
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
            threads_total--;
        }

        logError("worker", LOG_DEBUG, "threads finished, releasing callback");
        
        delete py;

        // Let`s inform master about our death
        wipc.shm->w_code = W_TERM;
        wipc.freeSHM();
        
        logError("worker", LOG_WARN, "worker process is finished");

        return 0;
    }

    /*!
     @method     int worker::nowaitWorker()
     @abstract   terminate withou wait 
     @discussion small ceanup and terminate
     */
    
    int worker::nowaitWorker() {
        if (wipc.shm != NULL) {
            wipc.shm->w_code = W_FAIL;
            wipc.freeSHM();   
        }
        
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
        
        // Init request
        r = new FCGX_Request;
        fcgi->initRequest(r);
        
        // Init handler
        h = new handler(fcgi, r);
        
        // accept loop
        while (working) {
            int f_rc=0, h_rc=0;
            
            pthread_mutex_lock(&serialize_mutex);
            
            if (!working) {
                pthread_mutex_unlock(&serialize_mutex);
                break;
            }
                
            // accepting connection
            f_rc = fcgi->acceptRequest(r);

            threads_busy++;
            
            pthread_mutex_unlock(&serialize_mutex);
            
            // in case if some shit happens
            if (f_rc < 0) {
                logError("acceptor", LOG_ERROR, "unable to accept, will try to continue error code: %d", f_rc);
                continue;
            }
            
            // procced request with handler
            h_rc = h->proceedRequest();
            
            if (h_rc < 0) {
                logError("acceptor", LOG_ERROR, "error while proceeding request: %d", h_rc);
            }
            
            threads_busy--;
        }
                
        // deallocating handler
        delete h;
        
        return 0;
    }

    int worker::scheduler() {

        wipc.shm->w_code = W_FINE;
        
        while (working) {
            
            wipc.shm->timestamp = time(NULL);
            
            switch (wipc.shm->m_code) {
                case M_TERM:
                    working = false;
                    break;
                default:
                    break;
            }
            
            
            usleep(250000);
        }
        
        int timeout = time(NULL) + 5;
        
        while (terminating) {
            wipc.shm->timestamp = time(NULL);
            
            if (timeout < time(NULL)) {
                wipc.shm->w_code = W_TERM;
                wipc.freeSHM();
                
                logError("scheduler", LOG_ERROR, "worker terminated by time out");
                exit(1);
            }
            
            
            if (threads_total == 0) {
                terminating = false; 
            }
            
            usleep(10000);
        }
        
        return 0;
    }
    
    /*!
        @method     void *worker::workerThread(void *data)
        @abstract   run worker thread
        @discussion static callback for pthread_create, worker thread only
    */

    void *worker::workerThread(void *data) {
        worker *w = (worker*)data;
        w->acceptor();
        pthread_exit(NULL);
    }
    
    /*!
     @method     void *worker::schedulerThread(void *data)
     @abstract   run scheduler thread
     @discussion 
     */
    
    void *worker::schedulerThread(void *data) {
        worker *w = (worker*)data;
        w->scheduler();
        pthread_exit(NULL);
    }
    
    /*!
        @method     void worker::sigHandler(int sig_type)
        @abstract   signal handler
        @discussion worker signal handler
    */

    void worker::sigHandler(int sig_type) {
        switch (sig_type) {
            case SIGABRT:
                // die here
                logError("worker", LOG_WARN, "got SIGABRT, emergency exit");
                
                wipc.shm->w_code = W_TERM;
                wipc.shm->signal = sig_type;
                
                exit(255);
                break;
            default:
                logError("worker", LOG_WARN, "got signal: %d, but i shouldn`t. passing signal to master", sig_type);
                wipc.shm->signal = sig_type;
                
                break;
        }
    }
}
