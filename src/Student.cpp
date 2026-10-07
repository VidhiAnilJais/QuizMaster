#include "Student.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "FileStore.h"
#include "UI.h"

namespace {
const int NAME_MIN_LENGTH = 3;
const int NAME_MAX_LENGTH = 50;
const int USERNAME_MIN_LENGTH = 3;
const int USERNAME_MAX_LENGTH = 20;
const int PASSWORD_MIN_LENGTH = 4;

bool isValidName(const std::string& name) {
    if (static_cast<int>(name.size()) < NAME_MIN_LENGTH) return false;
    if (static_cast<int>(name.size()) > NAME_MAX_LENGTH) return false;

    int letters = 0;
    for (size_t i = 0; i < name.size(); ++i) {
        char c = name[i];
        if (std::isalpha(static_cast<unsigned char>(c))) {
            ++letters;
        } else if (c != ' ' && c != '.' && c != '-' && c != '\'') {
            return false;                       // digits / symbols are not accepted
        }
    }
    return letters >= NAME_MIN_LENGTH;
}

bool isValidUsername(const std::string& username) {
    if (static_cast<int>(username.size()) < USERNAME_MIN_LENGTH) return false;
    if (static_cast<int>(username.size()) > USERNAME_MAX_LENGTH) return false;
    return UI::isAlphanumeric(username);
}
}

// ---------------------------------------------------------------------------
//  construction / destruction
// ---------------------------------------------------------------------------

Student::Student()
    : User(), studentId(""), name("") {}

Student::Student(const std::string& studentId, const std::string& name,
                 const std::string& username, const std::string& plainPassword)
    : User(username, plainPassword), studentId(studentId), name(name) {}

Student::~Student() {
    // The base class destructor wipes the credential hash for us.
}

// ---------------------------------------------------------------------------
//  encapsulation: read only access
// ---------------------------------------------------------------------------

const std::string& Student::getStudentId() const { return studentId; }
const std::string& Student::getName() const { return name; }

std::string Student::getRole() const { return ROLE_STUDENT; }

std::string Student::getDisplayName() const {
    return name.empty() ? getUsername() : name;
}

// ---------------------------------------------------------------------------
//  polymorphism entry point
// ---------------------------------------------------------------------------

void Student::showMenu() {
    if (isLoggedIn()) {
        showDashboard();
    } else {
        showPortalGate();
    }
}

// ---------------------------------------------------------------------------
//  the student portal gate (not logged in yet)
// ---------------------------------------------------------------------------

void Student::showPortalGate() {
    for (;;) {
        UI::clearScreen();
        std::vector<std::string> items;
        items.push_back("[1] Register      - create a new student account");
        items.push_back("[2] Login         - use an existing account");
        items.push_back("[3] Back          - return to the main menu");
        UI::menu("STUDENT PORTAL", "Online examination area", items, "");

        int choice = UI::readInt("Enter your choice: ", 1, 3);
        if (UI::inputExhausted()) return;

        if (choice == 1) {
            handleRegistration();
            UI::pause();
        } else if (choice == 2) {
            if (handleLogin()) {
                showDashboard();             // straight into the dashboard
                return;
            }
            UI::pause();
        } else {
            return;
        }
    }
}

bool Student::handleRegistration() {
    return registerStudent();
}

bool Student::handleLogin() {
    UI::header("STUDENT LOGIN", "Enter the credentials you registered with");
    std::string username = UI::readNonEmpty("  Username : ");
    std::string password = UI::readPassword("  Password : ");
    if (UI::inputExhausted()) return false;

    if (!login(username, password)) {
        UI::error("Invalid username or password.");
        return false;
    }

    loadProfile();
    UI::success("Login successful. Welcome, " + getDisplayName() + "!");
    return true;
}

void Student::loadProfile() {
    studentId.clear();
    name.clear();

    std::vector<UserRecord> records = User::loadRecords();
    for (size_t i = 0; i < records.size(); ++i) {
        if (records[i].role == User::ROLE_STUDENT && records[i].username == getUsername()) {
            studentId = records[i].id;
            name = records[i].name;
            return;
        }
    }
    UI::warning("Your profile details could not be read from the user file.");
}

