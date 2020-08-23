#include "AsmHelper.h"

void jmp_condition(int label){
    // 在if里getBool 然后JZ
    writer<<"BEQ Label_"<<label<<endl; //JZ
}

void jmp(int label){
    writer<<"B Label_"<<label<<endl;
}

void def_label(int label){
    writer<<"Label_"<<label<<":"<<endl;
}

void get_flag(){
    writer<<"ANDS R0,R0,R0"<<endl;
}

void alloc_stack(int diff){
    regCtrl.clear();
    if(diff*UNIT_SIZE<256){
        if(diff)writer<<"ADD R9,R9,#"<<diff*UNIT_SIZE<<endl;
    }else{
        writer<<"ldr r8,="<<diff*UNIT_SIZE<<endl;
        writer<<"ADD R9,R9,R8"<<endl;
    }
}

void free_stack(int diff){
    regCtrl.clear();
    if(diff*UNIT_SIZE<256){
        if(diff)writer<<"sub R9,R9,#"<<diff*UNIT_SIZE<<endl;
    }else{
        writer<<"ldr r8,="<<diff*UNIT_SIZE<<endl;
        writer<<"sub R9,R9,R8"<<endl;
    }
}

void load_constVal(int val){
    writer<<"ldr r0,="<<val<<endl;
}

void load_addrVal(int addr){
    if(addr>50){
        writer<<"ldr r8,="<<addr<<endl;
        writer<<"ldr r0,[r10,r8]"<<endl;
    }else{
        writer<<"ldr r0,[r9,#"<<addr<<"]"<<endl;
    }
}

void push_r0(){
    writer<<"push {r0}"<<endl;
}

void push_r1(){
    writer<<"push {r1}"<<endl;
}

void push_r11(){
    writer<<"push {r11}"<<endl;
}

void push_r12(){
    writer<<"push {r12}"<<endl;
}

void pop_r0(){
    writer<<"pop {r0}"<<endl;
}

void pop_r1(){
    writer<<"pop {r1}"<<endl;
}

void pop_r11(){
    writer<<"pop {r11}"<<endl;
}

void push_bp(){
    writer<<"push {r9}"<<endl;
}

void pop_r12(){
    writer<<"pop {r12}"<<endl;
}

void pop_bp(){
    writer<<"pop {r9}"<<endl;
}

void get_sign(){ // return a<0
    shr(31);
}

void get_not(){ // return !a
    get_flag();
    writer<<"ITE EQ"<<endl;
    writer<<"moveq r0,#1"<<endl;
    writer<<"movne r0,#0"<<endl;
}

void get_neg(){
    writer<<"mvn r0,r0"<<endl;
    writer<<"add r0,#1"<<endl;
}

void get_bool(){ // a-> 0/1
    get_flag();
    writer<<"ITE NE"<<endl;
    writer<<"movne r0,#1"<<endl;
    writer<<"moveq r0,#0"<<endl;
}

void shl(int step){
    if(step!=0)writer<<"mov r0,r0,lsl #"<<step<<endl;
}

void shr(int step){
    if(step!=0)writer<<"mov r0,r0,asr #"<<step<<endl;
}

void getChar(){
    writer<<"push {r1,r2,r3,r4,lr}"<<endl;
    writer<<"bl getchar"<<endl;
    writer<<"pop {r1,r2,r3,r4,lr}"<<endl;
}

void putChar(){
    writer<<"push {r1,r2,r3,r4,lr}"<<endl;
    writer<<"bl putchar"<<endl;
    writer<<"pop {r1,r2,r3,r4,lr}"<<endl;
}

void call_func(int varDiff, const string&func_name){
    regCtrl.clear();

    writer<<"push {r9}"<<endl;
    alloc_stack(varDiff);

    // call dest func
    writer<<"push {lr}"<<endl;
    writer<<"bl FUNC_"<<func_name<<endl;
    writer<<"pop {lr}"<<endl;

    writer<<"pop {r9}"<<endl;

    regCtrl.clear(true);
}

void func_declare(const string&name){
    writer<<endl<<"@-------- Func "<<name<<" start --------"<<endl;
    writer<<"FUNC_"<<name<<":"<<endl;
    writer<<"push {r12}"<<endl;
}

void func_declare_ends(const string&name){
    regCtrl.clear();
    writer<<"pop {r12}"<<endl;
    writer<<"mov pc,lr"<<endl;
    writer<<".pool"<<endl;
    writer<<"@-------- Func "<<name<<" end --------"<<endl<<endl;
}

void transToTemp(){
    writer<<"MOV r1,r0"<<endl;
}

void add(){
    writer<<"ADD r0,r0,r1"<<endl;
}

void sub(){
    writer<<"SUB r0,r0,r1"<<endl;
}

void mul(){
    writer<<"MUL r0,r0,r1"<<endl;
}

void div(){
    writer<<"push {r1,r2,r3,r4,r12,lr}"<<endl;
    writer<<"bl __aeabi_idiv(PLT)"<<endl;
    writer<<"pop {r1,r2,r3,r4,r12,lr}"<<endl;
}

void getint(){
    writer<<"push {r1,r2,r3,r4,lr}"<<endl;
    writer<<"bl getint"<<endl;
    writer<<"pop {r1,r2,r3,r4,lr}"<<endl;
}

void putint(){
    writer<<"push {r1,r2,r3,r4,lr}"<<endl;
    writer<<"bl putint"<<endl;
    writer<<"pop {r1,r2,r3,r4,lr}"<<endl;
}

void _sysy_starttime(){
    writer<<"push {r1,r2,r3,r4,lr}"<<endl;
    writer<<"bl _sysy_starttime"<<endl;
    writer<<"pop {r1,r2,r3,r4,lr}"<<endl;
}

void _sysy_stoptime(){
    writer<<"push {r1,r2,r3,r4,lr}"<<endl;
    writer<<"bl _sysy_stoptime"<<endl;
    writer<<"pop {r1,r2,r3,r4,lr}"<<endl;
}

void clear_ax(){
    writer<<"mov r0,#0"<<endl;
}

string get_cond(ExprAST* node){
    assert(node->type==binExpr);
    switch(((BinaryOpAST*)node)->op){
        case OP_LESS:
            return "lt";
        case OP_BIGGER:
            return "gt";
        case OP_EQUAL:
            return "eq";
        case OP_LESS_OR_EQUAL:
            return "le";
        case OP_BIGGER_OR_EQUAL:
            return "ge";
        case OP_NOT_EQUAL:
            return "ne";
        case OP_AND:
            return "ne";
        case OP_OR:
            return "eq";
        default:
            throw CompileERROR("Expected condition expression not satisfied.");
    }
}

string get_not_cond(ExprAST* node){
    assert(node->type==binExpr);
    switch(((BinaryOpAST*)node)->op){
        case OP_LESS:
            return "ge";
        case OP_BIGGER:
            return "le";
        case OP_EQUAL:
            return "ne";
        case OP_LESS_OR_EQUAL:
            return "gt";
        case OP_BIGGER_OR_EQUAL:
            return "lt";
        case OP_NOT_EQUAL:
            return "eq";
        case OP_AND:
            return "eq";
        case OP_OR:
            return "ne";
        default:
            throw CompileERROR("Expected condition expression not satisfied.");
    }
}
