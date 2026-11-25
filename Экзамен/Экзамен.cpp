#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
#include <algorithm>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

namespace fs = filesystem;

class InputClosed {};

namespace utils {

    string trim(const string& text)
    {
        const char* spaces = " \t\r\n";
        size_t begin = text.find_first_not_of(spaces);
        if (begin == string::npos)
            return "";
        size_t end = text.find_last_not_of(spaces);
        return text.substr(begin, end - begin + 1);
    }

    string cleanLogin(const string& login)
    {
        string result = trim(login);
        for (char& ch : result)
            if (ch >= 'A' && ch <= 'Z')
                ch = static_cast<char>(ch - 'A' + 'a');
        return result;
    }

    string escape(const string& text)
    {
        string result;
        for (char ch : text) {
            if (ch == '\\')      result += "\\\\";
            else if (ch == '|')  result += "\\p";
            else if (ch == '\n') result += "\\n";
            else if (ch == '\r') continue;
            else                 result += ch;
        }
        return result;
    }

    string unescape(const string& text)
    {
        string result;
        for (size_t i = 0; i < text.size(); ++i) {
            if (text[i] == '\\' && i + 1 < text.size()) {
                ++i;
                if (text[i] == 'p')      result += '|';
                else if (text[i] == 'n') result += '\n';
                else                     result += text[i];
            }
            else {
                result += text[i];
            }
        }
        return result;
    }

    string joinLine(const vector<string>& fields)
    {
        string line;
        for (size_t i = 0; i < fields.size(); ++i) {
            if (i > 0)
                line += '|';
            line += escape(fields[i]);
        }
        return line;
    }

    vector<string> splitLine(const string& line)
    {
        vector<string> fields;
        size_t start = 0;
        while (true) {
            size_t pos = line.find('|', start);
            if (pos == string::npos) {
                fields.push_back(unescape(line.substr(start)));
                break;
            }
            fields.push_back(unescape(line.substr(start, pos - start)));
            start = pos + 1;
        }
        return fields;
    }

    string nowString()
    {
        time_t now = time(nullptr);
        char buffer[32];
        tm localTime;
#ifdef _WIN32
        localtime_s(&localTime, &now);
#else
        localtime_r(&now, &localTime);
#endif
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &localTime);
        return buffer;
    }

    string numToText(double value, int precision = 1)
    {
        ostringstream stream;
        stream << fixed << setprecision(precision) << value;
        return stream.str();
    }

    int toInt(const string& text)
    {
        int value = 0;
        for (char ch : text)
            if (ch >= '0' && ch <= '9')
                value = value * 10 + (ch - '0');
        return value;
    }

    vector<int> readNumbers(const string& text)
    {
        vector<int> numbers;
        string current;
        for (char ch : text + " ") {
            if (ch >= '0' && ch <= '9') {
                current += ch;
            }
            else if (!current.empty()) {
                numbers.push_back(toInt(current));
                current.clear();
            }
        }
        return numbers;
    }

}

namespace console {

    string readLine(const string& prompt)
    {
        cout << prompt;
        string line;
        if (!getline(cin, line))
            throw InputClosed();
        return utils::trim(line);
    }

    int readInt(const string& prompt, int minValue, int maxValue)
    {
        while (true) {
            string line = readLine(prompt);
            try {
                size_t used = 0;
                int value = stoi(line, &used);
                if (used == line.size() && value >= minValue && value <= maxValue)
                    return value;
            }
            catch (const exception&) {
            }
            cout << "Введіть число від " << minValue << " до " << maxValue << ".\n";
        }
    }

    bool askYesNo(const string& question)
    {
        while (true) {
            string answer = readLine(question + " (y/n): ");
            if (answer == "y" || answer == "Y" || answer == "т" || answer == "Т")
                return true;
            if (answer == "n" || answer == "N" || answer == "н" || answer == "Н")
                return false;
        }
    }

    int chooseItem(const string& title, const vector<string>& items)
    {
        cout << "\n" << title << "\n";
        if (items.empty()) {
            cout << "  (порожньо)\n";
            return -1;
        }
        for (size_t i = 0; i < items.size(); ++i)
            cout << "  " << (i + 1) << ". " << items[i] << "\n";
        cout << "  0. Назад\n";
        return readInt("Ваш вибір: ", 0, static_cast<int>(items.size())) - 1;
    }

}

using Word = unsigned int;

class Sha256 {
public:

    static string hash(const string& data)
    {
        static const Word k[64] = {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
            0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
            0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
            0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
            0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2 };
        Word h[8] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                              0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };

        string message = data;
        unsigned long long bitLength = static_cast<unsigned long long>(data.size()) * 8;
        message += static_cast<char>(0x80);
        while (message.size() % 64 != 56)
            message += '\0';
        for (int shift = 56; shift >= 0; shift -= 8)
            message += static_cast<char>((bitLength >> shift) & 0xff);

