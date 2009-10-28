/*
 *  fp_ipc.cpp
 *  fastPy
 *
 *  Created by Alexandr Kutuzov on 26.09.09.
 *  Copyright 2009 White-label ltd. All rights reserved.
 *
 */

#include "fp_ipc.h"

namespace fp {
    
    /* IPC shared memory */
    
    ipc_shm::ipc_shm() {
        shm = NULL;
    }
    
    ipc_shm::~ipc_shm() {
    }
    
    int ipc_shm::initSHM(int w_num, bool force_create) {
        int rset = 0666;
        
        if (force_create) {
            rset = rset | IPC_CREAT;
        }
        
        // creating key
        if ((key = ftok("/tmp", w_num)) == -1) {
            return -1;
        }
        
        // get shared memory id
        if ((shmid = shmget(key, sizeof(wdata_t), rset)) == -1) {
            return -2;
        }
        
        // attach shared memory
        void *sh_ptr = shmat(shmid, NULL, 0);
        if (sh_ptr == (void *)(-1) || sh_ptr == NULL) {
            return -4;
        }
        
        shm = (struct wdata_t*) sh_ptr;
        
        if (force_create) {
            shm->timestamp = 0;
            shm->m_code = M_NRDY;
            shm->w_code = W_NRDY;
            shm->signal = 0;
            shm->threads_used = 0;
            shm->threads_free = 0;
            shm->conn_served = 0;
            shm->conn_failed = 0;            
        }
        
        return 0;
    }
            
    int ipc_shm::freeSHM(bool force_close) {        
        if (shm == NULL) {
            return -1;
        }
        
        if (shmdt(shm) == -1) {
            return -2;
        }
                
        if (force_close) {
            shmctl(shmid, IPC_RMID, NULL);
        }
        
        return 0;
    }

    /* IPC semaphore */
    
    ipc_sem::ipc_sem() {
    }
    
    ipc_sem::~ipc_sem() {
    }
    
    int ipc_sem::initSEM(int w_num, bool force_create) {
        int rset = 0666;
        semun_t arg;
        
        if (force_create) {
            rset = rset | IPC_CREAT;
        }
        
        // creating key
        if ((key = ftok("/tmp", w_num)) == -1) {
            return -1;
        }
                
        if ((semid = semget(key, 1, rset)) == -1) {
            return -3;
        }
        
        // initialize semaphore to 1
        arg.val = 1;
        if (semctl(semid, 0, SETVAL, arg) == -1) {
            return -5;
        }
        
        return 0;
    }
    
    int ipc_sem::lock() {
        sembuf sb = {0, -1, 0};
        
        if (semop(semid, &sb, 1) == -1) {
            return -1;
        }
        
        return 0;
    }
    
    int ipc_sem::unlock() {
        sembuf sb = {0, 1, 0};
        
        if (semop(semid, &sb, 1) == -1) {
            return -1;
        }
        return 0;
    }
    
    int ipc_sem::freeSEM(bool force_close) {
        semun_t arg;
                        
        if (force_close) {
            if (semctl(semid, 0, IPC_RMID, arg) == -1) {
                return -3;
            }
        }
        
        return 0;
    }
    
}
