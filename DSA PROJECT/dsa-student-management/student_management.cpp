/* ============================================================================
   PLMun Student Record Management System
   ----------------------------------------------------------------------------
   A console-based student record management system written in C++17 for the
   Data Structures and Algorithms final project of Group 7 (BSIT 2H).

   Core data structures (hand-written, no STL containers for storage):
     - Singly linked list   : primary storage for student records
     - Stack (LIFO)         : recently deleted records (undo / restore)
     - Queue (FIFO)         : student service line
     - Dynamic pointer array: temporary index used by binary search

   Algorithms:
     - Linear search (by ID and by last name)
     - Binary search (by ID, on a sorted pointer array)
     - Bubble sort (by ID, last name, GPA)
     - Selection sort (by ID, last name, GPA)
     - Recursion (count records, display records in reverse)
     - CSV file persistence (load on startup, auto-save after every change)

   Build (MinGW / g++):
     g++ -std=c++17 -Wall -Wextra -Wpedantic -O2 student_management.cpp ^
         -o student_management.exe

   Build (MSVC Developer Command Prompt):
     cl /std:c++17 /EHsc /W4 student_management.cpp

   Run:
     student_management.exe
   ============================================================================ */

#include <cerrno>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <utility>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::fixed;
using std::left;
using std::right;
using std::setfill;
using std::setprecision;
using std::setw;
using std::string;

/* ============================================================================
   SECTION 1. CONSTANTS AND SMALL UTILITIES
   ============================================================================ */

const char* const DATA_FILE = "students.csv";
const int MIN_ID = 100000;          // six digits
const int MAX_ID = 999999999;       // nine digits
const double MIN_GPA = 1.00;        // 1.00 is the highest/best grade
const double MAX_GPA = 5.00;        // 5.00 is failing
const int MENU_WIDTH = 82;

string trim(const string& text) {
    const size_t begin = text.find_first_not_of(" \t\r\n");
    if (begin == string::npos) {
        return "";
    }
    const size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

string toLower(string text) {
    for (char& c : text) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return text;
}

string fit(const string& text, size_t width) {
    if (text.size() <= width) {
        return text;
    }
    if (width <= 3) {
        return text.substr(0, width);
    }
    return text.substr(0, width - 3) + "...";
}

string formatGpa(double gpa) {
    std::ostringstream out;
    out << fixed << setprecision(2) << gpa;
    return out.str();
}

string formatPercent(int part, int total) {
    if (total <= 0) {
        return "0.0%";
    }
    std::ostringstream out;
    out << fixed << setprecision(1) << (100.0 * part / total) << "%";
    return out.str();
}

string centered(const string& text, int width = MENU_WIDTH) {
    const int length = static_cast<int>(text.size());
    if (length >= width) {
        return text;
    }
    return string((width - length) / 2, ' ') + text;
}

void printRule(char character = '=', int width = MENU_WIDTH) {
    cout << string(static_cast<size_t>(width), character) << '\n';
}

string currentTimestamp() {
    const std::time_t now = std::time(nullptr);
    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif
    char buffer[32] = {};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &localTime);
    return string(buffer);
}

bool isInteractiveConsole() {
#ifdef _WIN32
    return _isatty(_fileno(stdin)) != 0;
#else
    return isatty(fileno(stdin)) != 0;
#endif
}

void clearScreen() {
    // When input or output is redirected (for automated tests or file
    // capture), do not clear the screen so the transcript stays readable.
    if (!isInteractiveConsole()) {
        return;
    }
#ifdef _WIN32
    std::system("cls");
#else
    std::system("clear");
#endif
}

void pauseScreen() {
    cout << "\nPress Enter to continue...";
    string ignored;
    if (!std::getline(cin, ignored)) {
        cout << '\n';
    }
}

/* ---------------------------- Input helpers ------------------------------ */

string readLine(const string& prompt) {
    cout << prompt;
    string line;
    if (!std::getline(cin, line)) {
        cout << "\nInput stream closed. Exiting.\n";
        std::exit(0);
    }
    line = trim(line);
    // When input is redirected (automated tests, captured transcripts), echo
    // the answer so the transcript reads like a real console session.
    if (!isInteractiveConsole()) {
        cout << line << '\n';
    }
    return line;
}

int readInt(const string& prompt, int low, int high) {
    while (true) {
        const string text = readLine(prompt);
        std::istringstream parser(text);
        int value = 0;
        char extra = '\0';
        if ((parser >> value) && !(parser >> extra) && value >= low && value <= high) {
            return value;
        }
        cout << "  Invalid input. Enter a whole number from " << low
             << " to " << high << ".\n";
    }
}

int readIntWithDefault(const string& label, int current, int low, int high) {
    while (true) {
        const string text = readLine(label + " [" + std::to_string(current) + "]: ");
        if (text.empty()) {
            return current;
        }
        std::istringstream parser(text);
        int value = 0;
        char extra = '\0';
        if ((parser >> value) && !(parser >> extra) && value >= low && value <= high) {
            return value;
        }
        cout << "  Invalid input. Enter a whole number from " << low
             << " to " << high << ", or press Enter to keep " << current << ".\n";
    }
}

bool isValidName(const string& name) {
    if (name.size() < 2) {
        return false;
    }
    bool hasLetter = false;
    for (const char c : name) {
        const unsigned char u = static_cast<unsigned char>(c);
        if (std::isalpha(u) || u >= 128) {
            hasLetter = true;
            continue;
        }
        if (c == ' ' || c == '-' || c == '\'' || c == '.') {
            continue;
        }
        return false;
    }
    return hasLetter;
}

bool isValidCourse(const string& course) {
    if (course.size() < 2) {
        return false;
    }
    bool hasLetter = false;
    for (const char c : course) {
        const unsigned char u = static_cast<unsigned char>(c);
        if (std::isalnum(u) || u >= 128) {
            hasLetter = true;
            continue;
        }
        if (c == ' ' || c == '-' || c == '&' || c == '.' ||
            c == '/' || c == '(' || c == ')') {
            continue;
        }
        return false;
    }
    return hasLetter;
}

string readName(const string& prompt) {
    while (true) {
        const string value = readLine(prompt);
        if (isValidName(value)) {
            return value;
        }
        cout << "  Invalid name. Use letters, spaces, hyphens, apostrophes, or\n"
             << "  periods (at least two characters).\n";
    }
}

string readCourse(const string& prompt) {
    while (true) {
        const string value = readLine(prompt);
        if (isValidCourse(value)) {
            return value;
        }
        cout << "  Invalid course. Use letters, digits, spaces, or - & . / ( ).\n";
    }
}

string readNameWithDefault(const string& label, const string& current) {
    while (true) {
        const string value = readLine(label + " [" + current + "]: ");
        if (value.empty()) {
            return current;
        }
        if (isValidName(value)) {
            return value;
        }
        cout << "  Invalid name. Press Enter to keep the current value.\n";
    }
}

