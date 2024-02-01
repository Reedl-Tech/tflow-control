#include <iostream>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <giomm.h>
#include <glib-unix.h>
#include <json11.hpp>

#include "tflow-control.h"

TFlowControl::TFlowControl() 
{
    context = g_main_context_new();
    g_main_context_push_thread_default(context);
    
    main_loop = g_main_loop_new(context, false);

    tflow_ctrl_clis.reserve(2);
    tflow_ctrl_clis.emplace_back(this, (const char*)"Capture");
    tflow_ctrl_clis.emplace_back(this, (const char*)"Process");
}

TFlowControl::~TFlowControl()
{
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
    clock_t now = clock();

    for (auto& cli : tflow_ctrl_clis) {
        cli.onIdle(now);
    }

}

void TFlowControl::AttachIdle()
{
    GSource* src_idle = g_idle_source_new();
    g_source_set_callback(src_idle, (GSourceFunc)tflow_control_idle, this, nullptr);
    g_source_attach(src_idle, context);
    g_source_unref(src_idle);

    return;
}
