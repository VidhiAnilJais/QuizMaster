#include "Quiz.h"

#include <cctype>
#include <cstdlib>
#include <stdexcept>
#include <vector>

#include "FileStore.h"
#include "UI.h"

// ---------------------------------------------------------------------------
//  construction / destruction
// ---------------------------------------------------------------------------

Quiz::Quiz()
    : quizId(0), title(""), description("") {}

Quiz::Quiz(int quizId, const std::string& title, const std::string& description)
    : quizId(quizId), title(title), description(description) {}

Quiz::Quiz(const Quiz& other)
    : quizId(other.quizId), title(other.title), description(other.description),
      questions(other.questions) {}          // deep copy: each Quiz owns its questions

Quiz& Quiz::operator=(const Quiz& other) {
    if (this != &other) {
        quizId = other.quizId;
        title = other.title;
        description = other.description;
        questions = other.questions;
    }
    return *this;
}

Quiz::~Quiz() {
    // Composition: destroying the Quiz also destroys the Question objects it
    // owns. std::vector takes care of the memory (RAII).
}

// ---------------------------------------------------------------------------
//  encapsulation: read only access
// ---------------------------------------------------------------------------

int Quiz::getQuizId() const { return quizId; }
const std::string& Quiz::getTitle() const { return title; }
const std::string& Quiz::getDescription() const { return description; }
const std::vector<Question>& Quiz::getQuestions() const { return questions; }
int Quiz::getTotalQuestions() const { return static_cast<int>(questions.size()); }
bool Quiz::isEmpty() const { return questions.empty(); }

// ---------------------------------------------------------------------------
//  encapsulation: controlled modification
// ---------------------------------------------------------------------------

void Quiz::setQuizId(int id) {
    if (id <= 0) throw std::invalid_argument("Quiz id must be a positive number.");
    quizId = id;
}

void Quiz::setTitle(const std::string& newTitle) {
    if (UI::trim(newTitle).empty()) throw std::invalid_argument("Quiz title cannot be empty.");
    title = UI::trim(newTitle);
}

void Quiz::setDescription(const std::string& newDescription) {
    description = UI::trim(newDescription);
}

// ---------------------------------------------------------------------------
//  composition: add / remove questions
// ---------------------------------------------------------------------------

void Quiz::addQuestion(const Question& question) {
    if (!question.isValid()) {
        throw std::invalid_argument("Only a complete question can be added to a quiz.");
    }
    if (hasQuestion(question.getQuestionId())) {
        throw std::invalid_argument("Question " + std::to_string(question.getQuestionId()) +
                                    " is already part of this quiz.");
    }
    questions.push_back(question);           // copy -> composition
}

bool Quiz::removeQuestion(int questionId) {
    for (size_t i = 0; i < questions.size(); ++i) {
        if (questions[i].getQuestionId() == questionId) {
            questions.erase(questions.begin() + static_cast<long>(i));
            return true;
        }
    }
    return false;
}

bool Quiz::hasQuestion(int questionId) const { return findQuestion(questionId) != 0; }

const Question* Quiz::findQuestion(int questionId) const {
    for (size_t i = 0; i < questions.size(); ++i) {
        if (questions[i].getQuestionId() == questionId) return &questions[i];
    }
    return 0;
}

Question* Quiz::findQuestion(int questionId) {
    for (size_t i = 0; i < questions.size(); ++i) {
        if (questions[i].getQuestionId() == questionId) return &questions[i];
    }
    return 0;
}

// ---------------------------------------------------------------------------
//  behaviour
// ---------------------------------------------------------------------------

void Quiz::display() const {
    std::string text = title;
    if (text.size() > 34) text = text.substr(0, 31) + "...";
    UI::printLine("  " + UI::padLeft("QUIZ " + std::to_string(quizId), 8) + "  " +
                  UI::padRight(text, 36) +
                  UI::padLeft(UI::count(getTotalQuestions(), "question"), 12) +
                  (isEmpty() ? "  [empty]" : ""));
}