string readCourseWithDefault(const string& label, const string& current) {
    while (true) {
        const string value = readLine(label + " [" + current + "]: ");
        if (value.empty()) {
            return current;
        }
        if (isValidCourse(value)) {
            return value;
        }
        cout << "  Invalid course. Press Enter to keep the current value.\n";
    }
}

double readGpa(const string& prompt) {
    while (true) {
        const string text = readLine(prompt);
        std::istringstream parser(text);
        double value = 0.0;
        char extra = '\0';
        if ((parser >> value) && !(parser >> extra) &&
            value >= MIN_GPA && value <= MAX_GPA) {
            return std::round(value * 100.0) / 100.0;
        }
        cout << "  Invalid GPA. Enter a number from 1.00 (best) to 5.00 (failing).\n";
    }
}

double readGpaWithDefault(const string& label, double current) {
    while (true) {
        const string text = readLine(label + " [" + formatGpa(current) + "]: ");
        if (text.empty()) {
            return current;
        }
        std::istringstream parser(text);
        double value = 0.0;
        char extra = '\0';
        if ((parser >> value) && !(parser >> extra) &&
            value >= MIN_GPA && value <= MAX_GPA) {
            return std::round(value * 100.0) / 100.0;
        }
        cout << "  Invalid GPA. Enter 1.00 to 5.00, or press Enter to keep "
             << formatGpa(current) << ".\n";
    }
}

bool confirm(const string& prompt, bool defaultYes = false) {
    const string suffix = defaultYes ? " [Y/n]: " : " [y/N]: ";
    while (true) {
        const string value = toLower(readLine(prompt + suffix));
        if (value.empty()) {
            return defaultYes;
        }
        if (value == "y" || value == "yes") {
            return true;
        }
        if (value == "n" || value == "no") {
            return false;
        }
        cout << "  Please answer y or n.\n";
    }
}

/* --------------------------- CSV and parsing ----------------------------- */

string csvQuote(const string& value) {
    if (value.find_first_of(",\"\r\n") == string::npos) {
        return value;
    }
    string result = "\"";
    for (const char c : value) {
        if (c == '"') {
            result += "\"\"";
        } else {
            result += c;
        }
    }
    result += '"';
    return result;
}

// Returns the number of fields, or -1 when the line is malformed.
int parseCsvLine(const string& line, string fields[], int maxFields) {
    int count = 0;
    bool inQuotes = false;
    bool overflow = false;
    string current;

    for (size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    current += '"';
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                current += c;
            }
        } else if (c == '"') {
            inQuotes = true;
        } else if (c == ',') {
            if (count < maxFields) {
                fields[count++] = current;
            } else {
                overflow = true;
            }
            current.clear();
        } else {
            current += c;
        }
    }

    if (inQuotes) {
        return -1;
    }
    if (count < maxFields) {
        fields[count++] = current;
    } else {
        overflow = true;
    }
    return overflow ? -1 : count;
}

bool parseId(const string& text, int& value) {
    if (text.empty()) {
        return false;
    }
    for (const char c : text) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    errno = 0;
    char* end = nullptr;
    const long long parsed = std::strtoll(text.c_str(), &end, 10);
    if (errno != 0 || end == nullptr || *end != '\0' ||
        parsed < MIN_ID || parsed > MAX_ID) {
        return false;
    }
    value = static_cast<int>(parsed);
    return true;
}

bool parseGpaText(const string& text, double& value) {
    if (text.empty()) {
        return false;
    }
    std::istringstream parser(text);
    double parsed = 0.0;
    char extra = '\0';
    if (!(parser >> parsed) || (parser >> extra)) {
        return false;
    }
    if (parsed < MIN_GPA || parsed > MAX_GPA) {
        return false;
    }
    value = parsed;
    return true;
}

/* ============================================================================
   SECTION 2. STUDENT RECORD AND TABLE OUTPUT
   ============================================================================ */

struct Student {
    int id;
    string firstName;
    string lastName;
    string course;
    int yearLevel;
    double gpa;

    Student() : id(0), yearLevel(1), gpa(0.0) {}

    Student(int idValue, string first, string last, string courseName,
            int year, double grade)
        : id(idValue),
          firstName(std::move(first)),
          lastName(std::move(last)),
          course(std::move(courseName)),
          yearLevel(year),
          gpa(grade) {}

    string fullName() const {
        return lastName + ", " + firstName;
    }
};

enum class SortKey { ID, LAST_NAME, GPA };

string sortKeyName(SortKey key) {
    switch (key) {
        case SortKey::ID:        return "Student ID";
        case SortKey::LAST_NAME: return "Last Name";
        case SortKey::GPA:       return "GPA";
    }
    return "Unknown";
}

// Negative when a comes before b, positive when a comes after b.
int compareStudents(const Student& a, const Student& b, SortKey key) {
    switch (key) {
        case SortKey::ID:
            if (a.id < b.id) return -1;
            if (a.id > b.id) return 1;
            return 0;

        case SortKey::LAST_NAME: {
            const string aLast = toLower(a.lastName);
            const string bLast = toLower(b.lastName);
            if (aLast < bLast) return -1;
            if (aLast > bLast) return 1;
            const string aFirst = toLower(a.firstName);
            const string bFirst = toLower(b.firstName);
            if (aFirst < bFirst) return -1;
            if (aFirst > bFirst) return 1;
            if (a.id < b.id) return -1;
            if (a.id > b.id) return 1;
            return 0;
        }

        case SortKey::GPA:
            if (a.gpa < b.gpa) return -1;   // lower numeric GPA is better
            if (a.gpa > b.gpa) return 1;
            return compareStudents(a, b, SortKey::LAST_NAME); // stable tie-break
    }
    return 0;
}

void printStudentTableHeader() {
    cout << left
         << setw(11) << "ID"
         << setw(30) << "NAME (Last, First)"
         << setw(22) << "COURSE"
         << setw(5)  << "YR"
         << setw(7)  << "GPA"
         << '\n';
    printRule('-', 75);
}

void printStudentRow(const Student& student, int position = 0) {
    if (position > 0) {
        cout << left << setw(5) << position;
    }
    cout << left
         << setw(11) << student.id
         << setw(30) << fit(student.fullName(), 29)
         << setw(22) << fit(student.course, 21)
         << setw(5)  << student.yearLevel
         << setw(7)  << formatGpa(student.gpa)
         << '\n';
}

void printStudentDetails(const Student& student) {
    cout << "  ID       : " << student.id << '\n'
         << "  Name     : " << student.lastName << ", " << student.firstName << '\n'
         << "  Course   : " << student.course << '\n'
         << "  Year     : " << student.yearLevel << '\n'
         << "  GPA      : " << formatGpa(student.gpa) << "  (1.00 is best)\n";
}

