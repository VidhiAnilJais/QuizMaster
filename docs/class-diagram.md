# Class Diagram — QuizMaster

The picture version is **`docs/class-diagram.png`** (1900 × 1600 px).
The same diagram in text form is in this file, and a Mermaid source that renders
on GitHub is in **`docs/class-diagram.mmd`**.

Both are generated from the real headers in `include/`. To regenerate the PNG
after changing a class:

```
powershell -ExecutionPolicy Bypass -File docs\generate-class-diagram.ps1
```

---

## 1. The diagram in text

```
                        ┌──────────────────────────────────────────────┐
                        │                   <<abstract>>               │
                        │                      User                     │
                        ├──────────────────────────────────────────────┤
                        │ - string   username                          │
                        │ - string   passwordHash                      │
                        │ - bool     loggedIn                          │
                        ├──────────────────────────────────────────────┤
                        │ + getUsername() const                        │
                        │ + isLoggedIn() const                         │
                        │ + getRole() const                    *       │
                        │ + getDisplayName() const             *       │
                        │ + showMenu()                         *       │
                        │ + login(username, password) : bool           │
                        │ + logout()                                   │
                        │ # setCredential(username, password)          │
                        │ # setLoggedIn(bool)                          │
                        │ + static hashPassword(plain)                 │
                        │ + static loadRecords()                       │
                        │ + static usernameExists(u)                   │
                        │ + static ensureDefaultAdmin()                │
                        │ ~ User()   virtual                           │
                        └───────────────┬──────────────────┬───────────┘
                                        │                  │
                       generalization   │                  │  generalization
                     (hollow triangle)  │                  │  (hollow triangle)
                        ┌───────────────┘                  └───────────┐
                        │                                                  │
   ┌────────────────────▼─────────────────────┐        ┌───────────────────▼─────────────────┐
   │                Student                   │        │               Admin                  │
   ├──────────────────────────────────────────┤        ├─────────────────────────────────────┤
   │ - string   studentId                    │        │ - string             adminId         │
   │ - string   name                         │        │ - string             name            │
   ├──────────────────────────────────────────┤        │ - vector<Question>   questionBank    │
   │ + getStudentId() const                   │        │ - vector<Quiz>       quizzes         │
   │ + getName() const                        │        ├─────────────────────────────────────┤
   │ + getRole() const                        │        │ + getAdminId() const                 │
   │ + showMenu()                             │        │ + getName() const                    │
   │ + static registerStudent() : bool        │        │ + getRole() const                    │
   │ + attemptQuiz()                          │        │ + showMenu()                         │
   │ + viewResults() const                    │        │ + addQuestion()                      │
   │ + viewAvailableQuizzes() const           │        │ + updateQuestion()                   │
   │ + viewProfile() const                    │        │ + deleteQuestion()                   │
   │ ~ Student()                              │        │ + createQuiz()                       │
   └───────┬───────────────────────┬──────────┘        │ + viewResults() const                │
           │                       │                   │ + quizStatistics() const             │
           │ attempts a quiz       │ owns attempts    │ + deleteQuiz()                       │
           │                       │ 0..*             │ ~ Admin()                            │
           │                       │                  └──────┬────────────────────┬────────────┘
           ▼                       │                         │ manages quizzes     │ manages the
   ┌────────────────────┐          │                         ▼                    ▼  question bank
   │      Question      │◄─────────┼─────────◆────────┌──────────────┐   ┌──────────────────┐
   ├────────────────────┤          │          │        │     Quiz     │   │    Question      │
   │ - int     questionId Quiz  │        ├────────────────────┤   └────────┬─────────┘
   │ - string  questionText    │        │ - int    quizId      │            │
   │ - string  optionA, optionB│        │ - string title        │            │
   │ - string  optionC, optionD│        │ - string description │            │
   │ - char    correctAnswer   │        │ - vector<Question>    │            │
   ├────────────────────┤      │        │   questions           │            │
   │ + getQuestionId() const   │  owns  ├────────────────────┤            │
   │ + getQuestionText() const │  1:0..*│ + getQuizId() const   │            │
   │ + getOption(letter) const │        │ + getTitle() const    │            │
   │ + getCorrectAnswer() const│        │ + getTotalQuestions()│            │
   │ + isCorrect(answer) const │        │ + setTitle(title)     │            │
   │ + setQuestionText(text)   │        │ + addQuestion(q)      │            │
   │ + setOption(letter, text) │        │ + removeQuestion(id)  │            │
   │ + setCorrectAnswer(letter)│        │ + findQuestion(id)    │            │
   │ + display() const         │        │ + displayQuestions()  │            │
   │ + static normalizeAnswer()│        │ + startQuiz() : Result│            │
   │ + static loadAll/saveAll()│        │ + static loadAll/…    │            │
   │ ~ Question()              │        │ ~ Quiz()              │            │
   └───────────────────────────┘        └───────────┬──────────┘            │
                                                    │ creates 1:0..*         │
                                                    ▼                        │
                                        ┌───────────────────────────┐        │
                                        │          Result          │        │
                                        ├───────────────────────────┤        │
                                        │ - int    resultId        │        │
                                        │ - string studentId       │        │
                                        │ - string studentName     │        │
                                        │ - int    quizId          │        │
                                        │ - string quizTitle       │        │
                                        │ - string attemptedAt     │        │
                                        │ - int    correctAnswers  │        │
                                        │ - int    totalQuestions  │        │
                                        │ - int    score           │        │
                                        │ - double percentage      │        │
                                        │ - vector<AnswerRecord>   │        │
                                        │             answers      │        │
                                        ├───────────────────────────┤        │
                                        │ + getScore() const       │        │
                                        │ + getPercentage() const  │        │
                                        │ + isPassed() : bool      │        │
                                        │ + calculateScore()       │        │
                                        │ + getPerformanceGrade()  │        │
                                        │ + getDetailedRows()      │        │
                                        │ + displayResult() const  │        │
                                        │ + saveResult() const     │        │
                                        │ + static loadAll()       │        │
                                        │ ~ Result()               │        │
                                        └───────────────────────────┘        │
                                                                             │
   ╔═════════════════════════════════════════════════════════════════════════╧═════╗
   ║  support layer — utility classes, static members only, never instantiated      ║
   ║  ┌──────────────────────────────┐   ┌──────────────────────────────────────┐  ║
   ║  │  <<utility>>  UI             │   │  <<utility>>  FileStore              │  ║
   ║  │ - bool   colorEnabled        │   │ - string   dataDirectory            │  ║
   ║  │ + static print/header/box    │   │ + static readLines / writeLines     │  ║
   ║  │ + static readInt / readChoice│   │ + static appendLine                 │  ║
   ║  │ + static readPassword        │   │ + static escapeField / unescapeField│  ║
   ║  │ + static confirm             │   │ + static ensureDataFiles            │  ║
   ║  └──────────────────────────────┘   └──────────────────────────────────────┘  ║
   ╚═══════════════════════════════════════════════════════════════════════════════╝
```

