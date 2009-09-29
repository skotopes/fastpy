/*
 *  fp_ipc.h
 *  fastPy
 *
 *  Created by Alexandr Kutuzov on 26.09.09.
 *  Copyright 2009 White-label ltd. All rights reserved.
 *
 */

#ifndef FP_IPC_H
#define FP_IPC_H

#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "fp_core.h"
#include "fp_config.h"

namespace fp {
    struct wdata_t {
        // this shm timestamp
        time_t timestamp;
        
        // TODO: rewrite this part
        pthread_mutex_t access_mutex;
        
        // master flags
        bool blue_pill;
        bool red_pill;
        
        // worker status and signal
        uint32_t status;
        uint32_t signal;

        // worker threads stats
        uint32_t threads_used;
        uint32_t threads_free;
        
        // worker conn stats
        uint64_t conn_served;
        uint64_t conn_failed;
    };
        
    class ipc: public config {
    public:
        ipc();
        ~ipc();
        
        int initMQ(int w_num, bool force_create=false);
        int updateData(wdata_t &data);
        int readData(wdata_t &data);
        int closeMQ(bool force_close=false);
        
    private:
        key_t key;
        int shmid;
        wdata_t *sh_data;
    };
    
}

#endif
