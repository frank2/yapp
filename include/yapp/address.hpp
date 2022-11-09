#pragma once

#include <any>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace yapp
{
   class PE;

   class Offset;
   class RVA;
   class VA;

   template <typename T>
   T align(T base, T alignment) {
      if constexpr(std::is_base_of<Address<std::uint32_t>, T>::value || std::is_base_of<Address<std::uint64_t>, T>::value)
      {
         if (base.value % alignment.value == 0) { return base; }
         else { return T(base.value + (alignment.value - (base.value % alignment.value))); }
      }
      else
      {
         if (base % alignment == 0) { return base; }
         else { return base + (alignment - (base % alignment)); }
      }
   }
   
   template <typename T>
   class Address
   {
      static_assert(std::is_same<T, std::uint32_t>::value || std::is_same<T, std::uint64_t>::value,
                    "Address base must be a 32-bit int or a 64-bit int.");
      
   public:
      using BaseType = T;
      
      T value;
      
      Address() : value(0) {}
      Address(T value) : value(value) {}
      Address(const Address &other) : value(other.value) {}

      Address operator+(const T& other) const { return this->value + other; }
      Address& operator+=(const T& other) { this->value += other; return *this; }
      Address operator-(const T& other) const { return this->value - other; }
      Address& operator-=(const T& other) { this->value -= other; return *this; }
      Address operator*(const T& other) const { return this->value * other; }
      Address& operator*=(const T& other) { this->value *= other; return *this; }
      Address operator/(const T& other) const { return this->value / other; }
      Address& operator/=(const T& other) { this->value /= other; return *this; }
      Address operator%(const T& other) const { return this->value % other; }
      Address& operator%=(const T& other) { this->value %= other; return *this; }
      Address operator^(const T& other) const { return this->value ^ other; }
      Address& operator^=(const T& other) { this->value ^= other; return *this; }
      
      Address& operator=(const T& other) { this->value = other; return *this; }

      operator T() { return this->value; }
      T operator*() { return this->value; }
   };

   class Offset : public Address<std::uint32_t>
   {
   public:
      Offset() : Address() {}
      Offset(Address::BaseType value) : Address(value) {}
      Offset(const Offset &other) : Address(other.value) {}

      RVA as_rva(const PE &) const;
      VA as_va(const PE &) const;
      template <typename T>
      T* as_ptr(PE &) const;
      template <typename T>
      const T* as_ptr(const PE &) const;
      std::size_t as_memory(const PE &) const;
   };

   class RVA : public Address<std::uint32_t>
   {
   public:
      RVA() : Address() {}
      RVA(Address::BaseType value) : Address(value) {}
      RVA(const RVA &other) : Address(other.value) {}

      Offset as_offset(const PE &) const;
      VA as_va(const PE &) const;
      template <typename T>
      T* as_ptr(PE &) const;
      template <typename T>
      const T* as_ptr(const PE &) const;
      std::size_t as_memory(const PE &) const;
   };

   class VA32 : public Address<std::uint32_t>
   {
   public:
      VA32() : Address() {}
      VA32(Address::BaseType value) : Address(value) {}
      VA32(const VA32 &other) : Address(other.value) {}

      Offset as_offset(const PE &) const;
      RVA as_rva(const PE &) const;
      template <typename T>
      T* as_ptr(PE &) const;
      template <typename T>
      const T* as_ptr(const PE &) const;
      std::size_t as_memory(const PE &) const;
   };

   class VA64 : public Address<std::uint64_t>
   {
   public:
      VA64() : Address() {}
      VA64(Address::BaseType value) : Address(value) {}
      VA64(const VA64 &other) : Address(other.value) {}

      Offset as_offset(const PE &) const;
      RVA as_rva(const PE &) const;
      template <typename T>
      T* as_ptr(PE &) const;
      template <typename T>
      const T* as_ptr(const PE &) const;
      std::size_t as_memory(const PE &) const;
   };
     
   class VA
   {
   protected:
      std::any va_value;

   public:
      VA() : va_value(std::make_any<VA32>(VA32(0))) {}
      VA(VA32 v32) : va_value(std::make_any<VA32>(v32)) {}
      VA(VA64 v64) : va_value(std::make_any<VA64>(v64)) {}
      VA(const VA& other) : va_value(other.va_value) {}

      VA& operator=(VA32 va32) { this->va_value = va32; return *this; }
      VA& operator=(VA64 va64) { this->va_value = va64; return *this; }
      VA& operator=(std::uint32_t value) { this->va_value = VA32(value); return *this; }
      VA& operator=(std::uint64_t value) { this->va_value = VA64(value); return *this; }
      operator std::uint32_t() { return *this->get_32(); }
      operator std::uint64_t() {
         if (this->is_32())
            return *this->get_32();
         else
            return *this->get_64();
      }
      
      std::uint64_t operator*() {
         if (this->is_32()) { return *this->get_32(); }
         else { return *this->get_64(); }
      }
      
      Offset as_offset(const PE &) const;
      RVA as_rva(const PE &) const;
      template <typename T>
      T* as_ptr(PE &) const;
      template <typename T>
      const T* as_ptr(const PE &) const;
      std::size_t as_memory(const PE &) const;
      bool is_32() const { return this->va_value.type() == typeid(VA32); }
      bool is_64() const { return this->va_value.type() == typeid(VA64); }
      VA32 &get_32() { return *std::any_cast<VA32>(&this->va_value); }
      VA64 &get_64() { return *std::any_cast<VA64>(&this->va_value); }
      const VA32 &get_32() const { return *std::any_cast<VA32>(&this->va_value); }
      const VA64 &get_64() const { return *std::any_cast<VA64>(&this->va_value); }
   };
}

#include "../src/address.tpp"
