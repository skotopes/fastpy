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
    pthread_mutex_t ipc::access_mutex;
    
    ipc::ipc() {
        ready = false;
    }
    
    ipc::~ipc() {
        if (ready) closeMQ();
    }
    
    int ipc::initMQ(int w_num, bool force_create) {
        int rset = 0666;
        
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
        
        // attach shared memory
        sh_data = (wdata_t*)shmat(shmid, (void *)0, 0);
        if ((char *)sh_data == (char *)(-1)) {
            return -3;
        }
        
        ready = true;
    }
    
    int ipc::updateData(wdata_t &data) {
        if (!ready) {
            return -1;
        }
        
        pthread_mutex_lock(&access_mutex);
//        *sh_data = data;
        memcpy(sh_data, &data, sizeof(wdata_t));
        pthread_mutex_unlock(&access_mutex);
        
        return 0;
    }
    
    int ipc::readData(wdata_t &data) {
        if (!ready) {
            return -1;
        }

        pthread_mutex_lock(&access_mutex);
//        data = *sh_data;
        memcpy(&data, sh_data, sizeof(wdata_t));
        pthread_mutex_unlock(&access_mutex);
        
        return 0;
    }
    
    int ipc::closeMQ() {
        // probably key is already destroyed
        if (!ready) {
            return -1;
        }
        
        if (shmdt(sh_data) == -1) {
            return -2;
        }
        
        shmctl(shmid, IPC_RMID, NULL);
        
        ready = false;
        
        return 0;
    }

}
