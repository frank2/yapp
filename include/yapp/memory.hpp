//! @file memory.hpp
//! @brief Core array-like objects containing a pointer/size pairs for somewhat safer access of data.
//!
//! The core design of these objects is based on some of Rust's concepts of array safety when
//! handling pointers (which we have to do a lot when it comes to PE files). A pointer on its own
//! can be relatively unsafe; a pointer with some sort of size bound is much safer (though not fully safe).
//!
//! Memory objects, on their own, aren't intended to be a long-term solution to memory access-- 
//! they're intended to be used to either build other primitives on top or to temporarily
//! access some memory. On their own, they are not responsible for tracking whether or not the pointers
//! are valid, and are thus dangerous to use without some awareness of the scope of the pointers
//! they're using.
//!
//! Remember: the difference between a *Memory* and a *Memory* is that the memory *owns the data.* It is
//! not pointing at some arbitrary location like a memory because the data its pointing to is a vector that
//! the memory object contains.
//!
//! ## Alignment
//!
//! One important thing to be aware of is *alignment of data*. This is key to converting between
//! memory types. For example, you can convert a 1-byte sized typed memory (e.g., std::uint8_t) into a memory
//! of 8-byte sized type (e.g., std::uint64_t, so long as you have a multiple of eight bytes in the memory)
//! because 8 % 1 == 0, but you can't convert a 6-byte size typed memory between a 4-byte size typed memory
//! because 6 % 4 == 2. In other words, when the modulus result is 0 (i.e., there is no remainder when
//! divinding between the two type sizes), the two types can convert cleanly between each other.
//!

#pragma once

#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <yapp/exception.hpp>

namespace yapp
{
   class MemoryManager
   {
   private:
      using KeyType = std::pair<const void *, std::size_t>;
      std::map<KeyType, std::size_t> refcount;
      std::map<KeyType, std::set<KeyType>> children;
      std::map<KeyType, KeyType> parents;
      std::mutex mutex;

      static std::unique_ptr<MemoryManager> Instance;
      
      MemoryManager() {}
      
   public:
      static MemoryManager &GetInstance() {
         if (MemoryManager::Instance == nullptr)
            MemoryManager::Instance = std::unique_ptr<MemoryManager>(new MemoryManager());

         return *MemoryManager::Instance;
      }

      bool is_valid(const void *ptr, std::size_t size) const {
         auto key = std::make_pair(ptr, size);
                                          
         return this->refcount.find(key) != this->refcount.end() && this->refcount.at(key) > 0;
      }

      std::size_t ref(const void *ptr, std::size_t size)
      {
         auto key = std::make_pair(ptr, size);
         this->mutex.lock();

         if (this->refcount.find(key) == this->refcount.end())
         {
            this->refcount[key] = 0;
         }

         ++this->refcount.at(key);
         
         if (this->parents.find(key) != this->parents.end())
         {
            auto parent_key = this->parents.at(key);
            this->mutex.unlock();
            this->ref(parent_key.first, parent_key.second);
            this->mutex.lock();
         }

         auto count = this->refcount.at(key);
         this->mutex.unlock();
         
         return count;
      }

      std::size_t deref(const void *ptr, std::size_t size)
      {
         auto key = std::make_pair(ptr, size);
         this->mutex.lock();
         
         auto iter = this->refcount.find(key);

         if (iter == this->refcount.end()) {
            this->mutex.unlock();
            return 0;
         }
                  
         auto value = --iter->second;

         if (this->parents.find(key) != this->parents.end())
         {
            auto parent_key = this->parents.at(key);
            this->mutex.unlock();
            this->deref(parent_key.first, parent_key.second);
         }
         else { this->mutex.unlock(); }
         
         // if this ptr/size pair got dereferenced to 0, it needs to be invalidated
         if (value == 0)
            this->invalidate(ptr, size, true);

         return value;
      }

      void relationship(const void *parent_ptr, std::size_t parent_size, const void *child_ptr, std::size_t child_size)
      {
         if (parent_ptr == child_ptr && parent_size == child_size)
         {
            // refcount will be increased when the pointer/size pair has its
            // refcount increased by a constructor
            return;
         }

         auto parent_key = std::make_pair(parent_ptr, parent_size);
         auto child_key = std::make_pair(child_ptr, child_size);

         this->mutex.lock();

         this->parents.insert(std::make_pair(child_key, parent_key));

         if (this->children.find(parent_key) == this->children.end())
         {
            this->children.insert(std::make_pair(parent_key, std::set<KeyType>()));
         }

         this->children.at(parent_key).insert(child_key);

         this->mutex.unlock();
      }

      void invalidate(const void *ptr, std::size_t size, bool derefed_parent=false) {
         auto key = std::make_pair(ptr, size);

         this->mutex.lock();

         if (this->refcount.find(key) == this->refcount.end())
         {
            this->mutex.unlock();
            return;
         }
                  
         this->refcount.erase(key);

         if (this->parents.find(key) != this->parents.end())
         {
            auto parent_key = this->parents.at(key);
            auto &children = this->children.at(parent_key);

            if (children.find(key) != children.end())
               children.erase(key);

            if (!derefed_parent)
            {
               this->mutex.unlock();
               this->deref(parent_key.first, parent_key.second);
               this->mutex.lock();
            }
         }

         if (this->children.find(key) != this->children.end())
         {
            // there's a very real possibility that in the chaos
            // of invalidating and dereferencing memory that
            // the child set gets erased out from under us, so copy
            // the set data into a vector.
            std::vector<KeyType> child_keys(this->children.at(key).begin(), this->children.at(key).end());

            for (auto child_key : child_keys)
            {
               this->mutex.unlock();
               this->invalidate(child_key.first, child_key.second);
               this->mutex.lock();
            }
            
            this->children.erase(key);
         }

         this->parents.erase(key);
         this->mutex.unlock();
      }
   };
            
   /// @brief A slice of memory, containing a pointer/size pair, modelled after Rust's slice object.
   ///
   /// A memory of memory, denoted by a pointer/size pair. Just like with Rust, this is a typically
   /// unsafe primitive, so be careful how you use it!
   ///
   template <typename T,
             bool TIsVariadic=false,
             typename Allocator = std::allocator<std::uint8_t>>
   class Memory
   {
      static_assert(std::is_same<Allocator::value_type, std::uint8_t>::value,
                    "Allocator does not allocate uint8_t buffers.");
      
   public:
      /// @brief Access to the base type of this memory class.
      using BaseType = typename T;
      
      /// @brief The type used for dynamic search terms.
      ///
      using DynamicSearchTerm = std::vector<std::optional<T>>;

      /// @brief The type used for returning dynamic search results.
      using DynamicSearchResult = std::vector<std::pair<std::size_t, std::vector<T>>>;

      static const bool variadic = TIsVariadic;
      
      /// @brief A forward iterator for a memory object.
      ///
      struct Iterator
      {
         using iterator_category = std::forward_iterator_tag;
         using difference_type = std::ptrdiff_t;
         using value_type = T;
         using pointer = T*;
         using reference = T&;

         Iterator(T* ptr) : ptr(ptr) {}

         reference operator*() const { return *this->ptr; }
         pointer operator->() { return this->ptr; }

         Iterator& operator++() { ++this->ptr; return *this; }
         Iterator& operator++(int) { auto tmp = *this; ++(*this); return tmp; }

         friend bool operator== (const Iterator& a, const Iterator& b) { return a.ptr == b.ptr; }
         friend bool operator!= (const Iterator& a, const Iterator& b) { return a.ptr != b.ptr; }

      private:
         T* ptr;
      };

      /// @brief A const forward iterator for a memory object.
      ///
      struct ConstIterator
      {
         using iterator_category = std::forward_iterator_tag;
         using difference_type = std::ptrdiff_t;
         using value_type = const T;
         using pointer = const T*;
         using reference = const T&;

         ConstIterator(const T* ptr) : ptr(ptr) {}

         reference operator*() { return *this->ptr; }
         pointer operator->() { return this->ptr; }

         ConstIterator& operator++() { ++this->ptr; return *this; }
         ConstIterator& operator++(int) { auto tmp = *this; ++(*this); return tmp; }

         friend bool operator== (const ConstIterator& a, const ConstIterator& b) { return a.ptr == b.ptr; }
         friend bool operator!= (const ConstIterator& a, const ConstIterator& b) { return a.ptr != b.ptr; }

      private:
         const T* ptr;
      };

   protected:
      Allocator allocator;
      
      union {
         const T* c;
         T* m;
      } pointer;
      
      /// @brief The size, in bytes, of this memory region.
      ///
      std::size_t _size;

      bool allocated;

      void throw_if_unallocated() const { if (this->pointer.c != nullptr && !this->allocated) { throw NotAllocatedException(); } }
      template <typename U>
      void throw_if_out_of_bounds(std::size_t offset, std::size_t size, bool size_in_bytes=false) const {
         if (this->validate_range(offset, size, size_in_bytes)) { return; }

         auto byte_size = size;
         if (!size_in_bytes) { byte_size *= sizeof(U); }

         auto fixed_offset = offset;
         if (!size_in_bytes) { fixed_offset *= this->element_size(); }

         auto offending_offset = fixed_offset+byte_size;
         
         throw OutOfBoundsException(offending_offset / this->element_size(), this->size());
      }

