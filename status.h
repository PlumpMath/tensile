/** @file
 * @brief error handling routines
 *
 * @author Artem V. Andreev <artem@iling.spb.ru>
 */
#ifndef STATUS_H
#define STATUS_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <com_err.h>
#include <stdarg.h>
#include "compiler.h"

typedef errcode_t tn_status;

enum tn_severity {
    TN_FATAL,
    TN_EXCEPTION,
    TN_ERROR,
    TN_WARNING,
    TN_NOTICE,
    TN_INFO,
    TN_TRACE,
};

extern enum tn_severity tn_verbosity_level;

hint_printf_like(4, 5)
extern void tn_report_status(enum tn_severity severity,
                             const char *module,
                             tn_status status,
                             const char *fmt, ...);

hint_printf_like(4, 0)
extern void tn_report_statusv(enum tn_severity severity,
                              const char *module,
                              tn_status status,
                              const char *fmt, va_list args);

hint_printf_like(3, 4)
extern noreturn void tn_fatal_error(const char *module, tn_status status,
                                    const char *fmt, ...);

hint_printf_like(3, 4)
extern noreturn void tn_throw_exception(const char *module, tn_status status,
                                        const char *fmt, ...);

#define tn_internal_error(_severity, _status, _fmt, ...)                \
    (tn_report_status(_severity, __FILE__, _status,                     \
                     "%s():%d: " _fmt, __FUNCTION__, __LINE__, __VA_ARGS__))

typedef tn_status (*tn_exception_handler)(void *data, const char *module,
                                          tn_status status, const char *msg);

warn_unused_result
warn_null_args(1)
extern tn_status tn_with_exception(tn_status (*action)(void *),
                                   tn_exception_handler handler,
                                   void *data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* STATUS_H */
