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

    /* ================================ python */
    
    PyThreadState *pyengine::mainThreadState = NULL;
    long pyengine::tc_allocated = 0;
    
    pyengine::pyengine() {
        // initialize python threading environment
        
        Py_InitializeEx(0);
        PyEval_InitThreads();
        mainThreadState = PyThreadState_Get();
        PyEval_ReleaseLock();
        
        Py_SetProgramName(app_name);
        
        StartResponseType.tp_new = PyType_GenericNew;
        if (PyType_Ready(&StartResponseType) < 0)
            std::cout << "shiiit \r\n";
        
        setPath();
        initCallback();
    }
    
    pyengine::~pyengine() {
        releaseCallback();
        // Destroying GIL and finalizing python interpritator
        PyEval_AcquireLock();
        Py_Finalize();        
    }

    int pyengine::createThreadState(thread_t &t) {
        // creating python thread state
        PyEval_AcquireLock();
        t.mainInterpreterState = mainThreadState->interp;
        t.workerThreadState = PyThreadState_New(t.mainInterpreterState);
        PyEval_ReleaseLock();

        tc_allocated ++;
        t.tc_number = tc_allocated;
        
        return 0;
    }
    
    int pyengine::switchAndLockTC(thread_t &t) {
        PyEval_AcquireLock();
        PyThreadState_Swap(t.workerThreadState);
        t.in_use = true;
        
        return 0;
    }
    
    int pyengine::nullAndUnlockTC(thread_t &t) {
        PyThreadState_Swap(NULL);
        PyEval_ReleaseLock();
        t.in_use = false;
        
        return 0;
    }
    
    int pyengine::deleteThreadState(thread_t &t) {
        // Worker thread state is not needed any more
        PyEval_AcquireLock();
        PyThreadState_Swap(NULL);
        PyThreadState_Clear(t.workerThreadState);
        PyThreadState_Delete(t.workerThreadState);
        PyEval_ReleaseLock();
        
        return 0;
    }

    int pyengine::setPath() {
        std::string path_final;
        const char* python_path = Py_GetPath();
        
        // Setting path for object
        path_final = env.base_dir;
        path_final += ":";
        path_final += python_path;
        PySys_SetPath((char*)path_final.c_str());
        
        return 0;
    }

    /* ================================ pyengine::callback */

    bool pyengine::cbr_flag = false;
    PyObject *pyengine::pModule = NULL;
    PyObject *pyengine::pFunc = NULL;

    int pyengine::initCallback() {
        // getting srcipt object
        PyObject *pScript = PyString_FromString(env.script.c_str());
        
        // Importing object
        pModule = PyImport_Import(pScript);
        Py_DECREF(pScript);
        
        // if pModule was imported prperly we will be able to continue
        if (pModule == NULL) {
            return -1;
        }
        
        // get our method from module
        pFunc = PyObject_GetAttrString(pModule, env.point.c_str());
        
        if (pFunc == NULL || !PyCallable_Check(pFunc)) {
            Py_XDECREF(pFunc);
            Py_DECREF(pModule);
            return -2;
        }
        
        cbr_flag = true;
        
        return 0;
    }

    int pyengine::releaseCallback() {
        // cleanup handler
        Py_XDECREF(pFunc);
        Py_DECREF(pModule);
        
        return 0;
    }
    
    /* ================================ pyengine::helpers */
    
    StartResponseObject *pyengine::newSRObject() {
        StartResponseObject * r = (StartResponseObject *)PyObject_CallObject((PyObject *)&StartResponseType, NULL);
        return r;
    };

    PyTypeObject pyengine::StartResponseType = {
        PyObject_HEAD_INIT(&PyType_Type)
        0,                                          /* ob_size        */
        "start_response",                           /* tp_name        */
        sizeof(StartResponseObject),                /* tp_basicsize   */
        0,                                          /* tp_itemsize    */
        0,                                          /* tp_dealloc     */
        0,                                          /* tp_print       */
        0,                                          /* tp_getattr     */
        0,                                          /* tp_setattr     */
        0,                                          /* tp_compare     */
        0,                                          /* tp_repr        */
        0,                                          /* tp_as_number   */
        0,                                          /* tp_as_sequence */
        0,                                          /* tp_as_mapping  */
        0,                                          /* tp_hash        */
        &startResponse,                             /* tp_call        */
        0,                                          /* tp_str         */
        0,                                          /* tp_getattro    */
        0,                                          /* tp_setattro    */
        0,                                          /* tp_as_buffer   */
        Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags       */
        "start_response method",                    /* tp_doc         */
    };
    
    PyObject *pyengine::startResponse(PyObject *self, PyObject *args, PyObject *kw) {
        StartResponseObject *s = (StartResponseObject*)self;
        
        int rSize = PyTuple_Size(args);
        
        if (rSize == 2) {
            PyObject *pRC, *pHA;
            pRC = PyTuple_GetItem(args, 0); 
            pHA = PyTuple_GetItem(args, 1);
            
            if (pRC == NULL || pHA == NULL) {
                s->f->writeResponse(s->r, (char*)"500 null pointer in start response");
                return PyBool_FromLong(0);
            }
                
            s->f->writeResponse(s->r, (char*)"start response involved");
        } else {
            // throw python exception
            PyObject_Print(args, stdout, Py_PRINT_RAW);
            std::cout << rSize << std::endl;
            s->f->writeResponse(s->r, (char*)"args error");
        }

        return PyBool_FromLong(1);
    };    
    
    /* ================================ handler */
            
    handler::handler(fastcgi *f, FCGX_Request *r) {
        fcgi = f;
        req = r;

        py->createThreadState(t);
    }
    
    handler::~handler() {
        py->deleteThreadState(t);
    }
    
    int handler::proceedRequest() {
        // switch thread context
        py->switchAndLockTC(t);
        
        if (py->isCallbackReady()) {
            // TODO remove it
            fcgi->error500(req, "init ok");
            
            if (runModule() < 0) {
                PyErr_Print();
                fcgi->error500(req, "Handler fucked up");
            }
            
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
        
    int handler::runModule() {
        PyObject *pReturn, *pCallback, *pArgs = NULL, *pEnviron = NULL;
        char *pOutput;
        
        pCallback = py->getCallback();
        
        if (pCallback == NULL)
            return -1;
        
        pEnviron = PyDict_New();
        
        if (pEnviron == NULL)
            return -1;
        
        if (initArgs(pEnviron) < 0)
            return -1;
        
        StartResponseObject *sro = py->newSRObject();
        
        if (sro == NULL) {
            return -1;
        }
        
        sro->r = req;
        sro->f = fcgi;
        pArgs = PyTuple_Pack(2, pEnviron, sro);
        
        // Calling our stuff
        pReturn = PyObject_CallObject(pCallback, pArgs);
        
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
                
    int handler::initArgs(PyObject *dict) {
        // TODO: maybe it has sense to replace it ? 
        PyDict_SetItem(dict, PyString_FromString("wsgi.multiprocess"), PyBool_FromLong(1));
        PyDict_SetItem(dict, PyString_FromString("wsgi.multithread"), PyBool_FromLong(1));
        PyDict_SetItem(dict, PyString_FromString("wsgi.run_once"), PyBool_FromLong(0));
        PyDict_SetItem(dict, PyString_FromString("FASTPY_TC_NUMBER"), PyInt_FromLong(t.tc_number));

        return 0;
    }
    
    int handler::releaseArgs(PyObject *dict) {
        Py_XDECREF(dict);
        return 0;
    }
}
