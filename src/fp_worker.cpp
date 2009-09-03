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
        
        // create joinable threads
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

        // initialize python threading environment
        Py_InitializeEx(0);
        PyEval_InitThreads();
        mainThreadState = NULL;
        mainThreadState = PyThreadState_Get();
        PyEval_ReleaseLock();
    }
    
    worker::~worker() {
        // Destroying GIL and finalizing python interpritator
        PyEval_AcquireLock();
        Py_Finalize();
    }
    
    int worker::acceptor() {
        FCGX_Request request;
        
        fcgi->initRequest(request);
        
        // Create thread context
        PyEval_AcquireLock();
        PyInterpreterState * mainInterpreterState = mainThreadState->interp;
        PyThreadState * workerThreadState = PyThreadState_New(mainInterpreterState);
        PyEval_ReleaseLock();
        
        // run loop
        while (true) {
            int rc;
            
            rc = fcgi->acceptRequest(request);
            
            if (rc < 0)
                break;
            
            // do GIL then executing code
            PyEval_AcquireLock();
            PyThreadState_Swap(workerThreadState);

            // TODO some stuff from config and so on
            PyRun_SimpleString("print 'Today is'\n");

            // releasing GIL
            PyThreadState_Swap(NULL);
            PyEval_ReleaseLock();

            fcgi->sendResponse(request);
            
            // finishing request
            fcgi->finishRequest(request);
        }
        
        // Worker thread state is not needed any more
        // releasing lock and freeing mutexes 
        PyEval_AcquireLock();
        PyThreadState_Swap(NULL);
        PyThreadState_Clear(workerThreadState);
        PyThreadState_Delete(workerThreadState);
        PyEval_ReleaseLock();
        
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
