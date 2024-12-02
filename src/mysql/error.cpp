#include <sqlinq/mysql/error.h>
#include <sqlinq/mysql/database.h>
#include <string>
#include <system_error>
#include <mysql/mysql.h>

using namespace sqlinq::mysql;

std::error_code error::make_error_code(int err) {
  return std::error_code(err, error_category());
}

const char *error_category_impl::name() const noexcept {
  return "mysql";
}

std::string error_category_impl::message(int ev) const {
  return std::string{std::to_string(ev) + ": "};
}

const std::error_category &sqlinq::mysql::error_category() {
  static error_category_impl instance;
  return instance;
}