/* ============================================================================
   SECTION 3. PRIMARY DATA STRUCTURE: SINGLY LINKED LIST
   ----------------------------------------------------------------------------
   Each StudentNode owns one Student record and one forward pointer. The list
   keeps both head and tail pointers so appending a new record is O(1), while
   searching, updating, and deleting remain O(n) because a singly linked list
   has no random access.
   ============================================================================ */

struct StudentNode {
    Student data;
    StudentNode* next;

    explicit StudentNode(const Student& student)
        : data(student), next(nullptr) {}
};

struct SortStats {
    long long comparisons = 0;
    long long swaps = 0;
};

struct SearchResult {
    const Student* student = nullptr;
    long long comparisons = 0;
    bool arrayPrepared = false;

    bool found() const {
        return student != nullptr;
    }
};

struct StudentStats {
    int total = 0;
    double averageGpa = 0.0;
    double bestGpa = 0.0;      // lowest numeric value = highest/best grade
    double worstGpa = 0.0;
    string bestStudent;
    string worstStudent;
    int passing = 0;           // GPA <= 3.00
    int failing = 0;           // GPA > 3.00
    int deansList = 0;         // GPA <= 1.75
};

class StudentList {
public:
    StudentList() : head(nullptr), tail(nullptr), nodeCount(0) {}

    ~StudentList() {
        clear();
    }

    // A deep copy would duplicate every node. The system owns exactly one list,
    // so copying is disabled to prevent accidental double deletion.
    StudentList(const StudentList&) = delete;
    StudentList& operator=(const StudentList&) = delete;

    bool isEmpty() const {
        return head == nullptr;
    }

    int size() const {
        return nodeCount;
    }

    void clear() {
        StudentNode* current = head;
        while (current != nullptr) {
            StudentNode* next = current->next;
            delete current;
            current = next;
        }
        head = nullptr;
        tail = nullptr;
        nodeCount = 0;
    }

    // O(n): the linked list has no index, so duplicates are checked by walking.
    bool containsId(int id) const {
        return findNode(id) != nullptr;
    }

    // O(1): append using the tail pointer.
    bool add(const Student& student) {
        if (containsId(student.id)) {
            return false;
        }
        StudentNode* node = new StudentNode(student);
        if (tail == nullptr) {
            head = node;
            tail = node;
        } else {
            tail->next = node;
            tail = node;
        }
        ++nodeCount;
        return true;
    }

    Student* findById(int id) {
        StudentNode* node = findNode(id);
        return node == nullptr ? nullptr : &node->data;
    }

    const Student* findById(int id) const {
        const StudentNode* node = findNode(id);
        return node == nullptr ? nullptr : &node->data;
    }

    // O(n): find the node, copy the old record, then overwrite the data.
    bool update(int id, const Student& updated, Student& oldRecord) {
        StudentNode* node = findNode(id);
        if (node == nullptr) {
            return false;
        }
        oldRecord = node->data;
        Student replacement = updated;
        replacement.id = id; // the student ID is the primary key and never changes
        node->data = replacement;
        return true;
    }

    // O(n): unlink the node and release its memory.
    bool remove(int id, Student& removedRecord) {
        StudentNode* previous = nullptr;
        StudentNode* current = head;

        while (current != nullptr && current->data.id != id) {
            previous = current;
            current = current->next;
        }
        if (current == nullptr) {
            return false;
        }

        removedRecord = current->data;
        if (previous == nullptr) {
            head = current->next;
        } else {
            previous->next = current->next;
        }
        if (current == tail) {
            tail = previous;
        }
        delete current;
        --nodeCount;
        return true;
    }

    void displayAll() const {
        if (head == nullptr) {
            cout << "  (no records to display)\n";
            return;
        }
        printStudentTableHeader();
        for (const StudentNode* node = head; node != nullptr; node = node->next) {
            printStudentRow(node->data);
        }
    }

    // Recursion demonstration: the call stack reverses the list for free.
    void displayReverse() const {
        if (head == nullptr) {
            cout << "  (no records to display)\n";
            return;
        }
        printStudentTableHeader();
        displayReverseRecursive(head);
    }

    int countRecursive() const {
        return countRecursive(head);
    }

    // Bubble sort directly on the linked list. Swapping whole records keeps the
    // nodes in place; only the data moves. Stable: equal keys never swap.
    SortStats bubbleSort(SortKey key) {
        SortStats stats;
        if (head == nullptr || head->next == nullptr) {
            return stats;
        }

        bool swapped = false;
        do {
            swapped = false;
            for (StudentNode* current = head; current->next != nullptr;
                 current = current->next) {
                ++stats.comparisons;
                if (compareStudents(current->data, current->next->data, key) > 0) {
                    std::swap(current->data, current->next->data);
                    ++stats.swaps;
                    swapped = true;
                }
            }
        } while (swapped);

        return stats;
    }

    // Selection sort on the linked list. It scans the unsorted suffix for the
    // smallest key and swaps it into place. Not stable because of long swaps.
    SortStats selectionSort(SortKey key) {
        SortStats stats;
        for (StudentNode* start = head; start != nullptr && start->next != nullptr;
             start = start->next) {
            StudentNode* smallest = start;
            for (StudentNode* current = start->next; current != nullptr;
                 current = current->next) {
                ++stats.comparisons;
                if (compareStudents(current->data, smallest->data, key) < 0) {
                    smallest = current;
                }
            }
            if (smallest != start) {
                std::swap(start->data, smallest->data);
                ++stats.swaps;
            }
        }
        return stats;
    }

    bool isSortedBy(SortKey key) const {
        for (const StudentNode* node = head;
             node != nullptr && node->next != nullptr; node = node->next) {
            if (compareStudents(node->data, node->next->data, key) > 0) {
                return false;
            }
        }
        return true;
    }

    // Linear search: inspect every node until the ID is found.
    SearchResult linearSearchById(int id) const {
        SearchResult result;
        for (const StudentNode* node = head; node != nullptr; node = node->next) {
            ++result.comparisons;
            if (node->data.id == id) {
                result.student = &node->data;
                break;
            }
        }
        return result;
    }

