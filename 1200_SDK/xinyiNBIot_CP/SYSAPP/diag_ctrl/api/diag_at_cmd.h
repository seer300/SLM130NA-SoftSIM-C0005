#ifndef DIAG_AT_CMD_H
#define DIAG_AT_CMD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "diag_options.h"
#include "diag_item_types.h"


// 输出AT命令的回复和主动上报
diag_print_state_t diag_at_response_output(char *at_str, int str_len);


#ifdef __cplusplus
}
#endif

#endif  /* DIAG_AT_CMD_H */
