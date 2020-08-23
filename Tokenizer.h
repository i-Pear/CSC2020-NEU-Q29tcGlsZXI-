#ifndef COURSE_DESIGN_OF_COMPILING_NEWTOKEN_H
#define COURSE_DESIGN_OF_COMPILING_NEWTOKEN_H

#define DEBUG

#include "Utils.h"
#include <algorithm>


#pragma once


#include "Utils.h"


using namespace std;

class Tokenizer {
private:
    static const int TABLES_NUM = 6, LIMIT = 10000;
    bool inLineComment;

    unordered_set<string> key_words, delimiters; // 关键字集合，界符集合

    Allocator<string> words[TABLES_NUM]; // word分配器

    deque<int> res; // 缓存

    bool is_num(char c); // 数字判断

    string ctos(char c); // 字符转字符串

    void init();

public:

    int lineNumber;

    Tokenizer();

    static Token_Type type(int id); // 返回一个id的类型

    const string &operator[](int i); // 返回id对应的字符串

    int operator()(const string &str); // 给字符串分配一个id

    bool operator>>(int &i); // 流输出一个id

    Tokenizer &operator<<(const string &cod); // 流输入字符串

    friend void operator>>(ifstream &pre, Tokenizer &tk); // 接收预编译器输出流

    void push_back(int id);

    int forward(int num);
};

extern Tokenizer tokenizer;

#endif //COURSE_DESIGN_OF_COMPILING_NEWTOKEN_H