#pragma once

#include <any>
#include <type_traits>

#include <yapp/address.hpp>
#include <yapp/memory.hpp>
#include <yapp/headers/raw.hpp>
#include <yapp/headers/data_directory.hpp>

namespace yapp
{
namespace headers
{
   /// @brief The base class that handles both 32-bit and 64-bit optional headers.
   ///
   /// **Note**: the template only accepts raw::IMAGE_OPTIONAL_HEADER32 or raw::IMAGE_OPTIONAL_HEADER64.
   /// Anything else is a compile-time error. Additionally, this class is annoying to deal with.
   /// You probably want OptionalHeader, OptionalHeader32 or OptionalHeader64.
   ///
   template <typename T>
   class OptionalHeaderBase : public Memory<T>
   {
      static_assert(std::is_same<T, raw::IMAGE_OPTIONAL_HEADER32>::value || std::is_same<T, raw::IMAGE_OPTIONAL_HEADER64>::value,
                    "Template class of OptionalHeaderBase object is not raw::IMAGE_OPTIONAL_HEADER32 or raw::IMAGE_OPTIONAL_HEADER64.");

   public:
      OptionalHeaderBase() : Memory(true) {}
      OptionalHeaderBase(T* pointer) : Memory(pointer) {}
      OptionalHeaderBase(const T* pointer) : Memory(pointer) {}

      void set_defaults() {
         if constexpr (std::is_same<T, raw::IMAGE_OPTIONAL_HEADER32>::value)
         {
            (*this)->Magic = IMAGE_NT_OPTIONAL_HDR32_MAGIC;
            (*this)->MajorLinkerVersion = 0xE;
            (*this)->MinorLinkerVersion = 0;
            (*this)->SizeOfCode = 0;
            (*this)->SizeOfInitializedData = 0;
            (*this)->SizeOfUninitializedData = 0;
            (*this)->AddressOfEntryPoint = 0x1000;
            (*this)->BaseOfCode = 0x1000;
            (*this)->BaseOfData = 0;
            (*this)->ImageBase = 0x400000;
            (*this)->SectionAlignment = 0x1000;
            (*this)->FileAlignment = 0x400;
            (*this)->MajorOperatingSystemVersion = 4;
            (*this)->MinorOperatingSystemVersion = 0;
            (*this)->MajorImageVersion = 4;
            (*this)->MinorImageVersion = 0;
            (*this)->MajorSubsystemVersion = 4;
            (*this)->MinorSubsystemVersion = 0;
            (*this)->Win32VersionValue = 0;
            (*this)->SizeOfImage = 0;
            (*this)->SizeOfHeaders = 0;
            (*this)->CheckSum = 0;
            (*this)->Subsystem = raw::IMAGE_SUBSYSTEM_WINDOWS_GUI;
            (*this)->DllCharacteristics = IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE
               | IMAGE_DLLCHARACTERISTICS_NX_COMPAT
               | IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE;
            (*this)->SizeOfStackReserve = 0x40000;
            (*this)->SizeOfStackCommit = 0x2000;
            (*this)->SizeOfHeapReserve = 0x100000;
            (*this)->SizeOfHeapCommit = 0x1000;
            (*this)->LoaderFlags = 0;
            (*this)->NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
         }
         else
         {
            (*this)->Magic = IMAGE_NT_OPTIONAL_HDR64_MAGIC;
            (*this)->MajorLinkerVersion = 0xE;
            (*this)->MinorLinkerVersion = 0;
            (*this)->SizeOfCode = 0;
            (*this)->SizeOfInitializedData = 0;
            (*this)->SizeOfUninitializedData = 0;
            (*this)->AddressOfEntryPoint = 0x1000;
            (*this)->BaseOfCode = 0x1000;
            (*this)->ImageBase = 0x140000000ULL;
            (*this)->SectionAlignment = 0x1000;
            (*this)->FileAlignment = 0x400;
            (*this)->MajorOperatingSystemVersion = 6;
            (*this)->MinorOperatingSystemVersion = 0;
            (*this)->MajorImageVersion = 6;
            (*this)->MinorImageVersion = 0;
            (*this)->MajorSubsystemVersion = 6;
            (*this)->MinorSubsystemVersion = 0;
            (*this)->Win32VersionValue = 0;
            (*this)->SizeOfImage = 0;
            (*this)->SizeOfHeaders = 0;
            (*this)->CheckSum = 0;
            (*this)->Subsystem = raw::IMAGE_SUBSYSTEM_WINDOWS_GUI;
            (*this)->DllCharacteristics = IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE
               | IMAGE_DLLCHARACTERISTICS_NX_COMPAT
               | IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE;
            (*this)->SizeOfStackReserve = 0x100000;
            (*this)->SizeOfStackCommit = 0x1000;
            (*this)->SizeOfHeapReserve = 0x100000;
            (*this)->SizeOfHeapCommit = 0x1000;
            (*this)->LoaderFlags = 0;
            (*this)->NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
         }
      }

