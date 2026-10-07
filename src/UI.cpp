#include "UI.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#else
#include <unistd.h>
extern "C" {
#include <termios.h>
}
#endif

namespace {
/*
 * On Windows there is no _isatty in every MinGW version, so the console type
 * is asked from the operating system instead: FILE_TYPE_CHAR means the handle
 * is a real console, anything else (file, pipe) is not interactive.
 */
bool isInteractiveStandardOutput() {
#ifdef _WIN32
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (handle == INVALID_HANDLE_VALUE) return false;
    return (GetFileType(handle) == FILE_TYPE_CHAR);
#else
    return (isatty(STDOUT_FILENO) == 1);
#endif
}

bool isInteractiveStandardInput() {
#ifdef _WIN32
    HANDLE handle = GetStdHandle(STD_INPUT_HANDLE);
    if (handle == INVALID_HANDLE_VALUE) return false;
    return (GetFileType(handle) == FILE_TYPE_CHAR);
#else
    return (isatty(STDIN_FILENO) == 1);
#endif
}
}  // namespace

const int UI::WIDTH = 72;

bool UI::colorEnabled = false;
bool UI::exhausted = false;

// ---------------------------------------------------------------------------
//  colour handling
// ---------------------------------------------------------------------------

void UI::initialize() {
#ifdef _WIN32
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    bool ok = (handle != INVALID_HANDLE_VALUE) && GetConsoleMode(handle, &mode);
    if (ok) ok = SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    colorEnabled = ok && isInteractiveStandardOutput();
#else
    colorEnabled = isInteractiveStandardOutput();
#endif
}

void UI::setColorEnabled(bool enabled) { colorEnabled = enabled; }
bool UI::isColorEnabled() { return colorEnabled; }

void UI::setColor(Color color) {
    if (!colorEnabled) return;
    switch (color) {
        case COL_RED:     std::cout << "\033[31m"; break;
        case COL_GREEN:   std::cout << "\033[32m"; break;
        case COL_YELLOW:  std::cout << "\033[33m"; break;
        case COL_BLUE:    std::cout << "\033[34m"; break;
        case COL_MAGENTA: std::cout << "\033[35m"; break;
        case COL_CYAN:    std::cout << "\033[36m"; break;
        case COL_WHITE:   std::cout << "\033[37m"; break;
        case COL_BOLD:    std::cout << "\033[1m";  break;
        case COL_DEFAULT: break;
    }
}

void UI::resetColor() {
    if (colorEnabled) std::cout << "\033[0m";
}

// ---------------------------------------------------------------------------
//  low level printing
// ---------------------------------------------------------------------------

void UI::print(const std::string& text) { std::cout << text; }

void UI::println() { std::cout << std::endl; }

void UI::println(const std::string& text) { std::cout << text << std::endl; }

void UI::printLine(const std::string& text) {
    std::cout << text << std::endl;
}

void UI::printColoredLine(Color color, const std::string& text) {
    setColor(color);
    std::cout << text;
    resetColor();
    std::cout << std::endl;
}

void UI::printCentered(const std::string& text) {
    int pad = (WIDTH - static_cast<int>(text.size())) / 2;
    if (pad < 0) pad = 0;
    std::cout << std::string(static_cast<size_t>(pad), ' ') << text << std::endl;
}

void UI::printRule(char lineChar, int width) {
    printLine(repeat(lineChar, width));
}

void UI::printBlank() { std::cout << std::endl; }

// ---------------------------------------------------------------------------
//  structured blocks
// ---------------------------------------------------------------------------

void UI::banner(const std::string& title, const std::string& subtitle) {
    std::string border = repeat('=', WIDTH);
    printColoredLine(COL_CYAN, "+" + border + "+");
    setColor(COL_BOLD);
    printCentered(title);
    if (!subtitle.empty()) printCentered(subtitle);
    setColor(COL_CYAN);
    printCentered(repeat('-', WIDTH - 8));
    resetColor();
    printColoredLine(COL_CYAN, "+" + border + "+");
}

void UI::header(const std::string& title, const std::string& subtitle) {
    printBlank();
    printColoredLine(COL_BOLD, title);
    if (!subtitle.empty()) printColoredLine(COL_CYAN, subtitle);
    printRule('=');
}

void UI::section(const std::string& title) {
    printBlank();
    printColoredLine(COL_BLUE, "-- " + title + " " + repeat('-', static_cast<int>(title.size()) + 4));
}

