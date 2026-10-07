#ifndef FILESTORE_H
#define FILESTORE_H

#include <string>
#include <vector>

/*
 * FileStore - a stateless helper class (all members are static) that owns every
 *              detail about the flat-file persistence layer.
 *
 * Record format
 * -------------
 *   Every line of every data file is one record:
 *        TAG|field1|field2|field3...
 *   Fields are separated by '|'. Because a user is allowed to type anything,
 *   a tiny escape scheme is applied before writing and undone after reading:
 *        '\'  ->  "\\"
 *        '|'  ->  "\|"
 *   A new line always terminates a record, and the application never accepts
 *   multi line input, so a record can never be split accidentally.
 *
 * Failure handling
 * ----------------
 *   readLines()  throws std::runtime_error when the file cannot be opened.
 *                Callers decide whether that is fatal (first run) or not.
 *   writeLines() throws std::runtime_error and leaves the original file
 *                untouched: it writes to "name.tmp" first and only replaces
 *                the original after the temporary file was closed successfully.
 */
class FileStore {
public:
    // ---------- paths -------------------------------------------------------
    static void setDataDirectory(const std::string& directory);
    static std::string getDataDirectory();
    static std::string pathFor(const std::string& fileName);
    static std::string usersFile();
    static std::string questionsFile();
    static std::string quizzesFile();
    static std::string resultsFile();

    static bool fileExists(const std::string& fileName);
    static bool ensureDataDirectory();       // creates ./data if missing
    static void ensureDataFiles();           // creates the 4 files if missing
    static long long fileSize(const std::string& fileName);

    // ---------- raw file access ----------------------------------------------
    static std::vector<std::string> readLines(const std::string& fileName);
    static void writeLines(const std::string& fileName,
                           const std::vector<std::string>& lines);
    static void appendLine(const std::string& fileName, const std::string& line);
    static void removeFile(const std::string& fileName);

    // ---------- record helpers ----------------------------------------------
    static std::vector<std::string> split(const std::string& line);
    static std::string join(const std::vector<std::string>& fields);
    static std::string escapeField(const std::string& value);
    static std::string unescapeField(const std::string& value);
    static std::string makeRecord(const std::vector<std::string>& fields);
    static bool isBlankLine(const std::string& line);

    // ---------- field converters (throw std::invalid_argument on garbage) ----
    static int toInt(const std::string& value, const std::string& context);
    static double toDouble(const std::string& value, const std::string& context);
    static std::string toText(double value, int decimals);

private:
    FileStore();                              // never constructed

    static std::string dataDirectory;
};

#endif // FILESTORE_H