// ---------------------------------------------------------------------------
//  registration
// ---------------------------------------------------------------------------

bool Student::registerStudent() {
    UI::header("STUDENT REGISTRATION", "Create your examination account (all fields are required)");

    std::string fullName;
    for (;;) {
        fullName = UI::readNonEmpty("  Full name           : ");
        if (UI::inputExhausted()) return false;
        if (isValidName(fullName)) break;
        UI::error("Enter a valid name (letters, spaces, dots or hyphens, " +
                  std::to_string(NAME_MIN_LENGTH) + "-" + std::to_string(NAME_MAX_LENGTH) +
                  " characters).");
    }

    std::string username;
    for (;;) {
        username = UI::readNonEmpty("  Username            : ");
        if (UI::inputExhausted()) return false;
        if (!isValidUsername(username)) {
            UI::error("The username must be " + std::to_string(USERNAME_MIN_LENGTH) + "-" +
                      std::to_string(USERNAME_MAX_LENGTH) +
                      " characters long and may only contain letters, digits, '_', '.' or '-'.");
            continue;
        }
        if (User::usernameExists(username)) {
            UI::error("Username already exists.");
            continue;
        }
        break;
    }

    std::string password;
    for (;;) {
        password = UI::readPassword("  Password            : ");
        if (UI::inputExhausted()) return false;
        if (static_cast<int>(password.size()) < PASSWORD_MIN_LENGTH) {
            UI::error("The password must contain at least " +
                      std::to_string(PASSWORD_MIN_LENGTH) + " characters.");
            continue;
        }
        std::string again = UI::readPassword("  Confirm password    : ");
        if (UI::inputExhausted()) return false;
        if (password != again) {
            UI::error("The two passwords do not match.");
            continue;
        }
        break;
    }

    const std::string newId = User::nextIdForRole(User::ROLE_STUDENT, "STU");

    std::vector<std::string> lines;
    lines.push_back("  Student ID : " + newId + "   (generated automatically)");
    lines.push_back("  Name       : " + fullName);
    lines.push_back("  Username   : " + username);
    lines.push_back("");
    lines.push_back("  Please note your password - it cannot be recovered later.");
    UI::box("REGISTRATION SUMMARY", lines);

    if (!UI::confirm("Save this student account?")) {
        UI::info("Registration cancelled. Nothing was saved.");
        return false;
    }

    UserRecord record(User::ROLE_STUDENT, newId, fullName, username,
                      User::hashPassword(password));
    try {
        User::appendRecord(record);
    } catch (const std::exception& error) {
        UI::error("The account could not be saved: " + std::string(error.what()));
        return false;
    }

    UI::success("Account created successfully. You can log in now.");
    return true;
}

// ---------------------------------------------------------------------------
//  the student dashboard (logged in)
// ---------------------------------------------------------------------------

void Student::showDashboard() {
    for (;;) {
        UI::clearScreen();
        UI::printColoredLine(UI::COL_BOLD, "STUDENT DASHBOARD");
        UI::printColoredLine(UI::COL_CYAN, "Welcome, " + getDisplayName() +
                                         "  (" + studentId + ")");
        UI::printRule('-');
        UI::printLine("  [1] Available Quizzes");
        UI::printLine("  [2] Start Quiz");
        UI::printLine("  [3] My Results");
        UI::printLine("  [4] My Profile");
        UI::printLine("  [5] Logout");
        UI::printRule('-');

        int choice = UI::readInt("Enter your choice: ", 1, 5);
        if (UI::inputExhausted()) return;

        if (choice == 1) {
            viewAvailableQuizzes();
            UI::pause();
        } else if (choice == 2) {
            runQuizSession();
        } else if (choice == 3) {
            viewResults();
            UI::pause();
        } else if (choice == 4) {
            viewProfile();
            UI::pause();
        } else {
            logout();
            return;
        }
    }
}

// ---------------------------------------------------------------------------
//  quizzes
// ---------------------------------------------------------------------------

