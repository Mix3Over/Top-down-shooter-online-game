#include <sqlite3.h>
#include <cstdio>
#include <string>

#include "dbService.hpp"

#define DATABASE_FILE_NAME "game.db"
#define SESSION_LIFETIME_SECONDS "604800" // 7 days

struct StmtScope {
    sqlite3_stmt* s;
    explicit StmtScope(sqlite3_stmt* stmt) : s(stmt) {}
    ~StmtScope() {
        sqlite3_reset(s);
        sqlite3_clear_bindings(s);
    }
};

static bool bindText(sqlite3_stmt* stmt, int index, std::string_view text) {
    return sqlite3_bind_text(stmt, index, text.data(), static_cast<int>(text.size()), SQLITE_TRANSIENT) == SQLITE_OK;
}

static bool exec_sql(sqlite3* db, const std::string& sql) {
    char* err = nullptr;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
        printf("SQL ERROR: %s\n", err ? err : "unknown");
        sqlite3_free(err);
        return false;
    }
    return true;
}

static bool prepareStmt(sqlite3* db, sqlite3_stmt*& stmt, const char* sql) {
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        printf("Prepare failed: %s\nSQL: %s\n", sqlite3_errmsg(db), sql);
        return false;
    }
    return true;
}

dbService::~dbService() {
    sqlite3_finalize(stmtGetPlayerData);
    sqlite3_finalize(stmtSetPlayerData);
    sqlite3_finalize(stmtCreateAccount);
    sqlite3_finalize(stmtCreatePlayerData);
    sqlite3_finalize(stmtLoginPassword);
    sqlite3_finalize(stmtLoginSessionToken);
    sqlite3_finalize(stmtSaveSessionToken);
    sqlite3_finalize(stmtClearExpiredTokens);
    sqlite3_finalize(stmtLogout);
    sqlite3_finalize(stmtDeleteAccount);

    if (db) 
        sqlite3_close(db);
}

