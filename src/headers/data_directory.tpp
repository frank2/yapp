#include <yapp/pe.hpp>

template <typename U, typename=int>
struct has_index : std::false_type {};

template <typename U>
struct has_index<U, decltype((void) U::DirectoryIndex, 0)> : std::true_type {};

template <class U, class V=void>
struct enable_if_type { typedef V type; };

template <class U, class V=void>
struct has_platform_type : std::false_type {};

template <class U>
struct has_platform_type<U, typename enable_if_type<typename U::TypePlatform>::type> : std::true_type {};

template <typename T>
bool
yapp::headers::DataDirectory::has_directory
(const yapp::PE &pe) const
{
   static_assert(has_index<T>::value,
                 "Template class must have a DirectoryIndex static variable.");

   return this->has_directory(pe, T::DirectoryIndex);
}

template <typename T>
T
yapp::headers::DataDirectory::directory
(yapp::PE &pe)
{
   static_assert(has_index<T>::value,
                 "Template class must have a DirectoryIndex static variable.");

   if (!this->has_directory<T>(pe))
      throw DirectoryUnavailableException(T::DirectoryIndex);
   
   yapp::RVA addr = this->get(T::DirectoryIndex).VirtualAddress;
   std::size_t size = this->get(T::DirectoryIndex).Size;

   if constexpr (has_platform_type<T>::value)
   {
      /* handle ArchContainers */
      switch (pe.arch())
      {
      case yapp::PE::Arch::I386:
      case yapp::PE::Arch::ARM:
      {
         using ArchType = T::Type32;

         if constexpr (ArchType::variadic)
         {
            this->throw_if_out_of_bounds<std::uint8_t>(addr.as_memory(*this), size);
            return T(ArchType(addr.as_ptr<ArchType::BaseType>(*this), size));
         }
         else
         {
            return T(ArchType(addr.as_ptr<ArchType::BaseType>(pe)));
         }
      }

      case yapp::PE::Arch::AMD64:
      case yapp::PE::Arch::ARM64:
      {
         using ArchType = T::Type64;

         if constexpr (ArchType::variadic)
         {
            this->throw_if_out_of_bounds<std::uint8_t>(addr.as_memory(*this), size);
            return T(ArchType(addr.as_ptr<ArchType::BaseType>(*this), size));
         }
         else
         {
            return T(ArchType(addr.as_ptr<ArchType::BaseType>(pe)));
         }
      }

      case yapp::PE::Arch::UNSUPPORTED:
      default:
      {
         throw UnsupportedArchitectureException();
      }
      }
   }
   else
   {
      if constexpr (T::variadic)
      {
         this->throw_if_out_of_bounds<std::uint8_t>(addr.as_memory(*this), size);
         return T(addr.as_ptr<T::BaseType>(*this), size);
      }
      else
      {
         return T(addr.as_ptr<T::BaseType>(pe));
      }
   }
}

template <typename T>
const T
yapp::headers::DataDirectory::directory
(const yapp::PE &pe) const
{
   static_assert(has_index<T>::value,
                 "Template class must have a DirectoryIndex static variable.");

   if (!this->has_directory<T>(pe))
      throw DirectoryUnavailableException(T::DirectoryIndex);
   
   RVA addr = this->get(T::DirectoryIndex).VirtualAddress;
   std::size_t size = this->get(T::DirectoryIndex).Size;

   if constexpr (has_platform_type<T>::value)
   {
      /* handle ArchContainers */
      switch (pe.arch())
      {
      case yapp::PE::Arch::I386:
      case yapp::PE::Arch::ARM:
      {
         using ArchType = T::Type32;

         if constexpr (ArchType::variadic)
         {
            this->throw_if_out_of_bounds<std::uint8_t>(addr.as_memory(*this), size);
            return T(ArchType(addr.as_ptr<ArchType::BaseType>(*this), size));
         }
         else
         {
            return T(ArchType(addr.as_ptr<ArchType::BaseType>(pe)));
         }
      }

      case yapp::PE::Arch::AMD64:
      case yapp::PE::Arch::ARM64:
      {
         using ArchType = T::Type64;

         if constexpr (ArchType::variadic)
         {
            this->throw_if_out_of_bounds<std::uint8_t>(addr.as_memory(*this), size);
            return T(ArchType(addr.as_ptr<ArchType::BaseType>(*this), size));
         }
         else
         {
            return T(ArchType(addr.as_ptr<ArchType::BaseType>(pe)));
         }
      }

      case yapp::PE::Arch::UNSUPPORTED:
      default:
      {
         throw UnsupportedArchitectureException();
      }
      }
   }
   else
   {
      if constexpr (T::variadic)
      {
         this->throw_if_out_of_bounds<std::uint8_t>(addr.as_memory(*this), size);
         return T(addr.as_ptr<T::BaseType>(*this), size);
      }
      else
      {
         return T(addr.as_ptr<T::BaseType>(pe));
      }
   }
}
