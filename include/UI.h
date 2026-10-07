#ifndef UI_H
#define UI_H

#include <string>
#include <vector>

/*
 * UI - a stateless helper class (all members are static).
 *
 * Purpose : every console presentation / input concern of QuizMaster lives
 *           here, so that the domain classes (User, Student, Admin, Quiz,
 *           Question, Result) stay free of duplicated print statements and,
 *           more importantly, free of unsafe input handling.
 *
 * Note    : it is a class (not a namespace) only because the project brief
 *           asks for a small number of well named classes; it holds no state,
 *           so no object is ever created.
 */
class UI {
public:
    // ---------- colour handling (optional, always degrades gracefully) -------
    enum Color {
        COL_DEFAULT = 0,
        COL_RED,
        COL_GREEN,
        COL_YELLOW,
        COL_BLUE,
        COL_MAGENTA,
        COL_CYAN,
        COL_WHITE,
        COL_BOLD
    };

    static void initialize();                 // detect terminal + turn VT on
    static void setColorEnabled(bool enabled);
    static bool isColorEnabled();

    // ---------- low level printing ------------------------------------------
    static void setColor(Color color);
    static void resetColor();
    static void print(const std::string& text);
    static void printColored(Color color, const std::string& text);
    static void println();
    static void println(const std::string& text);
    static void printLine(const std::string& text = "");
    static void printColoredLine(Color color, const std::string& text = "");
    static void printCentered(const std::string& text);
    static void printRule(char lineChar = '-', int width = 72);
    static void printBlank();

    // ---------- structured blocks -------------------------------------------
    static void banner(const std::string& title, const std::string& subtitle);
    static void header(const std::string& title, const std::string& subtitle = "");
    static void section(const std::string& title);
    static void box(const std::string& title, const std::vector<std::string>& lines);
    static void menu(const std::string& title,
                     const std::string& subtitle,
                     const std::vector<std::string>& items,
                     const std::string& promptText);

    // ---------- semantic messages --------------------------------------------
    static void success(const std::string& message);
    static void error(const std::string& message);
    static void warning(const std::string& message);
    static void info(const std::string& message);
    static void confirmBox(const std::string& message);

    // ---------- validated input ---------------------------------------------
    static std::string readLine(const std::string& prompt = "");
    static std::string readNonEmpty(const std::string& prompt);
    static std::string readLineWithDefault(const std::string& prompt, const std::string& fallback);
    static int readInt(const std::string& prompt, int minValue, int maxValue);
    static int readAnyInt(const std::string& prompt);
    static char readChoice(const std::string& prompt, const std::string& allowed);
    static bool confirm(const std::string& question);
    static std::string readPassword(const std::string& prompt);
    static void pause(const std::string& message = "Press Enter to continue...");

    // ---------- screen / flow ------------------------------------------------
    static void clearScreen();
    static void waitForEnter();

    // true when the input stream reached EOF (piped / redirected input)
    static bool inputExhausted();
    static void resetInputState();

    // ---------- formatting helpers -------------------------------------------
    static std::string repeat(char ch, int count);
    static std::string padRight(const std::string& text, size_t width);
    static std::string padLeft(const std::string& text, size_t width);
    static std::string toUpper(const std::string& text);
    static std::string toLower(const std::string& text);
    static std::string joinTexts(const std::vector<std::string>& parts,
                                 const std::string& separator = ", ");
    // "1 question" / "5 questions"
    static std::string count(int value, const std::string& singular,
                             const std::string& plural = "");
    static std::string trim(const std::string& text);
    static std::string fixed(double value, int decimals);
    static bool containsLetters(const std::string& text);
    static bool isAlphanumeric(const std::string& text);

    static const int WIDTH;                   // console layout width

private:
    UI();                                     // never constructed
    static bool parseIntStrict(const std::string& text, int& outValue);
    static void discardRestOfLine();

    static bool colorEnabled;
    static bool exhausted;
};

#endif // UI_H