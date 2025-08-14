#include "mysql_backend.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <mysql.h>
#include <new>
#include <sqlinq/backend/intermediate_storage.hpp>
#include <sqlinq/types.h>

namespace sqlinq {

constexpr std::size_t MAX_DECIMAL_STR_LEN = 68;

enum_field_types map_buffer_type(BindData::Type type) {
  switch (type) {
  case BindData::Bit:
    return MYSQL_TYPE_TINY;
  case BindData::Tiny:
    return MYSQL_TYPE_TINY;
  case BindData::Short:
    return MYSQL_TYPE_SHORT;
  case BindData::Long:
    return MYSQL_TYPE_LONG;
  case BindData::LongLong:
    return MYSQL_TYPE_LONGLONG;
  case BindData::Float:
    return MYSQL_TYPE_FLOAT;
  case BindData::Double:
    return MYSQL_TYPE_DOUBLE;
  case BindData::Blob:
    return MYSQL_TYPE_BLOB;
  case BindData::Text:
    return MYSQL_TYPE_STRING;
  case BindData::Date:
    return MYSQL_TYPE_DATE;
  case BindData::Time:
    return MYSQL_TYPE_TIME;
  case BindData::Datetime:
  case BindData::Timestamp:
    return MYSQL_TYPE_DATETIME;
  case BindData::Decimal:
    return MYSQL_TYPE_STRING;
    break;
  default:
    break;
  }
  return MYSQL_TYPE_NULL;
}

void map_bind_data(const sqlinq::BindData *bd, MYSQL_BIND *mb) {
  mb->buffer = bd->buffer;
  mb->buffer_length = bd->buffer_length;
  mb->buffer_type = map_buffer_type(bd->type);
  mb->length = bd->length;
  mb->error = (my_bool *)bd->error;
  mb->is_null = (my_bool *)bd->is_null;
}

void MySQLBackend::map_bind_param(const sqlinq::BindData *bd, MYSQL_BIND *mb) {
  MYSQL_TIME my_time{};
  switch (bd->type) {
  case BindData::Date: {
    sqlinq::Date date;
    std::memcpy(&date, bd->buffer, sizeof(date));
    my_time.year = (unsigned)static_cast<int>(date.year());
    my_time.month = static_cast<unsigned>(date.month());
    my_time.day = static_cast<unsigned>(date.day());
    mb->buffer = storage_.allocate<MYSQL_TIME>();
    std::memcpy(mb->buffer, &my_time, sizeof(MYSQL_TIME));
    break;
  }
  case BindData::Time: {
    sqlinq::Time time;
    std::memcpy(&time, bd->buffer, sizeof(time));
    my_time.hour = (unsigned)time.hours().count();
    my_time.minute = (unsigned)time.minutes().count();
    my_time.second = (unsigned)time.seconds().count();
    mb->buffer = storage_.allocate<MYSQL_TIME>();
    std::memcpy(mb->buffer, &my_time, sizeof(MYSQL_TIME));
    break;
  }
  case BindData::Datetime: {
    sqlinq::Datetime dt;
    std::memcpy(&dt, bd->buffer, sizeof(dt));
    std::time_t t = std::chrono::system_clock::to_time_t(dt);
    std::tm utc_tm;
    utc_tm = *gmtime(&t);
    my_time.year = (unsigned)utc_tm.tm_year + 1900;
    my_time.month = (unsigned)utc_tm.tm_mon + 1;
    my_time.day = (unsigned)utc_tm.tm_mday;
    my_time.hour = (unsigned)utc_tm.tm_hour;
    my_time.minute = (unsigned)utc_tm.tm_min;
    my_time.second = (unsigned)utc_tm.tm_sec;
    mb->buffer = storage_.allocate<MYSQL_TIME>();
    std::memcpy(mb->buffer, &my_time, sizeof(MYSQL_TIME));
    break;
  }
  case BindData::Timestamp: {
    sqlinq::Timestamp ts;
    std::memcpy(&ts, bd->buffer, sizeof(ts));
    std::time_t t = ts.count();
    std::tm utc_tm;
    utc_tm = *gmtime(&t);
    my_time.year = (unsigned)utc_tm.tm_year + 1900;
    my_time.month = (unsigned)utc_tm.tm_mon + 1;
    my_time.day = (unsigned)utc_tm.tm_mday;
    my_time.hour = (unsigned)utc_tm.tm_hour;
    my_time.minute = (unsigned)utc_tm.tm_min;
    my_time.second = (unsigned)utc_tm.tm_sec;
    mb->buffer = storage_.allocate<MYSQL_TIME>();
    std::memcpy(mb->buffer, &my_time, sizeof(MYSQL_TIME));
    break;
  }
  case BindData::Decimal: {
    sqlinq::details::DecimalRuntime decimal;
    std::memcpy(&decimal, bd->buffer, sizeof(decimal));
    std::string decimal_str = sqlinq::details::to_string(decimal);
    mb->buffer = storage_.allocate<char>(decimal_str.size());
    std::memcpy(mb->buffer, decimal_str.data(), decimal_str.size());
    mb->buffer_length = decimal_str.size();
    *bd->length = decimal_str.size();
    break;
  }
  default:
    mb->buffer = bd->buffer;
    mb->buffer_length = bd->buffer_length;
  }
  mb->buffer_type = map_buffer_type(bd->type);
  mb->length = bd->length;
  mb->error = (my_bool *)bd->error;
  mb->is_null = (my_bool *)bd->is_null;
}

void MySQLBackend::map_bind_result(const sqlinq::BindData *bd, MYSQL_BIND *mb) {
  memset(mb, 0, sizeof(MYSQL_BIND));
  switch (bd->type) {
  case BindData::Date:
  case BindData::Time:
  case BindData::Datetime:
  case BindData::Timestamp:
    mb->buffer = storage_.allocate<MYSQL_TIME>(1);
    break;
  case BindData::Decimal:
    mb->buffer = storage_.allocate<char>(MAX_DECIMAL_STR_LEN);
    mb->buffer_length = MAX_DECIMAL_STR_LEN;
    mb->length = (unsigned long *)storage_.allocate<unsigned long>(1);
    break;
  default:
    mb->buffer = bd->buffer;
    mb->buffer_length = bd->buffer_length;
    mb->length = bd->length;
  }
  mb->buffer_type = map_buffer_type(bd->type);
  mb->error = (my_bool *)bd->error;
  mb->is_null = (my_bool *)bd->is_null;
}

void MySQLBackend::bind_param(const BindData *bd, std::size_t size) {
  auto bind = std::make_unique<MYSQL_BIND[]>(size);
  memset(bind.get(), 0, sizeof(MYSQL_BIND) * size);
  for (std::size_t i = 0; i < size; i++) {
    map_bind_param(&bd[i], &bind[i]);
  }
  if (mysql_stmt_bind_param(stmt_, bind.get())) {
    throw std::runtime_error(mysql_stmt_error(stmt_));
  }
}

void MySQLBackend::bind_result(const BindData *bd, const std::size_t size) {
  bind_ = bd;
  bind_size_ = size;
  result_ = mysql_stmt_result_metadata(stmt_);
  if (result_ == nullptr) {
    throw std::runtime_error(mysql_stmt_error(stmt_));
  }

  assert(mysql_num_fields(result_) == size &&
         "MySQLBackend::bind_result(): Invalid column count");
  my_bind_ = std::make_unique<MYSQL_BIND[]>(size);
  memset(my_bind_.get(), 0, sizeof(MYSQL_BIND) * size);
  for (std::size_t i = 0; i < size; i++) {
    map_bind_result(&bd[i], &my_bind_[i]);
  }
  if (mysql_stmt_bind_result(stmt_, my_bind_.get())) {
    throw std::runtime_error(mysql_stmt_error(stmt_));
  }
}

void MySQLBackend::connect(const char *host, const char *user,
                           const char *passwd, const char *db) {
  db_ = mysql_init(NULL);
  if (db_ == nullptr) {
    throw std::bad_alloc();
  }

  db_ = mysql_real_connect(db_, host, user, passwd, db, 0, NULL, 0);
  if (db_ == nullptr) {
    throw(std::runtime_error(mysql_error(db_)));
  }
}

void MySQLBackend::disconnect() {
  if (db_ != nullptr) {
    mysql_close(db_);
    db_ = nullptr;
  }
}

uint64_t MySQLBackend::last_inserted_rowid() const noexcept {
  return mysql_insert_id(db_);
}

void MySQLBackend::stmt_close() {
  my_bind_.reset();
  storage_.clear();
  if (result_ != nullptr) {
    mysql_stmt_free_result(stmt_);
    result_ = nullptr;
  }

  if (stmt_ != nullptr) {
    mysql_stmt_close(stmt_);
    stmt_ = nullptr;
  }
}

ExecStatus MySQLBackend::stmt_execute() {
  if (mysql_stmt_execute(stmt_)) {
    throw std::runtime_error(mysql_stmt_error(stmt_));
  }
  return ExecStatus::Ok;
}

void MySQLBackend::stmt_fetch_column(const int index, BindData &bd) {
  MYSQL_BIND bind;
  map_bind_data(&bd, &bind);
  if (mysql_stmt_fetch_column(stmt_, &bind, (unsigned int)index, 0)) {
    throw std::runtime_error(mysql_stmt_error(stmt_));
  }
}

ExecStatus MySQLBackend::stmt_fetch() {
  int status = mysql_stmt_fetch(stmt_);
  if (status == 1) {
    throw std::runtime_error(mysql_stmt_error(stmt_));
  } else if (status == MYSQL_NO_DATA) {
    return ExecStatus::NoData;
  }

  for (std::size_t i = 0; i < bind_size_; i++) {
    MYSQL_TIME my_time{};
    switch (bind_[i].type) {
    case BindData::Date: {
      std::memcpy(&my_time, my_bind_[i].buffer, sizeof(my_time));
      sqlinq::Date date{std::chrono::year{(int)my_time.year},
                        std::chrono::month{my_time.month},
                        std::chrono::day{my_time.day}};
      std::memcpy(bind_[i].buffer, &date, sizeof(date));
      break;
    }
    case BindData::Time: {
      std::memcpy(&my_time, my_bind_[i].buffer, sizeof(my_time));
      sqlinq::Time time{std::chrono::hours{(int)my_time.hour} +
                        std::chrono::minutes{my_time.minute} +
                        std::chrono::seconds{my_time.second}};
      std::memcpy(bind_[i].buffer, &time, sizeof(time));
      break;
    }
    case BindData::Datetime: {
      std::tm tm{};
      std::memcpy(&my_time, my_bind_[i].buffer, sizeof(my_time));
      tm.tm_year = (int)my_time.year - 1900;
      tm.tm_mon = (int)my_time.month - 1;
      tm.tm_mday = (int)my_time.day;
      tm.tm_hour = (int)my_time.hour;
      tm.tm_min = (int)my_time.minute;
      tm.tm_sec = (int)my_time.second;
      sqlinq::Datetime dt = std::chrono::system_clock::from_time_t(timegm(&tm));
      std::memcpy(bind_[i].buffer, &dt, sizeof(dt));
      break;
    }
    case BindData::Timestamp: {
      std::tm tm{};
      std::memcpy(&my_time, my_bind_[i].buffer, sizeof(my_time));
      tm.tm_year = (int)my_time.year - 1900;
      tm.tm_mon = (int)my_time.month - 1;
      tm.tm_mday = (int)my_time.day;
      tm.tm_hour = (int)my_time.hour;
      tm.tm_min = (int)my_time.minute;
      tm.tm_sec = (int)my_time.second;
      std::time_t t = timegm(&tm);
      sqlinq::Timestamp ts{t};
      std::memcpy(bind_[i].buffer, &ts, sizeof(ts));
      break;
    }
    case BindData::Decimal: {
      std::string s{(const char *)my_bind_[i].buffer, *my_bind_[i].length};
      s.erase(std::remove(s.begin(), s.end(), '.'), s.end());
      sqlinq::Decimal<18, 0> decimal{s};
      std::memcpy(bind_[i].buffer, &decimal, sizeof(decimal));
      break;
    }
    default:
      continue;
    }
  }

  if (status == MYSQL_DATA_TRUNCATED) {
    return ExecStatus::Truncated;
  }
  return ExecStatus::Row;
}

void MySQLBackend::stmt_init() {
  stmt_ = mysql_stmt_init(db_);
  if (stmt_ == nullptr) {
    throw std::bad_alloc();
  }
}

void MySQLBackend::stmt_prepare(std::string_view sql) {
  if (mysql_stmt_prepare(stmt_, sql.data(), sql.size())) {
    throw std::runtime_error(mysql_stmt_error(stmt_));
  }
}
} // namespace sqlinq
