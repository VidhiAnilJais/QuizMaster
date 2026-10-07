#include "Question.h"

#include <cctype>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "FileStore.h"
#include "UI.h"

const int Question::OPTION_COUNT = 4;

namespace {
// 'A' -> 0, 'B' -> 1, 'C' -> 2, 'D' -> 3, anything else -> -1
int optionIndex(char option) {
    switch (static_cast<char>(std::toupper(static_cast<unsigned char>(option)))) {
        case 'A': return 0;
        case 'B': return 1;
        case 'C': return 2;
        case 'D': return 3;
        default:  return -1;
    }
}
}

// ---------------------------------------------------------------------------
//  construction / destruction
// ---------------------------------------------------------------------------

Question::Question()
    : questionId(0), questionText(""), optionA(""), optionB(""),
      optionC(""), optionD(""), correctAnswer('A') {}

Question::Question(int questionId,
                   const std::string& questionText,
                   const std::string& optionA,
                   const std::string& optionB,
                   const std::string& optionC,
                   const std::string& optionD,
                   char correctAnswer)
    : questionId(questionId), questionText(questionText),
      optionA(optionA), optionB(optionB), optionC(optionC), optionD(optionD),
      correctAnswer(isValidOption(correctAnswer) ? correctAnswer : 'A') {}

Question::Question(const Question& other)
    : questionId(other.questionId), questionText(other.questionText),
      optionA(other.optionA), optionB(other.optionB), optionC(other.optionC),
      optionD(other.optionD), correctAnswer(other.correctAnswer) {}

Question& Question::operator=(const Question& other) {
    if (this != &other) {                       // guard against self assignment
        questionId = other.questionId;
        questionText = other.questionText;
        optionA = other.optionA;
        optionB = other.optionB;
        optionC = other.optionC;
        optionD = other.optionD;
        correctAnswer = other.correctAnswer;
    }
    return *this;
}

Question::~Question() {
    // No manual memory to release (std::string frees itself), but the
    // destructor is written explicitly so that the class documents its own
    // lifetime rules, as required by the project brief.
}

// ---------------------------------------------------------------------------
//  encapsulation: read only access
// ---------------------------------------------------------------------------

int Question::getQuestionId() const { return questionId; }

const std::string& Question::getQuestionText() const { return questionText; }

const std::string& Question::getOption(char option) const {
    switch (indexOfOption(option)) {
        case 0:  return optionA;
        case 1:  return optionB;
        case 2:  return optionC;
        case 3:  return optionD;
        default: return questionText;           // unreachable for valid input
    }
}

char Question::getCorrectAnswer() const { return correctAnswer; }

bool Question::isCorrect(char answer) const {
    if (!isValidOption(answer)) return false;
    return static_cast<char>(std::toupper(static_cast<unsigned char>(answer))) == correctAnswer;
}

bool Question::isValid() const {
    if (questionId <= 0) return false;
    if (questionText.empty()) return false;
    if (optionA.empty() || optionB.empty() || optionC.empty() || optionD.empty()) return false;
    return isValidOption(correctAnswer);
}

// ---------------------------------------------------------------------------
//  encapsulation: controlled modification
// ---------------------------------------------------------------------------

void Question::setQuestionId(int id) {
    if (id <= 0) {
        throw std::invalid_argument("Question id must be a positive number.");
    }
    questionId = id;
}

void Question::setQuestionText(const std::string& text) {
    if (UI::trim(text).empty()) {
        throw std::invalid_argument("Question text cannot be empty.");
    }
    questionText = UI::trim(text);
}

void Question::setOption(char option, const std::string& text) {
    if (!isValidOption(option)) {
        throw std::invalid_argument("Option must be one of A, B, C or D.");
    }
    if (UI::trim(text).empty()) {
        throw std::invalid_argument("Option text cannot be empty.");
    }
    switch (indexOfOption(option)) {
        case 0: optionA = UI::trim(text); break;
        case 1: optionB = UI::trim(text); break;
        case 2: optionC = UI::trim(text); break;
        case 3: optionD = UI::trim(text); break;
        default: break;
    }
}

void Question::setCorrectAnswer(char answer) {
    if (!isValidOption(answer)) {
        throw std::invalid_argument("The correct answer must be A, B, C or D.");
    }
    correctAnswer = static_cast<char>(std::toupper(static_cast<unsigned char>(answer)));
}