void Student::viewAvailableQuizzes() const {
    UI::header("AVAILABLE QUIZZES", "Every quiz published by the administrator");

    std::vector<Quiz> quizzes = Quiz::loadAll();
    if (quizzes.empty()) {
        UI::warning("No quiz has been created yet. Please check back later.");
        return;
    }

    UI::printLine("   #   QUIZ ID  TITLE                              QUESTIONS");
    UI::printRule('-');
    for (size_t i = 0; i < quizzes.size(); ++i) {
        std::string title = quizzes[i].getTitle();
        if (title.size() > 34) title = title.substr(0, 31) + "...";
        std::string status = quizzes[i].isEmpty() ? "[not published]" :
                             UI::count(quizzes[i].getTotalQuestions(), "question");
        UI::printLine("  " + UI::padLeft(std::to_string(i + 1), 3) + "  " +
                      UI::padLeft("QUIZ " + std::to_string(quizzes[i].getQuizId()), 8) + "  " +
                      UI::padRight(title, 35) + "  " + status);
    }
    UI::printRule('-');

    if (!UI::confirm("View the details of one quiz?")) return;
    int choice = UI::readInt("  Quiz number (1-" + std::to_string(quizzes.size()) + "): ",
                             1, static_cast<int>(quizzes.size()));
    if (UI::inputExhausted()) return;

    const Quiz& selected = quizzes[static_cast<size_t>(choice - 1)];
    std::vector<std::string> lines;
    lines.push_back("Title       : " + selected.getTitle());
    lines.push_back("Quiz ID     : " + std::to_string(selected.getQuizId()));
    lines.push_back("Questions   : " + std::to_string(selected.getTotalQuestions()));
    lines.push_back("");
    lines.push_back("Description :");
    lines.push_back(selected.getDescription().empty() ? "  (no description provided)"
                                                     : selected.getDescription());
    UI::box("QUIZ DETAILS", lines);
}

void Student::runQuizSession() {
    UI::header("START A QUIZ", "Choose the quiz you want to attempt");

    std::vector<Quiz> quizzes;
    {
        std::vector<Quiz> allQuizzes = Quiz::loadAll();
        for (size_t i = 0; i < allQuizzes.size(); ++i) {
            if (!allQuizzes[i].isEmpty()) quizzes.push_back(allQuizzes[i]);
        }
    }

    if (quizzes.empty()) {
        UI::warning("There is no quiz you can attempt right now.");
        return;
    }

    for (size_t i = 0; i < quizzes.size(); ++i) quizzes[i].display();
    UI::printRule('-');

    int choice = UI::readInt("Quiz number (1-" + std::to_string(quizzes.size()) + "): ",
                             1, static_cast<int>(quizzes.size()));
    if (UI::inputExhausted()) return;

    const Quiz& selected = quizzes[static_cast<size_t>(choice - 1)];

    std::vector<std::string> lines;
    lines.push_back("You are about to attempt:");
    lines.push_back("  " + selected.getTitle());
    lines.push_back("");
    lines.push_back("Questions  : " + UI::count(selected.getTotalQuestions(), "question"));
    lines.push_back("Pass mark  : " + std::to_string(Result::PASS_PERCENTAGE) + "%");
    lines.push_back("");
    lines.push_back("The quiz cannot be paused and answers cannot be changed");
    lines.push_back("after they are submitted.");
    UI::box("PLEASE READ", lines);

    if (!UI::confirm("Begin the quiz now?")) {
        UI::info("Quiz cancelled.");
        return;
    }

    UI::clearScreen();
    Result result;
    try {
        result = selected.startQuiz(studentId, name);
    } catch (const std::exception& error) {
        UI::error(std::string("The quiz could not be started: ") + error.what());
        return;
    }

    try {
        result.saveResult();
        UI::success("Your result has been saved (Result ID " +
                    std::to_string(result.getResultId()) + ").");
    } catch (const std::exception& error) {
        UI::error(std::string("The result could not be saved: ") + error.what());
    }

    displayDetailedResult(result);
}

void Student::displayDetailedResult(const Result& result) const {
    result.displayResult();

    std::vector<std::string> items;
    items.push_back("[1] View the detailed answer breakdown");
    items.push_back("[2] Return to the dashboard");
    UI::menu("QUIZ COMPLETED", "What would you like to do next?", items, "");

    int choice = UI::readInt("Enter your choice: ", 1, 2);
    if (UI::inputExhausted()) return;

    if (choice != 1) return;

UI::clearScreen();
    UI::header("DETAILED RESULT",
               result.getQuizTitle() + "  |  Result ID " + std::to_string(result.getResultId()));
    result.displayResult();
    UI::pause();
}

