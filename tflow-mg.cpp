#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#include <glib-unix.h>

#include <json11.hpp>

#include "tflow-control.h"
#include "tflow-mg.h"

struct user {
  const char *name, *pass, *access_token;
};

// Settings
struct settings {
  bool log_enabled;
  int log_level;
  long brightness;
  char *device_name;
};

static char tflow_mg_in[1024 * 1024];      // Buffer for messages from TFlowMG to MG
static char tflow_mg_out[1024 * 1024];      // Buffer for messages from MG to TFlowMG 

static struct settings s_settings = {true, 1, 57, NULL};

static const char *s_json_header =
    "Content-Type: application/json\r\n"
    "Cache-Control: no-cache\r\n";

static struct user *authenticate(struct mg_http_message *hm) {
  // In production, make passwords strong and tokens randomly generated
  // In this example, user list is kept in RAM. In production, it can
  // be backed by file, database, or some other method.
  static struct user users[] = {
      {"admin", "admin", "admin_token"},
      {"user1", "user1", "user1_token"},
      {"user2", "user2", "user2_token"},
      {NULL, NULL, NULL},
  };
  char user[64], pass[64];
  struct user *u, *result = NULL;
  mg_http_creds(hm, user, sizeof(user), pass, sizeof(pass));
  MG_VERBOSE(("user [%s] pass [%s]", user, pass));

  if (user[0] != '\0' && pass[0] != '\0') {
    // Both user and password is set, search by user/password
    for (u = users; result == NULL && u->name != NULL; u++)
      if (strcmp(user, u->name) == 0 && strcmp(pass, u->pass) == 0) result = u;
  } else if (user[0] == '\0') {
    // Only password is set, search by token
    for (u = users; result == NULL && u->name != NULL; u++)
      if (strcmp(pass, u->access_token) == 0) result = u;
  }
  return result;
}

static void handle_login(struct mg_connection *c, struct user *u) {
  char cookie[256];
  mg_snprintf(cookie, sizeof(cookie),
              "Set-Cookie: access_token=%s; Path=/; "
              "%sHttpOnly; SameSite=Lax; Max-Age=%d\r\n",
              u->access_token, c->is_tls ? "Secure; " : "", 3600 * 24);
  mg_http_reply(c, 200, cookie, "{%m:%m}", MG_ESC("user"), MG_ESC(u->name));
}

static void handle_logout(struct mg_connection *c) {
  char cookie[256];
  mg_snprintf(cookie, sizeof(cookie),
              "Set-Cookie: access_token=; Path=/; "
              "Expires=Thu, 01 Jan 1970 00:00:00 UTC; "
              "%sHttpOnly; Max-Age=0; \r\n",
              c->is_tls ? "Secure; " : "");
  mg_http_reply(c, 200, cookie, "true\n");
}

static void handle_debug(struct mg_connection *c, struct mg_http_message *hm) {
  int level = mg_json_get_long(hm->body, "$.level", MG_LL_DEBUG);
  mg_log_set(level);
  mg_http_reply(c, 200, "", "Debug level set to %d\n", level);
}

static size_t print_int_arr(void (*out)(char, void *), void *ptr, va_list *ap) {
  size_t i, len = 0, num = va_arg(*ap, size_t);  // Number of items in the array
  int *arr = va_arg(*ap, int *);              // Array ptr
  for (i = 0; i < num; i++) {
    len += mg_xprintf(out, ptr, "%s%d", i == 0 ? "" : ",", arr[i]);
  }
  return len;
}

static void handle_status_get(struct mg_connection *c) {
  
  // Send commands to TFlowCtrl 
  // Wait for reply with timeout


  // TODO: Loop over connected ctrl clients and composes JSON
  // ... Module version 
  // ? Add self status ? Is there any status beside OK?
  
  // Status of each particular module
  mg_http_reply(c, 200, s_json_header, "{%m:%d,%m:%d}\n",
                MG_ESC("capture"), 1,  
                MG_ESC("process"), 2);
}

