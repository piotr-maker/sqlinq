#ifndef SQLINQ_BACKEND_INTERMEDIATE_STORAGE_HPP_
#define SQLINQ_BACKEND_INTERMEDIATE_STORAGE_HPP_

#include <type_traits>

namespace sqlinq {

template <std::size_t N> class IntermediateStorage {
public:
  IntermediateStorage() noexcept : end_(buf_) {}

  template <typename T> void *allocate(std::size_t count = 1) noexcept {
    static_assert(!std::is_reference_v<T>, "Cannot allocate reference type");
    static_assert(!std::is_void_v<T>, "Cannot allocate void type");

    std::size_t used = size();
    std::size_t size = count * sizeof(T);
    if (used + size > N) {
      return nullptr;
    }
    char *ptr = end_;
    end_ += size;
    return ptr;
  }

  std::size_t capacity() const noexcept { return N; }
  void clear() noexcept { end_ = buf_; }
  bool empty() const noexcept { return end_ == buf_; }
  std::size_t size() const noexcept {
    return static_cast<std::size_t>(end_ - buf_);
  }

private:
  char *end_;
  char buf_[N];
};
} // namespace sqlinq

#endif // SQLINQ_BACKEND_INTERMEDIATE_STORAGE_HPP_
