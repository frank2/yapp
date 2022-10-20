//! @mainpage Yet Another PE Parser!
//!
//! Welcome to **YAPP**! This project is the product of multiple successful attempts at parsing
//! PE executables. It is intended to make the process of accessing PE headers not only easier
//! on Windows, but accessible on non-Windows platforms!
//!

#include <yapp.hpp>

#if defined(YAPP_WIN32) && defined(YAPP_SHARED)
BOOL WINAPI
DllMain
(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
   // do nothing for now
   return TRUE;
}
#endif
