#pragma once

#include <_bits/__noreturn.h>

__noreturn void panic(const char *fmt, ...) __attribute__((__format__ (__printf__, 1, 2)));
