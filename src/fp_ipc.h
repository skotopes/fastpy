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
#include <sys/sem.h> 

#include "fp_config.h"

namespace fp {
    // semafor union
    typedef union semun {
        int val;
        struct semid_ds *st;
        ushort * array;
    } semun_t;

    // worker state code 
    enum w_code_e {
        W_NRDY,     // worker not yet ready
        W_FINE,     // worker ready
        W_IRLD,     // worker in reload now
        W_BUSY,     // worker is busy and unable to serv conn
        W_FAIL,     // worker failure, probably we must terminate
        W_TERM      // worker terminated (alredy should be dead)
    };
    
    // master state code
    enum m_code_e {
        M_NRDY,     // master not yet ready
        M_FINE,     // master ready
        M_SKIP,     // master wants worker to skip connections 
        M_DRLD,     // master wants worker to reload code
        M_TERM      // master wants worker to terminate
    };
    
    struct wdata_t {
        // this shm timestamp
        time_t timestamp;
        
        // master and worker exchange point
        uint8_t m_code;
        uint8_t w_code;
        uint8_t signal;

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
        
        int initSHM(int w_num, bool force_create=false);
        int lock();
        int unlock();
        int freeSHM(bool force_close=false);
        
        wdata_t *shm;
        
    private:
        key_t key;
        int shmid;
        int semid;
    };
    
}

#endif
