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
    int worker::wpid = 0;
    ipc worker::wipc;
    
    worker::worker(fastcgi *fc) {
        fcgi = fc;
        py = new pyengine();

        // initing ipc
        wpid = getpid();
        int e = wipc.initMQ(wpid, true);
        if (e < 0) {
            std::cout << "mq init error: " << e << " eeno:"<< errno <<std::endl;
        }
        
        // init joinable threads
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        pthread_mutex_init(&accept_mutex, NULL);

        signal(SIGHUP, sigHandler);
        signal(SIGINT, sigHandler);
        signal(SIGABRT, sigHandler);
        signal(SIGTERM, sigHandler);
    }
    
    worker::~worker() {
        wipc.closeMQ();
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
        
        if (conf.threads_cnt > 0) {
            threads.reserve(conf.threads_cnt + 1);
            
            // starting worker threads
            for (int i=0;i < conf.threads_cnt;i++) {
                pthread_t thread;
                
                int ec = pthread_create(&thread, &attr, worker::workerThread, (void*)this);
                
                if (ec) {
                    return -2;
                }
                
                threads.push_back(thread);
            }

            // starting schedule thread
            pthread_t s_thread;
            
            int ec = pthread_create(&s_thread, &attr, worker::schedulerThread, (void*)this);
            
            if (ec) {
                return -2;
            }
            
            threads.push_back(s_thread);
        } else {
            std::cout << "Illigal thread count\r\n";
            return -3;
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
        
    int worker::acceptor() {
        FCGX_Request *r;
        handler *h;
        
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
            int rc = fcgi->acceptRequest(r);
            
            pthread_mutex_unlock(&accept_mutex);
            
            // in case if some shit happens
            if (rc < 0) {
                continue;
                //break;
            }
            
            // procced request with handler
            h->proceedRequest();
        }
        
        delete h;
        
        // TODO check out condition
        return 0;
    }
    
    int worker::scheduler() {
        bool able_to_die = false;
        int i=0;
        do {
            wdata_t m;
            i++;
            m.timestamp = i;
            int e = wipc.updateData(m);
            if (e < 0) {
                std::cout << "Unable to update shm: "<< e << std::endl;
            } else {
                std::cout << "Writing shm for pid: "<< wpid << std::endl;
            }
            
            if (!able_to_work) able_to_die = true;
                
            sleep(1);
        } while (!able_to_die);
        
        return 0;
    }

    void *worker::workerThread(void *data) {
        worker *w = (worker*) data;
        w->acceptor();
        pthread_exit(NULL);
    }

    void *worker::schedulerThread(void *data) {
        worker *w = (worker*) data;
        w->scheduler();
        pthread_exit(NULL);
    }
    
    void worker::sigHandler(int sig_type) {
        able_to_work = false;
        std::cout << "Got signal, probably i should stop: "<< sig_type << std::endl;
                
        //exit(1);
    }
}
