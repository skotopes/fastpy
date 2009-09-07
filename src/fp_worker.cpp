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
    
    worker::worker(fastcgi &fc) {
        fcgi = &fc;
        hand = new handler(&fc);
        
        // create joinable threads
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        
        def_vhost = vhosts["*"];
    }
    
    worker::~worker() {
    }
    
    int worker::acceptor() {
        // some thread specific stuff
        FCGX_Request r;
        thread_t t;
        
        // Init request and thread state
        fcgi->initRequest(r);
        hand->createThreadState(t);
                
        // run loop
        while (true) {
            // accepting connection
            int rc = fcgi->acceptRequest(r);
            
            // in case if some shit happens
            if (rc < 0)
                break;
            
            // procced request with handler
            hand->proceedRequest(t, r, def_vhost);
            
            // finishing request
            fcgi->finishRequest(r);
        }
        
        hand->deleteThreadState(t);
        
        // TODO check out condition
        return 0;
    }
    
    int worker::startWorker() {
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
        
        return 0;
    }
        
    void *worker::workerThread(void *data) {
        worker *w = (worker*) data;
        w->acceptor();
        pthread_exit(NULL);
    }
}
