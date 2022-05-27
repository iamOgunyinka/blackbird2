#ifndef GETPID_H
#define GETPID_H

#if defined(_MSC_VER)
#include <process.h>
static DWORD cgetpid() { return GetCurrentProcessId(); }
#else
#include <unistd.h>
using cgetpid = getpid;
#endif

#endif
