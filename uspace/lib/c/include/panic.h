#pragma once

#include <stdnoreturn.h>

noreturn void panic(const char *fmt, ...) __attribute__((format (printf, 1, 2)));
