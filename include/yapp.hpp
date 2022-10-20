#pragma once

/* preprocessor definitions to handle detecting the platform */
#include <yapp/platform.hpp>

/* stdlib includes */
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <exception>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>

/* third party includes */
#ifdef YAPP_WIN32
#include <windows.h>
#endif

/* local includes */
#include <yapp/exception.hpp>
#include <yapp/raw.hpp>
#include <yapp/slice.hpp>
