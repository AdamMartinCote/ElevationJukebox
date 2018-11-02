#include "Database.hpp"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdexcept>
#include <iostream>


using namespace elevation;

Database* Database::s_instance = nullptr;
constexpr char Database::DB_NAME[] = "server.db";

Database* Database::instance() {

    if(!s_instance) {
        s_instance = new Database;
    }
    return s_instance;
}

/*
 * Returns an empty user (all 0s) on unsuccessful search
 */
void Database::getUserByMac(const char* mac,
                            User_t* __restrict__ user) const {
    int errcode = 0;
    const char* query = sqlite3_mprintf(
            "SELECT rowid, ip, mac, name, user_id FROM user WHERE (mac = '%q');", mac);

    sqlite3_stmt *statement = nullptr;
    errcode = sqlite3_prepare_v2(m_db, query, strlen(query), &statement, 0); // strlen for perfo
    if (errcode)
        throw std::runtime_error(sqlite3_errstr(errcode));

    errcode = sqlite3_step(statement);
    if (errcode == SQLITE_ROW) {
        strcpy(user->ip, (char *)sqlite3_column_text(statement, 1));
        strcpy(user->mac, (char *)sqlite3_column_text(statement, 2)); // do last as a coherence check
        strcpy(user->name, (char *)sqlite3_column_text(statement, 3));
        user->user_id = sqlite3_column_int(statement, 4);
    } else {
        *user = { 0 };
    }
    return;
}

int Database::createUser(const User_t* user) const {
    int errcode = 0;
    char* errmsg = nullptr;
    const char* query = sqlite3_mprintf(
                "INSERT OR REPLACE INTO user VALUES ('%q', '%q', '%q', %u);",
                user->ip,
                user->mac,
                user->name,
                user->user_id);

    errcode = sqlite3_exec(m_db, query, NULL, NULL, &errmsg);
    if (errcode != SQLITE_OK) {
        std::string message;
        if (errmsg) {
            message = errmsg;
            free(errmsg);
        } else {
            message = "unknown database error";
        }
        throw std::runtime_error(message);
    }
    return errcode;
} 

int Database::connectUser(const struct User_t* user) const {
    uint32_t userIsConnected = 1;
    int errcode = 0;
    char* errmsg = nullptr;
    const char* query = sqlite3_mprintf(
                "INSERT INTO userConnection VALUES (%u, %u, julianday('now'));",
                user->user_id,
                userIsConnected);
    errcode = sqlite3_exec(m_db, query, NULL, NULL, &errmsg);
    if (errcode != SQLITE_OK) {
        std::string message;
        if (errmsg) {
            message = errmsg;
            free(errmsg);
        } else {
            message = "unknown database error";
        }
        throw std::runtime_error(message);
    }
    return errcode;
}

// int Database::connectAdmin(const std::string& login, const std::string& password) const {

// }

char* Database::getSaltByLogin(const char* login) const {
    int errcode = 0;
    const char* query = sqlite3_mprintf(
            "SELECT salt FROM adminLogin WHERE (login = '%q');", login);
    sqlite3_stmt *statement = nullptr;
    errcode = sqlite3_prepare_v2(m_db, query, strlen(query), &statement, 0); // strlen for perfo
    if (errcode)
        throw std::runtime_error(sqlite3_errstr(errcode));        
    errcode = sqlite3_step(statement);
    char* salt;
    if (errcode == SQLITE_ROW) {
        salt = (char *)sqlite3_column_text(statement, 0);
    } else {
        throw std::runtime_error(sqlite3_errstr(errcode));
    }
    return salt;
}

char* Database::getHashedPasswordByLogin(const char*) const {
    int errcode = 0;
    const char* query = sqlite3_mprintf(
            "SELECT hashedPassword FROM adminLogin WHERE (login = '%q');", login);
    sqlite3_stmt *statement = nullptr;
    errcode = sqlite3_prepare_v2(m_db, query, strlen(query), &statement, 0); // strlen for perfo
    if (errcode)
        throw std::runtime_error(sqlite3_errstr(errcode));        
    errcode = sqlite3_step(statement);
    char* hashedPassword;
    if (errcode == SQLITE_ROW) {
        salt = (char *)sqlite3_column_text(statement, 0);
    } else {
        throw std::runtime_error(sqlite3_errstr(errcode));
    }
    return hashedPassword;
}

int Database::updateTimestamp(const User_t* user) const {
    int errcode = 0;
    char* errmsg = nullptr;
    const char* query = sqlite3_mprintf(
            "UPDATE userConnection SET timeStamp = julianday('now') WHERE user_id = (SELECT user_id from user where mac = '%q');", user->mac);

    errcode = sqlite3_exec(m_db, query, NULL, NULL, &errmsg);
    if (errcode != SQLITE_OK) {
        std::string message;
        if (errmsg) {
            message = errmsg;
            free(errmsg);
        } else {
            message = "unknown database error";
        }
        throw std::runtime_error(message);
    }
    return errcode;
}


Database::Database() {
    int errcode = sqlite3_open(Database::DB_NAME, &m_db);
    if (errcode) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(m_db) << std::endl;

        sqlite3_close(m_db);
        throw std::runtime_error("Cannot connect to database");
    }
}

