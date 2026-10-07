#ifndef QUIZ_H
#define QUIZ_H

#include <string>
#include <vector>

#include "Question.h"
#include "Result.h"

/*
 * Quiz - a titled collection of Question objects.
 *
 * Composition: a Quiz *owns* its questions (std::vector<Question>). Adding a
 * question copies the object into the quiz, deleting a question removes the
 * copy. The master Question Bank (data/questions.txt) is owned by Admin and is
 * a different thing: it is the pool the admin picks from while composing a quiz.
 *
 * startQuiz() is the interactive examination engine: it asks every question,
 * validates the answer, then hands the raw counts to a Result object, which is
 * responsible for scoring and storing itself.
 */
class Quiz {
public:
    Quiz();
    Quiz(int quizId, const std::string& title, const std::string& description);
    Quiz(const Quiz& other);
    Quiz& operator=(const Quiz& other);
    ~Quiz();

    // ---------- encapsulation: read only access ------------------------------
    int getQuizId() const;
    const std::string& getTitle() const;
    const std::string& getDescription() const;
    const std::vector<Question>& getQuestions() const;
    int getTotalQuestions() const;
    bool isEmpty() const;

    // ---------- encapsulation: controlled modification ------------------------
    void setQuizId(int quizId);
    void setTitle(const std::string& title);
    void setDescription(const std::string& description);

    // ---------- composition ---------------------------------------------------
    void addQuestion(const Question& question);   // throws on duplicate id
    bool removeQuestion(int questionId);
    bool hasQuestion(int questionId) const;
    const Question* findQuestion(int questionId) const;
    Question* findQuestion(int questionId);

    // ---------- behaviour -----------------------------------------------------
    void display() const;                      // summary row
    void displayQuestions() const;             // readable table
    Result startQuiz(const std::string& studentId,
                     const std::string& studentName) const;

    // ---------- serialisation --------------------------------------------------
    std::string toHeaderRecord() const;
    static bool fromHeaderRecord(const std::string& line, Quiz& outQuiz);
    static std::vector<Quiz> loadAll();         // data/quizzes.txt
    static void saveAll(const std::vector<Quiz>& quizzes);
    static int nextQuizId();

private:
    int quizId;
    std::string title;
    std::string description;
    std::vector<Question> questions;           // composition
};

#endif // QUIZ_H