    // Binary search needs random access, which a linked list does not provide.
    // The method first copies node pointers into a dynamic array. If the list is
    // not already sorted by ID, that array is sorted with selection sort.
    SearchResult binarySearchById(int id) const {
        SearchResult result;
        if (head == nullptr) {
            return result;
        }

        StudentNode** index = new StudentNode*[static_cast<size_t>(nodeCount)];
        int position = 0;
        for (StudentNode* node = head; node != nullptr; node = node->next) {
            index[position++] = node;
        }

        if (!isSortedBy(SortKey::ID)) {
            for (int i = 0; i < nodeCount - 1; ++i) {
                int smallest = i;
                for (int j = i + 1; j < nodeCount; ++j) {
                    if (compareStudents(index[j]->data, index[smallest]->data,
                                        SortKey::ID) < 0) {
                        smallest = j;
                    }
                }
                if (smallest != i) {
                    std::swap(index[i], index[smallest]);
                }
            }
            result.arrayPrepared = true;
        }

        int low = 0;
        int high = nodeCount - 1;
        while (low <= high) {
            const int middle = low + (high - low) / 2;
            ++result.comparisons;
            if (index[middle]->data.id == id) {
                result.student = &index[middle]->data;
                break;
            }
            if (index[middle]->data.id < id) {
                low = middle + 1;
            } else {
                high = middle - 1;
            }
        }

        delete[] index;
        return result;
    }

    // Linear search by last name. Returns the number of matches; up to
    // maxOutput pointers are written to output. Comparisons are reported so the
    // student can compare this with binary search.
    int findByLastName(const string& query, const Student** output,
                       int maxOutput, long long& comparisons) const {
        comparisons = 0;
        const string needle = toLower(query);
        int found = 0;
        for (const StudentNode* node = head; node != nullptr; node = node->next) {
            ++comparisons;
            if (toLower(node->data.lastName).find(needle) != string::npos) {
                if (found < maxOutput) {
                    output[found] = &node->data;
                }
                ++found;
            }
        }
        return found;
    }

    StudentStats getStatistics() const {
        StudentStats stats;
        stats.total = nodeCount;
        if (head == nullptr) {
            return stats;
        }

        stats.bestGpa = head->data.gpa;
        stats.worstGpa = head->data.gpa;
        stats.bestStudent = describeStudent(head->data);
        stats.worstStudent = describeStudent(head->data);

        double sum = 0.0;
        for (const StudentNode* node = head; node != nullptr; node = node->next) {
            const Student& student = node->data;
            sum += student.gpa;

            if (student.gpa < stats.bestGpa) {
                stats.bestGpa = student.gpa;
                stats.bestStudent = describeStudent(student);
            }
            if (student.gpa > stats.worstGpa) {
                stats.worstGpa = student.gpa;
                stats.worstStudent = describeStudent(student);
            }
            if (student.gpa <= 3.00) {
                ++stats.passing;
            } else {
                ++stats.failing;
            }
            if (student.gpa <= 1.75) {
                ++stats.deansList;
            }
        }
        stats.averageGpa = sum / nodeCount;
        return stats;
    }

    void printCourseDistribution() const {
        if (head == nullptr) {
            cout << "  (no data)\n";
            return;
        }

        struct CourseEntry {
            string course;
            int count = 0;
        };

        CourseEntry* entries = new CourseEntry[static_cast<size_t>(nodeCount)];
        int uniqueCourses = 0;

        for (const StudentNode* node = head; node != nullptr; node = node->next) {
            const string key = toLower(node->data.course);
            int match = -1;
            for (int i = 0; i < uniqueCourses; ++i) {
                if (toLower(entries[i].course) == key) {
                    match = i;
                    break;
                }
            }
            if (match >= 0) {
                ++entries[match].count;
            } else {
                entries[uniqueCourses].course = node->data.course;
                entries[uniqueCourses].count = 1;
                ++uniqueCourses;
            }
        }

        // Selection sort the small course table by count (descending), then name.
        for (int i = 0; i < uniqueCourses - 1; ++i) {
            int chosen = i;
            for (int j = i + 1; j < uniqueCourses; ++j) {
                if (entries[j].count > entries[chosen].count ||
                    (entries[j].count == entries[chosen].count &&
                     toLower(entries[j].course) < toLower(entries[chosen].course))) {
                    chosen = j;
                }
            }
            if (chosen != i) {
                std::swap(entries[i], entries[chosen]);
            }
        }

        for (int i = 0; i < uniqueCourses; ++i) {
            cout << "  " << left << setw(28) << fit(entries[i].course, 27)
                 << right << setw(5) << entries[i].count << "  "
                 << setw(7) << formatPercent(entries[i].count, nodeCount)
                 << '\n';
        }

        delete[] entries;
    }

    bool saveToFile(const string& filename) const {
        const string temporary = filename + ".tmp";
        std::ofstream out(temporary, std::ios::trunc);
        if (!out) {
            return false;
        }

        out << "id,firstName,lastName,course,yearLevel,gpa\n";
        for (const StudentNode* node = head; node != nullptr; node = node->next) {
            const Student& student = node->data;
            out << student.id << ','
                << csvQuote(student.firstName) << ','
                << csvQuote(student.lastName) << ','
                << csvQuote(student.course) << ','
                << student.yearLevel << ','
                << formatGpa(student.gpa) << '\n';
        }
        out.flush();

        if (!out) {
            out.close();
            std::remove(temporary.c_str());
            return false;
        }
        out.close();

        // Replace the old file only after the new one is complete.
        std::remove(filename.c_str());
        if (std::rename(temporary.c_str(), filename.c_str()) != 0) {
            std::remove(temporary.c_str());
            return false;
        }
        return true;
    }

    bool loadFromFile(const string& filename, int& loaded, int& skipped) {
        loaded = 0;
        skipped = 0;

        std::ifstream in(filename);
        if (!in) {
            return false;
        }

        clear();
        string line;
        bool firstLine = true;

        while (std::getline(in, line)) {
            if (trim(line).empty()) {
                continue;
            }
            if (firstLine) {
                firstLine = false;
                const string lowered = toLower(trim(line));
                if (lowered.rfind("id,", 0) == 0 || lowered == "id") {
                    continue; // header row
                }
            }

            string fields[6];
            if (parseCsvLine(line, fields, 6) != 6) {
                ++skipped;
                continue;
            }

            int id = 0;
            int year = 0;
            double gpa = 0.0;
            std::istringstream yearParser(fields[4]);
            char extra = '\0';

            if (!parseId(trim(fields[0]), id) ||
                !(yearParser >> year) || (yearParser >> extra) ||
                !parseGpaText(trim(fields[5]), gpa) ||
                year < 1 || year > 5 ||
                !isValidName(trim(fields[1])) ||
                !isValidName(trim(fields[2])) ||
                !isValidCourse(trim(fields[3])) ||
                containsId(id)) {
                ++skipped;
                continue;
            }

            add(Student(id, trim(fields[1]), trim(fields[2]),
                        trim(fields[3]), year, gpa));
            ++loaded;
        }

        return true;
    }

private:
    StudentNode* head;
    StudentNode* tail;
    int nodeCount;

    StudentNode* findNode(int id) const {
        for (StudentNode* node = head; node != nullptr; node = node->next) {
            if (node->data.id == id) {
                return node;
            }
        }
        return nullptr;
    }

    int countRecursive(const StudentNode* node) const {
        if (node == nullptr) {
            return 0;
        }
        return 1 + countRecursive(node->next);
    }

