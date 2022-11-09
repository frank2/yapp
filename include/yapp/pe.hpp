#pragma once

#include <algorithm>
#include <cstring>
#include <string>

#include <yapp/memory.hpp>
#include <yapp/headers.hpp>
#include <yapp/address.hpp>

namespace yapp
{
   class PE : public Memory<std::uint8_t>
   {
   public:
      enum Arch
      {
         I386 = headers::raw::IMAGE_FILE_MACHINE_I386,
         AMD64 = headers::raw::IMAGE_FILE_MACHINE_AMD64,
         ARM = headers::raw::IMAGE_FILE_MACHINE_ARM,
         ARM64 = headers::raw::IMAGE_FILE_MACHINE_ARM64,
         UNSUPPORTED = headers::raw::IMAGE_FILE_MACHINE_UNKNOWN,
      };

      enum ImageType
      {
         DISK = 0,
         MEMORY = 1,
         VIRTUAL = 2,
      };

   protected:
      ImageType _image_type;
      
   public:
      PE() : _image_type(ImageType::DISK), Memory() {}
      PE(std::string &filename, ImageType _image_type=ImageType::DISK) : _image_type(_image_type), Memory(filename) {}
      PE(const Memory &memory, ImageType _image_type=ImageType::DISK) : _image_type(_image_type), Memory(memory) {}
      /* this constructor is intended for yanking PE images out of memory
      PE(void *image_base) : _image_type(ImageType::VIRTUAL), Memory() {
         this->parse_virtual(image_base);
      }
      */

      inline ImageType image_type() const { return this->_image_type; }

      headers::DOSHeader dos_header() {
         return this->cast_ptr<headers::DOSHeader::BaseType>(0);
      }

      const headers::DOSHeader dos_header() const {
         return this->cast_ptr<headers::DOSHeader::BaseType>(0);
      }

      headers::DOSHeader valid_dos_header() {
         auto dos_header = this->dos_header();
         dos_header.throw_invalid();
         return dos_header;
      }

      const headers::DOSHeader valid_dos_header() const {
         const auto dos_header = this->dos_header();
         dos_header.throw_invalid();
         return dos_header;
      }

      Offset e_lfanew() const {
         auto header = this->valid_dos_header();

         return header->e_lfanew;
      }

      Memory<std::uint8_t> dos_stub() {
         auto e_lfanew = this->e_lfanew();
         auto dos_end = sizeof(headers::DOSHeader::BaseType);

         if (*e_lfanew < dos_end) { return this->subsection(dos_end, 0); }

         return this->subsection(dos_end, std::size_t(*e_lfanew) - dos_end);
      }

      headers::NTHeaders32 nt_headers_32() {
         return this->cast_ptr<headers::NTHeaders32::BaseType>(*this->e_lfanew());
      }

      headers::NTHeaders64 nt_headers_64() {
         return this->cast_ptr<headers::NTHeaders64::BaseType>(*this->e_lfanew());
      }

      const headers::NTHeaders32 nt_headers_32() const {
         return this->cast_ptr<headers::NTHeaders32::BaseType>(*this->e_lfanew());
      }

      const headers::NTHeaders64 nt_headers_64() const {
         return this->cast_ptr<headers::NTHeaders64::BaseType>(*this->e_lfanew());
      }

      std::uint16_t machine() const {
         const auto nt_headers = this->nt_headers_32();
         return nt_headers->FileHeader.Machine;
      }

      Arch arch() const {
         auto machine = this->machine();

         switch (machine)
         {
         case Arch::I386: { return Arch::I386; }
         case Arch::AMD64: { return Arch::AMD64; }
         case Arch::ARM: { return Arch::ARM; }
         case Arch::ARM64: { return Arch::ARM64; }
         default: { return Arch::UNSUPPORTED; }
         }
      }

      std::uint16_t nt_magic() const {
         auto headers = this->nt_headers_32();
         return headers->OptionalHeader.Magic;
      }

      headers::NTHeaders valid_nt_headers() {
         auto magic = this->nt_magic();

         if (magic == headers::raw::IMAGE_NT_OPTIONAL_HDR32_MAGIC)
         {
            auto headers = this->nt_headers_32();
            headers.throw_invalid();
            return headers;
         }
         else if (magic == headers::raw::IMAGE_NT_OPTIONAL_HDR64_MAGIC)
         {
            auto headers = this->nt_headers_64();
            headers.throw_invalid();
            return headers;
         }
         
         throw UnexpectedOptionalMagicException(magic);
      }

