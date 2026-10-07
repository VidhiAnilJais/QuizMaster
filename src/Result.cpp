#include "Result.h"

#include <algorithm>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "FileStore.h"
#include "UI.h"

const int Result::PASS_PERCENTAGE = 40;

// ---------------------------------------------------------------------------
//  AnswerRecord
// ---------------------------------------------------------------------------

AnswerRecord::AnswerRecord() : questionId(0), chosen('-'), correct('-') {}

AnswerRecord::AnswerRecord(int questionId, char chosen, char correct)
    : questionId(questionId), chosen(chosen), correct(correct) {}

// ---------------------------------------------------------------------------
//  construction / destruction
// ---------------------------------------------------------------------------

Result::Result()
    : resultId(0), quizId(0), studentId(""), studentName(""), quizTitle(""),
      attemptedAt(currentTimestamp()), correctAnswers(0), totalQuestions(0),
      score(0), percentage(0.0) {}

Result::Result(int resultId,
               const std::string& studentId,
               const std::string& studentName,
               int quizId,
               const std::string& quizTitle,
               int correctAnswers,
               int totalQuestions)
    : resultId(resultId), quizId(quizId), studentId(studentId),
      studentName(studentName), quizTitle(quizTitle),
      attemptedAt(currentTimestamp()), correctAnswers(correctAnswers),
      totalQuestions(totalQuestions > 0 ? totalQuestions : 0),
      score(correctAnswers > 0 ? correctAnswers : 0), percentage(0.0) {
    if (totalQuestions <= 0) throw std::invalid_argument("A result needs at least one question.");
    if (correctAnswers < 0 || correctAnswers > totalQuestions) {
        throw std::invalid_argument("The number of correct answers is not valid.");
    }
    calculateScore();
}

Result::Result(const Result& other)
    : resultId(other.resultId), quizId(other.quizId), studentId(other.studentId),
      studentName(other.studentName), quizTitle(other.quizTitle),
      attemptedAt(other.attemptedAt), correctAnswers(other.correctAnswers),
      totalQuestions(other.totalQuestions), score(other.score),
      percentage(other.percentage), answers(other.answers) {}

Result& Result::operator=(const Result& other) {
    if (this != &other) {
        resultId = other.resultId;
        quizId = other.quizId;
        studentId = other.studentId;
        studentName = other.studentName;
        quizTitle = other.quizTitle;
        attemptedAt = other.attemptedAt;
        correctAnswers = other.correctAnswers;
        totalQuestions = other.totalQuestions;
        score = other.score;
        percentage = other.percentage;
        answers = other.answers;
    }
    return *this;
}

Result::~Result() {
    // Owns nothing but strings / primitives; the explicit destructor documents
    // the intended lifetime (RAII) of an evaluation record.
}

// ---------------------------------------------------------------------------
//  encapsulation: read only access
// ---------------------------------------------------------------------------

int Result::getResultId() const { return resultId; }
const std::string& Result::getStudentId() const { return studentId; }
const std::string& Result::getStudentName() const { return studentName; }
int Result::getQuizId() const { return quizId; }
const std::string& Result::getQuizTitle() const { return quizTitle; }
int Result::getCorrectAnswers() const { return correctAnswers; }
int Result::getTotal() const { return totalQuestions; }
int Result::getScore() const { return score; }
double Result::getPercentage() const { return percentage; }
const std::string& Result::getAttemptedAt() const { return attemptedAt; }

int Result::getWrongAnswers() const {
    int wrong = totalQuestions - correctAnswers;
    return (wrong < 0) ? 0 : wrong;
}

bool Result::isPassed() const { return percentage >= PASS_PERCENTAGE; }

const std::vector<AnswerRecord>& Result::getAnswers() const { return answers; }

bool Result::wasCorrect(int questionId) const {
    for (size_t i = 0; i < answers.size(); ++i) {
        if (answers[i].questionId == questionId) return (answers[i].chosen == answers[i].correct);
    }
    return false;
}

char Result::getChosenAnswer(int questionId) const {
    for (size_t i = 0; i < answers.size(); ++i) {
        if (answers[i].questionId == questionId) return answers[i].chosen;
    }
    return '-';
}

// ---------------------------------------------------------------------------
//  encapsulation: controlled modification
// ---------------------------------------------------------------------------

void Result::setResultId(int id) {
    if (id <= 0) throw std::invalid_argument("Result id must be a positive number.");
    resultId = id;
}

void Result::setAttemptedAt(const std::string& timestamp) { attemptedAt = timestamp; }

void Result::setQuizTitle(const std::string& title) { quizTitle = title; }

void Result::setAnswers(const std::vector<AnswerRecord>& newAnswers) {
    answers = newAnswers;
    // Keep the counters and the breakdown consistent with each other.
    int counted = 0;
    for (size_t i = 0; i < answers.size(); ++i) {
        if (answers[i].chosen == answers[i].correct) ++counted;
    }
    if (!answers.empty()) {
        correctAnswers = counted;
        totalQuestions = static_cast<int>(answers.size());
        calculateScore();
    }
}

