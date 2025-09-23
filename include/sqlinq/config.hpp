#ifndef SQLINQ_CONFIG_HPP_
#define SQLINQ_CONFIG_HPP_

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace sqlinq {

struct DatabaseConfig {
  std::string host;
  int port{};
  std::string user;
  std::string passwd;
  std::string database;
};

inline std::string trim(std::string s) {
  auto not_space = [](unsigned char c) { return !std::isspace(c); };
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
  s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
  return s;
}

inline std::map<std::string, DatabaseConfig>
parse_config_file(std::string_view fname) {
  std::ifstream file(fname.data());
  if (!file.is_open()) {
    throw std::runtime_error("Could not open config file: " +
                             std::string{fname});
  }

  std::map<std::string, DatabaseConfig> cfg;
  std::string section;

  std::size_t pos;
  std::string line;
  while (std::getline(file, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }

    if (line[0] == '[') {
      pos = line.find(']');
      if (pos == std::string::npos) {
        throw std::invalid_argument("Malformed section header in config file");
      }
      section = line.substr(1, pos - 1);
      cfg[section] = DatabaseConfig{};
      continue;
    }

    pos = line.find('=');
    if (pos == std::string::npos) {
      continue;
    }

    std::string key = trim(line.substr(0, pos));
    std::string val = trim(line.substr(pos + 1));

    if (section.empty()) {
      throw std::invalid_argument("key outside of section: " + key);
    }

    if (key == "host") {
      cfg[section].host = val;
    } else if (key == "port") {
      cfg[section].port = std::stoi(val);
    } else if (key == "user") {
      cfg[section].user = val;
    } else if (key == "password") {
      cfg[section].passwd = val;
    } else if (key == "database") {
      cfg[section].database = val;
    }
  }
  return cfg;
}

inline std::optional<std::string> get_env_var(std::string_view key) {
#ifdef _WIN32
  char *buf = nullptr;
  size_t sz = 0;
  if (_dupenv_s(&buf, &sz, std::string(key).c_str()) == 0 && buf != nullptr) {
    std::string val(buf);
    free(buf);
    return val;
  }
  return std::nullopt;
#else
  if (const char *val = std::getenv(std::string(key).c_str())) {
    return std::string(val);
  }
  return std::nullopt;
#endif
}

inline std::map<std::string, DatabaseConfig>
parse_env_config(const std::vector<std::string> &sections,
                 std::string_view prefix = "SQLINQ") {
  std::map<std::string, DatabaseConfig> cfg;

  for (const auto &section : sections) {
    DatabaseConfig dbc;
    auto to_key = [&](std::string sec, std::string_view key) {
      std::transform(
          sec.begin(), sec.end(), sec.begin(),
          [](const char c) -> char { return static_cast<char>(toupper(c)); });
      return std::string(prefix) + "_" + sec + "_" + std::string(key);
    };

    if (auto host = get_env_var(to_key(section, "HOST"))) {
      dbc.host = *host;
    }
    if (auto port = get_env_var(to_key(section, "PORT"))) {
      dbc.port = std::stoi(*port);
    }
    if (auto user = get_env_var(to_key(section, "USER"))) {
      dbc.user = *user;
    }
    if (auto passwd = get_env_var(to_key(section, "PASSWORD"))) {
      dbc.passwd = *passwd;
    }
    if (auto db = get_env_var(to_key(section, "DATABASE"))) {
      dbc.database = *db;
    }

    cfg[section] = std::move(dbc);
  }
  return cfg;
}
} // namespace sqlinq

#endif // SQLINQ_CONFIG_HPP_
