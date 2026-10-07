#include "User.h"

#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "FileStore.h"
#include "UI.h"

const char* User::ROLE_STUDENT = "STUDENT";
const char* User::ROLE_ADMIN = "ADMIN";

// ---------------------------------------------------------------------------
//  UserRecord
// ---------------------------------------------------------------------------

UserRecord::UserRecord()
    : role(""), id(""), name(""), username(""), passwordHash("") {}

UserRecord::UserRecord(const std::string& role, const std::string& id,
                       const std::string& name, const std::string& username,
                       const std::string& passwordHash)
    : role(role), id(id), name(name), username(username), passwordHash(passwordHash) {}

// ---------------------------------------------------------------------------
//  User - construction / destruction
// ---------------------------------------------------------------------------

User::User()
    : username(""), passwordHash(""), loggedIn(false) {}

User::User(const std::string& username, const std::string& password)
    : username(""), passwordHash(""), loggedIn(false) {
    setCredential(username, password);
}

User::~User() {
    // The session ends with the object: wipe the credential material.
    passwordHash.clear();
    loggedIn = false;
}

// ---------------------------------------------------------------------------
//  User - encapsulation: read only access
// ---------------------------------------------------------------------------

const std::string& User::getUsername() const { return username; }

bool User::isLoggedIn() const { return loggedIn; }

// ---------------------------------------------------------------------------
//  User - behaviour
// ---------------------------------------------------------------------------

bool User::login(const std::string& username, const std::string& password) {
    loggedIn = false;

    std::vector<UserRecord> records = loadRecords();
    for (size_t i = 0; i < records.size(); ++i) {
        if (records[i].username != username) continue;
        if (records[i].role != getRole()) continue;      // virtual call -> correct portal
        if (records[i].passwordHash != hashPassword(password)) continue;

        setUsername(records[i].username);
        setPasswordHash(records[i].passwordHash);
        setLoggedIn(true);
        return true;
    }
    return false;                                       // no message here, the UI reports it
}

void User::logout() {
    loggedIn = false;
    UI::info("Session closed. You have been logged out.");
}

// ---------------------------------------------------------------------------
//  User - protected helpers for derived classes
// ---------------------------------------------------------------------------

void User::setUsername(const std::string& value) { username = UI::trim(value); }

void User::setPasswordHash(const std::string& hash) { passwordHash = hash; }

void User::setLoggedIn(bool value) { loggedIn = value; }

const std::string& User::getPasswordHash() const { return passwordHash; }

void User::setCredential(const std::string& username, const std::string& plainPassword) {
    setUsername(username);
    setPasswordHash(hashPassword(plainPassword));
}

// ---------------------------------------------------------------------------
//  User - static helpers
// ---------------------------------------------------------------------------

std::string User::hashPassword(const std::string& password) {
    // FNV-1a, 64 bit. Not a cryptographic hash, but it is enough to keep plain
    // text passwords out of data/users.txt and it is easy to explain in a viva.
    unsigned long long hash = 1469598103934665603ULL;
    for (size_t i = 0; i < password.size(); ++i) {
        hash ^= static_cast<unsigned long long>(static_cast<unsigned char>(password[i]));
        hash *= 1099511628211ULL;
    }
    // mix the length in so that "ab"/"ba" cannot collide trivially
    hash ^= static_cast<unsigned long long>(password.size());
    hash *= 1099511628211ULL;

    std::ostringstream out;
    out << std::hex << std::uppercase << std::setfill('0') << std::setw(16) << hash;
    return out.str();
}

std::vector<UserRecord> User::loadRecords() {
    std::vector<UserRecord> records;
    std::string path = FileStore::usersFile();

    try {
        std::vector<std::string> lines = FileStore::readLines(path);
        for (size_t i = 0; i < lines.size(); ++i) {
            if (FileStore::isBlankLine(lines[i]) || lines[i][0] == '#') continue;

            UserRecord record;
            if (fromRecord(lines[i], record)) {
                records.push_back(record);
            } else {
                UI::warning("Skipped an unreadable user record in " + path +
                            " (line " + std::to_string(i + 1) + ").");
            }
        }
    } catch (const std::exception& error) {
        UI::warning("User file could not be read: " + std::string(error.what()));
    }
    return records;
}

