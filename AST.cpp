#include "AST.h"

map<OP,string> op_to_str;

int getLabelID(){
    static int count=0;
    return count++;
}

int calculate(OP op, ExprAST *lhs, ExprAST *rhs) {
    switch (op){
        default: E("error op")
        case OP_ADD: return lhs->value.value + rhs->value.value;
        case OP_MINUS:return lhs->value.value - rhs->value.value;
        case OP_MUL:return lhs->value.value * rhs->value.value;
        case OP_DIV:return lhs->value.value / rhs->value.value;
        case OP_ASSIGN:return lhs->value.value = rhs->value.value;
        case OP_EQUAL:return lhs->value.value == rhs->value.value;
        case OP_LESS:return lhs->value.value < rhs->value.value;
        case OP_DECLARE:return lhs->value.value = rhs->value.value;
        case OP_AND: return lhs->value.value && rhs->value.value;
        case OP_OR: return lhs->value.value || rhs->value.value;
        case OP_BIGGER: return lhs->value.value > rhs->value.value;
        case OP_LESS_OR_EQUAL: return lhs->value.value <= rhs->value.value;
        case OP_BIGGER_OR_EQUAL: return lhs->value.value >= rhs->value.value;
        case OP_NOT_EQUAL: return lhs->value.value != rhs->value.value;
        case OP_MOD: return lhs->value.value % rhs->value.value;
    }
}

int calculate(OP op, ExprAST *rhs) {
    switch (op) {
        default: E("error single tokenizer")
        case OP_NOT: return !rhs->value.value;
        case OP_MINUS : return -rhs->value.value;
        case OP_SINGLE_MINUS : return -rhs->value.value;
    }
}

ExprAST::ExprAST(NodeType type) :type(type),next(nullptr),value(0, false),back(nullptr),buff(BUFF_NO_EFFECT, -1){
    vi_id = 0;
}

ExprAST::ExprAST(NodeType type,int v) :type(type), next(nullptr),value(v, true),back(nullptr),buff(BUFF_NO_EFFECT, -1){
    vi_id = 0;
}

void ExprAST::bind(ExprAddr* node){
    forward.push_back(node);
}

NullExprAST::NullExprAST():ExprAST(nullExpr){
//    vi_id = vi.allocate("head");
}

ConstExprAST::ConstExprAST(int value) : ExprAST(constExpr,value){
//    vi_id = vi.allocate(to_string(value));
}

VariableAST::VariableAST(int name,bool isNewVar) :ExprAST(varExpr), name(name),isNewVar(isNewVar),addr(-10000000){
    binLabel=getLabelID();
}

BinaryOpAST::BinaryOpAST(OP op, ExprAST*lhs, ExprAST*rhs) :ExprAST(binExpr), op(op), LHS(lhs), RHS(rhs){
    if(this->op == OP_MINUS && this->LHS == nullptr){
        this->op = OP_SINGLE_MINUS;
    }
    binLabel=getLabelID();
//    vi_id = vi.allocate(op_to_str[op]);
#ifdef CONSTANT_FOLDING
    if(lhs && rhs && lhs->value.with_value &&rhs->value.with_value){
//        if(lhs)vi.associate(vi_id, lhs->vi_id,"","red");
//        if(rhs)vi.associate(vi_id, rhs->vi_id,"","red");
        value.value=calculate(op, LHS, RHS);
        value.with_value=true;
        delete LHS;
        delete RHS;
        LHS=nullptr;
        RHS=nullptr;
    }else if(!lhs && rhs && rhs->value.with_value){
//        if(rhs)vi.associate(vi_id, rhs->vi_id,"","red");
        value.value = calculate(op, RHS);
        value.with_value = true;
        delete RHS;
        RHS = nullptr;
    }else{
//        if(lhs)vi.associate(vi_id, lhs->vi_id);
//        if(rhs)vi.associate(vi_id, rhs->vi_id);
    }
#else
//    if(lhs)vi.associate(vi_id, lhs->vi_id);
//    if(rhs)vi.associate(vi_id, rhs->vi_id);
#endif

     if(lhs) bind(&LHS);
     if(rhs) bind(&RHS);
}

IfAST::IfAST(ExprAST*expression, ExprAST*code) : ExprAST(ifExpr),expression(expression), code(code),elseCode(nullptr){
    bind(&this->expression);
    bind(&this->code);
//    vi_id = vi.allocate("if");
//    vi.associate(vi_id, expression->vi_id,"expression");
//    vi.associate(vi_id, code->vi_id,"code");
}