// ---------------------------------------------------------------------------
//  behaviour
// ---------------------------------------------------------------------------

void Result::calculateScore() {
    // score is the number of correct answers; percentage is score/total * 100.
    if (totalQuestions <= 0) {
        score = 0;
        percentage = 0.0;
        return;
    }
    score = correctAnswers;
    if (score < 0) score = 0;
    if (score > totalQuestions) score = totalQuestions;
    percentage = (static_cast<double>(score) * 100.0) / static_cast<double>(totalQuestions);
    percentage = static_cast<double>(static_cast<int>(percentage * 100.0 + 0.5)) / 100.0;
}

std::string Result::getPerformanceGrade() const {
    if (percentage >= 90.0) return "Excellent";
    if (percentage >= 75.0) return "Very Good";
    if (percentage >= 50.0) return "Good";
    if (percentage >= static_cast<double>(PASS_PERCENTAGE)) return "Fair";
    return "Needs Improvement";
}

std::string Result::getSummaryLine() const {
    std::string scoreText = std::to_string(score) + "/" + std::to_string(totalQuestions);
    return "  " + UI::padLeft(std::to_string(resultId), 4) + "  " +
           UI::padRight(UI::toUpper(quizTitle), 30) + " " +
           UI::padLeft(scoreText, 7) + "  " +
           UI::padLeft(UI::fixed(percentage, 0) + "%", 5) + "  " +
           UI::padLeft(getPerformanceGrade(), 18) + " " + attemptedAt;
}

void Result::displayCompact() const {
    UI::printLine("  ID " + std::to_string(resultId) + "  |  " + quizTitle +
                  "  |  " + std::to_string(score) + "/" + std::to_string(totalQuestions) +
                  "  |  " + UI::fixed(percentage, 0) + "%  |  " + getPerformanceGrade());
}

std::vector<std::string> Result::getDetailedRows() const {
    std::vector<std::string> rows;
    for (size_t i = 0; i < answers.size(); ++i) {
        bool correct = (answers[i].chosen == answers[i].correct);
        std::string row = "   " + UI::padLeft(std::to_string(i + 1), 3) + ".  " +
                          "Question ID " + UI::padLeft(std::to_string(answers[i].questionId), 4) +
                          "   |   Your answer: " + UI::padLeft(std::string(1, answers[i].chosen), 2) +
                          "   |   Correct answer: " + UI::padLeft(std::string(1, answers[i].correct), 2) +
                          "   |   " + (correct ? "CORRECT" : "WRONG");
        rows.push_back(row);
    }
    return rows;
}

void Result::displayResult() const {
    std::string border = UI::repeat('=', UI::WIDTH);
    UI::printColoredLine(UI::COL_CYAN, border);
    UI::printColoredLine(UI::COL_BOLD, "                        QUIZ COMPLETED");
    UI::printColoredLine(UI::COL_CYAN, border);
    UI::printLine("  Student          : " + studentName + " (" + studentId + ")");
    UI::printLine("  Quiz             : " + quizTitle + " (ID " + std::to_string(quizId) + ")");
    UI::printLine("  Attempted on     : " + attemptedAt);
    UI::printLine("  Result ID        : " + std::to_string(resultId));
    UI::printLine("  ------------------------------------------");
    UI::printLine("  Correct Answers  : " + std::to_string(correctAnswers));
    UI::printLine("  Wrong Answers    : " + std::to_string(getWrongAnswers()));
    UI::printLine("  Total Questions  : " + std::to_string(totalQuestions));
    UI::printLine("  Score            : " + std::to_string(score) + "/" +
                                             std::to_string(totalQuestions));
    UI::printLine("  Percentage       : " + UI::fixed(percentage, 2) + "%");
    UI::printLine("  Result           : " + std::string(isPassed() ? "PASS" : "FAIL") +
                  " (pass mark " + std::to_string(PASS_PERCENTAGE) + "%)");
    UI::printLine("  ------------------------------------------");
    UI::printLine("  Performance      : " + getPerformanceGrade());
    UI::printLine("  ------------------------------------------");

    std::vector<std::string> rows = getDetailedRows();
    if (!rows.empty()) {
        UI::printLine("  QUESTION BY QUESTION:");
        for (size_t i = 0; i < rows.size(); ++i) UI::printLine(rows[i]);
        UI::printLine("  ------------------------------------------");
    }
    UI::printColoredLine(UI::COL_CYAN, border);
}

// ---------------------------------------------------------------------------
//  serialisation
// ---------------------------------------------------------------------------

