#include <yapp.hpp>

using namespace yapp;
using namespace yapp::headers;

bool
SectionHeader::is_aligned_to_file
(const PE &pe) const
{
   return pe.is_aligned_to_file((*this)->PointerToRawData);
}

bool
SectionHeader::is_aligned_to_section
(const PE &pe) const
{
   return pe.is_aligned_to_section((*this)->VirtualAddress);
}

std::size_t
SectionHeader::memory_address
(const PE &pe) const
{
   if (pe.image_type() == PE::ImageType::DISK)
      return pe.memory_address(Offset((*this)->PointerToRawData));
   else
      return pe.memory_address(RVA((*this)->VirtualAddress));
}

std::size_t
SectionHeader::section_size
(const PE &pe) const
{
   if (pe.image_type() == PE::ImageType::DISK)
      return (*this)->SizeOfRawData;
   else
      return (*this)->Misc.VirtualSize;
}

Memory<std::uint8_t>
SectionHeader::section_data
(PE &pe)
{
   return pe.subsection(this->memory_address(pe), this->section_size(pe));
}

const Memory<std::uint8_t>
SectionHeader::section_data
(const PE &pe) const
{
   return pe.subsection(this->memory_address(pe), this->section_size(pe));
}