void TFlowMg::_wait_and_reply_tflow_response(struct mg_connection* c, struct mg_data* my_data)
{
    ssize_t bytes_flush;
    do {
        bytes_flush = read(my_data->rd_fd, tflow_mg_in, sizeof(tflow_mg_in) - 1);
    } while (bytes_flush == sizeof(tflow_mg_in) - 1);

    // Wait for response with timeout
    struct pollfd waiter = { .fd = my_data->rd_fd, .events = POLLIN };
    int poll_res = poll(&waiter, 1, 3 * 1000);

    switch (poll_res) {   // 1sec
    case 0:
        mg_http_reply(c, 408, "", "TFlow doesn't respond\n");
        break;
    case 1:
        if (waiter.revents & POLLIN) {
            ssize_t bytes_read = read(my_data->rd_fd, tflow_mg_in, sizeof(tflow_mg_in) - 1);
            if (bytes_read < 0) {
                mg_http_reply(c, 403, "", "Denied on read\n");
                break;
            }
            tflow_mg_in[bytes_read] = '\0';

            mg_http_reply(c, 200, s_json_header, "%s\n", tflow_mg_in);
            memset(tflow_mg_in, 0, sizeof(tflow_mg_in));
            break;
        }
        else if (waiter.revents & POLLERR) {
            mg_http_reply(c, 403, "", "Denied on poll\n");
            break;
        }
        else if (waiter.revents & POLLHUP) {
            mg_http_reply(c, 403, "", "Denied - hup\n");
            break;
        }
        break;
    default:
        mg_http_reply(c, 403, "", "Denied - X3\n");
        break;
    }

    return;
}

void TFlowMg::_wait_and_reply_tflow_response(struct mg_connection* c, struct mg_data* my_data)
{
    ssize_t bytes_flush;
    do {
        bytes_flush = read(my_data->rd_fd, tflow_mg_in, sizeof(tflow_mg_in) - 1);
    } while (bytes_flush == sizeof(tflow_mg_in) - 1);

    // Wait for response with timeout
    struct pollfd waiter = { .fd = my_data->rd_fd, .events = POLLIN };
    int poll_res = poll(&waiter, 1, 3 * 1000);

    switch (poll_res) {   // 1sec
    case 0:
        mg_http_reply(c, 408, "", "TFlow doesn't respond\n");
        break;
    case 1:
        if (waiter.revents & POLLIN) {
            ssize_t bytes_read = read(my_data->rd_fd, tflow_mg_in, sizeof(tflow_mg_in) - 1);
            if (bytes_read < 0) {
                mg_http_reply(c, 403, "", "Denied on read\n");
                break;
            }
            tflow_mg_in[bytes_read] = '\0';

            mg_http_reply(c, 200, s_json_header, "%s\n", tflow_mg_in);
            memset(tflow_mg_in, 0, sizeof(tflow_mg_in));
            break;
        }
        else if (waiter.revents & POLLERR) {
            mg_http_reply(c, 403, "", "Denied on poll\n");
            break;
        }
        else if (waiter.revents & POLLHUP) {
            mg_http_reply(c, 403, "", "Denied - hup\n");
            break;
        }
        break;
    default:
        mg_http_reply(c, 403, "", "Denied - X3\n");
        break;
    }

    return;
}