   public:
      /// @brief Create a default memory object, with the option to *allocate* the
      /// memory to the size of the template type (true) or just create a null
      /// memory object (false).
      ///
      Memory(bool allocate=false) : _size(0), allocator(Allocator()), allocated(false) {
         this->pointer.c = nullptr;

         if (allocate) { this->allocate(1); }
      }
      /// @brief Construct a *Memory* object from a given *pointer* and *size*, with option to *copy*
      /// or interpret the size as bytes (*size_in_bytes*).
      ///
      /// @throw NullPointerException
      ///
      Memory(T *pointer, std::size_t size, bool copy=false, bool size_in_bytes=false)
         : allocator(Allocator()), allocated(false)
      {
         this->pointer.m = nullptr;
         this->set_memory(pointer, size, copy, size_in_bytes);
      }
      /// @brief Construct a *Memory* object from a given const *pointer* and *size*, with option to *copy*
      /// or interpret the size as bytes (*size_in_bytes*).
      ///
      /// ### Undefined behavior
      ///
      /// This constructor has the potential to cause undefined behavior if you use non-const functionality with it.
      /// Only use this constructor if you intend to create a `const Memory<T>` object.
      ///
      /// @throw NullPointerException
      ///
      Memory(const T *pointer, std::size_t size, bool copy=false, bool size_in_bytes=false)
         : allocator(Allocator()), allocated(false)
      {
         this->pointer.m = nullptr;
         this->set_memory(pointer, size, copy, size_in_bytes);
      }
      /// @brief Construct a memory object from a *pointer*.
      ///
      /// @throw NullPointerException
      ///
      Memory(T *pointer, bool copy=false)
         : allocator(Allocator()), allocated(false)
      {
         this->pointer.m = nullptr;
         this->set_memory(pointer, 1, copy, false);
      }
      /// @brief Contsruct a memory object from a const *pointer*.
      ///
      /// @throw NullPointerException
      ///
      Memory(const T* pointer, bool copy=false)
         : allocator(Allocator()), allocated(false)
      {
         this->pointer.m = nullptr;
         this->set_memory(pointer, 1, copy, false);
      }
      /// @brief Construct a memory object from a vector of *data*.
      ///
      /// @throw NullPointerException
      ///
      Memory(const std::vector<T> &data) : allocator(Allocator()), allocated(false) {
         this->pointer.m = nullptr;
         this->load_data(data);
      }
      /// @brief Construct a memory object with a given *size*, with the option to
      /// interpret the size in terms of bytes (*size_in_bytes*).
      ///
      Memory(std::size_t size, bool size_in_bytes=false) : allocator(Allocator()), allocated(false) {
         this->pointer.m = nullptr;

         auto fixed_size = size;
         if (!size_in_bytes) { fixed_size *= sizeof(T); }
         
         this->allocate(fixed_size, true);
      }
      /// @brief Construct a memory object from a given *filename*.
      ///
      /// @throw std::ios_base::failure
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      Memory(const std::string &filename) : allocator(Allocator()), allocated(false) {
         this->pointer.m = nullptr;
         this->load_file(filename);
      }

      Memory(const Memory &other)
         : allocator(other.allocator),
           allocated(false),
           _size(0)
      {
         this->pointer.m = nullptr;
         
         if (other.allocated) {
            this->allocate(other._size, true);
            this->write(0, other);
         }
         else
         {
            this->set_memory(other.pointer.m, other._size, false, true);
         }
      }
      ~Memory() {
         // deallocate calls invalidate, not deref, because it directly erases the memory
         if (this->allocated) { this->deallocate(); }
         // an unmanaged pointer gets dereferenced because it might be owned elsewhere
         else { MemoryManager::GetInstance().deref(this->pointer.c, this->_size); }
      }

      /// @brief Syntactic sugar to make memory objects more like arrays.
      ///
      /// @throw OutOfBoundsException
      /// @throw NullPointerException
      ///
      inline T& operator[](std::size_t index) {
         return this->get(index);
      }

      /// @brief Syntactic sugar to make memory objects more like arrays.
      ///
      /// @throw OutOfBoundsException
      /// @throw NullPointerException
      ///
      inline const T& operator[](std::size_t index) const {
         return this->get(index);
      }

      /// @brief Syntactic sugar to access the underlying type,
      /// or the first member of the array.
      ///
      /// @throw NullPointerException
      ///
      inline T* operator->() {
         auto ptr = this->ptr();
         
         if (ptr == nullptr) { throw NullPointerException(); }

         return ptr;
      }

      /// @brief Syntactic sugar to access the underlying type as const,
      /// or the first member of the array.
      ///
      /// @throw NullPointerException
      ///
      inline const T* operator->() const {
         const auto ptr = this->ptr();
         
         if (ptr == nullptr) { throw NullPointerException(); }

         return ptr;
      }
      
      /// @brief Syntactic sugar to access the underlying type as a reference,
      /// or the first member of the array.
      ///
      /// @throw NullPointerException
      ///
      inline T& operator*() {
         auto ptr = this->ptr();

         if (ptr == nullptr) { throw NullPointerException(); }

         return *ptr;
      }
      
      /// @brief Syntactic sugar to access the underlying type as a const reference,
      /// or the first member of the array.
      ///
      /// @throw NullPointerException
      ///
      inline const T& operator*() const {
         const auto ptr = this->ptr();

         if (ptr == nullptr) { throw NullPointerException(); }

         return *ptr;
      }

      /// @brief Set the memory region of this object.
      ///
      /// @throw NullPointerException
      ///
      void set_memory(T* pointer, std::size_t size, bool copy=false, bool size_in_bytes=false) {
         if (copy) {
            this->set_memory(reinterpret_cast<const T*>(pointer), size, true, size_in_bytes);
            return;
         }
        
         if (pointer == nullptr) { throw NullPointerException(); }
         
         if (this->allocated) { this->deallocate(); }
         else { MemoryManager::GetInstance().deref(this->pointer.c, this->_size); }
         
         std::size_t byte_size = size;
         if (!size_in_bytes) { byte_size *= sizeof(T); }
         
         this->pointer.m = pointer;
         this->_size = byte_size;
         this->allocated = false;

         MemoryManager::GetInstance().ref(pointer, byte_size);
      }

      /// @brief Set the memory region of this object with a const pointer.
      ///
      /// ### Undefined behavior
      ///
      /// If not copying the pointer, this constructor has the potential to cause undefined behavior
      /// if you use non-const functionality with the object after calling this function. Only use
      /// this function with a non-copied pointer if you intend to create a `const Memory<T>` object.
      ///
      /// @throw NullPointerException
      ///
      void set_memory(const T* pointer, std::size_t size, bool copy=false, bool size_in_bytes=false) {
         if (pointer == nullptr) { throw NullPointerException(); }
         
         if (this->allocated) { this->deallocate(); }
         else { MemoryManager::GetInstance().deref(this->pointer.c, this->_size); }
         
         std::size_t byte_size = size;
         if (!size_in_bytes) { byte_size *= sizeof(T); }

         if (copy)
         {
            this->allocate(byte_size, true);
            this->write(0, pointer, byte_size, true);
         }
         else {
            this->pointer.c = pointer;
            this->_size = byte_size;
            this->allocated = false;

            MemoryManager::GetInstance().ref(pointer, byte_size);
         }
      }
      
      /// @brief Allocate this memory region with the given allocator class and the *size* to allocate,
      /// with optional *initial* value and interpretation of the size as bytes (*size_in_bytes*).
      ///
      /// @throw InsufficientAllocationException
      /// @throw BadAllocationException
      ///
      void allocate(std::size_t size, bool size_in_bytes = false, std::optional<T> initial = std::nullopt) {
         if (this->allocated) { this->deallocate(); }

         std::size_t byte_size = size;
         if (!size_in_bytes) { byte_size *= sizeof(T); }
         if (byte_size < sizeof(T)) { throw InsufficientAllocationException(byte_size, sizeof(T)); }
                 
         this->pointer.m = reinterpret_cast<T*>(this->allocator.allocate(byte_size));
         if (this->pointer.m == nullptr) { throw BadAllocationException(); }
         
         this->_size = byte_size;
         this->allocated = true;
         
         MemoryManager::GetInstance().ref(this->pointer.m, byte_size);

         if (initial.has_value())
         {
            for (std::size_t i=0; i<this->elements(); ++i)
               this->get(i) = *initial;
         }
      }

      /// @brief Deallocate this memory with the given allocator class.
      ///
      /// @throw NotAllocatedException
      ///
      void deallocate() {
         this->throw_if_unallocated();

         MemoryManager::GetInstance().invalidate(this->pointer.m, this->_size);
         
         this->allocator.deallocate(reinterpret_cast<std::uint8_t *>(this->pointer.m), this->_size);
         
         this->pointer.m = nullptr;
         this->_size = 0;
         this->allocated = false;
      }