bool User::saveRecords(const std::vector<UserRecord>& records) {
    try {
        std::vector<std::string> lines;
        lines.reserve(records.size());
        for (size_t i = 0; i < records.size(); ++i) {
            lines.push_back(toRecord(records[i]));
        }
        FileStore::writeLines(FileStore::usersFile(), lines);
        return true;
    } catch (const std::exception& error) {
        UI::error("Users could not be saved: " + std::string(error.what()));
        return false;
    }
}

void User::appendRecord(const UserRecord& record) {
    FileStore::appendLine(FileStore::usersFile(), toRecord(record));
}

bool User::usernameExists(const std::string& username) {
    std::vector<UserRecord> records = loadRecords();
    for (size_t i = 0; i < records.size(); ++i) {
        if (records[i].username == username) return true;
    }
    return false;
}

bool User::usernameExistsForRole(const std::string& username, const std::string& role) {
    std::vector<UserRecord> records = loadRecords();
    for (size_t i = 0; i < records.size(); ++i) {
        if (records[i].username == username && records[i].role == role) return true;
    }
    return false;
}

std::string User::nextIdForRole(const std::string& role, const std::string& prefix) {
    std::vector<UserRecord> records = loadRecords();
    int highest = 0;
    for (size_t i = 0; i < records.size(); ++i) {
        if (records[i].role != role) continue;
        if (records[i].id.compare(0, prefix.size(), prefix) != 0) continue;

        const std::string tail = records[i].id.substr(prefix.size());
        if (tail.empty()) continue;
        bool numeric = true;
        for (size_t c = 0; c < tail.size(); ++c) {
            if (tail[c] < '0' || tail[c] > '9') { numeric = false; break; }
        }
        if (!numeric) continue;

        int value = std::atoi(tail.c_str());
        if (value > highest) highest = value;
    }

    std::ostringstream out;
    out << prefix << std::setw(3) << std::setfill('0') << (highest + 1);
    return out.str();
}

void User::ensureDefaultAdmin() {
    std::vector<UserRecord> records = loadRecords();
    for (size_t i = 0; i < records.size(); ++i) {
        if (records[i].role == ROLE_ADMIN) return;             // already configured
    }

    UserRecord admin(ROLE_ADMIN, "ADM001", "System Administrator",
                     "admin", hashPassword("admin123"));
    try {
        appendRecord(admin);
        UI::warning("No administrator was found, so the initial account 'admin' "
                    "was created. Change it before using the system seriously.");
    } catch (const std::exception& error) {
        UI::error("The default administrator could not be created: " +
                  std::string(error.what()));
    }
}

std::string User::toRecord(const UserRecord& record) {
    std::vector<std::string> fields;
    fields.push_back("USER");
    fields.push_back(record.role);
    fields.push_back(record.id);
    fields.push_back(record.name);
    fields.push_back(record.username);
    fields.push_back(record.passwordHash);
    return FileStore::makeRecord(fields);
}

bool User::fromRecord(const std::string& line, UserRecord& outRecord) {
    std::vector<std::string> fields = FileStore::split(line);
    if (fields.size() != 6) return false;
    if (fields[0] != "USER") return false;

    std::string role = FileStore::unescapeField(fields[1]);
    if (role != ROLE_STUDENT && role != ROLE_ADMIN) return false;

    std::string id = FileStore::unescapeField(fields[2]);
    std::string name = FileStore::unescapeField(fields[3]);
    std::string username = FileStore::unescapeField(fields[4]);
    std::string hash = FileStore::unescapeField(fields[5]);
    if (id.empty() || name.empty() || username.empty() || hash.empty()) return false;

    outRecord = UserRecord(role, id, name, username, hash);
    return true;
}