      const headers::NTHeaders valid_nt_headers() const {
         auto magic = this->nt_magic();

         if (magic == headers::raw::IMAGE_NT_OPTIONAL_HDR32_MAGIC)
         {
            auto headers = this->nt_headers_32();
            headers.throw_invalid();
            return headers;
         }
         else if (magic == headers::raw::IMAGE_NT_OPTIONAL_HDR64_MAGIC)
         {
            auto headers = this->nt_headers_64();
            headers.throw_invalid();
            return headers;
         }
         
         throw UnexpectedOptionalMagicException(magic);
      }

      bool validate_checksum() const {
         const auto nt_headers = this->valid_nt_headers();
         std::uint32_t file_checksum;

         if (nt_headers.is_32())
            file_checksum = nt_headers.get_32()->OptionalHeader.CheckSum;
         else
            file_checksum = nt_headers.get_64()->OptionalHeader.CheckSum;

         auto calc_checksum = this->calculate_checksum();
         return file_checksum == calc_checksum;
      }

      std::uint32_t calculate_checksum() const {
         const auto nt_headers = this->valid_nt_headers();
         std::uintptr_t checksum_addr;

         if (nt_headers.is_32())
            checksum_addr = reinterpret_cast<std::uintptr_t>(&nt_headers.get_32()->OptionalHeader.CheckSum);
         else
            checksum_addr = reinterpret_cast<std::uintptr_t>(&nt_headers.get_64()->OptionalHeader.CheckSum);

         std::uintptr_t base_addr = reinterpret_cast<std::uintptr_t>(this->ptr());
         auto checksum_offset = checksum_addr - base_addr;
         auto eof = this->size();
         std::uint64_t checksum = 0;

         for (std::size_t offset=0; offset<eof; offset += 4)
         {
            if (offset == checksum_offset) { continue; }
            
            std::vector<std::uint8_t> data;
            auto left = eof-offset;

            if (left >= 4) { data = this->read(offset, 4); }
            else {
               auto padding = 4 - left;
               data = this->read(offset, left);
               
               for (std::size_t i=0; i<padding; ++i)
                  data.push_back(0);
            }

            auto value = *reinterpret_cast<std::uint32_t *>(data.data());
            checksum = (checksum & 0xFFFFFFFF) + value + (checksum >> 32);

            if (checksum > 0xFFFFFFFF)
               checksum = (checksum & 0xFFFFFFFF) + (checksum >> 32);
         }

         checksum = (checksum & 0xFFFF) + (checksum >> 16);
         checksum = checksum + (checksum >> 16);
         checksum = checksum & 0xFFFF;
         checksum += eof;

         return (checksum & 0xFFFFFFFF);
      }

      RVA entrypoint() const
      {
         const auto nt_headers = this->valid_nt_headers();

         if (nt_headers.is_32())
            return nt_headers.get_32().optional_header()->AddressOfEntryPoint;
         else
            return nt_headers.get_64().optional_header()->AddressOfEntryPoint;
      }

      headers::DataDirectory data_directory()
      {
         auto nt_headers = this->valid_nt_headers();

         if (nt_headers.is_32())
            return nt_headers.get_32().optional_header().data_directory();
         else
            return nt_headers.get_64().optional_header().data_directory();
      }

      const headers::DataDirectory data_directory() const
      {
         const auto nt_headers = this->valid_nt_headers();

         if (nt_headers.is_32())
            return nt_headers.get_32().optional_header().data_directory();
         else
            return nt_headers.get_64().optional_header().data_directory();
      }

      std::uint64_t image_base() const {
         if (this->_image_type == ImageType::VIRTUAL)
            return reinterpret_cast<std::uint64_t>(this->ptr());
         else
         {
            auto nt_headers = this->valid_nt_headers();

            if (nt_headers.is_32())
               return nt_headers.get_32().optional_header()->ImageBase;
            else
               return nt_headers.get_64().optional_header()->ImageBase;
         }
      }

      Offset section_table_offset() const {
         auto offset = this->e_lfanew();
         auto nt_headers = this->valid_nt_headers();
         std::uint32_t size_of_optional = nt_headers.file_header()->SizeOfOptionalHeader;

         offset += static_cast<std::uint32_t>(sizeof(std::uint32_t)); // NT signature
         offset += static_cast<std::uint32_t>(sizeof(headers::FileHeader::BaseType));
         offset += size_of_optional;

         return offset;
      }

