#pragma once

#define CONSTANT_FOLDING

#include "Tokenizer.h"
//#include "Visualization.h"

#define E(str) throw CompileERROR(str);
#define curStr tokenizer[cur]
#define ET() E("error tokenizer '" + curStr + "'")
#ifdef DEBUG
#define DC(str) cout << (str) << endl;
#else
#define DC(str) ;
#endif

class ExprAST;

typedef ExprAST *ExprAddr;

int getLabelID();

enum OP {
    OP_ADD, OP_MINUS, OP_MUL, OP_DIV,
    OP_LESS, OP_AND, OP_OR, OP_BIGGER, OP_EQUAL,
    OP_LESS_OR_EQUAL, OP_BIGGER_OR_EQUAL, OP_NOT_EQUAL,
    OP_ASSIGN, OP_DECLARE, OP_NOT, OP_MOD,
    OP_SINGLE_MINUS
};

extern map<OP, string> op_to_str;


enum NodeType {
    nullExpr,
    constExpr,
    varExpr,
    binExpr,
    ifExpr,
    elseExpr,
    whileExpr,
    funcExpr,
    funcArg,
    returnExpr,
    callExpr,
    arrayPrototypeExpr,
    arrayCallExpr,
    breakExpr,
    continueExpr,
    initialListExpr,
    blockExpr
};

class Block {
public:
    int varCount;
};

class ExprValue {
public:
    int value;
    bool with_value;
    bool is_const;

    ExprValue() = default;

    ExprValue(int val, bool withVal, bool is_const = false) : value(val), with_value(withVal), is_const(is_const) {}
};

class TempValue {
public:
    bool isTemp;
    int tempName;

    TempValue() : isTemp(false), tempName(0) {}

    TempValue(bool isTemp, int tempName) : isTemp(isTemp), tempName(tempName) {}

    static int allocName() {
        static int id = 0;
        static string tempBase = "_t";
        return tokenizer(tempBase + to_string(id++));
    }
};

enum BuffFlag {
    BUFF_NO_EFFECT, BUFF_PUSH, BUFF_POP
};

class Buff {
public:
    BuffFlag flag;
    int name;
    int addr;

    Buff(BuffFlag f, int name) : flag(f), name(name), addr(0) {}
};

class ExprAST {
public:

    ExprAST *next;
    ExprAST *back;
    ExprValue value;
    NodeType type;
    int vi_id;
    list<ExprAddr *> forward;
    Buff buff;

    ExprAST(NodeType type);

    ExprAST(NodeType type, int v);

    void bind(ExprAddr *node);

    virtual ~ExprAST() = default;
};

int calculate(OP op, ExprAST *lhs, ExprAST *rhs);

int calculate(OP op, ExprAST *rhs);

class NullExprAST : public ExprAST {
public:
    NullExprAST();
};

int calculate(OP op, ExprAST *lhs, ExprAST *rhs);

int calculate(OP op, ExprAST *rhs);

class ConstExprAST : public ExprAST {
public:
    ConstExprAST(int value);
};

class VariableAST : public ExprAST {
public:
    int name;
    bool isNewVar;
    bool isGlobal;
    int addr;

    int binLabel;

    VariableAST(int name, bool isNewVar);
};

class PointerAST : public VariableAST {
public:
    PointerAST(NodeType type_, VariableAST *var) : VariableAST(var->name, var->isNewVar) {
        isGlobal = var->isGlobal;
        addr = var->addr;
        type = type_;
//        vi_id = vi.allocate("pointer");
    };
};

class BinaryOpAST : public ExprAST {
public:
    OP op;
    ExprAST *LHS;
    ExprAST *RHS;
    int binLabel;

    BinaryOpAST(OP op, ExprAST *lhs, ExprAST *rhs);

};

class IfAST : public ExprAST, public Block {
public:
    ExprAST *expression;
    ExprAST *code;
    ExprAST *elseCode;

    IfAST(ExprAST *expression, ExprAST *code);

};

class ElseAST : public ExprAST, public Block {
public:
    ExprAST *code;

    ElseAST(ExprAST *code);

};

class WhileAST : public ExprAST, public Block {
public:
    ExprAST *expression;
    ExprAST *code;

    WhileAST(ExprAST *expression, ExprAST *code);

};

class FuncPrototypeAST : public ExprAST, public Block {
public:
    enum ReturnType {
        RET_INT, RET_VOID
    };

    int name;
    ReturnType retType; // int or void
    ExprAST *args;
    ExprAST *code;
    int argsCount;
    int codeLines;