        for (size_t block = 0; block < message.size(); block += 64) {
            Word w[64];
            for (int i = 0; i < 16; ++i) {
                w[i] = 0;
                for (int j = 0; j < 4; ++j)
                    w[i] = (w[i] << 8) | static_cast<unsigned char>(message[block + i * 4 + j]);
            }
            for (int i = 16; i < 64; ++i) {
                Word s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
                Word s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
                w[i] = w[i - 16] + s0 + w[i - 7] + s1;
            }
            Word a = h[0], b = h[1], c = h[2], d = h[3];
            Word e = h[4], f = h[5], g = h[6], hh = h[7];
            for (int i = 0; i < 64; ++i) {
                Word s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
                Word ch = (e & f) ^ (~e & g);
                Word t1 = hh + s1 + ch + k[i] + w[i];
                Word s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
                Word maj = (a & b) ^ (a & c) ^ (b & c);
                Word t2 = s0 + maj;
                hh = g; g = f; f = e; e = d + t1;
                d = c; c = b; b = a; a = t1 + t2;
            }
            h[0] += a; h[1] += b; h[2] += c; h[3] += d;
            h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
        }

        ostringstream result;
        for (Word value : h)
            result << hex << setw(8) << setfill('0') << value;
        return result.str();
    }

private:
    static Word rotr(Word value, int bits)
    {
        return (value >> bits) | (value << (32 - bits));
    }
};

class Hasher {
public:
    static string hashLogin(const string& login)
    {
        return Sha256::hash("login#salt42#" + utils::cleanLogin(login));
    }
    static string hashPassword(const string& password)
    {
        return Sha256::hash("pass#salt77#" + password);
    }
};

struct Answer {
    string text;
    bool correct = false;
};

struct Question {
    string text;
    vector<Answer> answers;
};

struct Test {
    string name;
    vector<Question> questions;
};

struct Category {
    string name;
    vector<Test> tests;
};

struct User {
    string loginHash;
    string passwordHash;
    string fullName;
    string address;
    string phone;
};

struct Result {
    string loginHash;
    string userName;
    string categoryName;
    string testName;
    int correct = 0;
    int total = 0;
    int grade = 0;
    string date;

    double percent() const
    {
        return total == 0 ? 0.0 : 100.0 * correct / total;
    }
};

struct Session {
    string loginHash;
    string categoryName;
    string testName;
    vector<int> order;
    int position = 0;
    int correct = 0;
};

int getGrade(int correct, int total)
{
    if (total <= 0)
        return 0;
    int grade = (24 * correct + total) / (2 * total);
    return max(1, min(12, grade));
}

namespace testFile {

    void write(ostream& out, const vector<Category>& categories)
    {
        for (const auto& category : categories) {
            out << utils::joinLine({ "C", category.name }) << "\n";
            for (const auto& test : category.tests) {
                out << utils::joinLine({ "T", test.name }) << "\n";
                for (const auto& question : test.questions) {
                    out << utils::joinLine({ "Q", question.text }) << "\n";
                    for (const auto& answer : question.answers)
                        out << utils::joinLine({ "A", answer.text, answer.correct ? "1" : "0" }) << "\n";
                }
            }
        }
    }

    bool read(istream& in, vector<Category>& categories, string& error)
    {
        string line;
        int lineNumber = 0;
        while (getline(in, line)) {
            ++lineNumber;
            if (utils::trim(line).empty())
                continue;
            vector<string> fields = utils::splitLine(line);
            const string& tag = fields[0];
            if (tag == "C" && fields.size() >= 2) {
                categories.push_back({ fields[1],{} });
            }
            else if (tag == "T" && fields.size() >= 2 && !categories.empty()) {
                categories.back().tests.push_back({ fields[1],{} });
            }
            else if (tag == "Q" && fields.size() >= 2 && !categories.empty() &&
                !categories.back().tests.empty()) {
                categories.back().tests.back().questions.push_back({ fields[1],{} });
            }
            else if (tag == "A" && fields.size() >= 3 && !categories.empty() &&
                !categories.back().tests.empty() &&
                !categories.back().tests.back().questions.empty()) {
                categories.back().tests.back().questions.back().answers.push_back(
                    { fields[1], fields[2] == "1" });
            }
            else {
                error = "Невірний рядок №" + to_string(lineNumber);
                return false;
            }
        }
        return true;
    }

}

class Database {
public:
    vector<User> users;
    vector<Category> categories;
    vector<Result> results;
    vector<Session> sessions;
    string adminLoginHash;
    string adminPasswordHash;

    explicit Database(fs::path directory) : m_directory(move(directory))
    {
        fs::create_directories(m_directory);
        load();
        if (categories.empty()) {
            seedDemoTests();
            save();
        }
    }

    bool hasAdmin() const { return !adminLoginHash.empty(); }

    User* findUser(const string& loginHash)
    {
        for (auto& user : users)
            if (user.loginHash == loginHash)
                return &user;
        return nullptr;
    }

    bool loginExists(const string& loginHash) const
    {
        if (loginHash == adminLoginHash)
            return true;
        return any_of(users.begin(), users.end(),
            [&](const User& user) { return user.loginHash == loginHash; });
    }

    Category* findCategory(const string& name)
    {
        for (auto& category : categories)
            if (category.name == name)
                return &category;
        return nullptr;
    }

    Session* findSession(const string& loginHash, const string& categoryName,
        const string& testName)
    {
        for (auto& session : sessions)
            if (session.loginHash == loginHash && session.categoryName == categoryName &&
                session.testName == testName)
                return &session;
        return nullptr;
    }

