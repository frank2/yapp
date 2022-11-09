#pragma once

#include <yapp/memory.hpp>
#include <yapp/headers/raw.hpp>

namespace yapp
{
   class PE;
   
namespace headers
{
   class SectionHeader : public Memory<raw::IMAGE_SECTION_HEADER>
   {
   public:
      SectionHeader() : Memory() {}
      SectionHeader(Memory::BaseType *pointer) : Memory(pointer) {}
      SectionHeader(const Memory::BaseType *pointer) : Memory(pointer) {}

      std::size_t name_size() const {
         std::size_t size = 8;

         while (size != 0 && (*this)->Name[size-1] == 0)
            --size;

         return size;
      }

      bool name_is_string() const {
         for (std::size_t i=0; i<this->name_size(); ++i)
         {
            auto byte = (*this)->Name[i];
            
            if (byte < 0x20 || byte >= 0x7F)
               return false;
         }

         return true;
      }

      std::string name_string() const {
         return std::string(reinterpret_cast<const char *>(&(*this)->Name[0]), this->name_size());
      }

      std::vector<std::uint8_t> name_bytes() const {
         return std::vector<std::uint8_t>(&(*this)->Name[0], &(*this)->Name[8]);
      }

      bool has_offset(Offset offset) const {
         return *offset >= (*this)->PointerToRawData && *offset < ((*this)->PointerToRawData + (*this)->SizeOfRawData);
      }

      bool has_rva(RVA rva) const {
         return *rva >= (*this)->VirtualAddress && *rva < ((*this)->VirtualAddress + (*this)->Misc.VirtualSize);
      }

      bool is_aligned_to_file(const PE &) const;
      bool is_aligned_to_section(const PE &) const;
      std::size_t memory_address(const PE &) const;
      std::size_t section_size(const PE &) const;
      Memory<std::uint8_t> section_data(PE &);
      const Memory<std::uint8_t> section_data(const PE &) const;
   };

   class SectionTable : public Memory<raw::IMAGE_SECTION_HEADER>
   {
   public:
      SectionTable() : Memory() {}
      SectionTable(Memory::BaseType *pointer, std::size_t size) : Memory(pointer, size) {}
      SectionTable(const Memory::BaseType *pointer, std::size_t size) : Memory(pointer, size) {}

      SectionHeader operator[](std::size_t index) { return this->get_wrapped(index); }
      const SectionHeader operator[](std::size_t index) const { return this->get_wrapped(index); }

      SectionHeader get_wrapped(std::size_t index) { return &this->get(index); }
      const SectionHeader get_wrapped(std::size_t index) const { return &this->get(index); }

      bool has_offset(Offset offset) const {
         for (auto iter=this->cbegin();
              iter!=this->cend();
              ++iter)
         {
            SectionHeader header = &*iter;
            if (header.has_offset(offset)) { return true; }
         }

         return false;
      }
      
      bool has_rva(RVA rva) const {
         for (auto iter=this->cbegin();
              iter!=this->cend();
              ++iter)
         {
            SectionHeader header = &*iter;
            if (header.has_rva(rva)) { return true; }
         }

         return false;
      }
      
      SectionHeader section_by_offset(Offset offset) {
         for (auto iter=this->begin();
              iter!=this->end();
              ++iter)
         {
            SectionHeader section = &*iter;
            if (section.has_offset(offset)) { return section; }
         }

         throw SectionNotFoundException();
      }

      const SectionHeader section_by_offset(Offset offset) const {
         for (auto iter=this->cbegin();
              iter!=this->cend();
              ++iter)
         {
            const SectionHeader section = &*iter;
            if (section.has_offset(offset)) { return section; }
         }

         throw SectionNotFoundException();
      }

      SectionHeader section_by_rva(RVA rva) {
         for (auto iter=this->begin();
              iter!=this->end();
              ++iter)
         {
            SectionHeader section = &*iter;
            if (section.has_rva(rva)) { return section; }
         }

         throw SectionNotFoundException();
      }

      const SectionHeader section_by_rva(RVA rva) const {
         for (auto iter=this->cbegin();
              iter!=this->cend();
              ++iter)
         {
            const SectionHeader section = &*iter;
            if (section.has_rva(rva)) { return section; }
         }

         throw SectionNotFoundException();
      }

      SectionHeader section_by_name(const std::uint8_t *name, std::size_t size) {
         auto min_cmp = std::min<std::size_t>(size, 8);

         for (auto iter=this->begin();
              iter!=this->end();
              ++iter)
         {
            SectionHeader section = &*iter;

            if (min_cmp != section.name_size())
               continue;

            if (std::memcmp(&section->Name[0], name, min_cmp) == 0)
               return section;
         }

         throw SectionNotFoundException();
      }

      const SectionHeader section_by_name(const std::uint8_t *name, std::size_t size) const {
         auto min_cmp = std::min<std::size_t>(size, 8);
         
         for (auto iter=this->cbegin();
              iter!=this->cend();
              ++iter)
         {
            SectionHeader section = &*iter;

            if (min_cmp != section.name_size())
               continue;
            
            if (std::memcmp(&section->Name[0], name, min_cmp) == 0)
               return section;
         }

         throw SectionNotFoundException();
      }

      SectionHeader section_by_name(const std::string &name) {
         return this->section_by_name(reinterpret_cast<const std::uint8_t *>(name.c_str()), name.size());
      }

      const SectionHeader section_by_name(const std::string &name) const {
         return this->section_by_name(reinterpret_cast<const std::uint8_t *>(name.c_str()), name.size());
      }
   };
}}
