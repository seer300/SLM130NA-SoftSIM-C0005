
#if MOBILE_VER && LWM2M_COMMON_VER
#include "xy_utils.h"
#include "at_onenet.h"
#include "xy_cis_api.h"
#include "xy_at_api.h"
#include "xy_system.h"
#include "oss_nv.h"
#include "cloud_utils.h"
#include "xy_net_api.h"
#include "../inc/at_lwm2m.h"
#include "net_app_resume.h"

#include "lwip/sockets.h"
#include "lwip/opt.h"
#include "lwip/netdb.h"

lwm2m_common_user_config_nvm_t *g_lwm2m_common_config_data = NULL;
xy_lwm2m_cached_urc_head_t *xy_lwm2m_cached_urc_head = NULL;
xy_lwm2m_object_info_head_t *xy_lwm2m_object_info_head = NULL;

extern onenet_context_config_t onenet_context_configs[CIS_REF_MAX_NUM];
extern onenet_context_reference_t onenet_context_refs[CIS_REF_MAX_NUM];
extern osMutexId_t g_onenet_mutex;
extern osSemaphoreId_t cis_poll_sem;
extern cis_ret_t onet_miplread_req(st_context_t *onenet_context, struct onenet_read *param);
extern char *cis_cfg_tool(char *ip, unsigned int port, char is_bs, char *authcode, char is_dtls, char *psk, int *cfg_out_len);
extern uint16_t  g_cislwm2m_inited;
extern osMutexId_t g_onenet_module_init_mutex;
extern bool onet_at_get_notify_value(int value_type, int value_len, char *at_value, char *value);

#define INIT_NULL_PTR(ptr) char *ptr = NULL

#define GOTOERRORIFNULL(param) \
    do                         \
    {                          \
        if (param == NULL)     \
            goto error;        \
    } while (0)

#define FREEIFNOTNULL(param) \
    do                       \
    {                        \
        if (param)           \
        {                    \
            xy_free(param);  \
            param = NULL;    \
        }                    \
    } while (0)

int is_xy_lwm2m_running()
{
    onenet_context_reference_t *onenet_context_ref = NULL;
    onenet_context_ref = &onenet_context_refs[0];
    if (onenet_context_ref->onenet_context == NULL || onenet_context_ref->onet_at_thread_id == NULL)
    {
        return 0;
    }
    return 1;
}

int xy_lwm2m_deinit(onenet_context_reference_t *onenet_context_ref)
{
    if(onenet_context_ref->onenet_context != NULL)
        onenet_context_ref->thread_quit = 1;
    return 0;
}

int xy_lwm2m_init(onenet_context_reference_t *onenet_context_ref, onenet_context_config_t *onenet_context_config)
{
    int ret = 0;

    ret = cis_init_common((void **)&onenet_context_ref->onenet_context, onenet_context_config->config_hex, onenet_context_config->total_len, NULL);
    if (ret != CIS_RET_OK)
    {
        xy_lwm2m_deinit(onenet_context_ref);
        return -1;
    }
    onenet_context_ref->onenet_context_config = onenet_context_config;
    return 0;
}

void init_xy_lwm2m_cached_urc_list()
{
    if (xy_lwm2m_cached_urc_head != NULL)
    {
        clear_xy_lwm2m_cached_urc_list();
    }
    xy_lwm2m_cached_urc_head = xy_malloc(sizeof(xy_lwm2m_cached_urc_head_t));
    memset(xy_lwm2m_cached_urc_head, 0, sizeof(xy_lwm2m_cached_urc_head_t));
}

void clear_xy_lwm2m_cached_urc_list()
{
    if (xy_lwm2m_cached_urc_head == NULL)
    {
        return;
    }
    xy_lwm2m_cached_urc_common_t *node = xy_lwm2m_cached_urc_head->first;
    xy_lwm2m_cached_urc_common_t *temp_node;
    while (node != NULL)
    {
        temp_node = node->next;
        FREEIFNOTNULL(node->urc_data);
        FREEIFNOTNULL(node);
        node = temp_node;
    }

    FREEIFNOTNULL(xy_lwm2m_cached_urc_head);
}

int urc_exists(xy_lwm2m_cached_urc_common_t *node)
{
    xy_lwm2m_cached_urc_common_t *temp = xy_lwm2m_cached_urc_head->first;
    while (temp)
    {
        if (temp->urc_type == node->urc_type && strcmp(temp->urc_data, node->urc_data) == 0)
        {
            return 1;
        }
        temp = temp->next;
    }
    return 0;
}

int insert_cached_urc_node(xy_lwm2m_cached_urc_common_t *node)
{
    if (xy_lwm2m_cached_urc_head == NULL)
    {
        return -1;
    }
    if (xy_lwm2m_cached_urc_head->last)
    {
        if (urc_exists(node))
            return 0;
        xy_lwm2m_cached_urc_head->last->next = node;
        xy_lwm2m_cached_urc_head->last = node;
        xy_lwm2m_cached_urc_head->num++;
    }
    else
    {
        xy_lwm2m_cached_urc_head->first = xy_lwm2m_cached_urc_head->last = node;
        xy_lwm2m_cached_urc_head->num = 1;
        char *at_str = xy_malloc(32);
        snprintf(at_str, 32, "+QLAURC: \"buffer\"");
        send_urc_to_ext(at_str, strlen(at_str));
        xy_free(at_str);
    }
    return 0;
}

xy_lwm2m_cached_urc_common_t *get_and_remove_cached_urc_first_node()
{
    if (xy_lwm2m_cached_urc_head->num == 0)
    {
        return NULL;
    }
    xy_lwm2m_cached_urc_common_t *node = xy_lwm2m_cached_urc_head->first;

    if (xy_lwm2m_cached_urc_head->num == 1)
    {
        xy_lwm2m_cached_urc_head->first = xy_lwm2m_cached_urc_head->last = NULL;
    }
    else
    {
        xy_lwm2m_cached_urc_head->first = xy_lwm2m_cached_urc_head->first->next;
    }
    xy_lwm2m_cached_urc_head->num--;
    return node;
}

void init_xy_lwm2m_object_info_list()
{
    if (xy_lwm2m_object_info_head != NULL)
    {
        clear_xy_lwm2m_object_info_list();
    }
    xy_lwm2m_object_info_head = xy_malloc(sizeof(xy_lwm2m_object_info_head_t));
    memset(xy_lwm2m_object_info_head, 0, sizeof(xy_lwm2m_object_info_head_t));
}

void clear_xy_lwm2m_object_info_list()
{
    if (xy_lwm2m_object_info_head == NULL)
    {
        return;
    }
    xy_lwm2m_object_info_t *node = xy_lwm2m_object_info_head->first;
    xy_lwm2m_object_info_t *temp_node;
    while (node != NULL)
    {
        temp_node = node->next;
        FREEIFNOTNULL(node);
        node = temp_node;
    }

    FREEIFNOTNULL(xy_lwm2m_object_info_head);
}

int insert_object_info_node(xy_lwm2m_object_info_t *node)
{
    if (xy_lwm2m_object_info_head == NULL)
    {
        return -1;
    }
    node->next = xy_lwm2m_object_info_head->first;
    xy_lwm2m_object_info_head->first = node;
    xy_lwm2m_object_info_head->num++;
    return 0;
}

int remove_object_info_node(int obj_id)
{
    if (xy_lwm2m_object_info_head->num == 0)
    {
        return -1;
    }
    xy_lwm2m_object_info_t *node = xy_lwm2m_object_info_head->first;
    xy_lwm2m_object_info_t *prev = NULL;

    while (node)
    {
        if (node->obj_id == obj_id)
        {
            if (prev)
                prev->next = node->next;
            else
                xy_lwm2m_object_info_head->first = node->next;
            xy_free(node);
            xy_lwm2m_object_info_head->num--;
        }
        prev = node;
        node = node->next;
    }

    return -1;
}

