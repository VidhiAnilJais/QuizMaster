#include "Admin.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "FileStore.h"
#include "UI.h"

namespace {

const int TEXT_MAX_LENGTH = 300;

// Reads a field and offers the current value as the default.
std::string readField(const std::string& label, const std::string& current) {
    if (current.empty()) return UI::readNonEmpty(label + ": ");

    std::string hint = current;
    if (hint.size() > 40) hint = hint.substr(0, 37) + "...";
    return UI::trim(UI::readLineWithDefault(label + " [" + hint + "]: ", current));
}

std::string truncate(const std::string& text, size_t width) {
    if (text.size() <= width) return text;
    return text.substr(0, width - 3) + "...";
}

}  // namespace

// ---------------------------------------------------------------------------
//  construction / destruction
// ---------------------------------------------------------------------------

Admin::Admin()
    : User(), adminId(""), name("") {}

Admin::Admin(const std::string& adminId, const std::string& name,
             const std::string& username, const std::string& plainPassword)
    : User(username, plainPassword), adminId(adminId), name(name) {}

Admin::~Admin() {
    // questionBank and quizzes release their elements automatically.
}

// ---------------------------------------------------------------------------
//  encapsulation: read only access
// ---------------------------------------------------------------------------

const std::string& Admin::getAdminId() const { return adminId; }
const std::string& Admin::getName() const { return name; }

std::string Admin::getRole() const { return ROLE_ADMIN; }

std::string Admin::getDisplayName() const { return name.empty() ? getUsername() : name; }

// ---------------------------------------------------------------------------
//  persistence
// ---------------------------------------------------------------------------

void Admin::loadData() {
    try {
        questionBank = Question::loadAll();
        quizzes = Quiz::loadAll();
    } catch (const std::exception& error) {
        UI::error(std::string("Content could not be loaded: ") + error.what());
    }
}

void Admin::saveQuestions() const {
    try {
        Question::saveAll(questionBank);
    } catch (const std::exception& error) {
        UI::error(std::string("The question bank could not be saved: ") + error.what());
    }
}

void Admin::saveQuizzes() const {
    try {
        Quiz::saveAll(quizzes);
    } catch (const std::exception& error) {
        UI::error(std::string("The quizzes could not be saved: ") + error.what());
    }
}

// ---------------------------------------------------------------------------
//  lookup helpers
// ---------------------------------------------------------------------------

int Admin::indexOfQuestion(int questionId) const {
    for (size_t i = 0; i < questionBank.size(); ++i) {
        if (questionBank[i].getQuestionId() == questionId) return static_cast<int>(i);
    }
    return -1;
}

int Admin::indexOfQuiz(int quizId) const {
    for (size_t i = 0; i < quizzes.size(); ++i) {
        if (quizzes[i].getQuizId() == quizId) return static_cast<int>(i);
    }
    return -1;
}

int Admin::askQuestionId(const std::string& prompt) const {
    if (questionBank.empty()) {
        UI::warning("The question bank is empty. Add a question first.");
        return -1;
    }
    for (;;) {
        int id = UI::readAnyInt(prompt);
        if (UI::inputExhausted()) return -1;
        if (indexOfQuestion(id) >= 0) return id;
        UI::error("No question with ID " + std::to_string(id) + " exists in the question bank.");
    }
}

int Admin::askQuizId(const std::string& prompt) const {
    if (quizzes.empty()) {
        UI::warning("No quiz has been created yet. Create a quiz first.");
        return -1;
    }
    for (;;) {
        int id = UI::readAnyInt(prompt);
        if (UI::inputExhausted()) return -1;
        if (indexOfQuiz(id) >= 0) return id;
        UI::error("No quiz with ID " + std::to_string(id) + " exists.");
    }
}

int Admin::selectQuiz(const std::string& title) const {
    if (quizzes.empty()) {
        UI::warning("No quiz has been created yet.");
        return -1;
    }
    UI::header("SELECT A QUIZ", title);
    displayQuizTable(quizzes);
    int choice = UI::readInt("Quiz number (1-" + std::to_string(quizzes.size()) + "): ",
                             1, static_cast<int>(quizzes.size()));
    if (UI::inputExhausted()) return -1;
    return static_cast<int>(choice - 1);
}

