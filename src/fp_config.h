/*
 *  fp_config.h
 *  fastJs
 *
 *  Created by Alexandr Kutuzov on 02.09.09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef FP_CONFIG_H
#define FP_CONFIG_H

#include <fstream>
#include <map>
#include <vector>

#include "fp_core.h"

namespace fp {

    struct conf_t {
        std::string user;
        std::string group;
        
        int max_conn;
        int workers_cnt;
        int workers_ttl;
    };
    
    struct vhost_t {
        std::string hostname;
        std::string base_dir;
        std::string handler;
    };
    
    class config: public core {
    public:
        config();
        ~config();

        int readConf(char *file_name);

    private:
        std::ifstream *in_file;
        std::string group;
        vhost_t *vt;

    protected:
        static conf_t cnf;
        static std::map<std::string, vhost_t> vhosts;
        
    };
}
#endif
