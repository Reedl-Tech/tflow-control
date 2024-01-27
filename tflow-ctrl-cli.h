#pragma once

#include <cassert>
#include <time.h>

#include <glib-unix.h>

#define TFLOWCTRLSRV_SOCKET_NAME_BASE "_com.reedl.tflow.ctrl-server-"   // leading '_' will be replaced in real socket name

class TFlowControl;
      
class TFlowCtrlCli {
public:
    TFlowCtrlCli(TFlowControl* app, char *srv_name);
    ~TFlowCtrlCli();
    
    TFlowControl* app;

    void onIdle(clock_t now);
    int Connect();

    void Disconnect();
    int onMsg();

    int sendMsg(char *cmd, json11::Json::object params);
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

    std::string srv_name;                   // +
    clock_t last_idle_check;                // +

    int msg_seq_num = 0;
    int cam_fd;

    class TFlowCtrlMsg msg;

    clock_t last_send_ts;

};

