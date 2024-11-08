#include "diag_options.h"
#include "diag_format.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>


// define this globally (e.g. gcc -DPRINTF_INCLUDE_CONFIG_H ...) to include the
// printf_config.h header file
// default: undefined
#ifdef PRINTF_INCLUDE_CONFIG_H
#include "printf_config.h"
#endif


// support for the floating point type (%f)
// default: activated
#ifndef PRINTF_DISABLE_SUPPORT_FLOAT
#define PRINTF_SUPPORT_FLOAT
#endif

// support for exponential floating point notation (%e/%g)
// default: activated
#ifndef PRINTF_DISABLE_SUPPORT_EXPONENTIAL
#define PRINTF_SUPPORT_EXPONENTIAL
#endif

// support for the long long types (%llu or %p)
// default: activated
#ifndef PRINTF_DISABLE_SUPPORT_LONG_LONG
#define PRINTF_SUPPORT_LONG_LONG
#endif

// support for the ptrdiff_t type (%t)
// ptrdiff_t is normally defined in <stddef.h> as long or long long type
// default: activated
#ifndef PRINTF_DISABLE_SUPPORT_PTRDIFF_T
#define PRINTF_SUPPORT_PTRDIFF_T
#endif

///////////////////////////////////////////////////////////////////////////////

// internal flag definitions
#define FLAGS_ZEROPAD   (1U <<  0U)
#define FLAGS_LEFT      (1U <<  1U)
#define FLAGS_PLUS      (1U <<  2U)
#define FLAGS_SPACE     (1U <<  3U)
#define FLAGS_HASH      (1U <<  4U)
#define FLAGS_UPPERCASE (1U <<  5U)
#define FLAGS_CHAR      (1U <<  6U)
#define FLAGS_SHORT     (1U <<  7U)
#define FLAGS_LONG      (1U <<  8U)
#define FLAGS_LONG_LONG (1U <<  9U)
#define FLAGS_PRECISION (1U << 10U)
#define FLAGS_ADAPT_EXP (1U << 11U)


// import float.h for DBL_MAX
#if defined(PRINTF_SUPPORT_FLOAT)
#include <float.h>
#endif


// internal secure strlen
// \return The length of the string (excluding the terminating 0) limited by 'maxsize'
__RAM_FUNC static inline unsigned int _strnlen_s(const char* str, size_t maxsize)
{
  const char* s;
  for (s = str; *s && maxsize--; ++s);
  return (unsigned int)(s - str);
}
/*----------------------------------------------------------------------------------------------------*/


// internal test if char is a digit (0-9)
// \return true if char is a digit
__RAM_FUNC static inline bool _is_digit(char ch)
{
  return (ch >= '0') && (ch <= '9');
}
/*----------------------------------------------------------------------------------------------------*/


// internal ASCII string to unsigned int conversion
__RAM_FUNC static inline void _skip_digit(const char** str)
{
  while (_is_digit(**str)) {
    (*str)++;
  }
}
/*----------------------------------------------------------------------------------------------------*/


__RAM_FUNC static inline size_t _out_buffer_int(int data, void* buffer, size_t idx, size_t maxlen)
{
  size_t new_idx = idx + sizeof(int);
  int * write_to;
  if (new_idx <= maxlen) {
    write_to = (int *)((char *)buffer + idx);
    *write_to = data;
    return new_idx;
  }
  return (size_t)-1;
}
/*----------------------------------------------------------------------------------------------------*/


__RAM_FUNC static inline size_t _out_buffer_long(long data, void* buffer, size_t idx, size_t maxlen)
{
  size_t new_idx = idx + sizeof(long);
  long * write_to;
  if (new_idx <= maxlen) {
    write_to = (long *)((char *)buffer + idx);
    *write_to = data;
    return new_idx;
  }
  return (size_t)-1;
}
/*----------------------------------------------------------------------------------------------------*/


__RAM_FUNC static inline size_t _out_buffer_long_long(long long data, void* buffer, size_t idx, size_t maxlen)
{
  size_t new_idx = idx + sizeof(long long);
  long long * write_to;
  if (new_idx <= maxlen) {
    write_to = (long long *)((char *)buffer + idx);
    *write_to = data;
    return new_idx;
  }
  return (size_t)-1;
}
/*----------------------------------------------------------------------------------------------------*/


