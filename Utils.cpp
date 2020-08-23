#include "Utils.h"

int totalStackSize;

bool largeMem;

int READ_BUFFER;

CompileERROR::CompileERROR(const string &message, ERRORType type) : message(message), type(type) {}

const char *CompileERROR::what() const noexcept { return message.c_str(); }

string reg2str(RegType type){
    switch(type){
        case r0:
            return "r0";
        case r1:
            return "r1";
        case r2:
            return "r2";
        case r3:
            return "r3";
        case r4:
            return "r4";
        case r5:
            return "r5";
        case r6:
            return "r6";
        case r7:
            return "r7";
        case r8:
            return "r8";
        case r9:
            return "r9";
        case r10:
            return "r10";
        case r11:
            return "r11";
        case r12:
            return "r12";
        case null:
            return "null";
    }
}

ostream& operator <<(ostream& oss,RegType reg){
    oss<<reg2str(reg);
    return oss;
}

bool isEmpty(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

string trim(string s) {
    while ((!s.empty())&&isEmpty(s.front()))s.erase(s.begin());
    while ((!s.empty())&&isEmpty(s.back()))s.pop_back();
    return s;
}

bool startWith(const string &main, const string &sub) {
    if (sub.length() > main.length())return false;
    return (main.substr(0, sub.length()) == sub);
}

bool is_num(char c) {
    return c >= '0' && c <= '9';
}

string ctos(char c) {
    string str;
    str += c;
    return str;
}

bool isSymbol(char c){
    return !(isalpha(c)||c=='_');
}

ofstream writer;

int get_lowest_bit(int x){
    int i=0;
    while ((x&(1<<i))==0){
        i++;
    }
    return i;
}
