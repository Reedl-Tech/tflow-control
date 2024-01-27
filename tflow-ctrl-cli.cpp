#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>

#include <giomm.h>
#include <glib-unix.h>

#include <json11.hpp>
using namespace json11;

#include "tflow-control.h"

#define CLEAR(x) memset(&(x), 0, sizeof(x))

TFlowCtrlCli::TFlowCtrlCli(TFlowControl* _app, char *_srv_name)
{
    app = _app;
    context = app->context;

    // Create socket name from cli_name
    srv_name = std::string(_srv_name);

    sck_state_flag.v = Flag::UNDEF;
    sck_tag = NULL;
    sck_src = NULL;
    CLEAR(sck_gsfuncs);

    last_idle_check = 0;
}

TFlowCtrlCli::~TFlowCtrlCli()
{
    Disconnect();
}

#if 0
int TFlowCrlCli::onXXX()
{
    return 0;
}

int TFlowCtrlCli::onMsg()
{
    TFlowBuf::pck_t pck;

    // Read-out all data from the socket 
    res = recv(sck_fd, &pck, MSG_NOSIGNAL);
    err = errno;

    if (res <= 0) {
        if (err == EPIPE || err == ECONNREFUSED || err == ENOENT) {
            // May happens on Server close
            g_warning("TFlowCtrlCli: TFlow Buffer Server closed");
        }
        else {
            g_warning("TFlowCtrlCli: unexpected error (%ld, %d) - %s",
                res, err, strerror(err));
        }

        sck_state_flag.v = Flag::FALL;
        last_idle_check = 0; // aka Idle loop kick
        return -1;
    }

    switch (pck.hdr.id) { 
    case TFLOWCTRL_MSG_xxx:
    {
        g_debug("---TFlowCtrlCli: Received - xxx");
        struct TFlowCtrlCli::pck_xxx *pck_xxx = (struct TFlowCtrlCli::pck_xxx*)&pck;

        onXXX(pck_xxx);
        break;
    }
    default:
        g_warning("Oooops - Unknown message received %d", pck.hdr.id);
    }

    return 0;
}
#endif

gboolean tflow_ctrl_cli_dispatch(GSource* g_source, GSourceFunc callback, gpointer user_data)
{
    int rc;
    TFlowCtrlCli::GSourceCli* source = (TFlowCtrlCli::GSourceCli*)g_source;
    TFlowCtrlCli* cli = source->cli;

    g_info("TFlowCtrlCli: Incoming message");

    rc = cli->onMsg();

    if (rc) {
        // Critical error on PIPE. 
        return G_SOURCE_REMOVE;
    }
    else {
        return G_SOURCE_CONTINUE;
    }
   
}

int TFlowCtrlCli::sendMsg(char *cmd, json11::Json::object j_params)
{
    ssize_t res;

    if (sck_state_flag.v != Flag::SET) return 0;
    
    json11::Json j_msg = json11::Json::object{
        { "cmd"    , cmd      },
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
                srv_name, cmd, err, strerror(err));
        }
        sck_state_flag.v = Flag::FALL;
        last_idle_check = 0; // aka Idle loop kick
        return -1;
    }
    g_warning("TFlowCtrlCli: ->> [%s]  %s", srv_name, cmd);
    last_send_ts = clock();
    return 0;
}

int TFlowCtrlCli::sendSignature()
{
    json11::Json::object j_params = { 
        {"peer_signature" , "TFlowControl" },
        {"pid"            ,  getpid()      },
    };

    sendMsg("signature", j_params);
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
    size_t sock_name_len = sock_name.length();

    memcpy(sock_addr.sun_path, sock_name.c_str(), sock_name_len);  // NULL termination excluded
    sock_addr.sun_path[0] = 0;

    socklen_t sck_len = sizeof(sock_addr.sun_family) + sock_name_len;
    rc = connect(sck_fd, (const struct sockaddr*)&sock_addr, sck_len);
    if (rc == -1) {
        g_warning("TFlowCtrlCli: Can't connect to the Server [%s] %s (%d) - %s",
            srv_name, sock_name.c_str(), errno, strerror(errno));

        close(sck_fd);
        sck_fd = -1;

        return -1;
    }

    g_warning("TFlowCtrlCli: Connected to the Server [%s]", srv_name.c_str());

    /* Assign g_source on the socket */
    sck_gsfuncs.dispatch = tflow_ctrl_cli_dispatch;
    sck_src = (GSourceCli*)g_source_new(&sck_gsfuncs, sizeof(GSourceCli));
    sck_tag = g_source_add_unix_fd((GSource*)sck_src, sck_fd, (GIOCondition)(G_IO_IN /* | G_IO_ERR  | G_IO_HUP */));
    sck_src->cli = this;
    g_source_attach((GSource*)sck_src, context);

    return 0;
}

void TFlowCtrlCli::onIdle(clock_t now)
{
    clock_t dt = now - last_idle_check;

    /* Do not check to often*/
    if (dt < 3 * CLOCKS_PER_SEC) return;
    last_idle_check = now;

    if (sck_state_flag.v == Flag::SET || sck_state_flag.v == Flag::CLR) {

        // Check idle connection
        if (last_send_ts - now > 1000) {
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