void Quiz::displayQuestions() const {
    UI::header("QUESTIONS OF QUIZ: " + UI::toUpper(title),
               "Quiz ID " + std::to_string(quizId) + "  |  " +
               UI::count(getTotalQuestions(), "question"));

    if (isEmpty()) {
        UI::warning("This quiz has no questions yet.");
        return;
    }
    for (size_t i = 0; i < questions.size(); ++i) {
        UI::printLine("  " + std::to_string(i + 1) + ")  [Question ID " +
                      std::to_string(questions[i].getQuestionId()) + "]");
        questions[i].display();
    }
    UI::printRule('-');
}

Result Quiz::startQuiz(const std::string& studentId, const std::string& studentName) const {
    if (isEmpty()) {
        throw std::runtime_error("This quiz has no questions, so it cannot be started.");
    }
    if (studentId.empty() || studentName.empty()) {
        throw std::invalid_argument("A quiz can only be started for a logged in student.");
    }

    UI::header("QUIZ IN PROGRESS",
               title + "  |  " + UI::count(getTotalQuestions(), "question"));
    UI::info("Answer every question by typing A, B, C or D and pressing Enter.");
    UI::info("You cannot go back to a previous question once it is submitted.");
    UI::printBlank();

    int correctCount = 0;
    std::vector<AnswerRecord> answers;
    answers.reserve(questions.size());

    for (size_t i = 0; i < questions.size(); ++i) {
        const Question& question = questions[i];

        UI::printRule('-');
        UI::printColoredLine(UI::COL_BOLD, "Question " + std::to_string(i + 1) + " of " +
                                           std::to_string(questions.size()) +
                                           "   (ID " + std::to_string(question.getQuestionId()) + ")");
        UI::printColoredLine(UI::COL_WHITE, question.getQuestionText());
        UI::printColoredLine(UI::COL_CYAN, "   A. " + question.getOption('A'));
        UI::printColoredLine(UI::COL_CYAN, "   B. " + question.getOption('B'));
        UI::printColoredLine(UI::COL_CYAN, "   C. " + question.getOption('C'));
        UI::printColoredLine(UI::COL_CYAN, "   D. " + question.getOption('D'));

        char answer = '\0';
        for (;;) {
            std::string raw = UI::readLine("Enter your answer (A/B/C/D): ");
            try {
                answer = Question::normalizeAnswer(raw);
                break;
            } catch (const std::invalid_argument& error) {
                UI::error(std::string(error.what()) + " Please try again.");
                if (UI::inputExhausted()) throw;   // input finished: leave the loop
            }
        }

        bool correct = question.isCorrect(answer);
        answers.push_back(AnswerRecord(question.getQuestionId(), answer,
                                       question.getCorrectAnswer()));
        if (correct) {
            ++correctCount;
            UI::printColoredLine(UI::COL_GREEN, "   Correct!  Answer: " +
                                                std::string(1, question.getCorrectAnswer()));
        } else {
            UI::printColoredLine(UI::COL_RED, "   Wrong.    Correct answer: " +
                                              std::string(1, question.getCorrectAnswer()));
        }
    }

    UI::printRule('-');
    Result result(Result::nextResultId(), studentId, studentName, quizId, title,
                  correctCount, getTotalQuestions());
    result.setAnswers(answers);                 // also refreshes score + percentage
    return result;
}

// ---------------------------------------------------------------------------
//  serialisation
// ---------------------------------------------------------------------------

std::string Quiz::toHeaderRecord() const {
    std::vector<std::string> fields;
    fields.push_back("QUIZ");
    fields.push_back(std::to_string(quizId));
    fields.push_back(title);
    fields.push_back(description);
    fields.push_back(std::to_string(getTotalQuestions()));
    return FileStore::makeRecord(fields);
}

