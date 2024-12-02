#ifndef SQLINQ_MYSQL_DB_ERROR_H_
#define SQLINQ_MYSQL_DB_ERROR_H_

#include <system_error>

namespace sqlinq::mysql {

namespace error {
  std::error_code make_error_code(int err);
}

class error_category_impl : public std::error_category {
public:
  virtual const char* name() const noexcept override;
  virtual std::string message(int ev) const override;
};

const std::error_category& error_category();
}

#endif /* SQLINQ_MYSQL_DB_ERROR_H_ */