// ---------------------------------------------------------------------------
//  question editing form
// ---------------------------------------------------------------------------

void Admin::collectQuestionText(int questionId, Question& question) const {
    question.setQuestionId(questionId);

    std::string text;
    for (;;) {
        text = readField("  Question text     ", question.getQuestionText());
        if (UI::inputExhausted()) return;
        if (!text.empty() && static_cast<int>(text.size()) <= TEXT_MAX_LENGTH) break;
        UI::error("The question text must be 1-" + std::to_string(TEXT_MAX_LENGTH) +
                  " characters long.");
    }
    question.setQuestionText(text);

    const char labels[4] = { 'A', 'B', 'C', 'D' };
    for (int i = 0; i < 4; ++i) {
        std::string optionText;
        for (;;) {
            optionText = readField(std::string("  Option ") + labels[i] +
                                       std::string("         "),
                                   question.getOption(labels[i]));
            if (UI::inputExhausted()) return;
            if (!optionText.empty() && static_cast<int>(optionText.size()) <= TEXT_MAX_LENGTH) break;
            UI::error("An option cannot be empty.");
        }
        question.setOption(labels[i], optionText);
    }

    for (;;) {
        std::string raw = UI::readLineWithDefault(
            "  Correct answer (A/B/C/D) [" + std::string(1, question.getCorrectAnswer()) + "]: ",
            std::string(1, question.getCorrectAnswer()));
        if (UI::inputExhausted()) return;
        try {
            question.setCorrectAnswer(Question::normalizeAnswer(raw));
            break;
        } catch (const std::invalid_argument& error) {
            UI::error(std::string(error.what()) + " Please try again.");
        }
    }
}

void Admin::questionForm(Question& question, const Question* fallback) const {
    if (fallback != 0) question = *fallback;
    collectQuestionText(question.getQuestionId(), question);
}

// ---------------------------------------------------------------------------
//  table helpers
// ---------------------------------------------------------------------------

void Admin::displayQuestionTable(const std::vector<Question>& questions,
                                 const std::string& caption) const {
    if (questions.empty()) {
        UI::warning(caption + ": no questions to show.");
        return;
    }
    UI::printColoredLine(UI::COL_BOLD, "  " + caption);
    UI::printLine("  " + UI::padLeft("Q.ID", 6) + "  " + UI::padRight("QUESTION", 46) +
                  UI::padLeft("ANS", 5) + "  USED IN");
    UI::printRule('-');
    for (size_t i = 0; i < questions.size(); ++i) {
        std::vector<std::string> usedIn;
        for (size_t q = 0; q < quizzes.size(); ++q) {
            if (quizzes[q].hasQuestion(questions[i].getQuestionId())) {
                usedIn.push_back("Quiz " + std::to_string(quizzes[q].getQuizId()));
            }
        }
        std::string usage = usedIn.empty() ? "-" : UI::trim(UI::joinTexts(usedIn));
        if (usage.size() > 14) usage = usage.substr(0, 11) + "...";
        UI::printLine("  " + UI::padLeft("Q" + std::to_string(questions[i].getQuestionId()), 6) +
                      "  " + UI::padRight(truncate(questions[i].getQuestionText(), 46), 46) +
                      UI::padLeft(std::string(1, questions[i].getCorrectAnswer()), 5) + "  " + usage);
    }
    UI::printRule('-');
    UI::printLine("  Total questions: " + std::to_string(questions.size()));
}

void Admin::displayQuizTable(const std::vector<Quiz>& list) const {
    if (list.empty()) {
        UI::warning("No quiz has been created yet.");
        return;
    }
    UI::printLine("  " + UI::padLeft("#", 4) + "  " + UI::padLeft("QUIZ ID", 8) + "  " +
                  UI::padRight("TITLE", 30) + " " + UI::padLeft("QUESTIONS", 10) + "  STATUS");
    UI::printRule('-');
    for (size_t i = 0; i < list.size(); ++i) {
        std::string title = truncate(list[i].getTitle(), 30);
        UI::printLine("  " + UI::padLeft(std::to_string(i + 1), 4) + "  " +
                      UI::padLeft("QUIZ " + std::to_string(list[i].getQuizId()), 8) + "  " +
                      UI::padRight(title, 30) + " " +
                      UI::padLeft(std::to_string(list[i].getTotalQuestions()), 10) + "  " +
                      (list[i].isEmpty() ? "draft (no questions)" : "published"));
    }
    UI::printRule('-');
}

