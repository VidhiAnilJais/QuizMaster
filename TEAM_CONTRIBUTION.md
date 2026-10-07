# Team Contribution

This document splits QuizMaster into four independent, clearly owned modules so that
each member can develop, test and - most importantly - **explain** their own part in the
viva. Every module compiles and runs on its own; the only shared dependency between
modules is a header.

```
                        +--------------------------+
                        |      main.cpp (glue)     |
                        +--------------------------+
                          |         |         |
        +-----------------+   +-----+   +--+--------------+
        |                     |        |                  |
  +-----+------+      +-------+-----+  +----------+  +----------+
  |  MEMBER 1  |      | MEMBER 2 |  | MEMBER 3 |  | MEMBER 4|
  | User       |      | Admin    |  | Quiz      |  | Result   |
  | Student    |      | Question |  |           |  | FileStore|
  +------------+      +----------+  +----------+  | UI       |
                                                  +----------+
```

---

## Member 1 - `[Vidhi Anil Jais]`
### Student / User module

**Files owned**

| File | Contents |
|---|---|
| `include/User.h` | `UserRecord` struct, abstract class `User` |
| `src/User.cpp` | credential handling, `login()`, `logout()`, password hashing, `users.txt` I/O |
| `include/Student.h` | `class Student : public User` |
| `src/Student.cpp` | registration, portal gate, dashboard, quiz attempts, personal results |

**Functions this member should be able to explain**

* `User::login()` - how a record is located, why `getRole()` is compared, and why the
  error message is the same for an unknown username and a wrong password.
* `User::hashPassword()` - FNV-1a, and why a hash is stored instead of the password.
* `User::ensureDefaultAdmin()` - why exactly one account is configured by the program.
* `Student::registerStudent()` - the validation loop and the "Username already exists"
  check, and where the new student id comes from.
* `Student::showMenu()` - how the same method first shows the *portal gate* and, after a
  successful login, upgrades itself to the *dashboard*.

**Concepts demonstrated here**: abstraction (`User` cannot be instantiated), inheritance,
encapsulation of the credentials, exception handling in `FileStore`.

---

## Member 2 - `[Aarya]`
### Admin / Question management

**Files owned**

| File | Contents |
|---|---|
| `include/Admin.h` | `class Admin : public User`, admin dashboard actions |
| `src/Admin.cpp` | question bank CRUD, quiz authoring, result reports, statistics |
| `include/Question.h` | `class Question` |
| `src/Question.cpp` | the multiple choice item, answer checking, `questions.txt` I/O |

**Functions this member should be able to explain**

* `Question::normalizeAnswer()` - why invalid answers throw instead of returning `false`,
  and how the throwing is caught by `Quiz::startQuiz()` to re-ask the question.
* `Admin::addQuestion()` / `updateQuestion()` / `deleteQuestion()` - the confirm box,
  and why an update is **also** written into every quiz that already uses the question.
* `Admin::quizStatistics()` - how `std::map` and `std::set` produce attempts,
  participants, average, highest, lowest and pass rate.
* `Admin::viewResults()` - the filtering by student and by quiz.
* `Admin::showMenu()` - the dashboard dispatch, including the extra "Advanced" section.

**Concepts demonstrated here**: inheritance, encapsulation of the question bank
(`std::vector<Question>`), STL containers (`map`, `set`, `vector`),
operator overloading via `operator=` when a question is replaced.

---

## Member 3 - `[Khushali]`
### Quiz / Evaluation

**Files owned**

| File | Contents |
|---|---|
| `include/Quiz.h` | `class Quiz`, composition of questions |
| `src/Quiz.cpp` | `addQuestion()`, `removeQuestion()`, serialisation, **`startQuiz()`** |

**Functions this member should be able to explain**

* `Quiz::addQuestion()` - composition (the question is *copied* into the quiz) and the
  duplicate check that throws `std::invalid_argument`.