      headers::SectionTable section_table() {
         auto offset = this->section_table_offset();
         auto nt_headers = this->valid_nt_headers();
         auto number_of_sections = nt_headers.file_header()->NumberOfSections;

         // throw an exception if this goes out of range
         this->throw_if_out_of_bounds<headers::SectionTable::BaseType>(offset, number_of_sections);
         return headers::SectionTable(this->cast_ptr<headers::SectionTable::BaseType>(*offset), number_of_sections);
      }

      const headers::SectionTable section_table() const {
         auto offset = this->section_table_offset();
         auto nt_headers = this->valid_nt_headers();
         auto number_of_sections = nt_headers.file_header()->NumberOfSections;

         // throw an exception if this goes out of range
         this->throw_if_out_of_bounds<headers::SectionTable::BaseType>(offset, number_of_sections);
         return headers::SectionTable(this->cast_ptr<headers::SectionTable::BaseType>(*offset), number_of_sections);
      }

      headers::SectionHeader add_section(headers::SectionHeader section) {
         auto nt_headers = this->valid_nt_headers();

         if (nt_headers.file_header()->NumberOfSections == 0xFFFF)
            throw SectionTableOverflowException();

         nt_headers.file_header()->NumberOfSections++;

         auto section_table = this->section_table();
         section_table[section_table.size()-1].write(0, section.ptr());

         return section_table[section_table.size()-1];
      }

      // headers::SectionHeader append_section(headers::SectionHeader section) {
            
      bool validate_address(Offset offset) const {
         return *offset < this->size();
      }

      bool validate_address(RVA rva) const {
         try {
            auto nt_headers = this->valid_nt_headers();
            std::size_t image_size;

            if (nt_headers.is_32()) { image_size = nt_headers.get_32().optional_header()->SizeOfImage; }
            else { image_size = nt_headers.get_64().optional_header()->SizeOfImage; }

            return *rva < image_size;
         }
         catch (Exception &)
         {
            return false;
         }
      }

      bool validate_address(VA va) const {
         try {
            auto nt_headers = this->valid_nt_headers();
            auto image_base = this->image_base();
            std::uint64_t image_size;

            if (nt_headers.is_32()) { image_size = nt_headers.get_32().optional_header()->SizeOfImage; }
            else { image_size = nt_headers.get_32().optional_header()->SizeOfImage; }

            auto start = image_base;
            auto end = start + image_size;

            return start <= *va && *va < end;
         }
         catch (Exception &)
         {
            return false;
         }
      }

      bool is_aligned_to_file(Offset offset) const {
         try {
            auto nt_headers = this->valid_nt_headers();
            std::size_t alignment;

            if (nt_headers.is_32()) { alignment = nt_headers.get_32().optional_header()->FileAlignment; }
            else { alignment = nt_headers.get_64().optional_header()->FileAlignment; }

            return *offset % alignment == 0;
         }
         catch (Exception &) {
            return false;
         }
      }

      bool is_aligned_to_section(RVA rva) const {
         try {
            auto nt_headers = this->valid_nt_headers();
            std::size_t alignment;

            if (nt_headers.is_32()) { alignment = nt_headers.get_32().optional_header()->SectionAlignment; }
            else { alignment = nt_headers.get_64().optional_header()->SectionAlignment; }

            return *rva % alignment == 0;
         }
         catch (Exception &)
         {
            return false;
         }
      }

      template <typename T>
      T align_to_file(T value) const {
         auto nt_headers = this->valid_nt_headers();
         std::uint32_t alignment;

         if (nt_headers.is_32()) { alignment = nt_headers.get_32().optional_header()->FileAlignment; }
         else { alignment = nt_headers.get_64().optional_header()->FileAlignment; }

         return align<T>(value, alignment);
      }

      Offset align_to_file(Offset offset) const {
         return this->align_to_file<Offset>(offset);
      }

      template <typename T>
      T align_to_section(T value) const {
         auto nt_headers = this->valid_nt_headers();
         std::uint32_t alignment;

         if (nt_headers.is_32()) { alignment = nt_headers.get_32().optional_header()->SectionAlignment; }
         else { alignment = nt_headers.get_64().optional_header()->SectionAlignment; }

         return align<T>(value, alignment);
      }

      RVA align_to_section(RVA rva) const {
         return this->align_to_section<RVA>(rva);
      }