void report_recover_result(int result)
{
    if(result == RESUME_SUCCEED)
        send_urc_to_ext("+QLAURC: \"recovered\",0", strlen("+QLAURC: \"recovered\",0"));
    else if(result == RESUME_LIFETIME_TIMEOUT || result == RESUME_STATE_ERROR)
        send_urc_to_ext("+QLAURC: \"recovered\",3", strlen("+QLAURC: \"recovered\",3"));
    else
        (void) result;
}

void init_xy_lwm2m_lists()
{
    init_xy_lwm2m_cached_urc_list();
    init_xy_lwm2m_object_info_list();
}

void init_xy_lwm2m_config()
{
    g_lwm2m_common_config_data->lifetime = 86400;
    g_lwm2m_common_config_data->binding_mode = 1;

    g_lwm2m_common_config_data->ack_timeout = 2;
    g_lwm2m_common_config_data->retrans_max_times = 5;

    g_lwm2m_common_config_data->lifetime_enable = 1;
}

void clear_xy_lwm2m_config()
{
    clear_xy_lwm2m_cached_urc_list();
    clear_xy_lwm2m_object_info_list();
}

void generate_config_hex()
{
    onenet_context_config_t *onenet_context_config = &onenet_context_configs[0];
    FREEIFNOTNULL(onenet_context_config->config_hex);

    onenet_context_config->config_hex = cis_cfg_tool(g_lwm2m_common_config_data->server_host, g_lwm2m_common_config_data->port, g_lwm2m_common_config_data->bootstrap_flag, NULL,
                          g_lwm2m_common_config_data->security_mode == 0 ? 1 : 0, g_lwm2m_common_config_data->security_mode == 0 ? g_lwm2m_common_config_data->psk : NULL, &onenet_context_config->total_len);

    onenet_context_config->offset = onenet_context_config->total_len;
    onenet_context_config->index = 0;
}

int convert_cis_event_to_common_urc(char *at_str, int max_len, int cis_eid, void *param)
{
    switch (cis_eid)
    {
    case CIS_EVENT_REG_SUCCESS:
        snprintf(at_str, max_len, "+QLAREG: %d", XY_LWM2M_SUCCESS);
        break;
    case CIS_EVENT_REG_FAILED:
        snprintf(at_str, max_len, "+QLAREG: %d", XY_LWM2M_ERROR);
        break;
    case CIS_EVENT_REG_TIMEOUT:
        snprintf(at_str, max_len, "+QLAREG: %d", XY_LWM2M_TIMEOUT);
        break;
    case CIS_EVENT_UPDATE_SUCCESS:
        if ((int)param == -1)
            snprintf(at_str, max_len, "+QLAURC: \"ping\",%d", XY_LWM2M_SUCCESS);
        else if ((int)param == -2)
        {
            if (g_lwm2m_common_config_data->access_mode == 0)
                snprintf(at_str, max_len, "+QLAURC: \"lifetime_changed\",%d", onenet_context_refs[0].onenet_context->lifetime);
            else
            {
                xy_lwm2m_cached_urc_common_t *xy_lwm2m_cached_urc_common = xy_malloc(sizeof(xy_lwm2m_cached_urc_common_t));
                memset(xy_lwm2m_cached_urc_common, 0, sizeof(xy_lwm2m_cached_urc_common_t));
                xy_lwm2m_cached_urc_common->urc_type = XY_LWM2M_CACHED_URC_TYPE_OBSERVE;
                xy_lwm2m_cached_urc_common->urc_data = xy_malloc(MAX_ONE_NET_AT_SIZE);
                snprintf(xy_lwm2m_cached_urc_common->urc_data, MAX_ONE_NET_AT_SIZE, "\"lifetime_changed\",%d",
                         onenet_context_refs[0].onenet_context->lifetime);
                insert_cached_urc_node(xy_lwm2m_cached_urc_common);
                return -1;
            }
        }
        else if ((int)param == -3)
            snprintf(at_str, max_len, "+QLAURC: \"binding_changed\",%s", onenet_context_refs[0].onenet_context->server->binding == 1 ? "U" : "UQ");
        else
            snprintf(at_str, max_len, "+QLAUPDATE: %d,%d", XY_LWM2M_SUCCESS, (int)param);
        break;
    case CIS_EVENT_UPDATE_FAILED:
        if ((int)param != -1)
            snprintf(at_str, max_len, "+QLAUPDATE: %d,%d", XY_LWM2M_UPDATEFAILED, (int)param);
        else
            snprintf(at_str, max_len, "+QLAURC: \"ping\",%d", XY_LWM2M_UPDATEFAILED);
        break;
    case CIS_EVENT_UPDATE_TIMEOUT:
        if ((int)param != -1)
            snprintf(at_str, max_len, "+QLAUPDATE: %d,%d", XY_LWM2M_TIMEOUT, (int)param);
        else
            snprintf(at_str, max_len, "+QLAURC: \"ping\",%d", XY_LWM2M_TIMEOUT);
        break;
    case CIS_EVENT_UNREG_DONE:
        snprintf(at_str, max_len, "+QLADEREG: %d", XY_LWM2M_SUCCESS);
        xy_lwm2m_deinit(&onenet_context_refs[0]);
        clear_xy_lwm2m_config();
        break;
    case CIS_EVENT_NOTIFY_SUCCESS:
        snprintf(at_str, max_len, "+QLAURC: \"report_ack\",%d,%d", XY_LWM2M_SUCCESS, (int)param);
        break;
    case CIS_EVENT_NOTIFY_FAILED:
        snprintf(at_str, max_len, "+QLANOTIFY: %d", XY_LWM2M_ERROR);
        break;
    case CIS_EVENT_BOOTSTRAP_SUCCESS:
        if (g_lwm2m_common_config_data->access_mode == 0)
            snprintf(at_str, max_len, "+QLAURC: \"bs_finished\"");
        else
        {
            xy_lwm2m_cached_urc_common_t *xy_lwm2m_cached_urc_common = xy_malloc(sizeof(xy_lwm2m_cached_urc_common_t));
            memset(xy_lwm2m_cached_urc_common, 0, sizeof(xy_lwm2m_cached_urc_common_t));
            xy_lwm2m_cached_urc_common->urc_type = XY_LWM2M_CACHED_URC_TYPE_OBSERVE;
            xy_lwm2m_cached_urc_common->urc_data = xy_malloc(MAX_ONE_NET_AT_SIZE);
            snprintf(xy_lwm2m_cached_urc_common->urc_data, MAX_ONE_NET_AT_SIZE, "\"bs_finished\"");
            insert_cached_urc_node(xy_lwm2m_cached_urc_common);
            return -1;
        }
        break;
    default:
        return -1;
        break;
    }

    return 0;
}

int convert_coap_code_to_commond_result_code(int coap_code)
{
    int result_code;
    switch (coap_code)
    {
    case COAP_NO_ERROR:
        result_code = 0;
        break;
    case COAP_231_CONTINUE:
        result_code = 231;
        break;
    case COAP_400_BAD_REQUEST:
        result_code = 400;
        break;
    case COAP_404_NOT_FOUND:
        result_code = 404;
        break;
    case COAP_500_INTERNAL_SERVER_ERROR:
        result_code = 500;
        break;
    default:
        result_code = XY_LWM2M_ERROR;
        break;
    }
    return result_code;
}

cis_ret_t lwm2m_notify_data(st_context_t *onenet_context, struct onenet_notify *param)
{
    cis_data_t tmpdata = {0};
    cis_uri_t uri = {0};
    cis_ret_t ret = 0;
    cis_coapret_t result = CIS_NOTIFY_CONTINUE;

    if (param->flag != 0 && param->index == 0)
    {
        return CIS_RET_PARAMETER_ERR;
    }

    if (param->ackid != 0 && param->ackid != 1)
        return CIS_RET_PARAMETER_ERR;

    uri.objectId = param->objId;
    uri.instanceId = param->insId;
    uri.resourceId = param->resId;
    cis_uri_update(&uri);

    if ((ret = onet_read_param((struct onenet_read *)param, &tmpdata)) != CIS_RET_OK)
    {
        return ret;
    }

    if (param->flag == 0 && param->index == 0)
    {
        result = CIS_NOTIFY_CONTENT;
    }

    ret = cis_notify(onenet_context, &uri, &tmpdata, param->msgId, result, 0, param->ackid, param->raiflag);

    return ret;
}

