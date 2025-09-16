#ifndef CORE_MOCK_BACKEND_HPP_
#define CORE_MOCK_BACKEND_HPP_

#include <gmock/gmock.h>
#include <sqlinq/backend/backend_iface.hpp>

using namespace sqlinq;

class MockBackend : public BackendIface {
public:
  MOCK_METHOD(void, bind_params, (std::span<BoundValue>), (override));
  MOCK_METHOD(void, bind_result, (const BindData *, const std::size_t),
              (override));

  /*MOCK_METHOD(void, connect, (), (override));*/
  /*MOCK_METHOD(void, disconnect, (), (override));*/
  MOCK_METHOD(bool, is_connected, (), (const, noexcept, override));

  MOCK_METHOD(uint64_t, last_inserted_rowid, (), (const, noexcept, override));

  MOCK_METHOD(void, stmt_close, (), (override));
  MOCK_METHOD(ExecStatus, stmt_execute, (), (override));
  MOCK_METHOD(ExecStatus, stmt_fetch, (), (override));
  MOCK_METHOD(void, stmt_fetch_column, (const int, BindData &), (override));
  MOCK_METHOD(void, stmt_init, (), (override));
  MOCK_METHOD(void, stmt_prepare, (std::string_view), (override));
};

#endif // CORE_MOCK_BACKEND_HPP_
