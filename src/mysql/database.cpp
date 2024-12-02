#include <sqlinq/mysql/database.h>

using namespace std;
using namespace sqlinq::mysql;

template <std::size_t K, std::size_t N>
result<K, N>::result(statement<N> &stmt) : stmt_(std::move(stmt)) {
  if(stmt_.stmt_ == nullptr) {
    return;
  }
  result_ = mysql_stmt_result_metadata(stmt_);
  fields_ = mysql_fetch_fields(result_);
}