      /// @brief Reallocate this memory with the given allocator class with given *size* and options
      /// to interpret the size as bytes (*size_in_bytes*) or optional *padding* value.
      ///
      /// @throw InsufficientAllocationException
      /// @throw NotAllocatedException
      ///
      void reallocate(std::size_t size, bool size_in_bytes = false, std::optional<T> padding = std::nullopt) {
         // throws if pointer is non-null and allocated is false
         this->throw_if_unallocated();

         // if we get here, we're either non-null and allocated or
         // null and not allocated, so check if we're allocated
         if (!this->allocated)
         {
            this->allocate(size, size_in_bytes, padding);
            return;
         }

         std::size_t byte_size = size;
         if (!size_in_bytes) { byte_size *= sizeof(T); }
         
         auto ptr_start = reinterpret_cast<std::uint8_t *>(this->pointer.m);
         auto ptr_end = &ptr_start[this->_size];
         auto data = std::vector<std::uint8_t>(ptr_start, ptr_end);
         
         this->deallocate();
         this->allocate(byte_size, true, padding);
         this->write<std::uint8_t>(0, data.data(), (byte_size < data.size()) ? byte_size : data.size(), true);
      }

      /// @brief Get the element at the given *index* in the memory object.
      ///
      /// @throw OutOfBoundsException
      /// @throw NullPointerException
      ///
      inline T& get(std::size_t index) {
         if (this->ptr() == nullptr) { throw NullPointerException(); }
         if (index >= this->elements()) { throw OutOfBoundsException(index, this->elements()); }

         return this->ptr()[index];
      }

      /// @brief Get the const element at the given *index* in the memory.
      ///
      /// @throw OutOfBoundsException
      /// @throw NullPointerException
      ///
      inline const T& get(std::size_t index) const {
         if (this->ptr() == nullptr) { throw NullPointerException(); }
         if (index >= this->elements()) { throw OutOfBoundsException(index, this->elements()); }

         return this->ptr()[index];
      }

      /// @brief Get the first element in this memory.
      ///
      /// @throw OutOfBoundsException
      /// @throw NullPointerException
      ///
      inline T& front() {
         return this->get(0);
      }
      
      /// @brief Get the first element in this memory.
      ///
      /// @throw OutOfBoundsException
      /// @throw NullPointerException
      ///
      inline const T& front() const {
         return this->get(0);
      }

      /// @brief Get the last element in this memory.
      ///
      /// @throw OutOfBoundsException
      /// @throw NullPointerException
      ///
      inline T& back() {
         if (this->elements() == 0) { throw OutOfBoundsException(0, this->elements()); }
         
         return this->get(this->elements()-1);
      }
      
      /// @brief Get the first element in this memory.
      ///
      /// @throw OutOfBoundsException
      /// @throw NullPointerException
      ///
      inline const T& back() const {
         if (this->elements() == 0) { throw OutOfBoundsException(0, this->elements()); }
         
         return this->get(this->elements()-1);
      }

      /// @brief Check whether this memory is empty.
      ///
      inline bool is_empty() { return this->_size == 0; }

      /// @brief Check whether this memory is allocated.
      ///
      inline bool is_allocated() { return this->allocated; }

      /// @brief Get a forward iterator to the beginning of this memory.
      ///
      /// @throw NullPointerException
      ///
      inline Iterator begin() {
         if (this->ptr() == nullptr) { throw NullPointerException(); }
         
         return Iterator(&this->ptr()[0]);
      }

      inline ConstIterator cbegin() const {
         if (this->ptr() == nullptr) { throw NullPointerException(); }

         return ConstIterator(&this->ptr()[0]);
      }

      /// @brief Get the end iterator of this memory.
      ///
      /// To return the end of the memory as a pointer, see *Memory::eob*.
      ///
      /// @throw NullPointerException
      ///
      inline Iterator end() { return Iterator(this->eob()); }

      inline ConstIterator cend() const { return ConstIterator(this->eob()); }

      /// @brief Get the end pointer of this memory, or "end of buffer."
      ///
      /// To return the end iterator of the memory, see *Memory::end*.
      ///
      /// @throw NullPointerexception
      ///
      inline T* eob() {
         if (this->ptr() == nullptr) { throw NullPointerException(); }
         
         return reinterpret_cast<T *>(&reinterpret_cast<std::uint8_t *>(this->ptr())[this->_size]);
      }

      /// @brief Get the const end pointer of this memory, or "end of memory."
      ///
      /// To return the end iterator of the memory, see *Memory::end*.
      ///
      /// @throw NullPointerexception
      ///
      inline const T* eob() const {
         if (this->ptr() == nullptr) { throw NullPointerException(); }

         return reinterpret_cast<const T *>(&reinterpret_cast<const std::uint8_t *>(this->ptr())[this->_size]);
      }

      /// @brief Get the base pointer of this memory.
      ///
      inline T* ptr(void) {
         if (this->pointer.m == nullptr) { return nullptr; }

         if (!MemoryManager::GetInstance().is_valid(this->pointer.c, this->_size))
            throw InvalidPointerException(this->pointer.c, this->_size);
         
         return this->pointer.m;
      }
      
      /// @brief Get the const base pointer of this memory.
      ///
      inline const T* ptr(void) const {
         if (this->pointer.c == nullptr) { return nullptr; }

         if (!MemoryManager::GetInstance().is_valid(this->pointer.c, this->_size))
            throw InvalidPointerException(this->pointer.c, this->_size);
         
         return this->pointer.c;
      }

      /// @brief Get the length of this memory region in terms of its base type.
      ///
      inline std::size_t size(void) const { return this->elements(); }

      /// @brief Get the size in bytes of this memory region.
      ///
      inline std::size_t byte_size(void) const { return this->_size; }

      /// @brief Get the size of an element in this memory region.
      ///
      inline std::size_t element_size(void) const {
         if constexpr (this->variadic) { return this->_size; }
         else { return sizeof(T); }
      }

      /// @brief Get the number of elements in this memory region.
      ///
      inline std::size_t elements(void) const {
         if constexpr (this->variadic) { return (this->_size > 0) ? 1 : 0; }
         else { return this->_size / sizeof(T); }
      }

      /// @brief Get the amount of units of type *U* needed to fill the type of this memory,
      /// noting the structure variance with *UIsVariadic*.
      ///
      /// This returns 0 if the given type *U* is larger than the memory type *T*.
      ///
      template <typename U>
      inline std::size_t elements_needed() const {
         if constexpr (this->variadic) { return this->_size / sizeof(U); }
         else { return sizeof(T) / sizeof(U); }
      }

      /// @brief Check if this memory aligns with the given size boundary.
      ///
      inline bool aligns_with(std::size_t size) const {
         // variadic structures have to align on byte boundaries, so by default they align
         // with everything
         if constexpr (this->variadic) { return true; }
         
         std::size_t smaller = (this->element_size() < size) ? this->element_size() : size;
         std::size_t bigger = (this->element_size() > size) ? this->element_size() : size;

         return (bigger % smaller == 0);
      }

      /// @brief Check if the given type argument aligns with the memory's element boundaries.
      ///
      template <typename U, bool UIsVariadic=false>
      inline bool aligns_with() const { if constexpr (UIsVariadic) { return true; } else { return this->aligns_with(sizeof(U)); } }

      /// @brief Get a pointer into this memory of the given typename *U* at the given *offset*,
      /// with the option to interpret the offset in terms of bytes (*offset_in_bytes*).
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw NullPointerException
      ///
      template <typename U>
      U* cast_ptr(std::size_t offset, bool offset_in_bytes=false)
      {
         std::size_t fixed_offset = offset;
         if (!offset_in_bytes) { fixed_offset *= sizeof(T); }
         
         if (fixed_offset >= this->_size) { throw OutOfBoundsException(fixed_offset / sizeof(T), this->elements()); }
         if (!this->aligns_with<U>() || (offset_in_bytes && !this->aligns_with(fixed_offset))) { throw AlignmentException<T, U>(); }

         auto this_bytes = this->_size;
         auto cast_bytes = fixed_offset + sizeof(U);

         if (cast_bytes > this_bytes) { throw OutOfBoundsException(cast_bytes / sizeof(T), this->elements()); }
            
         return reinterpret_cast<U*>(&this->get(fixed_offset / sizeof(T)));
      }

      /// @brief Get a const pointer into this memory of the given typename *U* at the given *offset*,
      /// with the option to interpret the offset in terms of bytes (*offset_in_bytes*).
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw NullPointerException
      ///
      template <typename U>
      const U* cast_ptr(std::size_t offset, bool offset_in_bytes=false) const
      {
         std::size_t fixed_offset = offset;
         if (!offset_in_bytes) { fixed_offset *= sizeof(T); }
         
         if (fixed_offset >= this->_size) { throw OutOfBoundsException(fixed_offset / sizeof(T), this->elements()); }
         if (!this->aligns_with<U>() || (offset_in_bytes && !this->aligns_with(fixed_offset))) { throw AlignmentException<T, U>(); }

         auto this_bytes = this->_size;
         auto cast_bytes = fixed_offset + sizeof(U);

         if (cast_bytes > this_bytes) { throw OutOfBoundsException(cast_bytes / sizeof(T), this->elements()); }

         auto ptr = &this->get(fixed_offset / sizeof(T));
         return reinterpret_cast<const U*>(ptr);
      }

