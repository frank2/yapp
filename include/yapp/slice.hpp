//! @file slice.hpp
//! @brief Core array-like objects containing a pointer/size pairs for somewhat safer access of data.
//!
//! The core design of these objects is based on some of Rust's concepts of array safety when
//! handling pointers (which we have to do a lot when it comes to PE files). A pointer on its own
//! can be relatively unsafe; a pointer with some sort of size bound is much safer (though not fully safe).
//!
//! Slice objects, on their own, aren't intended to be a long-term solution to memory access-- 
//! they're intended to be used to either build other primitives on top or to temporarily
//! access some memory. On their own, they are not responsible for tracking whether or not the pointers
//! are valid, and are thus dangerous to use without some awareness of the scope of the pointers
//! they're using.
//!
//! Remember: the difference between a *Slice* and a *Buffer* is that the buffer *owns the data.* It is
//! not pointing at some arbitrary location like a slice because the data its pointing to is a vector that
//! the buffer object contains.
//!
//! ## Alignment
//!
//! One important thing to be aware of is *alignment of data*. This is key to converting between
//! slice types. For example, you can convert a 1-byte sized typed slice (e.g., std::uint8_t) into a slice
//! of 8-byte sized type (e.g., std::uint64_t, so long as you have a multiple of eight bytes in the slice)
//! because 8 % 1 == 0, but you can't convert a 6-byte size typed slice between a 4-byte size typed slice
//! because 6 % 4 == 2. In other words, when the modulus result is 0 (i.e., there is no remainder when
//! divinding between the two type sizes), the two types can convert cleanly between each other.
//!

#pragma once

#include <cstring>
#include <fstream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <yapp/exception.hpp>

namespace yapp
{
   /// @brief A slice of memory, containing a pointer/size pair, modelled after Rust's slice object.
   ///
   /// A slice of memory, denoted by a pointer/size pair. Just like with Rust, this is a typically
   /// unsafe primitive, so be careful how you use it!
   ///
   template <typename T>
   class Slice
   {
   public:
      /// @brief The type used for dynamic search terms.
      ///
      using DynamicSearchTerm = std::vector<std::optional<T>>;

      /// @brief The type used for returning dynamic search results.
      using DynamicSearchResult = std::vector<std::pair<std::size_t, std::vector<T>>>;
      
      /// @brief A forward iterator for a slice object.
      ///
      class Iterator
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

         friend bool operator== (const Iterator& a, const Iterator& b) { return a.ptr == b.ptr }
         friend bool operator!= (const Iterator& a, const Iterator& b) { return a.ptr != b.ptr }

      private:
         T* ptr;
      };
         
   protected:
      union {
         const T* c;
         T* m;
      } pointer;
      
      /// @brief The number of elements corresponding to this slice.
      ///
      std::size_t elements;

   public:
      Slice() : elements(0) { this->pointer.c = nullptr; }
      /// @brief Construct a *Slice* object from a given *pointer* and *size*.
      ///
      /// @throw NullPointerException
      ///
      Slice(T *pointer, std::size_t elements) : elements(elements) {
         this->pointer.m = pointer;
      }
      /// @brief Construct a *Slice* object from a given const *pointer* and *size*.
      ///
      /// ## Undefined behavior
      ///
      /// This constructor has the potential to cause undefined behavior if you use non-const functionality with it.
      /// Only use this constructor if you intend to create a `const Slice<T>` object.
      ///
      /// @throw NullPointerException
      ///
      Slice(const T *pointer, std::size_t elements) : elements(elements) {
         this->pointer.c = pointer;
      }
      Slice(const Slice &other) : pointer(other.pointer), elements(other.elements) {}

      /// @brief Syntactic sugar to make slice objects more like arrays.
      ///
      /// @throw OutOfBoundsException
      /// @throw NullPointerException
      ///
      inline T& operator[](std::size_t index) {
         return this->get(index);
      }

