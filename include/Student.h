#ifndef STUDENT_H
#define STUDENT_H

#include <string>
#include <vector>

#include "User.h"
#include "Quiz.h"
#include "Result.h"

/*
 * Student - a concrete User (inheritance).
 *
 * Inherits  : credential storage, login(), logout(), getRole().
 * Adds      : student identity (studentId + name) and the examination features
 *             registerStudent(), attemptQuiz(), viewResults(), showMenu().
 *
 * showMenu() is first shown as the anonymous "Student Portal" gate
 * (Register / Login / Back). As soon as login() succeeds the same method
 * upgrades itself into the student dashboard loop.
 */
class Student : public User {
public:
    Student();
    Student(const std::string& studentId,
            const std::string& name,
            const std::string& username,
            const std::string& plainPassword);
    virtual ~Student();

    // ---------- encapsulation: read only access ------------------------------
    const std::string& getStudentId() const;
    const std::string& getName() const;
    std::string getRole() const;                      // override -> "STUDENT"
    std::string getDisplayName() const;               // override -> name
    void showMenu() override;                         // override

    // ---------- portal features -------------------------------------------------
    static bool registerStudent();                    // interactive sign-up
    void attemptQuiz();
    void viewResults() const;
    void viewAvailableQuizzes() const;
    void viewProfile() const;

    // ---------- conversion helpers ---------------------------------------------
    UserRecord toUserRecord() const;
    static bool fromRecord(const std::string& line, Student& outStudent);

private:
    void showPortalGate();
    void showDashboard();
    bool handleRegistration();
    bool handleLogin();
    void runQuizSession();
    void displayDetailedResult(const Result& result) const;
    void loadProfile();                               // after a successful login
    bool selectResult(std::vector<Result>& owned, int& chosenIndex) const;

    std::string studentId;
    std::string name;
};

#endif // STUDENT_H