extern cis_data_t* prv_dataDup(const cis_data_t* src);
cis_ret_t lwm2m_send_data(st_context_t *onenet_context, struct onenet_notify *param)
{
    cis_data_t tmpdata = {0};
    cis_uri_t uri = {0};
    st_notify_t* notify = NULL;

    if (onenet_context == NULL)
    {
        return CIS_RET_PARAMETER_ERR;
    }
    st_context_t* ctx = (st_context_t*)onenet_context;

    uri.objectId = param->objId;
    uri.instanceId = param->insId;
    uri.resourceId = param->resId;
    cis_uri_update(&uri);

    if (onet_read_param((struct onenet_read *)param, &tmpdata) != CIS_RET_OK)
    {
        return CIS_CALLBACK_NOT_FOUND;
    }

    if (ctx->stateStep != PUMP_STATE_READY)
    {
        return CIS_RET_INVILID;
    }

    if(CIS_LIST_COUNT(ctx->notifyList) >= 10)
        return CIS_RET_MEMORY_ERR;

    notify = (st_notify_t*)cis_malloc(sizeof(st_notify_t));
    cissys_assert(notify != NULL);
    if (notify == NULL)
    {
        return CIS_RET_MEMORY_ERR;
    }
    memset(notify, 0, sizeof(st_notify_t));
    notify->isResponse = false;
    notify->next = NULL;
    notify->id = ++ctx->nextNotifyId;
    notify->mid = param->msgId;
    notify->result = CIS_NOTIFY_CONTENT;
    notify->value = NULL;
    notify->ackID = param->ackid;
    notify->raiflag = param->raiflag;
    notify->report_type = 1;

    notify->value = prv_dataDup(&tmpdata);
    if (notify->value == NULL)
    {
        cis_free(notify);
        return CIS_RET_MEMORY_ERR;
    }
    notify->uri = uri;

    cloud_mutex_lock(&ctx->lockNotify, MUTEX_LOCK_INFINITY);
    ctx->notifyList = (st_notify_t*)CIS_LIST_ADD(ctx->notifyList, notify);
    cloud_mutex_unlock(&ctx->lockNotify);
    //[XY]Add for onenet loop
    if(cis_poll_sem != NULL)
        osSemaphoreRelease(cis_poll_sem);
    return CIS_RET_OK;
}

//AT+QLACONFIG=<bootstrap_flag>,<severIP>,<port>,<endpoint_name>,<lifetime>,<security_mode>,[<PSK_ID>,<PSK>][,binding_mode]
int at_proc_qlaconfig_req(char *at_buf, char **rsp_cmd)
{
    lwm2m_common_module_init();
    if (g_req_type == AT_CMD_REQ)
    {
    	int port = 0;
        int bs_flag = 0;
        uint32_t lifetime = 0;
        int security_mode = 0;
        int binding_mode = 0;
        INIT_NULL_PTR(server_host);
        INIT_NULL_PTR(endpoint_name);
        INIT_NULL_PTR(psk_id);
        INIT_NULL_PTR(psk);
        onenet_context_reference_t *onenet_context_ref = &onenet_context_refs[0];
        onenet_context_config_t *onenet_context_config = &onenet_context_configs[0];
        unsigned int err_num = ATERR_XY_ERR;

        if (!xy_tcpip_is_ok())
        {
            return ATERR_NOT_NET_CONNECT;
        }

        if(g_lwm2m_common_config_data->lwm2m_recovery_mode == 0)
            report_recover_result(onenet_resume_session());

        if (is_xy_lwm2m_running())
        {
            return ATERR_NOT_ALLOWED;
        }

        if (at_parse_param("%d(0-1),%p(),%d(1-65535),%p(),%d(20-31536000),%d[0-3],%p,%p,%d[0-1]", at_buf, &bs_flag,
                           &server_host, &port, &endpoint_name, &lifetime, &security_mode, &psk_id, &psk, &binding_mode) != AT_OK)
        {
            return ATERR_PARAM_INVALID;
        }
        if (strlen(server_host) > 100 || strlen(endpoint_name) > 60)
        {
            return ATERR_PARAM_INVALID;
        }
        g_lwm2m_common_config_data->bootstrap_flag = bs_flag;
        g_lwm2m_common_config_data->port = port;
        g_lwm2m_common_config_data->lifetime = lifetime;
        g_lwm2m_common_config_data->security_mode = security_mode;
        g_lwm2m_common_config_data->binding_mode = binding_mode;
        memset(g_lwm2m_common_config_data->server_host, 0, sizeof(g_lwm2m_common_config_data->server_host));
        memset(g_lwm2m_common_config_data->endpoint_name, 0, sizeof(g_lwm2m_common_config_data->endpoint_name));
        strcpy(g_lwm2m_common_config_data->server_host, server_host);
        strcpy(g_lwm2m_common_config_data->endpoint_name, endpoint_name);
        if (g_lwm2m_common_config_data->security_mode == 0)
        {
            if(psk_id == NULL || psk == NULL || strlen(psk_id) > 20 || strlen(psk) > 20)
            {
                return ATERR_PARAM_INVALID;
            }
            memset(g_lwm2m_common_config_data->psk_id, 0, sizeof(g_lwm2m_common_config_data->psk_id));
            memset(g_lwm2m_common_config_data->psk, 0, sizeof(g_lwm2m_common_config_data->psk));
            strcpy(g_lwm2m_common_config_data->psk_id, psk_id);
            strcpy(g_lwm2m_common_config_data->psk, psk);
        }

        generate_config_hex();      //生成config_hex

        osThreadAttr_t thread_attr = {0};
        if (xy_lwm2m_init(onenet_context_ref, onenet_context_config) < 0)
        {
            err_num = ATERR_NOT_ALLOWED;
            goto error;
        }
        else
        {
            thread_attr.name = "xy_lwm2m_tk";
            thread_attr.priority = osPriorityNormal1;
            thread_attr.stack_size = osStackShared;
            onenet_context_ref->onet_at_thread_id = osThreadNew((osThreadFunc_t)(onet_at_pump), onenet_context_ref, &thread_attr);		
        }

        //store g_lwm2m_common_config_data to lfs
        cloud_save_file(LWM2M_COMMON_CONFIG_NVM_FILE_NAME,(void*)g_lwm2m_common_config_data,sizeof(lwm2m_common_user_config_nvm_t));

        return AT_END;
    error:
        if (onenet_context_config != NULL && onenet_context_config->config_hex != NULL)
        {
            xy_free(onenet_context_config->config_hex);
            memset(onenet_context_config, 0, sizeof(onenet_context_config_t));
        }
        if (onenet_context_ref != NULL)
        {
            if (onenet_context_ref->onenet_context != NULL)
                cis_deinit((void **)&onenet_context_ref->onenet_context);

            if (onenet_context_ref->onet_at_thread_id != NULL)
            {
                osThreadTerminate(onenet_context_ref->onet_at_thread_id);
                onenet_context_ref->onet_at_thread_id = NULL;
            }
            free_onenet_context_ref(onenet_context_ref);
        }
        return err_num;
    }
    else if (g_req_type == AT_CMD_QUERY)
    {
        if(g_lwm2m_common_config_data->lwm2m_recovery_mode == 0)
            report_recover_result(onenet_resume_session());

        if(g_lwm2m_common_config_data == NULL || (g_lwm2m_common_config_data->bootstrap_flag == 0 && g_lwm2m_common_config_data->server_host == NULL) ||
                (g_lwm2m_common_config_data->bootstrap_flag == 1 && (onenet_context_refs[0].onenet_context->server == NULL || onenet_context_refs[0].onenet_context->server->sessionH == NULL)))
        {
            return AT_END;
        }
        else
        {
            *rsp_cmd = xy_malloc(150);
            memset(*rsp_cmd, 0, 150);
            sprintf(*rsp_cmd, "%d,%s,%d,%s,%d,%d", g_lwm2m_common_config_data->bootstrap_flag, g_lwm2m_common_config_data->bootstrap_flag == 1 ? ((cisnet_t)onenet_context_refs[0].onenet_context->server->sessionH)->host : g_lwm2m_common_config_data->server_host,
                    g_lwm2m_common_config_data->bootstrap_flag == 1 ? ((cisnet_t)onenet_context_refs[0].onenet_context->server->sessionH)->port : g_lwm2m_common_config_data->port, g_lwm2m_common_config_data->endpoint_name, g_lwm2m_common_config_data->lifetime, g_lwm2m_common_config_data->security_mode);
            if (g_lwm2m_common_config_data->security_mode == 0)
            {
                sprintf(*rsp_cmd + strlen(*rsp_cmd), ",%s,%s", g_lwm2m_common_config_data->psk_id, g_lwm2m_common_config_data->psk);
            }
            else
            {
                strcat(*rsp_cmd, ",,");
            }
            sprintf(*rsp_cmd + strlen(*rsp_cmd), ",%d", g_lwm2m_common_config_data->binding_mode);
        }
    }
    else
    {
        return ATERR_PARAM_INVALID;
    }
    return AT_END;
}


