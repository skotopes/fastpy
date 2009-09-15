/*
 *  fp_worker.cpp
 *  fastJs
 *
 *  Created by Alexandr Kutuzov on 02.09.09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "fp_worker.h"

namespace fp {
    
    worker::worker(fastcgi *fc) {
        fcgi = fc;
        
        py = new pyengine();
        
        // create joinable threads
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);    
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
        while (true) {
            // accepting connection
            int rc = fcgi->acceptRequest(r);
            
            // in case if some shit happens
            if (rc < 0)
                break;
            
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
        
        if (py->initCallback() < 0) {
            std::cout << "callback initialization error\r\n";
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
}
