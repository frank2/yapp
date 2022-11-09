#pragma once

#include <any>
#include <map>

#include <yapp/arch_container.hpp>
#include <yapp/headers/data_directory.hpp>

namespace yapp
{
   class PE;
   
namespace headers
{
   template <typename T, typename U>
   class ExportThunkBase
   {
      static_assert(std::is_same<T, std::uint32_t>::value || std::is_same<T, std::uint64_t>::value,
                    "Template class for export thunk base must be std::uint32_t or std::uint64_t");
      static_assert(sizeof(U) == sizeof(T) / 2,
                    "ExportThunk base second type must be half the size of the original type.");
         
   public:
      using BaseType = typename T;
      using OrdinalType = typename U;
         
      BaseType value;

      ExportThunkBase() : value(0) {}
      ExportThunkBase(BaseType value) : value(value) {}
      ExportThunkBase(const ExportThunkBase& other) : value(other.value) {}

      RVA as_rva() { return this->value; }
      bool is_ordinal() const {
         if constexpr(std::is_same<BaseType, std::uint32_t>::value) { return (this->value & 0x80000000) != 0; }
         else { return (this->value & 0x8000000000000000ULL) != 0; }
      }
      OrdinalType ordinal() const {
         if constexpr (std::is_same<OrdinalType, std::uint16_t>::value) { return OrdinalType(this->value & 0xFFFF); }
         else { return OrdinalType(this->value & 0xFFFFFFFF); }
      }
      bool is_forwarder_string(const PE &pe) const;
      bool is_function(const PE &pe) const;
      Memory<char> forwarder_string(PE &pe) const;
      const Memory<char> forwarder_string(const PE &pe) const;
      void *function(PE &pe) const;
      const void *function(const PE &pe) const;
      std::any evaluate(PE &pe) const;
      const std::any evaluate(const PE &pe) const;
   };

   using ExportThunk32 = ExportThunkBase<std::uint32_t, std::uint16_t>;
   using ExportThunk64 = ExportThunkBase<std::uint64_t, std::uint32_t>;

   template <typename ExportThunkType>
   class ExportDirectoryBase : public Memory<raw::IMAGE_EXPORT_DIRECTORY>
   {
      static_assert(std::is_same<ExportThunk32, ExportThunkType>::value || std::is_same<ExportThunk64, ExportThunkType>::value,
                    "Export directory template argument must be ExportThunk32 or ExportThunk64.");
   public:
      ExportDirectoryBase() : Memory() {}
      ExportDirectoryBase(Memory::BaseType *pointer) : Memory(pointer) {}
      ExportDirectoryBase(const Memory::BaseType *pointer) : Memory(pointer) {}

      static const std::size_t DirectoryIndex = raw::IMAGE_DIRECTORY_ENTRY_EXPORT;

      Memory<char> name(PE &pe) const;
      const Memory<char> name(const PE &pe) const;
      
      Memory<ExportThunkType> functions(PE &pe) const;
      const Memory<ExportThunkType> functions(const PE &pe) const;

      Memory<RVA> names(PE &pe) const;
      const Memory<RVA> names(const PE &pe) const;

      Memory<typename ExportThunkType::OrdinalType> name_ordinals(PE &pe) const;
      const Memory<typename ExportThunkType::OrdinalType> name_ordinals(const PE &pe) const;

      std::map<std::string, ExportThunkType> export_map(const PE &pe) const;
   };

   using ExportDirectory32 = ExportDirectoryBase<ExportThunk32>;
   using ExportDirectory64 = ExportDirectoryBase<ExportThunk64>;

   class ExportDirectory : public ArchContainer<ExportDirectory32, ExportDirectory64>
   {
   public:
      ExportDirectory(ArchContainer::Type32 t32) : ArchContainer(t32) {}
      ExportDirectory(ArchContainer::Type64 t64) : ArchContainer(t64) {}
      ExportDirectory(const ExportDirectory &other) : ArchContainer(other) {}

      static const std::size_t DirectoryIndex = ExportDirectory32::DirectoryIndex;
   };
}}

#include "../src/headers/directories/export.tpp"