bool Quiz::fromHeaderRecord(const std::string& line, Quiz& outQuiz) {
    std::vector<std::string> fields = FileStore::split(line);
    if (fields.size() != 5) return false;
    if (fields[0] != "QUIZ") return false;

    try {
        int id = FileStore::toInt(FileStore::unescapeField(fields[1]), "quiz id");
        int declaredCount = FileStore::toInt(FileStore::unescapeField(fields[4]), "question count");
        if (id <= 0 || declaredCount < 0) return false;

        Quiz parsed(id, FileStore::unescapeField(fields[2]), FileStore::unescapeField(fields[3]));
        if (UI::trim(parsed.getTitle()).empty()) return false;
        outQuiz = parsed;
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

std::vector<Quiz> Quiz::loadAll() {
    std::vector<Quiz> quizzes;
    std::string path = FileStore::quizzesFile();

    try {
        std::vector<std::string> lines = FileStore::readLines(path);
        Quiz current;
        bool insideQuiz = false;
        int declaredCount = 0;
        int loadedCount = 0;

        for (size_t i = 0; i < lines.size(); ++i) {
            const std::string& line = lines[i];
            if (FileStore::isBlankLine(line) || line[0] == '#') continue;

            if (line.compare(0, 5, "QUIZ|") == 0) {
                // finish the quiz we were reading
                if (insideQuiz) {
                    if (loadedCount != declaredCount) {
                        UI::warning("Quiz \"" + current.getTitle() + "\" declared " +
                                    std::to_string(declaredCount) + " question(s) but only " +
                                    std::to_string(loadedCount) + " could be read.");
                    }
                    quizzes.push_back(current);
                }
                Quiz parsed;
                if (fromHeaderRecord(line, parsed)) {
                    // fromHeaderRecord already proved that field 4 is a number,
                    // so atoi() here is safe and cannot throw.
                    declaredCount = std::atoi(
                        FileStore::unescapeField(FileStore::split(line)[4]).c_str());
                    current = parsed;                     // starts with no questions
                    insideQuiz = true;
                    loadedCount = 0;
                } else {
                    insideQuiz = false;
                    UI::warning("Skipped an unreadable quiz record in " + path +
                                " (line " + std::to_string(i + 1) + ").");
                }
                continue;
            }

            if (!insideQuiz) continue;      // orphaned question line -> ignored

            Question question;
            if (Question::fromRecord(line, question)) {
                if (current.hasQuestion(question.getQuestionId())) {
                    UI::warning("Duplicate question id " +
                                std::to_string(question.getQuestionId()) +
                                " inside quiz \"" + current.getTitle() + "\" was ignored.");
                } else {
                    current.addQuestion(question);
                    ++loadedCount;
                }
            } else {
                UI::warning("Skipped an unreadable question of quiz \"" + current.getTitle() +
                            "\" in " + path + " (line " + std::to_string(i + 1) + ").");
            }
        }

        if (insideQuiz) quizzes.push_back(current);   // last quiz in the file
    } catch (const std::exception& error) {
        UI::warning("Quizzes could not be loaded: " + std::string(error.what()));
    }
    return quizzes;
}

void Quiz::saveAll(const std::vector<Quiz>& quizzes) {
    // One header record per quiz, immediately followed by its question records.
    std::vector<std::string> lines;
    for (size_t i = 0; i < quizzes.size(); ++i) {
        lines.push_back(quizzes[i].toHeaderRecord());
        const std::vector<Question>& questions = quizzes[i].getQuestions();
        for (size_t q = 0; q < questions.size(); ++q) {
            lines.push_back(questions[q].toRecord());
        }
    }
    FileStore::writeLines(FileStore::quizzesFile(), lines);
}

int Quiz::nextQuizId() {
    int highest = 0;
    std::vector<Quiz> quizzes = loadAll();
    for (size_t i = 0; i < quizzes.size(); ++i) {
        if (quizzes[i].getQuizId() > highest) highest = quizzes[i].getQuizId();
    }
    return highest + 1;
}