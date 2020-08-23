#pragma once

#include "Utils.h"
#include "AST.h"

using namespace std;

extern const int UNIT_BITS;
extern const int UNIT_SIZE;
extern int MAIN_FUNC_SHIFTING;

extern stack<int> loop_ending_labels;
extern stack<int> loop_start_labels;

extern stack<map<int,int>> tiny_constant_table;

extern map<int,vector<ExprAST*>> initialization_lists;

class SymTable{
public:
    struct ArrayInfo{
        int size;
        int dimCount;
        vector<int> dims;
        vector<int> diff_unit;

        void getDiff(){
            diff_unit.resize(dims.size());
            int mul=1;
            for(int i=dimCount-1; i>=0; i--){
                diff_unit[i]=mul;
                mul*=dims[i];
            }
            size=mul;
        }
    };

    struct Table{
        bool isGlobal;
        int diff;
        map<int, int> map;
        ::map<int, ArrayInfo> arrMap;
        ::map<int, int> constant;
        set<int> newlyDefined;
    };
    stack<Table> data;

    SymTable();

    bool exists(int id);

    void alloc(int id, int addr);

    void into(int diff);

    void pop();

    int operator[](int id);
};

extern map<int, FuncPrototypeAST*> FuncTable;

extern bool _isFuncArgTable;

extern stack<pair<int, int>> addrLenStack;

extern SymTable symTable;

void parse_init_list(ExprAST*node,vector<ExprAST*>& list,vector<int>& diff_unit,int& pos,int floor);

int getFuncCodeLines(ExprAST*node);

void distributeVars(ExprAST*node);

int getBlockVarInfo(ExprAST*node);

int getNodeCount(ExprAST*node);

int calculate_constant(ExprAST*node,int mode);