// ---------------------------------------------------------------------------
//  results
// ---------------------------------------------------------------------------

void Student::viewResults() const {
    UI::header("MY RESULTS", "Every quiz you have attempted so far");

    std::vector<Result> allResults = Result::loadAll();
    std::vector<Result> mine;
    for (size_t i = allResults.size(); i > 0; --i) {      // newest first
        if (allResults[i - 1].getStudentId() == studentId) mine.push_back(allResults[i - 1]);
    }

    if (mine.empty()) {
        UI::warning("You have not attempted any quiz yet.");
        return;
    }

    UI::printLine("  " + UI::padLeft("ID", 4) + "  " + UI::padRight("QUIZ", 30) + " " +
                  UI::padLeft("SCORE", 7) + "  " + UI::padLeft("PERCENT", 7) + "  " +
                  UI::padLeft("PERFORMANCE", 18) + " ATTEMPTED ON");
    UI::printRule('-');
    for (size_t i = 0; i < mine.size(); ++i) {
        UI::printLine(mine[i].getSummaryLine());
    }
    UI::printRule('-');

    double totalPercentage = 0.0;
    int passed = 0;
    for (size_t i = 0; i < mine.size(); ++i) {
        totalPercentage += mine[i].getPercentage();
        if (mine[i].isPassed()) ++passed;
    }
    UI::printLine("  Attempts: " + std::to_string(mine.size()) +
                  "     Passed: " + std::to_string(passed) +
                  "     Average: " + UI::fixed(totalPercentage / mine.size(), 2) + "%");

    if (!UI::confirm("Open one of these results?")) return;
    int choice = UI::readInt("Result number (1-" + std::to_string(mine.size()) + "): ",
                             1, static_cast<int>(mine.size()));
    if (UI::inputExhausted()) return;

    const Result& selected = mine[static_cast<size_t>(choice - 1)];
    UI::clearScreen();
    UI::header("RESULT DETAILS",
               selected.getQuizTitle() + "  |  Result ID " + std::to_string(selected.getResultId()));
    if (selected.getAnswers().empty()) {
        UI::warning("This result was stored without an answer breakdown.");
    }
    selected.displayResult();          // report card + per question breakdown
}

void Student::viewProfile() const {
    UI::header("MY PROFILE", "Account information");

    int attempts = 0;
    int passed = 0;
    double totalPercentage = 0.0;
    std::vector<Result> results = Result::loadAll();
    for (size_t i = 0; i < results.size(); ++i) {
        if (results[i].getStudentId() != studentId) continue;
        ++attempts;
        totalPercentage += results[i].getPercentage();
        if (results[i].isPassed()) ++passed;
    }

    std::vector<std::string> lines;
    lines.push_back("Student ID  : " + studentId);
    lines.push_back("Name        : " + name);
    lines.push_back("Username    : " + getUsername());
    lines.push_back("Role        : " + getRole());
    lines.push_back("Session     : " + std::string(isLoggedIn() ? "logged in" : "logged out"));
    lines.push_back("Password    : stored as a hash, never in plain text");
    lines.push_back("------------------------------------------");
    lines.push_back("Quiz attempts: " + std::to_string(attempts));
    lines.push_back("Passed       : " + std::to_string(passed));
    lines.push_back("Average score: " +
                    (attempts > 0 ? UI::fixed(totalPercentage / attempts, 2) + "%" : "n/a"));
    UI::box("PROFILE", lines);
}

// ---------------------------------------------------------------------------
//  conversion helpers
// ---------------------------------------------------------------------------

UserRecord Student::toUserRecord() const {
    return UserRecord(getRole(), studentId, name, getUsername(), getPasswordHash());
}

bool Student::fromRecord(const std::string& line, Student& outStudent) {
    UserRecord record;
    if (!User::fromRecord(line, record)) return false;
    if (record.role != User::ROLE_STUDENT) return false;

    Student parsed(record.id, record.name, record.username, "");
    parsed.setPasswordHash(record.passwordHash);
    outStudent = parsed;
    return true;
}