    FuncPrototypeAST(int name, ExprAST *args, ExprAST *code, ReturnType type);

};

enum FuncArgState {
    ARG_PLAIN, ARG_POINTER, ARG_QUOTE
};

class ArrayArg {
public:
    bool isArray;
    vector<ExprAST *> dimensions;

    ArrayArg(bool isArray) {
        this->isArray = isArray;
    }
};

class FuncArgAST : public ExprAST {
public:
    int name;
    int addr;
    ArrayArg array;

    FuncArgAST(int name, ArrayArg array);
};

class ReturnAST : public ExprAST {
public:
    ExprAST *res;

    ReturnAST(ExprAST *res);

};

class CallAST : public ExprAST {
public:
    int name;
    ExprAST *args;
    int argsCount;

    CallAST(int name, ExprAST *args);

};

class ArrayPrototypeAST : public ExprAST {
public:
    ExprAST *variable;
    vector<ExprAST *> dimensions;
    ExprAST *initValues;
    int initialization_list_id;

    ArrayPrototypeAST(ExprAST *variable, vector<ExprAST *> dimensions);

};

class ArrayCallAST : public ExprAST {
public:
    ExprAST *variable;
    vector<ExprAST *> indexes;
    vector<int> dimSizes;
    vector<int> dim_diff;

    ArrayCallAST(NodeType type, ExprAST *variable, vector<ExprAST *> dimensions);

};

class BreakAST : public ExprAST {
public:
    BreakAST(NodeType type);
};

class ContinueAST : public ExprAST {
public:
    ContinueAST(NodeType type);
};

class InitialListAST : public ExprAST {
public:
    ExprAST *head;

    InitialListAST(ExprAST *head);
};

class BlockAST : public ExprAST, public Block {
public:
    ExprAST *code;

    BlockAST(ExprAST *code) : ExprAST(blockExpr), code(code) {
        bind(&this->code);
    }
};


class Parser {
private:
    unordered_map<string, OP> ops;
    unordered_map<string, int> op_precedence;
public:
    int cur;
    ExprAST *head;


    int next();

    Parser();

    void init();

    int to_int(const string &str);

    inline void need(const string &str);

    ExprAST *ParseConst();

    ExprAST *ParseVariable(bool isNewVar);

    int get_precedence(int id, bool is_single = false);


    OP id_to_op(int id);

    ExprAST *ParsePrimary();

    ExprAST *ParseParenExpr();

    ExprAST *ParseExpression(int prec = 0);

    ExprAST *ParseBinOpRHS(int exprPrec, ExprAST *LHS, bool is_single = false);

    ExprAST *ParseVar(bool is_const = false);

    ExprAST *ParseIf();

    ExprAST *ParseElse();

    ExprAST *ParseWhile();

    ExprAST *ParseFuncPrototype(FuncPrototypeAST::ReturnType type);

    ExprAST *ParseReturn();

    ExprAST *ParseCall(int name);

    ExprAST *ParseArrayPrototype(bool is_const = false);

    ExprAST *ParseArrayCall(int name);

    ExprAST *ParseSingleOp();

    ExprAST *ParseInitialList();

    ExprAST *ParseVarOrArrayPrototype(bool is_const = false);

    ExprAST *ParseCode(bool oneLine = false);

    ExprAST *ParseStart();

    vector<ExprAST *> ParseArrayDimension();

    void findMatchedIf(ExprAST *p, ExprAST *elseA);


    void fixGlobalVar() {
        ExprAST *p = head->next, *father = head;
        ExprAST *pHead = new NullExprAST, *c = pHead;
        while (p) {
            if (p->type != funcExpr) {
                c->next = p;
                father->next = p->next;
                p = father;
                c->next->next = nullptr;
                c->next->back = nullptr;
                c = c->next;
            }
            father = p;
            p = p->next;
        }
        ExprAST *globalBlock = new BlockAST(pHead);
        globalBlock->next = head->next;
        head->next = globalBlock;
    }
};

void eraseEmpty(ExprAST *node, ExprAddr *from);

class BuffTable {
public:
    // 缓存使用情况
    map<string, int> buffUsedCount;
    // 变量相关的缓存
    map<int, vector<string>> varAboutBuffs;
    // 缓存命中的表达式
    map<string, vector<ExprAST *>> str2buffExpr;
    // 所有使用过的临时变量
    set<string> buffs;

