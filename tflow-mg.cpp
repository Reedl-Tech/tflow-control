#include <unistd.h>
#include <fcntl.h>

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

static void handle_stats_get(struct mg_connection *c) {
  int points[] = {21, 22, 22, 19, 18, 20, 23, 23, 22, 22, 22, 23, 22};

  // Send commands to TFlowCtrl 
  // Wait for reply with timeout

  mg_http_reply(c, 200, s_json_header, "{%m:%d,%m:%d,%m:[%M]}\n",
                MG_ESC("temperature"), 11,  //
                MG_ESC("humidity"), 22,     //
                MG_ESC("points"), print_int_arr,
                sizeof(points) / sizeof(points[0]), points);
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
        } else if (mg_http_match_uri(hm, "/api/stats/get")) {
            handle_stats_get(c);
        } else {
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

int TFlowMg::onMsg()
{
    ssize_t res;
    int err;

    // Read-out all data from the mongoose pipe
    res = read(pipe_fd[0], &mg_in_msg, sizeof(mg_in_msg)-1);
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

    return 0;
}

gboolean tflow_mg_fifo_dispatch(GSource* g_source, GSourceFunc callback, gpointer user_data)
{
    int rc;
    TFlowMg::GSourceMg* source = (TFlowMg::GSourceMg*)g_source;
    TFlowMg* mg = source->mg;

    g_info("TFlowCtrlCli: Incoming message");

    rc = mg->onMsg();

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
    mg_http_listen(&m->mgr, "http://192.168.2.2:8000", m->_on_msg, (void*)&m->mg_data);
    mg_wakeup_init(&m->mgr);  // Initialise wakeup socket pair
    for (;;) {             // Event loop
        mg_mgr_poll(&m->mgr, 1000);
    }
    mg_mgr_free(&m->mgr);

    return nullptr;
}

TFlowMg::TFlowMg(TFlowControl* _app)
{
    app = _app;

    pipe_tag = NULL;
    pipe_src = NULL;
    CLEAR(pipe_gsfuncs);

    last_idle_check = 0;

    int rc = pipe2(pipe_fd, O_NONBLOCK);
    if (rc) {
        g_warning("TFlow error in Mongoose initiation");
        return;
    }
    mg_data.wr_fd = pipe_fd[1];

    /* Assign g_source on the socket */
    pipe_gsfuncs.dispatch = tflow_mg_fifo_dispatch;
    pipe_src = (GSourceMg*)g_source_new(&pipe_gsfuncs, sizeof(GSourceMg));
    pipe_tag = g_source_add_unix_fd((GSource*)pipe_src, pipe_fd[0], (GIOCondition)(G_IO_IN /* | G_IO_ERR  | G_IO_HUP */));
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
    if (pipe_fd[0] != -1) {
        close(pipe_fd[0]);
    }
    if (pipe_fd[1] != -1) {
        close(pipe_fd[1]);
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