//AT+QLACFG="retransmit"[,<ACK_timeout>,<retrans_max_times>]
int at_proc_qlacfg_req(char *at_buf, char **rsp_cmd)
{
    lwm2m_common_module_init();
    if (g_req_type == AT_CMD_REQ)
    {
        if(g_lwm2m_common_config_data->lwm2m_recovery_mode == 0)
            report_recover_result(onenet_resume_session());

        char operation[16] = {0};
        if (at_parse_param("%16s", at_buf, operation) != AT_OK)
        {
            return ATERR_PARAM_INVALID;
        }
        if (at_strcasecmp(operation, "retransmit"))
        {
            int ack_timeout = -1;
            int retrans_max_times = -1;
            if (at_parse_param(",%d[2-20],%d[0-8]", at_buf, &ack_timeout, &retrans_max_times) != AT_OK)
            {
                return ATERR_PARAM_INVALID;
            }
            if (ack_timeout == -1 && retrans_max_times == -1)
            {
                *rsp_cmd = xy_malloc(64);
                snprintf(*rsp_cmd, 64, "\"retransmit\",%d,%d", g_lwm2m_common_config_data->ack_timeout, g_lwm2m_common_config_data->retrans_max_times);
                return AT_END;
            }
            else
            {
                if (ack_timeout == -1 || retrans_max_times == -1)
                {
                    return ATERR_PARAM_INVALID;
                }
                g_lwm2m_common_config_data->ack_timeout = ack_timeout;
                g_lwm2m_common_config_data->retrans_max_times = retrans_max_times;
            }
        }
        else if (at_strcasecmp(operation, "auto_ack"))
        {
            int is_auto_ack = -1;
            if (at_parse_param(",%d[0-1]", at_buf, &is_auto_ack) != AT_OK)
            {
                return ATERR_PARAM_INVALID;
            }
            if (is_auto_ack == -1)
            {
                *rsp_cmd = xy_malloc(64);
                snprintf(*rsp_cmd, 64, "\"auto_ack\",%d", g_lwm2m_common_config_data->is_auto_ack);
                return AT_END;
            }
            else
            {
                g_lwm2m_common_config_data->is_auto_ack = is_auto_ack;
            }
        }
        else if (at_strcasecmp(operation, "access_mode"))
        {
            int access_mode = -1;
            if (at_parse_param(",%d[0-1]", at_buf, &access_mode) != AT_OK)
            {
                return ATERR_PARAM_INVALID;
            }
            if (access_mode == -1)
            {
                *rsp_cmd = xy_malloc(64);
                snprintf(*rsp_cmd, 64, "\"access_mode\",%d", g_lwm2m_common_config_data->access_mode);
                return AT_END;
            }
            else
            {
                //g_lwm2m_common_config_data->access_mode = access_mode;
                g_lwm2m_common_config_data->access_mode_alternative = access_mode;
            }
        }
        else if (at_strcasecmp(operation, "platform"))
        {
            int platform = -1;
            if (at_parse_param(",%d[0-2]", at_buf, &platform) != AT_OK)
            {
                return ATERR_PARAM_INVALID;
            }
            if (platform == -1)
            {
                *rsp_cmd = xy_malloc(64);
                snprintf(*rsp_cmd, 64, "\"platform\",%d", g_lwm2m_common_config_data->platform);
                return AT_END;
            }
            else
            {
                g_lwm2m_common_config_data->platform = platform;
            }
        }
        else if (at_strcasecmp(operation, "cfg_res"))
        {
            int device_value = -1;
            if (at_parse_param(",,,,%d", at_buf, &device_value) != AT_OK)
            {
                return ATERR_PARAM_INVALID;
            }
            if (device_value == -1)
            {
                *rsp_cmd = xy_malloc(64);
                snprintf(*rsp_cmd, 64, "\"cfg_res\",3,0,17,%d", g_lwm2m_common_config_data->device_type);
                return AT_END;
            }
            else
            {
                g_lwm2m_common_config_data->device_type = device_value;
            }
        }
        else if (at_strcasecmp(operation, "recovery_mode"))
        {
            int recovery_mode = -1;
            if (at_parse_param(",%d[0-1]", at_buf, &recovery_mode) != AT_OK)
            {
                return ATERR_PARAM_INVALID;
            }
            if (recovery_mode == -1)
            {
                *rsp_cmd = xy_malloc(64);
                snprintf(*rsp_cmd, 64, "\"recovery_mode\",%d", g_lwm2m_common_config_data->lwm2m_recovery_mode);
                return AT_END;
            }
            else
            {
                g_lwm2m_common_config_data->lwm2m_recovery_mode = recovery_mode;
            }
        }
        else if (at_strcasecmp(operation, "lifetime_enable"))
        {
            int lifetime_enable = -1;
            if (at_parse_param(",%d[0-1]", at_buf, &lifetime_enable) != AT_OK)
            {
                return ATERR_PARAM_INVALID;
            }
            if (lifetime_enable == -1)
            {
                *rsp_cmd = xy_malloc(64);
                snprintf(*rsp_cmd, 64, "\"lifetime_enable\",%d", g_lwm2m_common_config_data->lifetime_enable);
                return AT_END;
            }
            else
            {
                g_lwm2m_common_config_data->lifetime_enable = lifetime_enable;
            }
        }
        else
        {
            return ATERR_PARAM_INVALID;
        }

        //store g_lwm2m_common_config_data to lfs
        cloud_save_file(LWM2M_COMMON_CONFIG_NVM_FILE_NAME,(void*)g_lwm2m_common_config_data,sizeof(lwm2m_common_user_config_nvm_t));
    }
    else if (g_req_type == AT_CMD_QUERY)
    {
        if(g_lwm2m_common_config_data->lwm2m_recovery_mode == 0)
            report_recover_result(onenet_resume_session());

        *rsp_cmd = xy_malloc(256);
        snprintf(*rsp_cmd, 256, "\r\n+QLACFG: \"retransmit\",%d,%d\r\n"
                                "+QLACFG: \"auto_ack\",%d\r\n"
                                "+QLACFG: \"access_mode\",%d\r\n"
                                "+QLACFG: \"platform\",%d\r\n"
                                "+QLACFG: \"cfg_res\",3,0,17,%d\r\n"
                                "+QLACFG: \"recovery_mode\",%d\r\n"
                                "+QLACFG: \"lifetime_enable\",%d\r\n"
                                "\r\nOK\r\n",
                 g_lwm2m_common_config_data->ack_timeout, g_lwm2m_common_config_data->retrans_max_times, g_lwm2m_common_config_data->is_auto_ack,
                 /*g_lwm2m_common_config_data->access_mode*/g_lwm2m_common_config_data->access_mode_alternative, g_lwm2m_common_config_data->platform, 
                 g_lwm2m_common_config_data->device_type, g_lwm2m_common_config_data->lwm2m_recovery_mode,
                 g_lwm2m_common_config_data->lifetime_enable);
    }
    else
    {
        return ATERR_PARAM_INVALID;
    }
    return AT_END;
}


