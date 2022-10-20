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
   template <typename T, typename U>
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

   /// @brief Thrown when search terms contain all wildcards.
   ///
   class SearchTooBroadException : public Exception
   {
   public:
      SearchTooBroadException() : Exception("The given search term was too broad; search terms cannot be all wildcards.") {}
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