      /// @brief Syntactic sugar to make slice objects more like arrays.
      ///
      /// @throw OutOfBoundsException
      /// @throw NullPointerException
      ///
      inline const T& operator[](std::size_t index) const {
         return this->get(index);
      }

      /// @brief Get the element at the given *index* in the slice.
      ///
      /// @throw OutOfBoundsException
      /// @throw NullPointerException
      ///
      inline T& get(std::size_t index) {
         if (this->ptr() == nullptr) { throw NullPointerException(); }
         if (index >= this->elements) { throw OutOfBoundsException(index, this->elements); }

         return this->ptr()[index];
      }

      /// @brief Get the const element at the given *index* in the slice.
      ///
      /// @throw OutOfBoundsException
      /// @throw NullPointerException
      ///
      inline const T& get(std::size_t index) const {
         if (this->ptr() == nullptr) { throw NullPointerException(); }
         if (index >= this->elements) { throw OutOfBoundsException(index, this->elements); }

         return this->ptr()[index];
      }

      /// @brief Get the first element in this slice.
      ///
      /// @throw OutOfBoundsException
      /// @throw NullPointerException
      ///
      inline T& front() {
         return this->get(0);
      }
      
      /// @brief Get the first element in this slice.
      ///
      /// @throw OutOfBoundsException
      /// @throw NullPointerException
      ///
      inline const T& front() const {
         return this->get(0);
      }

      
      /// @brief Get the last element in this slice.
      ///
      /// @throw OutOfBoundsException
      /// @throw NullPointerException
      ///
      inline T& back() {
         if (this->elements == 0) { throw OutOfBoundsException(0, this->elements); }
         
         return this->get(this->elements-1);
      }
      
      /// @brief Get the first element in this slice.
      ///
      /// @throw OutOfBoundsException
      /// @throw NullPointerException
      ///
      inline const T& back() const {
         if (this->elements == 0) { throw OutOfBoundsException(0, this->elements); }
         
         return this->get(this->elements-1);
      }

      /// @brief Check whether this slice is empty.
      ///
      inline bool empty() { return this->elements == 0; }

      /// @brief Get a forward iterator to the beginning of this slice.
      ///
      /// @throw NullPointerException
      ///
      inline Iterator begin() {
         if (this->ptr() == nullptr) { throw NullPointerException(); }
         
         return Iterator(&this->ptr()[0]);
      }

      /// @brief Get the end iterator of this slice.
      ///
      /// To return the end of the slice as a pointer, see *Slice::eob*.
      ///
      /// @throw NullPointerException
      ///
      inline Iterator end() { return Iterator(this->eob()); }

      /// @brief Get the end pointer of this slice, or "end of buffer."
      ///
      /// To return the end iterator of the slice, see *Slice::end*.
      ///
      /// @throw NullPointerexception
      ///
      inline T* eob() {
         if (this->ptr() == nullptr) { throw NullPointerException(); }
         
         return &this->ptr()[this->elements];
      }

      /// @brief Get the const end pointer of this slice, or "end of buffer."
      ///
      /// To return the end iterator of the slice, see *Slice::end*.
      ///
      /// @throw NullPointerexception
      ///
      inline const T* eob() const {
         if (this->ptr() == nullptr) { throw NullPointerException(); }
         
         return &this->ptr()[this->elements];
      }

      /// @brief Get the base pointer of this slice.
      ///
      inline T* ptr(void) { return this->pointer.m; }
      
      /// @brief Get the const base pointer of this slice.
      ///
      inline const T* ptr(void) const { return this->pointer.c; }

      /// @brief Get the length of this slice.
      ///
      inline std::size_t size(void) const { return this->elements; }

      /// @brief Get the size of an element in this slice.
      ///
      inline std::size_t element_size(void) const { return sizeof(T); }

      /// @brief Get the amount of units of type *U* needed to fill the type of this slice.
      ///
      /// This returns 0 if the given type *U* is larger than the slice type *T*.
      ///
      template <typename U>
      inline std::size_t elements_needed() const { return sizeof(T) / sizeof(U); }