    void displayReverseRecursive(const StudentNode* node) const {
        if (node == nullptr) {
            return;
        }
        displayReverseRecursive(node->next);
        printStudentRow(node->data);
    }

    static string describeStudent(const Student& student) {
        return student.lastName + ", " + student.firstName +
               " (" + std::to_string(student.id) + ")";
    }
};

/* ============================================================================
   SECTION 4. STACK (LIFO): RECENTLY DELETED RECORDS
   ----------------------------------------------------------------------------
   Every successful delete pushes the removed Student onto this stack. Popping
   restores the most recently deleted record, which is the classic undo model.
   A linked stack is used so push and pop are both O(1) and no fixed capacity is
   imposed on the undo history.
   ============================================================================ */

class DeletedStack {
public:
    DeletedStack() : topNode(nullptr), count(0) {}

    ~DeletedStack() {
        clear();
    }

    DeletedStack(const DeletedStack&) = delete;
    DeletedStack& operator=(const DeletedStack&) = delete;

    bool isEmpty() const {
        return topNode == nullptr;
    }

    int size() const {
        return count;
    }

    // O(1): link the new node in front of the current top.
    void push(const Student& student) {
        StackNode* node = new StackNode(student);
        node->next = topNode;
        topNode = node;
        ++count;
    }

    // O(1): unlink and delete the top node.
    bool pop(Student& removed) {
        if (topNode == nullptr) {
            return false;
        }
        StackNode* oldTop = topNode;
        removed = oldTop->data;
        topNode = oldTop->next;
        delete oldTop;
        --count;
        return true;
    }

    const Student* peek() const {
        return topNode == nullptr ? nullptr : &topNode->data;
    }

    void display() const {
        if (topNode == nullptr) {
            cout << "  (the recently deleted stack is empty)\n";
            return;
        }

        cout << left << setw(9) << "STACK"
             << setw(11) << "ID"
             << setw(30) << "NAME (Last, First)"
             << setw(22) << "COURSE"
             << setw(5)  << "YR"
             << setw(7)  << "GPA"
             << '\n';
        printRule('-', 84);

        int position = 1;
        for (const StackNode* node = topNode; node != nullptr; node = node->next) {
            const string label = (position == 1)
                                     ? "TOP ->"
                                     : ("#" + std::to_string(position));
            cout << left << setw(9) << label;
            printStudentRow(node->data);
            ++position;
        }
        cout << "\n  The top item (position 1) is the most recently deleted record.\n";
    }

    void clear() {
        while (topNode != nullptr) {
            StackNode* next = topNode->next;
            delete topNode;
            topNode = next;
        }
        count = 0;
    }

private:
    struct StackNode {
        Student data;
        StackNode* next;

        explicit StackNode(const Student& student)
            : data(student), next(nullptr) {}
    };

    StackNode* topNode;
    int count;
};

/* ============================================================================
   SECTION 5. QUEUE (FIFO): STUDENT SERVICE LINE
   ----------------------------------------------------------------------------
   A student who needs service is enqueued at the rear. The staff member serves
   the student at the front. Both enqueue and dequeue are O(1) because the queue
   keeps a rear pointer as well as a front pointer.
   ============================================================================ */

struct ServiceTicket {
    int studentId = 0;
    string studentName;
    string purpose;
    string queuedAt;
};

class ServiceQueue {
public:
    ServiceQueue() : frontNode(nullptr), rearNode(nullptr), count(0) {}

    ~ServiceQueue() {
        clear();
    }

    ServiceQueue(const ServiceQueue&) = delete;
    ServiceQueue& operator=(const ServiceQueue&) = delete;

    bool isEmpty() const {
        return frontNode == nullptr;
    }

    int size() const {
        return count;
    }

    void enqueue(const ServiceTicket& ticket) {
        QueueNode* node = new QueueNode(ticket);
        if (rearNode == nullptr) {
            frontNode = node;
            rearNode = node;
        } else {
            rearNode->next = node;
            rearNode = node;
        }
        ++count;
    }

    bool dequeue(ServiceTicket& served) {
        if (frontNode == nullptr) {
            return false;
        }
        QueueNode* oldFront = frontNode;
        served = oldFront->data;
        frontNode = oldFront->next;
        if (frontNode == nullptr) {
            rearNode = nullptr;
        }
        delete oldFront;
        --count;
        return true;
    }

    const ServiceTicket* peek() const {
        return frontNode == nullptr ? nullptr : &frontNode->data;
    }

    bool containsStudent(int studentId) const {
        for (const QueueNode* node = frontNode; node != nullptr; node = node->next) {
            if (node->data.studentId == studentId) {
                return true;
            }
        }
        return false;
    }

    void display() const {
        if (frontNode == nullptr) {
            cout << "  (the service queue is empty)\n";
            return;
        }

        cout << left << setw(7)  << "POS"
             << setw(12) << "STUDENT ID"
             << setw(30) << "NAME (Last, First)"
             << setw(25) << "PURPOSE"
             << "QUEUED AT"
             << '\n';
        printRule('-', 96);

        int position = 1;
        for (const QueueNode* node = frontNode; node != nullptr;
             node = node->next) {
            cout << left
                 << setw(7)  << position
                 << setw(12) << node->data.studentId
                 << setw(30) << fit(node->data.studentName, 29)
                 << setw(25) << fit(node->data.purpose, 24)
                 << node->data.queuedAt
                 << '\n';
            ++position;
        }

        cout << "\n  FRONT = position 1 (next to be served), REAR = position "
             << count << '\n';
    }

    void clear() {
        while (frontNode != nullptr) {
            QueueNode* next = frontNode->next;
            delete frontNode;
            frontNode = next;
        }
        rearNode = nullptr;
        count = 0;
    }

private:
    struct QueueNode {
        ServiceTicket data;
        QueueNode* next;

        explicit QueueNode(const ServiceTicket& ticket)
            : data(ticket), next(nullptr) {}
    };

    QueueNode* frontNode;
    QueueNode* rearNode;
    int count;
};

/* ============================================================================
   SECTION 6. SYSTEM CONTROLLER AND MENUS
   ============================================================================ */

class StudentManagementSystem {
public:
    StudentManagementSystem() = default;

    void run() {
        loadData();

        while (true) {
            showBanner();
            showMainMenu();

            const int choice = readInt("Enter choice: ", 1, 10);
            switch (choice) {
                case 1:  handleAdd();        break;
                case 2:  handleDisplay();    break;
                case 3:  handleSearch();     break;
                case 4:  handleUpdate();     break;
                case 5:  handleDelete();     break;
                case 6:  handleSort();       break;
                case 7:  handleStack();      break;
                case 8:  handleQueue();      break;
                case 9:  handleStatistics(); break;
                case 10:
                    if (confirm("Save data and exit", true)) {
                        cout << '\n';
                        if (students.saveToFile(DATA_FILE)) {
                            cout << "Data saved to " << DATA_FILE << ".\n";
                        } else {
                            cout << "Warning: data could not be saved.\n";
                        }
                        cout << "Goodbye.\n";
                        return;
                    }
                    continue; // return to the menu without the pause prompt
            }
            pauseScreen();
        }
    }

private:
    StudentList students;
    DeletedStack deletedStack;
    ServiceQueue serviceQueue;

