#pragma once

#include "Utils.h"
#include "Symbol.h"
#include "RegControl.h"

using namespace std;

void jmp_condition(int label);

void jmp(int label);

void def_label(int label);

void get_flag();

void alloc_stack(int diff);

void free_stack(int diff);

void load_constVal(int val);

void load_addrVal(int addr);

void push_r0();

void push_r1();

void push_r11();

void push_r3();

void push_r12();

void push_bp();

void pop_bp();

void pop_r0();

void pop_r1();

void pop_r11();

void pop_r3();

void pop_r12();

void get_sign();

void get_bool();

void get_not();

void get_neg();

void shl(int step);

void shr(int step);

void getChar();

void putChar();

void call_func(int varDiff, const string&func_name);

void func_declare(const string&name);

void func_declare_ends(const string&name);

void transToTemp();

void add();

void sub();

void mul();

void div();

void getint();

void putint();

void _sysy_starttime();

void _sysy_stoptime();

void clear_ax();

string get_cond(ExprAST* node);

string get_not_cond(ExprAST* node);