ElseAST::ElseAST(ExprAST*code) :ExprAST(elseExpr), code(code){
    bind(&this->code);
//    vi_id = vi.allocate("else");
//    vi.associate(vi_id, code->vi_id,"code");
}

WhileAST::WhileAST(ExprAST*expression, ExprAST*code) :ExprAST(whileExpr), expression(expression), code(code){
    bind(&this->expression);
    bind(&this->code);
//    vi_id = vi.allocate("while");
//    vi.associate(vi_id, expression->vi_id,"expression");
//    vi.associate(vi_id, code->vi_id,"code");
}

FuncPrototypeAST::FuncPrototypeAST(int name, ExprAST*args, ExprAST*code, ReturnType type) : ExprAST(funcExpr), name(name), args(args), code(code), retType(type){
    bind(&this->args);
    bind(&this->code);
//    vi_id = vi.allocate("func: " + tokenizer[name]);
//    vi.associate(vi_id, args->vi_id,"args");
//    vi.associate(vi_id, code->vi_id,"code");
}

FuncArgAST::FuncArgAST(int name, ArrayArg array) : ExprAST(funcArg), name(name), array(array){
//    vi_id = vi.allocate("arg: " + res + tokenizer[name]);
}

ReturnAST::ReturnAST(ExprAST*res) :ExprAST(returnExpr), res(res){
    bind(&this->res);
//    vi_id = vi.allocate("return");
//    if(res)vi.associate(vi_id, res->vi_id,"res");
}

CallAST::CallAST(int name, ExprAST*args) :ExprAST(callExpr), name(name), args(args){
    bind(&this->args);
//    vi_id = vi.allocate("call: " + tokenizer[name]);
//    vi.associate(vi_id, args->vi_id,"args");
}

ArrayPrototypeAST::ArrayPrototypeAST(ExprAST* variable,vector<ExprAST*> dimensions) : ExprAST(arrayPrototypeExpr), dimensions(dimensions),variable(variable) {
//    vi_id = vi.allocate("array");
//    vi.associate(vi_id, length->vi_id,"length");
initValues=nullptr;
}


int Parser::next(){
    tokenizer>>cur;
#ifdef DEBUG_
    cout << "Parser: curStr = " << curStr << endl;
#endif
    return cur;
}

int Parser::to_int(const string&str){
    int res=0;
    for(int i=0; i<str.length(); i++)res=res*10+(str[i]-'0');
    return res;
}

void Parser::need(const string&str){
    if(curStr!=str)E("need "+str)
}

ExprAST*Parser::ParseConst(){
    if(tokenizer.type(cur)==TYPE_NUM){
        int last=cur;
        next();
        auto res=new ConstExprAST(to_int(tokenizer[last]));
        return res;
    }
    E("need num")
}

ExprAST*Parser::ParseVariable(bool isNewVar){
    if(tokenizer.type(cur)==TYPE_SYMBOL){
        int name=cur;
        next();
        if(curStr == "("){
            next();
            return ParseCall(name);
        }else if(curStr == "["){
            return ParseArrayCall(name);
        }else{
            return new VariableAST(name, isNewVar);
        }
    }
    E("need variable")
}

int Parser::get_precedence(int id,bool is_single){
    if(is_single && tokenizer[id] == "-")return 120;
    if(!op_precedence.count(tokenizer[id]))return -1;
    return op_precedence[tokenizer[id]];
}

OP Parser::id_to_op(int id){
    string str=tokenizer[id];
    if(ops.count(str))return ops[str];
    E("error op")
}

ExprAST*Parser::ParsePrimary(){
    if(tokenizer.type(cur)==TYPE_SYMBOL)return ParseVariable(false);
    if(tokenizer.type(cur)==TYPE_NUM)return ParseConst();
    if(curStr == "!" || curStr == "~" || curStr == "-")return ParseSingleOp();
    if(curStr=="(")return ParseParenExpr();
    if(curStr==";")return nullptr;
    if(curStr==")"){
        next();
        return nullptr;
    }
    if(curStr=="]")return nullptr;
    ET()
}

ExprAST*Parser::ParseParenExpr(){
    next();
    ExprAST*V=ParseExpression();
    if(!V)return 0;
    if(curStr!=")")E("no )")
    next();
    return V;
}