      /// @brief Get a reference into this memory of the given typename *U* at the given *offset*, with the
      /// option to interpret the offset in terms of bytes (*offset_in_bytes*).
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw NullPointerException
      ///
      template <typename U>
      U& cast_ref(std::size_t offset, bool offset_in_bytes=false)
      {
         return *this->cast_ptr<U>(offset, offset_in_bytes);
      }

      /// @brief Get a const reference into this memory of the given typename *U* at the given *offset*, with
      /// the option to interpret the offset in terms of bytes (*offset_in_bytes*).
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw NullPointerException
      ///
      template <typename U>
      const U& cast_ref(std::size_t offset, bool offset_in_bytes=false) const
      {
         return *this->cast_ptr<U>(offset, offset_in_bytes);
      }

      /// @brief Validate that the given *offset* and *size* with type *U* is within the bounds of this
      /// memory region, with the option of interpretting the *size* and *offset* as bytes
      /// (*size_in_bytes*).
      ///
      template <typename U, bool UIsVariadic=false>
      bool validate_range(std::size_t offset, std::size_t size, bool size_in_bytes=false) const
      {
         auto byte_size = size;
         if (!size_in_bytes) { byte_size *= sizeof(U); }

         auto fixed_offset = offset;
         if (!size_in_bytes) { fixed_offset *= this->element_size(); }

         return fixed_offset+byte_size <= this->_size;
      }
      
      /// @brief Validate that the given *offset* and *size* of base type *T* is within the bounds of this
      /// memory region, with the option of interpretting the *size* and *offset* as bytes
      /// (*size_in_bytes*).
      ///
      bool validate_range(std::size_t offset, std::size_t size, bool size_in_bytes=false) const
      {
         return this->validate_range<T>(offset, size, size_in_bytes);
      }

      /// @brief Return the memory as a vector of bytes.
      ///
      /// Useful for when you just want the raw bytes of the memory, rather than the individual interpretted elements.
      ///
      /// @throw NullPointerException
      ///
      std::vector<std::uint8_t> as_bytes(void) const {
         if (this->ptr() == nullptr) { throw NullPointerException(); }
         
         return std::vector<std::uint8_t>(reinterpret_cast<const std::uint8_t *>(this->ptr()),
                                          reinterpret_cast<const std::uint8_t *>(this->eob()));
      }

      /// @brief Convert this memory object into a vector.
      ///
      /// @throw NullPointerException
      ///
      std::vector<T> to_vec(void) const {
         if (this->ptr() == nullptr) { throw NullPointerException(); }

         return std::vector<T>(this->ptr(), this->eob());
      }

      /// @brief Save this memory to disk.
      ///
      /// @throw std::ios_base::failure
      /// @throw NullPointerException
      ///
      void save(const std::string &filename) const {
         if (this->ptr() == nullptr) { throw NullPointerException(); }
         
         std::ofstream fp(filename);
         auto bytes = reinterpret_cast<const char *>(this->ptr());
         fp.write(bytes, this->_size);
         fp.close();
      }

      /// @brief Create a const subsection of this memory at the given *offset* with the given type *U* and *size*,
      /// with the option to interpret *size* and *offset* as bytes (*size_in_bytes*).
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw NullPointerException
      ///
      template <typename U, bool UIsVariadic=false>
      const Memory<U, UIsVariadic, Allocator> subsection(std::size_t offset, std::size_t size, bool size_in_bytes=false) const
      {
         std::size_t byte_size = size;
         if (!size_in_bytes) { byte_size *= sizeof(U); }

         std::size_t fixed_offset = offset;
         if (!size_in_bytes) { fixed_offset *= this->element_size(); }
         
         if (this->ptr() == nullptr) { throw NullPointerException(); }
         if (fixed_offset >= this->_size) { throw OutOfBoundsException(fixed_offset / this->element_size(), this->elements()); }
         if (!this->aligns_with<U, UIsVariadic>() || (size_in_bytes && !this->aligns_with(byte_size))) { throw AlignmentException<T, U>(); }

         auto this_bytes = this->_size;
         auto cast_bytes = fixed_offset + byte_size;

         if (cast_bytes > this_bytes) { throw OutOfBoundsException(cast_bytes / sizeof(T), this->elements()); }

         auto base_ptr = &this->ptr()[fixed_offset / this->element_size()];

         MemoryManager::GetInstance().relationship(this->pointer.c, this->_size, base_ptr, byte_size);

         return Memory<U, UIsVariadic, Allocator>(reinterpret_cast<const U*>(base_ptr), byte_size, false, true);
      }

      /// @brief Create a subsection of this memory at the given *offset* and *size*, with the option
      /// to interpret the size as bytes (*size_in_bytes*).
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw NullPointerException
      ///
      const Memory<T, TIsVariadic, Allocator> subsection(std::size_t offset, std::size_t size, bool size_in_bytes=false) const {
         return this->subsection<T, TIsVariadic>(offset, size, size_in_bytes);
      }
      
      /// @brief Create a subsection of this memory at the given *offset* with the given type *U* and *size*,
      /// with the option to interpret *size* and *offset* as bytes (*size_in_bytes*).
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw NullPointerException
      ///
      template <typename U, bool UIsVariadic=false>
      Memory<U, UIsVariadic, Allocator> subsection(std::size_t offset, std::size_t size, bool size_in_bytes=false)
      {
         std::size_t byte_size = size;
         if (!size_in_bytes) { byte_size *= sizeof(U); }

         std::size_t fixed_offset = offset;
         if (!size_in_bytes) { fixed_offset *= sizeof(T); }
         
         if (this->ptr() == nullptr) { throw NullPointerException(); }
         if (fixed_offset >= this->_size) { throw OutOfBoundsException(fixed_offset / sizeof(T), this->elements()); }
         if (!this->aligns_with<U, UIsVariadic>() || (size_in_bytes && !this->aligns_with(byte_size))) { throw AlignmentException<T, U>(); }

         auto this_bytes = this->_size;
         auto cast_bytes = fixed_offset + byte_size;

         if (cast_bytes > this_bytes) { throw OutOfBoundsException(cast_bytes / sizeof(T), this->elements()); }

         auto base_ptr = &this->ptr()[fixed_offset / this->element_size()];

         MemoryManager::GetInstance().relationship(this->pointer.c, this->_size, base_ptr, byte_size);

         return Memory<U, UIsVariadic, Allocator>(reinterpret_cast<U*>(base_ptr), byte_size, false, true);
      }

      /// @brief Create a subsection of this memory at the given *offset* and *size*, with the option
      /// to interpret the *offset* and *size* as bytes (*size_in_bytes*).
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw NullPointerException
      ///
      Memory<T, TIsVariadic, Allocator> subsection(std::size_t offset, std::size_t size, bool size_in_bytes=false) {
         return this->subsection<T, TIsVariadic>(offset, size, size_in_bytes);
      }

      /// @brief Reinterpret the memory as the given type *U*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U, bool UIsVariadic=false>
      Memory<U, UIsVariadic, Allocator> reinterpret() {
         if (!this->aligns_with<U, UIsVariadic>()) { throw AlignmentException<T, U>(); }

         auto needed = this->elements_needed<U>();
         auto adjusted_size = this->_size / sizeof(U);

         if (needed > 0 && adjusted_size % needed != 0) { throw InsufficientDataException<T, U>(this->read<U>(0, adjusted_size)); }

         return this->subsection<U, UIsVariadic>(0, adjusted_size);
      }

      /// @brief Reinterpret the memory as the given const type *U*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U, bool UIsVariadic=false>
      const Memory<U, UIsVariadic, Allocator> reinterpret() const {
         if (!this->aligns_with<U, UIsVariadic>()) { throw AlignmentException<T, U>(); }

         auto needed = this->elements_needed<U>();
         auto adjusted_size = this->_size / sizeof(U);

         if (needed > 0 && adjusted_size % needed != 0) { throw InsufficientDataException<T, U>(this->read<U>(0, adjusted_size)); }

         return this->subsection<U, UIsVariadic>(0, adjusted_size);
      }

      /// @brief Read an arbitrary amount of *U* values from the given *offset* into a vector of the given *size*,
      /// with the option of interpretting *size* and *offset* as bytes (*size_in_bytes*).
      ///
      /// Returns a vector of *U* items. If you want to create a memory object, use Memory::submemory.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw NullPointerException
      ///
      template <typename U, bool UIsVariadic=false>
      std::vector<U> read(std::size_t offset, std::size_t size, bool size_in_bytes=false) const
      {
         std::size_t byte_size = size;
         if (!size_in_bytes) { byte_size *= sizeof(U); }

         std::size_t fixed_offset = offset;
         if (!size_in_bytes) { fixed_offset *= sizeof(T); }
         
         if (this->ptr() == nullptr) { throw NullPointerException(); }
         if (fixed_offset >= this->_size) { throw OutOfBoundsException(fixed_offset / sizeof(T), this->elements()); }
         if (!this->aligns_with<U, UIsVariadic>() || (size_in_bytes && !this->aligns_with(byte_size))) { throw AlignmentException<T, U>(); }

         auto this_bytes = this->_size;
         auto cast_bytes = fixed_offset + byte_size;

         if (cast_bytes > this_bytes) { throw OutOfBoundsException(cast_bytes / sizeof(T), this->elements()); }

         return std::vector<U>(reinterpret_cast<const U*>(&this->ptr()[fixed_offset / sizeof(T)]),
                               reinterpret_cast<const U*>(&this->ptr()[cast_bytes / sizeof(T)]));
      }

