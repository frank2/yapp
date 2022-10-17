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