    void removeSession(const Session* target)
    {
        sessions.erase(remove_if(sessions.begin(), sessions.end(),
            [&](const Session& s) { return &s == target; }),
            sessions.end());
    }

    void removeUser(const string& loginHash)
    {
        users.erase(remove_if(users.begin(), users.end(),
            [&](const User& u) { return u.loginHash == loginHash; }),
            users.end());
        results.erase(remove_if(results.begin(), results.end(),
            [&](const Result& r) { return r.loginHash == loginHash; }),
            results.end());
        sessions.erase(remove_if(sessions.begin(), sessions.end(),
            [&](const Session& s) { return s.loginHash == loginHash; }),
            sessions.end());
    }

    void changeLogin(User& user, const string& newLoginHash)
    {
        for (auto& result : results)
            if (result.loginHash == user.loginHash)
                result.loginHash = newLoginHash;
        for (auto& session : sessions)
            if (session.loginHash == user.loginHash)
                session.loginHash = newLoginHash;
        user.loginHash = newLoginHash;
    }

    void save() const
    {
        saveAdmin();
        saveUsers();
        saveTests();
        saveResults();
        saveSessions();
    }

private:
    fs::path m_directory;

    fs::path filePath(const string& name) const { return m_directory / name; }

    void load()
    {
        loadAdmin();
        loadUsers();
        loadTests();
        loadResults();
        loadSessions();
    }

    void loadAdmin()
    {
        ifstream in(filePath("admin.txt"));
        string line;
        if (getline(in, line)) {
            auto fields = utils::splitLine(line);
            if (fields.size() >= 2) {
                adminLoginHash = fields[0];
                adminPasswordHash = fields[1];
            }
        }
    }

    void loadUsers()
    {
        ifstream in(filePath("users.txt"));
        string line;
        while (getline(in, line)) {
            auto f = utils::splitLine(line);
            if (f.size() >= 5)
                users.push_back({ f[0], f[1], f[2], f[3], f[4] });
        }
    }

    void loadTests()
    {
        ifstream in(filePath("tests.txt"));
        string error;
        testFile::read(in, categories, error);
    }

    void loadResults()
    {
        ifstream in(filePath("results.txt"));
        string line;
        while (getline(in, line)) {
            auto f = utils::splitLine(line);
            if (f.size() < 8)
                continue;
            Result r;
            r.loginHash = f[0];
            r.userName = f[1];
            r.categoryName = f[2];
            r.testName = f[3];
            r.correct = utils::toInt(f[4]);
            r.total = utils::toInt(f[5]);
            r.grade = utils::toInt(f[6]);
            r.date = f[7];
            results.push_back(r);
        }
    }

    void loadSessions()
    {
        ifstream in(filePath("sessions.txt"));
        string line;
        while (getline(in, line)) {
            auto f = utils::splitLine(line);
            if (f.size() < 6)
                continue;
            Session s;
            s.loginHash = f[0];
            s.categoryName = f[1];
            s.testName = f[2];
            s.position = utils::toInt(f[3]);
            s.correct = utils::toInt(f[4]);
            s.order = utils::readNumbers(f[5]);
            sessions.push_back(s);
        }
    }

    void saveAdmin() const
    {
        ofstream out(filePath("admin.txt"));
        if (hasAdmin())
            out << utils::joinLine({ adminLoginHash, adminPasswordHash }) << "\n";
    }

    void saveUsers() const
    {
        ofstream out(filePath("users.txt"));
        for (const auto& u : users)
            out << utils::joinLine({ u.loginHash, u.passwordHash, u.fullName, u.address, u.phone })
            << "\n";
    }

    void saveTests() const
    {
        ofstream out(filePath("tests.txt"));
        testFile::write(out, categories);
    }

    void saveResults() const
    {
        ofstream out(filePath("results.txt"));
        for (const auto& r : results)
            out << utils::joinLine({ r.loginHash, r.userName, r.categoryName, r.testName,
                                          to_string(r.correct), to_string(r.total),
                                          to_string(r.grade), r.date })
            << "\n";
    }

    void saveSessions() const
    {
        ofstream out(filePath("sessions.txt"));
        for (const auto& s : sessions) {
            string order;
            for (int index : s.order)
                order += to_string(index) + " ";
            out << utils::joinLine({ s.loginHash, s.categoryName, s.testName,
                                          to_string(s.position), to_string(s.correct),
                                          order })
                << "\n";
        }
    }

    static Question makeQuestion(const string& text, vector<Answer> answers)
    {
        return { text, move(answers) };
    }

