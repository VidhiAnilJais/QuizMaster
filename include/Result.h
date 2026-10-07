#ifndef RESULT_H
#define RESULT_H

#include <string>
#include <vector>

/*
 * AnswerRecord - one row of the per-question breakdown of a Result: which
 *                option the student picked and which one was correct.
 *                Plain supporting data holder, no behaviour.
 */
struct AnswerRecord {
    int questionId;
    char chosen;
    char correct;

    AnswerRecord();
    AnswerRecord(int questionId, char chosen, char correct);
};

/*
 * Result - the evaluation outcome of one finished quiz attempt.
 *
 * A Result is produced by Quiz::startQuiz() (it only receives the raw
 * correct / total counts) and then computes the derived values itself through
 * calculateScore(). The student and the quiz are referenced by id + name so
 * that a historical result stays readable even if a quiz is renamed later.
 */
class Result {
public:
    static const int PASS_PERCENTAGE;        // 40

    Result();
    Result(int resultId,
           const std::string& studentId,
           const std::string& studentName,
           int quizId,
           const std::string& quizTitle,
           int correctAnswers,
           int totalQuestions);
    Result(const Result& other);
    Result& operator=(const Result& other);
    ~Result();

    // ---------- encapsulation: read only access ------------------------------
    int getResultId() const;
    const std::string& getStudentId() const;
    const std::string& getStudentName() const;
    int getQuizId() const;
    const std::string& getQuizTitle() const;
    int getCorrectAnswers() const;
    int getWrongAnswers() const;
    int getTotal() const;
    int getScore() const;
    double getPercentage() const;
    const std::string& getAttemptedAt() const;
    bool isPassed() const;
    const std::vector<AnswerRecord>& getAnswers() const;
    bool wasCorrect(int questionId) const;
    char getChosenAnswer(int questionId) const;

    // ---------- encapsulation: controlled modification ------------------------
    void setResultId(int resultId);
    void setAttemptedAt(const std::string& timestamp);
    void setQuizTitle(const std::string& quizTitle);
    void setAnswers(const std::vector<AnswerRecord>& answers);

    // ---------- behaviour -----------------------------------------------------
    void calculateScore();                   // derives score + percentage
    std::string getPerformanceGrade() const; // Excellent / Very Good / ...
    std::string getSummaryLine() const;      // fixed width row for tables
    std::vector<std::string> getDetailedRows() const;   // per question breakdown
    void displayResult() const;              // full report card
    void displayCompact() const;

    // ---------- serialisation --------------------------------------------------
    std::string toRecord() const;
    static bool fromRecord(const std::string& line, Result& outResult);
    static std::vector<Result> loadAll();     // data/results.txt
    static int nextResultId();
    void saveResult() const;                 // appends one record
    static void saveAll(const std::vector<Result>& results);

    static std::string currentTimestamp();
    static std::vector<AnswerRecord> parseAnswerField(const std::string& field);

private:
    int resultId;
    int quizId;
    std::string studentId;
    std::string studentName;
    std::string quizTitle;
    std::string attemptedAt;
    int correctAnswers;
    int totalQuestions;
    int score;
    double percentage;
    std::vector<AnswerRecord> answers;       // per question breakdown
};

#endif // RESULT_H