// ---------------------------------------------------------------------------
//  question bank management
// ---------------------------------------------------------------------------

void Admin::addQuestion() {
    UI::header("ADD A NEW QUESTION", "The question is first stored in the question bank");

    const int newId = Question::nextQuestionId();
    Question question;
    collectQuestionText(newId, question);
    if (UI::inputExhausted()) return;

    std::vector<std::string> lines;
    lines.push_back("Question ID : Q" + std::to_string(question.getQuestionId()));
    lines.push_back("");
    lines.push_back(question.getQuestionText());
    lines.push_back("  A. " + question.getOption('A'));
    lines.push_back("  B. " + question.getOption('B'));
    lines.push_back("  C. " + question.getOption('C'));
    lines.push_back("  D. " + question.getOption('D'));
    lines.push_back("");
    lines.push_back("Correct answer : " + std::string(1, question.getCorrectAnswer()));
    UI::box("PREVIEW", lines);

    if (!UI::confirm("Save this question to the bank?")) {
        UI::info("The question was not saved.");
        return;
    }

    try {
        questionBank.push_back(question);
        saveQuestions();
    } catch (const std::exception& error) {
        UI::error(std::string("The question could not be saved: ") + error.what());
        return;
    }
    UI::success("Question Q" + std::to_string(question.getQuestionId()) +
                " added to the question bank.");

    if (UI::confirm("Add this question to a quiz now?")) {
        int index = selectQuiz("Select the quiz that should receive the new question");
        if (index < 0) return;
        try {
            quizzes[static_cast<size_t>(index)].addQuestion(question);
            saveQuizzes();
            UI::success("Question added to quiz \"" + quizzes[static_cast<size_t>(index)].getTitle() +
                        "\" (" + UI::count(quizzes[static_cast<size_t>(index)].getTotalQuestions(),
                                          "question") + ").");
        } catch (const std::invalid_argument& error) {
            UI::error(error.what());
        }
    }
}

void Admin::updateQuestion() {
    UI::header("UPDATE A QUESTION", "Press Enter to keep the value shown in brackets");
    displayQuestionTable(questionBank, "QUESTION BANK");

    const int id = askQuestionId("  Question ID to update: ");
    if (id < 0) return;

    Question& original = questionBank[static_cast<size_t>(indexOfQuestion(id))];
    original.display();
    UI::printRule('-');

    Question edited;
    edited = original;                            // start from a copy
    collectQuestionText(id, edited);
    if (UI::inputExhausted()) return;

    std::vector<std::string> lines;
    lines.push_back("OLD : " + original.getQuestionText());
    lines.push_back("NEW : " + edited.getQuestionText());
    lines.push_back("OLD answer : " + std::string(1, original.getCorrectAnswer()));
    lines.push_back("NEW answer : " + std::string(1, edited.getCorrectAnswer()));
    UI::box("CHANGES TO BE APPLIED", lines);

    if (!UI::confirm("Save these changes?")) {
        UI::info("The question was left unchanged.");
        return;
    }

    questionBank[static_cast<size_t>(indexOfQuestion(id))] = edited;   // operator=
    saveQuestions();
    saveQuizzes();                                 // quizzes own their own copies

    // Keep the copies inside every quiz in sync with the bank.
    int synced = 0;
    for (size_t q = 0; q < quizzes.size(); ++q) {
        if (quizzes[q].removeQuestion(id)) {
            quizzes[q].addQuestion(edited);
            ++synced;
        }
    }
    if (synced > 0) {
        saveQuizzes();
        UI::success("Question Q" + std::to_string(id) + " updated (" + std::to_string(synced) +
                    " quiz copy/copies synchronised).");
    } else {
        UI::success("Question Q" + std::to_string(id) + " updated.");
    }
}

