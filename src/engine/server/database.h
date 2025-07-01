#include <sqlite3.h>
#include <string>
#include <iostream>

class UserDatabase {
    sqlite3* db = nullptr;

public:
    struct UserInfo {
        std::string username;
        std::string password;
        int points;
        int money;
    };

    UserDatabase(const std::string& db_file) {
        if (sqlite3_open(db_file.c_str(), &db) != SQLITE_OK) {
            std::cerr << "Failed to open DB: " << sqlite3_errmsg(db) << "\n";
            db = nullptr;
        } else {
            // Create users table if not exists
            const char* sql_create =
                "CREATE TABLE IF NOT EXISTS users ("
                "username TEXT PRIMARY KEY, "
                "password TEXT NOT NULL,"
                "points INTEGER DEFAULT 0,"
                "money INTEGER DEFAULT 0)";
            char* err = nullptr;
            if (sqlite3_exec(db, sql_create, nullptr, nullptr, &err) != SQLITE_OK) {
                std::cerr << "Failed to create table: " << err << "\n";
                sqlite3_free(err);
            }
        }
    }

    ~UserDatabase() {
        if (db) sqlite3_close(db);
    }

    bool add_user(const std::string& username, const std::string& password, int points = 0, int money = 0) {
        if(user_exists(username))
        {// user already exists
            return false;
        }
        const char* sql = "INSERT INTO users (username, password, points, money) VALUES (?, ?, ?, ?);";
        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            printf("Failed to prepare insert: %s\n", sqlite3_errmsg(db));
            return false;
        }

        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 3, points);
        sqlite3_bind_int(stmt, 4, money);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            printf("Failed to add user: %s\n", sqlite3_errmsg(db));
            sqlite3_finalize(stmt);
            return false;
        }

        sqlite3_finalize(stmt);
        return true;
    }

    bool user_exists(const std::string& username)
    {
        sqlite3_stmt* stmt;
        const char* sql = "SELECT 1 FROM users WHERE username = ? LIMIT 1;";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
            return false;

        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

        bool exists = (sqlite3_step(stmt) == SQLITE_ROW);

        sqlite3_finalize(stmt);
        return exists;
    }

    bool authenticate(const std::string& username, const std::string& password) {
        if (!db) return false;
        const char* sql_query = "SELECT password FROM users WHERE username = ?;";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql_query, -1, &stmt, nullptr) != SQLITE_OK)
            return false;

        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

        bool result = false;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* db_password = sqlite3_column_text(stmt, 0);
            if (db_password && password == reinterpret_cast<const char*>(db_password)) {
                result = true;
            }
        }
        sqlite3_finalize(stmt);
        return result;
    }

    bool delete_user(const std::string& username) {
        const char* sql = "DELETE FROM users WHERE username = ?;";
        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            printf("Failed to prepare delete: %s\n", sqlite3_errmsg(db));
            return false;
        }

        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            printf("Failed to delete user: %s\n", sqlite3_errmsg(db));
            sqlite3_finalize(stmt);
            return false;
        }

        sqlite3_finalize(stmt);
        return true;
    }

    bool get_user_info(const std::string& username, UserInfo& out_info) {
        const char* sql = "SELECT username, password, points, money FROM users WHERE username = ?;";
        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            printf("Failed to prepare select: %s\n", sqlite3_errmsg(db));
            return false;
        }

        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

        int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            out_info.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            out_info.password = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            out_info.points = sqlite3_column_int(stmt, 2);
            out_info.money = sqlite3_column_int(stmt, 3);

            sqlite3_finalize(stmt);
            return true;
        } else {
            printf("User not found or error: %s\n", sqlite3_errmsg(db));
            sqlite3_finalize(stmt);
            return false;
        }
    }

    bool update_money(const std::string &username, int new_money)
    {
        const char *sql = "UPDATE users SET money = ? WHERE username = ?;";

        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
            return false;

        sqlite3_bind_int(stmt, 1, new_money);
        sqlite3_bind_text(stmt, 2, username.c_str(), -1, SQLITE_TRANSIENT);

        bool success = (sqlite3_step(stmt) == SQLITE_DONE);

        sqlite3_finalize(stmt);
        return success;
    }


    void list_users() {
        if (!db) return;
        const char* sql_query = "SELECT username FROM users;";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql_query, -1, &stmt, nullptr) != SQLITE_OK)
            return;

        std::cout << "Users in database:\n";
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* username = sqlite3_column_text(stmt, 0);
            if (username) std::cout << " - " << username << "\n";
        }
        sqlite3_finalize(stmt);
    }
};
