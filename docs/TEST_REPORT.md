# Test Report — QuizMaster

| | |
| --- | --- |
| **Project** | QuizMaster — Online Quiz Management and Evaluation System |
| **Build** | `g++ (MinGW-w64) 6.3.0`, `-std=c++17 -Wall -Wextra -Wpedantic` |
| **Platform** | Windows 10/11, PowerShell 5.1 |
| **Build warnings** | 0 |
| **Automated cases** | 17 scripted end-to-end runs |
| **Result** | **17 / 17 PASS** |
| **Reproduce** | `build.bat` then `powershell -ExecutionPolicy Bypass -File docs\run-tests.ps1` |

The harness in `docs/run-tests.ps1` launches the real executable once per case
against a throw-away copy of `data/` in `build/test-runs/`, feeds it a scripted
sequence of keystrokes, then asserts on both the printed output and the resulting
data files. Nothing is mocked and no data file is checked in afterwards.

---

## 1. How to run it

```
build.bat
powershell -ExecutionPolicy Bypass -File docs\run-tests.ps1
```

```
case                          result  exit  users quest quiz res
---------------------------------------------------------------------
t01-first-run                 PASS    0     1     0     0    0
t02-login-retry               PASS    0     3     14    12   5
t03-attempt-quiz              PASS    0     3     14    12   6
t04-register                  PASS    0     4     14    12   5
t05-admin-add-question        PASS    0     3     15    12   5
t06-admin-update-question     PASS    0     3     14    12   5
t07-admin-delete-question     PASS    0     3     13    11   5
t08-admin-create-quiz         PASS    0     3     14    14   5
t09-admin-reports             PASS    0     3     14    12   5
t10-student-results           PASS    0     3     14    12   5
t11-role-guard                PASS    0     3     14    12   5
t12-invalid-input             PASS    0     3     14    12   5
t13-corrupt-records           PASS    0     3     16    12   5
t14-status-and-profile        PASS    0     3     14    12   5
t15-bad-cli-option            PASS    1     0     0     0    0
t16-corrupt-skip              PASS    0     3     0     0    0
t17-escaped-fields            PASS    0     3     15    12   5

PASSED 17 / 17
```

`users / quest / quiz / res` are the record counts in `users.txt`,
`questions.txt`, `quizzes.txt` and `results.txt` **after** the run — this is how
the persistence tests prove that a change was actually written to disk.

---

## 2. Test matrix

| ID | Feature under test | Type | Result |
| --- | --- | --- | --- |
| T-01 | First run on an empty folder: create the 4 data files, create the default administrator, show System Status, exit cleanly | Integration | PASS |
| T-02 | Wrong password rejected, unknown username rejected, correct password accepted, polymorphic dashboard reached, logout | Integration | PASS |
| T-03 | Attempt a quiz: confirmation gate, invalid answers re-asked, scoring, percentage, pass/fail, result appended to disk | Integration | PASS |
| T-04 | Registration: empty name, illegal characters in username, duplicate username, short password, mismatched confirmation, cancel, then a successful save | Integration | PASS |
| T-05 | Admin adds a question to the bank (14 → 15) and declines the follow-up quiz offer | Integration | PASS |
| T-06 | Admin updates a question and the change is pushed into the quizzes that use it | Integration | PASS |
| T-07 | Admin deletes a question; it disappears from the bank *and* from the quiz that used it | Integration | PASS |
| T-08 | Admin creates a quiz and adds a question to it (12 → 14 quiz records) | Integration | PASS |
| T-09 | Admin reports: student results (with filter) and quiz statistics | Integration | PASS |
| T-10 | Student sees **only their own** results and opens a detailed per-question review | Integration | PASS |
| T-11 | Role separation: options 6/7/8/0/‑1/abc typed in the student dashboard are all rejected | Boundary | PASS |
| T-12 | Main-menu input validation: `abc`, `9`, `0`, `-3`, `2.5`, `x1` | Boundary | PASS |
| T-13 | Extra garbage records appended to `questions.txt` do not stop the program | Robustness | PASS |
| T-14 | System Status report and admin menu rendering | Integration | PASS |
| T-15 | Unknown command-line option → usage text + exit code 1 | Unit | PASS |
| T-16 | Two corrupt records injected into `questions.txt`: the 14 valid ones still load, the 2 bad ones are skipped | Unit | PASS |
| T-17 | A question whose text and options contain `\|` and `\\` survives a save → restart → load round trip byte for byte | Unit | PASS |

---

## 3. Manual test matrix

Every row below was also executed interactively by hand.