      bool validate() const {
         if constexpr (std::is_same<T, raw::IMAGE_OPTIONAL_HEADER32>::value)
            return (*this)->Magic == raw::IMAGE_NT_OPTIONAL_HDR32_MAGIC;
         else
            return (*this)->Magic == raw::IMAGE_NT_OPTIONAL_HDR64_MAGIC;
      }
      
      void throw_invalid() const {
         if constexpr (std::is_same<T, raw::IMAGE_OPTIONAL_HEADER32>::value)
         {              
            if ((*this)->Magic != raw::IMAGE_NT_OPTIONAL_HDR32_MAGIC)
               throw UnexpectedOptionalMagicException((*this)->Magic, raw::IMAGE_NT_OPTIONAL_HDR32_MAGIC);
         }
         else
         {
            if ((*this)->Magic != raw::IMAGE_NT_OPTIONAL_HDR64_MAGIC)
               throw UnexpectedOptionalMagicException((*this)->Magic, raw::IMAGE_NT_OPTIONAL_HDR64_MAGIC);
         }
      }

      std::size_t data_directory_size() const {
         auto sizes = (*this)->NumberOfRvaAndSizes;

         if (sizes > 16)
            return 16;
         else
            return sizes;
      }

      DataDirectory data_directory() {
         return DataDirectory(&(*this)->DataDirectory[0], this->data_directory_size());
      }

      const DataDirectory data_directory() const {
         return DataDirectory(&(*this)->DataDirectory[0], this->data_directory_size());
      }
   };

   using OptionalHeader32 = OptionalHeaderBase<raw::IMAGE_OPTIONAL_HEADER32>;
   using OptionalHeader64 = OptionalHeaderBase<raw::IMAGE_OPTIONAL_HEADER64>;

   class OptionalHeader
   {
   protected:
      std::any header;

   public:
      OptionalHeader(OptionalHeader32 header) : header(std::make_any<OptionalHeader32>(header)) {}
      OptionalHeader(OptionalHeader64 header) : header(std::make_any<OptionalHeader64>(header)) {}
      OptionalHeader(const OptionalHeader &other) : header(other.header) {}

      bool is_32() const { return this->header.type() == typeid(OptionalHeader32); }
      bool is_64() const { return this->header.type() == typeid(OptionalHeader64); }
      OptionalHeader32 &get_32() { return *std::any_cast<OptionalHeader32>(&this->header); }
      OptionalHeader64 &get_64() { return *std::any_cast<OptionalHeader64>(&this->header); }
      const OptionalHeader32 &get_32() const { return *std::any_cast<OptionalHeader32>(&this->header); }
      const OptionalHeader64 &get_64() const { return *std::any_cast<OptionalHeader64>(&this->header); }

      bool validate() const {
         if (this->is_32()) { return this->get_32().validate(); }
         else { return this->get_64().validate(); }
      }

      void throw_invalid() const {
         if (this->is_32()) { return this->get_32().throw_invalid(); }
         else { return this->get_64().throw_invalid(); }
      }

      std::size_t data_directory_size() const {
         if (this->is_32()) { return this->get_32().data_directory_size(); }
         else { return this->get_64().data_directory_size(); }
      }

      DataDirectory data_directory() {
         if (this->is_32()) { return this->get_32().data_directory(); }
         else { return this->get_64().data_directory(); }
      }
      
      const DataDirectory data_directory() const {
         if (this->is_32()) { return this->get_32().data_directory(); }
         else { return this->get_64().data_directory(); }
      }
   };
}}