    void loadData() {
        cout << "Loading records from " << DATA_FILE << "...\n";
        int loaded = 0;
        int skipped = 0;

        if (students.loadFromFile(DATA_FILE, loaded, skipped)) {
            cout << "Loaded " << loaded << " record(s).";
            if (skipped > 0) {
                cout << " Skipped " << skipped << " invalid line(s).";
            }
            cout << '\n';
        } else {
            cout << "No existing data file found. Starting with an empty list.\n";
            cout << "A new " << DATA_FILE << " will be created after the first change.\n";
        }
    }

    bool saveData(bool quiet = false) {
        const bool saved = students.saveToFile(DATA_FILE);
        if (!saved) {
            cout << "\n  [Warning] Could not write " << DATA_FILE
                 << ". Check that the folder is writable.\n";
        } else if (!quiet) {
            cout << "  Data saved to " << DATA_FILE << ".\n";
        }
        return saved;
    }

    void showBanner() const {
        clearScreen();
        cout << '\n';
        printRule('=');
        cout << centered("PLMUN STUDENT RECORD MANAGEMENT SYSTEM") << '\n';
        cout << centered("Singly Linked List | Stack | Queue | Searching | Sorting") << '\n';
        printRule('=');
        cout << "  Records in memory : " << students.size() << '\n';
        cout << "  Recently deleted  : " << deletedStack.size() << '\n';
        cout << "  Students in line  : " << serviceQueue.size() << '\n';
        cout << "  Data file         : " << DATA_FILE << "  (auto-save)\n";
        printRule('-');
    }

    void showMainMenu() const {
        cout << "\nMAIN MENU\n"
             << "   1. Add Student\n"
             << "   2. Display All Students\n"
             << "   3. Search Student\n"
             << "   4. Update Student\n"
             << "   5. Delete Student\n"
             << "   6. Sort Students\n"
             << "   7. Recently Deleted Stack\n"
             << "   8. Student Service Queue\n"
             << "   9. Student Statistics\n"
             << "  10. Exit\n";
        printRule('-');
    }

    /* ------------------------------ CREATE -------------------------------- */

    void handleAdd() {
        clearScreen();
        cout << "\nADD STUDENT\n";
        printRule('-');

        const int id = readInt("Student ID (6 to 9 digits): ", MIN_ID, MAX_ID);
        if (students.containsId(id)) {
            cout << "\nA student with ID " << id << " already exists.\n";
            cout << "Use Update Student (menu 4) to change that record.\n";
            return;
        }

        const string firstName = readName("First name: ");
        const string lastName = readName("Last name: ");
        const string course = readCourse("Course (example: BSIT): ");
        const int yearLevel = readInt("Year level (1 to 5): ", 1, 5);
        const double gpa = readGpa("GPA (1.00 = best, 5.00 = failing): ");

        const Student student(id, firstName, lastName, course, yearLevel, gpa);
        if (!students.add(student)) {
            cout << "\nThe record could not be added (duplicate ID).\n";
            return;
        }

        const bool saved = saveData(true);
        cout << "\nRecord added successfully.\n";
        cout << (saved ? "Saved to " : "Warning: could not save to ")
             << DATA_FILE << ".\n\n";
        printStudentDetails(student);
    }

    /* ------------------------------- READ --------------------------------- */

    void handleDisplay() {
        clearScreen();
        cout << "\nALL STUDENT RECORDS\n";
        printRule('-');

        if (students.isEmpty()) {
            cout << "  No students are stored yet. Use menu 1 to add a record.\n";
            return;
        }

        students.displayAll();
        const int recursiveCount = students.countRecursive();
        cout << "\n  Records shown        : " << students.size() << '\n';
        cout << "  Recursive count      : " << recursiveCount << '\n';

        if (students.size() >= 2) {
            cout << '\n';
            if (confirm("Display the records again in reverse order using recursion?",
                        false)) {
                cout << "\nREVERSE ORDER (recursive traversal)\n";
                printRule('-');
                students.displayReverse();
            }
        }
    }

    void handleSearch() {
        bool searching = true;
        while (searching) {
            clearScreen();
            cout << "\nSEARCH STUDENT\n";
            printRule('-');
            cout << "   1. Linear search by student ID\n"
                 << "   2. Binary search by student ID\n"
                 << "   3. Linear search by last name\n"
                 << "   0. Back to main menu\n";
            printRule('-');

            const int choice = readInt("Enter choice: ", 0, 3);
            switch (choice) {
                case 0:
                    searching = false;
                    break;
                case 1:
                    searchById(false);
                    break;
                case 2:
                    searchById(true);
                    break;
                case 3:
                    searchByLastName();
                    break;
            }
            if (searching) {
                pauseScreen();
            }
        }
    }

    void searchById(bool useBinarySearch) {
        if (students.isEmpty()) {
            cout << "\n  No records are available to search.\n";
            return;
        }

        const int id = readInt("Enter student ID: ", MIN_ID, MAX_ID);
        const SearchResult result = useBinarySearch
                                        ? students.binarySearchById(id)
                                        : students.linearSearchById(id);

        cout << '\n';
        if (result.found()) {
            cout << "RECORD FOUND\n";
            printRule('-');
            printStudentDetails(*result.student);
        } else {
            cout << "No student with ID " << id << " was found.\n";
        }

        cout << '\n' << (useBinarySearch ? "Binary" : "Linear")
             << " search comparisons: " << result.comparisons << '\n';

        if (useBinarySearch) {
            if (result.arrayPrepared) {
                cout << "The list was not sorted by ID, so a pointer array was\n"
                     << "prepared with selection sort first (O(n^2) preparation).\n";
            } else {
                cout << "The list was already sorted by ID, so binary search\n"
                     << "used the array directly (no preparation needed).\n";
            }
            cout << "Binary search itself is O(log n) once the data is sorted.\n";
        } else {
            cout << "Linear search is O(n): in the worst case it inspects every\n"
                 << "record because a linked list has no direct index.\n";
        }
    }

