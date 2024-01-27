#include <giomm.h>
#include <glib-unix.h>
#include <thread>
#include <signal.h>

#include "tflow-control.h"

TFlowControl *gp_app;

gboolean handle_signal(gpointer ctx)
{
    g_info("Got INT or TERM signal, terminating...");

    TFlowControl *app = (TFlowControl*)ctx;
    g_main_loop_quit(app->main_loop);

    return true;
}


static void setup_sig_handlers()
{
    GSource* src_sigint, * src_sigterm;

    src_sigint = g_unix_signal_source_new(SIGINT);
    src_sigterm = g_unix_signal_source_new(SIGTERM);

    g_source_set_callback(src_sigint, (GSourceFunc)handle_signal, gp_app, NULL);
    g_source_set_callback(src_sigterm, (GSourceFunc)handle_signal, gp_app, NULL);

    g_source_attach(src_sigint, gp_app->context);
    g_source_attach(src_sigterm, gp_app->context);

    g_source_unref(src_sigint);
    g_source_unref(src_sigterm);
}

int main(int argc, char** argv)
{
    Gio::init();

    g_info("TFlow Control Started");

    gp_app = new TFlowControl;

    setup_sig_handlers();

    // Block SIGPIPE signal
    signal(SIGPIPE, SIG_IGN);

    gp_app->AttachIdle();

#if CODE_BROWSE
    gp_app->OnIdle();   
#endif

    g_main_loop_run(gp_app->main_loop);

    delete gp_app;

    g_info("App thread exited");

    return 0;
}
