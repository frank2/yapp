#include <framework.hpp>
#include <yapp.hpp>

using namespace yapp;

#pragma pack(1)
struct SixByteStructure
{
   std::uint32_t dword;
   std::uint16_t word;
};

int test_slice()
{
   INIT();
   
   auto data = "\xde\xad\xbe\xef\xab\xad\x1d\xea\xde\xad\xbe\xa7\xde\xfa\xce\xd1";
   const Slice<char> slice(data, 16);

   ASSERT(slice.ptr() == &data[0]);
   ASSERT(slice.size() == 16);
   ASSERT(slice.eob() == slice.ptr()+16);
   ASSERT_THROWS(slice.get(16), OutOfBoundsException);

   auto byte_result = slice.cast_ref<const std::int8_t>(0);
   ASSERT(byte_result == -34);
   ASSERT_THROWS(slice.cast_ref<const std::int8_t>(slice.size()), OutOfBoundsException);

   auto subslice_4 = slice.subslice<std::uint32_t>(0, 4);
   ASSERT(subslice_4[0] == 0xEFBEADDE);

   auto subslice_6 = slice.subslice<SixByteStructure>(0, 2);
   ASSERT(subslice_6[0].word == 0xADAB);

   using ExpectedSubsliceException = AlignmentException<std::uint32_t, SixByteStructure>;
   ASSERT_THROWS(subslice_4.subslice<SixByteStructure>(0, 2), ExpectedSubsliceException);

   ASSERT(std::memcmp(slice.read<std::uint8_t>(8, 4).data(), "\xde\xad\xbe\xa7", 4) == 0);
   ASSERT(std::memcmp(slice.read<std::uint8_t>(0xC, 4).data(), "\xde\xfa\xce\xd1", 4) == 0);

   auto search_term = "\xde\xfa\xce\xd1";
   auto search_vec = std::vector<std::uint8_t>(search_term, &search_term[4]);
   
   ASSERT(slice.search(search_vec).size() == 1);
   ASSERT(slice.search<std::uint32_t>(0xD1CEFADE).size() == 1);
   ASSERT(slice.search<std::uint32_t>(0xFACEBABE).size() == 0);

   auto dynamic_data = "\xff\x27\x63\x58\x27\x64\xff\x27\x64\x88\x65\x43\x27\x38\x48\x58\x64\x27\x64";
   const Slice<char> dynamic_slice(dynamic_data, 19);
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

int test_buffer() {
   INIT();

   auto data = "\xde\xad\xbe\xef\xab\xad\x1d\xea\xde\xad\xbe\xa7\xde\xfa\xce\xd1";
   Buffer<std::uint8_t> buffer((const std::uint8_t *)data, 16);

   std::uint8_t facebabe[] = {0xFA, 0xCE, 0xBA, 0xBE};
   ASSERT_SUCCESS(buffer.write<std::uint8_t>(0, facebabe, 4));
   ASSERT(!buffer.contains(0xEFBEADDE));

   ASSERT_SUCCESS(buffer.write<std::uint32_t>(4, 0xEFBEADDE));
   ASSERT(buffer.contains(0xEFBEADDE));

   std::uint8_t abad1dea[] = {0xAB, 0xAD, 0x1D, 0xEA};
   ASSERT_SUCCESS(buffer.append<std::uint32_t>(0xEA1DADAB));
   ASSERT(buffer.contains<std::uint8_t>(abad1dea, 4));

   auto rhs = buffer.split_off(0x8);
   ASSERT(!buffer.contains<std::uint8_t>(abad1dea, 4));

   buffer.resize(0xC, 0x69);
   ASSERT(buffer.cast_ref<std::uint32_t>(8) == 0x69696969);

   ASSERT_SUCCESS(buffer.write<std::uint32_t>(8, 0x74EEFFC0));
   ASSERT(buffer.contains(0x74EEFFC0));

   ASSERT_SUCCESS(buffer.append<std::uint8_t>(rhs));
   ASSERT(buffer.contains<std::uint8_t>(abad1dea, 4));
   ASSERT(buffer.contains<std::uint32_t>(0x74EEFFC0));

   ASSERT(std::memcmp(buffer.ptr(),
                      "\xfa\xce\xba\xbe\xde\xad\xbe\xef\xc0\xff\xee\x74\xde\xad\xbe\xa7\xde\xfa\xce\xd1\xab\xad\x1d\xea",
                      buffer.size()) == 0);

   COMPLETE();
}

int
main
(int argc, char *argv[])
{
   INIT();

   LOG_INFO("Testing slice objects.");
   
   result += test_slice();

   LOG_INFO("Testing buffer objects.");

   result += test_buffer();
      
   COMPLETE();
}
