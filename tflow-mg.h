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
    int onMsg();

    //int sendMsg(const char* cmd, json11::Json::object params);
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
        int wr_fd;         // The saem as fifo_fd[WR]
        // Connections specific data?
        // ..
    } mg_data = {.mark = "MNG", .wr_fd = -1};

    int pipe_fd[2];             // Pipe TFlow <-- Mongoose 

    pthread_t           th;
    pthread_cond_t      th_cond;
    pthread_mutex_t     th_mutex;

    struct mg_mgr mgr;

    clock_t last_idle_check;

    int msg_seq_num = 0;
    char mg_in_msg[1024];
    char mg_out_msg[1024];

    clock_t last_send_ts;

    static void* _thread(void* ctx);

    static void _on_msg(struct mg_connection* c, int ev, void* ev_data);
   

};

