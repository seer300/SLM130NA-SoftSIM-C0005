#include <stdarg.h>
#include "xy_log.h"
#include "xy_system.h"
#include "onomondo/softsim/log.h"

#define SS_ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

uint32_t ss_log_mask = 0xffffffff;

static uint32_t subsys_lvl[_NUM_LOG_SUBSYS] = {
    [SBTLV] = LDEBUG,   [SCTLV] = LDEBUG,      [SVPCD] = LDEBUG,
    [SIFACE] = LDEBUG,  [SUICC] = LDEBUG,      [SCMD] = LDEBUG,
    [SLCHAN] = LDEBUG,  [SFS] = LDEBUG,        [SSTORAGE] = LDEBUG,
    [SACCESS] = LDEBUG, [SADMIN] = LDEBUG,     [SSFI] = LDEBUG,
    [SDFNAME] = LDEBUG, [SFILE] = LDEBUG,      [SPIN] = LDEBUG,
    [SAUTH] = LDEBUG,   [SPROACT] = LDEBUG,    [STLV8] = LDEBUG,
    [SSMS] = LDEBUG,    [SREMOTECMD] = LDEBUG, [SREFRESH] = LDEBUG,
    [SAPDU] = LDEBUG,
};

static const char *subsys_str[_NUM_LOG_SUBSYS] = {
    [SBTLV] = "BTLV",     [SCTLV] = "CTLV",     [SVPCD] = "VPCD",
    [SIFACE] = "IFACE",   [SUICC] = "UICC",     [SCMD] = "CMD",
    [SLCHAN] = "LCHAN",   [SFS] = "FS",         [SSTORAGE] = "STORAGE",
    [SACCESS] = "ACCESS", [SADMIN] = "ADMIN",   [SSFI] = "SFI",
    [SDFNAME] = "DFNAME", [SFILE] = "FILE",     [SPIN] = "PIN",
    [SAUTH] = "AUTH",     [SPROACT] = "PROACT", [SREMOTECMD] = "REMOTECMD",
    [STLV8] = "TLV8",     [SSMS] = "SMS",       [SREFRESH] = "REFRESH",
    [SAPDU] = "APDU",
};

static const char *level_str[_NUM_LOG_LEVEL] = {
    [LERROR] = "ERROR",
    [LINFO] = "INFO",
    [LDEBUG] = "DEBUG",
};

void ss_logp(uint32_t subsys, uint32_t level, const char *file, int line, const char *format, ...)
{
  return; // optionally return early here to disable all logging from softsim
  va_list ap;

  if (!(ss_log_mask & (1 << subsys)))
    return;

  if (level > subsys_lvl[subsys])
    return;

  char buf[1024];
  int len = sprintf(buf, "%8s %8s ", subsys_str[subsys], level_str[level]);
  va_start(ap, format);
  vsprintf(buf + len, format, ap);
  va_end(ap);

  // xinyi specific log implementation
  xy_printf(0, PLATFORM, DEBUG_LOG, "%s", buf);
}
