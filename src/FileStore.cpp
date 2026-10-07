#include "FileStore.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#ifdef _WIN32
#include <direct.h>
#include <sys/stat.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

std::string FileStore::dataDirectory = "data";

namespace {
const char* SEPARATOR = "|";

// "data" -> "data" + separator (Windows and POSIX both accept '/')
std::string joinPath(const std::string& directory, const std::string& name) {
    if (directory.empty()) return name;
    if (directory[directory.size() - 1] == '/' || directory[directory.size() - 1] == '\\') {
        return directory + name;
    }
    return directory + "/" + name;
}
}

// ---------------------------------------------------------------------------
//  paths
// ---------------------------------------------------------------------------

void FileStore::setDataDirectory(const std::string& directory) {
    dataDirectory = directory.empty() ? std::string("data") : directory;
}

std::string FileStore::getDataDirectory() { return dataDirectory; }

std::string FileStore::pathFor(const std::string& fileName) {
    return joinPath(dataDirectory, fileName);
}

std::string FileStore::usersFile()     { return pathFor("users.txt"); }
std::string FileStore::questionsFile() { return pathFor("questions.txt"); }
std::string FileStore::quizzesFile()   { return pathFor("quizzes.txt"); }
std::string FileStore::resultsFile()   { return pathFor("results.txt"); }

bool FileStore::fileExists(const std::string& fileName) {
    std::ifstream stream(fileName.c_str());
    return stream.good();
}

bool FileStore::ensureDataDirectory() {
#ifdef _WIN32
    int code = _mkdir(dataDirectory.c_str());
#else
    int code = mkdir(dataDirectory.c_str(), 0775);
#endif
    if (code == 0) return true;

    // Already exists is success, anything else is a real problem.
    struct stat info;
    if (stat(dataDirectory.c_str(), &info) == 0) return true;
    return false;
}

void FileStore::ensureDataFiles() {
    const char* files[] = { "users.txt", "questions.txt", "quizzes.txt", "results.txt" };
    for (size_t i = 0; i < sizeof(files) / sizeof(files[0]); ++i) {
        std::string path = pathFor(files[i]);
        if (!fileExists(path)) {
            std::ofstream create(path.c_str(), std::ios::out | std::ios::trunc);
            if (!create.is_open()) {
                throw std::runtime_error("Unable to create data file: " + path);
            }
        }
    }
}

long long FileStore::fileSize(const std::string& fileName) {
    std::ifstream stream(fileName.c_str(), std::ios::binary | std::ios::ate);
    if (!stream.is_open()) return -1;
    return static_cast<long long>(stream.tellg());
}

// ---------------------------------------------------------------------------
//  raw file access
// ---------------------------------------------------------------------------

std::vector<std::string> FileStore::readLines(const std::string& fileName) {
    std::vector<std::string> lines;
    std::ifstream stream(fileName.c_str());
    if (!stream.is_open()) {
        throw std::runtime_error("Could not open file for reading: " + fileName);
    }

    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1);
        lines.push_back(line);
    }
    if (stream.bad()) {
        throw std::runtime_error("Error while reading file: " + fileName);
    }
    return lines;
}

void FileStore::writeLines(const std::string& fileName, const std::vector<std::string>& lines) {
    // Write to a temporary file first: if anything goes wrong the previous
    // version of the data file survives untouched.
    const std::string temporary = fileName + ".tmp";
    {
        std::ofstream stream(temporary.c_str(), std::ios::out | std::ios::trunc);
        if (!stream.is_open()) {
            throw std::runtime_error("Could not open file for writing: " + temporary);
        }
        for (size_t i = 0; i < lines.size(); ++i) {
            stream << lines[i] << "\n";
        }
        stream.flush();
        if (!stream.good()) {
            stream.close();
            std::remove(temporary.c_str());
            throw std::runtime_error("Error while writing file: " + temporary);
        }
    }

    std::remove(fileName.c_str());
    if (std::rename(temporary.c_str(), fileName.c_str()) != 0) {
        std::remove(temporary.c_str());
        throw std::runtime_error("Could not replace file: " + fileName);
    }
}