void Admin::deleteQuestion() {
    UI::header("DELETE A QUESTION", "This removes the question from the bank and from every quiz");
    displayQuestionTable(questionBank, "QUESTION BANK");

    const int id = askQuestionId("  Question ID to delete: ");
    if (id < 0) return;

    Question& victim = questionBank[static_cast<size_t>(indexOfQuestion(id))];
    victim.display();
    UI::printRule('-');

    std::vector<std::string> lines;
    lines.push_back("You are about to delete question Q" + std::to_string(id) + ".");
    lines.push_back("");
    lines.push_back("This action cannot be undone.");
    UI::box("PLEASE CONFIRM", lines);
    if (!UI::confirm("Delete this question permanently?")) {
        UI::info("Nothing was deleted.");
        return;
    }

    questionBank.erase(questionBank.begin() + indexOfQuestion(id));

    int removedFrom = 0;
    for (size_t q = 0; q < quizzes.size(); ++q) {
        if (quizzes[q].removeQuestion(id)) ++removedFrom;
    }

    saveQuestions();
    saveQuizzes();
    UI::success("Question Q" + std::to_string(id) + " deleted (" +
                (removedFrom > 0 ? "removed from " + std::to_string(removedFrom) + " quiz(es)"
                                 : std::string("it was not used in any quiz")) + ").");
}

void Admin::viewQuestions() const {
    UI::header("QUESTION BANK", "Every question that can be used to build a quiz");
    displayQuestionTable(questionBank, "ALL QUESTIONS");
}

// ---------------------------------------------------------------------------
//  quiz management
// ---------------------------------------------------------------------------

void Admin::createQuiz() {
    UI::header("CREATE A QUIZ", "A quiz starts empty - add questions to it afterwards");

    const int newId = Quiz::nextQuizId();
    std::string title;
    for (;;) {
        title = UI::readNonEmpty("  Quiz title    : ");
        if (UI::inputExhausted()) return;
        if (static_cast<int>(title.size()) <= 80) break;
        UI::error("The title must be at most 80 characters long.");
    }

    std::string description = UI::readLineWithDefault(
        "  Description  [skip]: ", "No description provided.");
    if (UI::inputExhausted()) return;

    std::vector<std::string> lines;
    lines.push_back("Quiz ID    : QUIZ " + std::to_string(newId) + " (generated automatically)");
    lines.push_back("Title      : " + title);
    lines.push_back("Questions  : 0 (add questions next)");
    lines.push_back("");
    lines.push_back("Description:");
    lines.push_back("  " + description);
    UI::box("QUIZ OVERVIEW", lines);

    if (!UI::confirm("Create this quiz?")) {
        UI::info("The quiz was not created.");
        return;
    }

    try {
        quizzes.push_back(Quiz(newId, title, description));
        saveQuizzes();
    } catch (const std::exception& error) {
        UI::error(std::string("The quiz could not be created: ") + error.what());
        return;
    }
    UI::success("Quiz \"" + title + "\" created with ID " + std::to_string(newId) + ".");
    addQuestionToQuiz();
}

void Admin::viewQuizzes() const {
    UI::header("ALL QUIZZES", "Overview of every quiz in the system");
    displayQuizTable(quizzes);

    if (quizzes.empty() || !UI::confirm("Open a quiz to list its questions?")) return;
    int index = selectQuiz("Select a quiz");
    if (index < 0) return;
    quizzes[static_cast<size_t>(index)].displayQuestions();
}

void Admin::viewQuizQuestions() const {
    int index = selectQuiz("Select a quiz to display its questions");
    if (index < 0) return;
    quizzes[static_cast<size_t>(index)].displayQuestions();
}

void Admin::addQuestionToQuiz() {
    if (questionBank.empty()) {
        UI::warning("The question bank is empty. Use option 2 to add a question first.");
        return;
    }
    UI::header("ADD AN EXISTING QUESTION TO A QUIZ", "Pick a question from the bank");
    displayQuestionTable(questionBank, "AVAILABLE QUESTIONS");

    const int id = askQuestionId("  Question ID to add: ");
    if (id < 0) return;

    const int index = selectQuiz("Select the target quiz");
    if (index < 0) return;
    Quiz& target = quizzes[static_cast<size_t>(index)];

    try {
        target.addQuestion(questionBank[static_cast<size_t>(indexOfQuestion(id))]);
        saveQuizzes();
    } catch (const std::invalid_argument& error) {
        UI::error(error.what());
        return;
    }
    UI::success("Question Q" + std::to_string(id) + " added to \"" + target.getTitle() + "\" (" +
                UI::count(target.getTotalQuestions(), "question") + ").");
}

