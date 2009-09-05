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
        Py_SetProgramName(app_name);
        Py_InitializeEx(0);
        PyEval_InitThreads();
        mainThreadState = NULL;
        mainThreadState = PyThreadState_Get();
        PyEval_ReleaseLock();
        
        def_vhost = vhosts["*"];
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
            std::string out;
            
            rc = fcgi->acceptRequest(request);
            
            if (rc < 0)
                break;
            
            // do GIL then execute code
            PyEval_AcquireLock();
            PyThreadState_Swap(workerThreadState);

            handler_t h;
            
            if (initHandler(h) == 0) {
                if (runHandler(h, out) < 0) {
                    out = "Failed";
                    PyErr_Print();
                }

                releaseHandler(h);
            } else {
                out = "Unable to initialze handler";
                PyErr_Print();
            }
            
            // releasing GIL
            PyThreadState_Swap(NULL);
            PyEval_ReleaseLock();
            

            fcgi->sendResponse(request, out);
//            fcgi->writeResponse(request, out);
            
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
    
    int worker::initHandler(handler_t &h) {
        std::string path_final;
        const char* python_path = Py_GetPath();

        // Setting path for object
        path_final = def_vhost.base_dir;
        path_final += ":";
        path_final += python_path;
        PySys_SetPath((char*)path_final.c_str());
        
        // getting srcipt object 
        h.pScript = PyString_FromString(def_vhost.script.c_str());
        
        // Importing object
        h.pModule = PyImport_Import(h.pScript);
        Py_DECREF(h.pScript);

        // if pModule was imported prperly we will be able to continue
        if (h.pModule == NULL) {
            return -1;
        }
        
        h.pFunc = PyObject_GetAttrString(h.pModule, def_vhost.handler.c_str());
        /* pFunc is a new reference */
        
        if (h.pFunc == NULL || !PyCallable_Check(h.pFunc)) {
            Py_XDECREF(h.pFunc);
            Py_DECREF(h.pModule);
            return -2;
        }

        return 0;
    }

    int worker::runHandler(handler_t &h, std::string &output) {
        PyObject *pReturn;
        
        
        // Calling our stuff
        pReturn = PyObject_CallObject(h.pFunc, NULL);

        // in case if something goes wrong
        if (pReturn == NULL) {
            return -1;
        }

        // it placed here cause we do not need to have it earlier
        char *pOutput;

        // checking call result
        if (PyList_Check(pReturn)) {
            int rSize = PyList_Size(pReturn);
            
            for (int i=0; i<rSize; i++) {
                // sSobj is borrowed reference so we DO NOT NEED to decrement reference count  
                PyObject* sSobj = PyList_GetItem(pReturn, i);

                // Check if returned object is string
                if (!PyString_Check(sSobj)) {
                    Py_DECREF(pReturn);
                    return -3;                    
                }
                
                // get char array
                pOutput = PyString_AsString(sSobj);
                
                // stringToChar failed, freeing result and return to main cycle
                if (pOutput == NULL) {
                    Py_DECREF(pReturn);
                    return -3;
                }
                
                // TODO: PEP333 probably we on the right way, but we chould check if headers os sent 
                output += pOutput;
            }
        } else if (PyString_Check(pReturn)) {
            // get char array
            pOutput = PyString_AsString(pReturn);
            
            // stringToChar failed, freeing result and return to main cycle
            if (pOutput == NULL) {
                Py_DECREF(pReturn);
                return -4;
            }
            
            output = pOutput;            
        } else {
            // returned object of incompatible type, finish him.
            Py_DECREF(pReturn);
            return -2;
        }
        
        // if everything fine we can decriment reference count
        Py_DECREF(pReturn);
        
        return 0;
    }

    int worker::releaseHandler(handler_t &h) {
        // cleanup handler
        Py_XDECREF(h.pFunc);
        Py_DECREF(h.pModule);
        
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
