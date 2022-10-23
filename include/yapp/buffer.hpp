#pragma once

#include <yapp/slice.hpp>

namespace yapp
{
   /// @brief An owned-data slice object.
   ///
   /// Use this class, for example, when you want to directly manipulate the size of the slice.
   ///
   template <typename T>
   class Buffer : public Slice<T>
   {
   protected:
      std::vector<T> data;

      /// @brief Sets the pointer/size pair for the underlying slice object.
      ///
      void set(T* pointer, std::size_t elements) { this->pointer.m = pointer; this->elements = elements; }

   public:
      /// @brief Construct an empty Buffer object.
      ///
      Buffer() : Slice<T>() {
         this->data = std::vector<T>();
         this->set(this->data.data(), 0);
      }
      Buffer(const Buffer &other) : data(other.data), Slice<T>() {
         this->set(this->data.data(), this->data.size());
      }
      /// @brief Construct a Buffer from a *pointer*/*elements* pair.
      ///
      /// @throw NullPointerException
      ///
      Buffer(const T* pointer, std::size_t elements) : Slice<T>() {
         this->load_data(pointer, elements);
      }
      /// @brief Construct a Buffer object from a *slice* object.
      ///
      /// @throw NullPointerException
      ///
      Buffer(const Slice<T> &slice) : Slice<T>() {
         this->load_data(slice);
      }
      /// @brief Construct a Buffer object from a vector of *data*.
      ///
      /// @throw NullPointerException
      ///
      Buffer(const std::vector<T> &data) : Slice<T>() {
         this->load_data(data);
      }
      /// @brief Construct a Buffer object with an initial size of *elements*.
      ///
      Buffer(std::size_t elements) : Slice<T>() {
         this->data = std::vector<T>(elements);
         this->set(this->data.data(), this->data.size());
      }
      /// @brief Construct a Buffer object from a given *filename*.
      ///
      /// @throw std::ios_base::failure
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      Buffer(const std::string &filename) : Slice<T>() {
         this->load_file(filename);
      }

      /// @brief Load a *slice* of the same type as this buffer into the buffer.
      ///
      /// @throw NullPointerException
      ///
      void load_data(const Slice<T> &slice) {
         this->data = slice.to_vec();
         this->set(this->data.data(), this->data.size());
      }
      
      /// @brief Load a *slice* of type *U* into the buffer.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void load_data(const Slice<U> &slice) {
         Slice<T> reinterpretted = slice.reinterpret<T>();
         this->load_data(reinterpretted);
      }

