#pragma once

#include <any>
#include <type_traits>

#include <yapp/memory.hpp>
#include <yapp/headers/raw.hpp>
#include <yapp/headers/file.hpp>
#include <yapp/headers/optional.hpp>

namespace yapp
{
namespace headers
{
   /// @brief The base class that handles both 32-bit and 64-bit NT headers.
   ///
   /// **Note**: the template only accepts raw::IMAGE_NT_HEADERS32 or raw::IMAGE_NT_HEADERS64.
   /// Anything else is a compile-time error. Additionally, this class is annoying to deal with.
   /// You probably want NTHeaders, NTHeaders32 or NTHeaders64.
   ///
   template <typename T>
   class NTHeadersBase : public Memory<T>
   {
      static_assert(std::is_same<T, raw::IMAGE_NT_HEADERS32>::value || std::is_same<T, raw::IMAGE_NT_HEADERS64>::value,
                    "Template class of NTHeaders object is not raw::IMAGE_NT_HEADERS32 or raw::IMAGE_NT_HEADERS64.");

      using OptionalType = typename std::conditional<
         std::is_same<T, raw::IMAGE_NT_HEADERS32>::value,
         OptionalHeader32,
         OptionalHeader64
         >::type;
         
   public:
      NTHeadersBase() : Memory(true) {}
      NTHeadersBase(T* pointer) : Memory(pointer) {}
      NTHeadersBase(const T* pointer) : Memory(pointer) {}

      FileHeader file_header() {
         return FileHeader(&(*this)->FileHeader);
      }

      const FileHeader file_header() const {
         return FileHeader(&(*this)->FileHeader);
      }

      OptionalType optional_header() {
         return OptionalType(&(*this)->OptionalHeader);
      }

      const OptionalType optional_header() const {
         return OptionalType(&(*this)->OptionalHeader);
      }
            
      void set_defaults() {
         (*this)->Signature = raw::IMAGE_NT_SIGNATURE;

         if constexpr (std::is_same<T, raw::IMAGE_NT_HEADERS32>::value)
            this->file_header().set_defaults_32bit();
         else
            this->file_header().set_defaults_64bit();
         
         this->optional_header().set_defaults();
      }

      bool validate() const {
         return (*this)->Signature == raw::IMAGE_NT_SIGNATURE && this->optional_header().validate();
      }

      void throw_invalid() const {
         if ((*this)->Signature != raw::IMAGE_NT_SIGNATURE)
            throw InvalidNTSignatureException((*this)->Signature);

         this->optional_header().throw_invalid();
      }
   };

   using NTHeaders32 = NTHeadersBase<raw::IMAGE_NT_HEADERS32>;
   using NTHeaders64 = NTHeadersBase<raw::IMAGE_NT_HEADERS64>;

   /// @brief A wrapper class for NTHeaders32 and NTHeaders64.
   ///
   class NTHeaders
   {
   protected:
      std::any nt_headers;

   public:
      NTHeaders(NTHeaders32 nt32) : nt_headers(std::make_any<NTHeaders32>(nt32)) {}
      NTHeaders(NTHeaders64 nt64) : nt_headers(std::make_any<NTHeaders64>(nt64)) {}
      NTHeaders(const NTHeaders &other) : nt_headers(other.nt_headers) {}

      bool is_32() const { return this->nt_headers.type() == typeid(NTHeaders32); }
      bool is_64() const { return this->nt_headers.type() == typeid(NTHeaders64); }
      NTHeaders32 &get_32() { return *std::any_cast<NTHeaders32>(&this->nt_headers); }
      NTHeaders64 &get_64() { return *std::any_cast<NTHeaders64>(&this->nt_headers); }
      const NTHeaders32 &get_32() const { return *std::any_cast<NTHeaders32>(&this->nt_headers); }
      const NTHeaders64 &get_64() const { return *std::any_cast<NTHeaders64>(&this->nt_headers); }

      bool validate() const {
         if (this->is_32()) { return this->get_32().validate(); }
         else { return this->get_64().validate(); }
      }

      void throw_invalid() const {
         if (this->is_32()) { return this->get_32().throw_invalid(); }
         else { return this->get_64().throw_invalid(); }
      }

      FileHeader file_header() {
         if (this->is_32()) { return this->get_32().file_header(); }
         else { return this->get_64().file_header(); }
      }

      const FileHeader file_header() const {
         if (this->is_32()) { return this->get_32().file_header(); }
         else { return this->get_64().file_header(); }
      }
   };
}}
