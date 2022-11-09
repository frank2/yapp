//! @file exception.hpp
//! @brief The collection of exceptions which can be thrown by this library.
//!
#pragma once

#include <yapp/platform.hpp>

#include <cstdint>
#include <cstddef>
#include <exception>
#include <sstream>
#include <string>
#include <typeinfo>

namespace yapp
{
   /// @brief The base exception object.
   ///
   /// This object can be used to construct a custom YAPP exception.
   ///
   class Exception : public std::exception
   {
   public:
      std::string error;

      Exception() : std::exception() {}
      Exception(std::string error) : error(error), std::exception() {}   

      const char *what() const noexcept {
         return this->error.c_str();
      }
   };

   /// @brief Thrown when access to a slice object goes out-of-bounds.
   ///
   /// The offending *offset* and the proper *size* are given within the exception.
   ///
   class OutOfBoundsException : public Exception
   {
   public:
      std::size_t offset, size;

      OutOfBoundsException(std::size_t offset, std::size_t size) : offset(offset), size(size), Exception() {
         std::stringstream stream;
         stream << "Slice offset out-of-bounds: got offset " << offset << ", but size is " << size << ".";

         this->error = stream.str();
      }
   };

   /// @brief Thrown when a given slice object does not align with a given type.
   ///
   template <typename T, typename U=T>
   class AlignmentException : public Exception
   {
   public:
      AlignmentException() : Exception() {
         std::stringstream stream;
         stream << "Types " << typeid(T).name()
                << " (size " << sizeof(T)
                << ") and " << typeid(U).name()
                << " (size " << sizeof(U)
                << ") do not align with one another.";

         this->error = stream.str();
      }
      AlignmentException(std::size_t bytes) : Exception() {
         std::stringstream stream;
         stream << "Byte offset " << bytes
                << " did not align with " << typeid(T).name()
                << " (size " << sizeof(T) << ")";

         this->error = stream.str();
      }
   };

   /// @brief Thrown when the given search term is not long enough to match the slice type.
   ///
   /// For example: if you're attempting to search on a 4-byte sized slice with a 1-byte sized
   /// search term, but the search term is only two bytes long and not four, this exception
   /// would be raised.
   ///
   /// The offending search term is provided by the exception.
   ///
   template <typename T, typename U>
   class InsufficientDataException : public Exception
   {
   public:
      std::vector<U> data;
      
      InsufficientDataException(std::vector<U> &data) : data(data), Exception() {
         auto expected = sizeof(T) / sizeof(U);
         std::stringstream stream;

         stream << "Data is insufficient: got " << this->data.size()
                << " units of " << typeid(U).name()
                << ", but needed a multiple of " << expected;

         this->error = stream.str();
      }
   };

   /// @brief Thrown when an object encounters an unexpected null pointer.
   ///
   class NullPointerException : public Exception
   {
   public:
      NullPointerException() : Exception("Encountered an unexpected null pointer.") {}
   };

   /// @brief Thrown when search terms contain all wildcards.
   ///
   class SearchTooBroadException : public Exception
   {
   public:
      SearchTooBroadException() : Exception("The given search term was too broad; search terms cannot be all wildcards.") {}
    };

   /// @brief Thrown when a header allocation is insufficient compared to the header.
   ///
   class InsufficientAllocationException : public Exception
   {
   public:
      std::size_t attempted, needed;

      InsufficientAllocationException(std::size_t attempted, std::size_t needed)
         : attempted(attempted),
           needed(needed),
           Exception()
      {
         std::stringstream stream;

         stream << "The allocation size was insufficient: got " << this->attempted
                << " bytes, but needed at least " << this->needed;

         this->error = stream.str();
      }
   };

   /// @brief Thrown when the memory object isn't managed.
   ///
   class NotAllocatedException : public Exception
   {
   public:
      NotAllocatedException() : Exception("The memory object is not allocated.") {}
   };

   /// @brief Thrown when a memory allocator returns a bad pointer.
   ///
   class BadAllocationException : public Exception
   {
   public:
      BadAllocationException() : Exception("The allocator returned an invalid allocation.") {}
   };

   /// @brief Thrown when the parsed DOS signature for the PE file is invalid.
   ///
   class InvalidDOSSignatureException : public Exception
   {
   public:
      std::uint16_t bad_sig;
      
      InvalidDOSSignatureException(std::uint16_t bad_sig) : bad_sig(bad_sig), Exception() {
         std::stringstream stream;

         stream << "Invalid DOS signature: the signature " << std::hex << std::showbase << this->bad_sig
                << "is not a DOS signature.";

         this->error = stream.str();
      }
   };

