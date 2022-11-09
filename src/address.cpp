#include <yapp.hpp>

using namespace yapp;

RVA
Offset::as_rva
(const PE &pe) const
{
   return pe.offset_to_rva(*this);
}

VA
Offset::as_va
(const PE &pe) const
{
   return pe.offset_to_va(*this);
}

std::size_t
Offset::as_memory
(const PE &pe) const
{
   return pe.memory_address(*this);
}

Offset
RVA::as_offset
(const PE &pe) const
{
   return pe.rva_to_offset(*this);
}

VA
RVA::as_va
(const PE &pe) const
{
   return pe.rva_to_va(*this);
}

std::size_t
RVA::as_memory
(const PE &pe) const
{
   return pe.memory_address(*this);
}

Offset
VA32::as_offset
(const PE &pe) const
{
   return pe.va_to_offset(*this);
}

RVA
VA32::as_rva
(const PE &pe) const
{
   return pe.va_to_rva(*this);
}

std::size_t
VA32::as_memory
(const PE &pe) const
{
   return pe.memory_address(*this);
}

Offset
VA64::as_offset
(const PE &pe) const
{
   return pe.va_to_offset(*this);
}

RVA
VA64::as_rva
(const PE &pe) const
{
   return pe.va_to_rva(*this);
}

std::size_t
VA64::as_memory
(const PE &pe) const
{
   return pe.memory_address(*this);
}

Offset
VA::as_offset
(const PE &pe) const
{
   return pe.va_to_offset(*this);
}

RVA
VA::as_rva
(const PE &pe) const
{
   return pe.va_to_rva(*this);
}

std::size_t
VA::as_memory
(const PE &pe) const
{
   return pe.memory_address(*this);
}