      /// @brief Read an arbitrary amount of values from the memory at the given *offset* with the given *size*,
      /// with the option to interpret the *offset* and *size* as bytes (*size_in_bytes*).
      ///
      /// Returns a vector of *T* of the given elements.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw NullPointerException
      ///
      std::vector<T> read(std::size_t offset, std::size_t size, bool size_in_bytes=false) const {
         return this->read<T, TIsVariadic>(offset, size, size_in_bytes);
      }

      /// @brief Write a memory of *U* items in *data* to the given memory object at the given *offset*,
      /// with the option to interpret the offset in terms of bytes (*offset_in_bytes*).
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void write(std::size_t offset, const Memory<U> &data, bool offset_in_bytes=false)
      {
         std::size_t fixed_offset = offset;
         if (!offset_in_bytes) { fixed_offset *= sizeof(T); }
         
         if (this->ptr() == nullptr) { throw NullPointerException(); }
         if (fixed_offset >= this->_size) { throw OutOfBoundsException(fixed_offset / sizeof(T), this->elements()); }
         if (!this->aligns_with<U, data.variadic>()) { throw AlignmentException<T, U>(); }

         const auto reinterpretted = data.reinterpret<T, TIsVariadic>();
         auto this_bytes = this->_size;
         auto cast_bytes = fixed_offset + reinterpretted.byte_size();

         if (cast_bytes > this_bytes) { throw OutOfBoundsException(cast_bytes / sizeof(T), this->elements()); }

         std::memcpy(&this->ptr()[offset], reinterpretted.ptr(), reinterpretted.byte_size());
      }

      /// @brief Write a memory of items of *data* of type *T* at the given *offset*, with the option
      /// of interpretting the offset as bytes (*offset_in_bytes*).
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void write(std::size_t offset, const Memory<T> &data, bool offset_in_bytes=false) {
         this->write<T>(offset, data, offset_in_bytes);
      }

      /// @brief Write a *pointer* of the given *size* and type *U* at the given *offset*,
      /// with the option of interpretting the *size* and *offset* as bytes (*size_in_bytes*).
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U, bool UIsVariadic=false>
      void write(std::size_t offset, const U* pointer, std::size_t size, bool size_in_bytes=false) {
         auto memory = Memory<U, UIsVariadic, Allocator>(pointer, size, false, size_in_bytes);
         this->write<U>(offset, memory, size_in_bytes);
      }
      
      /// @brief Write a *pointer* of the given *size* and same type as the memory at the given *offset*,
      /// with the option of interpretting the *size* and *offset* as bytes (*size_in_bytes*).
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void write(std::size_t offset, const T* pointer, size_t size, bool size_in_bytes=false) {
         auto memory = Memory<T, TIsVariadic, Allocator>(pointer, size, false, size_in_bytes);
         this->write<T>(offset, memory, size_in_bytes);
      }

      /// @brief Write a *vector* of type *U* at the given *offset*, with the option of interpretting
      /// the offset in terms of bytes (*offset_in_bytes*).
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void write(std::size_t offset, const std::vector<U> &vector, bool offset_in_bytes=false) {
         auto vec_size = vector.size();
         if (!offset_in_bytes) { vec_size *= sizeof(U); }

         this->write<U>(offset * this->element_size(), vector.data(), vec_size, true);
      }

      /// @brief Write a *vector* of the same type at the given *offset*, with the option of interpretting
      /// the offset in terms of bytes (*offset_in_bytes*).
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void write(std::size_t offset, const std::vector<T> &vector, bool offset_in_bytes=false) {
         auto vec_size = vector.size();
         if (!offset_in_bytes) { vec_size *= sizeof(T); }

         this->write(offset * this->element_size(), vector.data(), vec_size, true);
      }

      /// @brief Write a *pointer* of type *U* at the given *offset*, with the option of
      /// interpretting the offset as bytes (*offset_in_bytes*).
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void write(std::size_t offset, const U* pointer, bool offset_in_bytes=false) {
         if (offset_in_bytes)
            this->write<U>(offset, pointer, sizeof(U), offset_in_bytes);
         else
            this->write<U>(offset, pointer, 1, offset_in_bytes);
      }

      /// @brief Write a *pointer* of the same type as the memory at the given *offset*,
      /// with the option to interpret the offset in terms of bytes (*offset_in_bytes*).
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void write(std::size_t offset, const T* pointer, bool offset_in_bytes=false) {
         if (offset_in_bytes)
            this->write<T>(offset, pointer, this->element_size(), offset_in_bytes);
         else
            this->write<T>(offset, pointer, 1, offset_in_bytes);
      }

      /// @brief Write a *reference* of type *U* at the given *offset*, with the option
      /// of interpretting the offset as bytes (*offset_in_bytes*).
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void write(std::size_t offset, const U& reference, bool offset_in_bytes=false) {
         this->write<U>(offset, &reference, offset_in_bytes);
      }

      /// @brief Write a *reference* of the same type as the memory at the given *offset*,
      /// with the option of interpretting the offset in terms of bytes (*offset_in_bytes*).
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void write(std::size_t offset, const T& reference, bool offset_in_bytes=false) {
         this->write<T>(offset, &reference, offset_in_bytes);
      }

      /// @brief Start the memory with the given memory *data* of type *U*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void start_with(const Memory<U> &data) {
         this->write<U>(0, data);
      }

