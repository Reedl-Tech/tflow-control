#include <sys/stat.h>
#include <glib-unix.h>

#include <json11.hpp>
#include "tflow-ctrl-process.h"

using namespace json11;

static const char *raw_cfg_default =  R"( 
    {"algo_param_1" : "xz"} 
)";

static int _cmd_cb_sign      (TFlowCtrlProcess* obj, Json& json) { return obj->cmd_cb_sign(json);       }
static int _cmd_cb_config    (TFlowCtrlProcess* obj, Json& json) { return obj->cmd_cb_config(json);     }
static int _cmd_cb_set_as_def(TFlowCtrlProcess* obj, Json& json) { return obj->cmd_cb_set_as_def(json); }

TFlowCtrlProcess::TFlowCtrlProcess(TFlowProcess& parent) :
    app(parent)
{
    
}

void TFlowCtrlProcess::Init()
{
    struct stat sb;
    int cfg_fd = -1;
    bool use_default_cfg = 0;
    Json json_cfg;
    
    cfg_fd = open(cfg_fname, O_RDWR);

    if (fstat(cfg_fd, &sb) < 0) {
        g_warning("Can't open configuration file %s", cfg_fname);
        use_default_cfg = true;
    }
    else if (!S_ISREG(sb.st_mode)) {
        g_warning("Config name isn't a file %s", cfg_fname);
        use_default_cfg = true;
    }

    if (!use_default_cfg) {
        char* raw_cfg = (char*)g_malloc(sb.st_size);
        int bytes_read = read(cfg_fd, raw_cfg, sb.st_size);
        if (bytes_read != sb.st_size) {
            g_warning("Can't read config file %s", cfg_fname);
            use_default_cfg = true;
        }

        if (!use_default_cfg) {
            std::string err;
            json_cfg = Json::parse(raw_cfg, err);
            if (json_cfg.is_null()) {
                g_warning("Error in JSON format - %s\n%s", (char*)err.data(), raw_cfg);
                use_default_cfg = true;
            }
        }
        free(raw_cfg);
        close(cfg_fd);
    }

    if (use_default_cfg) {
        std::string err;
        json_cfg = Json::parse(raw_cfg_default, err);
    }

    set_cmd_fields((tflow_cmd_field_t*)&cmd_flds_config, json_cfg);
}

int TFlowCtrlProcess::algo_param_1_get()
{
    return (int)cmd_flds_config.algo_param_1.v.u32;
}

int TFlowCtrlProcess::state_get()
{
    return (int)cmd_flds_config.state.v.u32;
}

/*********************************/
/*** Application specific part ***/
/*********************************/

int TFlowCtrlProcess::cmd_cb_set_as_def(Json& json_cfg)
{
    return 0;
}

int TFlowCtrlProcess::cmd_cb_sign(Json& in_params)
{
    return 0;
}

int TFlowCtrlProcess::cmd_cb_config(Json &in_params)
{
    g_info("Config command\n    params:\t");

    int rc = set_cmd_fields((tflow_cmd_field_t*)&cmd_flds_config, in_params);

    if (rc != 0) return -1;

    return 0;
}


