#ifndef ROM_FUCN_H
#define ROM_FUCN_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include "cmsis_device.h"


// #define ROM_CODE_BASE_OFFSET    0x11000
#define ROM_CODE_BASE_OFFSET    0x20111000


/* functions address */
#define xinyi_putchar_ADDR              (0x00000E05 + ROM_CODE_BASE_OFFSET)
#define xinyi_printf_ADDR               (0x00000E11 + ROM_CODE_BASE_OFFSET)
#define xinyi_sprintf_ADDR              (0x00000E3D + ROM_CODE_BASE_OFFSET)
#define xinyi_snprintf_ADDR             (0x00000E69 + ROM_CODE_BASE_OFFSET)
#define xinyi_vprintf_ADDR              (0x00000E91 + ROM_CODE_BASE_OFFSET)
#define xinyi_vsnprintf_ADDR            (0x00000EB1 + ROM_CODE_BASE_OFFSET)
#define fctprintf_ADDR                  (0x00000ECD + ROM_CODE_BASE_OFFSET)
#define xinyi_abs_ADDR                  (0x00000EFD + ROM_CODE_BASE_OFFSET)
#define __xinyi_itoa_ADDR               (0x00000F05 + ROM_CODE_BASE_OFFSET)
#define xinyi_itoa_ADDR                 (0x00000F35 + ROM_CODE_BASE_OFFSET)
#define __xinyi_utoa_ADDR               (0x00000F39 + ROM_CODE_BASE_OFFSET)
#define xinyi_utoa_ADDR                 (0x00000FBD + ROM_CODE_BASE_OFFSET)
#define xinyi_memccpy_ADDR              (0x00000FC1 + ROM_CODE_BASE_OFFSET)
#define xinyi_memchr_ADDR               (0x00000FDF + ROM_CODE_BASE_OFFSET)
#define xinyi_memcmp_ADDR               (0x00000FFB + ROM_CODE_BASE_OFFSET)
#define xinyi_memcpy_ADDR               (0x00001017 + ROM_CODE_BASE_OFFSET)
#define xinyi_memmove_ADDR              (0x0000102D + ROM_CODE_BASE_OFFSET)
#define xinyi_memset_ADDR               (0x00001061 + ROM_CODE_BASE_OFFSET)
#define xinyi_stpcpy_ADDR               (0x00001071 + ROM_CODE_BASE_OFFSET)
#define xinyi_stpncpy_ADDR              (0x00001083 + ROM_CODE_BASE_OFFSET)
#define xinyi_strcat_ADDR               (0x000010B7 + ROM_CODE_BASE_OFFSET)
#define xinyi_strchr_ADDR               (0x000010D5 + ROM_CODE_BASE_OFFSET)
#define xinyi_strcmp_ADDR               (0x000010EF + ROM_CODE_BASE_OFFSET)
#define xinyi_strcoll_ADDR              (0x00001109 + ROM_CODE_BASE_OFFSET)
#define xinyi_strcpy_ADDR               (0x0000110D + ROM_CODE_BASE_OFFSET)
#define xinyi_strcspn_ADDR              (0x0000111F + ROM_CODE_BASE_OFFSET)
#define xinyi_strlcat_ADDR              (0x00001141 + ROM_CODE_BASE_OFFSET)
#define xinyi_strlcpy_ADDR              (0x00001189 + ROM_CODE_BASE_OFFSET)
#define xinyi_strlen_ADDR               (0x000011B3 + ROM_CODE_BASE_OFFSET)
#define xinyi_strncat_ADDR              (0x000011C5 + ROM_CODE_BASE_OFFSET)
#define xinyi_strncmp_ADDR              (0x000011EF + ROM_CODE_BASE_OFFSET)
#define xinyi_strncpy_ADDR              (0x00001217 + ROM_CODE_BASE_OFFSET)
#define xinyi_strnlen_ADDR              (0x0000123D + ROM_CODE_BASE_OFFSET)
#define xinyi_strpbrk_ADDR              (0x00001257 + ROM_CODE_BASE_OFFSET)
#define xinyi_strrchr_ADDR              (0x0000128F + ROM_CODE_BASE_OFFSET)
#define xinyi_strsep_ADDR               (0x000012B7 + ROM_CODE_BASE_OFFSET)
#define xinyi_strspn_ADDR               (0x000012C1 + ROM_CODE_BASE_OFFSET)
#define __xinyi_strtok_r_ADDR           (0x000012E1 + ROM_CODE_BASE_OFFSET)
#define xinyi_strtok_r_ADDR             (0x00001333 + ROM_CODE_BASE_OFFSET)
#define xinyi_strxfrm_ADDR              (0x00001339 + ROM_CODE_BASE_OFFSET)
#define xinyi_timingsafe_bcmp_ADDR      (0x00001363 + ROM_CODE_BASE_OFFSET)
#define xinyi_timingsafe_memcmp_ADDR    (0x00001383 + ROM_CODE_BASE_OFFSET)
#define InvertUint8_ADDR                (0x000013B1 + ROM_CODE_BASE_OFFSET)
#define InvertUint16_ADDR               (0x000013DD + ROM_CODE_BASE_OFFSET)
#define CRC16_CCITT_ADDR                (0x00001409 + ROM_CODE_BASE_OFFSET)
#define CRC16_CCITT_FALSE_ADDR          (0x0000146D + ROM_CODE_BASE_OFFSET)
#define CRC16_XMODEM_ADDR               (0x0000149F + ROM_CODE_BASE_OFFSET)
#define CRC16_X25_ADDR                  (0x000014CF + ROM_CODE_BASE_OFFSET)
#define CRC16_MODBUS_ADDR               (0x0000153B + ROM_CODE_BASE_OFFSET)
#define CRC16_IBM_ADDR                  (0x000015A3 + ROM_CODE_BASE_OFFSET)
#define CRC16_MAXIM_ADDR                (0x00001607 + ROM_CODE_BASE_OFFSET)
#define CRC16_USB_ADDR                  (0x0000166F + ROM_CODE_BASE_OFFSET)
#define clock_days_before_month_ADDR    (0x000016DD + ROM_CODE_BASE_OFFSET)
#define clock_calendar_to_utc_ADDR      (0x000016F5 + ROM_CODE_BASE_OFFSET)
#define xy_mktime_ADDR                  (0x00001749 + ROM_CODE_BASE_OFFSET)
#define xy_gmtime_r_ADDR                (0x00001821 + ROM_CODE_BASE_OFFSET)