ExprAST*Parser::ParseExpression(int prec){
    ExprAST*LHS=ParsePrimary();
    if(!LHS)return nullptr;
    return ParseBinOpRHS(prec, LHS);
}

ExprAST*Parser::ParseBinOpRHS(int exprPrec, ExprAST*LHS,bool is_single){
    while(1){
        int prec=get_precedence(cur,is_single);
        if(prec<exprPrec)return LHS;
        int BinOp=cur;
        next();
        ExprAST*RHS=ParsePrimary();
        if(!RHS)return nullptr;
        int nextPrec=get_precedence(cur);
        if(prec<nextPrec){
            RHS=ParseBinOpRHS(prec+1, RHS);
            if(!RHS)return nullptr;
        }
        LHS=new BinaryOpAST(id_to_op(BinOp), LHS, RHS);
    }
}

ExprAST* Parser::ParseVar(bool is_const){
    ExprAST* LHS = nullptr;
    LHS = ParseVariable(true);
    LHS->value.is_const = is_const;
    if(curStr == "="){
        next();
        ExprAST* RHS = ParseExpression();
        LHS = new BinaryOpAST(OP_DECLARE, LHS, RHS);
        LHS->value.is_const = is_const;
    }else{
        LHS = new BinaryOpAST(OP_DECLARE, LHS, nullptr);
    }
    if(curStr == ";"){
        next();
    }else if(curStr == ","){
        next();
        ExprAST* NXT = ParseVarOrArrayPrototype(is_const);
        LHS->next = NXT;
    }
    return LHS;
}

ExprAST*Parser::ParseIf(){
    ExprAST*expr=ParseExpression();
    if(curStr=="{"){
        next();
        ExprAST*code=ParseCode();
        return new IfAST(expr, code);
    }else{
        ExprAST*code = ParseCode(true);
        if(curStr == ";")next();
        return new IfAST(expr, code);
    }

}

ExprAST*Parser::ParseElse(){
    bool oneLine = true;
    if(curStr == "{"){
        next();
        oneLine = false;
    }
    ExprAST*code=ParseCode(oneLine);
    return new ElseAST(code);
}

ExprAST*Parser::ParseWhile(){
    ExprAST*expr=ParseExpression();
    if(curStr=="{"){
        next();
        ExprAST*code=ParseCode();
        return new WhileAST(expr, code);
    }else{
        ExprAST*code=ParseCode(true);
        return new WhileAST(expr, code);
    }
}

ExprAST*Parser::ParseFuncPrototype(FuncPrototypeAST::ReturnType type){
    ExprAST*args=new NullExprAST, *p=args;
    if(tokenizer.type(cur)!=TYPE_SYMBOL)E("need variable name")
    int name=cur;
    next();
    need("("), next();
    while(curStr!=")"){
        need("int"), next();
        if(tokenizer.type(cur)!=TYPE_SYMBOL)E("need variable name")
        int argName = cur;
        ArrayArg arrayArg(false);
        next();
        if(curStr == "["){
            arrayArg.dimensions = ParseArrayDimension();
            arrayArg.isArray = true;
        }
        FuncArgAST*arg =new FuncArgAST(argName, arrayArg);
//        vi.associate(p->vi_id, arg->vi_id,"arg");
        p->next=arg;
        if(p->next){
            p->next->back = p;
        }
        p=p->next;
        if(curStr==")")break;
        next();
        if(curStr == ",")next();
    }
    next();
    need("{"), next();
    ExprAST*code=ParseCode();
    return new FuncPrototypeAST(name, args, code, type);
}

ExprAST*Parser::ParseReturn(){
    ExprAST*res=ParseExpression();
    return new ReturnAST(res);
}

