/*
 *  fp_thread.h
 *  fastPy
 *
 *  Created by Alexandr Kutuzov on 23.11.10.
 *  Copyright 2010 White-label ltd. All rights reserved.
 *
 */

#include "fp_thread.h"
#include <exception>

namespace fp {
    Thread::Thread():
        pth(),attr(),running(false)
    {
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    }

    Thread::Thread(const Thread&):
        pth(),attr(),running(false)
    {
        // "Thread: object copy is not allowed"
        throw std::exception();
    }

    Thread::~Thread()
    {
        if (running) {
            // "Thread: destructor called before joining"
            throw std::exception();
        }
    }

    Thread& Thread::operator=(const Thread&)
    {
        // "Thread: object copy is not allowed"
        throw std::exception();
    }

    bool Thread::start()
    {
        int ec = pthread_create(&pth, &attr, aCallback, this);
        if (ec)
            return false;
        running = true;

        return true;
    }

    bool Thread::stop()
    {
        if (!running)
            return true;

        int ec = pthread_join(pth, NULL);
        if (ec)
            return false;

        return true;
    }

    void *Thread::aCallback(void *data)
    {
        Thread *me = reinterpret_cast<Thread *>(data);

        me->run();
        me->running = false;

        pthread_exit(NULL);
    }
}