/* function type */
typedef void (*xinyi_putchar_type) (char);
typedef int (*xinyi_printf_type) (const char*, ...);
typedef int (*xinyi_sprintf_type) (char*, const char*, ...);
typedef int (*xinyi_snprintf_type) (char*, size_t, const char*, ...);
typedef int (*xinyi_vprintf_type) (const char*, va_list);
typedef int (*xinyi_vsnprintf_type) (char*, size_t, const char*, va_list);
typedef int (*fctprintf_type) (void);
typedef int (*xinyi_abs_type) (int);
typedef char * (*__xinyi_itoa_type) (int, char *, int);
typedef char * (*xinyi_itoa_type) (int, char *, int);
typedef char * (*__xinyi_utoa_type) (unsigned, char *, int);
typedef char * (*xinyi_utoa_type) (unsigned, char *, int);
typedef void * (*xinyi_memccpy_type) (void *__restrict, const void *__restrict, int, size_t);
typedef void * (*xinyi_memchr_type) (const void *, int, size_t);
typedef int (*xinyi_memcmp_type) (const void *, const void *, size_t);
typedef void * (*xinyi_memcpy_type) (void *__restrict, const void *__restrict, size_t);
typedef void * (*xinyi_memmove_type) (void *, const void *, size_t);
typedef void * (*xinyi_memset_type) (void *, int, size_t);
typedef char* (*xinyi_stpcpy_type) (char *__restrict, const char *__restrict);
typedef char * (*xinyi_stpncpy_type) (char *__restrict, const char *__restrict, size_t);
typedef char * (*xinyi_strcat_type) (char *__restrict, const char *__restrict);
typedef char * (*xinyi_strchr_type) (const char *, int);
typedef int (*xinyi_strcmp_type) (const char *, const char *);
typedef int (*xinyi_strcoll_type) (const char *, const char *);
typedef char* (*xinyi_strcpy_type) (char *, const char *);
typedef size_t (*xinyi_strcspn_type) (const char *, const char *);
typedef size_t (*xinyi_strlcat_type) (char *, const char *, size_t);
typedef size_t (*xinyi_strlcpy_type) (char *, const char *, size_t);
typedef size_t (*xinyi_strlen_type) (const char *);
typedef char * (*xinyi_strncat_type) (char *__restrict, const char *__restrict, size_t);
typedef int (*xinyi_strncmp_type) (const char *, const char *, size_t);
typedef char * (*xinyi_strncpy_type) (char *__restrict, const char *__restrict, size_t);
typedef size_t (*xinyi_strnlen_type) (const char *, size_t);
typedef char * (*xinyi_strpbrk_type) (const char *, const char *);
typedef char * (*xinyi_strrchr_type) (const char *, int);
typedef char * (*xinyi_strsep_type) (register char **, register const char *);
typedef size_t (*xinyi_strspn_type) (const char *, const char *);
typedef char * (*__xinyi_strtok_r_type) (register char *, register const char *, char **, int);
typedef char * (*xinyi_strtok_r_type) (register char *__restrict, register const char *__restrict, char **__restrict);
typedef size_t (*xinyi_strxfrm_type) (char *__restrict, const char *__restrict, size_t);
typedef int (*xinyi_timingsafe_bcmp_type) (const void *, const void *, size_t);
typedef int (*xinyi_timingsafe_memcmp_type) (const void *, const void *, size_t);
typedef void (*InvertUint8_type) (uint8_t *, uint8_t *);
typedef void (*InvertUint16_type) (uint16_t *, uint16_t *);
typedef uint16_t (*CRC16_CCITT_type) (uint8_t *, uint32_t);
typedef uint16_t (*CRC16_CCITT_FALSE_type) (uint8_t *, uint32_t);
typedef uint16_t (*CRC16_XMODEM_type) (uint8_t *, uint32_t);
typedef uint16_t (*CRC16_X25_type) (uint8_t *, uint32_t);
typedef uint16_t (*CRC16_MODBUS_type) (uint8_t *, uint32_t);
typedef uint16_t (*CRC16_IBM_type) (uint8_t *, uint32_t);
typedef uint16_t (*CRC16_MAXIM_type) (uint8_t *, uint32_t);
typedef uint16_t (*CRC16_USB_type) (uint8_t *, uint32_t);
typedef int (*clock_days_before_month_type) (int, bool);
typedef uint32_t (*clock_calendar_to_utc_type) (int, int, int);
//typedef uint64_t (*xy_mktime_type) (struct rtc_time *);
//typedef void (*xy_gmtime_r_type) (const uint64_t, struct rtc_time *);


