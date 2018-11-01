#include <sqlite3/sqlite3.h>
#include <stdint.h>
#include <cstddef>

#include "User.hpp"

#ifndef DATABASE_DATABASE_HPP
#define DATABASE_DATABASE_HPP

namespace elevation {

class Database {
public:
    static Database* instance();
    void getUserByMac(const char*, User_t* __restrict__) const;
    int createUser(const User_t*);

private:
    Database();

    sqlite3* m_db = 0;
    static Database* s_instance;
    const static char DB_NAME[];
};

} // namespace elevation


#endif // DATABASE_DATABASE_HPP