void Admin::removeQuestionFromQuiz() {
    int index = selectQuiz("Select the quiz you want to change");
    if (index < 0) return;
    Quiz& target = quizzes[static_cast<size_t>(index)];

    if (target.isEmpty()) {
        UI::warning("This quiz has no questions.");
        return;
    }
    UI::header("REMOVE A QUESTION FROM A QUIZ",
               target.getTitle() + "  |  " + UI::count(target.getTotalQuestions(), "question"));
    target.displayQuestions();

    int id = -1;
    for (;;) {
        id = UI::readAnyInt("  Question ID to remove: ");
        if (UI::inputExhausted()) return;
        if (target.hasQuestion(id)) break;
        UI::error("Question Q" + std::to_string(id) + " is not part of this quiz.");
    }

    if (!UI::confirm("Remove question Q" + std::to_string(id) + " from this quiz?" +
                     " (it stays in the question bank)")) {
        UI::info("Nothing was changed.");
        return;
    }

    target.removeQuestion(id);
    saveQuizzes();
    UI::success("Question removed. \"" + target.getTitle() + "\" now has " +
                UI::count(target.getTotalQuestions(), "question") + ".");
}

void Admin::deleteQuiz() {
    UI::header("DELETE A QUIZ", "The questions of the quiz are kept in the question bank");
    displayQuizTable(quizzes);

    const int id = askQuizId("  Quiz ID to delete: ");
    if (id < 0) return;

    Quiz& victim = quizzes[static_cast<size_t>(indexOfQuiz(id))];
    std::vector<std::string> lines;
    lines.push_back("Quiz ID   : " + std::to_string(victim.getQuizId()));
    lines.push_back("Title     : " + victim.getTitle());
    lines.push_back("Questions : " + std::to_string(victim.getTotalQuestions()));
    lines.push_back("");
    lines.push_back("Recorded student results are not affected.");
    lines.push_back("This action cannot be undone.");
    UI::box("PLEASE CONFIRM", lines);

    if (!UI::confirm("Delete this quiz permanently?")) {
        UI::info("Nothing was deleted.");
        return;
    }

    quizzes.erase(quizzes.begin() + indexOfQuiz(id));
    saveQuizzes();
    UI::success("Quiz " + std::to_string(id) + " deleted.");
}

// ---------------------------------------------------------------------------
//  reporting
// ---------------------------------------------------------------------------