/* function definition */
#define xinyi_putchar(par1)                          ((xinyi_putchar_type)xinyi_putchar_ADDR)(par1)
#define xinyi_printf(par1, ...)                      ((xinyi_printf_type)xinyi_printf_ADDR)(par1, __VA_ARGS__)
#define xinyi_sprintf(par1, par2, ...)               ((xinyi_sprintf_type)xinyi_sprintf_ADDR)(par1, par2, __VA_ARGS__)
#define xinyi_snprintf(par1, par2, par3, ...)        ((xinyi_snprintf_type)xinyi_snprintf_ADDR)(par1, par2, par3, __VA_ARGS__)
#define xinyi_vprintf(par1, par2)                    ((xinyi_vprintf_type)xinyi_vprintf_ADDR)(par1, par2)
#define xinyi_vsnprintf(par1, par2, par3, par4)      ((xinyi_vsnprintf_type)xinyi_vsnprintf_ADDR)(par1, par2, par3, par4)
#define fctprintf()                                  ((fctprintf_type)fctprintf_ADDR)()
#define xinyi_abs(par1)                              ((xinyi_abs_type)xinyi_abs_ADDR)(par1)
#define __xinyi_itoa(par1, par2, par3)               ((__xinyi_itoa_type)__xinyi_itoa_ADDR)(par1, par2, par3)
#define xinyi_itoa(par1, par2, par3)                 ((xinyi_itoa_type)xinyi_itoa_ADDR)(par1, par2, par3)
#define __xinyi_utoa(par1, par2, par3)               ((__xinyi_utoa_type)__xinyi_utoa_ADDR)(par1, par2, par3)
#define xinyi_utoa(par1, par2, par3)                 ((xinyi_utoa_type)xinyi_utoa_ADDR)(par1, par2, par3)
#define xinyi_memccpy(par1, par2, par3, par4)        ((xinyi_memccpy_type)xinyi_memccpy_ADDR)(par1, par2, par3, par4)
#define xinyi_memchr(par1, par2, par3)               ((xinyi_memchr_type)xinyi_memchr_ADDR)(par1, par2, par3)
#define xinyi_memcmp(par1, par2, par3)               ((xinyi_memcmp_type)xinyi_memcmp_ADDR)(par1, par2, par3)
#define xinyi_memcpy(par1, par2, par3)               ((xinyi_memcpy_type)xinyi_memcpy_ADDR)(par1, par2, par3)
#define xinyi_memmove(par1, par2, par3)              ((xinyi_memmove_type)xinyi_memmove_ADDR)(par1, par2, par3)
#define xinyi_memset(par1, par2, par3)               ((xinyi_memset_type)xinyi_memset_ADDR)(par1, par2, par3)
#define xinyi_stpcpy(par1, par2)                     ((xinyi_stpcpy_type)xinyi_stpcpy_ADDR)(par1, par2)
#define xinyi_stpncpy(par1, par2, par3)              ((xinyi_stpncpy_type)xinyi_stpncpy_ADDR)(par1, par2, par3)
#define xinyi_strcat(par1, par2)                     ((xinyi_strcat_type)xinyi_strcat_ADDR)(par1, par2)
#define xinyi_strchr(par1, par2)                     ((xinyi_strchr_type)xinyi_strchr_ADDR)(par1, par2)
#define xinyi_strcmp(par1, par2)                     ((xinyi_strcmp_type)xinyi_strcmp_ADDR)(par1, par2)
#define xinyi_strcoll(par1, par2)                    ((xinyi_strcoll_type)xinyi_strcoll_ADDR)(par1, par2)
#define xinyi_strcpy(par1, par2)                     ((xinyi_strcpy_type)xinyi_strcpy_ADDR)(par1, par2)
#define xinyi_strcspn(par1, par2)                    ((xinyi_strcspn_type)xinyi_strcspn_ADDR)(par1, par2)
#define xinyi_strlcat(par1, par2, par3)              ((xinyi_strlcat_type)xinyi_strlcat_ADDR)(par1, par2, par3)
#define xinyi_strlcpy(par1, par2, par3)              ((xinyi_strlcpy_type)xinyi_strlcpy_ADDR)(par1, par2, par3)
#define xinyi_strlen(par1)                           ((xinyi_strlen_type)xinyi_strlen_ADDR)(par1)
#define xinyi_strncat(par1, par2, par3)              ((xinyi_strncat_type)xinyi_strncat_ADDR)(par1, par2, par3)
#define xinyi_strncmp(par1, par2, par3)              ((xinyi_strncmp_type)xinyi_strncmp_ADDR)(par1, par2, par3)
#define xinyi_strncpy(par1, par2, par3)              ((xinyi_strncpy_type)xinyi_strncpy_ADDR)(par1, par2, par3)
#define xinyi_strnlen(par1, par2)                    ((xinyi_strnlen_type)xinyi_strnlen_ADDR)(par1, par2)
#define xinyi_strpbrk(par1, par2)                    ((xinyi_strpbrk_type)xinyi_strpbrk_ADDR)(par1, par2)
#define xinyi_strrchr(par1, par2)                    ((xinyi_strrchr_type)xinyi_strrchr_ADDR)(par1, par2)
#define xinyi_strsep(par1, par2)                     ((xinyi_strsep_type)xinyi_strsep_ADDR)(par1, par2)
#define xinyi_strspn(par1, par2)                     ((xinyi_strspn_type)xinyi_strspn_ADDR)(par1, par2)
#define __xinyi_strtok_r(par1, par2, par3, par4)     ((__xinyi_strtok_r_type)__xinyi_strtok_r_ADDR)(par1, par2, par3, par4)
#define xinyi_strtok_r(par1, par2, par3)             ((xinyi_strtok_r_type)xinyi_strtok_r_ADDR)(par1, par2, par3)
#define xinyi_strxfrm(par1, par2, par3)              ((xinyi_strxfrm_type)xinyi_strxfrm_ADDR)(par1, par2, par3)
#define xinyi_timingsafe_bcmp(par1, par2, par3)      ((xinyi_timingsafe_bcmp_type)xinyi_timingsafe_bcmp_ADDR)(par1, par2, par3)
#define xinyi_timingsafe_memcmp(par1, par2, par3)    ((xinyi_timingsafe_memcmp_type)xinyi_timingsafe_memcmp_ADDR)(par1, par2, par3)
#define InvertUint8(par1, par2)                      ((InvertUint8_type)InvertUint8_ADDR)(par1, par2)
#define InvertUint16(par1, par2)                     ((InvertUint16_type)InvertUint16_ADDR)(par1, par2)
#define CRC16_CCITT(par1, par2)                      ((CRC16_CCITT_type)CRC16_CCITT_ADDR)(par1, par2)
#define CRC16_CCITT_FALSE(par1, par2)                ((CRC16_CCITT_FALSE_type)CRC16_CCITT_FALSE_ADDR)(par1, par2)
#define CRC16_XMODEM(par1, par2)                     ((CRC16_XMODEM_type)CRC16_XMODEM_ADDR)(par1, par2)
#define CRC16_X25(par1, par2)                        ((CRC16_X25_type)CRC16_X25_ADDR)(par1, par2)
#define CRC16_MODBUS(par1, par2)                     ((CRC16_MODBUS_type)CRC16_MODBUS_ADDR)(par1, par2)
#define CRC16_IBM(par1, par2)                        ((CRC16_IBM_type)CRC16_IBM_ADDR)(par1, par2)
#define CRC16_MAXIM(par1, par2)                      ((CRC16_MAXIM_type)CRC16_MAXIM_ADDR)(par1, par2)
#define CRC16_USB(par1, par2)                        ((CRC16_USB_type)CRC16_USB_ADDR)(par1, par2)
#define clock_days_before_month(par1, par2)          ((clock_days_before_month_type)clock_days_before_month_ADDR)(par1, par2)
#define clock_calendar_to_utc(par1, par2, par3)      ((clock_calendar_to_utc_type)clock_calendar_to_utc_ADDR)(par1, par2, par3)
//#define xy_mktime(par1)                              ((xy_mktime_type)xy_mktime_ADDR)(par1)
//#define xy_gmtime_r(par1, par2)                      ((xy_gmtime_r_type)xy_gmtime_r_ADDR)(par1, par2)


/* const data definition */


#endif  /* ROM_FUCN_H */
