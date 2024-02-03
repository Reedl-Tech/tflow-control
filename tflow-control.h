#pragma once

#include <cassert>
#include <time.h>
#include <giomm.h>

#include "tflow-common.h"

#include "tflow-ctrl-cli.h"
#include "tflow-mg.h"

class TFlowControl {
public:
    TFlowControl();
    ~TFlowControl();

    GMainContext *context;
    GMainLoop *main_loop;
    
    void AttachIdle();
    void OnIdle();

    std::vector<TFlowCtrlCli> tflow_ctrl_clis; 
    TFlowMg *tflow_mg;
private:

};