//AT+QLAREG
int at_proc_qlareg_req(char *at_buf, char **rsp_cmd)
{
    lwm2m_common_module_init();
    if (g_req_type == AT_CMD_ACTIVE)
    {
        if (!xy_tcpip_is_ok())
        {
            return ATERR_NOT_NET_CONNECT;
        }
        if(g_lwm2m_common_config_data->lwm2m_recovery_mode == 0)
            report_recover_result(onenet_resume_session());

        if (!is_xy_lwm2m_running())
        {
            return ATERR_NOT_ALLOWED;
        }

        onenet_context_reference_t *onenet_context_ref = &onenet_context_refs[0];
        int ret = CIS_RET_ERROR;

        if (onenet_context_refs[0].onenet_context->registerEnabled == true)
        {
            return ATERR_NOT_ALLOWED;
        }

        onenet_context_refs[0].onenet_context->platform_common_type = g_lwm2m_common_config_data->platform;

        osMutexAcquire(g_onenet_mutex, osWaitForever);
        ret = onet_register(onenet_context_ref->onenet_context, g_lwm2m_common_config_data->lifetime);
        osMutexRelease(g_onenet_mutex);
        if (ret != CIS_RET_OK)
        {
            return ATERR_NOT_ALLOWED;
        }

        return AT_END;
    }
    else
    {
        return ATERR_PARAM_INVALID;
    }
}

//AT+QLAUPDATE=<mode>,<lifetime/binding_mode>
int at_proc_qlaupdate_req(char *at_buf, char **rsp_cmd)
{
    lwm2m_common_module_init();
    if (g_req_type == AT_CMD_REQ)
    {
        unsigned int err_num = ATERR_XY_ERR;
        if (!xy_tcpip_is_ok())
        {
            return ATERR_NOT_NET_CONNECT;
        }

        if(g_lwm2m_common_config_data->lwm2m_recovery_mode == 0)
            report_recover_result(onenet_resume_session());

        if (!is_xy_lwm2m_running())
        {
            return ATERR_NOT_ALLOWED;
        }
        int ret;
        int mode;
        int value;
        unsigned short mid;

        if (at_parse_param("%d(0-1),%d", at_buf, &mode, &value) != AT_OK)
        {
            return ATERR_PARAM_INVALID;
        }

        if (mode == 0)
        {
            if (value < 20 || value > 31536000)
            {
                return ATERR_PARAM_INVALID;
            }
            g_lwm2m_common_config_data->lifetime = value;
        }
        else
        {
            if (value != 0 && value != 1)
            {
                return ATERR_PARAM_INVALID;
            }
            g_lwm2m_common_config_data->binding_mode = value;
        }

        osMutexAcquire(g_onenet_mutex, osWaitForever);
        ret = cis_update_reg_common(onenet_context_refs[0].onenet_context, g_lwm2m_common_config_data->lifetime,
                                    g_lwm2m_common_config_data->binding_mode, 0, &mid, mode == 0 ? 1 : 0, RAI_NULL);
        osMutexRelease(g_onenet_mutex);
        //[XY]Add for onenet loop
        if (cis_poll_sem != NULL)
            osSemaphoreRelease(cis_poll_sem);
        if (ret != CIS_RET_OK)
        {
            return ATERR_NOT_ALLOWED;
        }

        *rsp_cmd = xy_malloc(40);
        sprintf(*rsp_cmd, "%d", mid);
        return AT_END;
    }
    else
    {
        return ATERR_PARAM_INVALID;
    }
}

//AT+QLADEREG
int at_proc_qladereg_req(char *at_buf, char **rsp_cmd)
{
    lwm2m_common_module_init();
    if (g_req_type == AT_CMD_ACTIVE)
    {
        if (!xy_tcpip_is_ok())
        {
            return ATERR_NOT_NET_CONNECT;
        }

        if(g_lwm2m_common_config_data->lwm2m_recovery_mode == 0)
            report_recover_result(onenet_resume_session());

        if (!is_xy_lwm2m_running())
        {
            return ATERR_NOT_ALLOWED;
        }
        int ret;
        st_context_t *onenet_context = onenet_context_refs[0].onenet_context;
        osMutexAcquire(g_onenet_mutex, osWaitForever);
        if (onenet_context->registerEnabled == true)
            ret = cis_unregister(onenet_context);
        osMutexRelease(g_onenet_mutex);
        if (ret != CIS_RET_OK)
        {
            return ATERR_NOT_ALLOWED;
        }

        return AT_END;
    }
    else
    {
        return ATERR_PARAM_INVALID;
    }
}

