#ifndef SQLINQ_BACKEND_IFACE_HPP_
#define SQLINQ_BACKEND_IFACE_HPP_

#include <cstdint>
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
  enum class Group {
    Container,
    Datetime,
    Decimal,
    Floating,
    Integral,
    Null
  };

  enum Type {
    Null,
    Bit,
    Tiny,
    Short,
    Long,
    LongLong,
    Float,
    Double,
    Date,
    Time,
    Datetime,
    Timestamp,
    Decimal,
    Blob,
    Text,
  };

  void *buffer;
  std::size_t *length;
  std::size_t buffer_length;
  bool *is_null;
  bool *error;
  Type type;

  Group group() const noexcept {
    switch(type) {
      case Bit:
      case Tiny:
      case Short:
      case Long:
      case LongLong:
        return Group::Integral;
      case Float:
      case Double:
        return Group::Floating;
      case Date:
      case Time:
      case Datetime:
      case Timestamp:
        return Group::Datetime;
      case Blob:
      case Text:
        return Group::Container;
      case Decimal:
        return Group::Decimal;
      default:
        break;
    }
    return Group::Null;
  }
};

class BackendIface {
public:
  virtual ~BackendIface() = default;
  virtual void bind_param(const BindData *bd, const std::size_t size) = 0;
  virtual void bind_result(const BindData *bd, const std::size_t size) = 0;

  /*virtual void connect() = 0;*/
  /*virtual void disconnect() = 0;*/
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