bool dbService::initialize(){
    if(db) {
        printf("Database is already initialized!\n");
        return false;
    }

    if (sqlite3_open(DATABASE_FILE_NAME, &db) != SQLITE_OK) {
        printf("Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        db = nullptr;
        return false;
    }

    if (!exec_sql(db, "PRAGMA foreign_keys = ON;")) {
        sqlite3_close(db);
        db = nullptr;
        return false;
    }

    const std::string createLoginTable =
        "CREATE TABLE IF NOT EXISTS PlayerLoginData ("
        "   id           INTEGER PRIMARY KEY AUTOINCREMENT,"
        "   login        TEXT NOT NULL UNIQUE,"
        "   password     TEXT NOT NULL"
        ");";

    const std::string createPlayerTable =
        "CREATE TABLE IF NOT EXISTS PlayerData ("
        "   id              INTEGER PRIMARY KEY,"
        "   name            TEXT NOT NULL,"
        "   games_played    INTEGER NOT NULL DEFAULT 0,"
        "   games_won       INTEGER NOT NULL DEFAULT 0,"
        "   kills           INTEGER NOT NULL DEFAULT 0,"
        "   deaths          INTEGER NOT NULL DEFAULT 0,"
        "   FOREIGN KEY (id) REFERENCES PlayerLoginData(id) ON DELETE CASCADE"
        ");";

    const std::string createSessionTable =
        "CREATE TABLE IF NOT EXISTS PlayerSessions ("
        "   token        TEXT PRIMARY KEY,"
        "   player_id    INTEGER NOT NULL,"
        "   expires_at   INTEGER NOT NULL,"
        "   FOREIGN KEY (player_id) REFERENCES PlayerLoginData(id) ON DELETE CASCADE"
        ");";

    if (!exec_sql(db, createLoginTable) 
        || !exec_sql(db, createPlayerTable)
        || !exec_sql(db, createSessionTable)) {

        sqlite3_close(db);
        db = nullptr;
        return false;
    }

    printf("Database %s created successfully.\n", DATABASE_FILE_NAME);

    if(!prepareStmt(
        db, stmtGetPlayerData,
        "SELECT name, games_played, games_won, kills, deaths "
        "FROM PlayerData WHERE id = ?;"
    ))
        return false;

    if(!prepareStmt(
        db, stmtSetPlayerData,
        "UPDATE PlayerData "
        "SET name = ?, games_played = ?, games_won = ?, kills = ?, deaths = ? "
        "WHERE id = ?;"
    ))
        return false;

    if(!prepareStmt(
        db, stmtCreateAccount,
        "INSERT INTO PlayerLoginData (login, password) VALUES (?, ?);"
    ))
        return false;
 
    if(!prepareStmt(
        db, stmtCreatePlayerData,
        "INSERT INTO PlayerData (id, name) "
        "SELECT id, login FROM PlayerLoginData WHERE login = ?;"
    ))
        return false;
 
    if(!prepareStmt(
        db, stmtLoginPassword,
        "SELECT id FROM PlayerLoginData WHERE login = ? AND password = ?;"
    ))
        return false;
 
    if(!prepareStmt(
        db, stmtLoginSessionToken,
        "SELECT s.player_id "
        "FROM PlayerSessions s JOIN PlayerLoginData l ON l.id = s.player_id "
        "WHERE l.login = ? AND s.token = ?;"
    ))
        return false;
 
    if(!prepareStmt(
        db, stmtSaveSessionToken,
        "INSERT OR REPLACE INTO PlayerSessions (token, player_id, expires_at) "
        "VALUES (?, ?, CAST(strftime('%s', 'now') AS INTEGER) + " SESSION_LIFETIME_SECONDS ");"
    ))
        return false;
 
    if(!prepareStmt(
        db, stmtLogout,
        "DELETE FROM PlayerSessions WHERE player_id = ? AND token = ?;"
    ))
        return false;
 
    if(!prepareStmt(
        db, stmtDeleteAccount,
        "DELETE FROM PlayerLoginData "
        "WHERE password = ? AND id = (SELECT player_id FROM PlayerSessions WHERE token = ?);"
    ))
        return false;
 
    if(!prepareStmt(
        db, stmtClearExpiredTokens,
        "DELETE FROM PlayerSessions "
        "WHERE expires_at <= CAST(strftime('%s', 'now') AS INTEGER);"
    ))
        return false;
 
    return true;
}

bool dbService::createAccount(std::string_view login, std::string_view password){
    if (login.empty() || password.empty()) {
        printf("createAccount failed: login and password cannot be empty\n");
        return false;
    }
 
    if(sqlite3_exec(db, "BEGIN;", nullptr, nullptr, nullptr) != SQLITE_OK){
        printf("createAccount failed: %s\n", sqlite3_errmsg(db));
        return false;
    }
 
    StmtScope scopeCreateAccount(stmtCreateAccount);
    StmtScope scopePlayerData(stmtCreatePlayerData);
 
    if (!bindText(stmtCreateAccount, 1, login) || !bindText(stmtCreateAccount, 2, password)){
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }
 
    int result = sqlite3_step(stmtCreateAccount);
    if (result == SQLITE_CONSTRAINT) {
        printf("createAccount failed: login '%s' is already taken\n", login.data());
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }
    if (result != SQLITE_DONE) {
        printf("createAccount failed: %s\n", sqlite3_errmsg(db));
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }
 
    if (!bindText(stmtCreatePlayerData, 1, login)){
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }
 
    if (sqlite3_step(stmtCreatePlayerData) != SQLITE_DONE) {
        printf("createAccount failed: %s\n", sqlite3_errmsg(db));
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }
 
    return sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr) == SQLITE_OK;
}

bool dbService::login_password(std::string_view login, std::string_view password, std::string& sessionToken, int& playerId){
    
    playerId = -1;
    int foundId = 0;
 
    {
        StmtScope scope(stmtLoginPassword);
 
        if (!bindText(stmtLoginPassword, 1, login) || !bindText(stmtLoginPassword, 2, password))
            return false;
 
        int result = sqlite3_step(stmtLoginPassword);
        if (result == SQLITE_DONE) {
            printf("login_password failed: wrong login or password\n");
            return false;
        }
        if (result != SQLITE_ROW) {
            printf("login_password failed: %s\n", sqlite3_errmsg(db));
            return false;
        }
 
        foundId = sqlite3_column_int(stmtLoginPassword, 0);
    }
 
    // For testsing the token is just the player id
    std::string token = std::to_string(foundId);
 
    if (!saveSession(foundId, token))
        return false;
 
    sessionToken = token;
    playerId = foundId;

    return true;
}

bool dbService::login_sessionToken(std::string_view login, std::string_view sessionToken, int& playerId){
    if (!clearExpiredTokens())
        return false;
 
    int foundId = 0;
    playerId = -1;
 
    {
        StmtScope scope(stmtLoginSessionToken);
 
        if (!bindText(stmtLoginSessionToken, 1, login) || !bindText(stmtLoginSessionToken, 2, sessionToken)){
            printf("login_sessionToken failed: bind failed\n");
            return false;
        }
 
        int result = sqlite3_step(stmtLoginSessionToken);
        if (result == SQLITE_DONE) {
            printf("login_sessionToken failed: invalid or expired session\n");
            return false;
        }
        if (result != SQLITE_ROW) {
            printf("login_sessionToken failed: %s\n", sqlite3_errmsg(db));
            return false;
        }
 
        foundId = sqlite3_column_int(stmtLoginSessionToken, 0);
    }
 
    if(!saveSession(foundId, std::string(sessionToken)))
        return false;
    
    playerId = foundId;

    return true;
}