// AT+QLAADDOBJ=<objectID>,<instanceID>,<resource_number>,<resourceID>
int at_proc_qlaaddobj_req(char *at_buf, char **rsp_cmd)
{
    lwm2m_common_module_init();
    if (g_req_type == AT_CMD_REQ)
    {
        int *res_ids = NULL;
        char *res_ids_fmt = NULL;
        unsigned int err_num = AT_END;

        if (!xy_tcpip_is_ok())
        {
            return ATERR_NOT_NET_CONNECT;
        }

        if(g_lwm2m_common_config_data->lwm2m_recovery_mode == 0)
            report_recover_result(onenet_resume_session());

        if (!is_xy_lwm2m_running())
        {
            return ATERR_NOT_ALLOWED;
        }

        int ret;
        int obj_id;
        int inst_id;
        int res_num;
        int i;
        if (at_parse_param("%d(0-65535),%d(0-8),%d(0-)", at_buf, &obj_id, &inst_id, &res_num) != AT_OK)
        {
            return ATERR_PARAM_INVALID;
        }

        res_ids_fmt = xy_malloc(100);
        memset(res_ids_fmt, 0, 100);
        strcpy(res_ids_fmt, ",,,%d,%d");
        res_ids = xy_malloc(res_num * sizeof(int));
        memset(res_ids, 0, res_num * sizeof(int));
        for (i = 0; i < res_num; i++)
        {
            strcat(res_ids_fmt, ",%d");
            if (at_parse_param(res_ids_fmt, at_buf, &res_ids[i]) != AT_OK)
            {
                err_num = ATERR_PARAM_INVALID;
                goto out;
            }
            res_ids_fmt[strlen(res_ids_fmt) - 1] = '\0';    //去掉末尾的'd'
            res_ids_fmt[strlen(res_ids_fmt) - 1] = '\0';    //去掉末尾的'%'
        }

        cis_inst_bitmap_t bitmap = {0};
        uint8_t instPtr[1] = {0};
        uint16_t instBytes;
        cis_res_count_t rescount;

        bitmap.instanceBitmap = instPtr;
		bitmap.instanceCount = 8;
		bitmap.instanceBytes = 1;
		rescount.attrCount = res_num;
		rescount.actCount = res_num;

		instPtr[0] = 0x80 >> inst_id;
		st_object_t *objectP = prv_findObject(onenet_context_refs[0].onenet_context, obj_id);

		if(objectP != NULL)
		{
			if(instPtr[0] & *objectP->instBitmapPtr)    //obj_id和ins_id 已存在直接报错
			{
				err_num = ATERR_PARAM_INVALID;
				goto out;
			}
			else
			{
				//obj_id存在，ins_id不存在，需先删除obj，再重新添加
				instPtr[0] |= *objectP->instBitmapPtr;
				if(res_num < objectP->attributeCount)
				{
					rescount.attrCount = objectP->attributeCount;
					rescount.actCount = objectP->actionCount;
				}
				osMutexAcquire(g_onenet_mutex, osWaitForever);
				cis_delobject(onenet_context_refs[0].onenet_context, obj_id);
				osMutexRelease(g_onenet_mutex);
			}
		}

        osMutexAcquire(g_onenet_mutex, osWaitForever);
        ret = cis_addobject(onenet_context_refs[0].onenet_context, obj_id, &bitmap, &rescount);
        osMutexRelease(g_onenet_mutex);

        if (ret != CIS_RET_OK)
        {
            err_num = ATERR_NOT_ALLOWED;
            goto out;
        }
        else
        {
            xy_lwm2m_object_info_t *xy_lwm2m_object_info = xy_malloc(sizeof(xy_lwm2m_object_info_t));
            xy_lwm2m_object_info->obj_id = obj_id;
            xy_lwm2m_object_info->instance_id = inst_id;
            xy_lwm2m_object_info->resource_count = res_num;
            for (i = 0; i < res_num; i++)
            {
                xy_lwm2m_object_info->resouce_ids[i] = res_ids[i];
            }
            if (insert_object_info_node(xy_lwm2m_object_info) < 0)
            {
                xy_free(xy_lwm2m_object_info);
            }
            err_num = AT_END;
        }
out:
        if (res_ids_fmt != NULL)
            xy_free(res_ids_fmt);
        if (res_ids != NULL)
            xy_free(res_ids);
        return err_num;
    }
    else if (g_req_type == AT_CMD_QUERY)
    {
        if (!is_xy_lwm2m_running())
        {
            *rsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
            return AT_END;
        }
        if (xy_lwm2m_object_info_head == NULL || xy_lwm2m_object_info_head->num == 0)
        {
            *rsp_cmd = xy_malloc(32);
            strcpy(*rsp_cmd, "");
        }
        else
        {
            st_context_t *context = onenet_context_refs[0].onenet_context;
            int i, len, temp_len;
            len = 0;
            *rsp_cmd = xy_malloc(90 * xy_lwm2m_object_info_head->num);
            memset(*rsp_cmd, 0, 90 * xy_lwm2m_object_info_head->num);
            xy_lwm2m_object_info_t *node = xy_lwm2m_object_info_head->first;
            while (node)
            {
                temp_len = sprintf(*rsp_cmd + len, "\r\n+QLAADDOBJ: %d,%d,%d", node->obj_id, node->instance_id, node->resource_count);
                len += temp_len;
                for (i = 0; i < node->resource_count; i++)
                {
                    temp_len = sprintf(*rsp_cmd + len, ",%d", node->resouce_ids[i]);
                    len += temp_len;
                }
                node = node->next;
            }
            strcat(*rsp_cmd, "\r\n\r\nOK\r\n");
        }
    }
    else
    {
        return ATERR_PARAM_INVALID;
    }
    return AT_END;
}


//AT+QLADELOBJ=<objectID>
int at_proc_qladelobj_req(char *at_buf, char **rsp_cmd)
{
    lwm2m_common_module_init();
    if (g_req_type == AT_CMD_REQ)
    {
        if (!xy_tcpip_is_ok())
            return ATERR_NOT_NET_CONNECT;

        if(g_lwm2m_common_config_data->lwm2m_recovery_mode == 0)
            report_recover_result(onenet_resume_session());

        if (!is_xy_lwm2m_running())
        {
            return ATERR_NOT_ALLOWED;
        }

        int ret;
        int obj_id;

        if (at_parse_param("%d(0-)", at_buf, &obj_id) != AT_OK)
        {
            return ATERR_PARAM_INVALID;
        }

        osMutexAcquire(g_onenet_mutex, osWaitForever);
        ret = cis_delobject(onenet_context_refs[0].onenet_context, obj_id);
        osMutexRelease(g_onenet_mutex);
        if (ret != CIS_RET_OK)
            return ATERR_NOT_ALLOWED;

        remove_object_info_node(obj_id);
    }
    else
    {
        return ATERR_PARAM_INVALID;
    }

    return AT_END;
}

//AT+QLARDRSP=<messageID>,<result>,<objectID>,<instantID>,<resourceID>,<value_type>,<len>,<value>,<index>
int at_proc_qlardrsp_req(char *at_buf, char **rsp_cmd)
{
    lwm2m_common_module_init();
    if (g_req_type == AT_CMD_REQ)
    {
        unsigned int err_num = ATERR_XY_ERR;
        struct onenet_read param = {0};
        cis_coapret_t result;
        char *at_value = NULL;
        int ret = -1;
        if (!xy_tcpip_is_ok())
        {
            return ATERR_NOT_NET_CONNECT;
        }

        if(g_lwm2m_common_config_data->lwm2m_recovery_mode == 0)
            report_recover_result(onenet_resume_session());

        if (!is_xy_lwm2m_running())
        {
            return ATERR_NOT_ALLOWED;
        }

        if (at_parse_param("%d(0-),%d(1-15),%d(0-),%d(0-),%d(0-),%d(1-5),%d(1-1024),%p(),%d(0-)",
                at_buf, &param.msgId, &param.result, &param.objId, &param.insId, &param.resId, &param.value_type, &param.len, &at_value, &param.index) != AT_OK)
        {
            return ATERR_PARAM_INVALID;
        }

        if (0 != check_coap_result_code(param.result, RSP_READ))
        {
            return ATERR_PARAM_INVALID;
        }

        if (onet_check_reqType(CALLBACK_TYPE_READ, param.msgId) != 0)
        {
            return ATERR_PARAM_INVALID;
        }

        if ((result = get_coap_result_code(param.result)) != CIS_COAP_205_CONTENT)
        {
            osMutexAcquire(g_onenet_mutex, osWaitForever);
            cis_response(onenet_context_refs[param.ref].onenet_context, NULL, NULL, param.msgId, result, param.raiflag);
            osMutexRelease(g_onenet_mutex);
            return AT_END;
        }

        param.value = xy_malloc2(strlen(at_buf));
        if(param.value == NULL)
            return ATERR_NO_MEM;
        memset(param.value, 0, strlen(at_buf));

        //value
        if (!onet_at_get_notify_value(param.value_type, param.len, at_value, param.value))
        {
            err_num = ATERR_PARAM_INVALID;
            goto out;
        }

        if (param.index == 0)
            param.flag = 0;
        else
            param.flag = 2;

        osMutexAcquire(g_onenet_mutex, osWaitForever);
        ret = onet_miplread_req(onenet_context_refs[0].onenet_context, &param);
        osMutexRelease(g_onenet_mutex);
        if (ret != CIS_RET_OK)
        {
            err_num = ATERR_NOT_ALLOWED;
            goto out;
        }
        err_num = AT_END;
    out:
        if (param.value != NULL)
            xy_free(param.value);

        return err_num;
    }
    else
    {
        return ATERR_PARAM_INVALID;
    }

    return AT_END;
}

//AT+QLAWRRSP=<messageID>,<result>
int at_proc_qlawrrsp_req(char *at_buf, char **rsp_cmd)
{
    lwm2m_common_module_init();
    if (g_req_type == AT_CMD_REQ)
    {
        if (!xy_tcpip_is_ok())
        {
            return  ATERR_NOT_NET_CONNECT;
        }

        if(g_lwm2m_common_config_data->lwm2m_recovery_mode == 0)
            report_recover_result(onenet_resume_session());

        if (!is_xy_lwm2m_running())
        {
            return ATERR_NOT_ALLOWED;
        }

        int msg_id, result, ret;

        if (at_parse_param("%d(0-),%d(1-15)", at_buf, &msg_id, &result) != AT_OK)
        {
            return ATERR_PARAM_INVALID;
        }

        if (0 != check_coap_result_code(result, RSP_WRITE))
        {
            return ATERR_PARAM_INVALID;
        }

        if (onet_check_reqType(CALLBACK_TYPE_WRITE, msg_id) != 0)
        {
            return ATERR_PARAM_INVALID;
        }

        osMutexAcquire(g_onenet_mutex, osWaitForever);
        ret = cis_response(onenet_context_refs[0].onenet_context, NULL, NULL, msg_id, get_coap_result_code(result), RAI_NULL);
        osMutexRelease(g_onenet_mutex);
        if (ret != CIS_RET_OK)
            return ATERR_NOT_ALLOWED;

    }
    else
    {
        return ATERR_PARAM_INVALID;
    }

    return AT_END;
}

