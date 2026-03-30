#include <stdio.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>

// Error handling function
void handle_error(int rc, sqlite3 *db) {
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(1);
    }
}

// Function to set (insert/update) a config value
void set_config(sqlite3 *db, const char *key, const char *value) {
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO Config (Key, Value) VALUES (?, ?) ON CONFLICT(Key) DO UPDATE SET Value = excluded.Value;";
    
    sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, key, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, value, -1, SQLITE_STATIC);
    
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "Failed to set config: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
}

// Function to get a config value
void get_config(sqlite3 *db, const char *key) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT Value FROM Config WHERE Key = ?;";

    sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, key, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        printf("Config [%s] = %s\n", key, sqlite3_column_text(stmt, 0));
    } else {
        printf("Config [%s] not found.\n", key);
    }

    sqlite3_finalize(stmt);
}

// Function to delete a config key
void delete_config(sqlite3 *db, const char *key) {
    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM Config WHERE Key = ?;";

    sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, key, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "Failed to delete config: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
}

int main() {
    sqlite3 *db;
    int rc;

    // Open the database (create if it doesn't exist)
    rc = sqlite3_open("config.db", &db);
    handle_error(rc, db);

    // Create the Config table if it doesn't exist
    const char *create_table_sql = "CREATE TABLE IF NOT EXISTS Config (Key TEXT PRIMARY KEY, Value TEXT NOT NULL);";
    rc = sqlite3_exec(db, create_table_sql, 0, 0, 0);
    handle_error(rc, db);

    // Set some configuration values
    set_config(db, "theme", "dark");
    set_config(db, "font_size", "12");
    set_config(db, "auto_update", "true");

    // Retrieve config values
    get_config(db, "theme");
    get_config(db, "font_size");

    // Delete a config value
    delete_config(db, "auto_update");
    get_config(db, "auto_update");

    // Close database
    sqlite3_close(db);
    return 0;
}

