/*
 *  QuizMaster - Online Quiz Management and Evaluation System
 *  ------------------------------------------------------------------------
 *  main.cpp : start-up, command line handling, main menu and the polymorphic
 *             portal dispatch that ties the two User derived classes together.
 *
 *  Compile (Windows / MinGW):
 *      g++ -std=c++17 src/<each>.cpp -Iinclude -o QuizMaster.exe
 *  Compile (Linux / macOS):
 *      g++ -std=c++17 src/<each>.cpp -Iinclude -o QuizMaster
 *  (see README.md for the one line wildcard commands)
 */

#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "Admin.h"
#include "FileStore.h"
#include "Question.h"
#include "Quiz.h"
#include "Result.h"
#include "Student.h"
#include "UI.h"
#include "User.h"

namespace {

const char* APPLICATION_NAME = "QuizMaster";
const char* APPLICATION_SUBTITLE = "Online Quiz Management and Evaluation System";

void printBanner() {
    UI::banner(APPLICATION_NAME, APPLICATION_SUBTITLE);
}

// ---------------------------------------------------------------------------
//  About the project
// ---------------------------------------------------------------------------

void showAbout() {
    UI::clearScreen();
    UI::header("ABOUT THE PROJECT", "Project documentation at a glance");

    UI::section("PROJECT");
    UI::printLine("  Name    : " + std::string(APPLICATION_NAME));
    UI::printLine("  Full    : " + std::string(APPLICATION_SUBTITLE));
    UI::printLine("  Purpose : A C++ OOP based system for conducting and managing");
    UI::printLine("            online quizzes, including registration, authoring,");
    UI::printLine("            evaluation and result reporting.");

    UI::section("TECHNOLOGY");
    UI::printLine("  Language    : C++ (C++14 compatible, builds with -std=c++17)");
    UI::printLine("  Storage     : Flat files inside the data/ folder");
    UI::printLine("  Interfaces  : Standard C++ library only (no external libraries)");

    UI::section("OOP CONCEPTS DEMONSTRATED");
    std::vector<std::string> concepts;
    concepts.push_back("Abstraction     - User is an abstract base class with pure virtual methods");
    concepts.push_back("Inheritance     - Student and Admin are derived from User");
    concepts.push_back("Polymorphism    - User* drives both portals through showMenu() and getRole()");
    concepts.push_back("Encapsulation   - private attributes with getters and controlled setters");
    concepts.push_back("Composition     - a Quiz owns its Question objects");
    concepts.push_back("Constructors    - overloaded constructors plus explicit destructors");
    concepts.push_back("Static members  - shared helpers such as User::hashPassword()");
    concepts.push_back("STL containers  - std::vector, std::string, std::map, std::set");
    concepts.push_back("File handling   - escape-safe records written through a temporary file");
    concepts.push_back("Exceptions      - std::invalid_argument / std::runtime_error");
    for (size_t i = 0; i < concepts.size(); ++i) UI::printLine("  - " + concepts[i]);

    UI::section("MODULES");
    std::vector<std::string> modules;
    modules.push_back("src/User.cpp      - abstract base class, credentials, login/logout");
    modules.push_back("src/Student.cpp   - registration, dashboard, attempts, own results");
    modules.push_back("src/Admin.cpp     - question bank, quiz management, reports");
    modules.push_back("src/Quiz.cpp      - quiz composition and the quiz engine");
    modules.push_back("src/Question.cpp  - a single multiple choice item");
    modules.push_back("src/Result.cpp    - scoring, grading and result storage");
    modules.push_back("src/FileStore.cpp - flat file persistence layer");
    modules.push_back("src/UI.cpp        - console rendering and safe input handling");
    modules.push_back("src/main.cpp      - menus and portal dispatch");
    for (size_t i = 0; i < modules.size(); ++i) UI::printLine("  " + modules[i]);

    UI::section("TEAM");
    UI::printLine("  Member 1 - [Name]        (Student / User module)");
    UI::printLine("  Member 2 - [Name]        (Admin / Question management)");
    UI::printLine("  Member 3 - [Name]        (Quiz / Evaluation)");
    UI::printLine("  Member 4 - [Name]        (Result / File handling / Integration)");

    UI::printBlank();
    UI::info("Replace the placeholders above and in README.md with the real names.");
}

void showSystemStatus() {
    UI::clearScreen();
    UI::header("SYSTEM STATUS", "Data directory and stored content");

    std::vector<std::string> lines;
    lines.push_back("Data directory : " + FileStore::getDataDirectory() +
                    (FileStore::ensureDataDirectory() ? "  (available)" : "  (NOT available)"));
    lines.push_back("Users file     : " +
                    UI::count(static_cast<int>(User::loadRecords().size()), "account"));
    lines.push_back("Questions file : " +
                    UI::count(static_cast<int>(Question::loadAll().size()), "question"));
    lines.push_back("Quizzes file   : " +
                    UI::count(static_cast<int>(Quiz::loadAll().size()), "quiz", "quizzes"));
    lines.push_back("Results file   : " +
                    UI::count(static_cast<int>(Result::loadAll().size()), "result"));
    lines.push_back("");
    lines.push_back("Colours        : " + std::string(UI::isColorEnabled() ? "enabled" : "disabled"));
    lines.push_back("Data files     : users.txt, questions.txt, quizzes.txt, results.txt");
    UI::box("QUIZMASTER", lines);
}

// ---------------------------------------------------------------------------
//  polymorphic portal dispatch
// ---------------------------------------------------------------------------

/*
 * The whole point of this function: it never knows whether it received a
 * Student or an Admin. It only holds a reference to the abstract base class
 * User and calls the virtual method showMenu() - run time polymorphism.
 */
void runPortal(User& user) {
    UI::clearScreen();
    user.showMenu();              // virtual dispatch -> Student or Admin screen

    if (!user.isLoggedIn()) {
        UI::printBlank();
        UI::info("Returned from the " + user.getRole() + " portal.");
    }
}

/*
 * Both portals are owned through a std::unique_ptr<User>. This is safe only
 * because User declares a virtual destructor - if it did not, deleting a
 * Student through a User pointer would be undefined behaviour.
 */
void runStudentPortal() {
    std::unique_ptr<User> portal(new Student());
    runPortal(*portal);           // upcast: Student&  ->  User&
}

void runAdminPortal() {
    std::unique_ptr<User> portal(new Admin());
    runPortal(*portal);           // upcast: Admin&    ->  User&
}

// ---------------------------------------------------------------------------
//  main menu
// ---------------------------------------------------------------------------

void printMainMenu() {
    std::vector<std::string> items;
    items.push_back("[1] Student        - register, login and attempt quizzes");
    items.push_back("[2] Admin          - manage questions, quizzes and results");
    items.push_back("[3] About Project  - documentation, concepts and team");
    items.push_back("[4] System Status  - data files and stored records");
    items.push_back("[5] Exit");
    UI::menu(APPLICATION_NAME, APPLICATION_SUBTITLE, items, "");
}

void sayGoodbye() {
    UI::clearScreen();
    UI::printColoredLine(UI::COL_BOLD, "                        THANK YOU");
    UI::printRule('=');
    UI::printLine("  All quiz data, questions and results have been saved in the");
    UI::printLine("  '" + FileStore::getDataDirectory() + "' folder and will be available");
    UI::printLine("  the next time you start QuizMaster.");
    UI::printRule('=');
    UI::printLine("  Goodbye.");
    UI::printBlank();
}

std::string option(const std::string& flag) {
    return std::string("--") + flag;
}

void printUsage() {
    UI::printLine("Usage: " + std::string(APPLICATION_NAME) + " [options]");
    UI::printLine("  --no-color        disable ANSI colours (also used automatically when");
    UI::printLine("                    the output is redirected to a file)");
    UI::printLine("  --color           force colours on");
    UI::printLine("  --data-dir <path> use another folder for the data files");
    UI::printLine("  --help            show this help");
}

}  // namespace

