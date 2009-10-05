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
    
    ipc::ipc() {
        shm = NULL;
    }
    
    ipc::~ipc() {
    }
    
    int ipc::initSHM(int w_num, bool force_create) {
        int rset = 0666;
        semun_t arg;
        
        if (force_create) {
            rset = rset | IPC_CREAT;
        }
        
        // creating key
        if ((key = ftok(app_name, w_num)) == -1) {
            return -1;
        }
        
        // get shared memory id
        if ((shmid = shmget(key, sizeof(wdata_t), rset)) == -1) {
            return -2;
        }

        if ((semid = semget(key, 1, rset)) == -1) {
            return -3;
        }
        
        // attach shared memory
        void *sh_ptr = shmat(shmid, NULL, 0);
        if (sh_ptr == (void *)(-1) || sh_ptr == NULL) {
            return -4;
        }

        // initialize semaphore to 1
        arg.val = 1;
        if (semctl(semid, 0, SETVAL, arg) == -1) {
            return -5;
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
    
    int ipc::lock() {
        sembuf sb = {0, -1, 0};

        if (semop(semid, &sb, 1) == -1) {
            return -1;
        }
        
        return 0;
    }

    int ipc::unlock() {
        sembuf sb = {0, 1, 0};
        
        if (semop(semid, &sb, 1) == -1) {
            return -1;
        }
        return 0;
    }
        
    int ipc::freeSHM(bool force_close) {
        semun_t arg;
        
        if (shm == NULL) {
            return -1;
        }
        
        if (shmdt(shm) == -1) {
            return -2;
        }
                
        if (force_close) {
            if (semctl(semid, 0, IPC_RMID, arg) == -1) {
                return -3;
            }
            
            shmctl(shmid, IPC_RMID, NULL);
        }
        
        return 0;
    }

}
