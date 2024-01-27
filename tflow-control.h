#pragma once

#include <cassert>
#include <time.h>
#include <giomm.h>

class Flag {
public:
    enum states {
        UNDEF,
        CLR,
        SET,
        FALL,
        RISE
    };
    enum states v = Flag::UNDEF;
};

#include "tflow-ctrl-cli.h"

class TFlowControl {
public:
    TFlowControl();
    ~TFlowControl();

    GMainContext *context;
    GMainLoop *main_loop;
    
    void AttachIdle();
    void OnIdle();

    std::vector<TFlowCtrlCli> tflow_ctrl_clis; 
private:

};

