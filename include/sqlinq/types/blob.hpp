#ifndef SQLINQ_TYPES_BLOB_HPP_
#define SQLINQ_TYPES_BLOB_HPP_

#include <cstddef>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>
#include <vector>

namespace sqlinq {
using Blob = std::vector<std::byte>;

inline std::string to_string(const Blob &blob) {
  std::ostringstream ss;
  ss << "[ ";
  for (const std::byte b : blob) {
    ss << std::hex << std::setw(2) << std::setfill('0')
       << std::to_integer<int>(b) << ' ';
  }
  ss << ']';
  return ss.str();
}
} // namespace sqlinq

#endif /* SQLINQ_TYPES_BLOB_HPP_ */