ExprAST*Parser::ParseCall(int name){
    ExprAST*args=new NullExprAST, *p=args;
    int argCnt = 0;
    while(curStr!=")"){
        ExprAST*exprAst=ParseExpression();
//        vi.associate(p->vi_id, exprAst->vi_id, "arg");
        p->next=exprAst;
        if(p->next){
            p->next->back = p;
        }
        p=p->next;
        argCnt ++;
        if(curStr==",")next();
        else if(curStr==")" || curStr==";")break;
    }
    next();
    return new CallAST(name, args);
}
//#define ASS vi.associate(p->vi_id, mid->vi_id,"next");p->next=mid;
#define ASS p->next=mid;if(p->next)p->next->back=p;
ExprAST*Parser::ParseCode(bool oneLine){
    ExprAST*pHead=new NullExprAST();
    ExprAST*p=pHead;
    while(curStr!="EOF"){
        if(curStr=="int"){
            next();
            ExprAST* mid;
            int judge = tokenizer.forward(1);
            if(tokenizer[judge] == "("){
                mid = ParseFuncPrototype(FuncPrototypeAST::RET_INT);
            }else if(tokenizer[judge] == "["){
                mid = ParseArrayPrototype();
            } else{
                mid = ParseVar();
            }
            ASS
        } else if(curStr == "void"){
            next();
            ExprAST* mid = ParseFuncPrototype(FuncPrototypeAST::RET_VOID);
            ASS
        } else if(curStr=="if"){
            next();
            ExprAST* mid = ParseIf();
            ASS
        } else if(curStr=="else"){
            next();
            ExprAST* mid =ParseElse();
            findMatchedIf(p, mid);
            continue;
//            ASS
        } else if(curStr=="while"){
            next();
            ExprAST* mid =ParseWhile();
            ASS
        }else if(curStr=="return"){
            next();
            ExprAST* mid =ParseReturn();
            ASS
        } else if(curStr=="break"){
            next();
            ExprAST* mid = new BreakAST(breakExpr);
            ASS
        } else if(curStr == "continue"){
            next();
            ExprAST* mid = new ContinueAST(continueExpr);
            ASS
        } else if(curStr == "const"){
            next();
            need("int"),next();
            ExprAST* mid;
            int judge = tokenizer.forward(1);
            if(tokenizer[judge] == "("){
                mid = ParseFuncPrototype(FuncPrototypeAST::RET_INT);
            }else if(tokenizer[judge] == "["){
                mid = ParseArrayPrototype(true);
            } else{
                mid = ParseVar(true);
            }
            ASS
        } else if(curStr=="}"){
            next();
            break;
        } else if(curStr==";" || curStr == ","){
            next();
            continue;
        } else if(curStr == "{"){
            next();
            ExprAST *code = ParseCode();
            ExprAST *mid = new BlockAST(code);
            ASS
        } else{
            ExprAST* mid = ParseExpression();
            ASS
        }
        if(p->next==nullptr)break;
        while(p->next){
            p=p->next;
        }
        if(oneLine){
//            if(curStr == ";" && tokenizer[tokenizer.forward(1)] == "else"){
//                next();
//                next();
//                ExprAST* mid =ParseElse();
//                findMatchedIf(p, mid);
//            }
            if(curStr == "else"){
                next();
                ExprAST* mid =ParseElse();
                findMatchedIf(p, mid);
            }
            return pHead;
        }
    }
    return pHead;
}

ExprAST*Parser::ParseStart(){
    next();
    head=ParseCode();
    fixGlobalVar();
    return head;
}

ExprAST *Parser::ParseSingleOp() {
    return ParseBinOpRHS(get_precedence(cur,true), nullptr,true);
}

void Parser::init() {
//    vector<pair<string, int>> precs = {{"=",5},{"<", 10},{">", 10},{">=", 10},{"<=", 10},{"!=", 10},{"||", 10},{"&&", 10},{"^", 10},{"|",10},{"~",60},{"<<",10},{">>", 10},{"==", 20},{"+", 40},{"-", 40},{"*", 50},{"/", 50},{"%", 50},{"!", 60},{"&",60},{"$",60},{"@",60}};
    vector<pair<string, int>> precs = {{"=", 10}, {"||", 20}, {"&&", 30}, {"|", 40}, {"^", 50}, {"&", 60}, {"!=", 70}, {"==", 70}, {"<=", 80}, {">=", 80}, {"<", 80}, {">", 80}, {"<<", 90}, {">>", 90}, {"+", 100}, {"-", 100}, {"/", 110}, {"*", 110}, {"%", 110}, {"!", 120}, {"&", 120}, {"!", 120}};
    for(auto& p:precs)op_precedence.insert(p);
    vector<pair<string, OP>> opss = {{"+",OP_ADD},{"-",OP_MINUS},{"/",OP_DIV},{"*",OP_MUL},{"=",OP_ASSIGN},{"==",OP_EQUAL},{"<",OP_LESS},{"!",OP_NOT},{">", OP_BIGGER},{"||", OP_OR},{"&&", OP_AND},{">=", OP_BIGGER_OR_EQUAL},{"<=", OP_LESS_OR_EQUAL},{"!=", OP_NOT_EQUAL},{"%", OP_MOD}};
    for(auto& p:opss)ops.insert(p);
    vector<pair<OP,string>> opstrs = {{OP_ADD, "+"},{OP_MINUS,"-"},{OP_MUL,"*"},{OP_DIV,"/"},{OP_NOT_EQUAL,"!="},{OP_LESS_OR_EQUAL,"<="},{OP_LESS,"<"},{OP_BIGGER_OR_EQUAL,">="},{OP_BIGGER,">"},{OP_ASSIGN,"="},{OP_EQUAL,"=="},{OP_MOD,"%"},{OP_OR,"||"},{OP_NOT,"!"},{OP_DECLARE,"="}};
    for(auto& p:opstrs)op_to_str.insert(p);
}

