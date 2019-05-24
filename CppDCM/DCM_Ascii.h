#ifndef _CPPDCM_DCM_ASCII_H
#define _CPPDCM_DCM_ASCII_H
#include <utility>

namespace DCM {

template <typename T>
class AsciiIteratorGenerator {
public:
  class AsciiIterator : public std::iterator<std::forward_iterator_tag, char> {
    //    typedef AsciiIterator<T> iterator;

  public:
    AsciiIterator() : __ptr(nullptr), __pos(0) {}
    AsciiIterator(T const *v, std::size_t p) : __ptr(v), __pos(p) {}
    ~AsciiIterator() {}

    iterator operator++(int) /* postfix */ { return pos_++; }
    iterator &operator++() /* prefix */ {
      ++pos_;
      return *this;
    }
    reference operator*() const { return '0' + (__ptr[__pos]); }
    pointer operator->() const { return __ptr + __pos; }
    iterator operator+(difference_type v) const { return __pos + v; }
    bool operator==(const iterator &rhs) const { return __pos == rhs.__pos; }
    bool operator!=(const iterator &rhs) const { return __pos != rhs.__pos; }

  private:
    T const *__ptr;
    std::size_t __pos;
  };

  AsciiIteratorGenerator(T const *array, int len) : __ptr(array), __len(len) {
  }

  AsciiIterator begin() {
    return AsciiIterator(__ptr, 0);
  }

  AsciiIterator end() {
    return AsciiIterator(__ptr, __len);
  }

  const AsciiIterator cbegin() {
    return AsciiIterator(__ptr, 0);
  }

  const AsciiIterator cend() {
    return AsciiIterator(__ptr, __len);
  }

private:
  T const *__ptr;
  std::size_t __len;
};

} // namespace DCM

#endif