      RVA offset_to_rva(Offset offset) const {
         if (!this->validate_address(offset)) { throw InvalidOffsetException(offset); }

         auto section_table = this->section_table();

         // the offset wasn't found in the section table, it's an out-of-bounds offset
         // (e.g., somewhere in a DOS/NT header)
         if (!section_table.has_offset(offset))
         {
            if (!validate_address(RVA(*offset)))
               throw InvalidRVAException(RVA(*offset));

            return RVA(*offset);
         }

         auto section = section_table.section_by_offset(offset);
         auto rva = RVA(*offset);
         rva -= section->PointerToRawData;
         rva += section->VirtualAddress;

         if (!this->validate_address(rva) || !section.has_rva(rva))
            throw InvalidRVAException(rva);

         return rva;
      }

      VA offset_to_va(Offset offset) const {
         if (!this->validate_address(offset))
            throw InvalidOffsetException(offset);

         RVA rva = this->offset_to_rva(offset);
         return this->rva_to_va(rva);
      }

      Offset rva_to_offset(RVA rva) const {
         if (!this->validate_address(rva))
         {
            throw InvalidRVAException(rva);
         }

         auto section_table = this->section_table();

         if (!section_table.has_rva(rva))
         {
            if (!this->validate_address(Offset(*rva)))
               throw InvalidOffsetException(Offset(*rva));

            return Offset(*rva);
         }

         auto section = section_table.section_by_rva(rva);
         Offset offset = *rva;
         offset -= section->VirtualAddress;
         offset += section->PointerToRawData;

         if (!this->validate_address(offset) || !section.has_offset(offset))
         {
            throw InvalidOffsetException(offset);
         }

         return offset;
      }

      VA rva_to_va(RVA rva) const {
         if (!this->validate_address(rva))
            throw InvalidRVAException(rva);

         auto image_base = this->image_base();
         auto arch = this->arch();

         if (arch == Arch::UNSUPPORTED)
            throw UnsupportedArchitectureException();

         VA va;

         if (arch == Arch::I386 || arch == Arch::ARM)
            va = VA32(*rva + static_cast<std::uint32_t>(image_base));
         else
            va = VA64(*rva + image_base);

         if (!this->validate_address(va))
            throw InvalidVAException(va);

         return va;
      }

      RVA va_to_rva(VA va) const {
         if (!validate_address(va))
            throw InvalidVAException(va);

         auto image_base = this->image_base();
         RVA rva = static_cast<std::uint32_t>(*va - image_base);;

         if (!this->validate_address(rva))
            throw InvalidRVAException(rva);

         return rva;
      }

      Offset va_to_offset(VA va) const {
         auto rva = this->va_to_rva(va);
         return this->rva_to_offset(rva);
      }

      std::size_t memory_address(Offset offset) const {
         if (this->_image_type == ImageType::DISK && this->validate_address(offset))
            return *offset;
         else
            return *offset.as_rva(*this);
      }

      std::size_t memory_address(RVA rva) const {
         if (this->_image_type == ImageType::DISK)
         {
            return *rva.as_offset(*this);
         }
         else if (this->validate_address(rva))
            return *rva;
         else
            throw InvalidRVAException(rva);
      }

      std::size_t memory_address(VA va) const {
         return this->memory_address(va.as_rva(*this));
      }

      Memory<char> cstring_at(std::size_t memory_offset) {
         auto begin = memory_offset;
         auto end = begin;

         while (end+1 <= this->size() && this->get(end) != 0)
            ++end;

         return this->subsection<char>(begin, end+1);
      }

      const Memory<char> cstring_at(std::size_t memory_offset) const {
         auto begin = memory_offset;
         auto end = begin;

         while (end+1 <= this->size() && this->get(end) != 0)
            ++end;

         return this->subsection<char>(begin, end+1);
      }
      
      Memory<std::uint16_t> wstring_at(std::size_t memory_offset) {
         auto begin = memory_offset;
         auto end = begin;

         while (end+2 <= this->size() && this->cast_ref<std::uint16_t>(end) != 0) { end += 2; }

         return this->subsection<std::uint16_t>(begin, end+2, true);
      }

      const Memory<std::uint16_t> wstring_at(std::size_t memory_offset) const {
         auto begin = memory_offset;
         auto end = begin;

         while (end+2 <= this->size() && this->cast_ref<std::uint16_t>(end) != 0)
            end += 2;

         return this->subsection<std::uint16_t>(begin, end+2, true);
      }
   };
}
