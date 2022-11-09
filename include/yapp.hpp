#pragma once

/* preprocessor definitions to handle detecting the platform */
#include <yapp/platform.hpp>

/* stdlib includes */
#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <exception>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <typeinfo>
#include <type_traits>
#include <utility>
#include <vector>

/* third party includes */
#ifdef YAPP_WIN32
#include <windows.h>
#endif

/* local includes */
#include <yapp/exception.hpp>
#include <yapp/memory.hpp>
#include <yapp/address.hpp>
#include <yapp/arch_container.hpp>
#include <yapp/headers.hpp>
#include <yapp/pe.hpp>
