#ifndef SQLINQ_BACKEND_IFACE_HPP_
#define SQLINQ_BACKEND_IFACE_HPP_

#include "sqlinq/config.hpp"
#include "sqlinq/table.hpp"
#include "sqlinq/query_ast.hpp"
#include <cstdint>
#include <span>
#include <string_view>

namespace sqlinq {
enum class ExecStatus {
  Ok,
  Row,
  NoData,
  Truncated,
  Error
};

struct BindData {
  void *buffer;
  std::size_t *length;
  std::size_t buffer_length;
  bool *is_null;
  bool *error;
  column::Type type;
};

class BackendIface {
public:
  virtual ~BackendIface() = default;
  virtual void bind_params(std::span<BoundValue> params) = 0;
  virtual void bind_result(const BindData *bd, const std::size_t size) = 0;

  virtual void connect(const DatabaseConfig& cfg) = 0;
  virtual void disconnect() = 0;
  virtual bool is_connected() const noexcept = 0;
  virtual uint64_t last_inserted_rowid() const noexcept = 0;

  virtual void stmt_close() = 0;
  virtual ExecStatus stmt_execute() = 0;
  virtual ExecStatus stmt_fetch() = 0;
  virtual void stmt_fetch_column(const int index, BindData &bd) = 0;
  virtual void stmt_init() = 0;
  virtual void stmt_prepare(std::string_view sql) = 0;
};
} // namespace sqlinq

#endif // SQLINQ_BACKEND_IFACE_HPP_