| # | Test | Input | Expected | Actual |
| --- | --- | --- | --- | --- |
| M-01 | Fresh installation | run with an empty data folder | 4 files created, default admin created, warning to change the password | as expected |
| M-02 | Demo login | `rahul` / `rahul123` | Student dashboard for Rohan Verma (STU001) | as expected |
| M-03 | Wrong password | `rahul` / `wrongpass` | *Invalid username or password*, no menu | as expected |
| M-04 | Unknown user | `ghost` / `x` | *Invalid username or password* | as expected |
| M-05 | Admin login | `admin` / `admin123` | Admin dashboard with 12 options | as expected |
| M-06 | Password masking | type a password at the console | characters are replaced by `*`, backspace works | as expected |
| M-07 | Quiz attempt, all correct | `B C B C C` on Quiz 1 | 5/5, 100.00 %, Excellent, PASS | as expected |
| M-08 | Mixed answers | `B D C A C` | 2/5, 40.00 %, Fair, PASS (pass mark 40) | as expected |
| M-09 | Lower-case answer | `c` instead of `C` | accepted, converted to upper case | as expected |
| M-10 | Invalid answers | `Z`, `q`, `5` | each rejected with *Answer must be A, B, C or D* and re-asked | as expected |
| M-11 | Cancelling a quiz | answer `N` at *Begin the quiz now?* | *Quiz cancelled*, nothing written | as expected |
| M-12 | Empty quiz | start a quiz that has no questions | reported as not available | as expected |
| M-13 | Result persistence | attempt a quiz, restart the program | the attempt is in *My Results* with the same score | as expected |
| M-14 | Registration persistence | register `kavya`, restart, log in | login succeeds, id STU00x is reused next time | as expected |
| M-15 | Question validation | empty text / empty option / answer `F` | each rejected, field re-asked | as expected |
| M-16 | Question preview | add a question | preview box shows the id, text, options and the answer before saving | as expected |
| M-17 | Decline a save | answer `N` at the confirmation | nothing is written | as expected |
| M-18 | Update propagation | change a question used by Quiz 1 | the quiz shows the new text as well | as expected |
| M-19 | Delete propagation | delete question Q1 | removed from the bank and from Quiz 1 (12 → 11 quiz records) | as expected |
| M-20 | Duplicate question in one quiz | add Q1 to Quiz 1 twice | rejected with a clear message | as expected |
| M-21 | Statistics | admin → Quiz Statistics | per-quiz averages, highest/lowest, pass rate, overall totals | as expected |
| M-22 | Profile | student/admin → profile | id, name, username, role, login state | as expected |
| M-23 | System Status | main menu → System Status | file sizes and record counts of all four files | as expected |
| M-24 | `--help` | `QuizMaster.exe --help` | usage text, exit code 0 | as expected |
| M-25 | Unknown option | `QuizMaster.exe --bogus` | *Unknown option* + usage, exit code 1 | as expected |
| M-26 | Alternative data folder | `--data-dir D:\exam` | the program creates and uses that folder | as expected |
| M-27 | Piped input / EOF | `echo 5 \| QuizMaster.exe` | polite *Input stream closed* message, data saved, exit 0 | as expected |
| M-28 | Corrupt record | append `GARBAGE` to a data file | the bad line is skipped, the rest loads | as expected |
| M-29 | Escaped delimiters | store a question containing `\|` or `\` | it is escaped on write and restored on read | as expected |
| M-30 | Idempotent restart | run twice without touching anything | no duplicated records, no change in counts | as expected |

---

## 4. Defects found during testing and fixed

| # | Symptom | Root cause | Fix |
| --- | --- | --- | --- |
| D-01 | `undefined reference to vtable for User` | `User::getRole()` was declared but never defined, so the base class still emitted a vtable | implemented the pure virtual in `User.cpp` |
| D-02 | Selecting *Admin* fell through into the student flow | missing `break` in the role switch | added the missing `break` |
| D-03 | The first appended record was concatenated onto the last existing line | `FileStore::appendLine()` did not check for a missing trailing newline | `appendLine()` now appends a newline first when required |
| D-04 | Detailed review showed the wrong question/answer | off-by-one indices in `Result::fromRecord()` | indices corrected |
| D-05 | `Z` and `5` were accepted as answers | the option test only rejected non-letters | added `Question::isValidOption()` and used it in the reader |
| D-06 | The per-question breakdown was printed twice | `viewResults()` printed the details and then called `displayResult()`, which printed them again | `displayResult()` no longer repeats the table |
| D-07 | Colours were lost on Windows consoles | VT sequences were written without enabling virtual-terminal processing | console mode enabled at start-up, auto-disabled when output is redirected |
| D-08 | "1 question(s)" | unconditional plural | added `UI::count(value, singular, plural)` |
| D-09 | Piped/LF-only input was mis-parsed on Windows | the harness fed `\n` only | test harness now emits `\r\n`; `readLine()` also tolerates a trailing `\r` |
| D-10 | `make` failed on Windows because `build/` already existed | `mkdir -p build` was run as part of the compile rule | `build` is now an order-only prerequisite and `MKDIR` switches between `mkdir -p` and `mkdir` |
| D-11 | **A question containing `\|` or `\\` was silently lost on the next run** — the record was written escaped but `FileStore::split()` split on the escaped separator too, so the field count was wrong and `fromRecord()` rejected the row | the reader was not escape-aware | `split()` now breaks a field only on a *bare* separator and copies `\` + next character through untouched; verified by T-17 |

---

## 5. Known limitations

These are deliberate design boundaries, not bugs:

1. **Flat-file storage.** Records are escaped `|`-separated lines. This is
   deliberate — the brief forbids a database. `FileStore` escapes `|` and `\`
   and rejects unparsable lines instead of crashing.
2. **No password recovery.** Passwords are stored as FNV-1a 64-bit hashes, so
   they cannot be printed back. There is no reset e-mail mechanism in a console
   application; a new account must be registered.
3. **FNV-1a is a hash, not a KDF.** It is enough to keep plaintext out of a
   demonstration data file, but a production system should use bcrypt or Argon2
   with a salt.
4. **Single process, no locking.** Two QuizMaster instances writing to the same
   data folder at the same instant could overwrite each other. Writes are atomic
   (`.tmp` + rename), so a crash cannot corrupt a file, but there is no lock.
5. **Whole-file rewrites.** Adding one question rewrites `questions.txt`. Fine
   for hundreds of records, not for millions.
6. **English only.** All screen text is hard-coded English; there is no
   localisation layer.