Parser::Parser() {init();}

vector<ExprAST *> Parser::ParseArrayDimension() {
    vector<ExprAST*> res;
    while(curStr == "["){
        next();
        ExprAST* a = ParseExpression();
        res.push_back(a);
        if(tokenizer[tokenizer.forward(1)] == "[")next();
    }
    if(curStr == "]"){
        next();
    }
    return res;
}

ExprAST *Parser::ParseArrayCall(int name) {
    ExprAST* var = new VariableAST(name, false);
    vector<ExprAST*> v = ParseArrayDimension();
//    if(curStr == "]"){
//        next();
//    }
    return new ArrayCallAST(arrayCallExpr, var, v);
}

ExprAST *Parser::ParseArrayPrototype(bool is_const) {
    ExprAST* variable = new VariableAST(cur, true);
    variable->value.is_const = is_const;
    next();
    vector<ExprAST*> v = ParseArrayDimension();
    ArrayPrototypeAST *res;
//    if(curStr == "]")next();
    if(curStr == "="){
        next();
        ExprAST* initialList = ParseInitialList();
        res = new ArrayPrototypeAST(variable, v);
        res->initValues = initialList;
        res->value.is_const = is_const;
        return res;
    }else if(curStr == ";"){
        next();
        res = new ArrayPrototypeAST(variable, v);
        res->value.is_const = is_const;
        return res;
    }else if(curStr == ","){
        next();
        res = new ArrayPrototypeAST(variable, v);
        res->value.is_const = is_const;
        res->next = ParseVarOrArrayPrototype(is_const);
        return res;
    }
    E("throw an exception when parse ArrayPrototype")
}

ExprAST *Parser::ParseInitialList() {
    ExprAST* pHead = new NullExprAST, *p = pHead;
    need("{"), next();
    while(curStr != "}"){
        ExprAST* value ;
        if(curStr == "{"){
            value = ParseInitialList();
        }else{
            value = ParseExpression();
        }
        if(curStr == ",")next();
        p->next = value;
        if(!p->next)break;
        p->next->back = p;
        p = p->next;
    }
    next();
    return new InitialListAST(pHead);
}

void Parser::findMatchedIf(ExprAST *p, ExprAST *elseA) {
    ExprAST *t = p;
    bool matchedIf = false;
    while(t){
        if(t->type == ifExpr){
            ((IfAST*)t)->elseCode = elseA;
            ((IfAST*)t)->bind(&((IfAST*)t)->elseCode);
            matchedIf = true;
            break;
        }
        t = t->back;
    }
    if(!matchedIf){
        E("else can't find if")
    }
}

ExprAST *Parser::ParseVarOrArrayPrototype(bool is_const) {
    int forward = tokenizer.forward(1);
    if(tokenizer[forward] == "[")return ParseArrayPrototype(is_const);
    else return ParseVar(is_const);
}


ArrayCallAST::ArrayCallAST(NodeType type, ExprAST *variable,vector<ExprAST*> dimensions) : ExprAST(type), variable(variable),
                                                                                           indexes(dimensions) {
    for(auto&i:indexes){
        bind(&i);
    }
}

BreakAST::BreakAST(NodeType type) : ExprAST(type) {

}

ContinueAST::ContinueAST(NodeType type) : ExprAST(type) {

}

InitialListAST::InitialListAST(ExprAST *head) :ExprAST(initialListExpr),head(head){

}

void eraseEmpty(ExprAST* node,ExprAddr* from){
    if (node){
        if(node->type==nullExpr&&from){
            *from=node->next;
            eraseEmpty(*from,from);
            return;
        }
        for(auto i:node->forward){
            if(i)eraseEmpty(*i,i);
        }
        eraseEmpty(node->next,&node->next);
    }
}