`◆` = composition, `▷` = association/dependency, `*` = pure virtual.

---

## 2. Packages and responsibilities

| Class | Header | Responsibility |
| --- | --- | --- |
| `User` | `include/User.h` | Abstract base. Holds the credentials (`username`, `passwordHash`) and the session flag. Implements `login()`, `logout()`, the FNV-1a `hashPassword()` and the whole `users.txt` mapping. `User` can never be instantiated. |
| `Student` | `include/Student.h` | Concrete `User`. Adds `studentId` + `name` and the candidate features: registration, dashboard, attempt a quiz, own results, profile. |
| `Admin` | `include/Admin.h` | Concrete `User`. Owns the in-memory `questionBank` and `quizzes` and exposes every content-management and reporting feature. |
| `Question` | `include/Question.h` | One immutable-by-convention MCQ: id, text, four options, correct answer. Knows how to validate an answer (`isCorrect`, `normalizeAnswer`) and how to map itself to/from one `questions.txt` record. |
| `Quiz` | `include/Quiz.h` | **Composition**: owns `std::vector<Question>` copies. `startQuiz()` is the examination engine — it asks the questions, builds the `AnswerRecord` list and returns a `Result`. |
| `Result` | `include/Result.h` | One finished attempt. Derives correct count, score, percentage and grade, prints the detailed table and appends itself to `results.txt`. Holds one `AnswerRecord` per question. |
| `UI` | `include/UI.h` | Stateless helper, all members `static`. Colours, banners, boxes, tables and **all** validated input. Nothing else in the program touches `std::cin`. |
| `FileStore` | `include/FileStore.h` | Stateless helper, all members `static`. The only class that opens a file: reading, atomic rewriting via `.tmp`, appending, and escaping the `|` field separator. |

