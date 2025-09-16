#include "sqlite_backend.hpp"
#include <cassert>
#include <cstring>
#include <sqlinq/types.h>
#include <stdexcept>

namespace sqlinq {
void fetch_numeric_column(sqlite3_stmt *stmt, const int index,
                          const BindData &bind) {
  int col_type = sqlite3_column_type(stmt, index);
  assert(col_type == SQLITE_INTEGER && "Invalid column type");
  if (bind.error != nullptr) {
    *bind.error = col_type != SQLITE_INTEGER;
  }

  if (bind.is_null != nullptr) {
    *bind.is_null = false;
  }

  if (bind.buffer == nullptr) {
    return;
  }

  switch (bind.type) {
  case column::Type::Bit:
    *(bool *)(bind.buffer) = (bool)sqlite3_column_int(stmt, index);
    break;
  case column::Type::TinyInt:
    *(char *)(bind.buffer) = (char)sqlite3_column_int(stmt, index);
    break;
  case column::Type::SmallInt:
    *(short *)(bind.buffer) = (short)sqlite3_column_int(stmt, index);
    break;
  case column::Type::Int:
    *(long *)(bind.buffer) = (long)sqlite3_column_int(stmt, index);
    break;
  case column::Type::BigInt:
    *(long long *)(bind.buffer) = (long long)sqlite3_column_int64(stmt, index);
    break;
  default:
    break;
  }
}

void fetch_floating_column(sqlite3_stmt *stmt, const int index,
                           const BindData &bind) {
  int col_type = sqlite3_column_type(stmt, index);
  assert(col_type == SQLITE_FLOAT && "Invalid column type");
  if (bind.error != nullptr) {
    *bind.error = col_type != SQLITE_FLOAT;
  }

  if (bind.is_null != nullptr) {
    *bind.is_null = false;
  }

  if (bind.buffer == nullptr) {
    return;
  }

  switch (bind.type) {
  case column::Type::Float:
    *(float *)(bind.buffer) = (float)sqlite3_column_double(stmt, index);
    break;
  case column::Type::Double:
    *(double *)(bind.buffer) = (double)sqlite3_column_double(stmt, index);
    break;
  default:
    break;
  }
}

bool fetch_container_column(sqlite3_stmt *stmt, const int index,
                            const BindData &bind) {
  bool truncated = true;
  std::size_t column_bytes = (std::size_t)sqlite3_column_bytes(stmt, index);
  int type = sqlite3_column_type(stmt, index);
  assert((type == SQLITE_TEXT || type == SQLITE_BLOB) && "Invalid column type");
  if (bind.buffer != nullptr && bind.buffer_length > 0) {
    const char *data = nullptr;
    if (bind.type == column::Type::Blob) {
      data = (const char *)sqlite3_column_blob(stmt, index);
    } else if (bind.type == column::Type::Text) {
      data = (const char *)sqlite3_column_text(stmt, index);
    }

    assert(data != nullptr && "Column is null");
    std::size_t size = std::min(column_bytes, bind.buffer_length);
    memcpy(bind.buffer, data, size);
    truncated = column_bytes > size;
  }

  if (bind.error != nullptr) {
    *bind.error = (type != SQLITE_TEXT && type != SQLITE_BLOB);
  }

  if (bind.is_null != nullptr) {
    *bind.is_null = false;
  }

  if (bind.length != nullptr) {
    *bind.length = column_bytes;
  }
  return truncated;
}

void fetch_datetime_column(sqlite3_stmt *stmt, const int index,
                           const BindData &bind) {
  int col_type = sqlite3_column_type(stmt, index);
  if (bind.type == column::Type::Timestamp) {
    assert(col_type == SQLITE_INTEGER && "Invalid column type");
  } else {
    assert(col_type == SQLITE_TEXT && "Invalid column type");
  }
  if (bind.error != nullptr) {
    *bind.error = bind.type == column::Type::Timestamp ? col_type == SQLITE_INTEGER
                                                   : col_type != SQLITE_TEXT;
  }

  if (bind.is_null != nullptr) {
    *bind.is_null = false;
  }

  if (bind.buffer == nullptr) {
    return;
  }

  const char *data;
  switch (bind.type) {
  case column::Type::Date: {
    data = (const char *)sqlite3_column_text(stmt, index);
    sqlinq::Date date;
    sqlinq::from_string(data, date);
    memcpy(bind.buffer, (void *)&date, sizeof(date));
    break;
  }
  case column::Type::Time: {
    data = (const char *)sqlite3_column_text(stmt, index);
    sqlinq::Time time;
    sqlinq::from_string(data, time);
    memcpy(bind.buffer, (void *)&time, sizeof(time));
    break;
  }
  case column::Type::Datetime: {
    data = (const char *)sqlite3_column_text(stmt, index);
    sqlinq::Datetime dt;
    sqlinq::from_string(data, dt);
    memcpy(bind.buffer, (void *)&dt, sizeof(dt));
    break;
  }
  case column::Type::Timestamp: {
    long long v;
    v = (int64_t)sqlite3_column_int64(stmt, index);
    sqlinq::Timestamp ts{v};
    memcpy(bind.buffer, (void *)&ts, sizeof(ts));
    break;
  }
  default:
    break;
  }
}

void fetch_decimal_column(sqlite3_stmt *stmt, const int index,
                          const BindData &bind) {
  int col_type = sqlite3_column_type(stmt, index);
  assert(col_type == SQLITE_INTEGER && "Invalid column type");
  if (bind.error != nullptr) {
    *bind.error = col_type != SQLITE_INTEGER;
  }

  if (bind.is_null != nullptr) {
    *bind.is_null = false;
  }

  if (bind.buffer == nullptr) {
    return;
  }

  int64_t raw = (long long)sqlite3_column_int64(stmt, index);
  auto decimal = sqlinq::Decimal<18, 0>::from_raw(raw);
  memcpy(bind.buffer, (void *)&decimal, sizeof(decimal));
}

void SQLiteBackend::bind_params(std::span<BoundValue> params) {
  int rc = SQLITE_OK;
  sqlite3_reset(stmt_);
  sqlite3_clear_bindings(stmt_);
  for (int idx = 1; idx <= static_cast<int>(params.size()); idx++) {
    std::string s;
    BoundValue &p = params[std::size_t(idx) - 1];
    switch (p.type()) {
    case column::Type::Null:
      rc = sqlite3_bind_null(stmt_, idx);
      break;
    case column::Type::Bit:
    case column::Type::TinyInt:
      rc = sqlite3_bind_int64(stmt_, idx, *(int8_t *)p.ptr());
      break;
    case column::Type::SmallInt:
      rc = sqlite3_bind_int64(stmt_, idx, *(int16_t *)p.ptr());
      break;
    case column::Type::Int:
      rc = sqlite3_bind_int64(stmt_, idx, *(int32_t *)p.ptr());
      break;
    case column::Type::BigInt:
    case column::Type::Decimal:
    case column::Type::Timestamp:
      rc = sqlite3_bind_int64(stmt_, idx, *(int64_t *)p.ptr());
      break;
    case column::Type::Float:
      rc = sqlite3_bind_double(stmt_, idx, *(float *)p.ptr());
      break;
    case column::Type::Double:
      rc = sqlite3_bind_double(stmt_, idx, *(double *)p.ptr());
      break;
    case column::Type::Blob:
      rc = sqlite3_bind_blob(stmt_, idx, (char *)p.ptr(), (int)p.size(), NULL);
      break;
    case column::Type::Text:
      rc = sqlite3_bind_text(stmt_, idx, (char *)p.ptr(), (int)p.size(), NULL);
      break;
    case column::Type::Date:
      s = sqlinq::to_string(*(Date *)p.ptr());
      rc = sqlite3_bind_text(stmt_, idx, (char *)s.data(), (int)s.size(),
                             SQLITE_TRANSIENT);
      break;
    case column::Type::Time:
      s = sqlinq::to_string(*(Time *)p.ptr());
      rc = sqlite3_bind_text(stmt_, idx, (char *)s.data(), (int)s.size(),
                             SQLITE_TRANSIENT);
      break;
    case column::Type::Datetime:
      s = sqlinq::to_string(*(Datetime *)p.ptr());
      rc = sqlite3_bind_text(stmt_, idx, (char *)s.data(), (int)s.size(),
                             SQLITE_TRANSIENT);
      break;
    }

    if (rc != SQLITE_OK) {
      throw std::runtime_error(sqlite3_errmsg(db_));
    }
  }
}

void SQLiteBackend::bind_result(const BindData *bd, const std::size_t size) {
  bind_ = bd;
  bind_size_ = size;
}

void SQLiteBackend::connect(const char *fname) {
  if (int rc = sqlite3_open(fname, &db_); rc != SQLITE_OK) {
    throw std::runtime_error("Failed to open database");
  }
}

void SQLiteBackend::disconnect() {
  if (db_ != nullptr) {
    sqlite3_close(db_);
    db_ = nullptr;
  }
}

uint64_t SQLiteBackend::last_inserted_rowid() const noexcept {
  return (uint64_t)sqlite3_last_insert_rowid(db_);
}

void SQLiteBackend::stmt_close() {
  if (stmt_ != nullptr) {
    bind_ = nullptr;
    sqlite3_finalize(stmt_);
    stmt_ = nullptr;
  }
}

ExecStatus SQLiteBackend::stmt_execute() {
  int rc = sqlite3_step(stmt_);
  switch (rc) {
  case SQLITE_OK:
  case SQLITE_DONE:
    return ExecStatus::Ok;
  case SQLITE_ROW:
    omit_step_ = true;
    return ExecStatus::Ok;
  default:
    throw std::runtime_error(sqlite3_errmsg(db_));
  }
}

ExecStatus SQLiteBackend::stmt_fetch() {
  int rc = SQLITE_ROW;
  if (bind_ == nullptr) {
    return ExecStatus::Error;
  }

  if (!omit_step_) {
    rc = sqlite3_step(stmt_);
  }
  omit_step_ = false;
  truncated_ = false;
  for (int index = 0; index < static_cast<int>(bind_size_); index++) {
    const BindData *bind = &bind_[index];
    const int type = sqlite3_column_type(stmt_, index);
    if (type == SQLITE_NULL && bind->is_null != nullptr) {
      *bind->is_null = true;
      continue;
    }
    switch (bind->type) {
    case column::Type::Bit:
    case column::Type::TinyInt:
    case column::Type::SmallInt:
    case column::Type::Int:
    case column::Type::BigInt:
      fetch_numeric_column(stmt_, index, *bind);
      break;
    case column::Type::Float:
    case column::Type::Double:
      fetch_floating_column(stmt_, index, *bind);
      break;
    case column::Type::Text:
    case column::Type::Blob:
      truncated_ = fetch_container_column(stmt_, index, *bind);
      break;
    case column::Type::Date:
    case column::Type::Time:
    case column::Type::Datetime:
    case column::Type::Timestamp:
      fetch_datetime_column(stmt_, index, *bind);
      break;
    case column::Type::Decimal:
      fetch_decimal_column(stmt_, index, *bind);
      break;
    default:
      break;
    }
  }
  switch (rc) {
  case SQLITE_OK:
    return ExecStatus::Ok;
  case SQLITE_ROW:
    return truncated_ == true ? ExecStatus::Truncated : ExecStatus::Row;
  case SQLITE_DONE:
    return ExecStatus::NoData;
  default:
    throw std::runtime_error(sqlite3_errmsg(db_));
  }
  return ExecStatus::Error;
}

void SQLiteBackend::stmt_fetch_column(const int index, BindData &bind) {
  truncated_ = fetch_container_column(stmt_, index, bind);
}

void SQLiteBackend::stmt_prepare(std::string_view sql) {
  if (sqlite3_prepare_v2(db_, sql.data(), (int)sql.size(), &stmt_, 0)) {
    throw std::runtime_error(sqlite3_errmsg(db_));
  }
}
} // namespace sqlinq