      /// @brief Load a *pointer* of type *U* with the given *elements* into the buffer.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void load_data(const U* pointer, std::size_t elements) {
         const auto slice = Slice<U>(pointer, elements);
         this->load_data<U>(slice);
      }

      /// @brief Load a *pointer* of the same type as this buffer with the given *elements* into the buffer.
      ///
      /// @throw NullPointerException
      ///
      void load_data(const T* pointer, std::size_t elements) {
         const auto slice = Slice<T>(pointer, elements);
         this->load_data(slice);
      }

      /// @brief Load a *vector* of type *U* into this buffer.
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

      /// @brief Load a *vector* of the same type into this buffer.
      ///
      /// @throw NullPointerException
      ///
      void load_data(const std::vector<T> &data) {
         this->load_data(data.data(), data.size());
      }

      /// @brief Load a *pointer* of type *U* into this buffer.
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

      /// @brief Load a *pointer* of the same type as this buffer into the buffer.
      ///
      /// @throw NullPointerException
      ///
      void load_data(const T* pointer) {
         this->load_data(pointer, 1);
      }

      /// @brief Load a *reference* of type *U* into this buffer.
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

      /// @brief Load a *reference* of the same type as this buffer into the buffer.
      ///
      /// @throw NullPointerException
      ///
      void load_data(const T& reference) {
         this->load_data(&reference);
      }

      /// @brief Load a *filename*'s data into the buffer.
      ///
      /// @throw std::ios_base::failure
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void load_file(const std::string &filename) {
         std::ifstream fp(filename);
         std::streampos filesize;

         fp.seekg(0, std::ios::end);
         filesize = fp.tellg();
         fp.seekg(0, std::ios::beg);

         auto u8_data = std::vector<std::uint8_t>(filesize);
         u8_data.insert(u8_data.begin(),
                        std::istream_iterator<std::uint8_t>(fp),
                        std::istream_iterator<std::uint8_t>());

         fp.close();

         auto u8_slice = Slice<std::uint8_t>(u8_data.data(), u8_data.size());
         this->data = u8_slice.reinterpret<T>().to_vec();

         this->set(this->data.data(), this->data.size());
      }

      /// @brief Resize the underlying vector within the buffer object with the given number of *elements* and optional padding-value *value*.
      ///
      void resize(std::size_t elements, T value = T()) {
         this->data.resize(elements, value);
         this->set(this->data.data(), this->data.size());
      }

      /// @brief Append the given *slice* of type *U* to the end of the buffer.
      ///
      /// This resizes the buffer to accomodate the new data and then appends it to the end of the buffer.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void append(const Slice<U> &slice) {
         const auto reinterpretted = slice.reinterpret<T>();
         this->resize(this->elements + reinterpretted.size());
         this->write<T>(this->elements - reinterpretted.size(), reinterpretted);
      }

      /// @brief Append the given *slice* of the same type as the buffer to the end of the buffer.
      ///
      /// This resizes the buffer to accomodate the new data and then appends it to the end of the buffer.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void append(const Slice<T> &slice) {
         this->append<T>(slice);
      }

      /// @brief Append the given *pointer* of type *U* with the given number of *elements* to the end of the buffer.
      ///
      /// This resizes the buffer to accomodate the new data and then appends it to the end of the buffer.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      template <typename U>
      void append(const U* pointer, std::size_t elements) {
         const auto slice = Slice<U>(pointer, elements);
         this->append<U>(slice);
      }

      /// @brief Append the given *pointer* of the same type as the buffer with the given number of *elements*
      /// to the end of the buffer.
      ///
      /// This resizes the buffer to accomodate the new data and then appends it to the end of the buffer.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void append(const T* pointer, std::size_t elements) {
         const auto slice = Slice<T>(pointer, elements);
         this->append<T>(slice);
      }

      /// @brief Append the given *vector* of type *U* to the end of the buffer.
      ///
      /// This resizes the buffer to accomodate the new data and then appends it to the end of the buffer.
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

      /// @brief Append the given *vector* of the same type as the buffer to the end of the buffer.
      ///
      /// This resizes the buffer to accomodate the new data and then appends it to the end of the buffer.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void append(const std::vector<T> &vector) {
         this->append(vector.data(), vector.size());
      }
      
      /// @brief Append the given *pointer* of type *U* to the end of the buffer.
      ///
      /// This resizes the buffer to accomodate the new data and then appends it to the end of the buffer.
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

      /// @brief Append the given *pointer* of the same type as the buffer to the end of the buffer.
      ///
      /// This resizes the buffer to accomodate the new data and then appends it to the end of the buffer.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void append(const T* pointer) {
         this->append(pointer, 1);
      }
      
      /// @brief Append the given *reference* of type *U* to the end of the buffer.
      ///
      /// This resizes the buffer to accomodate the new data and then appends it to the end of the buffer.
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

      /// @brief Append the given *reference* of the same type as the buffer to the end of the buffer.
      ///
      /// This resizes the buffer to accomodate the new data and then appends it to the end of the buffer.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullReferenceException
      ///
      void append(const T& reference) {
         this->append(&reference);
      }

      /// @brief Insert the given *slice* of type *U* at the given *offset* in the buffer.
      ///
      /// This expands the underlying vector and injects the data at the given offset.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void insert(std::size_t offset, const Slice<U> &slice) {
         if (offset > this->elements) { return OutOfBoundsException(offset, this->elements); }

         const auto reinterpretted = slice.reinterpret<T>();

         auto iter = this->data.begin();
         this->data.insert(iter+offset, reinterpretted.begin(), reinterpretted.end());
         this->set(this->data.data(), this->data.size());
      }

      /// @brief Insert the given *slice* of the same type as the buffer at the given *offset* in the buffer.
      ///
      /// This expands the underlying vector and injects the data at the given offset.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void insert(std::size_t offset, const Slice<T> &slice) {
         this->insert<T>(offset, slice);
      }

      /// @brief Insert the given *pointer* of type *U* with the given number of *elements* at the given *offset* in the buffer.
      ///
      /// This expands the underlying vector and injects the data at the given offset.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void insert(std::size_t offset, const U* pointer, std::size_t elements) {
         const auto slice = Slice<U>(pointer, elements);
         this->insert<U>(slice);
      }

      /// @brief Insert the given *pointer* of the same type as the buffer with the given number of *elements*
      /// at the given *offset* in the buffer.
      ///
      /// This expands the underlying vector and injects the data at the given offset.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void insert(std::size_t offset, const T* pointer, std::size_t elements) {
         const auto slice = Slice<T>(pointer, elements);
         this->insert(slice);
      }

      /// @brief Insert the given *vector* of type *U* at the given *offset*.
      ///
      /// This expands the underlying vector and injects the data at the given offset.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void insert(std::size_t offset, const std::vector<U> &vector) {
         this->insert<U>(offset, vector.data(), vector.size());
      }
      
      /// @brief Insert the given *vector* of the same type as the buffer at the given *offset*.
      ///
      /// This expands the underlying vector and injects the data at the given offset.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void insert(std::size_t offset, const std::vector<T> &vector) {
         this->insert(offset, vector.data(), vector.size());
      }

      /// @brief Insert the given *pointer* of type *U* at the given *offset* into the buffer.
      ///
      /// This expands the underlying vector and injects the data at the given offset.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      template <typename U>
      void insert(std::size_t offset, const U* pointer) {
         this->insert<U>(offset, pointer, 1);
      }

      /// @brief Insert the given *pointer* of the same type as the buffer at the given *offset* into the buffer.
      ///
      /// This expands the underlying vector and injects the data at the given offset.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullPointerException
      ///
      void insert(std::size_t offset, const T* pointer) {
         this->insert(offset, pointer, 1);
      }
      
      /// @brief Insert the given *reference* of type *U* at the given *offset* into the buffer.
      ///
      /// This expands the underlying vector and injects the data at the given offset.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullReferenceException
      ///
      template <typename U>
      void insert(std::size_t offset, const U& reference) {
         this->insert<U>(offset, &reference);
      }

      /// @brief Insert the given *reference* of the same type as the buffer at the given *offset* into the buffer.
      ///
      /// This expands the underlying vector and injects the data at the given offset.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullReferenceException
      ///
      void insert(std::size_t offset, const T& reference) {
         this->insert(offset, &reference);
      }

      /// @brief Remove (or "erase") the range of elements, specified by the *start* offset and the *end* offset.
      ///
      /// @throw OutOfBoundsException
      ///
      void erase(std::size_t start, std::size_t end) {
         if (end > this->elements) { throw OutOfBoundsException(end, this->elements); }

         auto iter = this->data.begin();
         this->data.erase(iter+start, iter+end);
         this->set(this->data.data(), this->data.size());
      }
      
      /// @brief Remove (or "erase") the element at the given *offset*, shrinking the backing vector.
      ///
      /// @throw OutOfBoundsException
      ///
      void erase(std::size_t offset) {
         this->erase(offset, offset+1);
      }

      /// @brief Push a *value* of type *T* onto the end of the stack.
      ///
      /// This essentially does the same operation as *Buffer::append*, but only for the type of the buffer.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw InsufficientDataException
      /// @throw NullReferenceException
      ///
      void push(const T& reference) {
         this->append(&reference);
      }

      /// @brief Pop an element off the end of the buffer, if one exists.
      ///
      std::optional<T> pop() {
         if (this->elements == 0) { return std::nullopt; }

         T value = this->get(this->elements-1); // intentionally convert this reference to a value
         this->resize(this->elements-1);

         return value;
      }

      /// @brief Clear this buffer of any data.
      ///
      void clear() {
         this->data = std::vector<T>();
         this->set(this->data.data(), this->data.size());
      }

      /// @brief Split this buffer in two at the given *midpoint*, returning the data buffer that was split.
      ///
      /// There is no const equivalent because it resizes the underlying buffer. For similar functionality that
      /// contains const structures, see *Slice::split_at*.
      ///
      /// @throw OutOfBoundsException
      /// @throw AlignmentException
      /// @throw NullPointerException
      ///
      Buffer<T> split_off(std::size_t midpoint) {
         auto split_pair = this->split_at(midpoint);
         auto split_buffer = Buffer<T>(split_pair.second.to_vec());
         this->resize(midpoint);

         return split_buffer;
      }

      /// @brief Represent this buffer as a slice object.
      ///
      Slice<T> &as_slice() {
         return *reinterpret_cast<Slice<T> *>(this);
      }

      /// @brief Represent this buffer as a const slice object.
      ///
      const Slice<T> &as_slice() const {
         return *reinterpret_cast<const Slice<T> *>(this);
      }
   };
}
