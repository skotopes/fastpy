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

    PyThreadState *pyengine::mainThreadState = NULL;
    
    pyengine::pyengine() {
        // initialize python threading environment
        Py_SetProgramName(app_name);
        
        Py_InitializeEx(0);
        PyEval_InitThreads();
        mainThreadState = PyThreadState_Get();
        PyEval_ReleaseLock();
    }
    
    pyengine::~pyengine() {
        // Destroying GIL and finalizing python interpritator
        PyEval_AcquireLock();
        Py_Finalize();        
    }

    int pyengine::createThreadState(thread_t &t) {
        // creating python thread state
        //pthread_mutex_lock(&ts_create_mutex);
        
        PyEval_AcquireLock();
        t.mainInterpreterState = mainThreadState->interp;
        t.workerThreadState = PyThreadState_New(t.mainInterpreterState);
        PyEval_ReleaseLock();
        
        //pthread_mutex_unlock(&ts_create_mutex);
        
        return 0;
    }
    
    int pyengine::switchAndLockTC(thread_t &t) {
        PyEval_AcquireLock();
        PyThreadState_Swap(t.workerThreadState);

        return 0;
    }
    
    int pyengine::nullAndUnlockTC(thread_t &t) {
        PyThreadState_Swap(NULL);
        PyEval_ReleaseLock();
        
        return 0;
    }
    
    int pyengine::deleteThreadState(thread_t &t) {
        // Worker thread state is not needed any more
        // releasing lock and freeing mutexes
        PyEval_AcquireLock();
        PyThreadState_Swap(NULL);
        PyThreadState_Clear(t.workerThreadState);
        PyThreadState_Delete(t.workerThreadState);
        PyEval_ReleaseLock();
        
        return 0;
    }
    /* ========================================================================== */
    static PyObject *start_response(PyObject *self, PyObject *args, PyObject *kw) {
        start_response_t *s = (start_response_t*)self;
        s->f->writeResponse(s->r, (char*)"start response involved");
        
        std::cout << "we are here";
        
        return PyBool_FromLong(1);
    };
    
    static PyTypeObject start_response_type = {
        PyObject_HEAD_INIT(&PyType_Type)
        0,                          /* ob_size        */
        "start_response",           /* tp_name        */
        sizeof(start_response_t),	/* tp_basicsize   */
        0,                          /* tp_itemsize    */
        0,                          /* tp_dealloc     */
        0,                          /* tp_print       */
        0,                          /* tp_getattr     */
        0,                          /* tp_setattr     */
        0,                          /* tp_compare     */
        0,                          /* tp_repr        */
        0,                          /* tp_as_number   */
        0,                          /* tp_as_sequence */
        0,                          /* tp_as_mapping  */
        0,                          /* tp_hash        */
        &start_response,            /* tp_call        */
        0,                          /* tp_str         */
        0,                          /* tp_getattro    */
        0,                          /* tp_setattro    */
        0,                          /* tp_as_buffer   */
        Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,         /* tp_flags       */
        "start_response method",    /* tp_doc         */
    };

    /* ========================================================================== */
    
    int handler::gat_no = 0;
    
    handler::handler(fastcgi *f, FCGX_Request *r) {
        fcgi = f;
        req = r;
        
        v = vhosts["*"];
        
        py->createThreadState(t);
        
        at_no = gat_no;
        gat_no++;
        // TODO REMOVE IT
        start_response_type.tp_new = PyType_GenericNew;
        
        if (PyType_Ready(&start_response_type) < 0)
            std::cout << "shiiit \r\n";
        
        setPyEnv();
    }
    
    handler::~handler() {
        py->deleteThreadState(t);
    }
    
    int handler::proceedRequest() {
        // switch thread context
        py->switchAndLockTC(t);
        
        module_t m;
        
        if (initModule(m) == 0) {
            // TODO remove it
            fcgi->error500(req, "init ok");
            
            if (runModule(m) < 0) {
                PyErr_Print();
                fcgi->error500(req, "Handler fucked up");
            }
            
            releaseModule(m);
        } else {
            PyErr_Print();
            fcgi->error500(req, "Handler init fail");
        }
        
        // switch thread context to null, execution finished
        py->nullAndUnlockTC(t);
        
        // finishing request
        fcgi->finishRequest(req);

        return 0;
    }
    
    int handler::initModule(module_t &m) {        
        // getting srcipt object
        PyObject *pScript = PyString_FromString(v.script.c_str());
        
        // Importing object
        m.pModule = PyImport_Import(pScript);
        Py_DECREF(pScript);
        
        // if pModule was imported prperly we will be able to continue
        if (m.pModule == NULL) {
            return -1;
        }
        
        // get our method from module
        m.pFunc = PyObject_GetAttrString(m.pModule, v.point.c_str());
        
        if (m.pFunc == NULL || !PyCallable_Check(m.pFunc)) {
            Py_XDECREF(m.pFunc);
            Py_DECREF(m.pModule);
            return -2;
        }
        
        return 0;
    }
    
    int handler::runModule(module_t &m) {
        PyObject *pReturn, *pArgs = NULL, *pEnviron = NULL;
        char *pOutput;
        
        pEnviron = PyDict_New();
        
        if (pEnviron == NULL) {
            return -1;
        }
        
        if (initArgs(pEnviron) < 0) {
            return -1;
        }
                
        pArgs = PyTuple_Pack(2, pEnviron, &start_response_type);
        Py_INCREF(&start_response_type);
        
        // Calling our stuff
        pReturn = PyObject_CallObject(m.pFunc, pArgs);
        
        releaseArgs(pEnviron);
        
        // in case if something goes wrong
        if (pReturn == NULL) {
            return -1;
        }

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
                
                // TODO: PEP333 probably we on the right way, but we should check if headers is sent 
                fcgi->writeResponse(req, pOutput);
            }
        } else if (PyString_Check(pReturn)) {
            // get char array
            pOutput = PyString_AsString(pReturn);
            
            // stringToChar failed, freeing result and return to main cycle
            if (pOutput == NULL) {
                Py_DECREF(pReturn);
                return -4;
            }
            
            fcgi->writeResponse(req, pOutput);
        } else {
            // returned object of incompatible type, finish him.
            Py_DECREF(pReturn);
            return -2;
        }
        
        // if everything fine we can decriment reference count
        Py_DECREF(pReturn);
        
        return 0;
    }
    
    int handler::releaseModule(module_t &m) {
        // cleanup handler
        Py_XDECREF(m.pFunc);
        Py_DECREF(m.pModule);
        
        return 0;
    }
    
    int handler::setPyEnv() {
        std::string path_final;
        const char* python_path = Py_GetPath();
        
        // Setting path for object
        path_final = v.base_dir;
        path_final += ":";
        path_final += python_path;
        PySys_SetPath((char*)path_final.c_str());
        return 0;
    }
        
    int handler::initArgs(PyObject *dict) {
        PyDict_SetItem(dict, PyString_FromString("wsgi.multiprocess"), PyBool_FromLong(1));
        PyDict_SetItem(dict, PyString_FromString("wsgi.multithread"), PyBool_FromLong(1));
        PyDict_SetItem(dict, PyString_FromString("wsgi.run_once"), PyBool_FromLong(0));
//        PyDict_SetItem(dict, PyString_FromString("FASTPY_THREAD"), PyInt_FromLong(t));

        return 0;
    }
    
    int handler::releaseArgs(PyObject *dict) {
        Py_XDECREF(dict);
        return 0;
    }
}
