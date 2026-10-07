#ifndef QUESTION_H
#define QUESTION_H

#include <string>
#include <vector>

/*
 * Question - an immutable-by-convention value object holding one multiple
 *            choice item plus the four possible answers and the correct one.
 *
 * All data is private (encapsulation). Every attribute is reached through a
 * getter / setter, and a question can only exist in a *valid* state: the id
 * must be positive and the correct answer must be one of A, B, C, D. That
 * invariant is enforced by the setters, which is what allows the file loader
 * to simply ask a question "are you valid?".
 */
class Question {
public:
    static const int OPTION_COUNT;           // 4 -> defined in Question.cpp

    Question();                              // required by std::vector
    Question(int questionId,
             const std::string& questionText,
             const std::string& optionA,
             const std::string& optionB,
             const std::string& optionC,
             const std::string& optionD,
             char correctAnswer);
    Question(const Question& other);
    Question& operator=(const Question& other);
    ~Question();

    // ---------- encapsulation: read only access ------------------------------
    int getQuestionId() const;
    const std::string& getQuestionText() const;
    const std::string& getOption(char option) const;
    char getCorrectAnswer() const;
    bool isCorrect(char answer) const;
    bool isValid() const;

    // ---------- encapsulation: controlled modification ------------------------
    void setQuestionId(int questionId);
    void setQuestionText(const std::string& questionText);
    void setOption(char option, const std::string& text);
    void setCorrectAnswer(char correctAnswer);

    // ---------- behaviour -----------------------------------------------------
    void display() const;                    // polished question card
    void displayCompact() const;             // one line, for tables

    // ---------- serialisation --------------------------------------------------
    std::string toRecord() const;
    static bool fromRecord(const std::string& line, Question& outQuestion);
    static std::vector<Question> loadAll();   // data/questions.txt
    static void saveAll(const std::vector<Question>& questions);
    static int nextQuestionId();

    // ---------- static validation helpers --------------------------------------
    static bool isValidOption(char option);
    static char normalizeAnswer(const std::string& rawAnswer);

private:
    int indexOfOption(char option) const;     // 'A' -> 0 ... 'D' -> 3, -1 if bad

    int questionId;
    std::string questionText;
    std::string optionA;
    std::string optionB;
    std::string optionC;
    std::string optionD;
    char correctAnswer;
};

#endif // QUESTION_H