void UI::box(const std::string& title, const std::vector<std::string>& lines) {
    int inner = WIDTH - 4;
    std::string top = "+" + repeat('-', inner) + "+";
    printColoredLine(COL_CYAN, top);
    setColor(COL_BOLD);
    std::string titleLine = "| " + title;
    if (static_cast<int>(titleLine.size()) < inner) titleLine += std::string(inner - titleLine.size(), ' ');
    titleLine += " |";
    setColor(COL_CYAN);
    printLine(titleLine);
    printLine("|" + repeat('-', inner) + "|");
    resetColor();
    for (size_t i = 0; i < lines.size(); ++i) {
        std::string row = " " + lines[i];
        if (static_cast<int>(row.size()) > inner - 1) row = row.substr(0, static_cast<size_t>(inner - 1));
        while (static_cast<int>(row.size()) < inner) row += " ";
        printLine(row + " |");
    }
    printColoredLine(COL_CYAN, top);
}

void UI::menu(const std::string& title,
              const std::string& subtitle,
              const std::vector<std::string>& items,
              const std::string& promptText) {
    printBlank();
    printColoredLine(COL_BOLD, title);
    if (!subtitle.empty()) printColoredLine(COL_CYAN, subtitle);
    printRule('-');
    for (size_t i = 0; i < items.size(); ++i) {
        std::cout << "  " << items[i] << std::endl;
    }
    printRule('-');
    std::cout << promptText << std::flush;
}

// ---------------------------------------------------------------------------
//  semantic messages
// ---------------------------------------------------------------------------

void UI::success(const std::string& message) {
    printColoredLine(COL_GREEN, "  [OK] " + message);
}

void UI::error(const std::string& message) {
    printColoredLine(COL_RED, "  [X] " + message);
}

void UI::warning(const std::string& message) {
    printColoredLine(COL_YELLOW, "  [!] " + message);
}

void UI::info(const std::string& message) {
    printColoredLine(COL_CYAN, "  [i] " + message);
}

void UI::confirmBox(const std::string& message) {
    std::vector<std::string> lines;
    lines.push_back(message);
    lines.push_back("");
    lines.push_back("Press Y to confirm, any other key to cancel.");
    box("CONFIRMATION", lines);
}

// ---------------------------------------------------------------------------
//  input helpers
// ---------------------------------------------------------------------------

void UI::discardRestOfLine() {
    if (!std::cin.eof()) std::cin.ignore(10000, '\n');
    if (std::cin.fail()) std::cin.clear();
}

void UI::resetInputState() { exhausted = false; }

bool UI::inputExhausted() { return exhausted; }

std::string UI::readLine(const std::string& prompt) {
    if (!prompt.empty()) std::cout << prompt << std::flush;

    std::string line;
    if (!std::getline(std::cin, line)) {
        // End of input (piped file finished or <Ctrl+D>): recover politely
        // instead of looping forever on a dead stream.
        exhausted = true;
        std::cin.clear();
        std::cout << std::endl;
        return std::string();
    }
    // Tolerate Windows line endings.
    if (!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1);
    return line;
}

