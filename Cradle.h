#ifndef CRADLE_H_INCLUDED
#define CRADLE_H_INCLUDED

#define BUFFER_SIZE 33

#include <cstdio>
#include <cctype>
#include "Errors.h"
#include <fstream>
#include <vector>

#ifndef LINUX
#include <windows.h>
#endif

void clearConsole() {
#ifdef LINUX
    std::cout << "\033[2J\033[1;1H";
#else
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD coord = {0, 0};
    DWORD count;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hStdOut, &csbi);
    FillConsoleOutputCharacter(hStdOut, ' ', csbi.dwSize.X * csbi.dwSize.Y, coord, &count);
    SetConsoleCursorPosition(hStdOut, coord);
#endif
}

inline void pause() {
    if (getchar() == '\n') getchar();
}

namespace Cradle {

    char look;
    std::vector<std::string> code;
    bool eof;

    struct Position {
        unsigned line;
        unsigned column;
    } where;

    void read(const char* filePath, std::vector<std::string>& vec) {
        std::ifstream file(filePath);
        if (file.fail())
            fatalError(0xF1 ,"Could not open file %s\n", filePath);

        std::string line;
        while (!file.eof()) {
            std::getline(file, line);
            line += '\n';
            vec.push_back(line);
        }
    }

    void nextChar() {
        if(++where.column == code[where.line].size()) {
            ++where.line;
            where.column = 0;
        }
        if (where.line < code.size())
            look = code[where.line][where.column];
        else
            eof = true;
    }

    void skipWhite() {
        while (look == ' ' || look == '\t')
            nextChar();
    }

    void init() {
        where.line = 0;
        where.column = -1;
        nextChar();
        skipWhite();
    }

    void match(char c) {
        if (look != c)
            fatalError(0x4, "'%c' expected\n\tAt: %u:%u\n", c, where.line + 1, where.column + 1);
        nextChar();
    }

    void getName(char* name) {
        if(!isalpha(look) && look != '_')
            fatalError(0x1, "Identifier expected\n\tAt: %u:%u\n", where.line + 1, where.column + 1);

        unsigned i = 0;
        for (; isalnum(look) || look == '_'; ++i) {
            if (i > BUFFER_SIZE)
                fatalError(0x2, "Identifier too long\n\tAt: %u:%u\n", where.line + 1, where.column + 1);
            name[i] = look;
            nextChar();
        }

        name[i] = '\0';
        skipWhite();
    }

    int getNum() {
        if(!isdigit(look))
            fatalError(0x3, "Integer expected\n\tAt: %u:%u\n", where.line + 1, where.column + 1);

        int i = 0;

        while (isdigit(look)) {
            i *= 10;
            i += look - '0';
            nextChar();
        }

        skipWhite();
        return i;
    }

    void newLine() {
        while (look == '\n')
            nextChar();
    }

    void discardLine() {
        while (look != '\n') nextChar();
        newLine();
    }

    bool isAddOp() {
        return look == '+' || look == '-';
    }

    bool isMulOp() {
        return look == '*' || look == '/';
    }
}
#endif // CRADLE_H_INCLUDED
