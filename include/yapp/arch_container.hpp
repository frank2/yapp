#pragma once

#include <any>

#include <yapp/platform.hpp>

namespace yapp
{
   template <typename T32, typename T64>
   class ArchContainer
   {
   protected:
      std::any _container;

   public:
      #ifdef YAPP_64BIT
      using TypePlatform = typename T64;
      #else
      using TypePlatform = typename T32;
      #endif
      using Type32 = typename T32;
      using Type64 = typename T64;

      ArchContainer(Type32 t32) : _container(std::make_any<Type32>(t32)) {}
      ArchContainer(Type64 t64) : _container(std::make_any<Type64>(t64)) {}
      ArchContainer(const ArchContainer &other) : _container(other._container) {}

      std::any &container() { return this->_container; }
      const std::any &container() const { return this->_container; }
      bool is_32() const { return this->_container.type() == typeid(Type32); }
      bool is_64() const { return this->_container.type() == typeid(Type64); }
      Type32 &get_32() { return *std::any_cast<Type32>(&this->_container); }
      Type64 &get_64() { return *std::any_cast<Type64>(&this->_container); }
      const Type32 &get_32() const { return *std::any_cast<Type32>(&this->_container); }
      const Type64 &get_64() const { return *std::any_cast<Type64>(&this->_container); }
   };
}