//AT+QLAEXERSP=<messageID>,<result>
int at_proc_qlaexersp_req(char *at_buf, char **rsp_cmd)
{
    lwm2m_common_module_init();
    if (g_req_type == AT_CMD_REQ)
    {
        if (!xy_tcpip_is_ok())
        {
            return ATERR_NOT_NET_CONNECT;
        }

        if(g_lwm2m_common_config_data->lwm2m_recovery_mode == 0)
            report_recover_result(onenet_resume_session());

        if (!is_xy_lwm2m_running())
        {
            return ATERR_NOT_ALLOWED;
        }

        int msg_id, result, ret;

        if (at_parse_param("%d,%d", at_buf, &msg_id, &result) != AT_OK)
        {
            return ATERR_PARAM_INVALID;
        }

        if (0 != check_coap_result_code(result, RSP_EXECUTE))
        {
            return ATERR_PARAM_INVALID;
        }

        if (onet_check_reqType(CALLBACK_TYPE_EXECUTE, msg_id) != 0)
        {
            return ATERR_PARAM_INVALID;
        }

        osMutexAcquire(g_onenet_mutex, osWaitForever);
        ret = cis_response(onenet_context_refs[0].onenet_context, NULL, NULL, msg_id, get_coap_result_code(result), RAI_NULL);
        osMutexRelease(g_onenet_mutex);
        if (ret != CIS_RET_OK)
        {
            return ATERR_NOT_ALLOWED;
        }
    }
    else
    {
        return ATERR_PARAM_INVALID;
    }
    return AT_END;
}

//AT+QLAOBSRSP=<messageID>,<result>,<objectID>,<instantID>,<resourceID>,<value_type>,<len>,<value>,<index>
int at_proc_qlaobsrsp_req(char *at_buf, char **rsp_cmd)
{
    lwm2m_common_module_init();
    if (g_req_type == AT_CMD_REQ)
    {
    	unsigned int err_num = ATERR_XY_ERR;
        struct onenet_read param = {0};
		char *at_value = NULL;
        int ret = CIS_RET_OK;
        if (!xy_tcpip_is_ok())
        {
            return ATERR_NOT_NET_CONNECT;
        }

        if(g_lwm2m_common_config_data->lwm2m_recovery_mode == 0)
            report_recover_result(onenet_resume_session());

        if (!is_xy_lwm2m_running())
        {
            return ATERR_NOT_ALLOWED;
        }

        cis_mid_t temp_observeMid;

        if (at_parse_param("%d(0-),%d(1-15),%d(0-),%d(0-),%d(0-),%d(1-5),%d(0-1024),%p(),%d(0-)", at_buf, &param.msgId, &param.result,
                           &param.objId, &param.insId, &param.resId, &param.value_type, &param.len, &at_value, &param.index) != AT_OK)
        {
            return ATERR_PARAM_INVALID;
        }

        if (0 != check_coap_result_code(param.result, RSP_OBSERVE))
        {
            return ATERR_PARAM_INVALID;
        }
        if (onet_check_reqType(CALLBACK_TYPE_OBSERVE, param.msgId) != 0 && onet_check_reqType(CALLBACK_TYPE_OBSERVE_CANCEL, param.msgId) != 0)
        {
            return ATERR_PARAM_INVALID;
        }

        param.value = xy_malloc(strlen(at_buf));
        memset(param.value, 0, strlen(at_buf));

        //value
        if (!onet_at_get_notify_value(param.value_type, param.len, at_value, param.value))
        {
            err_num = ATERR_PARAM_INVALID;
            goto out;
        }

        if (param.index == 0)
            param.flag = 0;
        else
            param.flag = 2;

        osMutexAcquire(g_onenet_mutex, osWaitForever);
        if (observe_findByMsgid(onenet_context_refs[0].onenet_context, param.msgId) == NULL)
        {
            if (!packet_asynFindObserveRequest(onenet_context_refs[param.ref].onenet_context, param.msgId, &temp_observeMid))
            {
                osMutexRelease(g_onenet_mutex);
                err_num = ATERR_NOT_ALLOWED;
                goto out;
            }
        }

        ret = cis_response(onenet_context_refs[0].onenet_context, NULL, NULL, param.msgId, get_coap_result_code(param.result), param.raiflag);
        osMutexRelease(g_onenet_mutex);
        if (ret != CIS_RET_OK)
        {
            err_num = ATERR_NOT_ALLOWED;
            goto out;
        }

        err_num = AT_END;
out:
        if (param.value != NULL)
            xy_free(param.value);
        return err_num;
    }
    else
    {
        return ATERR_PARAM_INVALID;
    }
    return AT_END;
}

//AT+QLANOTIFY=<objectID>,<instantID>,<resourceID>,<value_type>,<len>,<value>,<index>[,<ACK>]
int at_proc_qlanotify_req(char *at_buf, char **rsp_cmd)
{
    lwm2m_common_module_init();
    if (g_req_type == AT_CMD_REQ)
    {
        int err_num = AT_END;
        struct onenet_notify param = {0};
        int ret;
        st_observed_t *observe = NULL;
        char *at_value = NULL;

        if (!xy_tcpip_is_ok())
            return ATERR_NOT_NET_CONNECT;

        if(g_lwm2m_common_config_data->lwm2m_recovery_mode == 0)
            report_recover_result(onenet_resume_session());

        if (!is_xy_lwm2m_running())
        {
            return ATERR_NOT_ALLOWED;
        }

        if (at_parse_param("%d(0-),%d(0-),%d(0-),%d(1-5),%d(0-1024),%p(),%d(0-),%d[0-1]", at_buf, &param.objId, &param.insId,
                           &param.resId, &param.value_type, &param.len, &at_value, &param.index, &param.ackid) != AT_OK)
        {
            return ATERR_PARAM_INVALID;
        }

        param.value = xy_malloc2(strlen(at_buf));
        if(param.value == NULL)
            return ATERR_NO_MEM;
        memset(param.value, 0, strlen(at_buf));
        //value
        if (!onet_at_get_notify_value(param.value_type, param.len, at_value, param.value))
        {
            err_num = ATERR_PARAM_INVALID;
            goto out;
        }

        if (param.index == 0)
            param.flag = 0;
        else
            param.flag = 2;

        st_uri_t uriP = {0};
        uri_make(param.objId, param.insId, param.resId, &uriP);
        observe = observe_findByUri(onenet_context_refs[0].onenet_context, &uriP);
        if (observe == NULL)
        {
            uri_make(param.objId, param.insId, URI_INVALID, &uriP);
            observe = observe_findByUri(onenet_context_refs[0].onenet_context, &uriP);
            if (observe == NULL)
            {
                err_num = ATERR_PARAM_INVALID;
                goto out;
            }
        }
        param.msgId = observe->msgid;
        osMutexAcquire(g_onenet_mutex, osWaitForever);
        ret = lwm2m_notify_data(onenet_context_refs[0].onenet_context, &param);
        osMutexRelease(g_onenet_mutex);
        if (ret != CIS_RET_OK && ret != COAP_205_CONTENT)
        {
            err_num = ATERR_NOT_ALLOWED;
            goto out;
        }

    out:
        if (param.value != NULL)
            xy_free(param.value);
        return err_num;
    }
    else
    {
        return ATERR_PARAM_INVALID;
    }

    return AT_END;
}

