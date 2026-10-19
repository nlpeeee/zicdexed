#pragma once
#include <stdio.h>
// host stand-in: warnings go to stderr, debug is dropped
#define LOGMODULE(name) static const char FromLog[] = name;
#define LOGWARN(...) do { fprintf(stderr, "[%s warn] ", FromLog); fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } while (0)
#define LOGERR(...) LOGWARN(__VA_ARGS__)
#define LOGNOTE(...) do {} while (0)
#define LOGDBG(...) do {} while (0)
