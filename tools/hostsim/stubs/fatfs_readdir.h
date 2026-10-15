// FatFs-style directory iteration for the host: readdir() without "." and ".."
// (MiniDexed's loader recurses into every non-.syx entry, which loops on a PC filesystem).
#pragma once
#include <dirent.h>
#include <string.h>
static inline struct dirent* zic_readdir(DIR* d)
{
	struct dirent* e;
	while ((e = readdir(d)) && (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))) {}
	return e;
}
#define readdir zic_readdir
