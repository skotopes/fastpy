/*
 *  fp_config.cpp
 *  fastPy
 *
 *  Created by Alexandr Kutuzov on 02.09.09.
 *  Copyright 2009 White-label ltd. All rights reserved.
 *
 */

#include "fp_config.h"

namespace fp {
    char *config::app_name = NULL;
    bool config::detach = false;
    bool config::verbose = false;
    bool config::debug = false;
    conf_t config::conf;
    wsgi_t config::wsgi;
    
    
    config::config() {
        in_file = new std::ifstream;
    }
    
    config::~config() {
        
    }
    
    int config::readConf(char *file_name) {
        char buff[65535];
        std::string line;
        in_file->open(file_name, std::fstream::in);
        
        if (!in_file->is_open()) {
            ts_cout("config: unable to open file");
            return -1;
        }
        
        
        while (!in_file->eof()) {
            // get line from file
            in_file->getline(buff,65535);
            line = buff;
            
            // strip comments
            size_t cm_pos = line.find_first_of("#");
            if (cm_pos != line.size()-1){
                line = line.substr(0, cm_pos);
            }
            
            // strip whitespaces
            trim(line);
            
            if (line.size() == 0) {
                continue;
            }
            
            if (line.compare(0, 1, "[") == 0) {
                if (line.compare(line.size()-1, 1, "]") == 0) {
                    tmp_group = line.substr(1, line.size()-2);
                    
                    if (tmp_group.size() == 0) {
                        ts_cout("config: missed group definition");
                        return -1;
                    }
                } else {
                    ts_cout("config: broken group definition");
                    return -1;
                }
            } else {
                if (tmp_group.size() == 0) {
                    ts_cout("config: definition without group");
                    return -1;
                }
                
                size_t eq_pos = line.find("=");
                
                if (eq_pos == line.size() - 1 || eq_pos == std::string::npos) {
                    ts_cout("config: declaration error");
                    return -1;
                } else {
                    std::string key, val;
                    
                    key = line.substr(0, eq_pos );
                    val = line.substr(eq_pos + 1, line.size());
                    
                    trim(key);
                    trim(val);
                    
                    if (tmp_group.compare("server") == 0) {
                        if (key.compare("workers") == 0) {
                            conf.workers_cnt = toInt(val);
                        } else if (key.compare("threads") == 0) {
                            conf.threads_cnt = toInt(val);
                        } else if (key.compare("user") == 0) {
                            conf.user = val;
                        } else if (key.compare("group") == 0) {
                            conf.group = val;
                        } else {
                            ts_cout("config: unknown key used: "+key);
                        }
                    } else if (tmp_group.compare("wsgi") == 0) {
                        if (key.compare("wsgi_path") == 0) {
                            wsgi.base_dir = val;
                        } else if (key.compare("wsgi_script") == 0) {
                            wsgi.script = val;
                        } else if (key.compare("wsgi_handler") == 0) {
                            wsgi.point = val;
                        } else if (key.compare("wsgi_load_site") == 0) {
                            wsgi.load_site = toBool(val);
                        } else {
                            ts_cout("config: ignoring unknown key: "+ key);
                        }
                    } else if (tmp_group.compare("environment") == 0) {
                        
                    } else {
                        ts_cout("config: i don`t know what do you want from me");
                        return -1;
                    }
                }
            }
        }
        
        return 0;
    }
    
}
