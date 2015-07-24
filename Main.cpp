#include "Cradle.h"
#include <iostream>
#include <stack>
#include <map>

#define INPUT_BUFFER_SIZE BUFFER_SIZE
#define STACK_MAX 1024

std::map <std::string, int> vars;
std::map <std::string, std::map<std::string, int>> structures;
std::map <std::string, unsigned> procedures;
std::stack <Cradle::Position> callStack;
unsigned short stackSize = 0;

int factor();
int term();
int expression();
void assignment();
void input();
void output();
void program();
bool command();
bool runIf();
bool tryElse();
void discardElse();
void runGoto();
void print();
void callProc();
void returnFromProc();
void doImport();
void createStructure();

using namespace Cradle;

void setupProcedures(const std::vector<std::string>& vec) {
    std::string proc = "procedure";
    for (unsigned i = 0; i < vec.size(); ++i) {
        if (vec[i].compare(0, proc.size(), proc) == 0) {
            std::string name = vec[i];
            name.erase(0, proc.size() + 1);
            name.erase(name.find('\n'));
            if (name.find_first_of(' ') < name.size())
                name.erase(name.find_first_of(' '), name.size());

            if (name.size() > BUFFER_SIZE)
                fatalError(0x2, "Identifier too long\n\tAt line %u\n", i + 1);

            procedures[name] = i;
        }
    }
}

int main(int argc, char** argv) {
    if (argc < 2)
        fatalError(0xF0, "No input file\n");

    read(argv[1], code);

    setupProcedures(code);

    init();

    program();

    return 0;
}

int factor() {
    int value = 0;

    skipWhite();
    if (look == '(') {
        nextChar();
        value = expression();
        match(')');
    } else if (isalpha(look) || look == '_') {
        char name[BUFFER_SIZE];
        getName(name);
        if (look != '.')
            value = vars[name];
        else {
            std::string str(name);
            nextChar();
            getName(name);
            value = structures[str][name];
        }
    } else
        value = getNum();

    return value;
}

int term() {
    int value = 0;

    value = factor();

    skipWhite();
    while (isMulOp()) {
        switch (look) {
        case '*':
            nextChar();
            value *= factor();
            break;
        case '/':
            nextChar();
            value /= factor();
            break;
        }
    }

    return value;
}

int expression() {
    int value = 0;

    if (!isAddOp())
        value = term();
    while (isAddOp()) {
        switch (look) {
        case '+':
            nextChar();
            value += term();
            break;
        case '-':
            nextChar();
            value -= term();
            break;
        }
    }

    return value;
}

void assignment() {
    char name[BUFFER_SIZE];
    getName(name);

    if (look == '.') {
        std::string str(name);
        nextChar();
        getName(name);
        if (structures[str].find(name) == structures[str].end())
            stackSize += sizeof(int);

        skipWhite();
        match('=');
        skipWhite();

        structures[str][name] = expression();
    } else {
        if (vars.find(name) == vars.end())
            stackSize += sizeof(int);
        skipWhite();
        match('=');
        skipWhite();

        vars[name] = expression();
    }

    if (stackSize > STACK_MAX)
            fatalError(0xB, "Stack Overflow\n\tAt: %u:%u", where.line + 1, where.column + 1);
}

void input() {
    char buffer[INPUT_BUFFER_SIZE];
    char name[BUFFER_SIZE];

    skipWhite();
    getName(name);

    std::cin >> buffer;
    vars[name] = atoi(buffer);
}

void output() {
    skipWhite();
    std::cout << expression();
}

void program() {
    while (command()) {
        newLine();
    }
}

bool command() {
    while (look == ' ' || look == '\t') skipWhite();

    if (look != '\n') {
        while (code[where.line][0] == '#') discardLine();

        Position before = where;
        char com[BUFFER_SIZE];

        if (eof) return false;
        getName(com);

        std::string comStr(com);

        if (comStr == "get") input();
        else if (comStr == "put") output();
        else if (comStr == "print") print();
        else if (comStr == "if") return runIf();
        else if (comStr == "else") discardLine();
        else if (comStr == "goto") runGoto();
        else if (comStr == "clear") clearConsole();
        else if (comStr == "pause") pause();
        else if (comStr == "procedure") discardLine();
        else if (comStr == "call") callProc();
        else if (comStr == "return") returnFromProc();
        else if (comStr == "import") doImport();
        else if (comStr == "structure") createStructure();
        else if (comStr == "exit") return false;
        else {
            where = before;
            --where.column;
            nextChar();
            assignment();
        }
    }
    return true;
}

inline bool ifCommand() {
    if (look == '\n')
        fatalError(0x6, "Command expected\n\tAt: %u:%u\n", where.line + 1, where.column + 1);
    return command();
}

