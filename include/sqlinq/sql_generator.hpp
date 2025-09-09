#ifndef SQLINQ_SQL_GENERATOR_HPP_
#define SQLINQ_SQL_GENERATOR_HPP_

#include <array>
#include <sstream>
#include <string>

#include "query_ast.hpp"

namespace sqlinq {

constexpr std::ostream &operator<<(std::ostream &os,
                                   const AggregateExpr::Function &fn) {
  switch (fn) {
  case AggregateExpr::Function::Avg:
    os << "AVG";
    break;
  case AggregateExpr::Function::Count:
    os << "COUNT";
    break;
  case AggregateExpr::Function::Min:
    os << "MIN";
    break;
  case AggregateExpr::Function::Max:
    os << "MAX";
    break;
  case AggregateExpr::Function::Sum:
    os << "SUM";
    break;
  default:
    break;
  }
  return os;
}

constexpr std::ostream &operator<<(std::ostream &os,
                                   const ValueCondition &condition) {
  if (condition.value.is_null()) {
    switch (condition.value_op) {
    case ValueCondition::Operator::Equal:
      os << " IS NULL";
      break;
    case ValueCondition::Operator::NotEqual:
      os << " IS NOT NULL";
      break;
    default:
      throw std::invalid_argument("Invalid value operator with NULL value");
    }
  } else {
    switch (condition.value_op) {
    case ValueCondition::Operator::Less:
      os << " < ?";
      break;
    case ValueCondition::Operator::LessEqual:
      os << " <= ?";
      break;
    case ValueCondition::Operator::Equal:
      os << " = ?";
      break;
    case ValueCondition::Operator::NotEqual:
      os << " != ?";
      break;
    case ValueCondition::Operator::GreaterEqual:
      os << " >= ?";
      break;
    case ValueCondition::Operator::Greater:
      os << " > ?";
      break;
    }
  }
  return os;
}

constexpr std::ostream &operator<<(std::ostream &os, const FilterChain &chain) {
  if (!chain.empty()) {
    os << " WHERE ";
    for (const auto &expr : chain) {
      if (expr.kind == FilterExpr::Kind::And) {
        os << " AND ";
      } else if (expr.kind == FilterExpr::Kind::Or) {
        os << " OR ";
      } else if (expr.kind == FilterExpr::Kind::Leaf) {
        os << expr.condition.column_name << expr.condition;
      }
    }
  }
  return os;
}

class SqlGenerator {
public:
  static std::string build_delete(const QueryAst &ast) {
    std::stringstream ss;
    ss << "DELETE FROM " << ast.table_name << ast.filter_chain;
    return ss.str();
  }

  static constexpr std::string
  build_insert(std::string_view table_name,
               const std::vector<std::string_view> columns) {
    std::string query{"INSERT INTO "};
    query += table_name;
    for (std::size_t i = 0; i < columns.size(); i++) {
      if (i == 0) {
        query += std::string{'('} + std::string{columns[i]};
        continue;
      }
      query += std::string{','} + std::string{columns[i]};
    }
    query += ") VALUES(";
    query += std::string_view{bind_string.data(), (columns.size() * 2 - 1)};
    query += ')';
    return query;
  }

  static std::string build_update(const QueryAst &ast) {
    std::stringstream ss;
    ss << "UPDATE " << ast.table_name << " SET ";
    for (std::size_t i = 0; i < ast.column_names.size(); i++) {
      if (i != 0) {
        ss << ", " << ast.column_names[i] << " = ?";
      } else {
        ss << ast.column_names[i] << " = ?";
      }
    }
    ss << ast.filter_chain;
    return ss.str();
  }

  static std::string build_select(const QueryAst &ast) {
    std::stringstream ss;
    ss << "SELECT ";
    if (ast.aggr_expr.fn() != AggregateExpr::Function::None) {
      ss << ast.aggr_expr.fn() << '(';
      if (ast.aggr_expr.is_distinct()) {
        ss << "DISTINCT ";
      }
      ss << (!ast.aggr_expr.column_name().empty() ? ast.aggr_expr.column_name()
                                                  : std::string_view{"*"});
      ss << ')';
    } else {
      if (!ast.column_names.empty()) {
        for (std::size_t i = 0; i < ast.column_names.size(); i++) {
          if (i != 0) {
            ss << ", " << ast.column_names[i];
          } else {
            ss << ast.column_names[i];
          }
        }
      } else {
        ss << '*';
      }
    }
    ss << " FROM " << ast.table_name << ast.filter_chain;
    for (std::size_t i = 0; i < ast.group_expr.size(); i++) {
      if (i == 0) {
        ss << " GROUP BY ";
        ss << ast.group_expr[i];
        continue;
      }
      ss << ',' << ast.group_expr[i];
    }

    for (std::size_t i = 0; i < ast.order_expr.size(); i++) {
      if (i == 0) {
        ss << " ORDER BY ";
        ss << ast.order_expr[i];
        continue;
      }
      ss << ',' << ast.order_expr[i];
    }
    if (ast.fetch.has_value()) {
      ss << " LIMIT " << ast.fetch.value();
    }
    if (ast.skip.has_value()) {
      ss << " OFFSET " << ast.skip.value();
    }
    return ss.str();
  }

private:
  static constexpr std::array bind_string{
      '?', ',', '?', ',', '?', ',', '?', ',', '?', ',', '?', ',', '?', ',',
      '?', ',', '?', ',', '?', ',', '?', ',', '?', ',', '?', ',', '?', ',',
      '?', ',', '?', ',', '?', ',', '?', ',', '?', ',', '?', ',', '?', ',',
      '?', ',', '?', ',', '?', ',', '?', ',', '?', ',', '?', ',', '?', ',',
      '?', ',', '?', ',', '?', ',', '?', ',', '?', ',', '?', ',', '?', ',',
      '?', ',', '?', ',', '?', ',', '?', ',', '?', ',', '?', ',', '?', ','};
};
} // namespace sqlinq

#endif // SQLINQ_SQL_GENERATOR_HPP_
