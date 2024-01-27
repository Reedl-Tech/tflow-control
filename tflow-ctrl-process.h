#pragma once 

#include <stdint.h>
#include <vector>

#include "tflow-ctrl.h"

#define TFLOW_CMD_EOMSG .eomsg = {.name = nullptr, .type = CFT_LAST, .max_len = 0, .v = {.u32 = 0} }

class TFlowCtrlProcess;
static int _cmd_cb_sign      (TFlowCtrlProcess* obj, Json& json);
static int _cmd_cb_config    (TFlowCtrlProcess* obj, Json& json);
static int _cmd_cb_set_as_def(TFlowCtrlProcess* obj, Json& json);

class TFlowProcess;
class TFlowCtrlProcess : private TFlowCtrl<TFlowCtrlProcess> {
public:

    TFlowCtrlProcess(TFlowProcess& app);
    void Init();

    int algo_param_1_get();
    int state_get();

    struct tflow_cmd_flds_sign {
        tflow_cmd_field_t   eomsg;
    } cmd_flds_sign = {
        TFLOW_CMD_EOMSG
    };

    struct tflow_cmd_flds_config {
        tflow_cmd_field_t   state;
        tflow_cmd_field_t   algo_param_1;
        tflow_cmd_field_t   eomsg;
    } cmd_flds_config = {
        .state        = { "state",        CFT_NUM, 0, {.u32 = 0} },
        .algo_param_1 = { "algo_param_1", CFT_TXT, 0, {.u32 = 0} },
        TFLOW_CMD_EOMSG
    };

    struct tflow_cmd_flds_set_as_def {
        tflow_cmd_field_t   eomsg;
    } cmd_flds_set_as_def = {
        TFLOW_CMD_EOMSG
    };

    int cmd_cb_sign(Json& json);
    int cmd_cb_config(Json& json);
    int cmd_cb_set_as_def(Json& json);

#define TFLOW_PROCESS_RPC_CMD_SIGN       0
#define TFLOW_PROCESS_RPC_CMD_CONFIG     1
#define TFLOW_PROCESS_RPC_CMD_SET_AS_DEF 2
#define TFLOW_PROCESS_RPC_CMD_LAST       3

    tflow_cmd_t ctrl_process_rpc_cmds[TFLOW_PROCESS_RPC_CMD_LAST + 1] = {
        [TFLOW_PROCESS_RPC_CMD_SIGN      ] = { "sign",        (tflow_cmd_field_t*)&cmd_flds_sign,       _cmd_cb_sign },
        [TFLOW_PROCESS_RPC_CMD_CONFIG    ] = { "config",      (tflow_cmd_field_t*)&cmd_flds_config,     _cmd_cb_config },
        [TFLOW_PROCESS_RPC_CMD_SET_AS_DEF] = { "set_as_def",  (tflow_cmd_field_t*)&cmd_flds_set_as_def, _cmd_cb_set_as_def },
        [TFLOW_PROCESS_RPC_CMD_LAST] = { nullptr , nullptr, nullptr }
    };

private:

    TFlowProcess& app;      // AV: Why?

    const char* cfg_fname = "tflow-process-config.json";
};
