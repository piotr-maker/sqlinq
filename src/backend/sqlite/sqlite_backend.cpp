#include "sqlite_backend.hpp"
#include <cassert>
#include <cstring>
#include <sqlinq/types.h>
#include <stdexcept>

namespace sqlinq {

int bind_numeric_param(sqlite3_stmt *stmt, const int index,
                       const BindData &bind) {
  if (bind.buffer == nullptr) {
    return SQLITE_ERROR;
  }

  switch (bind.type) {
  case BindData::Bit: {
    bool val = *(bool *)bind.buffer;
    return sqlite3_bind_int(stmt, index, val);
  }
  case BindData::Tiny: {
    int8_t val = *(int8_t *)bind.buffer;
    return sqlite3_bind_int(stmt, index, val);
  }
  case BindData::Short: {
    int16_t val = *(int16_t *)bind.buffer;
    return sqlite3_bind_int(stmt, index, val);
  }
  case BindData::Long: {
    int32_t val = *(int32_t *)bind.buffer;
    return sqlite3_bind_int(stmt, index, val);
  }
  case BindData::LongLong: {
    int64_t val = *(int64_t *)bind.buffer;
    return sqlite3_bind_int64(stmt, index, val);
  }
  default:
    break;
  }
  return SQLITE_ERROR;
}

int bind_floating_param(sqlite3_stmt *stmt, const int index,
                        const BindData &bind) {
  if (bind.buffer == nullptr) {
    return SQLITE_ERROR;
  }

  switch (bind.type) {
  case BindData::Float:
    return sqlite3_bind_double(stmt, index, *(float *)bind.buffer);
  case BindData::Double:
    return sqlite3_bind_double(stmt, index, *(double *)bind.buffer);
  default:
    break;
  }
  return SQLITE_ERROR;
}

int bind_container_param(sqlite3_stmt *stmt, const int index,
                         const BindData &bind) {
  if (bind.buffer == nullptr || bind.buffer_length == 0) {
    return SQLITE_ERROR;
  }

  int length = (int)bind.buffer_length;
  switch (bind.type) {
  case BindData::Blob:
    return sqlite3_bind_blob(stmt, index, bind.buffer, length, NULL);
  case BindData::Text:
    return sqlite3_bind_text(stmt, index, (char *)bind.buffer, length, NULL);
  default:
    break;
  }
  return SQLITE_ERROR;
}

int bind_datetime_param(sqlite3_stmt *stmt, const int index,
                        const BindData &bind) {
  if (bind.buffer == nullptr) {
    return SQLITE_ERROR;
  }

  std::string s;
  switch (bind.type) {
  case BindData::Date: {
    sqlinq::Date date;
    memcpy(&date, bind.buffer, sizeof(date));
    s = sqlinq::to_string(date);
    return sqlite3_bind_text(stmt, index, (char *)s.data(), (int)s.size(),
                             SQLITE_TRANSIENT);
  }
  case BindData::Time: {
    sqlinq::Time time;
    memcpy(&time, bind.buffer, sizeof(time));
    s = sqlinq::to_string(time);
    return sqlite3_bind_text(stmt, index, (char *)s.data(), (int)s.size(),
                             SQLITE_TRANSIENT);
  }
  case BindData::Datetime: {
    sqlinq::Datetime dt;
    memcpy(&dt, bind.buffer, sizeof(dt));
    s = sqlinq::to_string(dt);
    return sqlite3_bind_text(stmt, index, (char *)s.data(), (int)s.size(),
                             SQLITE_TRANSIENT);
  }
  case BindData::Timestamp: {
    sqlinq::Timestamp ts;
    memcpy(&ts, bind.buffer, sizeof(ts));
    long long count = ts.count();
    return sqlite3_bind_int64(stmt, index, count);
  }
  default:
    break;
  }
  return SQLITE_OK;
}
int bind_decimal_param(sqlite3_stmt *stmt, const int index,
                       const BindData &bind) {
  if (bind.buffer == nullptr) {
    return SQLITE_ERROR;
  }

  sqlinq::Decimal<8, 2> decimal;
  memcpy(&decimal, bind.buffer, sizeof(decimal));
  int64_t raw = static_cast<int64_t>(decimal);
  return sqlite3_bind_int64(stmt, index, raw);
}

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
  case BindData::Bit:
    *(bool *)(bind.buffer) = (bool)sqlite3_column_int(stmt, index);
    break;
  case BindData::Tiny:
    *(char *)(bind.buffer) = (char)sqlite3_column_int(stmt, index);
    break;
  case BindData::Short:
    *(short *)(bind.buffer) = (short)sqlite3_column_int(stmt, index);
    break;
  case BindData::Long:
    *(long *)(bind.buffer) = (long)sqlite3_column_int(stmt, index);
    break;
  case BindData::LongLong:
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
  case BindData::Float:
    *(float *)(bind.buffer) = (float)sqlite3_column_double(stmt, index);
    break;
  case BindData::Double:
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
    if (bind.type == BindData::Blob) {
      data = (const char *)sqlite3_column_blob(stmt, index);
    } else if (bind.type == BindData::Text) {
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
  if (bind.type == BindData::Timestamp) {
    assert(col_type == SQLITE_INTEGER && "Invalid column type");
  } else {
    assert(col_type == SQLITE_TEXT && "Invalid column type");
  }
  if (bind.error != nullptr) {
    *bind.error = bind.type == BindData::Timestamp ? col_type == SQLITE_INTEGER
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
  case BindData::Date: {
    data = (const char *)sqlite3_column_text(stmt, index);
    sqlinq::Date date;
    sqlinq::from_string(data, date);
    memcpy(bind.buffer, (void *)&date, sizeof(date));
    break;
  }
  case BindData::Time: {
    data = (const char *)sqlite3_column_text(stmt, index);
    sqlinq::Time time;
    sqlinq::from_string(data, time);
    memcpy(bind.buffer, (void *)&time, sizeof(time));
    break;
  }
  case BindData::Datetime: {
    data = (const char *)sqlite3_column_text(stmt, index);
    sqlinq::Datetime dt;
    sqlinq::from_string(data, dt);
    memcpy(bind.buffer, (void *)&dt, sizeof(dt));
    break;
  }
  case BindData::Timestamp: {
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

void SQLiteBackend::bind_param(const BindData *bd, const std::size_t size) {
  int rc = SQLITE_OK;
  sqlite3_reset(stmt_);
  sqlite3_clear_bindings(stmt_);
  for (int index = 1; index <= static_cast<int>(size); index++) {
    const BindData *bind = &bd[index - 1];
    if (*bind->is_null) {
      if (sqlite3_bind_null(stmt_, index)) {
        throw std::runtime_error(sqlite3_errmsg(db_));
      }
      continue;
    }
    switch (bind->group()) {
    case BindData::Group::Integral:
      rc = bind_numeric_param(stmt_, index, *bind);
      break;
    case BindData::Group::Floating:
      rc = bind_floating_param(stmt_, index, *bind);
      break;
    case BindData::Group::Container:
      rc = bind_container_param(stmt_, index, *bind);
      break;
    case BindData::Group::Datetime:
      rc = bind_datetime_param(stmt_, index, *bind);
      break;
    case BindData::Group::Decimal:
      rc = bind_decimal_param(stmt_, index, *bind);
      break;
    default:
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
    switch (bind->group()) {
    case BindData::Group::Integral:
      fetch_numeric_column(stmt_, index, *bind);
      break;
    case BindData::Group::Floating:
      fetch_floating_column(stmt_, index, *bind);
      break;
    case BindData::Group::Container:
      truncated_ = fetch_container_column(stmt_, index, *bind);
      break;
    case BindData::Group::Datetime:
      fetch_datetime_column(stmt_, index, *bind);
      break;
    case BindData::Group::Decimal:
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
