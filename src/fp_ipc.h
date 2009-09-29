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
        pthread_mutex_t access_mutex;
        uint32_t reload_ba;
        uint32_t timestamp;
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
        int closeMQ();
        
    private:
        bool ready;

        key_t key;
        int shmid;
        wdata_t *sh_data;
    };
    
}

#endif