Support value types (declared next to the class that uses them):
`UserRecord` (in `User.h`), `AnswerRecord` (in `Result.h`).

---

## 3. Relationships

| # | From → To | Kind | Multiplicity | Meaning in the code |
| --- | --- | --- | --- | --- |
| 1 | `Student` → `User` | generalization | — | A student *is a* user: it inherits `login()`, `logout()`, `getUsername()` and overrides `getRole()`, `getDisplayName()`, `showMenu()`. |
| 2 | `Admin` → `User` | generalization | — | An admin *is a* user, with the same three overrides. |
| 3 | `Quiz` ◆ `Question` | **composition** | `1 : 0..*` | A quiz owns private **copies** of its questions. `Quiz::addQuestion()` stores a copy on purpose: editing a question in the master bank can then be pushed into every quiz that uses it (`Admin::updateQuestion()`). |
| 4 | `Quiz` → `Result` | association (creates) | `1 : 0..*` | `startQuiz()` produces exactly one `Result` per completed attempt; each result points back at its quiz by `quizId` + `quizTitle`. |
| 5 | `Student` → `Quiz` | association (uses) | `0..*` | A student may attempt a quiz any number of times. |
| 6 | `Student` → `Result` | association (owns) | `0..*` | A student owns the attempts whose `studentId` matches; this is the only filter applied in `Student::viewResults()`. |
| 7 | `Admin` → `Quiz` | association (manages) | `1 : 0..*` | Create, add/remove questions, delete. |
| 8 | `Admin` → `Question` | association (manages) | `1 : 0..*` | Add, update, delete in the master bank. |
| 9 | `Admin` → `Result` | association (reports) | `1 : 0..*` | Reads *all* results and derives statistics. |
| 10 | `Result` ◆ `AnswerRecord` | **composition** | `1 : 0..*` | Every attempt owns one `AnswerRecord{questionId, chosen, correct}` per question — that is what makes the detailed review possible. |
| 11 | all domain classes → `FileStore` | dependency | — | Persistence is centralised: no domain class contains a `std::ifstream`/`std::ofstream`. |
| 12 | all domain classes → `UI` | dependency | — | Rendering and input are centralised: no domain class parses `std::cin`. |

---

## 4. OOP concepts demonstrated

| Concept | Where |
| --- | --- |
| **Abstraction** | `User` is abstract (`= 0` on `getRole()`, `getDisplayName()`, `showMenu()`); the program only ever holds a `User*`. |
| **Encapsulation** | Every attribute of all eight classes is `private`. `Question` for example only exposes `setOption(char, text)` — it decides internally whether the letter is valid. |
| **Inheritance** | `Student` and `Admin` publicly derive from `User` and reuse `login()`, `logout()`, `hashPassword()`, `loadRecords()`. |
| **Polymorphism** | `user->getRole()` and `user->showMenu()` are called through the base pointer; the derived object decides the menu. Registration returns a `Student`, login returns an `Admin` — the same two calls, two different portals. |
| **Composition** | `Quiz` owns `Question` copies; `Result` owns `AnswerRecord`s. Neither is shared, so deleting a quiz or a result cannot leave a dangling object. |
| **Association** | `Student`↔`Quiz` (attempts), `Admin`↔`Quiz`/`Question`/`Result` (manages, reports). |
| **Abstraction of data / information hiding** | `users.txt`, `questions.txt`, `quizzes.txt`, `results.txt` are reachable only through the static `toRecord()` / `fromRecord()` / `loadAll()` / `saveAll()` functions of their class. |
| **Static members** | `UI` and `FileStore` hold no state at all — they are namespaces written as classes, which is why they can never be instantiated or inherited from. |
| **Constructor / destructor chaining** | `Admin` constructs its `questionBank`/`quizzes` in the constructor and flushes them in the destructor, so unsaved edits are never lost. |

---

## 5. Grading rules implemented in `Result`

| Rule | Value | Code |
| --- | --- | --- |
| `score` | number of correct answers | `Result::calculateScore()` |
| `percentage` | `score * 100.0 / totalQuestions`, rounded to 2 decimals | `Result::calculateScore()` |
| pass mark | `percentage >= 40` | `Result::isPassed()` |
| grade | `Excellent ≥ 90`, `Very Good ≥ 75`, `Good ≥ 50`, `Fair ≥ 40`, otherwise `Poor` | `Result::getPerformanceGrade()` |