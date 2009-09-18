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

    worker::worker(fastcgi *fc) {
        fcgi = fc;
        
        // 1 worker - 1 python interpritator
        py = new pyengine();
        
        // init joinable threads
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

        signal(SIGHUP, sigHandler);
        signal(SIGINT, sigHandler);
        signal(SIGABRT, sigHandler);
        signal(SIGTERM, sigHandler);
    }
    
    worker::~worker() {
        
    }
    
    int worker::acceptor() {
        FCGX_Request *r;
        
        // Init request and handler        
        r = new FCGX_Request;
        fcgi->initRequest(r);
        
        handler h(fcgi, r);
        
        // run loop
        while (able_to_work) {
            // accepting connection
            int rc = fcgi->acceptRequest(r);
            
            // in case if some shit happens
            if (rc < 0) {
                continue;
                //break;
            }
            
            // procced request with handler
            h.proceedRequest();
        }
        
        delete &h;
        
        // TODO check out condition
        return 0;
    }
    
    int worker::startWorker() {
        // init types
        if (py->typeInit() < 0) {
            std::cout << "type initialization error\r\n";
            return -1;
        }
        
        if (py->setPath() < 0) {
            std::cout << "path change error\r\n";
            return -1;
        } 
        
        // calculate how much threads we need and reserv space for all of them
        int wnt = cnf.max_conn / cnf.workers_cnt;
        threads.reserve(wnt);

        for (int i=0;i < wnt;i++) {
            pthread_t thread;
            
            int ec = pthread_create(&thread, &attr, worker::workerThread, (void*)this);
            
            if (ec) {
                return -1;
            }
            
            threads.push_back(thread);            
        }
        
        return 0;
    }
    
    int worker::waitWorker() {
        std::vector<pthread_t>::iterator t_it;
        
        for (t_it = threads.begin(); t_it < threads.end(); t_it++) {
            pthread_join((*t_it), NULL);
        }

        py->releaseCallback();
        
        return 0;
    }
        
    void *worker::workerThread(void *data) {
        worker *w = (worker*) data;
        w->acceptor();
        pthread_exit(NULL);
    }
    
    void worker::sigHandler(int sig_type) {
        //able_to_work = false;
        std::cout << "Got signal, probably i should stop: "<< sig_type << std::endl;
                
        exit(1);
    }
}