bool runIf() {
    skipWhite();
    match('(');
    skipWhite();

    int left = expression();
    char op = look;
    nextChar();
    skipWhite();
    int right = expression();

    match(')');
    skipWhite();

    switch (op) {
    case '=':
        if (left != right) {
            discardLine();
            tryElse();
        }
        else {
            bool value = ifCommand();
            discardElse();
            return value;
        }
        break;
    case '!':
        if (left == right) {
            discardLine();
            tryElse();
        }
        else {
            bool value = ifCommand();
            discardElse();
            return value;
        }
        break;
    case '>':
        if (left <= right) {
            discardLine();
            tryElse();
        }
        else {
            bool value = ifCommand();
            discardElse();
            return value;
        }
        break;
    case '<':
        if (left >= right) {
            discardLine();
            tryElse();
        }
        else {
            bool value = ifCommand();
            discardElse();
            return value;
        }
        break;
    default:
        fatalError(0x5, "Evaluation operator expected\n\tAt: %u:%u\n", where.line + 1, where.column + 1);
    }

    return true;
}

bool tryElse() {
    Position before = where;
    --before.column;

    skipWhite();

    char name[BUFFER_SIZE];
    getName(name);
    std::string str(name);

    if (str == "else")
        return command();
    else {
        where = before;
        nextChar();
    }

    discardElse();
    return true;
}

void discardElse() {
    Position before = where;
    if (before.column != -1) --before.column;

    newLine();
    skipWhite();

    char name[BUFFER_SIZE];

    getName(name);
    std::string str(name);

    if (str == "else") {
        discardLine();
        discardElse();
    }
    else {
        where = before;
        nextChar();
    }
}

void runGoto() {
    skipWhite();
    int line = expression() - 1;

    if (line < 0 || line > code.size())
        fatalError(0x7, "%i is not a valid line number\n\tAt: %u:%u", where.line + 1, where.column + 1);

    where.line = line;
    where.column = -1;
}

void print() {
    std::string line = code[where.line];
    line.erase(0, where.column);

    if (line[0] != '"')
        fatalError(0xF, "'\"' expected\n\tAt: %u:%u", where.line + 1, where.column + 1);
    line.erase(line.begin());

    bool done = false;
    for (unsigned i = 0; i < line.size() - 1 && !done; ++i) {
        switch (line.at(i)) {
            case '\\':
                if (++i == line.size() - 1)
                    fatalError(0xD, "End of line unexpected\n\tAt: %u:%u", where.line + 1, where.column + 1 + i);
                switch (line.at(i)) {
                    case 'n':
                        std::cout << '\n';
                        break;
                    case 't':
                        std::cout << '\t';
                        break;
                    case '\\':
                        std::cout << '\\';
                        break;
                    case '"':
                        std::cout << '"';
                        break;
                    default:
                        fatalError(0xE, "Unknown special character\n\tAt: %u:%u", where.line + 1, where.column + 1 + i);
                        break;
                }
                break;
            case '\n':
                fatalError(0xF, "'\"' expected\n\tAt: %u%u", where.line + 1, where.column + 1 + i);
                break;
            case '"':
                done = true;
                break;
            default:
                std::cout << line[i];
                break;
        }
    }

    where.column = -1;
    ++where.line;
    nextChar();
}

void callProc() {
    char name[BUFFER_SIZE];
    getName(name);
    skipWhite();

    if (procedures.find(name) == procedures.end())
        fatalError(0x9, "Procedure '%s' not found\n\tAt: %u:%u", where.line + 1, where.column + 1);

    std::string arg = "__arg";
    for (unsigned i = 0; look != '\n' && look != '#'; ++i) {
        vars[arg + std::to_string(i)] = expression();
        skipWhite();
    }

    stackSize += sizeof(Position);

    if (stackSize > STACK_MAX) {
        fatalError(0xB, "Stack Overflow\n\tAt: %u:%u", where.line + 1, where.column + 1);
    }

    callStack.push(where);

    where.column = -1;
    where.line = procedures[name] + 1;
}

void returnFromProc() {

    if (callStack.empty()) {
        fatalError(0x8, "'return' unexpected\n\tAt: %u:%u", where.line + 1, where.column + 1);
    }

    skipWhite();
    if (look != '\n' && look != '#')
        vars["__returned"] = expression();

    where = callStack.top();

    callStack.pop();
    stackSize -= sizeof(Position);
}

void doImport() {
    skipWhite();
    std::string name;

    while (look != '\n' && look != '#') {
        name += look;
        nextChar();
    }

    std::vector<std::string> content;
    read(name.c_str(), content);

    code.swap(content);
    Position before = where;
    where.line = 0;
    where.column = -1;
    nextChar();
    createStructure();
    code.swap(content);
    where = before;
    --where.column;
    nextChar();
}

void createStructure() {
    skipWhite();
    char name[BUFFER_SIZE];
    getName(name);
    std::string str(name);

    do {
        newLine();
        skipWhite();

        getName(name);

        skipWhite();
        match(':');
        skipWhite();

        structures[str][name] = expression();

        if (look == ',')
            nextChar();
        stackSize += sizeof(int);

        if (stackSize > STACK_MAX)
            fatalError(0xB, "Stack Overflow\n\tAt: %u:%u", where.line + 1, where.column + 1);
    } while (look != ';');
    nextChar();
}
