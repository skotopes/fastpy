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
        sh_data = NULL;
    }
    
    ipc::~ipc() {
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
        void *sh_ptr = shmat(shmid, NULL, 0);
        if (sh_ptr == (void *)(-1) || sh_ptr == NULL) {
            return -3;
        }
        
        sh_data = (struct wdata_t*) sh_ptr;
        
        if (force_create) {
            pthread_mutex_init(&sh_data->access_mutex, NULL);
        }
        
        return 0;
    }
    
    int ipc::updateData(wdata_t &data) {
        if (sh_data == NULL) {
            return -1;
        }
        
        pthread_mutex_lock(&sh_data->access_mutex);
        *sh_data = data;
        pthread_mutex_unlock(&sh_data->access_mutex);
        
        return 0;
    }
    
    int ipc::readData(wdata_t &data) {
        if (sh_data == NULL) {
            return -1;
        }

        pthread_mutex_lock(&sh_data->access_mutex);
        data = *sh_data;
        pthread_mutex_unlock(&sh_data->access_mutex);

        return 0;
    }
    
    int ipc::closeMQ(bool force_close) {
        // probably key is already destroyed
        if (sh_data == NULL) {
            return -1;
        }
        
        if (shmdt(sh_data) == -1) {
            return -2;
        }
        
        if (force_close) {
            shmctl(shmid, IPC_RMID, NULL);
        }
        
        return 0;
    }

}
