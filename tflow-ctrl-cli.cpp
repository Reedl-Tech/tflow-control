#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>

#include <giomm.h>
#include <glib-unix.h>

#include <json11.hpp>

#include "tflow-control.hpp"

#define CLEAR(x) memset(&(x), 0, sizeof(x))

static struct timespec diff_timespec(
    const struct timespec* time1,
    const struct timespec* time0)
{
    assert(time1);
    assert(time0);
    struct timespec diff = { .tv_sec = time1->tv_sec - time0->tv_sec, //
        .tv_nsec = time1->tv_nsec - time0->tv_nsec };
    if (diff.tv_nsec < 0) {
        diff.tv_nsec += 1000000000; // nsec/sec
        diff.tv_sec--;
    }
    return diff;
}

static double diff_timespec_msec(
    const struct timespec* time1,
    const struct timespec* time0)
{
    struct timespec d_tp = diff_timespec(time1, time0);
    return d_tp.tv_sec * 1000 + (double)d_tp.tv_nsec / (1000 * 1000);
}

TFlowCtrlCli::TFlowCtrlCli(TFlowControl* _app, const char *_srv_name) 
{
    app = _app;

    srv_name = std::string(_srv_name);

    sck_state_flag.v = Flag::UNDEF;
    sck_tag = NULL;
    sck_src = NULL;
    CLEAR(sck_gsfuncs);

    in_msg_size = 1024 * 1024;
    in_msg = (char*)g_malloc(in_msg_size);

    last_idle_check_tp = { 0 };
}

TFlowCtrlCli::~TFlowCtrlCli()
{
    Disconnect();

    if (in_msg) {
        g_free(in_msg);
    }

}

#if 0
int TFlowCrlCli::onXXX()
{
    return 0;
}
#endif

