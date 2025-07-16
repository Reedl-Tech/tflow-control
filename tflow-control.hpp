#pragma once

#include <cassert>
#include <unordered_map>
#include <time.h>
#include <giomm.h>

#include "tflow-common.hpp"
#include "tflow-ctrl-cli.hpp"
#include "tflow-mg.hpp"

class TFlowControl {
public:
    enum SRV_NAME {
        SRV_NAME_CAPTURE = 0,
        SRV_NAME_PROCESS = 1,
        SRV_NAME_VSTREAM = 2,
        NUM     = 3
    };

    TFlowControl();
    ~TFlowControl();

    GMainContext *context;
    GMainLoop *main_loop;
    
    void AttachIdle();
    void OnIdle();

    std::vector<TFlowCtrlCli> tflow_ctrl_clis; 
    TFlowMg *tflow_mg;

    std::unordered_map<std::string, int> config_ids;
private:


    
};

