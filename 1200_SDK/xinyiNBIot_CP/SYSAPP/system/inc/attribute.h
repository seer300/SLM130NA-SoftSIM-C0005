#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H

#ifdef __cplusplus
extern "C" {
#endif


#define _ATTRIBURE_STR(x) _ATTRIBURE_VAL(x)
#define _ATTRIBURE_VAL(x) #x

/* force function on RAM */
#define __RAM_FUNC                  __attribute__((section(".ram.text" "." __FILE__ "." _ATTRIBURE_STR(__LINE__))))
/* force function on FLASH */
#define __FLASH_FUNC                __attribute__((section(".flash.text" "." __FILE__ "." _ATTRIBURE_STR(__LINE__))))

#ifndef __WEAK
#define __WEAK                       __attribute__((weak))
#endif


#ifdef __cplusplus
}
#endif

#endif  /* ATTRIBUTE_H */
