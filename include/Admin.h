#ifndef ADMIN_H
#define ADMIN_H

#include <string>
#include <vector>

#include "User.h"
#include "Question.h"
#include "Quiz.h"
#include "Result.h"

/*
 * Admin - a concrete User (inheritance).
 *
 * Inherits  : credential storage, login(), logout(), getRole().
 * Owns      : the Question Bank and the Quiz collection while the admin session
 *             is open (loaded from disk on login, saved on every change).
 * Adds      : content management (createQuiz, addQuestion, updateQuestion,
 *             deleteQuestion) and reporting (viewResults, quizStatistics).
 */
class Admin : public User {
public:
    Admin();
    Admin(const std::string& adminId,
          const std::string& name,
          const std::string& username,
          const std::string& plainPassword);
    virtual ~Admin();

    // ---------- encapsulation: read only access ------------------------------
    const std::string& getAdminId() const;
    const std::string& getName() const;
    std::string getRole() const;                      // override -> "ADMIN"
    std::string getDisplayName() const;               // override -> name
    void showMenu() override;                         // override

    // ---------- question bank management ---------------------------------------
    void addQuestion();
    void updateQuestion();
    void deleteQuestion();
    void viewQuestions() const;                       // question bank + usage

    // ---------- quiz management -------------------------------------------------
    void createQuiz();
    void viewQuizzes() const;
    void viewQuizQuestions() const;
    void addQuestionToQuiz();
    void removeQuestionFromQuiz();
    void deleteQuiz();

    // ---------- reporting ---------------------------------------------------------
    void viewResults() const;
    void quizStatistics() const;
    void viewProfile() const;

    // ---------- conversion helpers -------------------------------------------------
    UserRecord toUserRecord() const;
    static bool fromRecord(const std::string& line, Admin& outAdmin);

private:
    // persistence
    void loadData();
    void saveQuestions() const;
    void saveQuizzes() const;
    bool showLoginScreen();             // returns true when the login succeeded

    // lookup / selection helpers (return index, -1 when not found)
    int indexOfQuestion(int questionId) const;
    int indexOfQuiz(int quizId) const;
    int askQuestionId(const std::string& prompt) const;
    int askQuizId(const std::string& prompt) const;
    int selectQuiz(const std::string& title) const;

    // question editing
    void questionForm(Question& question, const Question* fallback) const;
    void collectQuestionText(int questionId, Question& question) const;

    void displayQuestionTable(const std::vector<Question>& questions,
                              const std::string& caption) const;
    void displayQuizTable(const std::vector<Quiz>& quizzes) const;

    std::string adminId;
    std::string name;
    std::vector<Question> questionBank;               // master pool
    std::vector<Quiz> quizzes;
};

#endif // ADMIN_H