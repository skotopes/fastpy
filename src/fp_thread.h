/*
 *  fp_thread.h
 *  fastPy
 *
 *  Created by Alexandr Kutuzov on 23.11.10.
 *  Copyright 2010 White-label ltd. All rights reserved.
 *
 */

#ifndef FP_THREAD_H
#define FP_THREAD_H

#include <pthread.h>

namespace fp {
    class Thread {
    public:
        Thread();
        Thread(const Thread&);
        virtual ~Thread();
        Thread& operator=(const Thread&);

        bool start();
        bool stop();

    protected:
        virtual void run()=0;

    private:
        static void *aCallback(void *data);

        pthread_t pth;
        pthread_attr_t attr;
        volatile bool running;
    };
}

#endif 