/*
 *  fp_config.h
 *  fastPy
 *
 *  Created by Alexandr Kutuzov on 02.09.09.
 *  Copyright 2009 White-label ltd. All rights reserved.
 *
 */

#ifndef FP_CONFIG_H
#define FP_CONFIG_H

#include <fstream>
#include <map>
#include <vector>

#include "fp_log.h"
#include "fp_helpers.h"

namespace fp {
    
    struct conf_t {
        std::string user;
        std::string group;
        
        int workers_cnt;
        int threads_cnt;
        int workers_ttl;
        
        bool accept_mt;
    };
    
    struct wsgi_t {
        std::string base_dir;
        std::string script;
        std::string point;
        
        bool load_site;
    };
    
    class config {
    public:
        config();
        virtual ~config();

        int readConf(char *file_name);
        static bool detach;
        static bool verbose;
        static bool debug;

    private:
        std::ifstream *in_file;
        std::string tmp_group;

    protected:
        static char *app_name;
        static conf_t conf;
        static wsgi_t wsgi;
    };
}
#endif