void Admin::viewResults() const {
    UI::header("STUDENT RESULTS", "Every recorded attempt of every student");

    std::vector<Result> results = Result::loadAll();
    if (results.empty()) {
        UI::warning("No result has been recorded yet.");
        return;
    }

    std::vector<std::string> items;
    items.push_back("[1] All results (" + std::to_string(results.size()) + ")");
    items.push_back("[2] Results of one student");
    items.push_back("[3] Results for one quiz");
    items.push_back("[4] Back");
    UI::menu("FILTER", "Choose how the results should be displayed", items, "");
    int choice = UI::readInt("Enter your choice: ", 1, 4);
    if (UI::inputExhausted()) return;
    if (choice == 4) return;

    std::vector<Result> filtered;
    if (choice == 2) {
        std::set<std::string> students;
        for (size_t i = 0; i < results.size(); ++i) {
            students.insert(results[i].getStudentId() + "|" + results[i].getStudentName());
        }
        std::vector<std::string> lines;
        std::vector<std::string> ids;              // parallel list for the selection
        int position = 1;
        for (std::set<std::string>::const_iterator it = students.begin(); it != students.end(); ++it) {
            std::string entry = *it;
            size_t split = entry.find('|');
            ids.push_back(entry.substr(0, split));
            lines.push_back("  " + UI::padLeft(std::to_string(position++), 3) + ".  " +
                            entry.substr(split + 1) + "   (" + entry.substr(0, split) + ")");
        }
        UI::box("REGISTERED STUDENTS WITH RESULTS", lines);
        int pick = UI::readInt("Student number: ", 1, static_cast<int>(lines.size()));
        if (UI::inputExhausted()) return;

        for (size_t i = 0; i < results.size(); ++i) {
            if (results[i].getStudentId() == ids[static_cast<size_t>(pick - 1)]) {
                filtered.push_back(results[i]);
            }
        }
    } else if (choice == 3) {
        std::set<std::string> titles;
        for (size_t i = 0; i < results.size(); ++i) {
            titles.insert(std::to_string(results[i].getQuizId()) + "|" + results[i].getQuizTitle());
        }
        std::vector<std::string> lines;
        std::vector<int> ids;                    // parallel list for the selection
        int position = 1;
        for (std::set<std::string>::const_iterator it = titles.begin(); it != titles.end(); ++it) {
            std::string entry = *it;
            size_t split = entry.find('|');
            ids.push_back(std::atoi(entry.substr(0, split).c_str()));
            lines.push_back("  " + UI::padLeft(std::to_string(position++), 3) + ".  " +
                            entry.substr(split + 1) + "   (ID " + entry.substr(0, split) + ")");
        }
        UI::box("QUIZZES WITH RESULTS", lines);
        int pick = UI::readInt("Quiz number: ", 1, static_cast<int>(lines.size()));
        if (UI::inputExhausted()) return;

        for (size_t i = 0; i < results.size(); ++i) {
            if (results[i].getQuizId() == ids[static_cast<size_t>(pick - 1)]) {
                filtered.push_back(results[i]);
            }
        }
    } else {
        filtered = results;
    }

    if (filtered.empty()) {
        UI::warning("No result matches that filter.");
        return;
    }

    UI::header("RESULT LIST",
               std::to_string(filtered.size()) + " attempt(s) matched the filter");
    UI::printLine("  " + UI::padLeft("ID", 4) + "  " + UI::padLeft("STUDENT", 20) + "  " +
                  UI::padRight("QUIZ", 26) + " " + UI::padLeft("SCORE", 7) + "  " +
                  UI::padLeft("PERCENT", 8) + "  " + UI::padLeft("GRADE", 18) + " ATTEMPTED ON");
    UI::printRule('-');
    for (size_t i = 0; i < filtered.size(); ++i) {
        const Result& r = filtered[i];
        UI::printLine("  " + UI::padLeft(std::to_string(r.getResultId()), 4) + "  " +
                      UI::padLeft(truncate(r.getStudentName(), 20), 20) + "  " +
                      UI::padRight(truncate(r.getQuizTitle(), 26), 26) + " " +
                      UI::padLeft(std::to_string(r.getScore()) + "/" +
                                      std::to_string(r.getTotal()), 7) + "  " +
                      UI::padLeft(UI::fixed(r.getPercentage(), 1) + "%", 8) + "  " +
                      UI::padLeft(r.getPerformanceGrade(), 18) + " " + r.getAttemptedAt());
    }
    UI::printRule('-');

    if (!UI::confirm("Open one of these results?")) return;
    int pick = UI::readInt("Result number (1-" + std::to_string(filtered.size()) + "): ",
                           1, static_cast<int>(filtered.size()));
    if (UI::inputExhausted()) return;

    const Result& selected = filtered[static_cast<size_t>(pick - 1)];
    UI::printBlank();
    if (selected.getAnswers().empty()) {
        UI::warning("This result was stored without an answer breakdown.");
    }
    selected.displayResult();
}

