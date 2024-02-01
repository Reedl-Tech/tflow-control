#pragma once

#include <cassert>
#include <time.h>

#include <glib-unix.h>
#include <json11.hpp>

#define TFLOWCTRLSRV_SOCKET_NAME_BASE "_com.reedl.tflow.ctrl-server-"   // leading '_' will be replaced in real socket name

class TFlowControl;
      
class TFlowCtrlCli {
public:
    TFlowCtrlCli(TFlowControl* app, const char *srv_name);
    ~TFlowCtrlCli();
    
    TFlowControl* app;

    void onIdle(clock_t now);
    int Connect();

    void Disconnect();
    int onMsg();

    int sendMsg(const char *cmd, json11::Json::object params);
    int sendSignature();

    int sck_fd;                 // +
    Flag sck_state_flag;        // +

    typedef struct
    {
        GSource g_source;
        TFlowCtrlCli* cli;
    } GSourceCli;

    GSourceCli* sck_src;
    gpointer sck_tag;
    GSourceFuncs sck_gsfuncs;

private:

    GMainContext* context;

    std::string my_cli_name = "Control";

    std::string srv_name;
    clock_t last_idle_check;

    int msg_seq_num = 0;
    char in_msg[4096];

    clock_t last_send_ts;

};

