# Screenshots & Demo Transcripts

This folder holds the material you can paste into your project report. Two kinds
of file are provided:

| File | What it is |
| --- | --- |
| `student-flow.txt` | A real, complete console transcript of the **student** journey: login → available quizzes → quiz details → own results → detailed review → profile → logout. |
| `admin-flow.txt` | A real, complete console transcript of the **admin** journey: login → question bank → quizzes → quiz statistics → all student results → logout. |

Both files were produced by running the actual executable and capturing its
output — they are not hand-written mock-ups.

---

## 1. Producing them again

```powershell
build\QuizMaster.exe --no-color --data-dir <a scratch folder>  < demo-keys.txt  > out.txt
```

`--no-color` strips the ANSI colour codes so the transcript stays readable in a
printed report or a PDF. Colours are also switched off automatically whenever
the output is redirected to a file, so the flag is only needed for clarity.

The scratch folder is a *copy* of `data/` — the demonstration data shipped with
the project is never modified by a demo run.

---

## 2. Turning a transcript into report figures

The transcripts are plain text, so the quickest route is a monospaced code block
in the report:

````markdown
```text
STUDENT DASHBOARD
Welcome, Rohan Verma  (STU001)
------------------------------------------------------------------------
  [1] Available Quizzes
  [2] Start Quiz
  [3] My Results
  [4] My Profile
  [5] Logout
------------------------------------------------------------------------
```
````

If your report tool accepts images, take real screenshots instead:

| Figure to capture | Where to get it |
| --- | --- |
| Fig. 1 — Welcome screen | main menu on start-up |
| Fig. 2 — Student portal | main menu → `1` Student |
| Fig. 3 — Student dashboard | log in as `rahul` / `rahul123` |
| Fig. 4 — Question with options | dashboard → `2` Start Quiz → pick a quiz |
| Fig. 5 — Detailed result | after answering every question |
| Fig. 6 — My results | dashboard → `3` My Results |
| Fig. 7 — Admin dashboard | log in as `admin` / `admin123` |
| Fig. 8 — Question bank | admin → `3` View Questions |
| Fig. 9 — Quiz statistics | admin → `7` Quiz Statistics |
| Fig. 10 — System status | main menu → `4` System Status |

Suggested caption format:

> **Fig. 3** Student dashboard reached through the abstract base pointer after
> `User::login()` — the same call that prints the admin menu for an `Admin`
> object (polymorphism).

---

## 3. Demo credentials used in the transcripts

| Role | Username | Password |
| --- | --- | --- |
| Administrator | `admin` | `admin123` |
| Student | `rahul` | `rahul123` |
| Student | `priya` | `priya123` |

These are demonstration accounts stored as FNV-1a hashes in `data/users.txt`.
Change the administrator password before using the system for anything real.