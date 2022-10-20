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
   auto vec = std::vector<std::uint8_t>(data, data+16);
   auto slice = Slice<std::uint8_t>(vec.data(), vec.size());

   ASSERT(slice.ptr() == &vec[0]);
   ASSERT(slice.size() == vec.size());
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
   auto dynamic_vec = std::vector<std::uint8_t>(dynamic_data, &dynamic_data[19]);
   auto dynamic_slice = Slice<std::uint8_t>(dynamic_vec.data(), dynamic_vec.size());
   std::optional<std::uint8_t> dynamic_search[6] = {
      std::nullopt,
      0x27,
      0x64,
      std::nullopt,
      0x27,
      0x64
   };

   ASSERT(dynamic_slice.search_dynamic(std::vector<std::optional<std::uint8_t>>(dynamic_search, &dynamic_search[6])).size() == 1);
   
   ASSERT(slice.contains<std::uint32_t>(0xDEADBEEF) == false);
   ASSERT(slice.contains<std::uint32_t>(0xEFBEADDE) == true);

   COMPLETE();
}

int
main
(int argc, char *argv[])
{
   INIT();

   LOG_INFO("Testing slice objects.");
   
   result += test_slice();
      
   COMPLETE();
}
