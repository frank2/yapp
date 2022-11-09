#include <framework.hpp>
#include <yapp.hpp>

using namespace yapp;
using namespace yapp::headers;

#pragma pack(1)
struct SixByteStructure
{
   std::uint32_t dword;
   std::uint16_t word;
};

int test_readonly_memory()
{
   INIT();
   
   auto data = "\xde\xad\xbe\xef\xab\xad\x1d\xea\xde\xad\xbe\xa7\xde\xfa\xce\xd1";
   const Memory<char> slice(data, (std::size_t)16);

   ASSERT(slice.ptr() == &data[0]);
   ASSERT(slice.size() == 16);
   ASSERT(slice.eob() == slice.ptr()+16);
   ASSERT_THROWS(slice.get(16), OutOfBoundsException);

   auto byte_result = slice.cast_ref<const std::int8_t>(0);
   ASSERT(byte_result == -34);
   ASSERT_THROWS(slice.cast_ref<const std::int8_t>(slice.size()), OutOfBoundsException);

   auto subslice_4 = slice.subsection<std::uint32_t>(0, 4);
   ASSERT(subslice_4[0] == 0xEFBEADDE);

   auto subslice_6 = slice.subsection<SixByteStructure>(0, 2);
   ASSERT(subslice_6[0].word == 0xADAB);

   using ExpectedSubsliceException = AlignmentException<std::uint32_t, SixByteStructure>;
   ASSERT_THROWS(subslice_4.subsection<SixByteStructure>(0, 2), ExpectedSubsliceException);

   ASSERT(std::memcmp(slice.read<std::uint8_t>(8, 4).data(), "\xde\xad\xbe\xa7", 4) == 0);
   ASSERT(std::memcmp(slice.read<std::uint8_t>(0xC, 4).data(), "\xde\xfa\xce\xd1", 4) == 0);

   auto search_term = "\xde\xfa\xce\xd1";
   auto search_vec = std::vector<std::uint8_t>(search_term, &search_term[4]);
   
   ASSERT(slice.search(search_vec).size() == 1);
   ASSERT(slice.search<std::uint32_t>(0xD1CEFADE).size() == 1);
   ASSERT(slice.search<std::uint32_t>(0xFACEBABE).size() == 0);

   auto dynamic_data = "\xff\x27\x63\x58\x27\x64\xff\x27\x64\x88\x65\x43\x27\x38\x48\x58\x64\x27\x64";
   const Memory<char> dynamic_slice(dynamic_data, (std::size_t)19);
   std::optional<char> dynamic_search[6] = {
      std::nullopt,
      0x27,
      0x64,
      std::nullopt,
      0x27,
      0x64
   };

   ASSERT(dynamic_slice.search_dynamic(std::vector<std::optional<char>>(dynamic_search, &dynamic_search[6])).size() == 1);
   
   ASSERT(slice.contains<std::uint32_t>(0xDEADBEEF) == false);
   ASSERT(slice.contains<std::uint32_t>(0xEFBEADDE) == true);
   ASSERT(slice.reinterpret<std::uint32_t>()[2] == 0xA7BEADDE);

   auto split = slice.split_at(0x8);

   ASSERT(std::memcmp(split.first.ptr(), &data[0], 8) == 0);
   ASSERT(std::memcmp(split.second.ptr(), &data[8], 8) == 0);

   COMPLETE();
}