// ---------------------------------------------------------------------------
//  behaviour
// ---------------------------------------------------------------------------

void Question::display() const {
    UI::printColoredLine(UI::COL_BOLD, "  Q" + std::to_string(questionId) + ". " + questionText);
    UI::printColoredLine(UI::COL_CYAN, "      A. " + optionA);
    UI::printColoredLine(UI::COL_CYAN, "      B. " + optionB);
    UI::printColoredLine(UI::COL_CYAN, "      C. " + optionC);
    UI::printColoredLine(UI::COL_CYAN, "      D. " + optionD);
}

void Question::displayCompact() const {
    std::string text = questionText;
    if (text.size() > 52) text = text.substr(0, 49) + "...";
    UI::printLine("  " + UI::padLeft(std::to_string(questionId), 4) + "  " +
                  UI::padRight(text, 55) + "  Ans: " + correctAnswer);
}

// ---------------------------------------------------------------------------
//  serialisation
// ---------------------------------------------------------------------------

std::string Question::toRecord() const {
    std::vector<std::string> fields;
    fields.push_back("Q");
    fields.push_back(std::to_string(questionId));
    fields.push_back(questionText);
    fields.push_back(optionA);
    fields.push_back(optionB);
    fields.push_back(optionC);
    fields.push_back(optionD);
    fields.push_back(std::string(1, correctAnswer));
    return FileStore::makeRecord(fields);
}

bool Question::fromRecord(const std::string& line, Question& outQuestion) {
    std::vector<std::string> fields = FileStore::split(line);
    if (fields.size() != 8) return false;
    if (fields[0] != "Q") return false;

    try {
        int id = FileStore::toInt(FileStore::unescapeField(fields[1]), "question id");
        char answer = normalizeAnswer(FileStore::unescapeField(fields[7]));

        Question parsed(id,
                        FileStore::unescapeField(fields[2]),
                        FileStore::unescapeField(fields[3]),
                        FileStore::unescapeField(fields[4]),
                        FileStore::unescapeField(fields[5]),
                        FileStore::unescapeField(fields[6]),
                        answer);
        if (!parsed.isValid()) return false;

        outQuestion = parsed;                   // operator= copies every field
        return true;
    } catch (const std::exception&) {
        return false;                           // corrupted record -> skip it
    }
}

std::vector<Question> Question::loadAll() {
    std::vector<Question> questions;
    std::string path = FileStore::questionsFile();

    try {
        std::vector<std::string> lines = FileStore::readLines(path);
        for (size_t i = 0; i < lines.size(); ++i) {
            if (FileStore::isBlankLine(lines[i]) || lines[i][0] == '#') continue;

            Question question;
            if (fromRecord(lines[i], question)) {
                questions.push_back(question);
            } else {
                UI::warning("Skipped an unreadable question record in " + path +
                            " (line " + std::to_string(i + 1) + ").");
            }
        }
    } catch (const std::exception& error) {
        UI::warning("Question bank could not be loaded: " + std::string(error.what()));
    }
    return questions;
}

void Question::saveAll(const std::vector<Question>& questions) {
    std::vector<std::string> lines;
    lines.reserve(questions.size());
    for (size_t i = 0; i < questions.size(); ++i) {
        lines.push_back(questions[i].toRecord());
    }
    FileStore::writeLines(FileStore::questionsFile(), lines);
}

int Question::nextQuestionId() {
    int highest = 0;
    std::vector<Question> questions = loadAll();
    for (size_t i = 0; i < questions.size(); ++i) {
        if (questions[i].getQuestionId() > highest) highest = questions[i].getQuestionId();
    }
    return highest + 1;
}

// ---------------------------------------------------------------------------
//  static validation helpers
// ---------------------------------------------------------------------------

int Question::indexOfOption(char option) const { return optionIndex(option); }

bool Question::isValidOption(char option) { return optionIndex(option) >= 0; }

char Question::normalizeAnswer(const std::string& rawAnswer) {
    std::string cleaned = UI::trim(rawAnswer);
    if (cleaned.size() != 1) {
        throw std::invalid_argument("Answer must be exactly one character: A, B, C or D.");
    }
    char value = static_cast<char>(std::toupper(static_cast<unsigned char>(cleaned[0])));
    if (value != 'A' && value != 'B' && value != 'C' && value != 'D') {
        throw std::invalid_argument("Answer must be A, B, C or D.");
    }
    return value;
}