    void seedDemoTests()
    {
        Category math{ "Математика",{} };
        math.tests.push_back({ "Дискретна математика",
            { makeQuestion("Скільки підмножин має множина з 3 елементів?",
                          { { "6", false },{ "8", true },{ "9", false } }),
             makeQuestion("Скільки ребер має повний граф на 4 вершинах?",
                          { { "6", true },{ "4", false },{ "8", false } }),
             makeQuestion("Які з чисел прості?",
                          { { "2", true },{ "9", false },{ "13", true },{ "15", false } }) } });
        math.tests.push_back({ "Математичний аналіз",
            { makeQuestion("Чому дорівнює похідна sin(x)?",
                          { { "cos(x)", true },{ "-cos(x)", false },{ "tan(x)", false } }),
             makeQuestion("Чому дорівнює інтеграл від 2x dx?",
                          { { "x^2 + C", true },{ "2 + C", false },{ "x + C", false } }),
             makeQuestion("Границя sin(x)/x при x -> 0 дорівнює:",
                          { { "0", false },{ "1", true },{ "нескінченності", false } }) } });
        Category physics{ "Фізика",{} };
        physics.tests.push_back({ "Квантова фізика",
            { makeQuestion("Хто запропонував гіпотезу квантів енергії?",
                          { { "Планк", true },{ "Ньютон", false },{ "Галілей", false } }),
             makeQuestion("Принцип невизначеності сформулював:",
                          { { "Гейзенберг", true },{ "Ейнштейн", false },{ "Бор", false } }) } });
        physics.tests.push_back({ "Механіка",
            { makeQuestion("Одиниця виміру сили в СІ:",
                          { { "Ньютон", true },{ "Джоуль", false },{ "Паскаль", false } }),
             makeQuestion("Другий закон Ньютона:",
                          { { "F = ma", true },{ "E = mc^2", false },{ "p = mv^2", false } }),
             makeQuestion("Які величини є векторними?",
                          { { "Швидкість", true },{ "Маса", false },{ "Сила", true },{ "Час", false } }) } });
        categories.push_back(math);
        categories.push_back(physics);
    }
};

class Report {
public:

    static vector<string> build(const string& title,
        const vector<Result>& results)
    {
        vector<string> lines;
        lines.push_back("=== " + title + " ===");
        if (results.empty()) {
            lines.push_back("Даних немає.");
            return lines;
        }
        double percentSum = 0, gradeSum = 0;
        for (const auto& r : results) {
            lines.push_back(r.date + " | " + r.userName + " | " + r.categoryName + " / " +
                r.testName + " | " + to_string(r.correct) + "/" +
                to_string(r.total) + " | " +
                utils::numToText(r.percent()) + "% | оцінка " +
                to_string(r.grade));
            percentSum += r.percent();
            gradeSum += r.grade;
        }
        double count = static_cast<double>(results.size());
        lines.push_back("--- Спроб: " + to_string(results.size()) +
            ", середній відсоток: " + utils::numToText(percentSum / count) +
            "%, середня оцінка: " + utils::numToText(gradeSum / count) + " ---");
        return lines;
    }
};

class GuestMenu {
public:
    explicit GuestMenu(Database& database) : m_database(database) {}

    void run()
    {
        while (true) {
            cout << "\n--- Режим тестованого ---\n"
                "1. Реєстрація\n2. Вхід\n0. Назад\n";
            int choice = console::readInt("Ваш вибір: ", 0, 2);
            if (choice == 0)
                return;
            if (choice == 1)
                registerUser();
            else {
                string loginHash;
                if (login(loginHash))
                    userMenu(loginHash);
            }
        }
    }

    bool registerUser()
    {
        cout << "\n--- Реєстрація ---\n";
        User user;
        user.fullName = readNonEmpty("ПІБ: ");
        user.address = readNonEmpty("Домашня адреса: ");
        user.phone = readNonEmpty("Телефон: ");
        string login;
        while (true) {
            login = readNonEmpty("Логін: ");
            if (!m_database.loginExists(Hasher::hashLogin(login)))
                break;
            cout << "Такий логін вже існує, оберіть інший.\n";
        }
        user.loginHash = Hasher::hashLogin(login);
        user.passwordHash = Hasher::hashPassword(readPassword());
        m_database.users.push_back(user);
        m_database.save();
        cout << "Користувача зареєстровано.\n";
        return true;
    }

private:
    Database& m_database;

    static string readNonEmpty(const string& prompt)
    {
        while (true) {
            string value = console::readLine(prompt);
            if (!value.empty())
                return value;
            cout << "Поле не може бути порожнім.\n";
        }
    }

    static string readPassword()
    {
        while (true) {
            string password = console::readLine("Пароль (мінімум 4 символи): ");
            if (password.size() >= 4)
                return password;
            cout << "Пароль закороткий.\n";
        }
    }

    bool login(string& loginHash)
    {
        loginHash = Hasher::hashLogin(console::readLine("Логін: "));
        string passwordHash = Hasher::hashPassword(console::readLine("Пароль: "));
        User* user = m_database.findUser(loginHash);
        if (user == nullptr || user->passwordHash != passwordHash) {
            cout << "Невірний логін або пароль.\n";
            return false;
        }
        cout << "Вітаємо, " << user->fullName << "!\n";
        return true;
    }

    void userMenu(const string& loginHash)
    {
        while (true) {
            cout << "\n--- Меню тестованого ---\n"
                "1. Мої результати\n2. Пройти тест\n3. Продовжити збережений тест\n0. Вийти\n";
            int choice = console::readInt("Ваш вибір: ", 0, 3);
            if (choice == 0)
                return;
            if (choice == 1)
                showResults(loginHash);
            else if (choice == 2)
                startTest(loginHash);
            else
                resumeTest(loginHash);
        }
    }

