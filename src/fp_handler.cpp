/*
 *  fp_handler.cpp
 *  fastPy
 *
 *  Created by Alexandr Kutuzov on 06.09.09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "fp_handler.h"

namespace fp {
    
    handler::handler(fastcgi *f) {
        fcgi = f;
        // initialize python threading environment
        Py_SetProgramName(app_name);
        Py_InitializeEx(0);
        PyEval_InitThreads();
        mainThreadState = NULL;
        mainThreadState = PyThreadState_Get();
        PyEval_ReleaseLock();
        
    }
    
    handler::~handler() {
        // Destroying GIL and finalizing python interpritator
        PyEval_AcquireLock();
        Py_Finalize();        
    }
    
    int handler::createThreadState(thread_t &t) {

        PyEval_AcquireLock();
        t.mainInterpreterState = mainThreadState->interp;
        t.workerThreadState = PyThreadState_New(t.mainInterpreterState);
        PyEval_ReleaseLock();
        
        return 0;
    }

    int handler::deleteThreadState(thread_t &t) {
        // Worker thread state is not needed any more
        // releasing lock and freeing mutexes 
        PyEval_AcquireLock();
        PyThreadState_Swap(NULL);
        PyThreadState_Clear(t.workerThreadState);
        PyThreadState_Delete(t.workerThreadState);
        PyEval_ReleaseLock();

        return 0;
    }

    int handler::proceedRequest(thread_t &t, FCGX_Request &r, vhost_t &v) {
        // Do GIL then execute code
        PyEval_AcquireLock();
        PyThreadState_Swap(t.workerThreadState);
        
        // REFACTORING MUST FIX IT FIRST
        std::string out;
        handler_t h;
        
        if (initHandler(h, v) == 0) {
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

        fcgi->sendResponse(r, out);
        // fcgi->writeResponse(request, out);
        
        return 0;
    }
    
    int handler::initHandler(handler_t &h, vhost_t &v) {
        std::string path_final;
        const char* python_path = Py_GetPath();
        
        // Setting path for object
        path_final = v.base_dir;
        path_final += ":";
        path_final += python_path;
        PySys_SetPath((char*)path_final.c_str());
        
        // getting srcipt object 
        h.pScript = PyString_FromString(v.script.c_str());
        
        // Importing object
        h.pModule = PyImport_Import(h.pScript);
        Py_DECREF(h.pScript);
        
        // if pModule was imported prperly we will be able to continue
        if (h.pModule == NULL) {
            return -1;
        }
        
        h.pFunc = PyObject_GetAttrString(h.pModule, v.point.c_str());
        /* pFunc is a new reference */
        
        if (h.pFunc == NULL || !PyCallable_Check(h.pFunc)) {
            Py_XDECREF(h.pFunc);
            Py_DECREF(h.pModule);
            return -2;
        }
        
        return 0;
    }
    
    int handler::runHandler(handler_t &h, std::string &output) {
        PyObject *pReturn;
/*        PyObject *pArgs, *pEnviron = NULL;
        
        if (initEnviron(pEnviron) < 0) {
            return -1;
        }*/
        
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
    
    int handler::releaseHandler(handler_t &h) {
        // cleanup handler
        Py_XDECREF(h.pFunc);
        Py_DECREF(h.pModule);
        
        return 0;
    }
    
    int handler::initEnviron(PyObject *dict) {
        dict = PyDict_New();
        
        if (dict == NULL) {
            return -1;
        }
        
        PyDict_SetItem(dict, PyString_FromString("wsgi.multiprocess"), PyString_FromString("true"));
        PyDict_SetItem(dict, PyString_FromString("wsgi.multithread"), PyString_FromString("true"));
        
        return 0;
    }
    
    int handler::releaseEnviron(PyObject *dict) {
        PyDict_Clear(dict);
        Py_DECREF(dict);
        return 0;
    }
    
}
