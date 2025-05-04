#pragma once 

#include "mongoose.h"


class TFlowControl;
class TFlowMg {
public:
    TFlowMg(TFlowControl* app);
    ~TFlowMg();

    TFlowControl* app;

    // void onIdle(clock_t now);
    // int Connect();

    //void Disconnect();
    int onMsgFromMg();
    int sendMsgToMg(const json11::Json::object &params);

    //int sendSignature();

    // Fifo to received data from Mongoose
//    Flag fifo_state_flag;

    typedef struct
    {
        GSource g_source;
        TFlowMg* mg;
    } GSourceMg;

    GSourceMg*   pipe_src;
    gpointer     pipe_tag;
    GSourceFuncs pipe_gsfuncs;

private:
    
    // Data used exclusively by Mongoose from his own thread
    // Pointer stored in mg_connection.fn_data
    struct mg_data {
        char mark[4];
        int wr_fd;         
        int rd_fd;
        // Connections specific data?
        // ..
    } mg_data = {.mark = "MNG", .wr_fd = -1};

    int pipe_fd_mg2tflow[2];    // Pipe TFlow <-- Mongoose 
    int pipe_fd_tflow2mg[2];    // Pipe TFlow --> Mongoose

    pthread_t           th;
    pthread_cond_t      th_cond;
    pthread_mutex_t     th_mutex;

    struct mg_mgr mgr;

    clock_t last_idle_check;

    int msg_seq_num = 0;
    char mg_in_msg[1024 * 1024];   // Messages from Mongoose to CtrlCli 
    char mg_out_msg[1024 * 1024];  // Messages from CtrlCli to Mongoose

    clock_t last_send_ts;

    static void* _thread(void* ctx);

    static void _on_msg(struct mg_connection* c, int ev, void* ev_data);
    static void _wait_and_reply_tflow_response(struct mg_connection* c, struct mg_data* my_data);

};