    void showResults(const string& loginHash)
    {
        vector<Result> own;
        for (const auto& r : m_database.results)
            if (r.loginHash == loginHash)
                own.push_back(r);
        for (const auto& line : Report::build("Мої результати", own))
            cout << line << "\n";
    }

    void startTest(const string& loginHash)
    {
        vector<string> names;
        for (const auto& c : m_database.categories)
            names.push_back(c.name);
        int catIndex = console::chooseItem("Оберіть розділ:", names);
        if (catIndex < 0)
            return;
        Category& category = m_database.categories[catIndex];

        vector<string> testNames;
        for (const auto& t : category.tests)
            testNames.push_back(t.name + " (питань: " + to_string(t.questions.size()) + ")");
        int testIndex = console::chooseItem("Оберіть тест:", testNames);
        if (testIndex < 0)
            return;
        const Test& test = category.tests[testIndex];
        if (test.questions.empty()) {
            cout << "У цьому тесті ще немає питань.\n";
            return;
        }

        if (Session* old = m_database.findSession(loginHash, category.name, test.name)) {
            if (console::askYesNo("Є незавершений сеанс цього тесту. Продовжити його?")) {
                runTest(loginHash, test, *old);
                return;
            }
            m_database.removeSession(old);
        }

        Session session;
        session.loginHash = loginHash;
        session.categoryName = category.name;
        session.testName = test.name;
        session.order.resize(test.questions.size());
        for (size_t i = 0; i < session.order.size(); ++i)
            session.order[i] = static_cast<int>(i);
        mt19937 generator(random_device{}());
        shuffle(session.order.begin(), session.order.end(), generator);
        runTest(loginHash, test, session);
    }

    void resumeTest(const string& loginHash)
    {
        vector<Session*> own;
        vector<string> names;
        for (auto& s : m_database.sessions) {
            if (s.loginHash != loginHash)
                continue;
            own.push_back(&s);
            names.push_back(s.categoryName + " / " + s.testName + " (питання " +
                to_string(s.position + 1) + " з " +
                to_string(s.order.size()) + ")");
        }
        int index = console::chooseItem("Збережені тести:", names);
        if (index < 0)
            return;
        Session* session = own[index];

        const Test* test = findTest(session->categoryName, session->testName);
        bool valid = test != nullptr && test->questions.size() == session->order.size();
        if (!valid) {
            cout << "Тест було змінено або видалено. Збережений сеанс скасовано.\n";
            m_database.removeSession(session);
            m_database.save();
            return;
        }
        runTest(loginHash, *test, *session);
    }

    const Test* findTest(const string& categoryName, const string& testName)
    {
        Category* category = m_database.findCategory(categoryName);
        if (category == nullptr)
            return nullptr;
        for (const auto& test : category->tests)
            if (test.name == testName)
                return &test;
        return nullptr;
    }

    void runTest(const string& loginHash, const Test& test, Session session)
    {
        cout << "\n=== " << session.categoryName << " / " << session.testName << " ===\n"
            << "Вводьте номери правильних відповідей через пробіл (p - призупинити).\n";
        const int total = static_cast<int>(session.order.size());

        while (session.position < total) {
            const Question& question = test.questions[session.order[session.position]];
            cout << "\nПитання " << (session.position + 1) << "/" << total << ": "
                << question.text << "\n";
            for (size_t i = 0; i < question.answers.size(); ++i)
                cout << "  " << (i + 1) << ") " << question.answers[i].text << "\n";

            vector<bool> chosen;
            while (true) {
                string input = console::readLine("Ваша відповідь: ");
                if (input == "p" || input == "P") {
                    saveSession(session);
                    cout << "Тест призупинено. Продовжити можна з меню.\n";
                    return;
                }
                if (readAnswers(input, question.answers.size(), chosen))
                    break;
                cout << "Введіть номери від 1 до " << question.answers.size()
                    << " через пробіл або p.\n";
            }
            if (isCorrect(question, chosen))
                ++session.correct;
            ++session.position;
        }
        finishTest(loginHash, session, total);
    }

    static bool readAnswers(const string& input, size_t answerCount,
        vector<bool>& chosen)
    {
        chosen.assign(answerCount, false);
        bool any = false;
        size_t i = 0;
        while (i < input.size()) {
            if (input[i] == ' ') {
                ++i;
                continue;
            }
            if (input[i] < '0' || input[i] > '9')
                return false;
            size_t number = 0;
            while (i < input.size() && input[i] >= '0' && input[i] <= '9') {
                number = number * 10 + static_cast<size_t>(input[i] - '0');
                if (number > answerCount)
                    return false;
                ++i;
            }
            if (number < 1)
                return false;
            chosen[number - 1] = true;
            any = true;
        }
        return any;
    }

    static bool isCorrect(const Question& question, const vector<bool>& chosen)
    {
        for (size_t i = 0; i < question.answers.size(); ++i)
            if (question.answers[i].correct != chosen[i])
                return false;
        return true;
    }

    void saveSession(const Session& session)
    {
        Session* existing =
            m_database.findSession(session.loginHash, session.categoryName, session.testName);
        if (existing != nullptr)
            *existing = session;
        else
            m_database.sessions.push_back(session);
        m_database.save();
    }

