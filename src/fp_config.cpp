/*
 *  fp_config.cpp
 *  fastJs
 *
 *  Created by Alexandr Kutuzov on 02.09.09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "fp_config.h"

namespace fp {
    char *config::app_name;
    conf_t config::cnf;
    env_t config::env;

    
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
            printLine("Config error: unable to open file");
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
                    group = line.substr(1, line.size()-2);

                    if (group.size() == 0) {
                        printLine("Config error: dummy group definition");
                        return -1;
                    }
                } else {
                    printLine("Config error: broken group definition");
                    return -1;
                }
            } else {
                if (group.size() == 0) {
                    printLine("Config error: definition without group");
                    return -1;
                }
                
                size_t eq_pos = line.find("=");
                
                if (eq_pos == line.size() - 1 || eq_pos == std::string::npos) {
                    printLine("Config error: declaration error");
                    return -1;
                } else {
                    std::string key, val;
                    
                    key = line.substr(0, eq_pos );
                    val = line.substr(eq_pos + 1, line.size());
                    
                    trim(key);
                    trim(val);
                    
                    if (group.compare("server") == 0) {
                        if (key.compare("max_connections") == 0) {
                            cnf.max_conn = toInt(val);
                        } else if (key.compare("workers") == 0) {
                            cnf.workers_cnt = toInt(val);
                        } else if (key.compare("user") == 0) {
                            
                        } else if (key.compare("group") == 0) {
                            
                        }
                    } else if (group.compare("environment") == 0) {
                        if (key.compare("wsgi_path") == 0) {
                            env.base_dir = val;
                        } else if (key.compare("wsgi_script") == 0) {
                            env.script = val;
                        } else if (key.compare("wsgi_handler") == 0) {
                            env.point = val;
                        }
                    } else {
                        printLine("Config error: i don`t know what do you want from me");
                        return -1;
                    }
                }
            }
        }
                
        return 0;
    }

}