int TFlowCtrlCli::onCtrlMsgParse(const char *ctrl_in_msg)
{
    std::string j_err;
    const json11::Json j_in_msg = json11::Json::parse(ctrl_in_msg, j_err);

    if (j_in_msg.is_null()) {
        g_warning("TFlowCtrlCli: bad ctrl response - %s", j_err.c_str());
        return 0;
    }

    const json11::Json ctrl_resp_cmd = j_in_msg["cmd"];

    //{ "cmd"     , cmd           },
    //{ "dir"     , "response"    },        // For better log readability only
    //{ "err"     , resp_err_int  },        // Present in case of error
    //{ "err_msg" , resp_err_str  },        // Present in case of error
    //{ "params"  , j_resp_params }         // Present in case of NO error

    if (!ctrl_resp_cmd.is_string()) {
        //app->tflow_mg->sendMsgToMg(...);
        return 0;
    }

    const json11::Json &ctrl_resp_err = j_in_msg["err"];
    const json11::Json &ctrl_resp_err_msg = j_in_msg["err_msg"];

    if (ctrl_resp_err.is_number()) {
        // TFlowCtrlServer Report an error 
        // { "cmd" : { "err" : <code> , "err_msg", "some error text" } }

        app->tflow_mg->sendMsgToMg(json11::Json::object({ 
                {
                    ctrl_resp_cmd.string_value().c_str(),
                    json11::Json::object({
                        { "err", ctrl_resp_err.int_value() },
                        { "err_msg", ctrl_resp_err_msg.is_string() ?
                            ctrl_resp_err_msg.string_value() : "unknown" } })
                } })
        );
        return 0;
    }

    // All good - repack as { "module"  : { "cmd" : { params } } }
    const json11::Json &ctrl_resp_params = j_in_msg["params"];
    if (!ctrl_resp_params.is_object()) {
        return 0;   // Bad format from TFlow Module - do nothing.
    }

    const std::string &cmd_name = ctrl_resp_cmd.string_value();
    int received_config_id = -1;

    // TODO: Q: ? Direct action via reference or via parent's callback?
    // Steal config_id from "config" and "ui_sign" commands
    if (0 == strcmp(cmd_name.c_str(), "signature")       ||
        0 == strcmp(cmd_name.c_str(), "config")          ||
        0 == strcmp(cmd_name.c_str(), "config_streamer") ||
        0 == strcmp(cmd_name.c_str(), "config_recorder") ||
        0 == strcmp(cmd_name.c_str(), "ui_sign")) {
            
        const json11::Json &j_config_id = ctrl_resp_params["config_id"];
        if (j_config_id.is_number()) {
            received_config_id = j_config_id.int_value();
        }
    }

#if 0
    // Call parent's callback
    ctrl_onRespMsg(this, cmd_name.c_str(), ctrl_resp_params);
#if CODE_BROWSER
    TFlowControl::onCliRespMsg(resp, ctrl_resp_params);
#endif
#endif
    if (0 == strcmp(srv_name.c_str(), "Capture")) {

        // For messages from Capture and Process repack as in the following:
        // From TFlowCapture : {"cmd" : "config" , "dir": "request", "params" : { _params_ }} }						
        // TO WEB UI  : {"capture" : { "config" : { _params_ } } }						
        const json11::Json::object j_resp_cmd({
            { cmd_name, ctrl_resp_params } });      // For ex.: { "config" : { _params_ }

        const json11::Json::object j_resp({                            // For ex.: {"capture" : { "config" : { _params_ } } } 
            { "capture", j_resp_cmd } });

        app->saveCfgID("capture", received_config_id);  

        app->tflow_mg->sendMsgToMg(j_resp);
    } 
    else if (0 == strcmp(srv_name.c_str(), "Process")) {

        if ( 0 == strcmp("player", cmd_name.c_str()) || 
             0 == strcmp("player_dir", cmd_name.c_str()) ) {

            // Mimic player as a standalone module, but not part of Process
            const json11::Json::object j_resp({ 
                { cmd_name.c_str(), ctrl_resp_params } }); 

            app->tflow_mg->sendMsgToMg(j_resp);    // For ex.: {"player" : { _params_ } }  
        }
        else {
            const json11::Json::object j_resp_cmd({
                { ctrl_resp_cmd.string_value(), ctrl_resp_params } });      // For ex.: { "config" : { _params_ }

            const json11::Json::object j_resp({ 
                { "mvision", j_resp_cmd } });                         // For ex.: {"mvision" : { "config" : { _params_ } } } 

            app->saveCfgID("mvision", received_config_id);  

            app->tflow_mg->sendMsgToMg(j_resp);
        }
    } 
    else if (0 == strcmp(srv_name.c_str(), "VStream")) {

        // From TFlowVStream : {"cmd" : "streaming_config" , "dir": "request", "params" : { _params_ }} }						
        // TO WEB UI  : {"streaming" : { "config" : { _params_ } } }						

        if (0 == strncmp("recording_", cmd_name.c_str(), 10)) {
            std::string cmd_name_stripped(cmd_name.c_str() + 10);

            const json11::Json::object j_resp_cmd({
                { cmd_name_stripped.c_str(), ctrl_resp_params } });      // For ex.: { "config" : { _params_ }

            const json11::Json::object j_resp({
                { "recording", j_resp_cmd } });                          // For ex.: {"recording" : { "config" : { _params_ } } } 

            app->saveCfgID("recording", received_config_id);  

            app->tflow_mg->sendMsgToMg(j_resp);
        }
        else if (0 == strncmp("streaming_", cmd_name.c_str(), 10)) {
            std::string cmd_name_stripped(cmd_name.c_str() + 10);

            const json11::Json::object j_resp_cmd({
                { cmd_name_stripped.c_str(), ctrl_resp_params } });      // For ex.: { "config" : { _params_ }

            const json11::Json::object j_resp({
                { "streaming", j_resp_cmd } });                          // For ex.: {"streaming" : { "config" : { _params_ } } } 

            app->saveCfgID("streaming", received_config_id);  

            app->tflow_mg->sendMsgToMg(j_resp);
        }

    }
    //app->tflow_mg->sendMsgToMg(json11::Json::object({ {
    //    ctrl_resp_cmd.string_value().c_str(),
    //        ctrl_resp_player_params } } ));

    return 0;
}

int TFlowCtrlCli::onCtrlMsg()
{
    ssize_t res;
    int err;

    // Read-out all data from the socket 
    res = recv(sck_fd, in_msg, in_msg_size - 1, MSG_NOSIGNAL);

    if (res <= 0) {
        err = errno;
        if (err == EPIPE || err == ECONNREFUSED || err == ENOENT) {
            // May happens on Server close
            g_warning("TFlowCtrlCli: TFlow Control Server closed");
        }
        else {
            g_warning("TFlowCtrlCli: unexpected error (%ld, %d) - %s",
                res, err, strerror(err));
        }

        sck_state_flag.v = Flag::FALL;
        //last_idle_check = 0; // aka Idle loop kick
        return -1;
    }

    in_msg[res] = 0;

    return onCtrlMsgParse(in_msg);
}

gboolean tflow_ctrl_cli_dispatch(GSource* g_source, GSourceFunc callback, gpointer user_data)
{
    int rc;
    TFlowCtrlCli::GSourceCli* source = (TFlowCtrlCli::GSourceCli*)g_source;
    TFlowCtrlCli* cli = source->cli;

    g_info("TFlowCtrlCli: Incoming message");

    rc = cli->onCtrlMsg();

    if (rc) {
        // Critical error on PIPE. 
        return G_SOURCE_REMOVE;
    }
    else {
        return G_SOURCE_CONTINUE;
    }
   
}

