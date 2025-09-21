#ifndef SQLINQ_MYSQL_BACKEND_HPP_
#define SQLINQ_MYSQL_BACKEND_HPP_

#include <memory>
#include <mysql/mysql.h>
#include <sqlinq/backend/backend_iface.hpp>
#include <sqlinq/backend/intermediate_storage.hpp>

namespace sqlinq {
class MySQLBackend final : public BackendIface {
public:
  explicit MySQLBackend() : db_(nullptr), stmt_(nullptr), result_(nullptr) {}

  void bind_params(std::span<BoundValue> params) override;
  void bind_result(const BindData *bd, const std::size_t size) override;

  void connect(const DatabaseConfig &cfg) override;
  void disconnect() override;
  bool is_connected() const noexcept override { return db_ != nullptr; }

  uint64_t last_inserted_rowid() const noexcept override;

  void stmt_close() override;
  ExecStatus stmt_execute() override;
  ExecStatus stmt_fetch() override;
  void stmt_fetch_column(const int index, BindData &bd) override;
  void stmt_init() override;
  void stmt_prepare(std::string_view sql) override;

private:
  void map_bind_result(const sqlinq::ColumnInfo *ci, MYSQL_BIND *mb);

  void map_bind_param(const sqlinq::BindData *bd, MYSQL_BIND *mb);
  void map_bind_result(const sqlinq::BindData *bd, MYSQL_BIND *mb);

private:
  MYSQL *db_;
  MYSQL_STMT *stmt_;
  MYSQL_RES *result_;
  const BindData *bind_;
  const ColumnInfo *info_;
  std::size_t bind_size_;
  IntermediateStorage<4096> storage_;
  std::unique_ptr<MYSQL_BIND[]> my_bind_;
};
} // namespace sqlinq

#endif // SQLINQ_MYSQL_BACKEND_HPP_
