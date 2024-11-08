#include "ctlw_log.h"
#ifdef PLATFORM_XINYI
#include "factory_nv.h"
#include "xy_utils.h"
extern softap_fac_nv_t *g_softap_fac_nv;
static ctiot_log_e g_ctiot_log_level = LOG_INFO;
#else
static ctiot_log_e g_ctiot_log_level = LOG_DEBUG;
#endif

static const char *g_log_names[] =
{
	"",
    "DEBUG",
    "INFO",
    "WARNING",
    "ERR",
    "FATAL",
};

void ctiot_set_log_level(ctiot_log_e level)
{
    g_ctiot_log_level = level;
}

ctiot_log_e ctiot_get_log_level(void)
{
    return g_ctiot_log_level;
}

const char *ctiot_get_log_level_name(ctiot_log_e log_level)
{
    if (log_level >= LOG_MAX)
    {
        return "UNKOWN";
    }

    return g_log_names[log_level];
}



#ifdef PLATFORM_XINYI

int xy_ctiot_log_info(uint8_t log_level, const char * module, const char *class, const char* function, int32_t line, const char *fmt,...)
{
    if(HWREGB(BAK_MEM_XY_DUMP) == 1)
    {
        g_ctiot_log_level = LOG_DEBUG;
    }
    else if(HWREGB(BAK_MEM_XY_DUMP) == 0)
    {
        g_ctiot_log_level = LOG_INFO;
    }
    if(log_level >= ctiot_get_log_level())
    {
        va_list arg;
        va_start(arg, fmt);
        char * log_info = xy_malloc2(100);

        if(log_info == NULL)
            return -1;
        
        vsnprintf(log_info, 100, fmt, arg);
        va_end(arg);
        xy_printf(0,XYAPP, WARN_LOG, "[%s][%s][%d] %s\r\n", ctiot_get_log_level_name((log_level)), function, line, log_info);
        xy_free(log_info);
    }

    return 0;
}
#endif