      /// @brief Start the memory with the given memory *data* of the same type as the target memory.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void start_with(const Memory<T> &data) {
         this->write(0, data);
      }

      /// @brief Start the memory with the given *pointer* of type *U* of the given *size*,
      /// with the option of interpretting the size in terms of bytes (*size_in_bytes*).
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U, bool UIsVariadic=false>
      void start_with(const U* pointer, std::size_t size, bool size_in_bytes=false) {
         std::size_t byte_size = size;
         if (!size_in_bytes) { byte_size *= sizeof(U); }
         
         auto memory = Memory<U, UIsVariadic, Allocator>(pointer, byte_size, false, true);
         this->write<U>(0, memory);
      }

      /// @brief Start the memory with the given *pointer* of the same type as the memory and the given *size*,
      /// with the option of interpretting *size* in terms of bytes (*size_in_bytes*).
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void start_with(const T* pointer, std::size_t size, bool size_in_bytes=false) {
         std::size_t byte_size = size;
         if (!size_in_bytes) { byte_size *= sizeof(T); }
         
         auto memory = Memory<T, TIsVariadic, Allocator>(pointer, byte_size, false, true);
         this->write(0, memory);
      }

      /// @brief Start the memory region with the given *vector* of type *U*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void start_with(const std::vector<U> &vector) {
         this->write<U>(0, vector.data(), vector.size());
      }

      /// @brief Start the memory with the given *vector* of the same type.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void start_with(const std::vector<T> &vector) {
         this->write(0, vector.data(), vector.size());
      }

      /// @brief Start the memory with the given *pointer* of type *U*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void start_with(const U* pointer) {
         this->write<U>(0, pointer, 1);
      }

      /// @brief Start the memory with the given *pointer* of the same type as the memory.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void start_with(const T* pointer) {
         this->write(0, pointer, 1);
      }

      /// @brief Start the memory with the given *reference* of type *U*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void start_with(const U& reference) {
         this->write<U>(0, &reference);
      }

      /// @brief Start the memory with the given *reference* of the same type as the memory.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void start_with(const T& reference) {
         this->write(0, &reference);
      }

      /// @brief End the memory with the given memory *data* of type *U*.
      ///
      /// @throw OutOFBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void end_with(const Memory<U> &data)
      {
         if (!this->aligns_with<U, data.variadic>()) { throw AlignmentException<T, U>(); }
         
         auto adjusted_size = data.byte_size() / sizeof(T);
         if (adjusted_size > this->elements()) { throw OutOfBoundsException(adjusted_size, this->elements()); }

         auto offset = this->elements() - adjusted_size;
         this->write<U>(offset, data);
      }

      /// @brief End the memory with the given memory *data* of the same type as the memory.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void end_with(const Memory<T> &data) {
         this->end_with<T>(data);
      }

      /// @brief End the memory with the given *pointer* of type *U* with the given *size*, with the option
      /// to interpret the size in terms of bytes (*size_in_bytes*).
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U, bool UIsVariadic=false>
      void end_with(const U* pointer, std::size_t size, bool size_in_bytes=false) {
         std::size_t byte_size = size;
         if (!size_in_bytes) { byte_size *= sizeof(U); }
         
         auto memory = Memory<U, UIsVariadic, Allocator>(pointer, byte_size, false, true);
         this->end_with<U>(memory);
      }

      /// @brief End the memory with the given *pointer* and number of *elements* of the same type as the target memory,
      /// with the option of interpretting the size as bytes (*size_in_bytes*).
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void end_with(const T* pointer, std::size_t size, bool size_in_bytes=false) {
         std::size_t byte_size = size;
         if (!size_in_bytes) { byte_size *= sizeof(T); }
         
         auto memory = Memory<T, TIsVariadic, Allocator>(pointer, byte_size, false, true);
         this->end_with(memory);
      }

      /// @brief End the memory with the given *vector* of type *U*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void end_with(const std::vector<U> &vector) {
         this->end_with<U>(vector.data(), vector.size());
      }

      /// @brief End the memory with the given *vector* of the same type as the target memory.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void end_with(const std::vector<T> &vector) {
         this->end_with(vector.data(), vector.size());
      }
      
      /// @brief End the memory with the given *pointer* of type *U*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void end_with(const U* pointer) {
         this->end_with<U>(pointer, 1);
      }

      /// @brief End the memory with the given *pointer* of the same type as the target memory.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void end_with(const T* pointer) {
         this->end_with(pointer, 1);
      }

      /// @brief End the memory with the given *reference* of type *U*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void end_with(const U& reference) {
         this->end_with<U>(&reference);
      }

      /// @brief End the memory with the given *reference* of the same type as the target memory.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void end_with(const T& reference) {
         this->end_with<T>(&reference);
      }

      /// @brief Search for the given memory *term* of type *U* in the target memory.
      ///
      /// Returns a vector of offsets to where the search term was found.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      std::vector<std::size_t> search(const Memory<U> &term) const
      {
         if (!this->aligns_with<U, term.variadic>()) { throw AlignmentException<T, U>(); }

         const auto reinterpretted = term.reinterpret<T, TIsVariadic>();
         if (reinterpretted.size() > this->size()) { throw OutOfBoundsException(reinterpretted.size(), this->size()); }
         
         auto potential_offsets = std::vector<std::size_t>();

         for (std::size_t i=0; i<=(this->size()-reinterpretted.size()); ++i)
         {
            if (this->get(i) == reinterpretted[0])
            {
               potential_offsets.push_back(i);
            }
         }

         auto result = std::vector<std::size_t>();

         if (potential_offsets.size() == 0)
         {
            return result;
         }

         for (auto offset : potential_offsets)
         {
            bool found = true;

            /* we do this instead of std::memcmp because we want to hit potential operator== functions. */
            for (std::size_t i=1; i<reinterpretted.size(); ++i)
            {
               if (this->get(offset+i) != reinterpretted[i])
               {
                  found = false;
                  break;
               }
            }

            if (found) { result.push_back(offset); }
         }

         return result;
      }

      /// @brief Search for the given memory *term* of the same type as the target memory.
      ///
      /// Returns a vector of offsets where the search term was found.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      std::vector<std::size_t> search(const Memory<T> &term) const {
         return this->search<T>(term);
      }

      /// @brief Search for the given *pointer* of type *U* with the given *size* in the target memory,
      /// with the option of interpretting the *size* in terms of bytes (*size_in_bytes*).
      ///
      /// Returns a series of offsets that match the search term.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U, bool UIsVariadic=false>
      std::vector<std::size_t> search(const U* pointer, std::size_t size, bool size_in_bytes=false) const {
         std::size_t byte_size = size;
         if (!size_in_bytes) { byte_size *= sizeof(U); }
         
         const auto memory = Memory<U, UIsVariadic, Allocator>(pointer, byte_size, false, true);
         return this->search<U>(memory);
      }

      /// @brief Search for the given *pointer* of the same type of this memory with the given *size*,
      /// with the option of interpretting the size in terms of bytes (*size_in_bytes*).
      ///
      /// Returns a series of offsets that match the search term.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      std::vector<std::size_t> search(const T* pointer, std::size_t size, bool size_in_bytes=false) const {
         std::size_t byte_size = size;
         if (!size_in_bytes) { byte_size *= sizeof(T); }
         
         auto memory = Memory<T, TIsVariadic, Allocator>(pointer, byte_size, false, true);
         return this->search(memory);
      }

      /// @brief Search for the given vector *term* of type *U* in the target memory.
      ///
      /// Returns a series of offsets that match the search term.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      std::vector<std::size_t> search(const std::vector<U> &term) const {
         return this->search<U>(term.data(), term.size());
      }

      /// @brief Search for the given vector *term* of the same type as the target memory.
      ///
      /// Returns a series of offsets that match the search term.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      std::vector<std::size_t> search(const std::vector<T> &term) const {
         return this->search(term.data(), term.size());
      }

      /// @brief Search for the given *pointer* of type *U* in the target memory.
      ///
      /// Returns a series of offsets that match the search term.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      std::vector<std::size_t> search(const U* pointer) const {
         return this->search<U>(pointer, 1);
      }

      /// @brief Search for the given *pointer* of the same type of this memory.
      ///
      /// Returns a series of offsets that match the search term.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      std::vector<std::size_t> search(const T* pointer) const {
         return this->search(pointer, 1);
      }

      /// @brief Search for the given *reference* of type *U* in this memory.
      ///
      /// Returns a series of offsets that match the search term.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      std::vector<std::size_t> search(const U& reference) const {
         return this->search<U>(&reference);
      }

      /// @brief Search for the given *reference* of the same type as the memory.
      ///
      /// Returns a series of offsets that match the search term.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      std::vector<std::size_t> search(const T& reference) const {
         return this->search(&reference);
      }

      /// @brief Search for the given *term* of the same type as the memory with optional wildcards.
      ///
      /// Using a vector of std::optional<*T*>, we can use std::nullopt as a wildcard pattern for
      /// searching. However, unlike the other search counterparts, this can only be done against
      /// the current type: a given vector search term cannot be converted between type bases in
      /// a dynamic search due to the fact that the wildcard can't be converted between types.
      ///
      /// Returns a vector of a pair: an offset and the vector which matched the term.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      DynamicSearchResult search_dynamic(DynamicSearchTerm &term) const
      {
         if (term.size() > this->size()) { throw OutOfBoundsException(term.size(), this->size()); }
         
         std::size_t shift = 0;

         for (std::size_t i=0; i<term.size(); ++i)
         {
            if (!term[i].has_value()) { ++shift; continue; }
            
            // we found a value, so we found our shift point
            break;
         }

         if (shift == term.size()) { throw SearchTooBroadException(); }

         auto start_value = *term[shift];
         auto potential_offsets = std::vector<std::size_t>();

         for (std::size_t i=shift; i<=(this->size()-(term.size()-shift)); ++i)
         {            
            if (this->get(i) == start_value)
               potential_offsets.push_back(i);
         }

         auto result = std::vector<std::pair<std::size_t, std::vector<T>>>();

         if (potential_offsets.size() == 0)
            return result;

         for (auto offset : potential_offsets)
         {
            bool found = true;
            auto adjusted = offset - shift;

            for (std::size_t i=shift+1; i<term.size(); ++i)
            {
               if (!term[i].has_value()) { continue; }
               
               if (this->get(adjusted+i) != *term[i])
               {
                  found = false;
                  break;
               }
            }

            if (found) {
               auto data = this->read(adjusted, term.size());
               result.push_back(std::make_pair(adjusted, data));
            }
         }

         return result;
      }

      /// @brief Check if this memory contains the given memory *data* of type *U*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      bool contains(const Memory<U> &data) const {
         return this->search<U>(data).size() > 0;
      }

      /// @brief Check if this memory contains the given memory *data* of the same type of the memory.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      bool contains(const Memory<T> &data) const {
         return this->search(data).size() > 0;
      }

      /// @brief Check if the given *pointer* of type *U* with the given *size* is contained within the memory,
      /// with the option of interpretting the size in terms of bytes (*size_in_bytes*).
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U, bool UIsVariadic=false>
      bool contains(const U* pointer, std::size_t size, bool size_in_bytes=false) const {
         return this->search<U, UIsVariadic>(pointer, size, size_in_bytes).size() > 0;
      }

      /// @brief Check if the given *pointer* of the same type as the memory with the given *size* is contained within the memory,
      /// with the option of interpretting the size in terms of bytes (*size_in_bytes*).
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      bool contains(const T* pointer, std::size_t size, bool size_in_bytes=false) const {
         return this->search(pointer, size, size_in_bytes).size() > 0;
      }

      /// @brief Check if this memory contains the given *vector* of type *U*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      bool contains(const std::vector<U> &vector) const {
         return this->search<U>(vector.data(), vector.size()).size() > 0;
      }
      
      /// @brief Check if this memory contains the given *vector* of the same data type.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      bool contains(const std::vector<T> &vector) const {
         return this->search(vector.data(), vector.size()).size() > 0;
      }

      /// @brief Check if the given *pointer* of type *U* is contained within the memory.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      bool contains(const U* pointer) const {
         return this->search<U>(pointer).size() > 0;
      }

      /// @brief Check if the given *pointer* of the same type as the memory is contained within the memory.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      bool contains(const T* pointer) const {
         return this->search(pointer).size() > 0;
      }
      
      /// @brief Check if the given *reference* of type *U* is contained within the memory.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      bool contains(const U& reference) const {
         return this->search<U>(&reference).size() > 0;
      }

      /// @brief Check if the given *reference* of the same type as the memory is contained within the memory.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      bool contains(const T& reference) const {
         return this->search(&reference).size() > 0;
      }

      /// @brief Split the memory in two at the given *midpoint*, with the option of
      /// interpretting the midpoint in terms of bytes (*mid_in_bytes*).
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw NullPointerException
      ///
      std::pair<Memory<T>, Memory<T>> split_at(std::size_t midpoint, bool mid_in_bytes=false) const {
         std::size_t fixed_point = midpoint;
         if (!mid_in_bytes) { fixed_point *= sizeof(T); }
         if (midpoint > this->_size) { throw OutOfBoundsException(fixed_point / sizeof(T), this->elements()); }
         if (mid_in_bytes && !this->aligns_with(fixed_point)) { throw AlignmentException<T>(fixed_point); }

         auto left = this->subsection(0, fixed_point, true);
         auto right = this->subsection(fixed_point, this->_size-fixed_point, true);

         return std::make_pair(left, right);
      }

      /// @brief Swap two values at offsets *left* and *right*.
      ///
      /// @throw OutOfBoundsException
      /// @throw NullPointerException
      ///
      void swap(std::size_t left, std::size_t right) {
         if (left == right) { return; }

         auto& temp = this->get(left);
         this->get(left) = this->get(right);
         this->get(right) = temp;
      }

      /// @brief Reverse the order of the elements of this memory.
      ///
      /// @throw OutOfBoundsException
      /// @throw NullPointerException
      ///
      void reverse() {
         if (this->elements() == 0) { return; }

         for (std::size_t i=0; i<(this->elements()/2); ++i)
            this->swap(i, this->elements()-i-1);
      }

      /// @brief Load a *memory* object of the same type as this object into this object.
      ///
      /// In other words, reallocate space in this object and copy the Memory argument.
      ///
      /// @throw NullPointerException
      ///
      void load_data(const Memory<T> &memory) {
         this->allocate(memory.byte_size());
         this->write<T>(0, memory);
      }
      
      /// @brief Load *memory* of type *U* into this memory object.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void load_data(const Memory<U> &memory) {
         Memory<T, TIsVariadic, Allocator> reinterpretted = memory.reinterpret<T, TIsVariadic>();
         this->load_data(reinterpretted);
      }

      /// @brief Load a *pointer* of type *U* with the given *size* into the memory,
      /// with the option to interpret the size in bytes (*size_in_bytes*).
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U, bool UIsVariadic>
      void load_data(const U* pointer, std::size_t size, bool size_in_bytes=false) {
         std::size_t byte_size = size;
         if (!size_in_bytes) { byte_size *= sizeof(U); }
         
         const auto memory = Memory<U, UIsVariadic, Allocator>(pointer, byte_size, false, true);
         this->load_data<U>(memory);
      }

      /// @brief Load a *pointer* of the same type as this Memory object with the given *size* into the object,
      /// with the option to interpret the size in bytes (*size_in_bytes*).
      ///
      /// @throw NullPointerException
      ///
      void load_data(const T* pointer, std::size_t size, bool size_in_bytes=false) {
         std::size_t byte_size = size;
         if (!size_in_bytes) { byte_size *= this->element_size(); }
         
         const auto memory = Memory<T, TIsVariadic, Allocator>(pointer, byte_size, false, true);
         this->load_data(memory);
      }

      /// @brief Load a *vector* of type *U* into this memory object.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void load_data(const std::vector<U> &data) {
         this->load_data<U>(data.data(), data.size());
      }

      /// @brief Load a *vector* of the same type as this object into this object.
      ///
      /// @throw NullPointerException
      ///
      void load_data(const std::vector<T> &data) {
         this->load_data(data.data(), data.size());
      }

      /// @brief Load a *pointer* of type *U* into this memory.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void load_data(const U* pointer) {
         this->load_data<U>(pointer, 1);
      }

      /// @brief Load a *pointer* of the same type as this object into the object.
      ///
      /// @throw NullPointerException
      ///
      void load_data(const T* pointer) {
         this->load_data(pointer, 1);
      }

      /// @brief Load a *reference* of type *U* into this memory.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void load_data(const U& reference) {
         this->load_data<U>(&reference);
      }

      /// @brief Load a *reference* of the same type as this memory into the memory.
      ///
      /// @throw NullPointerException
      ///
      void load_data(const T& reference) {
         this->load_data(&reference);
      }

      /// @brief Load a *filename*'s data into the memory.
      ///
      /// @throw std::ios_base::failure
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void load_file(const std::string &filename) {
         std::ifstream fp(filename, std::ios::binary);
         if (!fp.is_open()) { throw OpenFileFailureException(filename); }
         
         std::streampos filesize;

         fp.seekg(0, std::ios::end);
         filesize = fp.tellg();
         fp.seekg(0, std::ios::beg);

         auto u8_data = std::vector<std::uint8_t>(filesize);
         u8_data.insert(u8_data.begin(),
                        std::istreambuf_iterator<char>(fp),
                        std::istreambuf_iterator<char>());

         fp.close();

         auto u8_memory = Memory<std::uint8_t>(u8_data.data(), u8_data.size());
         auto reinterpretted = u8_memory.reinterpret<T, TIsVariadic>();

         this->allocate(reinterpretted.byte_size());
         this->write(0, reinterpretted);
      }

      /// @brief Resize the underlying vector within the memory object with the given *size* and
      /// optional padding-value *value*, with the option to interpret the size in terms of bytes
      /// (*size_in_bytes*).
      ///
      void resize(std::size_t size, bool size_in_bytes=false, std::optional<T> value = std::nullopt) {
         this->throw_if_unallocated();
         this->reallocate(size, size_in_bytes, value);
      }

      /// @brief Append the given *memory* of type *U* to the end of the memory.
      ///
      /// This resizes the memory to accomodate the new data and then appends it to the end of the memory.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void append(const Memory<U> &memory) {
         this->throw_if_unallocated();
         
         const auto reinterpretted = memory.reinterpret<T, TIsVariadic>();
         this->resize(this->_size + reinterpretted.byte_size(), true);
         this->write<T>(this->_size - reinterpretted.byte_size(), reinterpretted, true);
      }

      /// @brief Append the given *memory* of the same type as the memory to the end of the memory.
      ///
      /// This resizes the memory to accomodate the new data and then appends it to the end of the memory.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void append(const Memory<T> &memory) {
         this->append<T>(memory);
      }

      /// @brief Append the given *pointer* of type *U* with the given *size* to the end of the memory,
      /// with the option to interpret the size in terms of bytes (*size_in_bytes*).
      ///
      /// This resizes the memory to accomodate the new data and then appends it to the end of the memory.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      template <typename U, bool UIsVariadic=false>
      void append(const U* pointer, std::size_t size, bool size_in_bytes=false) {
         auto byte_size = size;
         if (!size_in_bytes) { byte_size *= sizeof(U); }
         
         const auto memory = Memory<U, UIsVariadic, Allocator>(pointer, byte_size, false, true);
         this->append<U>(memory);
      }

      /// @brief Append the given *pointer* of the same type as the memory with the given *size*
      /// to the end of the memory, with the option of interpretting the *size* in terms of bytes
      /// (*size_in_bytes*).
      ///
      /// This resizes the memory to accomodate the new data and then appends it to the end of the memory.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void append(const T* pointer, std::size_t size, bool size_in_bytes=false) {
         auto byte_size = size;
         if (!size_in_bytes) { byte_size *= this->element_size(); }
         
         const auto memory = Memory<T, TIsVariadic, Allocator>(pointer, byte_size, false, true);
         this->append<T>(memory);
      }

      /// @brief Append the given *vector* of type *U* to the end of the memory.
      ///
      /// This resizes the memory to accomodate the new data and then appends it to the end of the memory.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void append(const std::vector<U> &vector) {
         this->append<U>(vector.data(), vector.size());
      }

      /// @brief Append the given *vector* of the same type as the memory to the end of the memory.
      ///
      /// This resizes the memory to accomodate the new data and then appends it to the end of the memory.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void append(const std::vector<T> &vector) {
         this->append(vector.data(), vector.size());
      }
      
      /// @brief Append the given *pointer* of type *U* to the end of the memory.
      ///
      /// This resizes the memory to accomodate the new data and then appends it to the end of the memory.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void append(const U* pointer) {
         this->append<U>(pointer, 1);
      }

      /// @brief Append the given *pointer* of the same type as the memory to the end of the memory.
      ///
      /// This resizes the memory to accomodate the new data and then appends it to the end of the memory.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void append(const T* pointer) {
         this->append(pointer, 1);
      }
      
      /// @brief Append the given *reference* of type *U* to the end of the memory.
      ///
      /// This resizes the memory to accomodate the new data and then appends it to the end of the memory.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullReferenceException
      ///
      template <typename U>
      void append(const U& reference) {
         this->append<U>(&reference);
      }

      /// @brief Append the given *reference* of the same type as the memory to the end of the memory.
      ///
      /// This resizes the memory to accomodate the new data and then appends it to the end of the memory.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullReferenceException
      ///
      void append(const T& reference) {
         this->append(&reference);
      }

      /// @brief Insert the given *memory* of type *U* at the given *offset* in the memory,
      /// with the option to interpret the offset in terms of bytes (*offset_in_bytes*).
      ///
      /// This expands the underlying vector and injects the data at the given offset.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void insert(std::size_t offset, const Memory<U> &memory, bool offset_in_bytes=false) {
         this->throw_if_unallocated();

         auto fixed_offset = offset;
         if (!offset_in_bytes) { fixed_offset *= this->element_size(); }
         
         if (fixed_offset > this->_size) { return OutOfBoundsException(fixed_offset / this->element_size(), this->elements()); }

         const auto reinterpretted = memory.reinterpret<T, TIsVariadic>();
         auto split_point = this->split_at(offset);
         auto split_data = split_point.second.to_vec();

         this->reallocate(this->_size + reinterpretted.byte_size(), true);
         this->write(fixed_offset, reinterpretted, true);
         this->write(fixed_offset + reinterpretted.byte_size(), split_data.data(), split_data.size() * sizeof(T), true);
      }

      /// @brief Insert the given *memory* of the same type as the object at
      /// the given *offset* in the memory, with the option of interpretting the offset
      /// in terms of bytes (*offset_in_bytes*).
      ///
      /// This expands the underlying vector and injects the data at the given offset.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void insert(std::size_t offset, const Memory<T> &memory, bool offset_in_bytes=false) {
         this->insert<T>(offset, memory, offset_in_bytes);
      }

      /// @brief Insert the given *pointer* of type *U* with the given *size*
      /// at the given *offset* in the memory, with the option to interpret the size and
      /// offset as bytes (*size_in_bytes*).
      ///
      /// This expands the underlying vector and injects the data at the given offset.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U, bool UIsVariadic>
      void insert(std::size_t offset, const U* pointer, std::size_t size, bool size_in_bytes=false) {
         const auto memory = Memory<U, UIsVariadic, Allocator>(pointer, size, false, size_in_bytes);
         this->insert<U>(offset, memory, size_in_bytes);
      }

      /// @brief Insert the given *pointer* of the same type as the memory with the given *size*
      /// at the given *offset* in the memory, with the option to interpret the size and offset
      /// in terms of bytes (*size_in_bytes*).
      ///
      /// This expands the underlying vector and injects the data at the given offset.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void insert(std::size_t offset, const T* pointer, std::size_t size, bool size_in_bytes=false) {
         const auto memory = Memory<T, TIsVariadic, Allocator>(pointer, size, false, size_in_bytes);
         this->insert(offset, memory, size_in_bytes);
      }

      /// @brief Insert the given *vector* of type *U* at the given *offset*, with the option
      /// of interpretting the offset in terms of bytes (*offset_in_bytes*).
      ///
      /// This expands the underlying vector and injects the data at the given offset.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void insert(std::size_t offset, const std::vector<U> &vector, bool offset_in_bytes=false) {
         auto byte_size = vector.size();
         if (!offset_in_bytes) { byte_size *= sizeof(U); }

         auto fixed_offset = offset;
         if (!offset_in_bytes) { fixed_offset *= this->element_size(); }
         
         this->insert<U>(fixed_offset, vector.data(), byte_size, true);
      }
      
      /// @brief Insert the given *vector* of the same type as the memory at the given *offset*,
      /// with the option of interpretting the offset in terms of bytes (*offset_in_bytes*).
      ///
      /// This expands the underlying vector and injects the data at the given offset.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void insert(std::size_t offset, const std::vector<T> &vector, bool offset_in_bytes=false) {
         auto byte_size = vector.size();
         if (!offset_in_bytes) { byte_size *= this->element_size(); }

         auto fixed_offset = offset;
         if (!offset_in_bytes) { fixed_offset *= this->element_size(); }
         
         this->insert(fixed_offset, vector.data(), byte_size, true);
      }

      /// @brief Insert the given *pointer* of type *U* at the given *offset* into the memory,
      /// with the option to interpret the offset in terms of bytes (*offset_in_bytes*).
      ///
      /// This expands the underlying vector and injects the data at the given offset.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void insert(std::size_t offset, const U* pointer, bool offset_in_bytes=false) {
         auto fixed_offset = offset;
         if (!offset_in_bytes) { fixed_offset *= sizeof(U); }
         
         this->insert<U>(fixed_offset, pointer, sizeof(U), true);
      }

      /// @brief Insert the given *pointer* of the same type as the memory at the given *offset*
      /// into the memory, with the option of interpretting the offset in terms of bytes
      /// (*offset_in_bytes*).
      ///
      /// This expands the underlying vector and injects the data at the given offset.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void insert(std::size_t offset, const T* pointer, bool offset_in_bytes=false) {
         auto fixed_offset = offset;
         if (!offset_in_bytes) { fixed_offset *= this->element_size(); }
         
         this->insert(fixed_offset, pointer, this->element_size(), true);
      }
      
      /// @brief Insert the given *reference* of type *U* at the given *offset* into the memory,
      /// with the option of interpretting the offset in terms of bytes (*offset_in_bytes*).
      ///
      /// This expands the underlying vector and injects the data at the given offset.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullReferenceException
      ///
      template <typename U>
      void insert(std::size_t offset, const U& reference, bool offset_in_bytes=false) {
         this->insert<U>(offset, &reference, offset_in_bytes);
      }

      /// @brief Insert the given *reference* of the same type as the memory at the given *offset* into the memory,
      /// with the option of interpretting the offset in terms of bytes (*offset_in_bytes*).
      ///
      /// This expands the underlying vector and injects the data at the given offset.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullReferenceException
      ///
      void insert(std::size_t offset, const T& reference, bool offset_in_bytes=false) {
         this->insert(offset, &reference, offset_in_bytes);
      }

      /// @brief Remove (or "erase") the range of elements, specified by the *start* offset and the *end* offset,
      /// with the option to interpret the *start* and *end* as bytes (*offset_in_bytes*).
      ///
      /// @throw OutOfBoundsException
      /// @throw NotAllocatedException
      ///
      void erase(std::size_t start, std::size_t end, bool offset_in_bytes=false) {
         this->throw_if_unallocated();
         
         auto fixed_start = start;
         if (!offset_in_bytes) { fixed_start *= this->element_size(); }

         auto fixed_end = end;
         if (!offset_in_bytes) { fixed_end *= this->element_size(); }
         
         if (fixed_start == 0 && fixed_end == this->_size) {
            this->deallocate();
            return;
         }
         
         if (fixed_end > this->_size) { throw OutOfBoundsException(fixed_end / this->element_size(), this->elements()); }

         std::vector<T> end_data;
         
         if (fixed_end != this->_size)
            end_data = this->subsection(fixed_end, this->_size-fixed_end, true).to_vec();

         auto size_delta = this->_size - (fixed_end - fixed_start);
         this->reallocate(size_delta);

         if (end_data.size() > 0)
            this->write(fixed_start, end_data.data(), end_data.size() * this->element_size(), true);
      }
      
      /// @brief Remove (or "erase") the element at the given *offset*, shrinking the backing vector
      /// by one element. The offset can be interpretted in terms of bytes with the *offset_in_bytes* flag.
      ///
      /// @throw OutOfBoundsException
      ///
      void erase(std::size_t offset, bool offset_in_bytes=false) {
         auto fixed_offset = offset;
         if (!offset_in_bytes) { fixed_offset *= this->element_size(); }
         
         this->erase(fixed_offset, fixed_offset + this->element_size(), true);
      }

      /// @brief Push a *value* of type *T* onto the end of the stack.
      ///
      /// This essentially does the same operation as *Memory::append*, but only for the type of the memory.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullReferenceException
      ///
      void push(const T& reference) {
         this->append(&reference);
      }

      /// @brief Pop an element off the end of the memory, if one exists.
      ///
      /// @throw NotAllocatedException
      ///
      std::optional<T> pop() {
         if (this->elements() == 0) { return std::nullopt; }

         T value = this->get(this->elements()-1); // intentionally convert this reference to a value
         this->resize(this->elements()-1);

         return value;
      }

      /// @brief Clear this memory of any data.
      ///
      /// @throw NotAllocatedException
      ///
      void clear() {
         this->throw_if_unallocated();
         this->deallocate();
      }

      /// @brief Split this memory in two at the given *midpoint*, returning the data memory that was split.
      ///
      /// There is no const equivalent because it resizes the underlying memory. For similar functionality that
      /// contains const structures, see *Memory::split_at*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw NullPointerException
      /// @throw NotAllocatedException
      ///
      Memory<T> split_off(std::size_t midpoint) {
         this->throw_if_unallocated();
         
         auto split_pair = this->split_at(midpoint);
         auto split_memory = Memory<T, TIsVariadic, Allocator>(split_pair.second.ptr(), split_pair.second.size(), true);
         this->resize(midpoint);

         return split_memory;
      }

      std::string to_hex(bool uppercase=false) const {
         std::stringstream stream;
         stream << std::hex << std::setw(2) << std::setfill('0');

         if (uppercase) { stream << std::uppercase; }

         auto reinterpretted = this->reinterpret<std::uint8_t>();

         for (auto byte : reinterpretted)
            stream << reinterpret_cast<unsigned int>(byte);

         return stream.str();
      }
   };
}
