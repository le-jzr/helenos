#pragma once

#include <_bits/__noreturn.h>

__noreturn void __panic(const char *fmt, ...) __attribute__((__format__ (__printf__, 1, 2)));

#define ___panic_line(x) #x
#define __panic_line(x) ___panic_line(x)
#define panic(fmt, ...) __panic(__FILE__ ":%s:" __panic_line(__LINE__) ": " fmt, __func__ __VA_OPT__(,) __VA_ARGS__)
