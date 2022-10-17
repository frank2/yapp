#pragma once

/* preprocessor definitions to handle detecting the platform */
#include <yapp/platform.hpp>

/* stdlib includes */
#include <cstdint>
#include <cstddef>

/* third party includes */
#ifdef YAPP_WIN32
#include <windows.h>
#endif

/* local includes */
#include <yapp/raw.hpp>
