#include <stdio.h>
#include <sqlite3.h>
#include <stdlib.h>

void handle_error(int rc, sqlite3 *db) {
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(1);
    }
}

int main() {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    // Open (or create) the database
    rc = sqlite3_open("example.db", &db);
    handle_error(rc, db);

    // Create table if not exists
    const char *create_table_sql = "CREATE TABLE IF NOT EXISTS Users (ID INTEGER PRIMARY KEY, Name TEXT NOT NULL);";
    rc = sqlite3_exec(db, create_table_sql, 0, 0, 0);
    handle_error(rc, db);

    // === INSERT DATA using Prepared Statements ===
    const char *insert_sql = "INSERT INTO Users (Name) VALUES (?);";
    rc = sqlite3_prepare_v2(db, insert_sql, -1, &stmt, 0);
    handle_error(rc, db);

    // Bind value safely
    sqlite3_bind_text(stmt, 1, "Alice", -1, SQLITE_STATIC);
    
    // Execute the statement
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Insert failed: %s\n", sqlite3_errmsg(db));
    }
    
    // Clean up
    sqlite3_finalize(stmt);

    // === SELECT DATA ===
    const char *select_sql = "SELECT ID, Name FROM Users;";
    rc = sqlite3_prepare_v2(db, select_sql, -1, &stmt, 0);
    handle_error(rc, db);

    printf("\nUsers in Database:\n");
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const unsigned char *name = sqlite3_column_text(stmt, 1);
        printf("ID: %d, Name: %s\n", id, name);
    }

    // Clean up
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return 0;
}

