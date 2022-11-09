#pragma once

#include <yapp/memory.hpp>
#include <yapp/headers/raw.hpp>

namespace yapp
{
namespace headers
{
   /// @brief The IMAGE_FILE_HEADER wrapper class.
   ///
   class FileHeader : public Memory<raw::IMAGE_FILE_HEADER>
   {
   public:
      FileHeader() : Memory(true) {}
      FileHeader(raw::IMAGE_FILE_HEADER *pointer) : Memory(pointer) {}
      FileHeader(const raw::IMAGE_FILE_HEADER *pointer) : Memory(pointer) {}

      /// @brief Set the defaults for this header for 32-bit machines.
      ///
      void set_defaults_32bit() {
         (*this)->Machine = raw::IMAGE_FILE_MACHINE_I386;
         (*this)->NumberOfSections = 0;
         (*this)->TimeDateStamp = 0; // TODO
         (*this)->PointerToSymbolTable = 0;
         (*this)->NumberOfSymbols = 0;
         (*this)->SizeOfOptionalHeader = static_cast<std::uint16_t>(sizeof(raw::IMAGE_OPTIONAL_HEADER32));
         (*this)->Characteristics = raw::IMAGE_FILE_EXECUTABLE_IMAGE | raw::IMAGE_FILE_32BIT_MACHINE;
      }

      /// @brief Set the defaults for this header for 64-bit machines.
      ///
      void set_defaults_64bit() {
         (*this)->Machine = raw::IMAGE_FILE_MACHINE_AMD64;
         (*this)->NumberOfSections = 0;
         (*this)->TimeDateStamp = 0; // TODO
         (*this)->PointerToSymbolTable = 0;
         (*this)->NumberOfSymbols = 0;
         (*this)->SizeOfOptionalHeader = static_cast<std::uint16_t>(sizeof(raw::IMAGE_OPTIONAL_HEADER64));
         (*this)->Characteristics = raw::IMAGE_FILE_EXECUTABLE_IMAGE | raw::IMAGE_FILE_32BIT_MACHINE;
      }

      /// @brief Set the defaults for this header, dependent on what architecture the code is running.
      ///
      /// **Note**: the default behavior of set_defaults for this header may not be ideal-- you may be on
      /// a 32-bit machine creating a 64-bit image. For this, call the specific architecture defaults function.
      ///
      void set_defaults() {
         #ifdef YAPP_64BIT
         this->set_defaults_64bit();
         #else
         this->set-defaults_32bit();
         #endif
      }
   };
}}