    // 查找缓存，如果没有则创建一个
    void findBuff(ExprAST *expr, string buffStr, vector<int> aboutVars) {
        if (buffUsedCount.count(buffStr)) {
            cout << "Detected Existed Buff: " << buffStr << endl;
            buffUsedCount[buffStr]++;
            expr->buff.flag = BUFF_POP;
            expr->buff.name = tokenizer(buffStr);
        } else {
            cout << "Detected New Buff: " << buffStr << endl;
            buffUsedCount[buffStr] = 0;
            for (auto &i:aboutVars) {
                varAboutBuffs[i].push_back(buffStr);
            }
            expr->buff.flag = BUFF_PUSH;
            expr->buff.name = tokenizer(buffStr);
            str2buffExpr[buffStr].push_back(expr);
        }
        buffs.insert(buffStr);
    }

    // 清除缓存
    void eraseAbout(int var) {
        for (auto &i:varAboutBuffs[var]) {
            if (buffUsedCount.count(i)) {
                if (buffUsedCount[i] == 0) {
                    cout << "No Used Buff: " << i << endl;
                    for (auto &expr:str2buffExpr[i]) {
                        expr->buff.flag = BUFF_NO_EFFECT;
                    }
                    str2buffExpr[i].clear();
                }
                cout << "Buff Invalid: " << i << endl;
                buffUsedCount.erase(i);
            }
        }
    }

    // 初始化
    void clear(bool inner = false) {
        buffUsedCount.clear();
        varAboutBuffs.clear();
        str2buffExpr.clear();
        if(!inner)
        buffs.clear();
    }

    // 清除所有没有使用过的缓存
    void clearUnusedBuff() {
        for (auto &i:buffUsedCount) {
            if (i.second == 0) {
                cout << "No Used Buff: " << i.first << endl;
                for (auto &expr:str2buffExpr[i.first]) {
                    expr->buff.flag = BUFF_NO_EFFECT;
                }
                str2buffExpr[i.first].clear();
            }
        }
    }

    // 给所有临时变量分配空间
    ExprAST *generateTempVarDeclare() {
        ExprAST *head = new NullExprAST, *p = head;
        for (auto &i:buffs) {
            p->next = new BinaryOpAST(OP_DECLARE, new VariableAST(tokenizer(i), true), nullptr);
            p = p->next;
        }
        return head->next;
    }
};

class BuffOptimizer {
public:
    stack<ExprAST *> exprStack;
    BuffTable buffTable;
    int ifState;

    BuffOptimizer():ifState(0){}

    // 启动函数
    void startFromHead(ExprAST *head) {
        cout << "==Start Optimize Sub Expression==" << endl;
        auto p = head;
        while (p) {
            switch (p->type) {
                default:
                    throw CompileERROR("not a block or func");
                case funcExpr:
                    start(((FuncPrototypeAST *) p)->code);
                    break;
                case blockExpr:
                    start(((BlockAST *) p)->code);
                    break;
            }
            p = p->next;
        }
        cout << "==End Optimize Sub Expression==" << endl;
    }

    void start(ExprAST *head) {
        while (!exprStack.empty())exprStack.pop();
        exprStack.push(head);
        startBlock();
        buffTable.clearUnusedBuff();
        auto li = buffTable.generateTempVarDeclare();
        if (li) {
            auto p = li;
            while(p->next)p = p->next;
            p->next = head->next;
            head->next = li;
            cout << "Created Buff Temp Variables" << endl;
        }
    }

    void debug(ExprAST *cur) {
        cout << expr2str(cur) << " ";
        vector<int> abouts = getAboutVars(cur);
        cout << "About Vars: ";
        for (auto &i:abouts) {
            cout << tokenizer[i] << " ";
        }
        cout << endl;
    }

    // 开始块级分析
    void startBlock() {
        ExprAST *cur;
        buffTable.clear();
        do {
            cur = nextCode();
            if (cur && (cur->type == binExpr || cur->type == arrayCallExpr)) {
                if (onlyVarOrConst(cur)) {
//                    debug(cur);
                }
                executeExpr(cur);
            }
        } while (cur);
    }

