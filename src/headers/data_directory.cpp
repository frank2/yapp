#include <yapp.hpp>

using namespace yapp;
using namespace yapp::headers;

bool
DataDirectory::has_directory
(const PE &pe, std::size_t directory) const
{
   if (directory >= this->size()) { return false; }

   RVA addr = this->get(directory).VirtualAddress;
   return pe.validate_address(addr);
}