void TFlowMg::_on_msg(struct mg_connection* c, int ev, void* ev_data)
{
    struct mg_data *my_data = (struct mg_data* )c->fn_data;

    if (ev == MG_EV_OPEN && c->is_listening) {
        // Connection created
    }
    else if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message* hm = (struct mg_http_message*)ev_data;
        struct user *u = authenticate(hm);

        if (mg_http_match_uri(hm, "/websocket")) {
            mg_ws_upgrade(c, hm, NULL);  // Upgrade HTTP to Websocket
            c->data[0] = 'W';            // Set some unique mark on a connection
        }
        else if (mg_http_match_uri(hm, "/api/#") && u == NULL) {
            mg_http_reply(c, 403, "", "Not Authorised\n");
        } else if (mg_http_match_uri(hm, "/api/login")) {
            handle_login(c, u);
        } else if (mg_http_match_uri(hm, "/api/logout")) {
            handle_logout(c);
        } else if (mg_http_match_uri(hm, "/api/status/get")) {
            handle_status_get(c);
        }

        else if (mg_http_match_uri(hm, "/api/capture/get")) {
            // TODO: Add packet token
            const char* capture_get = "{\"capture\" : {}}"; // Send empty command - set nothing and get back all current controls
            int res = write(my_data->wr_fd, capture_get, strlen(capture_get));
            // Wait for token sent and discard all others
            _wait_and_reply_tflow_response(c, my_data);
        }

        else if (mg_http_match_uri(hm, "/api/playback/get")) {
            // TODO: Add packet token
            const char* playback_get = "{\"playback\" : {}}"; // Send empty command - set nothing and get back all current controls
            int res = write(my_data->wr_fd, playback_get, strlen(playback_get));
            // Wait for token sent and discard all others
            _wait_and_reply_tflow_response(c, my_data);
        }
        else if (mg_http_match_uri(hm, "/api/playback/set")) {
            int res = write(my_data->wr_fd, hm->body.ptr, hm->body.len);
            _wait_and_reply_tflow_response(c, my_data);
        } 
        else if (mg_http_match_uri(hm, "/api/playback/get_dir")) {
            int res = write(my_data->wr_fd, hm->body.ptr, hm->body.len);
            _wait_and_reply_tflow_response(c, my_data);
        }
        else {
            // Serve static files
            struct mg_http_serve_opts opts = {.root_dir = "/home/root/web_root"};
            mg_http_serve_dir(c, (mg_http_message*)ev_data, &opts);
        }
    }
    else if (ev == MG_EV_WS_OPEN) {
        c->data[0] = 'W';  // Mark this connection as an established WS client
    }
    else if (ev == MG_EV_WS_MSG) {
        // Got websocket frame. Received data is wm->data
        struct mg_ws_message* wm = (struct mg_ws_message*)ev_data;
        mg_ws_send(c, wm->data.ptr, wm->data.len, WEBSOCKET_OP_TEXT);
        mg_iobuf_del(&c->recv, 0, c->recv.len);
#if 0
        mg_rpc_add(&s_rpc_head, mg_str("rpc.list"), mg_rpc_list, &s_rpc_head);

        struct mg_ws_message* wm = (struct mg_ws_message*)ev_data;
        struct mg_iobuf io = { 0, 0, 0, 512 };
        struct mg_rpc_req r = { &s_rpc_head, 0, mg_pfn_iobuf, &io, 0, wm->data };
        mg_rpc_process(&r);
        if (io.buf) mg_ws_send(c, (char*)io.buf, io.len, WEBSOCKET_OP_TEXT);
        mg_iobuf_free(&io);
#endif
    }
    else if (ev == MG_EV_WAKEUP) {
        struct mg_str* data = (struct mg_str*)ev_data;
        // Broadcast message to all connected websocket clients.
        // Traverse over all connections
        for (struct mg_connection* wc = c->mgr->conns; wc != NULL; wc = wc->next) {
            // Send only to marked connections
            if (wc->data[0] == 'W')
                mg_ws_send(wc, data->ptr, data->len, WEBSOCKET_OP_TEXT);
        }
    }

}

int TFlowMg::sendMsgToMg(const char* component, const json11::Json::object &j_params)
{
    json11::Json j_msg = json11::Json::object{
        { component , j_params }
    };

    std::string j_msg_dump = j_msg.dump();
    write(pipe_fd_tflow2mg[1], j_msg_dump.c_str(), j_msg_dump.length() + 1);
    return 0;
}

int TFlowMg::onMsgFromMg()
{
    ssize_t res;
    int err;

    // Read-out all data from the mongoose pipe
    res = read(pipe_fd_mg2tflow[0], &mg_in_msg, sizeof(mg_in_msg)-1);
    err = errno;

    if (res <= 0) {
        if (err == EPIPE || err == ECONNREFUSED || err == ENOENT) {
            g_warning("TFlowMg: TFlow Mongoose closed");
        }
        else {
            g_warning("TFlowMg: unexpected error (%ld, %d) - %s",
                res, err, strerror(err));
        }

        return -1;
    }

    mg_in_msg[res] = 0;

    // Parse input msg to Json. Pass Json to ... 
    std::string j_err;
    const json11::Json j_in_msg = json11::Json::parse(mg_in_msg, j_err);

    if (j_in_msg.is_null()) {
        snprintf(mg_out_msg, sizeof(mg_out_msg) - 1, "{\"X3\":\"X3\"}");
        g_warning("TFlowMG: bad http request - %s", j_err.c_str());
        res = write(pipe_fd_tflow2mg[1], &mg_in_msg, sizeof(mg_in_msg) - 1);
        return 0;
    }

    auto del_me = j_in_msg.dump();

    const json11::Json http_req_playback  = j_in_msg["playback"];
    const json11::Json http_req_mvision   = j_in_msg["mvision"];
    const json11::Json http_req_capture   = j_in_msg["capture"];
    //const json11::Json http_req_preview   = j_in_msg["preview"];
    //const json11::Json http_req_recording = j_in_msg["recording"];

    if (http_req_capture.is_object()) {
        TFlowCtrlCli& cli = app->tflow_ctrl_clis.at(TFlowControl::SRV_NAME_CAPTURE);
        if (cli.sck_state_flag.v != Flag::SET) {
            // CtrlServer isn't connected
            sendMsgToMg("capture", json11::Json::object({ { "state", "off" } }));
            return 0;
        }

        cli.sendMsgToCtrl("capture", http_req_playback.object_items());
        // Request will be processed by a tflow-ctrl-capture 
    }


    if (http_req_playback.is_object()) {
        TFlowCtrlCli &cli = app->tflow_ctrl_clis.at(TFlowControl::SRV_NAME_PROCESS);
        if (cli.sck_state_flag.v != Flag::SET) {
            // CtrlServer isn't connected
            sendMsgToMg("playback", json11::Json::object({ { "state", "off" } }));
            return 0;
        }

        cli.sendMsgToCtrl("player", http_req_playback.object_items());
    } 

    if (http_req_mvision.is_object()) {
        TFlowCtrlCli& cli = app->tflow_ctrl_clis.at(TFlowControl::SRV_NAME_PROCESS);
        cli.sendMsgToCtrl("config", http_req_playback.object_items());
    }


    // Does Mongoose provide multiple parrallel requests at a time?
    // If so, as we don't support real multi-user environment yet, lets block
    // the further MG message processing until response received or timeout
    // triggered.
    // TODO: ^^^    

    return 0;
}