std::string Result::toRecord() const {
    // The per question breakdown is stored as "qid:chosen:correct,..."
    std::string answerField;
    for (size_t i = 0; i < answers.size(); ++i) {
        if (i > 0) answerField += ",";
        answerField += std::to_string(answers[i].questionId);
        answerField += ":";
        answerField += answers[i].chosen;
        answerField += ":";
        answerField += answers[i].correct;
    }

    std::vector<std::string> fields;
    fields.push_back("RESULT");
    fields.push_back(std::to_string(resultId));
    fields.push_back(studentId);
    fields.push_back(studentName);
    fields.push_back(std::to_string(quizId));
    fields.push_back(quizTitle);
    fields.push_back(std::to_string(correctAnswers));
    fields.push_back(std::to_string(totalQuestions));
    fields.push_back(attemptedAt);
    fields.push_back(answerField);
    return FileStore::makeRecord(fields);
}

std::vector<AnswerRecord> Result::parseAnswerField(const std::string& field) {
    std::vector<AnswerRecord> parsed;
    if (field.empty()) return parsed;

    std::vector<std::string> entries;
    std::string current;
    for (size_t i = 0; i <= field.size(); ++i) {
        if (i == field.size() || field[i] == ',') {
            entries.push_back(current);
            current.clear();
        } else {
            current += field[i];
        }
    }

    for (size_t i = 0; i < entries.size(); ++i) {
        if (entries[i].empty()) continue;
        std::vector<std::string> parts;
        std::string piece;
        for (size_t c = 0; c <= entries[i].size(); ++c) {
            if (c == entries[i].size() || entries[i][c] == ':') {
                parts.push_back(piece);
                piece.clear();
            } else {
                piece += entries[i][c];
            }
        }
        if (parts.size() != 3) continue;
        try {
            int id = FileStore::toInt(parts[0], "answer question id");
            if (id <= 0 || parts[1].empty() || parts[2].empty()) continue;
            parsed.push_back(AnswerRecord(id, parts[1][0], parts[2][0]));
        } catch (const std::exception&) {
            // A damaged answer entry must not damage the whole result.
        }
    }
    return parsed;
}

bool Result::fromRecord(const std::string& line, Result& outResult) {
    std::vector<std::string> fields = FileStore::split(line);
    if (fields.size() < 9) return false;          // 9 = record without breakdown (older format)
    if (fields.size() > 10) return false;
    if (fields[0] != "RESULT") return false;

    try {
        // RESULT|resultId|studentId|studentName|quizId|quizTitle|correct|total|timestamp|answers
        int id = FileStore::toInt(FileStore::unescapeField(fields[1]), "result id");
        int quiz = FileStore::toInt(FileStore::unescapeField(fields[4]), "quiz id");
        int correct = FileStore::toInt(FileStore::unescapeField(fields[6]), "correct answers");
        int total = FileStore::toInt(FileStore::unescapeField(fields[7]), "total questions");
        if (id <= 0 || quiz <= 0 || total <= 0 || correct < 0 || correct > total) return false;

        Result parsed(id,
                      FileStore::unescapeField(fields[2]),
                      FileStore::unescapeField(fields[3]),
                      quiz,
                      FileStore::unescapeField(fields[5]),
                      correct,
                      total);
        parsed.setAttemptedAt(FileStore::unescapeField(fields[8]));
        if (fields.size() == 10) {
            parsed.setAnswers(parseAnswerField(FileStore::unescapeField(fields[9])));
        }
        outResult = parsed;
        return true;
    } catch (const std::exception&) {
        return false;                       // corrupted record -> skip it
    }
}

std::vector<Result> Result::loadAll() {
    std::vector<Result> results;
    std::string path = FileStore::resultsFile();

    try {
        std::vector<std::string> lines = FileStore::readLines(path);
        for (size_t i = 0; i < lines.size(); ++i) {
            if (FileStore::isBlankLine(lines[i]) || lines[i][0] == '#') continue;

            Result result;
            if (fromRecord(lines[i], result)) {
                results.push_back(result);
            } else {
                UI::warning("Skipped an unreadable result record in " + path +
                            " (line " + std::to_string(i + 1) + ").");
            }
        }
    } catch (const std::exception& error) {
        UI::warning("Results could not be loaded: " + std::string(error.what()));
    }
    return results;
}

int Result::nextResultId() {
    int highest = 0;
    std::vector<Result> results = loadAll();
    for (size_t i = 0; i < results.size(); ++i) {
        if (results[i].getResultId() > highest) highest = results[i].getResultId();
    }
    return highest + 1;
}

void Result::saveResult() const {
    FileStore::appendLine(FileStore::resultsFile(), toRecord());
}

void Result::saveAll(const std::vector<Result>& results) {
    std::vector<std::string> lines;
    lines.reserve(results.size());
    for (size_t i = 0; i < results.size(); ++i) {
        lines.push_back(results[i].toRecord());
    }
    FileStore::writeLines(FileStore::resultsFile(), lines);
}

std::string Result::currentTimestamp() {
    std::time_t now = std::time(0);
    // localtime() is fine here: the program is single threaded.
    const std::tm* local = std::localtime(&now);
    if (local == 0) return "unknown";

    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", local);
    return std::string(buffer);
}