* `Quiz::startQuiz()` - the whole examination loop: display, validate, count, and hand
  the raw counts over to `Result`.
* `Quiz::loadAll()` - how a `QUIZ` header line and the following `Q` lines are
  reassembled, and what happens when the declared question count does not match.
* `Quiz::saveAll()` - why the header and its questions must be written together.
* `Result::calculateScore()` (with Member 4) - the scoring formula
  `percentage = correct / total * 100`.

**Concepts demonstrated here**: composition vs inheritance, `const` correctness
(`startQuiz()` is `const`), exception handling, STL `vector` operations
(`push_back`, `erase`, `reserve`).

---

## Member 4 - `[Himanshi]`
### Result / File handling / Integration / Testing

**Files owned**

| File | Contents |
|---|---|
| `include/Result.h`, `src/Result.cpp` | `AnswerRecord`, `class Result`, scoring, grading, `results.txt` |
| `include/FileStore.h`, `src/FileStore.cpp` | the whole persistence layer |
| `include/UI.h`, `src/UI.cpp` | console rendering and every validated input |
| `src/main.cpp` | start-up, main menu, polymorphic portal dispatch, About screen |
| `README.md`, `TEAM_CONTRIBUTION.md`, `docs/`, `screenshots/` | documentation |

**Functions this member should be able to explain**

* `FileStore::writeLines()` - why the data is written to a `.tmp` file and only then
  renamed, so a crash can never destroy the previous version.
* `FileStore::escapeField()` / `unescapeField()` - why a user may type `|` without
  breaking the record format.
* `FileStore::appendLine()` - why the previous last line is terminated first.
* `Question::fromRecord()` and friends - returning `false` instead of throwing, so one
  corrupted line cannot stop the whole application.
* `UI::readAnyInt()` / `readChoice()` / `readPassword()` - the validation rules and the
  EOF (closed input stream) handling.
* `runPortal(User&)` in `main.cpp` - the polymorphism demonstration, and why the portal
  is owned by a `std::unique_ptr<User>` (virtual destructor).
* `Result::calculateScore()` and `getPerformanceGrade()` - scoring and the grade bands.

**Concepts demonstrated here**: encapsulation, static members, exception handling,
file handling, polymorphism, RAII with `std::unique_ptr`.

---

## How to work together

1. Agree on the headers first. `include/*.h` are the contract; never change a header
   without telling the owner of the matching `.cpp`.
2. `FileStore` and `UI` are shared infrastructure. Member 4 owns them, but everyone may
   use them - they are stable, tested helpers.
3. Integrate at `main.cpp`. Member 4 wires the portals together, so members 1 to 3 can
   develop against a `Student` or an `Admin` object without waiting for the whole menu.
4. `User::showMenu()` is `= 0`, so as soon as `Student` and `Admin` compile, the program
   links and runs - there is no half finished state that does not build.

## Suggested viva questions per member

| Member | Likely question | Where to look |
|---|---|---|
| 1 | "Why is `User` abstract? Why is `getRole()` pure virtual?" | `include/User.h` |
| 1 | "How do you store a password safely?" | `User::hashPassword()` in `src/User.cpp` |
| 2 | "What happens if the admin deletes a question that a quiz uses?" | `Admin::deleteQuestion()` |
| 2 | "How do you calculate the pass rate?" | `Admin::quizStatistics()` |
| 3 | "Where exactly is the score computed? Why not inside the quiz?" | `Result::calculateScore()` |
| 3 | "How does composition show up in the code?" | `Quiz::addQuestion()` / `~Quiz()` |
| 4 | "How is the data saved, and what happens if the program crashes?" | `FileStore::writeLines()` |
| 4 | "Where is polymorphism used?" | `runPortal(User&)` in `src/main.cpp` |
| 4 | "How do you handle a user typing nonsense?" | `UI::readAnyInt()`, `Quiz::startQuiz()` |

---

