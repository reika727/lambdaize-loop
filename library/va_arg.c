#include <stdarg.h>
void *va_arg_ptr(va_list vl)
{
    return va_arg(vl, void *);
}
