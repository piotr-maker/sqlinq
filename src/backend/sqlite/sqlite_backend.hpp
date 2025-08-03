#ifndef SQLINQ_SQLITE_BACKEND_HPP_
#define SQLINQ_SQLITE_BACKEND_HPP_

#include <sqlinq/backend/backend_iface.hpp>
#include <sqlite3.h>

namespace sqlinq {
class SQLiteBackend final : public BackendIface {
public:
  explicit SQLiteBackend()
      : db_(nullptr), stmt_(nullptr), truncated_(false), omit_step_(true),
        bind_(nullptr), bind_size_(0) {}

  void bind_param(const BindData *bd, const std::size_t size) override;
  void bind_result(const BindData *bd, const std::size_t size) override;

  void connect(const char *fname);
  void disconnect();
  bool is_connected() const noexcept override { return db_ != nullptr; }

  uint64_t last_inserted_rowid() const noexcept override;

  void stmt_close() override;
  ExecStatus stmt_execute() override;
  ExecStatus stmt_fetch() override;
  void stmt_fetch_column(const int index, BindData &bd) override;
  void stmt_init() noexcept override {}
  void stmt_prepare(std::string_view sql) override;

private:
  sqlite3 *db_;
  sqlite3_stmt *stmt_;
  bool truncated_;
  bool omit_step_;
  const BindData *bind_;
  std::size_t bind_size_;
};
} // namespace sqlinq

#endif // SQLINQ_SQLITE_BACKEND_HPP_
