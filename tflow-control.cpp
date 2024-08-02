#include <iostream>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <giomm.h>
#include <glib-unix.h>
#include <json11.hpp>

#include "tflow-control.h"

#define  IDLE_INTERVAL_MSEC 100

TFlowControl::TFlowControl()
{
    context = g_main_context_new();
    g_main_context_push_thread_default(context);
    
    main_loop = g_main_loop_new(context, false);

    // TFlowControl::SRV_NAME
    tflow_ctrl_clis.reserve(3);
    tflow_ctrl_clis.emplace_back(this, (const char*)"Capture"); 
    tflow_ctrl_clis.emplace_back(this, (const char*)"Process");
    tflow_ctrl_clis.emplace_back(this, (const char*)"VStream");
    

    tflow_mg = new TFlowMg(this);
}

TFlowControl::~TFlowControl()
{
    if (tflow_mg) delete tflow_mg;

    g_main_loop_unref(main_loop);
    main_loop = NULL;

    g_main_context_pop_thread_default(context);
    g_main_context_unref(context);
    context = NULL;
}


static gboolean tflow_control_idle(gpointer data)
{
    TFlowControl* app = (TFlowControl*)data;
    app->OnIdle();

    return true;
}

void TFlowControl::OnIdle()
{
    struct timespec now_tp;
    clock_gettime(CLOCK_MONOTONIC, &now_tp);

    // TODO: Chek inet connection 
    //       If OK start MG server
    for (auto& cli : tflow_ctrl_clis) {
        cli.onIdle(&now_tp);
    }

}

void TFlowControl::AttachIdle()
{
    GSource* src_idle = g_timeout_source_new(IDLE_INTERVAL_MSEC);
    g_source_set_callback(src_idle, (GSourceFunc)tflow_control_idle, this, nullptr);
    g_source_attach(src_idle, context);
    g_source_unref(src_idle);

    return;
}