    void finishTest(const string& loginHash, const Session& session, int total)
    {
        Result result;
        result.loginHash = loginHash;
        result.userName = m_database.findUser(loginHash)->fullName;
        result.categoryName = session.categoryName;
        result.testName = session.testName;
        result.correct = session.correct;
        result.total = total;
        result.grade = getGrade(session.correct, total);
        result.date = utils::nowString();
        m_database.results.push_back(result);

        Session* old =
            m_database.findSession(loginHash, session.categoryName, session.testName);
        if (old != nullptr)
            m_database.removeSession(old);
        m_database.save();

        cout << "\n=== Результат ===\n"
            << "Правильних відповідей: " << result.correct << " з " << total << "\n"
            << "Відсоток: " << utils::numToText(result.percent()) << "%\n"
            << "Оцінка (12-бальна шкала): " << result.grade << "\n";
    }
};

class AdminMenu {
public:
    explicit AdminMenu(Database& database) : m_database(database) {}

    void run()
    {
        if (!checkAdmin())
            return;
        while (true) {
            cout << "\n--- Меню адміністратора ---\n"
                "1. Управління користувачами\n2. Статистика\n"
                "3. Управління тестуванням\n4. Змінити логін/пароль адміна\n0. Вийти\n";
            int choice = console::readInt("Ваш вибір: ", 0, 4);
            if (choice == 0)
                return;
            if (choice == 1)      manageUsers();
            else if (choice == 2) showStatistics();
            else if (choice == 3) manageTests();
            else                  changeAdminData();
        }
    }

private:
    Database& m_database;

    bool checkAdmin()
    {
        if (!m_database.hasAdmin()) {
            cout << "Перший вхід: задайте логін і пароль адміністратора.\n";
            setAdminData();
            return true;
        }
        string loginHash = Hasher::hashLogin(console::readLine("Логін адміна: "));
        string passwordHash = Hasher::hashPassword(console::readLine("Пароль адміна: "));
        if (loginHash != m_database.adminLoginHash || passwordHash != m_database.adminPasswordHash) {
            cout << "Невірний логін або пароль.\n";
            return false;
        }
        return true;
    }

    void setAdminData()
    {
        string loginHash;
        while (true) {
            string login = console::readLine("Новий логін: ");
            if (login.empty()) {
                cout << "Логін не може бути порожнім.\n";
                continue;
            }
            loginHash = Hasher::hashLogin(login);
            bool usedByUser = m_database.findUser(loginHash) != nullptr;
            if (!usedByUser)
                break;
            cout << "Такий логін вже має користувач.\n";
        }
        string password;
        while (password.size() < 4)
            password = console::readLine("Новий пароль (мінімум 4 символи): ");
        m_database.adminLoginHash = loginHash;
        m_database.adminPasswordHash = Hasher::hashPassword(password);
        m_database.save();
        cout << "Дані адміністратора збережено.\n";
    }

    void changeAdminData()
    {
        string current = console::readLine("Поточний пароль: ");
        if (Hasher::hashPassword(current) != m_database.adminPasswordHash) {
            cout << "Невірний пароль.\n";
            return;
        }
        setAdminData();
    }

    vector<string> userNames() const
    {
        vector<string> names;
        for (const auto& u : m_database.users)
            names.push_back(u.fullName + " | " + u.phone + " | " + u.address);
        return names;
    }

    void manageUsers()
    {
        while (true) {
            cout << "\n--- Користувачі ---\n"
                "1. Список\n2. Створити\n3. Змінити\n4. Видалити\n0. Назад\n";
            int choice = console::readInt("Ваш вибір: ", 0, 4);
            if (choice == 0)
                return;
            if (choice == 1) {
                auto names = userNames();
                cout << "\nКористувачі:\n";
                if (names.empty())
                    cout << "  (порожньо)\n";
                for (size_t i = 0; i < names.size(); ++i)
                    cout << "  " << (i + 1) << ". " << names[i] << "\n";
            }
            else if (choice == 2) {
                GuestMenu(m_database).registerUser();
            }
            else if (choice == 3) {
                editUser();
            }
            else {
                deleteUser();
            }
        }
    }

    void editUser()
    {
        int index = console::chooseItem("Кого змінити?", userNames());
        if (index < 0)
            return;
        User& user = m_database.users[index];
        cout << "Порожній ввід - залишити без змін.\n";
        string value = console::readLine("ПІБ [" + user.fullName + "]: ");
        if (!value.empty()) user.fullName = value;
        value = console::readLine("Адреса [" + user.address + "]: ");
        if (!value.empty()) user.address = value;
        value = console::readLine("Телефон [" + user.phone + "]: ");
        if (!value.empty()) user.phone = value;

        value = console::readLine("Новий логін (порожньо - не змінювати): ");
        if (!value.empty()) {
            string newHash = Hasher::hashLogin(value);
            if (m_database.loginExists(newHash))
                cout << "Логін зайнятий, не змінено.\n";
            else
                m_database.changeLogin(user, newHash);
        }
        value = console::readLine("Новий пароль (порожньо - не змінювати): ");
        if (!value.empty()) {
            if (value.size() < 4)
                cout << "Пароль закороткий, не змінено.\n";
            else
                user.passwordHash = Hasher::hashPassword(value);
        }

        for (auto& result : m_database.results)
            if (result.loginHash == user.loginHash)
                result.userName = user.fullName;
        m_database.save();
        cout << "Збережено.\n";
    }

