#include <cstdarg>
namespace {
    void looper_internal_recursion(bool (*loopee)(va_list), va_list vl)
    {
        va_list stored;
        va_copy(stored, vl);
        if (loopee(vl)) {
            looper_internal_recursion(loopee, stored);
        }
        va_end(stored);
        return;
    }
}
extern "C" void looper(bool (*loopee)(va_list), ...)
{
    va_list vl;
    va_start(vl, loopee);
    looper_internal_recursion(loopee, vl);
    va_end(vl);
    return;
}
