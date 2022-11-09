#include <yapp/address.hpp>
#include <yapp/pe.hpp>

template <typename T>
T*
yapp::Offset::as_ptr
(yapp::PE &pe) const
{
   return pe.cast_ptr<T>(this->as_memory(pe));
}

template <typename T>
const T*
yapp::Offset::as_ptr
(const yapp::PE &pe) const
{
   return pe.cast_ptr<T>(this->as_memory(pe));
}

template <typename T>
T*
yapp::RVA::as_ptr
(yapp::PE &pe) const
{
   return pe.cast_ptr<T>(this->as_memory(pe));
}

template <typename T>
const T*
yapp::RVA::as_ptr
(const yapp::PE &pe) const
{
   return pe.cast_ptr<T>(this->as_memory(pe));
}

template <typename T>
T*
yapp::VA32::as_ptr
(yapp::PE &pe) const
{
   return pe.cast_ptr<T>(this->as_memory(pe));
}

template <typename T>
const T*
yapp::VA32::as_ptr
(const yapp::PE &pe) const
{
   return pe.cast_ptr<T>(this->as_memory(pe));
}

template <typename T>
T*
yapp::VA64::as_ptr
(yapp::PE &pe) const
{
   return pe.cast_ptr<T>(this->as_memory(pe));
}

template <typename T>
const T*
yapp::VA64::as_ptr
(const yapp::PE &pe) const
{
   return pe.cast_ptr<T>(this->as_memory(pe));
}

template <typename T>
T*
yapp::VA::as_ptr
(yapp::PE &pe) const
{
   return pe.cast_ptr<T>(this->as_memory(pe));
}

template <typename T>
const T*
yapp::VA::as_ptr
(const yapp::PE &pe) const
{
   return pe.cast_ptr<T>(this->as_memory(pe));
}
