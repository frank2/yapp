#pragma once

#if defined(_WIN32) || defined(WIN32)
#define YAPP_WIN32
#endif

#if defined(_M_AMD64) || defined(__x86_64__)
#define YAPP_64BIT
#endif
