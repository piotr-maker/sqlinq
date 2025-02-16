#include <sqlinq/sqlite/database.hpp>
#include <sqlite3.h>

using namespace std;
using namespace sqlinq::sqlite;

Statement::Statement(Database &db, std::string_view sql) {
  sqlite3_prepare_v2(db.db_, sql.data(), sql.size(), &stmt_, 0);
}

int Database::backup(Database &db) {
  sqlite3_backup *backup = sqlite3_backup_init(db.db_, "main", db_, "main");
  if (backup) {
    (void)sqlite3_backup_step(backup, -1);
    (void)sqlite3_backup_finish(backup);
  }
  return sqlite3_errcode(db_);
}

int Database::restore(Database &db) {
  sqlite3_backup *backup = sqlite3_backup_init(db_, "main", db.db_, "main");
  if (backup) {
    (void)sqlite3_backup_step(backup, -1);
    (void)sqlite3_backup_finish(backup);
  }
  return sqlite3_errcode(db.db_);
}