int TFlowCtrlCli::sendMsgToCtrl(const char *cmd, const json11::Json::object &j_params)
{
    ssize_t res;

    if (sck_state_flag.v != Flag::SET) return 0;
    
    json11::Json j_msg = json11::Json::object{
        { "cmd"    , cmd      },
        { "dir"    , "request"},        // For better log readability only
        { "params" , j_params }
    };

    std::string s_msg = j_msg.dump();

    res = send(sck_fd, s_msg.c_str(), s_msg.length(), MSG_NOSIGNAL | MSG_DONTWAIT);
    int err = errno;
    if (res == -1) {
        if (err == EPIPE) {
            g_warning("TFlowCtrlCli: Can't send");
        }
        else {
            g_warning("TFlowCtrlCli: Send message error to [%s], %s (%d) - %s",
                srv_name.c_str(), cmd, err, strerror(err));
        }
        sck_state_flag.v = Flag::FALL;
        last_idle_check_tp = { 0 }; // aka Idle loop kick - TODO: rework for "connect to idle once"
        return -1;
    }
    g_info("TFlowCtrlCli: [%s] ->> [%s]  %s", 
        my_cli_name.c_str(), srv_name.c_str(), cmd);

    clock_gettime(CLOCK_MONOTONIC, &last_send_tp);
    
    // TODO: Mark pending response flag?
    // ???

    return 0;
}

int TFlowCtrlCli::sendSignature()
{
    json11::Json::object j_params = { 
        {"peer_signature" ,  my_cli_name.c_str() },
        {"pid"            ,  getpid()            },
    };

    sendMsgToCtrl("signature", j_params);
    return 0;
}

void TFlowCtrlCli::Disconnect()
{
    if (sck_fd != -1) {
        close(sck_fd);
        sck_fd = -1;
    }

    if (sck_src) {
        if (sck_tag) {
            g_source_remove_unix_fd((GSource*)sck_src, sck_tag);
            sck_tag = nullptr;
        }
        g_source_destroy((GSource*)sck_src);
        g_source_unref((GSource*)sck_src);
        sck_src = nullptr;
    }

    return;
}

int TFlowCtrlCli::Connect()
{
    int rc;
    struct sockaddr_un sock_addr;
    
    sck_fd = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_NONBLOCK, 0);
    if (sck_fd == -1) {
        g_warning("TFlowCtrlCli: Can't create socket for local client (%d) - %s", errno, strerror(errno));
        return -1;
    }

    // Initialize socket address
    memset(&sock_addr, 0, sizeof(struct sockaddr_un));
    sock_addr.sun_family = AF_UNIX;

    std::string sock_name = std::string(TFLOWCTRLSRV_SOCKET_NAME_BASE);

    sock_name += srv_name;
    std::transform(sock_name.begin(), sock_name.end(), sock_name.begin(),
        [](unsigned char c) { return std::tolower(c); });

    size_t sock_name_len = sock_name.length();

    memcpy(sock_addr.sun_path, sock_name.c_str(), sock_name_len);  // NULL termination excluded
    sock_addr.sun_path[0] = 0;

    socklen_t sck_len = sizeof(sock_addr.sun_family) + sock_name_len;
    rc = connect(sck_fd, (const struct sockaddr*)&sock_addr, sck_len);
    if (rc == -1) {
        //g_warning("TFlowCtrlCli: Can't connect to the Server [%s] %s (%d) - %s",
        //    srv_name.c_str(), sock_name.c_str(), errno, strerror(errno));

        close(sck_fd);
        sck_fd = -1;

        return -1;
    }

    g_info("TFlowCtrlCli: Connected to the server [%s]", srv_name.c_str());

    /* Assign g_source on the socket */
    sck_gsfuncs.dispatch = tflow_ctrl_cli_dispatch;
    sck_src = (GSourceCli*)g_source_new(&sck_gsfuncs, sizeof(GSourceCli));
    sck_tag = g_source_add_unix_fd((GSource*)sck_src, sck_fd, (GIOCondition)(G_IO_IN /* | G_IO_ERR  | G_IO_HUP */));
    sck_src->cli = this;
    g_source_attach((GSource*)sck_src, app->context);

    return 0;
}

void TFlowCtrlCli::onIdle(struct timespec* now_tp)
{

    double dt = diff_timespec_msec(now_tp, &last_idle_check_tp);

    /* Do not check to often*/
    if (dt < 3000) return;
    last_idle_check_tp = *now_tp;

    if (sck_state_flag.v == Flag::SET || sck_state_flag.v == Flag::CLR) {

        // Check idle connection
        if (diff_timespec_msec(&last_send_tp, now_tp) > 1000) {
            // sendPing();
        }
        return;
    }

    if (sck_state_flag.v == Flag::UNDEF || sck_state_flag.v == Flag::RISE) {
        int rc;

        rc = Connect();
        if (rc) {
            sck_state_flag.v = Flag::RISE;
        }
        else {
            sck_state_flag.v = Flag::SET;
            sendSignature();
        }
        return;
    }

    if (sck_state_flag.v == Flag::FALL) {
        // Connection aborted.
        // Most probably TFlow Ctrl Server is closed
        Disconnect();

        // Try to reconnect
        sck_state_flag.v = Flag::RISE;
    }

}