   /// @brief Thrown when the parsed NT signature for the PE file is invalid.
   ///
   class InvalidNTSignatureException : public Exception
   {
   public:
      std::uint32_t bad_sig;
      
      InvalidNTSignatureException(std::uint32_t bad_sig) : bad_sig(bad_sig), Exception() {
         std::stringstream stream;

         stream << "Invalid NT signature: the signature " << std::hex << std::showbase << this->bad_sig
                << "is not an NT signature.";

         this->error = stream.str();
      }
   };

   /// @brief Thrown when the optional magic value isn't an expected value.
   ///
   class UnexpectedOptionalMagicException : public Exception
   {
   public:
      std::uint16_t bad_sig;
      std::uint16_t expected_sig;
      
      UnexpectedOptionalMagicException(std::uint16_t bad_sig, std::uint16_t expected_sig)
         : bad_sig(bad_sig), expected_sig(expected_sig), Exception()
      {
         std::stringstream stream;

         stream << "Unexpected optional magic: the magic value was  " << std::hex << std::showbase << this->bad_sig
                << ", but wanted " << this->expected_sig;

         this->error = stream.str();
      }
      UnexpectedOptionalMagicException(std::uint16_t bad_sig)
         : bad_sig(bad_sig), expected_sig(0), Exception()
      {
         std::stringstream stream;

         stream << "Unexpected optional magic: the magic value was  " << std::hex << std::showbase << this->bad_sig
                << ", but is not valid.";

         this->error = stream.str();
      }
 
   };

   class SectionNotFoundException : public Exception
   {
   public:
      SectionNotFoundException() : Exception("The section could not be found with the given parameter.") {}
   };

   class Offset;
   
   class InvalidOffsetException : public Exception
   {
   public:
      std::uint32_t offset;

      InvalidOffsetException(Offset offset);
   };

   class RVA;

   class InvalidRVAException : public Exception
   {
   public:
      std::uint32_t rva;

      InvalidRVAException(RVA rva);
   };

   class VA;

   class InvalidVAException : public Exception
   {
   public:
      std::uint64_t va;

      InvalidVAException(VA va);
   };

   class SectionTableOverflowException : public Exception
   {
   public:
      SectionTableOverflowException() : Exception("Operation would overflow the section table.") {}
   };

   class UnsupportedArchitectureException : public Exception
   {
   public:
      UnsupportedArchitectureException() : Exception("The architecture of this PE file is unsupported.") {}
   };

   class OpenFileFailureException : public Exception
   {
   public:
      std::string filename;

      OpenFileFailureException(const std::string &filename) : filename(filename), Exception() {
         std::stringstream stream;

         stream << "Failed to open file \"" << filename << "\".";

         this->error = stream.str();
      }
   };

   class DirectoryUnavailableException : public Exception
   {
   public:
      std::size_t index;

      DirectoryUnavailableException(std::size_t index) : index(index), Exception() {
         std::stringstream stream;

         stream << "Directory index " << index << " is either null or invalid.";

         this->error = stream.str();
      }
   };

   class InvalidPointerException : public Exception
   {
   public:
      const void *ptr;
      std::size_t size;

      InvalidPointerException(const void *ptr, std::size_t size) : ptr(ptr), size(size), Exception() {
         std::stringstream stream;

         stream << std::hex << std::showbase << "The given pointer " << std::uintptr_t(ptr)
                << std::dec << std::noshowbase << " with size " << size
                << " was invalidated before it could be used.";

         this->error = stream.str();
      }
   };

#ifdef YAPP_WIN32
   #include <windows.h>
   /// @brief Only on Windows. Thrown when `GetLastError()` returns a nonzero result.
   ///
   /// The error code is stored as the *code* variable in the thrown exception for further reference.
   ///
   class Win32Exception : public Exception
   {
   public:
      DWORD code;

      Win32Exception(DWORD code) : code(code), Exception() {
         LPSTR buffer = nullptr;
         std::stringstream stream;

         FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            code,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR)&buffer,
            0,
            NULL
         );
         
         stream << "Win32 Error 0x" 
                << std::hex << std::uppercase << this->code << std::nouppercase 
                << ": " << buffer;
         LocalFree(buffer);

         this->error = stream.str();
      }
      Win32Exception() : Win32Exception(GetLastError()) {}

      static void ThrowLastError() { if (GetLastError() != 0) { throw Win32Exception(); } }
   };
#endif
}