      /// @brief Check if this slice aligns with the given size boundary.
      ///
      inline bool aligns_with(std::size_t size) const {
         std::size_t smaller = (this->element_size() < size) ? this->element_size() : size;
         std::size_t bigger = (this->element_size() > size) ? this->element_size() : size;

         return (bigger % smaller == 0);
      }

      /// @brief Check if the given type argument aligns with the slice's element boundaries.
      ///
      template <typename U>
      inline bool aligns_with() const { return this->aligns_with(sizeof(U)); }

      /// @brief Get a pointer into this slice of the given typename *U* at the given *offset*.
      ///
      /// Note that the *offset* argument is relative to the size of the type of the slice, just like
      /// an array offset. The argument type must align with the slice type. This is easy for byte slices,
      /// slightly more complicated for other slices.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw NullPointerException
      ///
      template <typename U>
      U* cast_ptr(std::size_t offset)
      {
         if (offset >= this->elements) { throw OutOfBoundsException(offset, this->elements); }
         if (!this->aligns_with<U>()) { throw AlignmentException<T, U>(); }

         auto this_bytes = this->elements * sizeof(T);
         auto cast_bytes = offset * sizeof(T) + sizeof(U);

         if (cast_bytes > this_bytes) { throw OutOfBoundsException(cast_bytes / sizeof(T), this->elements); }

         return reinterpret_cast<U*>(&this->get(offset));
      }

      /// @brief Get a pointer into this slice of the given typename *U* at the given *offset*.
      ///
      /// Note that the *offset* argument is relative to the size of the type of the slice, just like
      /// an array offset. The argument type must align with the slice type. This is easy for byte slices,
      /// slightly more complicated for other slices.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw NullPointerException
      ///
      template <typename U>
      const U* cast_ptr(std::size_t offset) const
      {
         if (offset >= this->elements) { throw OutOfBoundsException(offset, this->elements); }
         if (!this->aligns_with<U>()) { throw AlignmentException<T, U>(); }

         auto this_bytes = this->elements * sizeof(T);
         auto cast_bytes = offset * sizeof(T) + sizeof(U);

         if (cast_bytes > this_bytes) { throw OutOfBoundsException(cast_bytes / sizeof(T), this->elements); }

         return reinterpret_cast<U*>(&this->get(offset));
      }

      /// @brief Get a reference into this slice of the given typename *U* at the given *offset*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw NullPointerException
      ///
      template <typename U>
      U& cast_ref(std::size_t offset)
      {
         return *this->cast_ptr<U>(offset);
      }

      /// @brief Get a const reference into this slice of the given typename *U* at the given *offset*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw NullPointerException
      ///
      template <typename U>
      const U& cast_ref(std::size_t offset) const
      {
         return *this->cast_ptr<U>(offset);
      }

      /// @brief Return the slice as a vector of bytes.
      ///
      /// Useful for when you just want the raw bytes of the slice, rather than the individual interpretted elements.
      ///
      /// @throw NullPointerException
      ///
      std::vector<std::uint8_t> as_bytes(void) const {
         if (this->ptr() == nullptr) { throw NullPointerException(); }
         
         auto type_size = sizeof(T);
         auto start = reinterpret_cast<std::uint8_t *>(this->ptr());
         auto end = &start[this->elements * type_size];

         return std::vector<std::uint8_t>(start, end);
      }

      /// @brief Validate that the given *pointer* is within range of this buffer.
      ///
      template <typename U>
      bool validate_range(const U* pointer) const
      {
         if (this->ptr() == nullptr) { return false; }
         
         auto me = reinterpret_cast<std::uintptr_t>(pointer);
         auto start = reinterpret_cast<std::uintptr_t>(this->ptr());
         auto end = reinterpret_cast<std::uintptr_t>(this->eob());

         return (me >= start && me < end);
      }
      