void Admin::quizStatistics() const {
    UI::header("QUIZ STATISTICS", "Aggregated performance per quiz");

    std::vector<Result> results = Result::loadAll();
    if (results.empty()) {
        UI::warning("There is no result to analyse yet.");
        return;
    }

    struct QuizStats {
        QuizStats() : attempts(0), passed(0), totalPercentage(0.0), highest(0.0), lowest(100.0) {}
        int attempts;
        int passed;
        double totalPercentage;
        double highest;
        double lowest;
    };

    std::map<int, QuizStats> stats;
    std::map<int, std::string> titles;
    std::set<std::string> students;

    for (size_t i = 0; i < results.size(); ++i) {
        const Result& r = results[i];
        QuizStats& entry = stats[r.getQuizId()];
        ++entry.attempts;
        entry.totalPercentage += r.getPercentage();
        if (r.isPassed()) ++entry.passed;
        if (r.getPercentage() > entry.highest) entry.highest = r.getPercentage();
        if (r.getPercentage() < entry.lowest) entry.lowest = r.getPercentage();
        titles[r.getQuizId()] = r.getQuizTitle();
        students.insert(r.getStudentId());
    }

    UI::printLine("  " + UI::padLeft("QUIZ ID", 8) + "  " + UI::padRight("TITLE", 24) + " " +
                  UI::padLeft("ATTEMPTS", 9) + " " + UI::padLeft("STUDENTS", 9) + " " +
                  UI::padLeft("AVERAGE", 9) + " " + UI::padLeft("HIGHEST", 8) + " " +
                  UI::padLeft("LOWEST", 8) + " " + UI::padLeft("PASSED", 8));
    UI::printRule('-');

    double grandTotal = 0.0;
    int grandAttempts = 0;
    for (std::map<int, QuizStats>::const_iterator it = stats.begin(); it != stats.end(); ++it) {
        const QuizStats& s = it->second;
        double average = s.totalPercentage / s.attempts;
        grandTotal += s.totalPercentage;
        grandAttempts += s.attempts;

        std::set<std::string> participants;
        for (size_t i = 0; i < results.size(); ++i) {
            if (results[i].getQuizId() == it->first) participants.insert(results[i].getStudentId());
        }

        UI::printLine("  " + UI::padLeft("QUIZ " + std::to_string(it->first), 8) + "  " +
                      UI::padRight(truncate(titles[it->first], 24), 24) + " " +
                      UI::padLeft(std::to_string(s.attempts), 9) + " " +
                      UI::padLeft(std::to_string(participants.size()), 9) + " " +
                      UI::padLeft(UI::fixed(average, 1) + "%", 9) + " " +
                      UI::padLeft(UI::fixed(s.highest, 1) + "%", 8) + " " +
                      UI::padLeft(UI::fixed(s.lowest, 1) + "%", 8) + " " +
                      UI::padLeft(std::to_string(s.passed) + "/" +
                                      std::to_string(s.attempts), 8));
    }
    UI::printRule('-');

    int passedOverall = 0;
    for (size_t i = 0; i < results.size(); ++i) {
        if (results[i].isPassed()) ++passedOverall;
    }

    std::vector<std::string> lines;
    lines.push_back("Total attempts        : " + std::to_string(results.size()));
    lines.push_back("Unique students       : " + std::to_string(students.size()));
    lines.push_back("Quizzes with results  : " + std::to_string(stats.size()));
    lines.push_back("Overall average score : " +
                    UI::fixed(grandTotal / grandAttempts, 2) + "%");
    lines.push_back("Overall pass rate     : " + std::to_string(passedOverall) + "/" +
                    std::to_string(results.size()) + "  (" +
                    UI::fixed((grandAttempts > 0 ? (static_cast<double>(passedOverall) * 100.0 /
                                                   grandAttempts) : 0.0), 1) + "%)");
    UI::box("OVERALL SUMMARY", lines);
}

void Admin::viewProfile() const {
    UI::header("ADMIN PROFILE", "Account and content overview");

    std::vector<std::string> lines;
    lines.push_back("Admin ID      : " + adminId);
    lines.push_back("Name          : " + name);
    lines.push_back("Username      : " + getUsername());
    lines.push_back("Role          : " + getRole());
    lines.push_back("Session       : " + std::string(isLoggedIn() ? "logged in" : "logged out"));
    lines.push_back("--------------------------------------");
    lines.push_back("Questions in bank : " + std::to_string(questionBank.size()));
    lines.push_back("Quizzes created   : " + std::to_string(quizzes.size()));
    int published = 0;
    for (size_t i = 0; i < quizzes.size(); ++i) {
        if (!quizzes[i].isEmpty()) ++published;
    }
    lines.push_back("Published quizzes : " + std::to_string(published));
    lines.push_back("Draft quizzes     : " + std::to_string(quizzes.size() - published));
    UI::box("PROFILE", lines);
}

