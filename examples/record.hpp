#ifndef SQLINQ_EXAMPLES_RECORD_HPP_
#define SQLINQ_EXAMPLES_RECORD_HPP_

#include <ctime>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>
#include <sqlinq/types/decimal.hpp>

namespace detail {

template <std::size_t M, std::size_t D>
void print_val(sqlinq::decimal<M, D>&& value) {
  constexpr static int width = M + D + 2;
  std::cout << '|' << std::setw(width) << std::right << value;
  std::cout << std::setfill(' ');
}

template <std::size_t M, std::size_t D>
void print_val(const sqlinq::decimal<M, D>& value) {
  constexpr static int width = M + D + 2;
  std::cout << '|' << std::setw(width) << std::right << value;
  std::cout << std::setfill(' ');
}

inline void print_val(std::time_t&& value) {
  std::cout << '|' << std::setw(20) << std::right << value;
}

inline void print_val(const std::time_t& value) {
  std::cout << '|' << std::setw(20) << std::right << value;
}

template <std::integral T>
void print_val(T&& value) {
  std::cout << '|' << std::setw(8) << std::left << value;
}

inline void print_val(std::string&& text) {
  std::cout << '|' << std::setw(30) << std::left << text;
}

inline void print_val(const std::string& text) {
  std::cout << '|' << std::setw(30) << std::left<< text;
}

template <typename T>
void print_val(std::optional<T>&& opt) {
  if (!opt.has_value()) {
    print_val(std::string{"NULL"});
    return;
  }
  print_val(opt.value());
}

template <typename Tuple, std::size_t...Is>
void print_record_impl(Tuple&& tup, std::index_sequence<Is...>) {
  (print_val(std::get<Is>(std::forward<Tuple>(tup))),...);
}
} // namespace detail

template <typename... Args>
void print_record(std::tuple<Args...>&& tup) {
  constexpr std::size_t size = std::tuple_size_v<std::remove_reference_t<decltype(tup)>>;
  detail::print_record_impl(std::forward<decltype(tup)>(tup), std::make_index_sequence<size>{});
  std::cout << '|' << '\n';
}
#endif /* SQLINQ_EXAMPLES_RECORD_HPP_ */
