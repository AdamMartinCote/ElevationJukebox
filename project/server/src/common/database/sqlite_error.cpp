#include "sqlite_error.hpp"
#include <sqlite3/sqlite3.h>

namespace elevation {

sqlite_error::sqlite_error()
    : runtime_error(sqlite3_errstr(SQLITE_OK))
    , m_code(SQLITE_OK)
{ }

sqlite_error::sqlite_error(int code)
    : runtime_error(sqlite3_errstr(code))
    , m_code(code)
{ }

sqlite_error::sqlite_error(int code, const std::string& msg)
    : runtime_error(msg)
    , m_code(code)
{ }

sqlite_error::~sqlite_error()
{ }

int sqlite_error::code() const {
    return m_code;
}

}