__RAM_FUNC static inline size_t _out_buffer_double(double data, void* buffer, size_t idx, size_t maxlen)
{
  size_t new_idx = idx + sizeof(double);
  double * write_to;
  if (new_idx <= maxlen) {
    write_to = (double *)((char *)buffer + idx);
    *write_to = data;
    return new_idx;
  }
  return (size_t)-1;
}
/*----------------------------------------------------------------------------------------------------*/


__RAM_FUNC static inline size_t _out_buffer_string(const char* data, void* buffer, size_t idx, size_t maxlen)
{
  unsigned int str_len = _strnlen_s(data, (size_t)-1);
  size_t new_idx = idx + str_len + 1;
  char * write_to = (char *)buffer + idx;
  unsigned int i;
  new_idx = (new_idx + 3) & 0xFFFFFFFC; 
  if (new_idx <= maxlen) {
    for(i = 0; i < str_len; i++) {
      write_to[i] = data[i];
    }
    write_to[i] = '\0';
    return new_idx;
  }
  return (size_t)-1;
}
/*----------------------------------------------------------------------------------------------------*/


// internal vsnprintf
__RAM_FUNC int diag_format_get_arguments_length(const char* format, va_list va)
{
  unsigned int flags, n;
  size_t idx = 0U;

  while (*format)
  {
    // format specifier?  %[flags][width][.precision][length]
    if (*format != '%') {
      format++;
      continue;
    }
    else {
      // yes, evaluate it
      format++;
    }

    // evaluate flags
    flags = 0U;
    do {
      switch (*format) {
        case '0': flags |= FLAGS_ZEROPAD; format++; n = 1U; break;
        case '-': flags |= FLAGS_LEFT;    format++; n = 1U; break;
        case '+': flags |= FLAGS_PLUS;    format++; n = 1U; break;
        case ' ': flags |= FLAGS_SPACE;   format++; n = 1U; break;
        case '#': flags |= FLAGS_HASH;    format++; n = 1U; break;
        default :                                   n = 0U; break;
      }
    } while (n);

    // evaluate width field
    if (_is_digit(*format)) {
      _skip_digit(&format);
    }
    else if (*format == '*') {
      const int w = va_arg(va, int);
      if (w < 0) {
        flags |= FLAGS_LEFT;    // reverse padding
      }
      format++;
      idx += sizeof(int);
    }

    // evaluate precision field
    if (*format == '.') {
      flags |= FLAGS_PRECISION;
      format++;
      if (_is_digit(*format)) {
        _skip_digit(&format);
      }
      else if (*format == '*') {
        (void) va_arg(va, int);
        format++;
        idx += sizeof(int);
      }
    }

    // evaluate length field
    switch (*format) {
      case 'l' :
        flags |= FLAGS_LONG;
        format++;
        if (*format == 'l') {
          flags |= FLAGS_LONG_LONG;
          format++;
        }
        break;
      case 'h' :
        flags |= FLAGS_SHORT;
        format++;
        if (*format == 'h') {
          flags |= FLAGS_CHAR;
          format++;
        }
        break;
#if defined(PRINTF_SUPPORT_PTRDIFF_T)
      case 't' :
        flags |= (sizeof(ptrdiff_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
        format++;
        break;
#endif
      case 'j' :
        flags |= (sizeof(intmax_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
        format++;
        break;
      case 'z' :
        flags |= (sizeof(size_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
        format++;
        break;
      default :
        break;
    }

    // evaluate specifier
    switch (*format) {
      case 'd' :
      case 'i' :
      case 'u' :
      case 'x' :
      case 'X' :
      case 'o' :
      case 'b' : {
        // set the base
        // unsigned int base;
        if (*format == 'x' || *format == 'X') {
          // base = 16U;
        }
        else if (*format == 'o') {
          // base =  8U;
        }
        else if (*format == 'b') {
          // base =  2U;
        }
        else {
          // base = 10U;
          flags &= ~FLAGS_HASH;   // no hash for dec format
        }
        // uppercase
        if (*format == 'X') {
          flags |= FLAGS_UPPERCASE;
        }

        // no plus or space flag for u, x, X, o, b
        if ((*format != 'i') && (*format != 'd')) {
          flags &= ~(FLAGS_PLUS | FLAGS_SPACE);
        }

        // ignore '0' flag when precision is given
        if (flags & FLAGS_PRECISION) {
          flags &= ~FLAGS_ZEROPAD;
        }

        // convert the integer
        if ((*format == 'i') || (*format == 'd')) {
          // signed
          if (flags & FLAGS_LONG_LONG) {
#if defined(PRINTF_SUPPORT_LONG_LONG)
            (void) va_arg(va, long long);
            idx += sizeof(long long);
#endif
          }
          else if (flags & FLAGS_LONG) {
            (void) va_arg(va, long);
            idx += sizeof(long);
          }
          else {
            (void) ( (flags & FLAGS_CHAR) ? (char)va_arg(va, int) : (flags & FLAGS_SHORT) ? (short int)va_arg(va, int) : va_arg(va, int) );
            idx += sizeof(int);
          }
        }
        else {
          // unsigned
          if (flags & FLAGS_LONG_LONG) {
#if defined(PRINTF_SUPPORT_LONG_LONG)
            (void) va_arg(va, unsigned long long);
            idx += sizeof(long long);
#endif
          }
          else if (flags & FLAGS_LONG) {
            (void) va_arg(va, long);
            idx += sizeof(long);
          }
          else {
            (void) ( (flags & FLAGS_CHAR) ? (unsigned char)va_arg(va, unsigned int) : (flags & FLAGS_SHORT) ? (unsigned short int)va_arg(va, unsigned int) : va_arg(va, unsigned int) );
            idx += sizeof(int);
          }
        }
        format++;
        break;
      }
#if defined(PRINTF_SUPPORT_FLOAT)
      case 'f' :
      case 'F' : {
        if (*format == 'F') flags |= FLAGS_UPPERCASE;
        (void) va_arg(va, double);
        idx += sizeof(double);
        format++;
        break;
      }
#if defined(PRINTF_SUPPORT_EXPONENTIAL)
      case 'e' :
      case 'E' :
      case 'g' :
      case 'G' : {
        if ((*format == 'g')||(*format == 'G')) flags |= FLAGS_ADAPT_EXP;
        if ((*format == 'E')||(*format == 'G')) flags |= FLAGS_UPPERCASE;
        (void) va_arg(va, double);
        idx += sizeof(double);
        format++;
        break;
      }
#endif  // PRINTF_SUPPORT_EXPONENTIAL
#endif  // PRINTF_SUPPORT_FLOAT
      case 'c' : {
        // char output
        (void) (char)va_arg(va, int);
        idx += sizeof(int);
        format++;
        break;
      }

      case 's' : {
        const char* p = va_arg(va, char*);
        idx += _strnlen_s(p, (size_t)-1) + 1;
        idx = (idx + 3) & 0xFFFFFFFC;
        format++;
        break;
      }

      case 'p' : {
        flags |= FLAGS_ZEROPAD | FLAGS_UPPERCASE;
#if defined(PRINTF_SUPPORT_LONG_LONG)
        const bool is_ll = sizeof(uintptr_t) == sizeof(long long);
        if (is_ll) {
          (void) (uintptr_t)va_arg(va, void*);
          idx += sizeof(long long);
        }
        else {
#endif
          (void) (uintptr_t)va_arg(va, void*);
          idx += sizeof(long);
#if defined(PRINTF_SUPPORT_LONG_LONG)
        }
#endif
        format++;
        break;
      }

      case '%' :
        format++;
        break;

      default :
        format++;
        break;
    }
  }

  // return written chars without terminating \0
  return (int)idx;
}
/*----------------------------------------------------------------------------------------------------*/


// internal vsnprintf
__RAM_FUNC int diag_format_arguments_to_buffer(char* buffer, const size_t maxlen, const char* format, va_list va)
{
  unsigned int flags, n;
  size_t idx = 0U;

  while (*format)
  {
    // format specifier?  %[flags][width][.precision][length]
    if (*format != '%') {
      format++;
      continue;
    }
    else {
      // yes, evaluate it
      format++;
    }

    // evaluate flags
    flags = 0U;
    do {
      switch (*format) {
        case '0': flags |= FLAGS_ZEROPAD; format++; n = 1U; break;
        case '-': flags |= FLAGS_LEFT;    format++; n = 1U; break;
        case '+': flags |= FLAGS_PLUS;    format++; n = 1U; break;
        case ' ': flags |= FLAGS_SPACE;   format++; n = 1U; break;
        case '#': flags |= FLAGS_HASH;    format++; n = 1U; break;
        default :                                   n = 0U; break;
      }
    } while (n);

    // evaluate width field
    if (_is_digit(*format)) {
      _skip_digit(&format);
    }
    else if (*format == '*') {
      const int w = va_arg(va, int);
      if (w < 0) {
        flags |= FLAGS_LEFT;    // reverse padding
      }
      format++;
      idx = _out_buffer_int(w, buffer, idx, maxlen);
      if(idx == (size_t)-1) return -1;
    }

    // evaluate precision field
    if (*format == '.') {
      flags |= FLAGS_PRECISION;
      format++;
      if (_is_digit(*format)) {
        _skip_digit(&format);
      }
      else if (*format == '*') {
        const int prec = (int)va_arg(va, int);
        format++;
        idx = _out_buffer_int(prec, buffer, idx, maxlen);
        if(idx == (size_t)-1) return -1;
      }
    }

    // evaluate length field
    switch (*format) {
      case 'l' :
        flags |= FLAGS_LONG;
        format++;
        if (*format == 'l') {
          flags |= FLAGS_LONG_LONG;
          format++;
        }
        break;
      case 'h' :
        flags |= FLAGS_SHORT;
        format++;
        if (*format == 'h') {
          flags |= FLAGS_CHAR;
          format++;
        }
        break;
#if defined(PRINTF_SUPPORT_PTRDIFF_T)
      case 't' :
        flags |= (sizeof(ptrdiff_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
        format++;
        break;
#endif
      case 'j' :
        flags |= (sizeof(intmax_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
        format++;
        break;
      case 'z' :
        flags |= (sizeof(size_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
        format++;
        break;
      default :
        break;
    }

    // evaluate specifier
    switch (*format) {
      case 'd' :
      case 'i' :
      case 'u' :
      case 'x' :
      case 'X' :
      case 'o' :
      case 'b' : {
        // set the base
        // unsigned int base;
        if (*format == 'x' || *format == 'X') {
          // base = 16U;
        }
        else if (*format == 'o') {
          // base =  8U;
        }
        else if (*format == 'b') {
          // base =  2U;
        }
        else {
          // base = 10U;
          flags &= ~FLAGS_HASH;   // no hash for dec format
        }
        // uppercase
        if (*format == 'X') {
          flags |= FLAGS_UPPERCASE;
        }

        // no plus or space flag for u, x, X, o, b
        if ((*format != 'i') && (*format != 'd')) {
          flags &= ~(FLAGS_PLUS | FLAGS_SPACE);
        }

        // ignore '0' flag when precision is given
        if (flags & FLAGS_PRECISION) {
          flags &= ~FLAGS_ZEROPAD;
        }

        // convert the integer
        if ((*format == 'i') || (*format == 'd')) {
          // signed
          if (flags & FLAGS_LONG_LONG) {
#if defined(PRINTF_SUPPORT_LONG_LONG)
            const long long value = va_arg(va, long long);
            idx = _out_buffer_long_long(value, buffer, idx, maxlen);
            if(idx == (size_t)-1) return -1;
#endif
          }
          else if (flags & FLAGS_LONG) {
            const long value = va_arg(va, long);
            idx = _out_buffer_long(value, buffer, idx, maxlen);
            if(idx == (size_t)-1) return -1;
          }
          else {
            const int value = (flags & FLAGS_CHAR) ? (char)va_arg(va, int) : (flags & FLAGS_SHORT) ? (short int)va_arg(va, int) : va_arg(va, int);
            idx = _out_buffer_int(value, buffer, idx, maxlen);
            if(idx == (size_t)-1) return -1;
          }
        }
        else {
          // unsigned
          if (flags & FLAGS_LONG_LONG) {
#if defined(PRINTF_SUPPORT_LONG_LONG)
            const unsigned long long value = va_arg(va, unsigned long long);
            idx = _out_buffer_long_long((long long)value, buffer, idx, maxlen);
            if(idx == (size_t)-1) return -1;
#endif
          }
          else if (flags & FLAGS_LONG) {
            const long value = va_arg(va, long);
            idx = _out_buffer_long(value, buffer, idx, maxlen);
            if(idx == (size_t)-1) return -1;
          }
          else {
            const unsigned int value = (flags & FLAGS_CHAR) ? (unsigned char)va_arg(va, unsigned int) : (flags & FLAGS_SHORT) ? (unsigned short int)va_arg(va, unsigned int) : va_arg(va, unsigned int);
            idx = _out_buffer_int((int)value, buffer, idx, maxlen);
            if(idx == (size_t)-1) return -1;
          }
        }
        format++;
        break;
      }
#if defined(PRINTF_SUPPORT_FLOAT)
      case 'f' :
      case 'F' : {
        if (*format == 'F') flags |= FLAGS_UPPERCASE;
        const double value = va_arg(va, double);
        idx = _out_buffer_double(value, buffer, idx, maxlen);
        if(idx == (size_t)-1) return -1;
        format++;
        break;
      }
#if defined(PRINTF_SUPPORT_EXPONENTIAL)
      case 'e' :
      case 'E' :
      case 'g' :
      case 'G' : {
        if ((*format == 'g')||(*format == 'G')) flags |= FLAGS_ADAPT_EXP;
        if ((*format == 'E')||(*format == 'G')) flags |= FLAGS_UPPERCASE;
        const double value = va_arg(va, double);
        idx = _out_buffer_double(value, buffer, idx, maxlen);
        if(idx == (size_t)-1) return -1;
        format++;
        break;
      }
#endif  // PRINTF_SUPPORT_EXPONENTIAL
#endif  // PRINTF_SUPPORT_FLOAT
      case 'c' : {
        // char output
        const char value = (char) va_arg(va, int);
        idx = _out_buffer_int((int)value, buffer, idx, maxlen);
        if(idx == (size_t)-1) return -1;
        format++;
        break;
      }

      case 's' : {
        const char* p = va_arg(va, char*);
        idx = _out_buffer_string(p, buffer, idx, maxlen);
        if(idx == (size_t)-1) return -1;
        format++;
        break;
      }

      case 'p' : {
        flags |= FLAGS_ZEROPAD | FLAGS_UPPERCASE;
#if defined(PRINTF_SUPPORT_LONG_LONG)
        const bool is_ll = sizeof(uintptr_t) == sizeof(long long);
        if (is_ll) {
          const uintptr_t value = (uintptr_t)va_arg(va, void*);
          idx = _out_buffer_long_long((long long)value, buffer, idx, maxlen);
          if(idx == (size_t)-1) return -1;
        }
        else {
#endif
          const unsigned long value = (uintptr_t)va_arg(va, void*);
          idx = _out_buffer_long((long)value, buffer, idx, maxlen);
          if(idx == (size_t)-1) return -1;
#if defined(PRINTF_SUPPORT_LONG_LONG)
        }
#endif
        format++;
        break;
      }

      case '%' :
        format++;
        break;

      default :
        format++;
        break;
    }
  }

  // return written chars without terminating \0
  return (int)idx;
}
/*----------------------------------------------------------------------------------------------------*/


__RAM_FUNC void diag_format_fixed_args(void* buffer, const size_t arg_num, va_list va)
{
  size_t i;
  int * data = (int *)buffer;
  for (i = 0; i < arg_num; i++) {
    data[i] = (int) va_arg(va, int);
  }
}
/*----------------------------------------------------------------------------------------------------*/
