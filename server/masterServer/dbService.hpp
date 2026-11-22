#include <string_view>
#include <string>
#include <sqlite3.h>

class dbPlayerData{
    public:
        std::string name;
        int gamesPlayed = 0;
        int gamesWon = 0;
        int kills = 0;
        int deaths = 0;
};

class dbService{
    public:
        dbService() = default;
        ~dbService();

        dbService(const dbService&) = delete;
        dbService& operator=(const dbService&) = delete;

        bool initialize();

        bool createAccount(std::string_view login, std::string_view password);
        bool login_password(std::string_view login, std::string_view password, std::string& sessionToken, int& playerId);
        bool login_sessionToken(std::string_view login, std::string_view sessionToken, int& playerId);

        bool logout(int playerId, std::string_view sessionToken);
        bool deleteAccount(std::string_view sessionToken, std::string_view password);

        bool getPlayerData(int playerId, dbPlayerData& playerData);
        bool setPlayerData(int playerId, const dbPlayerData& playerData);

    private:
        bool saveSession(int playerId, const std::string& token);
        bool clearExpiredTokens();

        sqlite3* db = nullptr;

        sqlite3_stmt* stmtGetPlayerData = nullptr;
        sqlite3_stmt* stmtSetPlayerData = nullptr;
        sqlite3_stmt* stmtCreateAccount = nullptr;
        sqlite3_stmt* stmtCreatePlayerData = nullptr;
        sqlite3_stmt* stmtLoginPassword = nullptr;
        sqlite3_stmt* stmtLoginSessionToken = nullptr;
        sqlite3_stmt* stmtSaveSessionToken = nullptr;
        sqlite3_stmt* stmtClearExpiredTokens = nullptr;
        sqlite3_stmt* stmtLogout = nullptr;
        sqlite3_stmt* stmtDeleteAccount = nullptr;
};