    void searchByLastName() {
        if (students.isEmpty()) {
            cout << "\n  No records are available to search.\n";
            return;
        }

        const string query = readLine("Enter last name or part of it: ");
        if (query.empty()) {
            cout << "\n  Search text cannot be empty.\n";
            return;
        }

        const int capacity = students.size();
        const Student** matches = new const Student*[static_cast<size_t>(capacity)];
        long long comparisons = 0;
        const int found = students.findByLastName(query, matches, capacity, comparisons);

        cout << "\nLinear name search examined " << comparisons << " record(s).\n";
        if (found == 0) {
            cout << "No last name contains \"" << query << "\".\n";
        } else {
            cout << found << " matching record(s):\n\n";
            printStudentTableHeader();
            for (int i = 0; i < found && i < capacity; ++i) {
                printStudentRow(*matches[i]);
            }
        }

        delete[] matches;
    }

    /* ------------------------------ UPDATE -------------------------------- */

    void handleUpdate() {
        clearScreen();
        cout << "\nUPDATE STUDENT\n";
        printRule('-');

        if (students.isEmpty()) {
            cout << "  No records are available to update.\n";
            return;
        }

        const int id = readInt("Student ID to update: ", MIN_ID, MAX_ID);
        const Student* current = students.findById(id);
        if (current == nullptr) {
            cout << "\nNo student with ID " << id << " was found.\n";
            return;
        }

        cout << "\nCurrent record:\n";
        printStudentDetails(*current);
        cout << "\nPress Enter on any field to keep its current value.\n";

        Student updated = *current;
        updated.firstName = readNameWithDefault("First name", current->firstName);
        updated.lastName = readNameWithDefault("Last name", current->lastName);
        updated.course = readCourseWithDefault("Course", current->course);
        updated.yearLevel = readIntWithDefault("Year level", current->yearLevel, 1, 5);
        updated.gpa = readGpaWithDefault("GPA", current->gpa);

        cout << "\nProposed record:\n";
        printStudentDetails(updated);

        if (!confirm("Save this update?", true)) {
            cout << "\nUpdate cancelled.\n";
            return;
        }

        Student oldRecord;
        if (students.update(id, updated, oldRecord)) {
            saveData(true);
            cout << "\nRecord updated and saved to " << DATA_FILE << ".\n";
        } else {
            cout << "\nUpdate failed: the record no longer exists.\n";
        }
    }

    /* ------------------------------ DELETE -------------------------------- */

    void handleDelete() {
        clearScreen();
        cout << "\nDELETE STUDENT\n";
        printRule('-');

        if (students.isEmpty()) {
            cout << "  No records are available to delete.\n";
            return;
        }

        const int id = readInt("Student ID to delete: ", MIN_ID, MAX_ID);
        const Student* record = students.findById(id);
        if (record == nullptr) {
            cout << "\nNo student with ID " << id << " was found.\n";
            return;
        }

        cout << "\nRecord to delete:\n";
        printStudentDetails(*record);

        if (!confirm("Delete this record? It will be pushed onto the deleted stack",
                     false)) {
            cout << "\nDelete cancelled.\n";
            return;
        }

        Student removed;
        if (students.remove(id, removed)) {
            deletedStack.push(removed);
            saveData(true);
            cout << "\nRecord deleted.\n";
            cout << "It was pushed onto the Recently Deleted Stack (menu 7).\n";
            cout << "Current stack size: " << deletedStack.size() << '\n';
        } else {
            cout << "\nDelete failed: the record no longer exists.\n";
        }
    }

    /* ------------------------------- SORT --------------------------------- */

    void handleSort() {
        clearScreen();
        cout << "\nSORT STUDENT RECORDS\n";
        printRule('-');

        if (students.size() < 2) {
            cout << "  At least two records are needed to demonstrate sorting.\n";
            return;
        }

        cout << "Sort by:\n"
             << "   1. Student ID\n"
             << "   2. Last name\n"
             << "   3. GPA (1.00 best first)\n";
        const int keyChoice = readInt("Enter choice: ", 1, 3);
        const SortKey key = (keyChoice == 1) ? SortKey::ID
                           : (keyChoice == 2) ? SortKey::LAST_NAME
                                              : SortKey::GPA;

        cout << "\nAlgorithm:\n"
             << "   1. Bubble sort\n"
             << "   2. Selection sort\n";
        const int algorithm = readInt("Enter choice: ", 1, 2);

        const auto start = std::chrono::high_resolution_clock::now();
        const SortStats stats = (algorithm == 1)
                                    ? students.bubbleSort(key)
                                    : students.selectionSort(key);
        const auto end = std::chrono::high_resolution_clock::now();
        const double elapsedMs =
            std::chrono::duration<double, std::milli>(end - start).count();

        std::ostringstream elapsedText;
        elapsedText << fixed << setprecision(3) << elapsedMs;

        cout << "\nSorted by " << sortKeyName(key) << " using "
             << (algorithm == 1 ? "bubble sort" : "selection sort") << ".\n";
        cout << "Comparisons: " << stats.comparisons
             << "   Swaps: " << stats.swaps << '\n';
        cout << "Elapsed time: " << elapsedText.str() << " ms\n";

        if (algorithm == 1) {
            cout << "Bubble sort: O(n^2) average/worst, O(n) best when already\n"
                 << "sorted, and stable because only out-of-order neighbours swap.\n";
        } else {
            cout << "Selection sort: O(n^2) comparisons in every case, at most\n"
                 << "n-1 swaps, and not stable because of long-distance swaps.\n";
        }

        if (key == SortKey::ID) {
            cout << "The list is now sorted by ID, so binary search can run in\n"
                 << "O(log n) without preparing the pointer array.\n";
        } else {
            cout << "The list is not sorted by ID now. A later binary search will\n"
                 << "prepare a sorted pointer array first.\n";
        }

        saveData(true);
        cout << "\nSORTED RECORDS\n";
        printRule('-');
        students.displayAll();
    }

    /* ------------------------------- STACK -------------------------------- */

    void handleStack() {
        bool active = true;
        while (active) {
            clearScreen();
            cout << "\nRECENTLY DELETED STACK (LIFO)\n";
            printRule('-');
            cout << "Stack size: " << deletedStack.size() << '\n';

            const Student* top = deletedStack.peek();
            if (top != nullptr) {
                cout << "Top record: " << top->lastName << ", " << top->firstName
                     << " (" << top->id << ")\n";
            } else {
                cout << "Top record: (none)\n";
            }
            cout << "Push is automatic when a record is deleted (menu 5).\n";
            printRule('-');
            cout << "   1. Pop and restore the most recent record\n"
                 << "   2. Peek at the top record\n"
                 << "   3. Display the whole stack\n"
                 << "   0. Back to main menu\n";

            const int choice = readInt("Enter choice: ", 0, 3);
            switch (choice) {
                case 0:
                    active = false;
                    break;
                case 1:
                    restoreFromStack();
                    break;
                case 2:
                    if (top == nullptr) {
                        cout << "\nThe stack is empty.\n";
                    } else {
                        cout << "\nTOP OF STACK (most recently deleted)\n";
                        printRule('-');
                        printStudentDetails(*top);
                    }
                    break;
                case 3:
                    cout << '\n';
                    deletedStack.display();
                    break;
            }
            if (active) {
                pauseScreen();
            }
        }
    }

