#include <sqlinq/sqlite/error.h>
#include <sqlite3.h>
#include <system_error>

using namespace sqlinq::sqlite;

std::error_code error::make_error_code(int err) {
  return std::error_code(err, error_category());
}

const char *error_category_impl::name() const noexcept {
  return "sqlite3";
}

std::string error_category_impl::message(int ev) const {
  return sqlite3_errstr(ev);
}

const std::error_category &sqlinq::sqlite::error_category() {
  static error_category_impl instance;
  return instance;
}