bool dbService::clearExpiredTokens(){
    StmtScope scope(stmtClearExpiredTokens);
 
    if (sqlite3_step(stmtClearExpiredTokens) != SQLITE_DONE) {
        printf("clearExpiredTokens failed: %s\n", sqlite3_errmsg(db));
        return false;
    }
 
    return true;
}
 
bool dbService::saveSession(int playerId, const std::string& token){
    StmtScope scope(stmtSaveSessionToken);
 
    if (!bindText(stmtSaveSessionToken, 1, token) || sqlite3_bind_int(stmtSaveSessionToken, 2, playerId) != SQLITE_OK){
        printf("saveSession failed: bind failed\n");
        return false;
    }
 
    if (sqlite3_step(stmtSaveSessionToken) != SQLITE_DONE) {
        printf("saveSession failed: %s\n", sqlite3_errmsg(db));
        return false;
    }
 
    return true;
}

bool dbService::logout(int playerId, std::string_view sessionToken){
    StmtScope scope(stmtLogout);
 
    if (sqlite3_bind_int(stmtLogout, 1, playerId) != SQLITE_OK || !bindText(stmtLogout, 2, sessionToken)){
        printf("logout failed: bind failed\n");
        return false;
    }
 
    if (sqlite3_step(stmtLogout) != SQLITE_DONE) {
        printf("logout failed: %s\n", sqlite3_errmsg(db));
        return false;
    }
 
    if (sqlite3_changes(db) <= 0) {
        printf("logout failed: session not found\n");
        return false;
    }
 
    return true;
}

bool dbService::deleteAccount(std::string_view sessionToken, std::string_view password){
    if (!clearExpiredTokens())
        return false;
    
    StmtScope scope(stmtDeleteAccount);
 
    if (!bindText(stmtDeleteAccount, 1, password) || !bindText(stmtDeleteAccount, 2, sessionToken)){
        printf("deleteAccount failed: Bind failed\n");
        return false;
    }
 
    if (sqlite3_step(stmtDeleteAccount) != SQLITE_DONE) {
        printf("deleteAccount failed: %s\n", sqlite3_errmsg(db));
        return false;
    }
 
    if (sqlite3_changes(db) <= 0) {
        printf("deleteAccount failed: invalid session or wrong password\n");
        return false;
    }
 
    return true;
}

bool dbService::getPlayerData(int playerId, dbPlayerData& playerData) {
    StmtScope scope(stmtGetPlayerData);
 
    if (sqlite3_bind_int(stmtGetPlayerData, 1, playerId) != SQLITE_OK)
        return false;
 
    int result = sqlite3_step(stmtGetPlayerData);
    if (result == SQLITE_DONE){
        printf("getPlayerData failed: Player with ID: %d not found\n", playerId);
        return false;
    }
    if (result != SQLITE_ROW) {
        printf("getPlayerData failed: %s\n", sqlite3_errmsg(db));
        return false;
    }
 
    playerData.name = reinterpret_cast<const char*>(sqlite3_column_text(stmtGetPlayerData, 0));
    playerData.gamesPlayed = sqlite3_column_int(stmtGetPlayerData, 1);
    playerData.gamesWon = sqlite3_column_int(stmtGetPlayerData, 2);
    playerData.kills = sqlite3_column_int(stmtGetPlayerData, 3);
    playerData.deaths = sqlite3_column_int(stmtGetPlayerData, 4);

    return true;
}
 
bool dbService::setPlayerData(int playerId, const dbPlayerData& playerData) {
    StmtScope scope(stmtSetPlayerData);
 
    if (!bindText(stmtSetPlayerData, 1, playerData.name)
        || sqlite3_bind_int(stmtSetPlayerData, 2, playerData.gamesPlayed) != SQLITE_OK
        || sqlite3_bind_int(stmtSetPlayerData, 3, playerData.gamesWon)    != SQLITE_OK
        || sqlite3_bind_int(stmtSetPlayerData, 4, playerData.kills)       != SQLITE_OK
        || sqlite3_bind_int(stmtSetPlayerData, 5, playerData.deaths)      != SQLITE_OK
        || sqlite3_bind_int(stmtSetPlayerData, 6, playerId)               != SQLITE_OK) {
        return false;
    }
 
    if (sqlite3_step(stmtSetPlayerData) != SQLITE_DONE) {
        printf("setPlayerData failed: %s\n", sqlite3_errmsg(db));
        return false;
    }

    if(sqlite3_changes(db) <= 0){
        printf("setPlayerData failed: Player with ID: %d not found!\n", playerId);
        return false;
    }
 
    return true;
}