int test_dynamic_memory() {
   INIT();

   auto data = "\xde\xad\xbe\xef\xab\xad\x1d\xea\xde\xad\xbe\xa7\xde\xfa\xce\xd1";
   Memory<std::uint8_t> buffer((const std::uint8_t *)data, 16, true);

   std::uint8_t facebabe[] = {0xFA, 0xCE, 0xBA, 0xBE};
   ASSERT_SUCCESS(buffer.write<std::uint8_t>(0, facebabe, (std::size_t)4));
   ASSERT(!buffer.contains(0xEFBEADDE));

   ASSERT_SUCCESS(buffer.write<std::uint32_t>(4, 0xEFBEADDE));
   ASSERT(buffer.contains(0xEFBEADDE));

   std::uint8_t abad1dea[] = {0xAB, 0xAD, 0x1D, 0xEA};
   ASSERT_SUCCESS(buffer.append<std::uint32_t>(0xEA1DADAB));
   ASSERT(buffer.contains<std::uint8_t>(abad1dea, 4));

   auto rhs = buffer.split_off(0x8);
   ASSERT(!buffer.contains<std::uint8_t>(abad1dea, 4));

   buffer.resize(0xC, false, 0x69);
   ASSERT(buffer.cast_ref<std::uint32_t>(8) == 0x69696969);

   ASSERT_SUCCESS(buffer.write<std::uint32_t>(8, 0x74EEFFC0));
   ASSERT(buffer.contains(0x74EEFFC0));

   ASSERT_SUCCESS(buffer.append<std::uint8_t>(rhs));
   ASSERT(buffer.contains<std::uint8_t>(abad1dea, 4));
   ASSERT(buffer.contains<std::uint32_t>(0x74EEFFC0));

   ASSERT(std::memcmp(buffer.ptr(),
                      "\xfa\xce\xba\xbe\xde\xad\xbe\xef\xc0\xff\xee\x74\xde\xad\xbe\xa7\xde\xfa\xce\xd1\xab\xad\x1d\xea",
                      buffer.byte_size()) == 0);

   auto invalid_slice = buffer.subsection(0, buffer.size());
   buffer.deallocate();

   ASSERT_THROWS((void)invalid_slice.read(0, 4), InvalidPointerException);

   COMPLETE();
}

int test_compiled() {
   INIT();

   PE compiled(std::string("../test/corpus/compiled.exe"));
   
   auto dos_header = compiled.dos_header();
   ASSERT(dos_header.validate() == true);

   auto header_64 = compiled.nt_headers_64();
   ASSERT(header_64.validate() == false);

   ASSERT(Offset(0).as_ptr<headers::DOSHeader::BaseType>(compiled)->e_magic == headers::raw::IMAGE_DOS_SIGNATURE);
   ASSERT(Offset(dos_header->e_lfanew).as_ptr<headers::NTHeaders32::BaseType>(compiled)->Signature == headers::raw::IMAGE_NT_SIGNATURE);

   // this will implicitly test rva-to-offset conversion
   auto string_rva = RVA(0x3000);
   auto string_data = std::string(" * a 'compiled' PE\n");
   ASSERT(std::memcmp(string_rva.as_ptr<char>(compiled), string_data.c_str(), string_data.size()) == 0);
   ASSERT_THROWS((void)RVA(0x4000).as_offset(compiled), InvalidRVAException);
   
   COMPLETE();
}

int test_dll() {
   INIT();

   PE dll(std::string("../test/corpus/dll.dll"));

   ASSERT_SUCCESS((void)dll.data_directory().directory<ExportDirectory>(dll));

   auto export_directory = dll.data_directory().directory<ExportDirectory>(dll);
   auto &export32 = export_directory.get_32();

   ASSERT(std::string(export32.name(dll).ptr()) == std::string("dll.dll"));
   ASSERT_SUCCESS((void)export32.export_map(dll));

   auto export_map = export32.export_map(dll);
   
   ASSERT(export_map.find(std::string("export")) != export_map.end());
   ASSERT(export_map[std::string("export")].as_rva() == RVA(0x1024));
   
   COMPLETE();
}

int
main
(int argc, char *argv[])
{
   INIT();

   LOG_INFO("Testing readonly memory operations.");
   PROCESS_RESULT(test_readonly_memory);

   LOG_INFO("Testing dynamic memory operations.");
   PROCESS_RESULT(test_dynamic_memory);

   LOG_INFO("Testing parsing compiled.exe.");
   PROCESS_RESULT(test_compiled);

   LOG_INFO("Testing parsing dll.dll");
   PROCESS_RESULT(test_dll);
      
   COMPLETE();
}