int main(int argc, char* argv[]) {
    UI::initialize();

    // ---- command line ------------------------------------------------------
    std::string dataDirectory = "data";
    bool forceColor = false;
    bool disableColor = false;

    for (int i = 1; i < argc; ++i) {
        std::string argument = argv[i];
        if (argument == option("help") || argument == "-h") {
            printUsage();
            return 0;
        } else if (argument == option("no-color")) {
            disableColor = true;
        } else if (argument == option("color")) {
            forceColor = true;
        } else if (argument == option("data-dir") && i + 1 < argc) {
            dataDirectory = argv[++i];
        } else {
            UI::printLine("Unknown option: " + argument);
            printUsage();
            return 1;
        }
    }
    if (disableColor) UI::setColorEnabled(false);
    if (forceColor) UI::setColorEnabled(true);

    FileStore::setDataDirectory(dataDirectory);

    // ---- start-up: make sure the data layer is usable ----------------------
    try {
        if (!FileStore::ensureDataDirectory()) {
            UI::error("The data folder '" + dataDirectory + "' does not exist and could not "
                      "be created. Please create it manually and run QuizMaster again.");
            return 1;
        }
        FileStore::ensureDataFiles();
        User::ensureDefaultAdmin();     // initial configuration
    } catch (const std::exception& error) {
        UI::error(std::string("The data files could not be prepared: ") + error.what());
        return 1;
    }

    UI::printBlank();
    printBanner();
    UI::printBlank();
    UI::success("System ready. Data is loaded from '" + dataDirectory + "'.");
    UI::info("Demo accounts: admin/admin123  |  rahul/rahul123  |  priya/priya123");
    UI::printBlank();

    // ---- main loop ---------------------------------------------------------
    for (;;) {
        printMainMenu();
        int choice = UI::readInt("Enter your choice: ", 1, 5);

        if (UI::inputExhausted()) {          // input stream closed (piped / EOF)
            UI::printBlank();
            UI::warning("Input stream closed - leaving QuizMaster.");
            break;
        }

        switch (choice) {
            case 1: runStudentPortal(); UI::pause(); break;
            case 2: runAdminPortal();   UI::pause(); break;
            case 3: showAbout();        UI::pause(); break;
            case 4: showSystemStatus(); UI::pause(); break;
            case 5: sayGoodbye();       return 0;
            default: break;
        }
    }

    sayGoodbye();
    return 0;
}