    // 主处理函数
    void executeExpr(ExprAST *expr) {
        if (!expr)return;
        if (expr->type == binExpr) {
            BinaryOpAST *bin = (BinaryOpAST *) expr;
            if (bin->op == OP_DECLARE || bin->op == OP_ASSIGN) {
                if (bin->RHS && (bin->RHS->type == binExpr || bin->RHS->type == arrayCallExpr))executeExpr(bin->RHS);
                if (bin->LHS && bin->LHS->type == varExpr) {
                    buffTable.eraseAbout(((VariableAST *) bin->LHS)->name);
                }else if(bin->LHS && bin->LHS->type == arrayCallExpr){
                    for(auto &i:((ArrayCallAST*)bin->LHS)->indexes){
                        executeExpr(i);
                    }
                }
                return;
            }
            if (bin->LHS && (bin->LHS->type == binExpr || bin->LHS->type == arrayCallExpr))executeExpr(bin->LHS);
            if (bin->RHS && (bin->RHS->type == binExpr || bin->RHS->type == arrayCallExpr))executeExpr(bin->RHS);
            if (!onlyVarOrConst(bin))return;
            buffTable.findBuff(bin, expr2str(bin), getAboutVars(bin));
        } else if (expr->type == arrayCallExpr) {
            ArrayCallAST *arr = (ArrayCallAST *) expr;
            for (auto &i:arr->indexes) {
                executeExpr(i);
            }
        }
    }

    // 获取下一个代码
    ExprAST *nextCode() {
        if (exprStack.empty())return nullptr;
        while (!exprStack.empty() && exprStack.top() == nullptr)exprStack.pop();
        if (exprStack.empty())return nullptr;
        auto cur = exprStack.top();
        switch (cur->type) {
            default:
                exprStack.pop();
                exprStack.push(cur->next);
                return cur;
                break;
            case ifExpr:
                exprStack.pop();
                exprStack.push(cur->next);
                buffTable.clear(true);
//                exprStack.push(((IfAST *) cur)->elseCode);
//                exprStack.push(((IfAST *) cur)->code);
//                exprStack.push(((IfAST *) cur)->expression);
                return nextCode();
            case elseExpr:
                exprStack.pop();
                exprStack.push(cur->next);
                exprStack.push(((ElseAST *) cur)->code);
                return nextCode();
            case whileExpr:
                exprStack.pop();
                exprStack.push(cur->next);
                buffTable.clear(true);
//                exprStack.push(((WhileAST *) cur)->code);
//                exprStack.push(((WhileAST *) cur)->expression);
                return nextCode();
            case blockExpr:
                exprStack.pop();
                exprStack.push(cur->next);
                exprStack.push(((BlockAST *) cur)->code);
                return nextCode();
            case returnExpr:
                exprStack.pop();
                exprStack.push(cur->next);
                exprStack.push(((ReturnAST*) cur)->res);
                return nextCode();
        }
    }

    // 将表达式转化为字符串
    string expr2str(ExprAST *expr) {
        if (!expr)return "NULL";
        static string split = "_";
        if (expr->value.with_value)return to_string(expr->value.value);
        switch (expr->type) {
            case binExpr:
                return "(" + op_to_str[((BinaryOpAST *) expr)->op] + split + expr2str(((BinaryOpAST *) expr)->LHS) +
                       split + expr2str(((BinaryOpAST *) expr)->RHS) + ")";
            case varExpr:
                return tokenizer[((VariableAST *) expr)->name];
            case nullExpr:
                return "$nullExpr$";
            case constExpr:
                return to_string(expr->value.value);
            default:
                return "$undefined$";
        }
    }

    void append(vector<int> &l, vector<int> &r) {
        l.insert(l.end(), r.begin(), r.end());
    }

    // 获取和表达式相关的变量
    vector<int> getAboutVars(ExprAST *expr) {
        vector<int> ret;
        if (!expr || expr->value.with_value)return ret;
        if (expr->type == binExpr) {
            vector<int> lhs = getAboutVars(((BinaryOpAST *) expr)->LHS);
            vector<int> rhs = getAboutVars(((BinaryOpAST *) expr)->RHS);
            append(ret, lhs);
            append(ret, rhs);
        } else if (expr->type == varExpr) {
            ret.push_back(((VariableAST *) expr)->name);
        } else {
            throw CompileERROR("only var or bin is allowed");
        }
        return ret;
    }

    // 判断表达式是否只由常数和变量组成
    bool onlyVarOrConst(ExprAST *exprAst) {
        if (!exprAst)return true;
        if (exprAst->value.with_value)return true;
        switch (exprAst->type) {
            case binExpr:
                return onlyVarOrConst(((BinaryOpAST *) exprAst)->LHS) && onlyVarOrConst(((BinaryOpAST *) exprAst)->RHS);
            case varExpr:
                return true;
            default:
                return false;
        }
    }
};