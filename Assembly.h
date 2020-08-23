#pragma once

#include "Utils.h"
#include "AST.h"
#include "AsmHelper.h"
#include "Symbol.h"
#include "RegControl.h"

using namespace std;

extern int lastFuncRetLabel;

extern vector<pair<string,int>> blocks;

void _generateAsm(ExprAST* astNode,RegType target=r0,bool once=false);

void generateAsm(ExprAST* astNode);

RegType loadVar(ExprAST* node,bool is_Silent=false);

void deal_conditions(ExprAST* node,int successLabel,int failLabel);

int get_left_son(ExprAST* node);
