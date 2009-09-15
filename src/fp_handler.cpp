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
    }
    
    pyengine::~pyengine() {
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
    
    int pyengine::typeInit() {
        if (PyType_Ready(&StartResponseType) < 0)
            return -1;
        if (PyType_Ready(&FastStreamType) < 0)
            return -1;
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
            PyErr_Print();
            return -1;
        }
        
        // get our method from module
        pFunc = PyObject_GetAttrString(pModule, (char *)env.point.c_str());
        
        if (pFunc == NULL || !PyCallable_Check(pFunc)) {
            Py_XDECREF(pFunc);
            Py_DECREF(pModule);
            PyErr_Print();
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
    
    /* ================================ pyengine::start_response */
    
    StartResponseObject *pyengine::newSRObject() {
        StartResponseObject * r = PyObject_NEW(StartResponseObject ,&StartResponseType);
        return r;
    };
    
    PyTypeObject pyengine::StartResponseType = {
        PyObject_HEAD_INIT(NULL)
        0,                                              /*ob_size*/
        "fast_stream" ,                                 /*tp_name*/
        sizeof(StartResponseObject),                    /*tp_basicsize*/
        0,                                              /*tp_itemsize*/
        0,                                              /*tp_dealloc*/
        0,                                              /*tp_print*/
        0,                                              /*tp_getattr*/
        0,                                              /*tp_setattr*/
        0,                                              /*tp_compare*/
        0,                                              /*tp_repr*/
        0,                                              /*tp_as_number*/
        0,                                              /*tp_as_sequence*/
        0,                                              /*tp_as_mapping*/
        0,                                              /*tp_hash */
        &startResponse,                                 /*tp_call*/
        0,                                              /*tp_str*/
        0,                                              /*tp_getattro*/
        0,                                              /*tp_setattro*/
        0,                                              /*tp_as_buffer*/
        Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,       /*tp_flags*/
        "start_response method",                        /* tp_doc */
        0,                                              /* tp_traverse */
        0,                                              /* tp_clear */
        0,                                              /* tp_richcompare */
        0,                                              /* tp_weaklistoffset */
        0,                                              /* tp_iter */
        0,                                              /* tp_iternext */
        0,                                              /* tp_methods */
        0,                                              /* tp_members */
        0,                                              /* tp_getset */
        0,                                              /* tp_base */
        0,                                              /* tp_dict */
        0,                                              /* tp_descr_get */
        0,                                              /* tp_descr_set */
        0,                                              /* tp_dictoffset */
        0,                                              /* tp_init */
        0,                                              /* tp_alloc */
        PyType_GenericNew,                              /* tp_new */
    };

    PyObject *pyengine::startResponse(PyObject *self, PyObject *args, PyObject *kw) {
        std::stringstream status, headers;
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
            
            if (!PyString_Check(pRC)||!PyList_Check(pHA)) {
                s->f->error500(s->r, (char*)"args error");
                return PyBool_FromLong(0);
            }

            status << "Status: " << (char*)PyString_AsString(pRC) << "\r\n";
            s->f->writeResponse(s->r, (char*)status.str().c_str());

            for (int i=0; i<PyList_Size(pHA); i++) {
                PyObject *t= PyList_GetItem(pHA, i);
                if (PyTuple_Check(t) && PyTuple_Size(t) == 2) {
                    PyObject *pA0 = PyTuple_GetItem(t, 0);
                    PyObject *pA1 = PyTuple_GetItem(t, 1);
                    headers << (char*)PyString_AsString(pA0) << ": " << (char*)PyString_AsString(pA1) << "\r\n";
                }
            }

            s->f->writeResponse(s->r, (char*)headers.str().c_str());
            s->f->writeResponse(s->r, (char *)"\r\n");
        } else {
            // throw python exception
            PyObject_Print(args, stdout, Py_PRINT_RAW);
            std::cout << rSize << std::endl;
            s->f->writeResponse(s->r, (char*)"args error");
        }

        return PyBool_FromLong(1);
    };    

    /* ================================ pyengine::fast_stream */
    
    FastStreamObject *pyengine::newFSObject() {
        FastStreamObject * r = PyObject_NEW(FastStreamObject ,&FastStreamType);
        return r;
    };    
    
    PyMemberDef pyengine::FastStreamMembers[] = {
        {NULL} /* Sentinel */
    };
    
    PyMethodDef pyengine::FastStreamMethods[] = {
    {"read", (PyCFunction)fcgi_Stream_read, METH_VARARGS,
        "read([size]) -> read at most size bytes, returned as a string."
        },
        {"readline", (PyCFunction)fcgi_Stream_readline, METH_VARARGS,
        "readline([size]) -> next line from the stream, as a string."
        },
        {"readlines", (PyCFunction)fcgi_Stream_readlines, METH_VARARGS,
        "readlines([size]) -> list of strings, each a line from the stream."
        },
        {"write", (PyCFunction)fcgi_Stream_write, METH_VARARGS,
        "write(str) -> None. Write string str to stream."
        },
        {"writelines", (PyCFunction)fcgi_Stream_writelines, METH_O,
        "writelines(sequence_of_strings) -> None.  Write the strings to the stream."
        },
        {"flush", (PyCFunction)fcgi_Stream_flush, METH_NOARGS,
        "flush() -> None. Flush the internal I/O buffer."
        },
        {NULL}  /* Sentinel */
    };
    
    PyTypeObject pyengine::FastStreamType = {
        PyObject_HEAD_INIT(NULL)
        0,                                              /*ob_size*/
        "fast_stream" ,                                 /*tp_name*/
        sizeof(FastStreamObject),                       /*tp_basicsize*/
        0,                                              /*tp_itemsize*/
        (destructor)fcgi_Stream_dealloc,                /*tp_dealloc*/
        0,                                              /*tp_print*/
        0,                                              /*tp_getattr*/
        0,                                              /*tp_setattr*/
        0,                                              /*tp_compare*/
        0,                                              /*tp_repr*/
        0,                                              /*tp_as_number*/
        0,                                              /*tp_as_sequence*/
        0,                                              /*tp_as_mapping*/
        0,                                              /*tp_hash */
        0,                                              /*tp_call*/
        0,                                              /*tp_str*/
        0,                                              /*tp_getattro*/
        0,                                              /*tp_setattro*/
        0,                                              /*tp_as_buffer*/
        Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,       /*tp_flags*/
        "FastCGI I/O Stream object",                    /* tp_doc */
        0,                                              /* tp_traverse */
        0,                                              /* tp_clear */
        0,                                              /* tp_richcompare */
        0,                                              /* tp_weaklistoffset */
        (getiterfunc)fcgi_Stream_getiter,               /* tp_iter */
        (iternextfunc)fcgi_Stream_iternext,             /* tp_iternext */
        FastStreamMethods,                              /* tp_methods */
        FastStreamMembers,                              /* tp_members */
        0,                                              /* tp_getset */
        0,                                              /* tp_base */
        0,                                              /* tp_dict */
        0,                                              /* tp_descr_get */
        0,                                              /* tp_descr_set */
        0,                                              /* tp_dictoffset */
        0,                                              /* tp_init */
        0,                                              /* tp_alloc */
        fcgi_Stream_new,                                /* tp_new */
    };
            
    size_t pyengine::new_buffersize(size_t currentsize) {
        if (currentsize > SMALLCHUNK) {
            if (currentsize <= BIGCHUNK)
                return currentsize + currentsize;
            else
                return currentsize + BIGCHUNK;
        }
        return currentsize + SMALLCHUNK;
    }
    
    PyObject *pyengine::fcgi_Stream_read(FastStreamObject *self, PyObject *args) {
        FCGX_Stream *s;
        long bytesrequested = -1;
        size_t bytesread, buffersize, chunksize;
        PyObject *v;
        
        fcgi_Stream_Check();
        
        s = *(self->s);
        
        if (!PyArg_ParseTuple(args, "|l:read", &bytesrequested))
            return NULL;
        
        if (bytesrequested == 0)
            return PyString_FromString("");
        
        if (bytesrequested < 0)
            buffersize = new_buffersize((size_t)0);
        else
            buffersize = bytesrequested;
        
        if (buffersize > INT_MAX) {
            PyErr_SetString(PyExc_OverflowError,
                            "requested number of bytes is more than a Python string can hold");
            return NULL;
        }
        
        v = PyString_FromStringAndSize((char *)NULL, buffersize);
        if (v == NULL)
            return NULL;
        
        bytesread = 0;
        
        for (;;) {
            Py_BEGIN_ALLOW_THREADS
            chunksize = FCGX_GetStr(BUF(v) + bytesread, buffersize - bytesread, s);
            Py_END_ALLOW_THREADS
            
            if (chunksize == 0) {
                if (FCGX_HasSeenEOF(s))
                    break;
                PyErr_SetString(PyExc_IOError, "Read failed");
                Py_DECREF(v);
                return NULL;
            }
            
            bytesread += chunksize;
            if (bytesread < buffersize) {
                break;
            }
            
            if (bytesrequested < 0) {
                buffersize = new_buffersize(buffersize);
                if (_PyString_Resize(&v, buffersize) < 0)
                    return NULL;
            } else {
                /* Got what was requested. */
                break;
            }
        }
        
        if (bytesread != buffersize)
            _PyString_Resize(&v, bytesread);
        
        return v;
    }
    
    PyObject *pyengine::get_line(FastStreamObject *self, long bytesrequested) {
        FCGX_Stream *s;
        size_t bytesread, buffersize;
        PyObject *v;
        int c, done;
        
        s = *(self->s);
        
        if (bytesrequested == 0)
            return PyString_FromString("");
        
        if (bytesrequested < 0)
            buffersize = new_buffersize((size_t)0);
        else
            buffersize = bytesrequested;
        
        if (buffersize > INT_MAX) {
            PyErr_SetString(PyExc_OverflowError,
                            "requested number of bytes is more than a Python string can hold");
            return NULL;
        }
        
        v = PyString_FromStringAndSize((char *)NULL, buffersize);
        if (v == NULL)
            return NULL;
        
        bytesread = 0;
        done = 0;
        
        for(;;) {
            Py_BEGIN_ALLOW_THREADS
            while (buffersize - bytesread > 0) {
                c = FCGX_GetChar(s);
                if (c == EOF) {
                    if (bytesread == 0) {
                        Py_BLOCK_THREADS
                        Py_DECREF(v);
                        return PyString_FromString("");
                    } else {
                        done = 1;
                        break;
                    }
                }
                
                *(BUF(v) + bytesread) = (char) c;
                bytesread++;
                
                if (c == '\n') {
                    done = 1;
                    break;
                }
            }
            Py_END_ALLOW_THREADS
            if (done)
                break;
            
            if (bytesrequested < 0) {
                buffersize = new_buffersize(buffersize);
                if (_PyString_Resize(&v, buffersize) < 0)
                    return NULL;
            }
        }
        
        if (bytesread != buffersize)
            _PyString_Resize(&v, bytesread);
        
        return v;
    }
    
    PyObject *pyengine::fcgi_Stream_readline(FastStreamObject *self, PyObject *args) {
        long bytesrequested = -1;
        
        fcgi_Stream_Check();
        
        if (!PyArg_ParseTuple(args, "|l:readline", &bytesrequested))
            return NULL;
        
        return get_line(self, bytesrequested);
    }
    
    PyObject *pyengine::fcgi_Stream_readlines(FastStreamObject *self, PyObject *args) {
        int err;
        long sizehint = 0;
        size_t total = 0;
        PyObject *list;
        PyObject *l;
        
        fcgi_Stream_Check();
        
        if (!PyArg_ParseTuple(args, "|l:readlines", &sizehint))
            return NULL;
        
        if ((list = PyList_New(0)) == NULL)
            return NULL;
        
        for (;;) {
            l = get_line(self, -1);
            if (l == NULL || PyString_GET_SIZE(l) == 0) {
                Py_XDECREF(l);
                break;
            }
            
            err = PyList_Append(list, l);
            Py_DECREF(l); 
            if (err != 0)
                break;
            
            total += PyString_GET_SIZE(l);
            if (sizehint && total >= sizehint)
                break;
        }
        
        return list;
    }
    
    PyObject *pyengine::fcgi_Stream_getiter(FastStreamObject *self) {
        fcgi_Stream_Check();
        
        Py_INCREF(self);
        return (PyObject *)self;
    }
    
    PyObject *pyengine::fcgi_Stream_iternext(FastStreamObject *self) {
        PyObject *l;
        
        fcgi_Stream_Check();
        
        l = get_line(self, -1);
        if (l == NULL || PyString_GET_SIZE(l) == 0) {
            Py_XDECREF(l);
            return NULL;
        }
        return (PyObject *)l;
    }
    
    PyObject *pyengine::fcgi_Stream_write(FastStreamObject *self, PyObject *args) {
        int wrote;
        FCGX_Stream *s;
        int len;
        char *data;
        
        fcgi_Stream_Check();
        
        s = *(self->s);
        
        if (!PyArg_ParseTuple(args, "s#", &data, &len))
            return NULL;
        
        if (len == 0) {
            Py_RETURN_NONE;
        }
        
        Py_BEGIN_ALLOW_THREADS
        wrote = FCGX_PutStr(data, len, s);
        Py_END_ALLOW_THREADS
        
        if (wrote < len) {
            if (wrote < 0) {
                PyErr_SetString(PyExc_IOError, "Write failed");
            } else {
                char msgbuf[256];
                PyOS_snprintf(msgbuf, sizeof(msgbuf),
                              "Write failed, wrote %d of %d bytes",
                              wrote, len);
                PyErr_SetString(PyExc_IOError, msgbuf);
            }
            return NULL;
        }
        
        Py_RETURN_NONE;
    }
    
    PyObject *pyengine::fcgi_Stream_writelines(FastStreamObject *self, PyObject *seq) {
        #define CHUNKSIZE 1000
        FCGX_Stream *s;
        PyObject *list, *line;
        PyObject *it;   /* iter(seq) */
        PyObject *result;
        int i, j, index, len, nwritten, islist;
        
        fcgi_Stream_Check();
        
        s = *(self->s);
        
        result = NULL;
        list = NULL;
        islist = PyList_Check(seq);
        if  (islist)
            it = NULL;
        else {
            it = PyObject_GetIter(seq);
            if (it == NULL) {
                PyErr_SetString(PyExc_TypeError,
                                "writelines() requires an iterable argument");
                return NULL;
            }
            /* From here on, fail by going to error, to reclaim "it". */
            list = PyList_New(CHUNKSIZE);
            if (list == NULL)
                goto error;
        }
        
        /* Strategy: slurp CHUNKSIZE lines into a private list,
         checking that they are all strings, then write that list
         without holding the interpreter lock, then come back for more. */
        for (index = 0; ; index += CHUNKSIZE) {
            if (islist) {
                Py_XDECREF(list);
                list = PyList_GetSlice(seq, index, index+CHUNKSIZE);
                if (list == NULL)
                    goto error;
                j = PyList_GET_SIZE(list);
            }
            else {
                for (j = 0; j < CHUNKSIZE; j++) {
                    line = PyIter_Next(it);
                    if (line == NULL) {
                        if (PyErr_Occurred())
                            goto error;
                        break;
                    }
                    PyList_SetItem(list, j, line);
                }
            }
            if (j == 0)
                break;
            
            for (i = 0; i < j; i++) {
                PyObject *v = PyList_GET_ITEM(list, i);
                if (!PyString_Check(v)) {
                    const char *buffer;
                    int len;
                    if (PyObject_AsReadBuffer(v,
                                              (const void**)&buffer,
                                              (Py_ssize_t*)&len) ||
                        PyObject_AsCharBuffer(v,
                                              &buffer,
                                              (Py_ssize_t*)&len)) {
                        PyErr_SetString(PyExc_TypeError,
                                        "writelines() argument must be a sequence of strings");
                        goto error;
                    }
                    line = PyString_FromStringAndSize(buffer,
                                                      len);
                    if (line == NULL)
                        goto error;
                    Py_DECREF(v);
                    PyList_SET_ITEM(list, i, line);
                }
            }
            
            Py_BEGIN_ALLOW_THREADS
            errno = 0;
            for (i = 0; i < j; i++) {
                line = PyList_GET_ITEM(list, i);
                len = PyString_GET_SIZE(line);
                nwritten = FCGX_PutStr(PyString_AS_STRING(line), len, s);
                if (nwritten != len) {
                    Py_BLOCK_THREADS
                    if (nwritten < 0) {
                        PyErr_SetString(PyExc_IOError, "Write failed");
                    } else {
                        char msgbuf[256];
                        PyOS_snprintf(msgbuf, sizeof(msgbuf),
                                      "Write failed, wrote %d of %d bytes",
                                      nwritten, len);
                        PyErr_SetString(PyExc_IOError, msgbuf);
                    }
                    goto error;
                }
            }
            Py_END_ALLOW_THREADS
            
            if (j < CHUNKSIZE)
                break;
        }
        
        Py_INCREF(Py_None);
        result = Py_None;
    error:
        Py_XDECREF(list);
        Py_XDECREF(it);
        return result;
        #undef CHUNKSIZE
    }
    
    PyObject *pyengine::fcgi_Stream_flush(FastStreamObject *self) {
        int rc;
        FCGX_Stream *s;
        
        fcgi_Stream_Check();
        
        s = *(self->s);
        
        Py_BEGIN_ALLOW_THREADS
        rc = FCGX_FFlush(s);
        Py_END_ALLOW_THREADS
        
        if (rc == -1) {
            PyErr_SetString(PyExc_IOError, "Flush failed");
            return NULL;
        }
        
        Py_RETURN_NONE;
    }
    
    void pyengine::fcgi_Stream_dealloc(FastStreamObject *self) {
        self->ob_type->tp_free((PyObject*)self);
    }
        
    PyObject *pyengine::fcgi_Stream_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
        FastStreamObject *self;
        
        self = (FastStreamObject *)type->tp_alloc(type, 0);
        fcgi_Stream_zero(self);
        return (PyObject *)self;
    }
    
    
    /* ================================ handler */
            
    handler::handler(fastcgi *f, FCGX_Request *r) {
        fcgi = f;
        req = r;

        py->createThreadState(t);
        pSro = py->newSRObject();
        pInput = py->newFSObject();
        pErrors = py->newFSObject();
        pCallback = py->getCallback();
        /*
        Py_INCREF(pInput);
        Py_INCREF(pErrors);
        Py_INCREF(pSro);
         */
        pInput->s = &req->in;
        pErrors->s = &req->err;
    }
    
    handler::~handler() {
        py->deleteThreadState(t);
    }
    
    int handler::proceedRequest() {
        // switch thread context
        py->switchAndLockTC(t);
        
        if (py->isCallbackReady()) {            
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
        PyObject *pReturn, *pArgs = NULL, *pEnviron = NULL;
        char *pOutput;
        
        if (pCallback == NULL)
            return -1;
        
        pEnviron = PyDict_New();
        
        if (pEnviron == NULL)
            return -1;
        
        if (initArgs(pEnviron) < 0)
            return -1;
        
        if (pSro == NULL) {
            return -1;
        }
        
        pSro->r = req;
        pSro->f = fcgi;

        pArgs = PyTuple_Pack(2, pEnviron, pSro);
        Py_DECREF(pEnviron);
        
        // Calling our stuff
        pReturn = PyObject_CallObject(pCallback, pArgs);
        releaseArgs(pArgs);
        Py_DECREF(pEnviron);
        
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
        // environ translator from python-fastcgi c wrapper
        for (char **e = req->envp; *e != NULL; e++) {
            PyObject *k, *v;
            char *p = strchr(*e, '=');
            if (p == NULL)
                continue;
            k = PyString_FromStringAndSize(*e, (int)(p-*e));
            if (k == NULL) {
                PyErr_Clear();
                continue;
            }
            v = PyString_FromString(p + 1);
            if (v == NULL) {
                PyErr_Clear();
                Py_DECREF(k);
                continue;
            }
            
            if (PyDict_SetItem(dict, k, v) != 0)
                PyErr_Clear();
            
            Py_DECREF(k);
            Py_DECREF(v);
        }
        
        // standart wsgi attributes
        PyObject * wsgiVersion = PyTuple_New(2);
        PyTuple_SetItem(wsgiVersion, 0, PyInt_FromLong(1));
        PyTuple_SetItem(wsgiVersion, 1, PyInt_FromLong(0));
        PyDict_SetItemString(dict, "wsgi.version", wsgiVersion);
        Py_DECREF(wsgiVersion);
        
        PyDict_SetItemString(dict, "wsgi.input", (PyObject *)pInput);
        PyDict_SetItemString(dict, "wsgi.errors", (PyObject *)pErrors);
        
        PyDict_SetItemString(dict, "wsgi.multiprocess", PyBool_FromLong(1));
        PyDict_SetItemString(dict, "wsgi.multithread", PyBool_FromLong(1));
        PyDict_SetItemString(dict, "wsgi.run_once", PyBool_FromLong(0));
        PyDict_SetItemString(dict, "FASTPY_TC_NUMBER", PyInt_FromLong(t.tc_number));
        
        return 0;
    }
    
    int handler::releaseArgs(PyObject *dict) {
        PyDict_Clear(dict);
        Py_XDECREF(dict);
        return 0;
    }
}
