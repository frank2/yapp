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
//! **Note**: One important thing to be aware of is *alignment of data*. This is key to converting between
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
   /// @brief A slice of memory, modelled after Rust's slice object.
   ///
   /// A slice of memory, denoted by a pointer/size pair. Just like with Rust, this is a typically
   /// unsafe primitive, so be careful how you use it!
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
      /// @brief The pointer making up this slice.
      ///
      T *pointer;

      /// @brief The number of elements corresponding to this slice.
      ///
      std::size_t elements;

   public:
      /// @brief Construct a *Slice* object from a given *pointer* and *size*.
      ///
      Slice(T *pointer, std::size_t elements) : pointer(pointer), elements(elements) {}

      /// @brief Syntactic sugar to make slice objects more like arrays.
      ///
      /// @throw OutOfBoundsException
      ///
      T& operator[](std::size_t index) {
         return this->get(index);
      }

      /// @brief Get the element at the given *index* in the slice.
      ///
      /// @throw OutOfBoundsException
      ///
      T& get(std::size_t index) {
         if (index >= this->elements) { throw OutOfBoundsException(index, this->elements); }

         return *(this->pointer + index);
      }

      /// @brief Get a forward iterator to the beginning of this slice.
      ///
      Iterator begin() { return Iterator(&this->pointer[0]); }

      /// @brief Get the end iterator of this slice.
      ///
      /// To return the end of the slice as a pointer, see *Slice::eob*.
      ///
      Iterator end() { return Iterator(this->eob()); }

      /// @brief Get the end pointer of this slice.
      ///
      /// To return the end iterator of the slice, see *Slice::end*.
      ///
      T* eob() { return &this->pointer[this->elements]; }
         
      /// @brief Get the base pointer of this slice.
      ///
      T* ptr(void) { return this->pointer; }

      /// @brief Get the length of this slice.
      ///
      std::size_t size(void) { return this->elements; }

      /// @brief Get the size of an element in this slice.
      ///
      std::size_t element_size(void) { return sizeof(T); }

      /// @brief Get the amount of units of type *U* needed to fill the type of this slice.
      ///
      /// This returns 0 if the given type *U* is larger than the slice type *T*.
      ///
      template <typename U>
      std::size_t elements_needed() { return sizeof(T) / sizeof(U); }

      /// @brief Check if this slice aligns with the given size boundary.
      ///
      bool aligns_with(std::size_t size) {
         std::size_t smaller = (this->element_size() < size) ? this->element_size() : size;
         std::size_t bigger = (this->element_size() > size) ? this->element_size() : size;

         return (bigger % smaller == 0);
      }

      /// @brief Check if the given type argument aligns with the slice's element boundaries.
      ///
      template <typename U>
      bool aligns_with() { return this->aligns_with(sizeof(U)); }

      /// @brief Get a pointer into this slice of the given typename *U* at the given *offset*.
      ///
      /// Note that the *offset* argument is relative to the size of the type of the slice, just like
      /// an array offset. The argument type must align with the slice type. This is easy for byte slices,
      /// slightly more complicated for other slices.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      ///
      template <typename U>
      U* cast_ptr(std::size_t offset)
      {
         if (offset >= this->elements) { throw OutOfBoundsException(offset, this->elements); }
         if (!this->aligns_with<U>()) { throw AlignmentException<T, U>(); }

         auto this_bytes = this->elements * sizeof(T);
         auto cast_bytes = offset * sizeof(T) + sizeof(U);

         if (cast_bytes > this_bytes) { throw OutOfBoundsException(cast_bytes / sizeof(T), this->elements); }

         return reinterpret_cast<U*>(&this->operator[](offset));
      }

      /// @brief Get a reference into this slice of the given typename *U* at the given *offset*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      ///
      template <typename U>
      U& cast_ref(std::size_t offset)
      {
         return *this->cast_ptr<U>(offset);
      }

      /// @brief Return the slice as a vector of bytes.
      ///
      /// Useful for when you just want the raw bytes of the slice, rather than the individual interpretted elements.
      ///
      std::vector<std::uint8_t> as_bytes(void) {
         auto type_size = sizeof(T);
         auto start = reinterpret_cast<std::uint8_t *>(this->pointer);
         auto end = &start[this->elements * type_size];

         return std::vector<std::uint8_t>(start, end);
      }

      /// @brief Validate that the given *pointer* is within range of this buffer.
      ///
      template <typename U>
      bool validate_range(U* pointer)
      {
         auto me = reinterpret_cast<std::uintptr_t>(pointer);
         auto start = reinterpret_cast<std::uintptr_t>(this->pointer);
         auto end = reinterpret_cast<std::uintptr_t>(this->eob());

         return (me >= start && me < end);
      }
      
      /// @brief Validate that the given *pointer* is aligned to an element boundary of the slice.
      ///
      template <typename U>
      bool validate_alignment(U* pointer) {         
         auto me = reinterpret_cast<std::uintptr_t>(pointer);
         auto start = reinterpret_cast<std::uintptr_t>(this->pointer);
         auto alignment = (me - start) % sizeof(T)

         return (alignment == 0);
      }

      /// @brief Validate with both Slice::validate_range and Slice::validate_alignment that the given *pointer* is
      /// valid in this buffer.
      ///
      template <typename U>
      bool validate_ptr(U* pointer) { return (this->validate_range(pointer) && this->validate_alignment(pointer)); }

      /// @brief Convert this slice object into a vector.
      ///
      std::vector<T> to_vec(void) {
         return std::vector<T>(this->pointer, this->eob());
      }

      /// @brief Save this slice to disk.
      ///
      /// @throw std::ios_base::failure
      ///
      void save(std::string filename) {
         std::ofstream fp(filename);
         auto bytes = reinterpret_cast<char *>(this->pointer);
         auto fixed_size = this->elements * sizeof(T);
         fp.write(bytes, fixed_size);
         fp.close();
      }

      /// @brief Create a subslice of this slice at the given *offset* with the given type *U* and *size*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      ///
      template <typename U>
      Slice<U> subslice(std::size_t offset, std::size_t size)
      {
         if (offset >= this->elements) { throw OutOfBoundsException(offset, this->elements); }
         if (!this->aligns_with<U>()) { throw AlignmentException<T, U>(); }

         auto this_bytes = this->elements * sizeof(T);
         auto cast_bytes = offset * sizeof(T) + sizeof(U) * size;

         if (cast_bytes > this_bytes) { throw OutOfBoundsException(cast_bytes / sizeof(T), this->elements); }

         return Slice<U>(reinterpret_cast<U*>(&this->pointer[offset]), size);
      }

      /// @brief Create a subslice of this slice at the given *offset* and *size*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      ///
      Slice<T> subslice(std::size_t offset, std::size_t size) {
         return this->subslice<T>(offset, size);
      }

      /// @brief Read an arbitrary amount of *U* values from the given *offset* into a vector of the given *size*.
      ///
      /// Returns a vector of *U* items. If you want to create a slice object, use Slice::subslice.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      ///
      template <typename U>
      std::vector<U> read(std::size_t offset, std::size_t size)
      {
         if (offset >= this->elements) { throw OutOfBoundsException(offset, this->elements); }
         if (!this->aligns_with<U>()) { throw AlignmentException<T, U>(); }

         auto this_bytes = this->elements * sizeof(T);
         auto cast_bytes = offset * sizeof(T) + sizeof(U) * size;

         if (cast_bytes > this_bytes) { throw OutOfBoundsException(cast_bytes / sizeof(T), this->elements); }

         return std::vector<U>(reinterpret_cast<U*>(&this->pointer[offset]),
                               reinterpret_cast<U*>(&this->pointer[cast_bytes / sizeof(T)]));
      }

      /// @brief Read an arbitrary amount of values from the buffer at the given *offset* with the given *size*.
      ///
      /// Returns a vector of *T* of the given elements.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      ///
      std::vector<T> read(std::size_t offset, std::size_t size) {
         return this->read<T>(offset, size);
      }

      /// @brief Write a vector of *U* items in *data* to the given slice object at the given *offset*.
      ///
      /// Remember that the offset is relative to the size of the type of the slice! Just like array offsets.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      template <typename U>
      void write(std::size_t offset, std::vector<U> &data)
      {
         if (offset >= this->elements) { throw OutOfBoundsException(offset, this->elements); }
         if (!this->aligns_with<U>()) { throw AlignmentException<T, U>(); }

         auto needed_elements = this->elements_needed<U>();
         if (needed_elements > 0 && data.size() % needed_elements != 0) {
            throw InsufficientDataException<T, U>(data);
         }
         
         auto this_bytes = this->elements * sizeof(T);
         auto cast_bytes = offset * sizeof(T) + sizeof(U) * data.size();

         if (cast_bytes > this_bytes) { throw OutOfBoundsException(cast_bytes / sizeof(T), this->elements); }

         std::memcpy(&this->pointer[offset], data.data(), data.size() * sizeof(U));
      }

      /// @brief Write a vector of slice items of type *T* at the given *offset*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      void write(std::size_t offset, std::vector<T> &data) {
         this->write<T>(offset, data);
      }

      /// @brief Write a *slice* of type *U* at the given *offset*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      template <typename U>
      void write(std::size_t offset, Slice<U> &slice) {
         this->write<U>(offset, slice.to_vec());
      }

      /// @brief Write a *slice* of the same type at the given *offset*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      void write(std::size_t offset, Slice<T> &slice) {
         this->write<T>(offset, slice.to_vec());
      }

      /// @brief Write a *pointer* of type *U* at the given *offset*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      template <typename U>
      void write(std::size_t offset, U* pointer) {
         this->write<U>(offset, std::vector<U>(pointer, &pointer[1]));
      }

      /// @brief Write a *pointer* of the same type as the slice at the given *offset*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      void write(std::size_t offset, T* pointer) {
         this->write<T>(offset, pointer);
      }

      /// @brief Write a *reference* of type *U* at the given *offset*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      template <typename U>
      void write(std::size_t offset, U& reference) {
         this->write<U>(offset, &reference);
      }

      /// @brief Write a *reference* of the same type as the slice at the given *offset*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      void write(std::size_t offset, T& reference) {
         this->write<T>(offset, &reference);
      }

      /// @brief Start the slice with the given vector *data* of type *U*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      template <typename U>
      void start_with(std::vector<U> &data) {
         this->write<U>(0, data);
      }

      /// @brief Start the slice with the given vector *data* of the same type as the slice.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      void start_with(std::vector<T> &data) {
         this->write<T>(0, data);
      }

      /// @brief Start the slice with the given *slice* of type *U*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      template <typename U>
      void start_with(Slice<U> &slice) {
         this->write<U>(0, slice);
      }

      /// @brief Start the slice with the given *slice* of the same type.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      void start_with(Slice<T> &slice) {
         this->write<T>(0, slice);
      }

      /// @brief Start the slice with the given *pointer* of type *U*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      template <typename U>
      void start_with(U* pointer) {
         this->write<U>(0, pointer);
      }

      /// @brief Start the slice with the given *pointer* of the same type as the slice.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      void start_with(T* pointer) {
         this->write<T>(0, pointer);
      }

      /// @brief Start the slice with the given *reference* of type *U*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      template <typename U>
      void start_with(U& reference) {
         this->write<U>(0, &reference);
      }

      /// @brief Start the slice with the given *reference* of the same type as the slice.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      void start_with(T& reference) {
         this->write<T>(0, &reference);
      }

      /// @brief End the slice with the given vector *data* of type *U*.
      ///
      /// @throw OutOFBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      template <typename U>
      void end_with(std::vector<U> &data)
      {
         if (!this->aligns_with<U>()) { throw AlignmentException<T, U>(); }
         
         auto adjusted_size = (data.size() * sizeof(U)) / sizeof(T);
         if (adjusted_size > this->elements) { throw OutOfBoundsException(adjusted_size, this->elements); }

         auto offset = this->elements - adjusted_size;
         this->write<U>(offset, data);
      }

      /// @brief End the slice with the given vector *data* of the same type as the slice.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      void end_with(std::vector<T> &data) {
         this->end_with<T>(data);
      }

      /// @brief End the slice with the given *slice* of type *U*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      template <typename U>
      void end_with(Slice<U> &slice) {
         this->end_with<U>(slice.to_vec());
      }

      /// @brief End the slice with the given *slice* of the same type as the target slice.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      void end_with(Slice<T> &slice) {
         this->end_with<T>(slice.to_vec());
      }

      /// @brief End the slice with the given *pointer* of type *U*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      template <typename U>
      void end_with(U* pointer) {
         this->end_with<U>(std::vector<U>(pointer, &pointer[1]));
      }

      /// @brief End the slice with the given *pointer* of the same type as the target slice.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      void end_with(T* pointer) {
         this->end_with<T>(std::vector<T>(pointer, &pointer[1]));
      }

      /// @brief End the slice with the given *reference* of type *U*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      template <typename U>
      void end_with(U& reference) {
         this->end_with<U>(&reference);
      }

      /// @brief End the slice with the given *reference* of the same type as the target slice.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      void end_with(T& reference) {
         this->end_with<T>(&reference);
      }

      /// @brief Search for the given vector *term* in the data of type *U* in the slice.
      ///
      /// Returns a vector of offsets to where the search term was found.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      template <typename U>
      std::vector<std::size_t> search(std::vector<U> &term)
      {
         if (!this->aligns_with<U>()) { throw AlignmentException<T, U>(); }

         auto needed_elements = this->elements_needed<U>();
         
         if (needed_elements > 0 && term.size() % needed_elements != 0)
         {
            throw InsufficientDataException<T, U>(term);
         }                  
          
         auto local_term_size = (term.size() * sizeof(U)) / sizeof(T);
         auto term_slice = Slice<T>(reinterpret_cast<T*>(term.data()), local_term_size);
         if (term_slice.size() > this->size()) { throw OutOfBoundsException(term_slice.size(), this->size()); }
         
         auto potential_offsets = std::vector<std::size_t>();

         for (std::size_t i=0; i<=(this->size()-term_slice.size()); ++i)
         {
            if (this->get(i) == term_slice[0])
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
            for (std::size_t i=1; i<term_slice.size(); ++i)
            {
               if (this->get(offset+i) != term_slice[i])
               {
                  found = false;
                  break;
               }
            }

            if (found) { result.push_back(offset); }
         }

         return result;
      }

      /// @brief Search for the given vector *term* of the same type as the target slice.
      ///
      /// Returns a vector of offsets where the search term was found.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      std::vector<std::size_t> search(std::vector<T> &term) {
         return this->search<T>(term);
      }

      /// @brief Search for the given slice *term* of type *U* in the target slice.
      ///
      /// Returns a series of offsets that match the search term.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      template <typename U>
      std::vector<std::size_t> search(Slice<U> &term) {
         return this->search<U>(term.to_vec());
      }

      /// @brief Search for the given slice *term* of the same type as the target slice.
      ///
      /// Returns a series of offsets that match the search term.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      std::vector<std::size_t> search(Slice<T> &term) {
         return this->search<T>(term.to_vec());
      }

      /// @brief Search for the given *pointer* of type *U* in the target slice.
      ///
      /// Returns a series of offsets that match the search term.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      template <typename U>
      std::vector<std::size_t> search(U* pointer) {
         return this->search<U>(std::vector<U>(pointer, &pointer[1]));
      }

      /// @brief Search for the given *pointer* of the same type of this slice.
      ///
      /// Returns a series of offsets that match the search term.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      std::vector<std::size_t> search(T* pointer) {
         return this->search<T>(std::vector<T>(pointer, &pointer[1]));
      }

      /// @brief Search for the given *reference* of type *U* in this slice.
      ///
      /// Returns a series of offsets that match the search term.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      template <typename U>
      std::vector<std::size_t> search(U& reference) {
         return this->search<U>(&reference);
      }

      /// @brief Search for the given *reference* of the same type as the slice.
      ///
      /// Returns a series of offsets that match the search term.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      std::vector<std::size_t> search(T& reference) {
         return this->search<T>(&reference);
      }

      /// @brief Search for the given *value* of type *U* in the given slice.
      ///
      /// Returns a series of offsets that match the search term.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      template <typename U>
      std::vector<std::size_t> search(U value) {
         return this->search<U>(&value);
      }

      /// @brief Search for the given *value* of the same type as the slice.
      ///
      /// Returns a series of offsets that match the search term.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      std::vector<std::size_t> search(T value) {
         return this->search<T>(&value);
      }

      /// @brief Search for the given *term* of the same type as the slice with optional wildcards.
      ///
      /// Using a vector of std::optional<*T*>, we can use std::nullopt as a wildcard pattern for
      /// searching. However, unlike the other search counterparts, this can only be done against
      /// the current type: a given vector search term cannot be converted between type bases in
      /// a dynamic search.
      ///
      /// Returns a vector of a pair: an offset and the vector which matched the term.
      ///
      /// @throw OutOfBoundsException
      ///
      DynamicSearchResult search_dynamic(DynamicSearchTerm &term)
      {
         if (term.size() > this->size()) { throw OutOfBoundsException(term.size(), this->size()); }
         
         std::size_t shift = 0;

         for (std::size_t i=0; i<term.size(); ++i)
         {
            if (!term[i].has_value()) { ++shift; continue; }
            
            // we found a value, we found our shift point
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

      /// @brief Check if this slice contains the given vector *data* of type *U*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      template <typename U>
      bool contains(std::vector<U> &data) {
         return this->search<U>(data).size() > 0;
      }

      /// @brief Check if this slice contains the given vector *data* of the same type of the slice.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      bool contains(std::vector<T> &data) {
         return this->search<T>(data).size() > 0;
      }

      /// @brief Check if this slice contains the given *slice* of type *U*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      template <typename U>
      bool contains(Slice<U> &slice) {
         return this->search<U>(slice).size() > 0;
      }
      
      /// @brief Check if this slice contains the given *slice* of the same data type.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      bool contains(Slice<T> &slice) {
         return this->search<T>(slice).size() > 0;
      }

      /// @brief Check if the given *pointer* of type *U* is contained within the slice.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      template <typename U>
      bool contains(U* pointer) {
         return this->search<U>(pointer).size() > 0;
      }

      /// @brief Check if the given *pointer* of the same type as the slice is contained within the slice.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      bool contains(T* pointer) {
         return this->search<T>(pointer).size() > 0;
      }
      
      /// @brief Check if the given *reference* of type *U* is contained within the slice.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      template <typename U>
      bool contains(U& reference) {
         return this->search<U>(&reference).size() > 0;
      }

      /// @brief Check if the given *reference* of the same type as the slice is contained within the slice.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      bool contains(T& reference) {
         return this->search<T>(&reference).size() > 0;
      }

      /// @brief Check if the given *value* of type *U* is contained within the slice.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      template <typename U>
      bool contains(U value) {
         return this->search<U>(&value).size() > 0;
      }
      
      /// @brief Check if the given *value* of the same type as the slice is contained within the slice.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      ///
      bool contains(T value) {
         return this->search<T>(&value).size() > 0;
      }
   };
}
      
