#pragma once

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN

#include <ctime>

int cgettimeofday(struct timeval* tp, struct timezone* tzp)
{
  // Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
  // This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
  // until 00:00:00 January 1, 1970
  static const uint64_t EPOCH = ((uint64_t)116'444'736'000'000'000ULL);

  SYSTEMTIME  system_time;
  FILETIME    file_time;
  uint64_t    time;

  GetSystemTime(&system_time);
  SystemTimeToFileTime(&system_time, &file_time);
  time = ((uint64_t)file_time.dwLowDateTime);
  time += ((uint64_t)file_time.dwHighDateTime) << 32;

  tp->tv_sec = (time_t)((time - EPOCH) / 10'000'000L);
  tp->tv_usec = (time_t)(system_time.wMilliseconds * time_t(1'000));
  return 0;
}

#else

#include <sys/time.h>

using cgettimeofday = gettimeofday;
#endif // _WIN32