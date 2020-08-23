#pragma once

#include <bits/stdc++.h>

using namespace std;

#define pr make_pair
#define DISTRIBUTE_DEBUG

extern bool largeMem;

extern int READ_BUFFER;

extern int totalStackSize;

enum RegType{
    null=100, r0=0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12
};

string reg2str(RegType type);

ostream& operator <<(ostream& oss,RegType reg);

class CompileERROR : public exception{
    /**
     * A general exception class
     */
public:
    enum ERRORType{
        unknown, import,
    };
    string message;
    ERRORType type;

    CompileERROR(const string&message, ERRORType type=unknown);

    const char*what() const noexcept;
};

/**
* return whether input is an empty char
*/
bool isEmpty(char c);

/**
* erase empty chars at head and tail
*/
string trim(string s);

/**
* return whether string SUB is a sub-string of string MAIN
*/
bool startWith(const string&main, const string&sub);

/**
 * judge c whether a num
 * */
bool is_num(char c);

/**
 * change char to string
 * */
string ctos(char c);

template<class T>
class Adapter{
public:
    T*var;

    bool operator<(const Adapter<T>&o) const{
        return *var<*o.var;
    }

    Adapter(){ var=new T; }
};

template<class T>
class Allocator{
private:
    unordered_map<int, Adapter<T>> id_to_T;
    map<Adapter<T>, int> T_to_id;
    int id;
public:
    Allocator(){
        id=0;
    }

    Allocator(int init){
        id=init;
    }

    T&operator[](int i){
        return *id_to_T[i].var;
    }

    int operator()(T t){
        Adapter<T> adapter;
        *adapter.var=t;
        if(!T_to_id.count(adapter)){
            T_to_id[adapter]=id;
            id_to_T[id]=adapter;
            id++;
        }
        return T_to_id[adapter];
    }
};

bool isSymbol(char c);

// 类型标志
enum Token_Type{
    TYPE_DELIMITER, TYPE_KEYWORD, TYPE_NUM, TYPE_SYMBOL, TYPE_CHARACTER, TYPE_STRING
};

extern ofstream writer;

int get_lowest_bit(int n);