    void deleteUser()
    {
        int index = console::chooseItem("Кого видалити?", userNames());
        if (index < 0)
            return;
        string hash = m_database.users[index].loginHash;
        if (console::askYesNo("Видалити користувача разом з його результатами?")) {
            m_database.removeUser(hash);
            m_database.save();
            cout << "Видалено.\n";
        }
    }

    vector<string> categoryNames() const
    {
        vector<string> names;
        for (const auto& c : m_database.categories)
            names.push_back(c.name);
        return names;
    }

    void showStatistics()
    {
        cout << "\n--- Статистика ---\n"
            "1. За категорією\n2. За конкретним тестом\n"
            "3. За користувачем\n4. Загальна\n0. Назад\n";
        int choice = console::readInt("Ваш вибір: ", 0, 4);
        if (choice == 0)
            return;

        vector<Result> selected;
        string title;
        if (choice == 1) {
            int c = console::chooseItem("Категорія:", categoryNames());
            if (c < 0) return;
            const string name = m_database.categories[c].name;
            title = "Категорія: " + name;
            for (const auto& r : m_database.results)
                if (r.categoryName == name) selected.push_back(r);
        }
        else if (choice == 2) {
            int c = console::chooseItem("Категорія:", categoryNames());
            if (c < 0) return;
            const Category& category = m_database.categories[c];
            vector<string> testNames;
            for (const auto& t : category.tests) testNames.push_back(t.name);
            int t = console::chooseItem("Тест:", testNames);
            if (t < 0) return;
            title = "Тест: " + category.name + " / " + category.tests[t].name;
            for (const auto& r : m_database.results)
                if (r.categoryName == category.name && r.testName == category.tests[t].name)
                    selected.push_back(r);
        }
        else if (choice == 3) {
            int u = console::chooseItem("Користувач:", userNames());
            if (u < 0) return;
            const User& user = m_database.users[u];
            title = "Користувач: " + user.fullName;
            for (const auto& r : m_database.results)
                if (r.loginHash == user.loginHash) selected.push_back(r);
        }
        else {
            title = "Усі результати";
            selected = m_database.results;
        }

        auto lines = Report::build(title, selected);
        cout << "\n";
        for (const auto& line : lines)
            cout << line << "\n";

        string path = console::readLine("\nФайл для збереження звіту (порожньо - не зберігати): ");
        if (!path.empty()) {
            ofstream out(path);
            if (!out) {
                cout << "Не вдалося відкрити файл.\n";
                return;
            }
            for (const auto& line : lines)
                out << line << "\n";
            cout << "Звіт збережено у " << path << "\n";
        }
    }

    void manageTests()
    {
        while (true) {
            cout << "\n--- Управління тестуванням ---\n"
                "1. Додати категорію\n2. Додати тест\n3. Додати питання\n"
                "4. Переглянути структуру\n5. Видалити (категорію/тест/питання)\n"
                "6. Імпорт з файлу\n7. Експорт у файл\n0. Назад\n";
            int choice = console::readInt("Ваш вибір: ", 0, 7);
            if (choice == 0)
                return;
            switch (choice) {
            case 1: addCategory(); break;
            case 2: addTest(); break;
            case 3: addQuestion(); break;
            case 4: showStructure(); break;
            case 5: deleteItem(); break;
            case 6: importTests(); break;
            default: exportTests(); break;
            }
        }
    }

    void addCategory()
    {
        string name = console::readLine("Назва категорії: ");
        if (name.empty())
            return;
        if (m_database.findCategory(name) != nullptr) {
            cout << "Така категорія вже існує.\n";
            return;
        }
        m_database.categories.push_back({ name,{} });
        m_database.save();
        cout << "Категорію додано.\n";
    }

    void addTest()
    {
        int c = console::chooseItem("До якої категорії додати тест?", categoryNames());
        if (c < 0)
            return;
        Category& category = m_database.categories[c];
        string name = console::readLine("Назва тесту: ");
        if (name.empty())
            return;
        for (const auto& t : category.tests)
            if (t.name == name) {
                cout << "Такий тест вже існує.\n";
                return;
            }
        category.tests.push_back({ name,{} });
        m_database.save();
        cout << "Тест додано.\n";
    }

    Test* chooseTest()
    {
        int c = console::chooseItem("Категорія:", categoryNames());
        if (c < 0)
            return nullptr;
        Category& category = m_database.categories[c];
        vector<string> names;
        for (const auto& t : category.tests)
            names.push_back(t.name);
        int t = console::chooseItem("Тест:", names);
        return t < 0 ? nullptr : &category.tests[t];
    }

    void addQuestion()
    {
        Test* test = chooseTest();
        if (test == nullptr)
            return;
        Question question;
        question.text = console::readLine("Текст питання: ");
        if (question.text.empty())
            return;
        cout << "Вводьте відповіді. Порожній рядок - завершити.\n";
        while (true) {
            string text = console::readLine("Відповідь №" +
                to_string(question.answers.size() + 1) + ": ");
            if (text.empty())
                break;
            bool correct = console::askYesNo("Це правильна відповідь?");
            question.answers.push_back({ text, correct });
        }
        bool hasCorrect = any_of(question.answers.begin(), question.answers.end(),
            [](const Answer& a) { return a.correct; });
        if (question.answers.size() < 2 || !hasCorrect) {
            cout << "Потрібно щонайменше 2 відповіді та 1 правильна. Питання не додано.\n";
            return;
        }
        test->questions.push_back(question);
        m_database.save();
        cout << "Питання додано.\n";
    }