      /// @brief Validate that the given *pointer* is aligned to an element boundary of the slice.
      ///
      template <typename U>
      bool validate_alignment(const U* pointer) const {
         if (this->ptr() == nullptr) { return false; }
         
         auto me = reinterpret_cast<std::uintptr_t>(pointer);
         auto start = reinterpret_cast<std::uintptr_t>(this->ptr());
         auto alignment = (me - start) % sizeof(T)

         return (alignment == 0);
      }

      /// @brief Validate with both Slice::validate_range and Slice::validate_alignment that the given *pointer* is
      /// valid in this buffer.
      ///
      template <typename U>
      bool validate_ptr(const U* pointer) const { return (this->validate_range(pointer) && this->validate_alignment(pointer)); }

      /// @brief Convert this slice object into a vector.
      ///
      /// @throw NullPointerException
      ///
      std::vector<T> to_vec(void) const {
         if (this->ptr() == nullptr) { throw NullPointerException(); }
         
         return std::vector<T>(this->ptr(), this->eob());
      }

      /// @brief Save this slice to disk.
      ///
      /// @throw std::ios_base::failure
      /// @throw NullPointerException
      ///
      void save(const std::string &filename) const {
         if (this->ptr() == nullptr) { throw NullPointerException(); }
         
         std::ofstream fp(filename);
         auto bytes = reinterpret_cast<const char *>(this->ptr());
         auto fixed_size = this->elements * sizeof(T);
         fp.write(bytes, fixed_size);
         fp.close();
      }

      /// @brief Create a const subslice of this slice at the given *offset* with the given type *U* and *size*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw NullPointerException
      ///
      template <typename U>
      const Slice<U> subslice(std::size_t offset, std::size_t size) const
      {
         if (this->ptr() == nullptr) { throw NullPointerException(); }
         if (offset >= this->elements) { throw OutOfBoundsException(offset, this->elements); }
         if (!this->aligns_with<U>()) { throw AlignmentException<T, U>(); }

         auto this_bytes = this->elements * sizeof(T);
         auto cast_bytes = offset * sizeof(T) + sizeof(U) * size;

         if (cast_bytes > this_bytes) { throw OutOfBoundsException(cast_bytes / sizeof(T), this->elements); }

         return Slice<U>(reinterpret_cast<const U*>(&this->ptr()[offset]), size);
      }

      /// @brief Create a subslice of this slice at the given *offset* and *size*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw NullPointerException
      ///
      const Slice<T> subslice(std::size_t offset, std::size_t size) const {
         return this->subslice<T>(offset, size);
      }
      
      /// @brief Create a subslice of this slice at the given *offset* with the given type *U* and *size*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw NullPointerException
      ///
      template <typename U>
      Slice<U> subslice(std::size_t offset, std::size_t size)
      {
         if (this->ptr() == nullptr) { throw NullPointerException(); }
         if (offset >= this->elements) { throw OutOfBoundsException(offset, this->elements); }
         if (!this->aligns_with<U>()) { throw AlignmentException<T, U>(); }

         auto this_bytes = this->elements * sizeof(T);
         auto cast_bytes = offset * sizeof(T) + sizeof(U) * size;

         if (cast_bytes > this_bytes) { throw OutOfBoundsException(cast_bytes / sizeof(T), this->elements); }

         return Slice<U>(reinterpret_cast<U*>(&this->ptr()[offset]), size);
      }

      /// @brief Create a subslice of this slice at the given *offset* and *size*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw NullPointerException
      ///
      Slice<T> subslice(std::size_t offset, std::size_t size) {
         return this->subslice<T>(offset, size);
      }

      /// @brief Reinterpret the slice as the given type *U*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      Slice<U> reinterpret() {
         if (!this->aligns_with<U>()) { throw AlignmentException<T, U>(); }

         auto needed = this->elements_needed<U>();
         auto adjusted_size = (this->size() * sizeof(T)) / sizeof(U);

         if (needed > 0 && adjusted_size % needed != 0) { throw InsufficientDataException<T, U>(this->read<U>(0, adjusted_size)); }

