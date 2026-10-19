#pragma once
#define PACKED __attribute__ ((packed))
#define ALIGN(n) __attribute__ ((aligned (n)))
#define NORETURN __attribute__ ((noreturn))
#define UNUSED __attribute__ ((unused))