gboolean tflow_mg_fifo_dispatch(GSource* g_source, GSourceFunc callback, gpointer user_data)
{
    int rc;
    TFlowMg::GSourceMg* source = (TFlowMg::GSourceMg*)g_source;
    TFlowMg* mg = source->mg;

    g_info("TFlowCtrlCli: Incoming message");

    rc = mg->onMsgFromMg();

    if (rc) {
        // Critical error on PIPE. 
        return G_SOURCE_REMOVE;
    }
    else {
        return G_SOURCE_CONTINUE;
    }
   
}

void* TFlowMg::_thread(void* ctx)
{
    TFlowMg* m = (TFlowMg*)ctx;

    /* Mongoose main thread */
    mg_mgr_init(&m->mgr);           // Initialise event manager
    mg_log_set(MG_LL_DEBUG);        // Set debug log level
    mg_http_listen(&m->mgr, "http://0.0.0.0:8000", m->_on_msg, (void*)&m->mg_data);
    mg_wakeup_init(&m->mgr);        // Initialise wakeup socket pair
    for (;;) {                      // Event loop
        mg_mgr_poll(&m->mgr, 1000);
    }
    mg_mgr_free(&m->mgr);

    return nullptr;
}

TFlowMg::TFlowMg(TFlowControl* _app)
{
    int rc;
    app = _app;

    pipe_tag = NULL;
    pipe_src = NULL;
    CLEAR(pipe_gsfuncs);

    last_idle_check = 0;

    rc = pipe2(pipe_fd_mg2tflow, O_NONBLOCK);
    if (rc) {
        g_warning("TFlow error in Mongoose initiation (mg2tflow)");
        return;
    }
    mg_data.wr_fd = pipe_fd_mg2tflow[1];

    rc = pipe2(pipe_fd_tflow2mg, O_NONBLOCK);
    if (rc) {
        g_warning("TFlow error in Mongoose initiation (tflow2mg)");
        return;
    }
    mg_data.rd_fd = pipe_fd_tflow2mg[0];

    /* Assign g_source on the socket */
    pipe_gsfuncs.dispatch = tflow_mg_fifo_dispatch;
    pipe_src = (GSourceMg*)g_source_new(&pipe_gsfuncs, sizeof(GSourceMg));
    pipe_tag = g_source_add_unix_fd((GSource*)pipe_src, pipe_fd_mg2tflow[0], (GIOCondition)(G_IO_IN /* | G_IO_ERR  | G_IO_HUP */));
    pipe_src->mg = this;
    g_source_attach((GSource*)pipe_src, app->context);

    /* Create mongoose thread */
    int ret;
    pthread_attr_t attr;

    pthread_cond_init(&th_cond, NULL);
    pthread_attr_init(&attr);

    ret = pthread_create(&th, &attr, _thread, this);
    pthread_attr_destroy(&attr);
}

TFlowMg::~TFlowMg()
{
    if (pipe_fd_mg2tflow[0] != -1) {
        close(pipe_fd_mg2tflow[0]);
    }
    if (pipe_fd_mg2tflow[1] != -1) {
        close(pipe_fd_mg2tflow[1]);
    }

    if (pipe_src) {
        if (pipe_tag) {
            g_source_remove_unix_fd((GSource*)pipe_src, pipe_tag);
            pipe_tag = nullptr;
        }
        g_source_destroy((GSource*)pipe_src);
        g_source_unref((GSource*)pipe_src);
        pipe_src = nullptr;
    }

    // Close Mongoose thread?
    // Send close signal
    // Wait a while
    // Force close if still running
}