//AT+QLASENDDATA=<value_type>,<len>,<value>,<ACK>
int at_proc_qlasend_req(char *at_buf, char **prsp_cmd)
{
	lwm2m_common_module_init();
    struct onenet_notify param = {0};
    cis_ret_t ret = CIS_RET_INVILID;
    uint32_t hex_data_len = 0;
    char *at_str = NULL;
    st_observed_t *observe = NULL;
    int err_num = AT_OK;

    param.objId = 19;
    param.insId = 1;
    param.resId = 0;

    if (!xy_tcpip_is_ok())
    {
        return ATERR_NOT_NET_CONNECT;
    }

    if(g_lwm2m_common_config_data->lwm2m_recovery_mode == 0)
        report_recover_result(onenet_resume_session());

    if (!is_xy_lwm2m_running())
    {
        return ATERR_NOT_ALLOWED;
    }

    if (g_req_type != AT_CMD_REQ || at_parse_param("%d(1-2),%d(1-1024),%p(),%d[0-1]", at_buf, &param.value_type, &param.len, at_str, &param.ackid) != AT_OK)
    {
       return ATERR_PARAM_INVALID;
    }

    param.value = xy_malloc2(strlen(at_buf));
    if(param.value == NULL)
        return ATERR_NO_MEM;
    memset(param.value, 0, strlen(at_buf));
    if(!onet_at_get_notify_value(param.value_type, param.len, at_str, param.value))
    {
        err_num = ATERR_PARAM_INVALID;
        goto out;
    }

    st_uri_t uriP = {0};
    uri_make(param.objId, param.insId, param.resId, &uriP);
    observe = observe_findByUri(onenet_context_refs[0].onenet_context, &uriP);
    if (observe == NULL)
    {
        uri_make(param.objId, param.insId, URI_INVALID, &uriP);
        observe = observe_findByUri(onenet_context_refs[0].onenet_context, &uriP);
        if (observe == NULL)
        {
            return  (ATERR_NOT_ALLOWED);
            goto out;
        }
    }
    param.msgId = observe->msgid;
    osMutexAcquire(g_onenet_mutex, osWaitForever);
    ret = lwm2m_send_data(onenet_context_refs[0].onenet_context, &param);
    osMutexRelease(g_onenet_mutex);
    if (ret != CIS_RET_OK && ret != COAP_205_CONTENT)
    {
        err_num = ATERR_NOT_ALLOWED;
    }

out:
    if (param.value != NULL)
        xy_free(param.value);

    return err_num;
}

//AT+QLARD?
int at_proc_qlard_req(char *at_buf, char **rsp_cmd)
{
    lwm2m_common_module_init();
    if (g_req_type == AT_CMD_ACTIVE)
    {
        if (!xy_tcpip_is_ok())
        {
            return ATERR_NOT_NET_CONNECT;
        }

        if(g_lwm2m_common_config_data->lwm2m_recovery_mode == 0)
            report_recover_result(onenet_resume_session());

        if (!is_xy_lwm2m_running())
        {
            return ATERR_NOT_ALLOWED;
        }

        if (xy_lwm2m_cached_urc_head->num == 0)
        {
            *rsp_cmd = xy_malloc(32);
            snprintf(*rsp_cmd, 32, "%d", xy_lwm2m_cached_urc_head->num);
        }
        else
        {
            xy_lwm2m_cached_urc_common_t *node = get_and_remove_cached_urc_first_node();
            if (node == NULL)
            {
                return ATERR_NOT_ALLOWED;
            }
            int temp_len = 32 + strlen(node->urc_data);
            *rsp_cmd = xy_malloc2(temp_len);
            if(*rsp_cmd == NULL)
                return ATERR_NO_MEM;
            snprintf(*rsp_cmd, temp_len, "%d,%s", xy_lwm2m_cached_urc_head->num, node->urc_data);
            FREEIFNOTNULL(node->urc_data);
            FREEIFNOTNULL(node);
        }

        return AT_END;
    }
    else if (g_req_type == AT_CMD_QUERY)
    {
        *rsp_cmd = xy_malloc(32);
        snprintf(*rsp_cmd, 32, "%d", xy_lwm2m_cached_urc_head->num);
    }
    else
    {
        return ATERR_PARAM_INVALID;
    }

    return AT_END;
}

//AT+QLASTATUS?
int at_proc_qlstatus_req(char *at_buf, char **rsp_cmd)
{
    lwm2m_common_module_init();
    if (g_req_type == AT_CMD_QUERY)
    {
        int status;

        if (!is_xy_lwm2m_running())
        {
            if(onenet_session_file_exist())
                status = 7;
            else
                status = 5;
        }
        else
        {
            st_context_t *onenet_context = onenet_context_refs[0].onenet_context;
            if (onenet_context->registerEnabled == true)
            {
                switch (registration_getStatus(onenet_context))
                {
                case STATE_REGISTERED:
                {
                    status = 2;
                    break;
                }

                case STATE_REG_FAILED:
                {
                    status = 0;
                    break;
                }
                case STATE_DEREG_PENDING:
                {
                    status = 3;
                    break;
                }
                case STATE_REG_PENDING:
                default:
                        status = 1;
                    break;
                }
            }
            else
            {
                status = 0;
            }
        }
        *rsp_cmd = xy_malloc(32);
        snprintf(*rsp_cmd, 32, "%d", status);

    }
    else
    {
        return ATERR_PARAM_INVALID;
    }

    return AT_END;
}

//AT+QLARECOVER
int at_proc_qlarecover_req(char *at_buf, char **rsp_cmd)
{
    lwm2m_common_module_init();
    if (g_req_type == AT_CMD_ACTIVE)
    {
        if (!xy_tcpip_is_ok())
            return ATERR_NOT_NET_CONNECT;

        if (is_xy_lwm2m_running())
           return ATERR_NOT_ALLOWED;

        if(g_lwm2m_common_config_data->lwm2m_recovery_mode == 1)
        {
            if(onenet_resume_session() != RESUME_SUCCEED)   //不需要恢复或恢复失败
            {
                *rsp_cmd = xy_malloc(30);
                snprintf(*rsp_cmd, 30, "3");
                return AT_END;
            }
        }
        else
        {
            return ATERR_NOT_ALLOWED;
        }

        *rsp_cmd = xy_malloc(30);
        snprintf(*rsp_cmd, 30, "0");
    }
    else
    {
        return ATERR_PARAM_INVALID;
    }

    return AT_END;
}

void lwm2m_common_module_init(void)
{
    osMutexAcquire(g_onenet_module_init_mutex, osWaitForever);
    if(g_cislwm2m_inited == 0)
    {
        g_cislwm2m_inited = 1;

        /*雁飞session恢复*/
        g_onenet_session_info = xy_malloc(sizeof(onenet_session_info_t));
        memset(g_onenet_session_info, 0, sizeof(onenet_session_info_t));

        if(Is_WakeUp_From_Dsleep())
            cloud_read_file(ONENET_SESSION_NVM_FILE_NAME,(void*)g_onenet_session_info,sizeof(onenet_session_info_t));
        else
            cloud_remove_file(ONENET_SESSION_NVM_FILE_NAME);

        cloud_mutex_create(&g_onenet_mutex);

        /*雁飞配置恢复*/
        g_lwm2m_common_config_data = xy_malloc(sizeof(lwm2m_common_user_config_nvm_t));
        memset(g_lwm2m_common_config_data, 0, sizeof(lwm2m_common_user_config_nvm_t));
        init_xy_lwm2m_lists();
        if( XY_ERR == cloud_read_file(LWM2M_COMMON_CONFIG_NVM_FILE_NAME,(void*)g_lwm2m_common_config_data,sizeof(lwm2m_common_user_config_nvm_t)))
            init_xy_lwm2m_config();

        osMutexRelease(g_onenet_module_init_mutex);
    }
    else
        osMutexRelease(g_onenet_module_init_mutex);
}
#endif


