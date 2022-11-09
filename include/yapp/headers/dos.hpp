#pragma once

#include <yapp/memory.hpp>
#include <yapp/headers/raw.hpp>

namespace yapp
{
namespace headers
{
   class DOSHeader : public Memory<raw::IMAGE_DOS_HEADER>
   {
   public:
      DOSHeader() : Memory(true) {}
      DOSHeader(raw::IMAGE_DOS_HEADER *pointer) : Memory(pointer) {}
      DOSHeader(const raw::IMAGE_DOS_HEADER *pointer) : Memory(pointer) {}

      virtual void set_defaults() {
         (*this)->e_magic = raw::IMAGE_DOS_SIGNATURE;
         (*this)->e_cblp = 0x90;
         (*this)->e_cp = 0x03;
         (*this)->e_crlc = 0;
         (*this)->e_cparhdr = 0x04;
         (*this)->e_minalloc = 0;
         (*this)->e_maxalloc = 0xFFFF;
         (*this)->e_ss = 0;
         (*this)->e_sp = 0xB8;
         (*this)->e_csum = 0;
         (*this)->e_ip = 0;
         (*this)->e_cs = 0;
         (*this)->e_lfarlc = 0x40;
         (*this)->e_ovno = 0;

         for (std::size_t i=0; i<4; ++i)
            (*this)->e_res[i] = 0;

         (*this)->e_oemid = 0;
         (*this)->e_oeminfo = 0;

         for (std::size_t i=0; i<10; ++i)
            (*this)->e_res2[i] = 0;

         (*this)->e_lfanew = 0xE0;
      }

      bool validate() const {
         return (*this)->e_magic == raw::IMAGE_DOS_SIGNATURE;
      }

      void throw_invalid() const {
         if (!this->validate())
            throw InvalidDOSSignatureException((*this)->e_magic);
      }
   };
}}