void FileStore::appendLine(const std::string& fileName, const std::string& line) {
    // If the file does not end with a newline (truncated file, hand edited
    // file) the new record would be glued onto the last one. Terminate the
    // previous line first so that existing data can never be corrupted.
    bool needsNewline = false;
    {
        std::fstream probe(fileName.c_str(), std::ios::in | std::ios::binary);
        if (probe.is_open()) {
            probe.seekg(0, std::ios::end);
            std::streamoff size = static_cast<std::streamoff>(probe.tellg());
            if (size > 0) {
                probe.seekg(-1, std::ios::end);
                char last = '\0';
                if (probe.get(last) && last != '\n' && last != '\r') needsNewline = true;
            }
        }
    }

    std::ofstream stream(fileName.c_str(), std::ios::out | std::ios::app);
    if (!stream.is_open()) {
        throw std::runtime_error("Could not open file for appending: " + fileName);
    }
    if (needsNewline) stream << "\n";
    stream << line << "\n";
    stream.flush();
    if (!stream.good()) {
        throw std::runtime_error("Error while appending to file: " + fileName);
    }
}

void FileStore::removeFile(const std::string& fileName) {
    std::remove(fileName.c_str());
}

// ---------------------------------------------------------------------------
//  record helpers
// ---------------------------------------------------------------------------

std::vector<std::string> FileStore::split(const std::string& line) {
    // Only a *bare* separator ends a field. A separator that follows a
    // backslash is part of the value (see escapeField), so the escape pair is
    // copied through untouched and unescapeField() restores it later.
    std::vector<std::string> fields;
    std::string current;
    for (size_t i = 0; i < line.size(); ++i) {
        if (line[i] == '\\' && i + 1 < line.size()) {
            current += line[i];
            current += line[i + 1];
            ++i;
            continue;
        }
        if (line[i] == SEPARATOR[0]) {
            fields.push_back(current);
            current.clear();
        } else {
            current += line[i];
        }
    }
    fields.push_back(current);
    return fields;
}

std::string FileStore::join(const std::vector<std::string>& fields) {
    std::string result;
    for (size_t i = 0; i < fields.size(); ++i) {
        if (i > 0) result += SEPARATOR;
        result += fields[i];
    }
    return result;
}

std::string FileStore::escapeField(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size() + 8);
    for (size_t i = 0; i < value.size(); ++i) {
        char c = value[i];
        if (c == '\\' || c == SEPARATOR[0]) escaped += '\\';
        escaped += c;
    }
    return escaped;
}

std::string FileStore::unescapeField(const std::string& value) {
    std::string result;
    result.reserve(value.size());
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '\\' && i + 1 < value.size()) {
            ++i;
            result += value[i];
        } else {
            result += value[i];
        }
    }
    return result;
}

std::string FileStore::makeRecord(const std::vector<std::string>& fields) {
    std::vector<std::string> safe;
    safe.reserve(fields.size());
    for (size_t i = 0; i < fields.size(); ++i) {
        std::string escaped = escapeField(fields[i]);
        // A record must stay on one line.
        for (size_t c = 0; c < escaped.size(); ++c) {
            if (escaped[c] == '\n' || escaped[c] == '\r') escaped[c] = ' ';
        }
        safe.push_back(escaped);
    }
    return join(safe);
}

bool FileStore::isBlankLine(const std::string& line) {
    for (size_t i = 0; i < line.size(); ++i) {
        if (!std::isspace(static_cast<unsigned char>(line[i]))) return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
//  field converters
// ---------------------------------------------------------------------------

int FileStore::toInt(const std::string& value, const std::string& context) {
    std::string text = value;
    // strip surrounding spaces that may survive an old, hand edited file
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text[text.size() - 1]))) {
        text.erase(text.size() - 1);
    }
    if (text.empty()) {
        throw std::invalid_argument(context + ": expected a number but the field was empty.");
    }

    size_t index = 0;
    if (text[0] == '+' || text[0] == '-') index = 1;
    if (index >= text.size()) {
        throw std::invalid_argument(context + ": '" + value + "' is not a number.");
    }
    for (size_t i = index; i < text.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(text[i]))) {
            throw std::invalid_argument(context + ": '" + value + "' is not a number.");
        }
    }
    return std::atoi(text.c_str());
}

double FileStore::toDouble(const std::string& value, const std::string& context) {
    if (value.empty()) {
        throw std::invalid_argument(context + ": expected a decimal number but the field was empty.");
    }
    const char* start = value.c_str();
    char* end = 0;
    double parsed = std::strtod(start, &end);
    if (end == start) {
        throw std::invalid_argument(context + ": '" + value + "' is not a decimal number.");
    }
    return parsed;
}

std::string FileStore::toText(double value, int decimals) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(decimals) << value;
    return out.str();
}