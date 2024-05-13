#pragma once

#include <iostream>
#include "../sqlite3/sqlite3.h"
#include <vector>
#include <string>

class Database {
    sqlite3* _db;
    string _db_file;

    bool check_cmd(const int return_value, const int expected_value,
        const string error_msg, sqlite3_stmt* stmt = nullptr){
        if (return_value != expected_value){
            cerr << error_msg << sqlite3_errmsg(_db) << endl;
            if (stmt){
                sqlite3_finalize(stmt);
            }
            return false;
        }
        return true;
    }

    bool open() {
        int return_value = sqlite3_open(_db_file.c_str(), &_db);
        if (return_value != SQLITE_OK) {
            cerr << "Can't open database: " << sqlite3_errmsg(_db) << endl;
            return false;
        }
        return true;
    }

    string get_string_from_vector(const vector<string>& elems){
        string result_string;
        for (const string& elem : elems) {
            result_string += elem + ",";
        }
        // Delete last ',' if not empty
        if (!result_string.empty()) {
            result_string.pop_back();
        }
        return result_string;
    }

    bool execute_statement(const string& sql, const vector<string>& values) {
        sqlite3_stmt* stmt;
        int return_value = sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr);
        if (!check_cmd(return_value, SQLITE_OK, "Error preparing statement: ")){
            return false;
        }

        for (size_t i = 0; i < values.size(); ++i) {
            return_value = sqlite3_bind_text(stmt, i + 1, values[i].c_str(), -1, SQLITE_STATIC);
            if (!check_cmd(return_value, SQLITE_OK, "Error binding value: ", stmt)){
                return false;
            }
        }

        return_value = sqlite3_step(stmt);
        if (!check_cmd(return_value, SQLITE_DONE, "Error executing statement: ", stmt)){
            return false;
        }

        sqlite3_finalize(stmt);
        return true;
    }

    string get_friends_string(const string& username) {
        return get_string_from_vector(get_friends(username));
    }

    string get_friend_requests_string(const string& username) {
        return get_string_from_vector(get_friend_requests(username));;
    }

    vector<string> get_vector_from_sqltext(const string& username, const string &sql){
        vector<string> vec;
        sqlite3_stmt* stmt;
        int return_value = sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr);
        if (!check_cmd(return_value, SQLITE_OK, "Error preparing statement: ")){
            return vec;
        }

        return_value = sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        if (!check_cmd(return_value, SQLITE_OK, "Error binding username: ", stmt)){
            return vec;
        }

        return_value = sqlite3_step(stmt);
        if (return_value == SQLITE_ROW) {
            const unsigned char* elems_str = sqlite3_column_text(stmt, 0);
            if (elems_str) {
                string elems_string{reinterpret_cast<const char*>(elems_str)};
                size_t pos = 0;
                string delimiter = ",";
                while ((pos = elems_string.find(delimiter)) != string::npos) {
                    vec.push_back(elems_string.substr(0, pos));
                    elems_string.erase(0, pos + delimiter.length());
                }
                // Add the last elem
                // vec.push_back(elems_string);
            }
        }

        sqlite3_finalize(stmt);
        return vec;
    }

    bool add_elem_to_sqltext(const string& sqltext_string, const string& sqlkey, 
                                const string& username, const string& elem) {
        vector<string> values;
        string sql = "UPDATE users SET ";
        // Adjust the SQL to check if the current content is empty or NULL, and format accordingly
        sql += sqlkey + " = CASE WHEN " + sqlkey + " IS NULL OR " + sqlkey + " = '' THEN ? ELSE " + sqlkey + " || ? END ";
        sql += "WHERE username = ?;";

        if (sqltext_string.empty()){
            values.push_back(elem + ','); // Append the element with a comma when it's the first entry
            values.push_back(',' + elem + ','); // Add the element with leading and trailing commas for subsequent entries
            values.push_back(username);
        }
        else{
            values.push_back(elem + ','); // Append the element with a comma when it's the first entry
            values.push_back(elem + ','); // Add the element with leading and trailing commas for subsequent entries
            values.push_back(username);
        }

        return execute_statement(sql, values);
    }