         return this->subslice<U>(0, adjusted_size);
      }

      /// @brief Reinterpret the slice as the given const type *U*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      const Slice<U> reinterpret() const {
         if (!this->aligns_with<U>()) { throw AlignmentException<T, U>(); }

         auto needed = this->elements_needed<U>();
         auto adjusted_size = (this->size() * sizeof(T)) / sizeof(U);

         if (needed > 0 && adjusted_size % needed != 0) { throw InsufficientDataException<T, U>(this->read<U>(0, adjusted_size)); }

         return this->subslice<U>(0, adjusted_size);
      }

      /// @brief Read an arbitrary amount of *U* values from the given *offset* into a vector of the given *size*.
      ///
      /// Returns a vector of *U* items. If you want to create a slice object, use Slice::subslice.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw NullPointerException
      ///
      template <typename U>
      std::vector<U> read(std::size_t offset, std::size_t size) const
      {
         if (this->ptr() == nullptr) { throw NullPointerException(); }
         if (offset >= this->elements) { throw OutOfBoundsException(offset, this->elements); }
         if (!this->aligns_with<U>()) { throw AlignmentException<T, U>(); }

         auto this_bytes = this->elements * sizeof(T);
         auto cast_bytes = offset * sizeof(T) + sizeof(U) * size;

         if (cast_bytes > this_bytes) { throw OutOfBoundsException(cast_bytes / sizeof(T), this->elements); }

         return std::vector<U>(reinterpret_cast<const U*>(&this->ptr()[offset]),
                               reinterpret_cast<const U*>(&this->ptr()[cast_bytes / sizeof(T)]));
      }

      /// @brief Read an arbitrary amount of values from the buffer at the given *offset* with the given *size*.
      ///
      /// Returns a vector of *T* of the given elements.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw NullPointerException
      ///
      std::vector<T> read(std::size_t offset, std::size_t size) const {
         return this->read<T>(offset, size);
      }

      /// @brief Write a slice of *U* items in *data* to the given slice object at the given *offset*.
      ///
      /// Remember that the offset is relative to the size of the type of the slice! Just like array offsets.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void write(std::size_t offset, const Slice<U> &data)
      {
         if (this->ptr() == nullptr) { throw NullPointerException(); }
         if (offset >= this->elements) { throw OutOfBoundsException(offset, this->elements); }
         if (!this->aligns_with<U>()) { throw AlignmentException<T, U>(); }

         const auto reinterpretted = data.reinterpret<T>();
         auto this_bytes = this->elements * sizeof(T);
         auto cast_bytes = offset * sizeof(T) + reinterpretted.size() * sizeof(T);

         if (cast_bytes > this_bytes) { throw OutOfBoundsException(cast_bytes / sizeof(T), this->elements); }

         std::memcpy(&this->ptr()[offset], reinterpretted.ptr(), reinterpretted.size() * sizeof(T));
      }

      /// @brief Write a slice of items of *data* of type *T* at the given *offset*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void write(std::size_t offset, const Slice<T> &data) {
         this->write<T>(offset, data);
      }

      /// @brief Write a *pointer* of the given number of *elements* and type *U* at the given *offset*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void write(std::size_t offset, const U* pointer, std::size_t elements) {
         auto slice = Slice<U>(pointer, elements);
         this->write<U>(offset, slice);
      }
      
      /// @brief Write a *pointer* of the given number of *elements* and same type as the slice at the given *offset*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void write(std::size_t offset, const T* pointer, size_t elements) {
         auto slice = Slice<T>(pointer, elements);
         this->write<T>(offset, slice);
      }

      /// @brief Write a *vector* of type *U* at the given *offset*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void write(std::size_t offset, const std::vector<U> &vector) {
         this->write<U>(offset, vector.data(), vector.size());
      }

      /// @brief Write a *vector* of the same type at the given *offset*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void write(std::size_t offset, const std::vector<T> &vector) {
         this->write<T>(offset, vector.data(), vector.size());
      }

      /// @brief Write a *pointer* of type *U* at the given *offset*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void write(std::size_t offset, const U* pointer) {
         this->write<U>(offset, pointer, 1);
      }

      /// @brief Write a *pointer* of the same type as the slice at the given *offset*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void write(std::size_t offset, const T* pointer) {
         this->write<T>(offset, pointer, 1);
      }

      /// @brief Write a *reference* of type *U* at the given *offset*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void write(std::size_t offset, const U& reference) {
         this->write<U>(offset, &reference);
      }

      /// @brief Write a *reference* of the same type as the slice at the given *offset*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void write(std::size_t offset, const T& reference) {
         this->write<T>(offset, &reference);
      }

      /// @brief Start the slice with the given slice *data* of type *U*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void start_with(const Slice<U> &data) {
         this->write<U>(0, data);
      }

      /// @brief Start the slice with the given slice *data* of the same type as the target slice.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void start_with(const Slice<T> &data) {
         this->write(0, data);
      }

      /// @brief Start the slice with the given *pointer* of type *U* of the given number of *elements*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void start_with(const U* pointer, std::size_t elements) {
         auto slice = Slice<U>(pointer, elements);
         this->write<U>(0, slice);
      }

      /// @brief Start the slice with the given *pointer* of the same type as the slice and the given number of *elements*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void start_with(const T* pointer, std::size_t elements) {
         auto slice = Slice<T>(pointer, elements);
         this->write(0, slice);
      }

      /// @brief Start the slice with the given *vector* of type *U*.
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

      /// @brief Start the slice with the given *vector* of the same type.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void start_with(const std::vector<T> &vector) {
         this->write(0, vector.data(), vector.size());
      }

      /// @brief Start the slice with the given *pointer* of type *U*.
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

      /// @brief Start the slice with the given *pointer* of the same type as the slice.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void start_with(const T* pointer) {
         this->write(0, pointer, 1);
      }

      /// @brief Start the slice with the given *reference* of type *U*.
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

      /// @brief Start the slice with the given *reference* of the same type as the slice.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void start_with(const T& reference) {
         this->write(0, &reference);
      }

      /// @brief End the slice with the given slice *data* of type *U*.
      ///
      /// @throw OutOFBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void end_with(const Slice<U> &data)
      {
         if (!this->aligns_with<U>()) { throw AlignmentException<T, U>(); }
         
         auto adjusted_size = (data.size() * sizeof(U)) / sizeof(T);
         if (adjusted_size > this->elements) { throw OutOfBoundsException(adjusted_size, this->elements); }

         auto offset = this->elements - adjusted_size;
         this->write<U>(offset, data);
      }

      /// @brief End the slice with the given slice *data* of the same type as the slice.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void end_with(const Slice<T> &data) {
         this->end_with<T>(data);
      }

      /// @brief End the slice with the given *pointer* of type *U* with the given number of *elements*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void end_with(const U* pointer, std::size_t elements) {
         auto slice = Slice<U>(pointer, elements);
         this->end_with<U>(slice);
      }

      /// @brief End the slice with the given *pointer* and number of *elements* of the same type as the target slice.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void end_with(const T* pointer, std::size_t elements) {
         auto slice = Slice<T>(pointer, elements);
         this->end_with(slice);
      }

      /// @brief End the slice with the given *vector* of type *U*.
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

      /// @brief End the slice with the given *vector* of the same type as the target slice.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void end_with(const std::vector<T> &vector) {
         this->end_with(vector.data(), vector.size());
      }
      
      /// @brief End the slice with the given *pointer* of type *U*.
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

      /// @brief End the slice with the given *pointer* of the same type as the target slice.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void end_with(const T* pointer) {
         this->end_with(pointer, 1);
      }

      /// @brief End the slice with the given *reference* of type *U*.
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

      /// @brief End the slice with the given *reference* of the same type as the target slice.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void end_with(const T& reference) {
         this->end_with<T>(&reference);
      }

      /// @brief Search for the given slice *term* of type *U* in the target slice.
      ///
      /// Returns a vector of offsets to where the search term was found.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      std::vector<std::size_t> search(const Slice<U> &term) const
      {
         if (!this->aligns_with<U>()) { throw AlignmentException<T, U>(); }

         const auto reinterpretted = term.reinterpret<T>();
         
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

      /// @brief Search for the given slice *term* of the same type as the target slice.
      ///
      /// Returns a vector of offsets where the search term was found.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      std::vector<std::size_t> search(const Slice<T> &term) const {
         return this->search<T>(term);
      }

      /// @brief Search for the given *pointer* of type *U* with the given *elements* in the target slice.
      ///
      /// Returns a series of offsets that match the search term.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      std::vector<std::size_t> search(const U* pointer, std::size_t elements) const {
         const auto slice = Slice<U>(pointer, elements);
         return this->search<U>(slice);
      }

      /// @brief Search for the given *pointer* of the same type of this slice with the given number of *elements*.
      ///
      /// Returns a series of offsets that match the search term.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      std::vector<std::size_t> search(const T* pointer, std::size_t elements) const {
         auto slice = Slice<T>(pointer, elements);
         return this->search(slice);
      }

      /// @brief Search for the given vector *term* of type *U* in the target slice.
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

      /// @brief Search for the given vector *term* of the same type as the target slice.
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

      /// @brief Search for the given *pointer* of type *U* in the target slice.
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

      /// @brief Search for the given *pointer* of the same type of this slice.
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

      /// @brief Search for the given *reference* of type *U* in this slice.
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

      /// @brief Search for the given *reference* of the same type as the slice.
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

      /// @brief Search for the given *term* of the same type as the slice with optional wildcards.
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

      /// @brief Check if this slice contains the given slice *data* of type *U*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      bool contains(const Slice<U> &data) const {
         return this->search<U>(data).size() > 0;
      }

      /// @brief Check if this slice contains the given slice *data* of the same type of the slice.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      bool contains(const Slice<T> &data) const {
         return this->search(data).size() > 0;
      }

      /// @brief Check if the given *pointer* of type *U* with the given number of *elements* is contained within the slice.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      bool contains(const U* pointer, std::size_t elements) const {
         return this->search<U>(pointer, elements).size() > 0;
      }

      /// @brief Check if the given *pointer* of the same type as the slice with the given number of *elements* is contained within the slice.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      bool contains(const T* pointer, std::size_t elements) const {
         return this->search<T>(pointer, elements).size() > 0;
      }

      /// @brief Check if this slice contains the given *vector* of type *U*.
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
      
      /// @brief Check if this slice contains the given *vector* of the same data type.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      bool contains(const std::vector<T> &vector) const {
         return this->search(vector.data(), vector.size()).size() > 0;
      }

      /// @brief Check if the given *pointer* of type *U* is contained within the slice.
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

      /// @brief Check if the given *pointer* of the same type as the slice is contained within the slice.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      bool contains(const T* pointer) const {
         return this->search(pointer).size() > 0;
      }
      
      /// @brief Check if the given *reference* of type *U* is contained within the slice.
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

      /// @brief Check if the given *reference* of the same type as the slice is contained within the slice.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      bool contains(const T& reference) const {
         return this->search(&reference).size() > 0;
      }

      /// @brief Split the slice in two at the given *midpoint*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw NullPointerException
      ///
      std::pair<Slice<T>, Slice<T>> split_at(std::size_t midpoint) const {
         if (midpoint > this->elements) { throw OutOfBoundsException(midpoint, this->elements); }

         auto left = this->subslice(0, midpoint);
         auto right = this->subslice(midpoint, this->elements-midpoint);

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

      /// @brief Reverse the order of the elements of this slice.
      ///
      /// @throw OutOfBoundsException
      /// @throw NullPointerException
      ///
      void reverse() {
         if (this->elements == 0) { return; }

         for (std::size_t i=0; i<(this->elements/2); ++i)
            this->swap(i, this->elements-i-1);
      }
   };
}
