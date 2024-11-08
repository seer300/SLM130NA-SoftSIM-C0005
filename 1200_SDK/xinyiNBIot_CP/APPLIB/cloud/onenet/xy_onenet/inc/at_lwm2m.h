#ifndef _AT_LWM2M_H
#define _AT_LWM2M_H

#define XY_LWM2M_SUCCESS 0
#define XY_LWM2M_TIMEOUT 1
#define XY_LWM2M_PKTNOTSENT 2
#define XY_LWM2M_RECOVERFAILED 3
#define XY_LWM2M_UPDATEFAILED 4
#define XY_LWM2M_RST 5

#define XY_LWM2M_ERROR 900


typedef struct lwm2m_common_user_config_nvm_s
{
    uint8_t server_host[102];
    uint8_t endpoint_name[62];
    uint8_t psk_id[21];
    uint8_t psk[21];
    uint16_t port;
    uint32_t lifetime;
    uint8_t bootstrap_flag;
    uint8_t binding_mode;
    uint8_t security_mode;
    uint8_t ack_timeout;
    uint8_t retrans_max_times;
    uint8_t is_auto_ack;
    uint8_t access_mode;
    uint8_t access_mode_alternative;
    uint8_t platform;

    uint8_t lifetime_enable;
    uint8_t dtls_mode;
    uint8_t dtls_version;
    uint8_t lwm2m_recovery_mode;

    uint8_t device_type;        //andlink 设备类型
    uint8_t al_bs_flag;         //andlink bs模式
} lwm2m_common_user_config_nvm_t;
extern lwm2m_common_user_config_nvm_t *g_lwm2m_common_config_data;

typedef enum
{
    XY_LWM2M_CACHED_URC_TYPE_READ = 0,
    XY_LWM2M_CACHED_URC_TYPE_WRITE,
    XY_LWM2M_CACHED_URC_TYPE_EXECUTE,
    XY_LWM2M_CACHED_URC_TYPE_OBSERVE,
    XY_LWM2M_CACHED_URC_TYPE_LIFETIME_CHANGED,
    XY_LWM2M_CACHED_URC_TYPE_BS_FINISHED,
    XY_LWM2M_CACHED_URC_TYPE_FOTA_END
} xy_lwm2m_cached_urc_type_e;

typedef struct xy_lwm2m_cached_urc_common_s
{
    struct xy_lwm2m_cached_urc_common_s *next;
    char urc_type;
    char *urc_data;
} xy_lwm2m_cached_urc_common_t;

// typedef struct
// {
//     xy_lwm2m_cached_urc_common_t *next;
//     char urc_type;
//     short objectId;
//     short instanceId;
//     short resourceId;
// } xy_lwm2m_cached_urc_read_t;

// typedef struct
// {
//     xy_lwm2m_cached_urc_common_t *next;
//     char urc_type;
//     char value_type;
//     short objectId;
//     short instanceId;
//     short resourceId;

//     short len;
//     short index;
//     union
//     {
//         void *value_ptr;
//         int value_int;
//         double value_float;
//     };

// } xy_lwm2m_cached_urc_write_t;

// typedef xy_lwm2m_cached_urc_read_t xy_lwm2m_cached_urc_execute_t;

// typedef struct
// {
//     xy_lwm2m_cached_urc_common_t *next;
//     char urc_type;
//     char flag;
//     short objectId;
//     short instanceId;
//     short resourceId;
// } xy_lwm2m_cached_urc_observe_t;

// typedef struct
// {
//     xy_lwm2m_cached_urc_common_t *next;
//     char urc_type;
//     int value;
// } xy_lwm2m_cached_urc_other_t;

typedef struct
{
    xy_lwm2m_cached_urc_common_t *first;
    xy_lwm2m_cached_urc_common_t *last;
    int num;
} xy_lwm2m_cached_urc_head_t;

extern xy_lwm2m_cached_urc_head_t *xy_lwm2m_cached_urc_head;

#define MAX_RESOURCE_COUNT 14

typedef struct xy_lwm2m_object_info_s
{
    struct xy_lwm2m_object_info_s *next;
    int obj_id;
    int instance_id;
    int resource_count;
    int resouce_ids[MAX_RESOURCE_COUNT];
} xy_lwm2m_object_info_t;

typedef struct
{
    xy_lwm2m_object_info_t *first;
    int num;
} xy_lwm2m_object_info_head_t;



int insert_cached_urc_node(xy_lwm2m_cached_urc_common_t *node);

int at_proc_qlaconfig_req(char *at_buf, char **rsp_cmd);
int at_proc_qlacfg_req(char *at_buf, char **rsp_cmd);
int at_proc_qlareg_req(char *at_buf, char **rsp_cmd);
int at_proc_qlaupdate_req(char *at_buf, char **rsp_cmd);
int at_proc_qladereg_req(char *at_buf, char **rsp_cmd);
int at_proc_qlaaddobj_req(char *at_buf, char **rsp_cmd);
int at_proc_qladelobj_req(char *at_buf, char **rsp_cmd);
int at_proc_qlardrsp_req(char *at_buf, char **rsp_cmd);
int at_proc_qlawrrsp_req(char *at_buf, char **rsp_cmd);
int at_proc_qlaexersp_req(char *at_buf, char **rsp_cmd);
int at_proc_qlaobsrsp_req(char *at_buf, char **rsp_cmd);
int at_proc_qlanotify_req(char *at_buf, char **rsp_cmd);
int at_proc_qlard_req(char *at_buf, char **rsp_cmd);
int at_proc_qlstatus_req(char *at_buf, char **rsp_cmd);
int at_proc_qlarecover_req(char *at_buf, char **rsp_cmd);
int at_proc_qlasend_req(char *at_buf, char **prsp_cmd);

extern xy_lwm2m_object_info_head_t *xy_lwm2m_object_info_head;


#endif