public:
    Database(const string& filename) : _db(nullptr), _db_file(filename) {
        bool opened = true;
        if (!open()) {
            cerr << "Failed to open database" << endl;
            opened = false;
        }
        if (opened){
            create_table();
        }
    }

    bool create_table() {
        string create_table_sql = "CREATE TABLE IF NOT EXISTS users ("
                                    "username TEXT PRIMARY KEY,"
                                    "password TEXT,"
                                    "friends TEXT,"
                                    "friend_requests TEXT"
                                    ");";

        char *errMsg = nullptr;
        int return_value = sqlite3_exec(_db, create_table_sql.c_str(), nullptr, nullptr, &errMsg);
        if (return_value != SQLITE_OK) {
            cerr << "Error creating table: " << errMsg << endl;
            sqlite3_free(errMsg);
            return false;
        }
        return true;
    }

    bool create_user(const string& username, const string& password) {
        string sql = "INSERT INTO users (username, password) VALUES (?, ?)";
        vector<string> values{username, password};
        return execute_statement(sql, values);
    }

    bool remove_user(const string& username) {
        string sql = "DELETE FROM users WHERE username = ?;";
        vector<string> values{username};
        return execute_statement(sql, values);
    }

    vector<string> get_all_users() {
        vector<string> users;
        string sql = "SELECT username FROM users;";
        
        sqlite3_stmt* stmt;
        int return_value = sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr);
        if (!check_cmd(return_value, SQLITE_OK, "Error preparing statement: ")){
            return users;
        }

        while ((return_value = sqlite3_step(stmt)) == SQLITE_ROW) {
            const unsigned char* username = sqlite3_column_text(stmt, 0);
            if (username) {
                users.push_back(reinterpret_cast<const char*>(username));
            }
        }

        sqlite3_finalize(stmt);
        return users;
    }

    bool user_exist(const string& username){
        vector<string> users = get_all_users();
        for (auto user: users){
            if (user == username){
                return true;
            }
        }
        return false;
    }

    bool add_friend(const string& username, const string& friend_name) {
        return add_elem_to_sqltext(get_friends_string(username), "friends", username, friend_name)
            && add_elem_to_sqltext(get_friends_string(friend_name), "friends", friend_name, username);
    }

    bool add_friend_request(const string& username, const string& requester_name) {
        return add_elem_to_sqltext(get_friend_requests_string(username), "friend_requests", username, requester_name);
    }

    bool are_friends(const string user1, const string user2){
        vector<string> user1_friends = get_friends(user1);
        for (auto _friend: user1_friends){
            if (_friend == user2) return true;
        }
        return false;
    }

    vector<string> get_friends(const string& username) {
        return get_vector_from_sqltext(username, "SELECT friends FROM users WHERE username = ?;");
    }
    vector<string> get_friend_requests(const string& username) {
        return get_vector_from_sqltext(username, "SELECT friend_requests FROM users WHERE username = ?;");
    }

    bool remove_friend(const string& username, const string& friend_name) {
        string sql = "UPDATE users SET friends = replace(friends, ?, '') WHERE username = ?;";
        vector<string> values{friend_name+ ",", username};
        return execute_statement(sql, values);
    }

    bool remove_friend_requests(const string& username, const string& friend_requester) {
        string sql = "UPDATE users SET friend_requests = replace(friend_requests, ?, '') WHERE username = ?;";
        vector<string> values{friend_requester + ",", username};
        return execute_statement(sql, values);
    }

    bool is_correct_password(const string password, const string username){
        return password == get_user_password(username);
    }

    string get_user_password(const string& username) {
        string password;
        string sql = "SELECT password FROM users WHERE username = ?;";
        
        sqlite3_stmt* stmt;
        int return_value = sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr);
        if (!check_cmd(return_value, SQLITE_OK, "Error preparing statement: ")){
            return password;
        }

        return_value = sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        if (!check_cmd(return_value, SQLITE_OK, "Error binding username: ", stmt)){
            return password;
        }

        return_value = sqlite3_step(stmt);
        if (return_value == SQLITE_ROW) {
            const unsigned char* pass = sqlite3_column_text(stmt, 0);
            if (pass) {
                password = reinterpret_cast<const char*>(pass);
            }
        }

        sqlite3_finalize(stmt);
        return password;
    }

    ~Database() {
        if (_db)
            sqlite3_close(_db);
    }
};