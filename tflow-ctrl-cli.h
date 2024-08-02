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

    void onIdle(struct timespec* now_tp);
    int Connect();

    void Disconnect();
    int onCtrlMsg();

    int sendMsgToCtrl(const char *cmd, const json11::Json::object &params);
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

    std::string my_cli_name = "Control";

    std::string srv_name;
    struct timespec last_idle_check_tp = { 0 };

    int msg_seq_num = 0;

    size_t in_msg_size;
    char *in_msg;

    struct timespec last_send_tp = { 0 };

    int onCtrlMsgParse(const char* msg);
};