// ---------------------------------------------------------------------------
//  polymorphism entry point
// ---------------------------------------------------------------------------

void Admin::showMenu() {
    if (!isLoggedIn() && !showLoginScreen()) {
        return;                              // the user left the portal again
    }

    loadData();

    for (;;) {
        UI::clearScreen();
        UI::printColoredLine(UI::COL_BOLD, "ADMIN DASHBOARD");
        UI::printColoredLine(UI::COL_CYAN, "Welcome, " + getDisplayName() + "  (" + adminId + ")");
        UI::printRule('-');
        UI::printLine("  CONTENT MANAGEMENT");
        UI::printLine("   [1] Create Quiz");
        UI::printLine("   [2] Add Question");
        UI::printLine("   [3] View Questions");
        UI::printLine("   [4] Update Question");
        UI::printLine("   [5] Delete Question");
        UI::printRule('-');
        UI::printLine("  REPORTS");
        UI::printLine("   [6] View Student Results");
        UI::printLine("   [7] Quiz Statistics");
        UI::printLine("   [8] Logout");
        UI::printRule('-');
        UI::printLine("  ADVANCED");
        UI::printLine("   [9] View Quizzes");
        UI::printLine("  [10] Add Existing Question to a Quiz");
        UI::printLine("  [11] Remove Question from a Quiz");
        UI::printLine("  [12] Delete Quiz");
        UI::printRule('-');

        int choice = UI::readInt("Enter your choice: ", 1, 12);
        if (UI::inputExhausted()) return;

        switch (choice) {
            case 1:  createQuiz();              UI::pause(); break;
            case 2:  addQuestion();             UI::pause(); break;
            case 3:  viewQuestions();           UI::pause(); break;
            case 4:  updateQuestion();          UI::pause(); break;
            case 5:  deleteQuestion();          UI::pause(); break;
            case 6:  viewResults();             UI::pause(); break;
            case 7:  quizStatistics();          UI::pause(); break;
            case 8:  logout();                  return;
            case 9:  viewQuizzes();             UI::pause(); break;
            case 10: addQuestionToQuiz();       UI::pause(); break;
            case 11: removeQuestionFromQuiz();  UI::pause(); break;
            case 12: deleteQuiz();              UI::pause(); break;
            default: break;
        }
    }
}

bool Admin::showLoginScreen() {
    for (;;) {
        UI::clearScreen();
        std::vector<std::string> items;
        items.push_back("[1] Login");
        items.push_back("[2] Back to main menu");
        UI::menu("ADMIN PORTAL", "Content management area", items, "");

        int choice = UI::readInt("Enter your choice: ", 1, 2);
        if (UI::inputExhausted()) return false;
        if (choice == 2) return false;

        UI::header("ADMIN LOGIN", "Administrator credentials are required");
        std::string username = UI::readNonEmpty("  Username : ");
        std::string password = UI::readPassword("  Password : ");
        if (UI::inputExhausted()) return false;

        if (!login(username, password)) {
            UI::error("Invalid username or password.");
            UI::pause();
            continue;
        }

        // Read the profile details that belong to this account.
        std::vector<UserRecord> records = User::loadRecords();
        for (size_t i = 0; i < records.size(); ++i) {
            if (records[i].role == User::ROLE_ADMIN && records[i].username == getUsername()) {
                adminId = records[i].id;
                name = records[i].name;
                break;
            }
        }
        UI::success("Login successful. Welcome, " + getDisplayName() + "!");
        return true;
    }
}

// ---------------------------------------------------------------------------
//  conversion helpers
// ---------------------------------------------------------------------------

UserRecord Admin::toUserRecord() const {
    return UserRecord(getRole(), adminId, name, getUsername(), getPasswordHash());
}

bool Admin::fromRecord(const std::string& line, Admin& outAdmin) {
    UserRecord record;
    if (!User::fromRecord(line, record)) return false;
    if (record.role != User::ROLE_ADMIN) return false;

    Admin parsed(record.id, record.name, record.username, "");
    parsed.setPasswordHash(record.passwordHash);
    outAdmin = parsed;
    return true;
}