std::string UI::trim(const std::string& text) {
    size_t first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return std::string();
    size_t last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

std::string UI::readNonEmpty(const std::string& prompt) {
    for (;;) {
        std::string value = trim(readLine(prompt));
        if (exhausted) return std::string();
        if (!value.empty()) return value;
        error("This field cannot be empty.");
    }
}

std::string UI::readLineWithDefault(const std::string& prompt, const std::string& fallback) {
    std::string value = trim(readLine(prompt));
    if (exhausted) return fallback;
    return value.empty() ? fallback : value;
}

bool UI::parseIntStrict(const std::string& text, int& outValue) {
    if (text.empty()) return false;

    size_t index = 0;
    bool negative = false;
    if (text[0] == '+' || text[0] == '-') {
        negative = (text[0] == '-');
        index = 1;
    }
    if (index >= text.size()) return false;

    long long accumulator = 0;
    for (; index < text.size(); ++index) {
        if (!std::isdigit(static_cast<unsigned char>(text[index]))) return false;
        accumulator = accumulator * 10 + (text[index] - '0');
        if (accumulator > 2147483647LL) return false;      // int overflow guard
    }
    outValue = static_cast<int>(negative ? -accumulator : accumulator);
    return true;
}

int UI::readAnyInt(const std::string& prompt) {
    for (;;) {
        std::string raw = trim(readLine(prompt));
        if (exhausted) return 0;

        int value = 0;
        if (parseIntStrict(raw, value)) return value;

        // Anything else ("abc", "12.5", "3x", "") is rejected and re-asked.
        error("Invalid input. Please enter a valid number.");
    }
}

int UI::readInt(const std::string& prompt, int minValue, int maxValue) {
    for (;;) {
        int value = readAnyInt(prompt);
        if (exhausted) return minValue;
        if (value < minValue || value > maxValue) {
            std::ostringstream range;
            range << "Please enter a number between " << minValue << " and " << maxValue << ".";
            error(range.str());
            continue;
        }
        return value;
    }
}

char UI::readChoice(const std::string& prompt, const std::string& allowed) {
    for (;;) {
        std::string raw = trim(readLine(prompt));
        if (exhausted) return '\0';
        if (raw.size() != 1) {
            error("Please enter exactly one of: " + allowed);
            continue;
        }
        char value = static_cast<char>(std::toupper(static_cast<unsigned char>(raw[0])));
        if (allowed.find(value) == std::string::npos) {
            error("'" + raw + "' is not valid. Allowed answers: " + allowed + ".");
            continue;
        }
        return value;
    }
}

bool UI::confirm(const std::string& question) {
    for (;;) {
        char answer = readChoice(question + " (Y/N): ", "YN");
        if (exhausted) return false;
        return (answer == 'Y');
    }
}

std::string UI::readPassword(const std::string& prompt) {
    std::cout << prompt << std::flush;

#ifdef _WIN32
    if (isInteractiveStandardInput()) {
        // Interactive console: hide what is typed.
        std::string secret;
        for (;;) {
            int key = _getch();
            if (key == 3) {                    // Ctrl+C
                std::cout << std::endl;
                exhausted = true;
                return std::string();
            }
            if (key == '\r' || key == '\n') break;
            if (key == '\b') {
                if (!secret.empty()) {
                    secret.erase(secret.size() - 1);
                    std::cout << "\b \b" << std::flush;
                }
                continue;
            }
            if (key >= 32 && key <= 126) {
                secret += static_cast<char>(key);
                std::cout << '*' << std::flush;
            }
        }
        std::cout << std::endl;
        return secret;
    }
#endif
#ifndef _WIN32
    if (isatty(STDIN_FILENO)) {
        termios original;
        if (tcgetattr(STDIN_FILENO, &original) == 0) {
            termios silent = original;
            silent.c_lflag &= static_cast<tcflag_t>(~ECHO);
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &silent);
            std::string secret;
            int c = 0;
            while ((c = std::getchar()) != '\n' && c != EOF) {
                if (c == '\b' || c == 127) {
                    if (!secret.empty()) {
                        secret.erase(secret.size() - 1);
                        std::cout << "\b \b" << std::flush;
                    }
                } else {
                    secret += static_cast<char>(c);
                    std::cout << '*' << std::flush;
                }
            }
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &original);
            std::cout << std::endl;
            return secret;
        }
    }
#endif
    // Fallback (piped input or unknown terminal): read the line as it is.
    return readLine("");
}

// ---------------------------------------------------------------------------
//  screen / flow
// ---------------------------------------------------------------------------

void UI::pause(const std::string& message) {
    std::cout << std::endl;
    if (exhausted) return;
    readLine(message);
}

void UI::waitForEnter() {
    if (exhausted) return;
    discardRestOfLine();
    std::cout << std::endl;
}

void UI::clearScreen() {
    if (colorEnabled) {
        std::cout << "\033[2J\033[H" << std::flush;
        return;
    }
#ifdef _WIN32
    std::system("cls");
#else
    if (std::system("clear") != 0) { /* nothing else to try */ }
#endif
}

// ---------------------------------------------------------------------------
//  formatting helpers
// ---------------------------------------------------------------------------

std::string UI::repeat(char ch, int count) {
    if (count <= 0) return std::string();
    return std::string(static_cast<size_t>(count), ch);
}

std::string UI::padRight(const std::string& text, size_t width) {
    if (text.size() >= width) return text;
    return text + std::string(width - text.size(), ' ');
}

std::string UI::padLeft(const std::string& text, size_t width) {
    if (text.size() >= width) return text;
    return std::string(width - text.size(), ' ') + text;
}

std::string UI::toUpper(const std::string& text) {
    std::string out = text;
    for (size_t i = 0; i < out.size(); ++i) out[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(out[i])));
    return out;
}

std::string UI::toLower(const std::string& text) {
    std::string out = text;
    for (size_t i = 0; i < out.size(); ++i) out[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(out[i])));
    return out;
}

std::string UI::joinTexts(const std::vector<std::string>& parts, const std::string& separator) {
    std::string out;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) out += separator;
        out += parts[i];
    }
    return out;
}

std::string UI::count(int value, const std::string& singular, const std::string& plural) {
    std::string many = plural.empty() ? (singular + "s") : plural;
    return std::to_string(value) + " " + (value == 1 ? singular : many);
}

std::string UI::fixed(double value, int decimals) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(decimals) << value;
    return out.str();
}

bool UI::containsLetters(const std::string& text) {
    for (size_t i = 0; i < text.size(); ++i) {
        if (std::isalpha(static_cast<unsigned char>(text[i]))) return true;
    }
    return false;
}

bool UI::isAlphanumeric(const std::string& text) {
    if (text.empty()) return false;
    for (size_t i = 0; i < text.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(text[i]);
        if (!std::isalnum(c) && c != '_' && c != '.' && c != '-') return false;
    }
    return true;
}