    void showStructure() const
    {
        cout << "\n(* - правильна відповідь)\n";
        for (const auto& category : m_database.categories) {
            cout << "\n[" << category.name << "]\n";
            for (const auto& test : category.tests) {
                cout << "  - " << test.name << " (питань: " << test.questions.size() << ")\n";
                for (size_t i = 0; i < test.questions.size(); ++i) {
                    cout << "      " << (i + 1) << ". " << test.questions[i].text << "\n";
                    for (const auto& a : test.questions[i].answers)
                        cout << "         " << (a.correct ? "* " : "  ") << a.text << "\n";
                }
            }
        }
    }

    void deleteItem()
    {
        cout << "\nЩо видалити? 1. Категорію  2. Тест  3. Питання  0. Назад\n";
        int level = console::readInt("Ваш вибір: ", 0, 3);
        if (level == 0)
            return;
        int c = console::chooseItem("Категорія:", categoryNames());
        if (c < 0)
            return;
        Category& category = m_database.categories[c];

        if (level == 1) {
            if (console::askYesNo("Видалити категорію \"" + category.name + "\" з усіма тестами?")) {
                m_database.categories.erase(m_database.categories.begin() + c);
                m_database.save();
                cout << "Видалено.\n";
            }
            return;
        }
        vector<string> names;
        for (const auto& t : category.tests)
            names.push_back(t.name);
        int t = console::chooseItem("Тест:", names);
        if (t < 0)
            return;
        if (level == 2) {
            category.tests.erase(category.tests.begin() + t);
            m_database.save();
            cout << "Видалено.\n";
            return;
        }
        Test& test = category.tests[t];
        vector<string> questions;
        for (const auto& q : test.questions)
            questions.push_back(q.text);
        int q = console::chooseItem("Питання:", questions);
        if (q < 0)
            return;
        test.questions.erase(test.questions.begin() + q);
        m_database.save();
        cout << "Видалено.\n";
    }

    void importTests()
    {
        string path = console::readLine("Файл для імпорту: ");
        ifstream in(path);
        if (!in) {
            cout << "Не вдалося відкрити файл.\n";
            return;
        }
        vector<Category> imported;
        string error;
        if (!testFile::read(in, imported, error)) {
            cout << "Помилка формату: " << error << "\n";
            return;
        }
        int addedTests = 0, skippedTests = 0;
        for (auto& newCategory : imported) {
            Category* target = m_database.findCategory(newCategory.name);
            if (target == nullptr) {
                m_database.categories.push_back({ newCategory.name,{} });
                target = &m_database.categories.back();
            }
            for (auto& newTest : newCategory.tests) {
                bool exists = any_of(target->tests.begin(), target->tests.end(),
                    [&](const Test& t) { return t.name == newTest.name; });
                if (exists) {
                    ++skippedTests;
                }
                else {
                    target->tests.push_back(move(newTest));
                    ++addedTests;
                }
            }
        }
        m_database.save();
        cout << "Імпортовано тестів: " << addedTests << ", пропущено (дублікати): "
            << skippedTests << "\n";
    }

    void exportTests()
    {
        vector<Category> selected;
        int c = console::chooseItem("Що експортувати? Оберіть категорію (0 - експортувати все):",
            categoryNames());
        if (c < 0) {
            selected = m_database.categories;
        }
        else {
            const Category& category = m_database.categories[c];
            vector<string> names;
            for (const auto& t : category.tests)
                names.push_back(t.name);
            int t = console::chooseItem("Тест (0 - уся категорія):", names);
            Category copy{ category.name,{} };
            if (t < 0)
                copy.tests = category.tests;
            else
                copy.tests.push_back(category.tests[t]);
            selected.push_back(copy);
        }
        string path = console::readLine("Файл для експорту: ");
        ofstream out(path);
        if (path.empty() || !out) {
            cout << "Не вдалося відкрити файл.\n";
            return;
        }
        testFile::write(out, selected);
        cout << "Експортовано у " << path << "\n";
    }
};

class App {
public:
    App() : m_database("data") {}

    void run()
    {
        cout << "=== Система тестування ===\n";
        try {
            while (true) {
                cout << "\n--- Головне меню ---\n"
                    "1. Тестований\n2. Адміністратор\n0. Вихід\n";
                int choice = console::readInt("Ваш вибір: ", 0, 2);
                if (choice == 0)
                    break;
                if (choice == 1)
                    GuestMenu(m_database).run();
                else
                    AdminMenu(m_database).run();
            }
        }
        catch (const InputClosed&) {
            cout << "\n";
        }
        m_database.save();
        cout << "До побачення!\n";
    }

private:
    Database m_database;
};

int main()
{
#ifdef _WIN32
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
#endif
    App app;
    app.run();
    return 0;
}