    void restoreFromStack() {
        Student restored;
        if (!deletedStack.pop(restored)) {
            cout << "\nThe stack is empty. There is nothing to restore.\n";
            return;
        }

        if (students.containsId(restored.id)) {
            deletedStack.push(restored);
            cout << "\nCannot restore ID " << restored.id
                 << " because that ID already exists in the list.\n";
            cout << "The record was pushed back onto the stack.\n";
            return;
        }

        students.add(restored);
        saveData(true);
        cout << "\nRestored from the stack:\n";
        printRule('-');
        printStudentDetails(restored);
        cout << "\nRecord count is now " << students.size() << ".\n";
    }

    /* ------------------------------- QUEUE -------------------------------- */

    void handleQueue() {
        bool active = true;
        while (active) {
            clearScreen();
            cout << "\nSTUDENT SERVICE QUEUE (FIFO)\n";
            printRule('-');
            cout << "Students waiting: " << serviceQueue.size() << '\n';

            const ServiceTicket* next = serviceQueue.peek();
            if (next != nullptr) {
                cout << "Next to serve  : " << next->studentName
                     << " (" << next->studentId << ") - " << next->purpose << '\n';
            } else {
                cout << "Next to serve  : (none)\n";
            }
            printRule('-');
            cout << "   1. Enqueue a student\n"
                 << "   2. Dequeue (serve the next student)\n"
                 << "   3. Peek at the next student\n"
                 << "   4. Display the queue\n"
                 << "   0. Back to main menu\n";

            const int choice = readInt("Enter choice: ", 0, 4);
            switch (choice) {
                case 0:
                    active = false;
                    break;
                case 1:
                    enqueueStudent();
                    break;
                case 2:
                    dequeueStudent();
                    break;
                case 3:
                    peekQueue();
                    break;
                case 4:
                    cout << '\n';
                    serviceQueue.display();
                    break;
            }
            if (active) {
                pauseScreen();
            }
        }
    }

    void enqueueStudent() {
        const int id = readInt("Student ID to enqueue: ", MIN_ID, MAX_ID);
        const Student* student = students.findById(id);
        if (student == nullptr) {
            cout << "\nNo student with ID " << id << " exists.\n";
            cout << "Add the student record first (menu 1).\n";
            return;
        }

        if (serviceQueue.containsStudent(id)) {
            cout << "\nThat student is already waiting in the queue.\n";
            return;
        }

        const string purpose = askPurpose();

        ServiceTicket ticket;
        ticket.studentId = student->id;
        ticket.studentName = student->lastName + ", " + student->firstName;
        ticket.purpose = purpose;
        ticket.queuedAt = currentTimestamp();
        serviceQueue.enqueue(ticket);

        cout << "\nEnqueued " << ticket.studentName
             << " at position " << serviceQueue.size() << ".\n";
        cout << "The service queue is session-only; it is not written to "
             << DATA_FILE << ".\n";
    }

    string askPurpose() {
        cout << "\nPurpose of visit:\n"
             << "   1. Enrollment\n"
             << "   2. Records / Transcript\n"
             << "   3. Payment\n"
             << "   4. Consultation\n"
             << "   5. Other\n";
        const int choice = readInt("Enter choice: ", 1, 5);

        switch (choice) {
            case 1: return "Enrollment";
            case 2: return "Records / Transcript";
            case 3: return "Payment";
            case 4: return "Consultation";
            default: break;
        }

        const string other = readLine("Describe the purpose: ");
        return other.empty() ? "Other" : other;
    }

    void dequeueStudent() {
        ServiceTicket served;
        if (!serviceQueue.dequeue(served)) {
            cout << "\nThe queue is empty. There is no student to serve.\n";
            return;
        }

        cout << "\nNOW SERVING (front of queue)\n";
        printRule('-');
        cout << "  Student  : " << served.studentName
             << " (" << served.studentId << ")\n";
        cout << "  Purpose  : " << served.purpose << '\n';
        cout << "  Queued   : " << served.queuedAt << '\n';
        cout << "  Served   : " << currentTimestamp() << '\n';
        cout << "  Remaining: " << serviceQueue.size() << " student(s)\n";
    }

    void peekQueue() {
        const ServiceTicket* ticket = serviceQueue.peek();
        if (ticket == nullptr) {
            cout << "\nThe queue is empty.\n";
            return;
        }
        cout << "\nNEXT STUDENT (front of queue)\n";
        printRule('-');
        cout << "  Student : " << ticket->studentName
             << " (" << ticket->studentId << ")\n";
        cout << "  Purpose : " << ticket->purpose << '\n';
        cout << "  Queued  : " << ticket->queuedAt << '\n';
        cout << "  Position: 1 of " << serviceQueue.size() << '\n';
    }

    /* ----------------------------- STATISTICS ----------------------------- */

    void handleStatistics() {
        clearScreen();
        cout << "\nSTUDENT STATISTICS\n";
        printRule('-');

        if (students.isEmpty()) {
            cout << "  No records are available. Add a student first.\n";
            return;
        }

        const StudentStats stats = students.getStatistics();

        cout << left << setw(38) << "  Total students (counter)"
             << ": " << stats.total << '\n';
        cout << left << setw(38) << "  Total students (recursive count)"
             << ": " << students.countRecursive() << '\n';
        cout << left << setw(38) << "  Average GPA"
             << ": " << formatGpa(stats.averageGpa) << '\n';
        cout << left << setw(38) << "  Highest GPA (best, lowest value)"
             << ": " << formatGpa(stats.bestGpa) << '\n';
        cout << "    " << stats.bestStudent << '\n';
        cout << left << setw(38) << "  Lowest GPA (highest value)"
             << ": " << formatGpa(stats.worstGpa) << '\n';
        cout << "    " << stats.worstStudent << '\n';

        cout << "\nGPA BANDS\n";
        printRule('-');
        cout << "  Passing (GPA <= 3.00)   : " << stats.passing
             << "  (" << formatPercent(stats.passing, stats.total) << ")\n";
        cout << "  Failing / at-risk (GPA > 3.00): " << stats.failing
             << "  (" << formatPercent(stats.failing, stats.total) << ")\n";
        cout << "  Dean's list (GPA <= 1.75): " << stats.deansList
             << "  (" << formatPercent(stats.deansList, stats.total) << ")\n";

        cout << "\nCOURSE DISTRIBUTION\n";
        printRule('-');
        students.printCourseDistribution();
    }
};

/* ============================================================================
   SECTION 7. PROGRAM ENTRY POINT
   ============================================================================ */

int main() {
    StudentManagementSystem system;
    system.run();
    return 0;
}
