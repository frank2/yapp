#pragma once

#include <yapp/memory.hpp>
#include <yapp/headers/raw.hpp>

namespace yapp
{
   class PE;
   
namespace headers
{
   class DataDirectory : public Memory<raw::IMAGE_DATA_DIRECTORY>
   {
   public:
      DataDirectory(Memory::BaseType *pointer, std::size_t size) : Memory(pointer, size) {}
      DataDirectory(const Memory::BaseType *pointer, std::size_t size) : Memory(pointer, size) {}

      bool has_directory(const PE &pe, std::size_t directory) const;
      template <typename T>
      bool has_directory(const PE &pe) const;

      template <typename T>
      T directory(PE &pe);
      template <typename T>
      const T directory(const PE &pe) const;
   };
}}

#include "../src/headers/data_directory.tpp"
