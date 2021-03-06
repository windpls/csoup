#ifndef CSOUP_INTERNAL_VECTOR_H_
#define CSOUP_INTERNAL_VECTOR_H_

#include "../util/allocators.h"

namespace csoup {
    namespace internal {
        
        template <typename T>
        class VectorIterator;
        
        ///////////////////////////////////////////////////////////////////////////////
        // Vector
        
        //! A container used for storing elements of the same type
        /*! \tparam Allocator Allocator for allocating stack memory.
         */
        template <typename T>
        class Vector {
        public:
            // Optimization note: Do not allocate memory for vector_ in constructor.
            // Do it lazily when first Push() -> Expand() -> Resize().
            Vector(size_t vectorCapacity = 1, Allocator* allocator = NULL) :
                                    allocator_(allocator), stack_(0),stackTop_(0), stackEnd_(0), initialCapacity_(vectorCapacity) {
                CSOUP_ASSERT(vectorCapacity > 0);
                CSOUP_ASSERT(allocator_ != NULL);
            }
            
            ~Vector() {
                clear();
                allocator_->free(stack_);
            }
            
            void clear() {
                for (T* p = stack_; p != stackTop_; p ++) {
                    p->~T();
                }

                stackTop_ = stack_;
            }
            
            void shrinkToFit() {
                if (empty()) {
                    // If the stack is empty, completely deallocate the memory.
                    clear();
                    
                    allocator_->free(stack_);
                    stack_ = 0;
                    stackTop_ = 0;
                    stackEnd_ = 0;
                }
                else
                    resize(size());
            }
            
            // Optimization note: try to minimize the size of this function for force inline.
            // Expansion is run very infrequently, so it is moved to another (probably non-inline) function.
            CSOUP_FORCEINLINE void push(const T& obj) {
                // Expand the stack if needed
                ensureExtraSize(1);
                new (stackTop_ ++) T(obj);
            }
            
            CSOUP_FORCEINLINE T* push() {
                ensureExtraSize(1);
                return stackTop_ ++;
            }
            
            void pop() {
                CSOUP_ASSERT(!empty());
                (-- stackTop_)->~T();
            }
            
            const T* back() const {
                CSOUP_ASSERT(!empty());
                return stackTop_ - 1;
            }
            
            T* back() {
                CSOUP_ASSERT(!empty());
                return stackTop_ - 1;
            }
            
            const T* front() const {
                CSOUP_ASSERT(!empty());
                return stack_;
            }
            
            T* front() {
                CSOUP_ASSERT(!empty());
                return stack_;
            }
            
            T* at(size_t i) {
                CSOUP_ASSERT(i < size());
                return stack_ + i;
            }
            
            const T* at(size_t i) const {
                CSOUP_ASSERT(i < size());
                return stack_ + i;
            }
            
            void reserve(size_t n) {
                if (n > size()) ensureExtraSize(n - size());
            }
            
            void remove(size_t index, bool del = true) {
                CSOUP_ASSERT(index < size());
                at(index)->~T();
                std::memmove(stack_ + index, stack_ + index + 1,
                             sizeof(T) * (size() - index - 1));
                --stackTop_;
            }
            
            void insert(size_t index, const T& obj) {
                if (index > size()) index = size();
                ensureExtraSize(1);
                std::memmove(stack_ + index + 1, stack_ + index,
                             sizeof(T) * (size() - index));
                new (stack_ + index) T(obj);

                ++ stackTop_;
            }
            
            T* insert(size_t index) {
                if (index > size()) index = size();
                ensureExtraSize(1);
                std::memmove(stack_ + index + 1, stack_ + index,
                             sizeof(T) * (size() - index));
                
                ++ stackTop_;
                return stack_ + index;
            }
            
            const T* base() const {
                CSOUP_ASSERT(!empty());
                return stack_;
            }
            
            T* base() {
                CSOUP_ASSERT(!empty());
                return stack_;
            }
            
            Allocator* allocator() {
                return allocator_;
            }
            
            VectorIterator<T> begin() {
                return VectorIterator<T>(size(), 0, stack_);
            }
            
            VectorIterator<T> end() {
                return VectorIterator<T>(size(), size(), stack_);
            }
            
            void remove(const VectorIterator<T>& it, bool del = true) {
                remove(it->pos_);
            }
            
            //Allocator& getAllocator() { return *allocator_; }
            size_t size() const { return stackTop_ - stack_; }
            size_t capacity() const { return (stackEnd_ - stack_); }
            bool empty() const { return size() == 0; }
            
        private:
            friend class VectorIterator<T>;
            
            // Prohibit copy constructor & assignment operator.
            Vector(const Vector&);
            Vector& operator=(const Vector&);
            
            void ensureExtraSize(size_t count) {
                // Expand the stack if needed
                if (stackTop_ + count >= stackEnd_)
                    expand(count + size() - capacity());
            }
            
            void expand(size_t count) {
                // Only expand the capacity if the current stack exists. Otherwise just create a stack with initial capacity.
                size_t newCapacity;
                if (stack_ == 0)
                    newCapacity = initialCapacity_;
                else {
                    newCapacity = capacity();
                    newCapacity += (newCapacity + 1) / 2;
                }
                size_t newSize = size() + count;
                if (newCapacity < newSize)
                    newCapacity = newSize;
                
                resize(newCapacity);
            }
            
            void resize(size_t count) {
                const size_t oriSize = size();  // Backup the current size
                
                stack_ = (T*)allocator_->realloc(stack_, capacity() * sizeof(T),
                                                        count * sizeof(T));
                stackTop_ = stack_ + oriSize;
                stackEnd_ = stack_ + count;
            }
            
            Allocator* allocator_;
            T *stack_;
            T *stackTop_;
            T *stackEnd_;
            size_t initialCapacity_;
        };
        
        template <class T>
        class VectorIterator {
        public:
            bool hasNext() const {
                return pos_ + 1 < size_;
            }
            
            void next() {
                // You can move to end
                CSOUP_ASSERT(hasNext());
                pos_ ++ ;
            }
            
            bool hasPrevious() const {
                return pos_ > 0;
            }
            
            void previous() {
                CSOUP_ASSERT(hasPrevious());
                pos_ --;
            }
            
            T* data() {
                CSOUP_ASSERT(pos_ < size_);
                return base_ + pos_;
            }
            
            const T* data() const {
                CSOUP_ASSERT(pos_ < size_);
                return base_ + pos_;
            }
            
            bool valid() const {
                return CSOUP_ASSERT(pos_ < size_);
            }
            
        private:
            friend class Vector<T>;
            VectorIterator(size_t s, size_t p, T* base) : pos_(p), size_(s), base_(base) {}
            
            size_t pos_;
            size_t size_;
            T* base_;
        };
        
    } // namespace internal
} // namespace rapidjson

#endif