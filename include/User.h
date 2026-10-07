#ifndef USER_H
#define USER_H

#include <string>
#include <vector>

/*
 * UserRecord - a plain supporting data structure: one row of data/users.txt.
 *              It carries no behaviour, it only groups the six stored fields so
 *              that loading / saving code stays readable.
 */
struct UserRecord {
    std::string role;            // "STUDENT" or "ADMIN"
    std::string id;              // studentId / adminId
    std::string name;
    std::string username;
    std::string passwordHash;

    UserRecord();
    UserRecord(const std::string& role, const std::string& id,
               const std::string& name, const std::string& username,
               const std::string& passwordHash);
};

/*
 * User - the abstract base class of the whole application (abstraction).
 *
 * What it holds   : credentials (username + hashed password) and session state.
 * What it defines : the login / logout contract and the *pure virtual* menu
 *                   hook showMenu(), so that every concrete user type decides
 *                   what its own screen looks like.
 * Why it exists  : Student and Admin share "log in, be authenticated, press
 *                  buttons, log out". Only the buttons differ. main.cpp can
 *                  therefore hold a User* and drive either portal through the
 *                  same two virtual calls (polymorphism).
 *
 * Note on passwords: only a hash is stored in users.txt, never the plain text.
 */
class User {
public:
    User();
    User(const std::string& username, const std::string& password);
    virtual ~User();                                   // must be virtual!

    // ---------- encapsulation: read only access ------------------------------
    const std::string& getUsername() const;
    bool isLoggedIn() const;
    virtual std::string getRole() const = 0;            // pure virtual
    virtual std::string getDisplayName() const = 0;     // pure virtual
    virtual void showMenu() = 0;                        // pure virtual

    // ---------- behaviour ------------------------------------------------------
    bool login(const std::string& username, const std::string& password);
    void logout();

    // ---------- static helpers (shared by both portals) -------------------------
    static std::string hashPassword(const std::string& password);
    static std::vector<UserRecord> loadRecords();      // every row of users.txt
    static bool saveRecords(const std::vector<UserRecord>& records);
    static void appendRecord(const UserRecord& record);
    static bool usernameExists(const std::string& username);
    static bool usernameExistsForRole(const std::string& username, const std::string& role);
    static std::string nextIdForRole(const std::string& role, const std::string& prefix);
    static void ensureDefaultAdmin();                 // initial configuration
    static std::string toRecord(const UserRecord& record);
    static bool fromRecord(const std::string& line, UserRecord& outRecord);

    static const char* ROLE_STUDENT;
    static const char* ROLE_ADMIN;

protected:
    // ---------- protected: available to derived classes only -------------------
    void setUsername(const std::string& username);
    void setPasswordHash(const std::string& hash);
    void setLoggedIn(bool loggedIn);
    void setCredential(const std::string& username, const std::string& plainPassword);
    const std::string& getPasswordHash() const;

private:
    std::string username;
    std::string passwordHash;
    bool loggedIn;
};

#endif // USER_H