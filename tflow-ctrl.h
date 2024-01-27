#pragma once 

#include <stdint.h>
#include <vector>
#include <json11.hpp>

using namespace json11;

#define TFLOW_CMD_EOMSG .eomsg = {.name = nullptr, .type = CFT_LAST, .max_len = 0, .v = {.u32 = 0} }

template <class T> class TFlowCtrl {
public:

    TFlowCtrl();

    typedef enum {
        CFT_NUM,
        CFT_TXT,
        CFT_RESERVED = 4,
        CFT_LAST = -1
    } tflow_cmd_field_type_t;

    typedef struct {
        const char* name;
        tflow_cmd_field_type_t type;

        int max_len;
        union {
            uint32_t u32;
            char* str;
        } v;
    } tflow_cmd_field_t;

    typedef struct {
        const char* name;
        tflow_cmd_field_t* fields;
        int (*cb)(T*, Json& in_params);
    } tflow_cmd_t;

    int set_cmd_fields(tflow_cmd_field_t* cmd_field, Json& in_params);
private:
    int set_field(tflow_cmd_field_t* cmd_field, Json& in_param);
};

template<class T>
TFlowCtrl<T>::TFlowCtrl()
{
}

template<class T>
int TFlowCtrl<T>::set_cmd_fields(tflow_cmd_field_t* in_cmd_fields, Json& in_params)
{
    // Loop over all config command fields and check json_cfg
    tflow_cmd_field_t* cmd_field = in_cmd_fields;

    while (cmd_field->name != nullptr) {

        Json in_field_param = in_params[cmd_field->name];
        if (!in_field_param.is_null()) {
            // Configuration parameter is found in Json config
            // Check field type is valid
            if (0 != set_field(cmd_field, in_field_param)) {
                return -1;
            }
        }
        cmd_field++;
    }
    return 0;

}

template<class T>
int TFlowCtrl<T>::set_field(tflow_cmd_field_t* cmd_field, Json& cfg_param)
{
    switch (cmd_field->type) {
    case CFT_TXT:
        if (!cfg_param.is_string()) return -1;
        if (cmd_field->v.str) free(cmd_field->v.str);
        cmd_field->v.str = strdup(cfg_param.string_value().data());
        return 0;
    case CFT_NUM:
        if (!cfg_param.is_number()) {
            cmd_field->v.u32 = cfg_param.number_value();
        }
        else if (!cfg_param.is_string()) {
            cmd_field->v.u32 = atoi(cfg_param.string_value().data());
        }
        else if (!cfg_param.is_bool()) {
            cmd_field->v.u32 = cfg_param.bool_value();
        }
        else {
            return -1;
        }
        return 0;
    default:
        g_info("Ooops... at %s (%d) Data type mismatch. Field name: %s (%d != %d)", __FILE__, __LINE__,
            cmd_field->name, cmd_field->type, cfg_param.type());
    }

    return -1;
}
