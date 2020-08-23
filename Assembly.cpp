#include "Assembly.h"

int lastFuncRetLabel;
int label_tmp;

#define PL label_tmp=getLabelID(),jmp(label_tmp),writer<<".pool"<<endl,def_label(label_tmp);

vector<pair<string, int>> blocks;

RegType loadVar(ExprAST*astNode, bool is_Silent){
    assert(astNode->type==varExpr);
    auto*node=(VariableAST*) (astNode);
    if(is_Silent){
        return regCtrl.load(node->name, node->addr, true);
    } else{
        return regCtrl.load_silent(node->name, node->addr, true);
    }
}

void deal_conditions(ExprAST*astNode, int successLabel, int failLabel){

    if(astNode->type==varExpr){
        writer<<"Label_"<<((VariableAST*) astNode)->binLabel<<":"<<endl;
        auto reg=loadVar(astNode);
        writer<<"ands "<<reg<<","<<reg<<","<<reg<<endl;
        writer<<"bne "<<" Label_"<<successLabel<<endl;
        writer<<"beq "<<" Label_"<<failLabel<<endl;
        return;
    }
    assert(astNode->type==binExpr);
    auto node=(BinaryOpAST*) astNode;
    if(node->op==OP_AND){
        int right_son_left=get_left_son(node->RHS);
        deal_conditions(node->LHS, right_son_left, failLabel);
        deal_conditions(node->RHS, successLabel, failLabel);
    } else if(node->op==OP_OR){
        int right_son_left=get_left_son(node->RHS);
        deal_conditions(node->LHS, successLabel, right_son_left);
        deal_conditions(node->RHS, successLabel, failLabel);
    } else if(node->op==OP_NOT){
        deal_conditions(node->RHS, failLabel, successLabel);
    } else if(node->op==OP_LESS||node->op==OP_LESS_OR_EQUAL||node->op==OP_NOT_EQUAL||node->op==OP_EQUAL||
              node->op==OP_BIGGER||node->op==OP_BIGGER_OR_EQUAL){

        writer<<"Label_"<<node->binLabel<<":"<<endl;

        // expand comparison
        if(node->value.with_value){
            if(node->value.value){
                writer<<"b"<<" Label_"<<successLabel<<endl;
            } else{
                writer<<"b"<<" Label_"<<failLabel<<endl;
            }
        } else if(node->LHS->type==varExpr&&node->RHS->value.with_value){
            if(node->RHS->value.value<256){
                auto reg=loadVar(node->LHS);
                writer<<"cmp "<<reg<<",#"<<node->RHS->value.value<<endl;
            } else{
                auto reg=loadVar(node->LHS);
                writer<<"ldr r12,="<<node->RHS->value.value<<endl;
                writer<<"cmp "<<reg<<",r12"<<endl;
            }
        } else if(node->LHS->type==varExpr&&node->RHS->type==varExpr){
            auto reg_L=loadVar(node->LHS);
            auto reg_R=loadVar(node->RHS);
            writer<<"cmp "<<reg_L<<","<<reg_R<<endl;
        } else if(node->LHS->type==varExpr){
            _generateAsm(node->RHS, r0);
            auto reg_L=loadVar(node->LHS);
            writer<<"cmp "<<reg_L<<",r0"<<endl;
        }else if(node->RHS->type==varExpr){
            _generateAsm(node->LHS, r0);
            auto reg_R=loadVar(node->RHS);
            writer<<"cmp r0,"<<reg_R<<endl;
        }else{
            push_r1();
            _generateAsm(node->RHS, r1);
            _generateAsm(node->LHS, r0);
            writer<<"cmp r0,r1"<<endl;
            pop_r1();
        }

        writer<<"b"<<get_cond(node)<<" Label_"<<successLabel<<endl;
        writer<<"b"<<get_not_cond(node)<<" Label_"<<failLabel<<endl;

    }
}

int get_left_son(ExprAST*astNode){
    if(astNode->type==varExpr){
        return ((VariableAST*) astNode)->binLabel;
    }
    assert(astNode->type==binExpr);
    auto node=(BinaryOpAST*) astNode;
    if(node->op==OP_AND||node->op==OP_OR){
        return get_left_son(node->LHS);
    }
    if(node->op==OP_LESS||node->op==OP_LESS_OR_EQUAL||node->op==OP_NOT_EQUAL||node->op==OP_EQUAL||node->op==OP_BIGGER||
       node->op==OP_BIGGER_OR_EQUAL){
        return node->binLabel;
    }
    throw CompileERROR("Condition expression analysis failed: can't find left son.");
}

void generateAsm(ExprAST*astNode){
    // header
    writer<<".extern memset, malloc, getchar, putchar\n"
            ".data\n";
    writer<<"dt: .rept "<<totalStackSize<<"\n";
    writer<<".long 0\n"
            ".endr\n"
            ".bss\n"
            "stack: .rept 2000000\n"
            ".long 0\n"
            ".endr\n"
            ".text\n"
            ".global main\n"
            "\n"
            "@-------- Func INIT_GLOBALS start --------\n"
            "FUNC_INIT_GLOBALS:\n";
    _generateAsm(((BlockAST*) astNode)->code);
    regCtrl.clear();
    writer<<"mov pc,lr\n"
            ".pool\n"
            "@-------- Func INIT_GLOBALS end --------\n"
            "\n"
            "start:\n"
            "\n";
    _generateAsm(((BlockAST*) astNode)->next);
    // ending
    writer<<"\n@ generate part end\n"
            "\n"
            "ust: .word dt\n"
            "sysst: .word stack\n"
            "main:\n";
    if(largeMem){
        writer<<"ldr r0,=500000000\n"
                "push {lr}\n"
                "bl malloc\n"
                "mov r9, r0\n"
                "pop {lr}\n";
    } else{
        writer<<"ldr r9,ust\n";
    }
    writer<<"ldr sp,sysst\n"
            "\n"
            "ldr r8,=2000000\n"
            "add sp,sp,r8\n"
            "mov r10,r9\n"
            "ldr r8,="+to_string(MAIN_FUNC_SHIFTING*UNIT_SIZE)+
            "\n"
            "add r9,r9,r8\n"

            "push {lr}\n"
            "bl FUNC_INIT_GLOBALS\n"
            "pop {lr}\n"

            "push {lr}\n"
            "bl FUNC_main\n"
            "pop {lr}"
            "\n"
            "bx lr\n"
            "\n"
            ".end\n";
}

void _generateAsm(ExprAST*astNode, RegType target, bool once){
    while(astNode){
        if(astNode->buff.flag==BUFF_POP){
            auto reg=regCtrl.load(astNode->buff.name, astNode->buff.addr, true);
            writer<<"mov "<<target<<","<<reg<<endl;
            astNode=astNode->next;
            continue;
        }
        switch(astNode->type){
            case nullExpr:{
                break;
            }
            case constExpr:{
                // load Num
                auto*node=(ConstExprAST*) (astNode);
                if(!node->value.with_value){
                    throw CompileERROR("Inner error: Const expr node with \"with_val=false\" ");
                }
                writer<<"ldr "<<target<<",="<<node->value.value<<endl;
                break;
            }
            case varExpr:{
                auto*node=(VariableAST*) (astNode);
                auto reg=regCtrl.load(node->name, node->addr, true);
                writer<<"mov "<<target<<","<<reg2str(reg)<<endl;
                break;
            }
            case blockExpr:{
                auto*node=(BlockAST*) (astNode);
                blocks.push_back({"block", node->varCount});
                alloc_stack(node->varCount);
                _generateAsm(node->code);
                free_stack(node->varCount);
                blocks.pop_back();
                break;
            }
            case binExpr:{
                auto*node=(BinaryOpAST*) (astNode);
                if(node->value.with_value){
                    writer<<"ldr "<<target<<",="<<node->value.value<<endl;
                    break;
                }
                switch(node->op){
                    case OP_ADD:{
                        if(node->LHS->value.with_value&&node->RHS->value.with_value){
                            int res=node->LHS->value.value+node->RHS->value.value;
                            writer<<"ldr "<<target<<",="<<res<<endl;
                        } else if(node->LHS->type==varExpr&&node->RHS->value.with_value){
                            if(node->RHS->value.value<256){
                                auto reg=loadVar(node->LHS);
                                if(node->RHS->value.value)writer<<"add "<<target<<","<<reg<<",#"<<node->RHS->value.value<<endl;
                            } else{
                                auto reg=loadVar(node->LHS);
                                writer<<"ldr r12,="<<node->RHS->value.value<<endl;
                                writer<<"add "<<target<<","<<reg<<",r12"<<endl;
                            }
                        } else if(node->RHS->type==varExpr&&node->LHS->value.with_value){
                            if(node->LHS->value.value<256){
                                auto reg=loadVar(node->RHS);
                                if(node->LHS->value.value)writer<<"add "<<target<<","<<reg<<",#"<<node->LHS->value.value<<endl;
                            } else{
                                auto reg=loadVar(node->RHS);
                                writer<<"ldr r12,="<<node->LHS->value.value<<endl;
                                writer<<"add "<<target<<","<<reg<<",r12"<<endl;
                            }
                        } else if(node->LHS->type==varExpr&&node->RHS->type==varExpr){
                            auto reg1=loadVar(node->LHS);
                            auto reg2=loadVar(node->RHS);
                            writer<<"add "<<target<<","<<reg1<<","<<reg2<<endl;
                        } else if(node->LHS->value.with_value){
                            if(node->LHS->value.value<256){
                                if(target!=r1)push_r1();
                                _generateAsm(node->RHS, r1);
                                if(node->LHS->value.value)writer<<"add "<<target<<","<<r1<<",#"<<node->LHS->value.value<<endl;
                                if(target!=r1)pop_r1();
                            } else{
                                if(target!=r1)push_r1();
                                _generateAsm(node->RHS, r1);
                                writer<<"ldr r12,="<<node->LHS->value.value<<endl;
                                writer<<"add "<<target<<","<<r1<<","<<r12<<endl;
                                if(target!=r1)pop_r1();
                            }
                        } else if(node->RHS->value.with_value){
                            if(node->RHS->value.value<256){
                                if(target!=r1)push_r1();
                                _generateAsm(node->LHS, r1);
                                if(node->RHS->value.value)writer<<"add "<<target<<","<<r1<<",#"<<node->RHS->value.value<<endl;
                                if(target!=r1)pop_r1();
                            } else{
                                if(target!=r1)push_r1();
                                _generateAsm(node->LHS, r1);
                                writer<<"ldr r12,="<<node->RHS->value.value<<endl;
                                writer<<"add "<<target<<","<<r1<<","<<r12<<endl;
                                if(target!=r1)pop_r1();
                            }
                        } else if(node->LHS->type==varExpr){
                            _generateAsm(node->RHS, target);
                            auto reg=loadVar(node->LHS, true);
                            writer<<"add "<<target<<","<<target<<","<<reg<<endl;
                        } else if(node->RHS->type==varExpr){
                            _generateAsm(node->LHS, target, true);
                            auto reg=loadVar(node->RHS);
                            writer<<"add "<<target<<","<<target<<","<<reg<<endl;
                        } else{
                            if(target==r1){
                                _generateAsm(node->RHS, r1);
                                _generateAsm(node->LHS, r0);
                                writer<<"add "<<target<<","<<r0<<","<<r1<<endl;
                            } else{
                                push_r1();
                                _generateAsm(node->RHS, r1);
                                _generateAsm(node->LHS, r0);
                                writer<<"add "<<target<<","<<r1<<","<<r0<<endl;
                                pop_r1();
                            }
                        }
                        break;
                    }
                    case OP_MINUS:{
                        if(node->LHS->value.with_value&&node->RHS->value.with_value){
                            int res=node->LHS->value.value-node->RHS->value.value;
                            writer<<"ldr "<<target<<",="<<res<<endl;
                        } else if(node->LHS->type==varExpr&&node->RHS->value.with_value){
                            if(node->RHS->value.value<256){
                                auto reg=loadVar(node->LHS);
                                if(node->RHS->value.value)writer<<"sub "<<target<<","<<reg<<",#"<<node->RHS->value.value<<endl;
                            } else{
                                auto reg=loadVar(node->LHS);
                                writer<<"ldr r12,="<<node->RHS->value.value<<endl;
                                writer<<"sub "<<target<<","<<reg<<",r12"<<endl;
                            }
                        } else if(node->RHS->type==varExpr&&node->LHS->value.with_value){
                            auto reg=loadVar(node->RHS);
                            writer<<"ldr r12,="<<node->LHS->value.value<<endl;
                            writer<<"sub "<<target<<",r12"<<","<<reg<<endl;
                        } else if(node->LHS->type==varExpr&&node->RHS->type==varExpr){
                            auto reg1=loadVar(node->LHS);
                            auto reg2=loadVar(node->RHS);
                            writer<<"sub "<<target<<","<<reg1<<","<<reg2<<endl;
                        } else if(node->LHS->value.with_value){
                            if(node->LHS->value.value<256){
                                if(target!=r1)push_r1();
                                _generateAsm(node->RHS, r1);
                                if(node->LHS->value.value)writer<<"rsb "<<target<<","<<r1<<",#"<<node->LHS->value.value<<endl;
                                if(target!=r1)pop_r1();
                            } else{
                                if(target!=r1)push_r1();
                                _generateAsm(node->RHS, r1);
                                writer<<"ldr r12,="<<node->LHS->value.value<<endl;
                                writer<<"rsb "<<target<<","<<r1<<","<<r12<<endl;
                                if(target!=r1)pop_r1();
                            }
                        } else if(node->RHS->value.with_value){
                            if(node->RHS->value.value<256){
                                if(target!=r1)push_r1();
                                _generateAsm(node->LHS, r1);
                                if(node->RHS->value.value)writer<<"sub "<<target<<","<<r1<<",#"<<node->RHS->value.value<<endl;
                                if(target!=r1)pop_r1();
                            } else{
                                if(target!=r1)push_r1();
                                _generateAsm(node->LHS, r1);
                                writer<<"ldr r12,="<<node->RHS->value.value<<endl;
                                writer<<"sub "<<target<<","<<r1<<","<<r12<<endl;
                                if(target!=r1)pop_r1();
                            }
                        } else if(node->LHS->type==varExpr){
                            _generateAsm(node->RHS, target);
                            auto reg=loadVar(node->LHS, true);
                            writer<<"sub "<<target<<","<<reg<<","<<target<<endl;
                        } else if(node->RHS->type==varExpr){
                            _generateAsm(node->LHS, target, true);
                            auto reg=loadVar(node->RHS, true);
                            writer<<"sub "<<target<<","<<target<<","<<reg<<endl;
                        } else{
                            if(target==r1){
                                _generateAsm(node->RHS, r1);
                                _generateAsm(node->LHS, r0);
                                writer<<"sub "<<target<<","<<r0<<","<<r1<<endl;
                            } else{
                                push_r1();
                                _generateAsm(node->RHS, r1);
                                _generateAsm(node->LHS, r0);
                                writer<<"sub "<<target<<","<<r0<<","<<r1<<endl;
                                pop_r1();
                            }
                        }
                        break;
                    }
                    case OP_MUL:{
                        if(node->LHS->value.with_value&&node->RHS->value.with_value){
                            int res=node->LHS->value.value*node->RHS->value.value;
                            writer<<"ldr "<<target<<",="<<res<<endl;
                        } else if(node->RHS->value.with_value&&__builtin_popcount(node->RHS->value.value)==1){
                            _generateAsm(node->LHS, target);
                            if(node->RHS->value.value!=1)
                            writer<<"mov "<<target<<","<<target<<",lsl #"
                                  <<get_lowest_bit(node->RHS->value.value)<<endl;
                        } else if(node->LHS->type==varExpr&&node->RHS->type==varExpr){
                            auto reg1=loadVar(node->LHS);
                            auto reg2=loadVar(node->RHS);
                            writer<<"mul "<<target<<","<<reg1<<","<<reg2<<endl;
                        } else if(node->LHS->type==varExpr){
                            _generateAsm(node->RHS, target);
                            auto reg=loadVar(node->LHS, true);
                            writer<<"mul "<<target<<","<<reg<<","<<target<<endl;
                        } else if(node->RHS->type==varExpr){
                            _generateAsm(node->LHS, target);
                            auto reg=loadVar(node->RHS, true);
                            writer<<"mul "<<target<<","<<target<<","<<reg<<endl;
                        } else{
                            if(target==r1){
                                _generateAsm(node->RHS, r1);
                                _generateAsm(node->LHS, r0);
                                writer<<"mul "<<target<<","<<r0<<","<<r1<<endl;
                            } else{
                                push_r1();
                                _generateAsm(node->RHS, r1);
                                _generateAsm(node->LHS, r0);
                                writer<<"mul "<<target<<","<<r0<<","<<r1<<endl;
                                pop_r1();
                            }
                        }
                        break;
                    }
                    case OP_DIV:{
                        if(node->RHS->value.with_value&&__builtin_popcount(node->RHS->value.value)==1){
                            _generateAsm(node->LHS, target);
                            if(node->RHS->value.value!=1)writer<<"mov "<<target<<","<<target<<",asr #"
                                  <<get_lowest_bit(node->RHS->value.value)<<endl;
                        } else{
                            push_r1();
                            _generateAsm(node->RHS, r1);
                            _generateAsm(node->LHS, r0);

                            writer<<"push {r2,r3,r4,r12,lr}"<<endl;
                            writer<<"bl __aeabi_idiv"<<endl;
                            writer<<"pop {r2,r3,r4,r12,lr}"<<endl;

                            pop_r1();
                            writer<<"mov "<<target<<",r0"<<endl;
                        }
                        break;
                    }
                    case OP_AND:
                    case OP_OR:
                    case OP_LESS:
                    case OP_BIGGER:
                    case OP_EQUAL:
                    case OP_LESS_OR_EQUAL:
                    case OP_BIGGER_OR_EQUAL:
                    case OP_NOT_EQUAL:
                    case OP_NOT:
                    {
                        throw CompileERROR("Unexpected dealing condition branches.");
                    }
                    case OP_ASSIGN:
                    {
                        if(node->LHS->type==varExpr){
                            if(node->RHS->value.with_value){
                                auto reg=regCtrl.load(((VariableAST*) (node->LHS))->name,
                                                      ((VariableAST*) (node->LHS))->addr, false);
                                regCtrl.mark_update(((VariableAST*) (node->LHS))->name);
                                writer<<"ldr "<<reg<<",="<<node->RHS->value.value<<endl;
                                break;
                            }
                            if(node->RHS->type==varExpr){
                                auto reg_R=regCtrl.load(((VariableAST*) (node->RHS))->name, ((VariableAST*) (node->RHS))->addr);
                                auto reg_L=regCtrl.load(((VariableAST*) (node->LHS))->name, ((VariableAST*) (node->LHS))->addr, false);
                                writer<<"mov "<<reg_L<<","<<reg_R<<endl;
                                regCtrl.mark_update(((VariableAST*) (node->LHS))->name);
                                break;
                            }
                            if(node->RHS->type==binExpr){
                                if(((BinaryOpAST*)node->RHS)->op==OP_ADD
                                   &&((BinaryOpAST*)node->RHS)->LHS->type==varExpr
                                   &&((VariableAST*)((BinaryOpAST*)node->RHS)->LHS)->name==((VariableAST*)node->LHS)->name
                                   &&((BinaryOpAST*)node->RHS)->RHS->value.with_value){
                                    auto reg=regCtrl.load(((VariableAST*) (node->LHS))->name,((VariableAST*) (node->LHS))->addr);
                                    if(((BinaryOpAST*)node->RHS)->RHS->value.value<256){
                                        writer<<"add "<<reg<<","<<reg<<",#"<<((BinaryOpAST*)node->RHS)->RHS->value.value<<endl;
                                    }else{
                                        writer<<"ldr r12,="<<((BinaryOpAST*)node->RHS)->RHS->value.value<<endl;
                                        writer<<"add "<<reg<<","<<reg<<",r12"<<endl;
                                    }
                                    regCtrl.mark_update(((VariableAST*) (node->LHS))->name);
                                    break;
                                }
                                if(((BinaryOpAST*)node->RHS)->op==OP_MINUS
                                   &&((BinaryOpAST*)node->RHS)->LHS->type==varExpr
                                   &&((VariableAST*)((BinaryOpAST*)node->RHS)->LHS)->name==((VariableAST*)node->LHS)->name
                                   &&((BinaryOpAST*)node->RHS)->RHS->value.with_value){
                                    auto reg=regCtrl.load(((VariableAST*) (node->LHS))->name,((VariableAST*) (node->LHS))->addr);
                                    if(((BinaryOpAST*)node->RHS)->RHS->value.value<256){
                                        writer<<"sub "<<reg<<","<<reg<<",#"<<((BinaryOpAST*)node->RHS)->RHS->value.value<<endl;
                                    }else{
                                        writer<<"ldr r12,="<<((BinaryOpAST*)node->RHS)->RHS->value.value<<endl;
                                        writer<<"sub "<<reg<<","<<reg<<",r12"<<endl;
                                    }
                                    regCtrl.mark_update(((VariableAST*) (node->LHS))->name);
                                    break;
                                }
                            }
                            _generateAsm(node->RHS, r0);
                            auto reg=regCtrl.load(((VariableAST*) (node->LHS))->name,
                                                  ((VariableAST*) (node->LHS))->addr, false);
                            regCtrl.mark_update(((VariableAST*) (node->LHS))->name);
                            writer<<"mov "<<reg<<",r0"<<endl;
                        } else if(node->LHS->type==arrayCallExpr){
                            if(((ArrayCallAST*) node->LHS)->dim_diff.size()==1){
                                // deal single-dim array
                                push_r11();
                                _generateAsm(node->RHS,r11);
                                auto cnt=(ArrayCallAST*) node->LHS;
                                if(cnt->indexes[0]->value.with_value){
                                    if(cnt->indexes[0]->value.value*4<256){
                                        auto reg_var=loadVar(cnt->variable);
                                        writer<<"str "<<r11<<",["<<reg_var<<",#"<<cnt->indexes[0]->value.value*4<<"]"<<endl;
                                    }else{
                                        writer<<"ldr r0,="<<cnt->indexes[0]->value.value*4<<endl;
                                        auto reg_var=loadVar(cnt->variable);
                                        writer<<"str "<<r11<<",["<<reg_var<<",r0]"<<endl;
                                    }
                                } else if(cnt->indexes[0]->type==varExpr){
                                    auto reg=loadVar(cnt->indexes[0]);
                                    writer<<"mov r0,"<<reg<<",lsl #"<<UNIT_BITS<<endl;
                                    auto reg_var=loadVar(cnt->variable);
                                    writer<<"str "<<r11<<",["<<reg_var<<",r0]"<<endl;
                                } else{
                                    _generateAsm(cnt->indexes[0], r0);
                                    writer<<"mov r0,r0,lsl #"<<UNIT_BITS<<endl;
                                    auto reg_var=loadVar(cnt->variable);
                                    writer<<"str "<<r11<<",["<<reg_var<<",r0]"<<endl;
                                }
                                pop_r11();
                            } else{
                                push_r11();
                                push_r1();
                                _generateAsm(node->RHS,r11);
                                // calculate offset --------------- start
                                auto cnt=(ArrayCallAST*) node->LHS;
                                _generateAsm(cnt->indexes[cnt->dimSizes.size()-1],r1);
                                for(int i=cnt->dimSizes.size()-2; i>=0; i--){
                                    if(cnt->dim_diff[i]!=1){
                                        if(cnt->indexes[i]->value.with_value){
                                            if(cnt->indexes[i]->value.value*cnt->dim_diff[i]>256){
                                                writer<<"ldr r12,="<<cnt->indexes[i]->value.value*cnt->dim_diff[i]<<endl;
                                                writer<<"add r1,r12"<<endl;
                                            }else{
                                                writer<<"add r1,#"<<cnt->indexes[i]->value.value*cnt->dim_diff[i]<<endl;
                                            }
                                        }if(__builtin_popcount(cnt->dim_diff[i])==1){
                                            _generateAsm(cnt->indexes[i]); // index -> AX
                                            writer<<"mov r0,r0,lsl #"<<get_lowest_bit(cnt->dim_diff[i])<<endl;
                                            writer<<"add r1,r0"<<endl;
                                        }else if(cnt->indexes[i]->type==varExpr){
                                            auto reg=loadVar(cnt->indexes[i]);
                                            writer<<"ldr r12,="<<cnt->dim_diff[i]<<endl;
                                            writer<<"mla r1,"<<reg<<",r12,r1"<<endl;
                                        } else{
                                            _generateAsm(cnt->indexes[i]); // index -> AX
                                            writer<<"ldr r12,="<<cnt->dim_diff[i]<<endl;
                                            writer<<"mla r1,r0,r12,r1"<<endl;
                                        }
                                    } else{
                                        _generateAsm(cnt->indexes[i]); // index -> AX
                                        writer<<"ADD r1,r0"<<endl;
                                    }
                                }
                                // bx is offset

                                // calculate offset --------------- end
                                writer<<"mov r1,r1,lsl #"<<UNIT_BITS<<endl;
                                auto reg_var=loadVar(cnt->variable);
                                writer<<"str "<<r11<<",[r1,"<<reg_var<<"]"<<endl;
                                pop_r1();
                                pop_r11();
                            }
                        }
                        break;
                    }
                    case OP_DECLARE:
                    {
                        if(!node->RHS)break;
                        if(node->RHS->type==arrayPrototypeExpr){
                            throw CompileERROR("Array declaration in OP_DECLARE");
                        } else{
                            // single var declaration
                            if(node->RHS->value.with_value){
                                auto reg=regCtrl.load(((VariableAST*) (node->LHS))->name,
                                                      ((VariableAST*) (node->LHS))->addr, false);
                                writer<<"ldr "<<reg<<",="<<node->RHS->value.value<<endl;
                                regCtrl.mark_update(((VariableAST*) (node->LHS))->name);
                            }else{
                                _generateAsm(node->RHS, r0);
                                auto reg=regCtrl.load(((VariableAST*) (node->LHS))->name,
                                                      ((VariableAST*) (node->LHS))->addr, false);
                                writer<<"mov "<<reg<<",r0"<<endl;
                                regCtrl.mark_update(((VariableAST*) (node->LHS))->name);
                            }
                        }
                        break;
                    }
                    case OP_MOD:
                    {
                        if(node->RHS->value.with_value&&node->RHS->value.value==2){
                            _generateAsm(node->LHS, target);
                            writer<<"and "<<target<<","<<target<<",#1"<<endl;
                            break;
                        }

                        if(target!=r1)push_r1();

                        _generateAsm(node->RHS, r1);
                        _generateAsm(node->LHS, r0);

                        writer<<"push {r2,r3,r4,r12,lr}"<<endl;
                        writer<<"bl __aeabi_idivmod"<<endl;
                        writer<<"pop {r2,r3,r4,r12,lr}"<<endl;

                        if(target!=r1)writer<<"mov "<<target<<",r1"<<endl;

                        if(target!=r1)pop_r1();

                        break;
                    }
                    case OP_SINGLE_MINUS:
                    {
                        _generateAsm(node->RHS, r0);
                        writer<<"mvn r0,r0"<<endl;
                        writer<<"add "<<target<<",r0,#1"<<endl;
                        break;
                    }
                }
                break;
            }
            case ifExpr:{
                /**
                 jz label_else
                 code---
                 jmp endif
                 label_else
                 code---
                 label_endif

                 jz label_endif
                 code---
                 label_endif
                 */
                auto*node=(IfAST*) (astNode);

                if(node->elseCode){

                    int label_start=getLabelID();
                    int label_else=getLabelID();
                    int label_endif=getLabelID();

                    // has else branch
                    regCtrl.clear();
                    writer<<"@ --- if condition start ---"<<endl;
                    deal_conditions(node->expression, label_start, label_else);
                    writer<<"@ --- if condition end ---"<<endl;
                    regCtrl.clear();

                    def_label(label_start);

                    blocks.push_back({"if", node->varCount});
                    alloc_stack(node->varCount);
                    writer<<"@ --- if code start ---"<<endl;
                    _generateAsm(node->code);
                    writer<<"@ --- if code end ---"<<endl;
                    free_stack(node->varCount);
                    blocks.pop_back();

                    jmp(label_endif);
                    def_label(label_else);

                    blocks.push_back({"else", node->varCount});
                    alloc_stack(((ElseAST*) node->elseCode)->varCount);
                    writer<<"@ --- else code start ---"<<endl;
                    _generateAsm(((ElseAST*) node->elseCode)->code);
                    writer<<"@ --- else code end ---"<<endl;
                    free_stack(((ElseAST*) node->elseCode)->varCount);
                    blocks.pop_back();

                    def_label(label_endif);
                    // astNode=astNode->next; // jump over next node
                } else{

                    int label_start=getLabelID();
                    int end_label=getLabelID();

                    // no else branch

                    regCtrl.clear();
                    writer<<"@ --- if condition start ---"<<endl;
                    deal_conditions(node->expression, label_start, end_label);
                    writer<<"@ --- if condition end ---"<<endl;
                    regCtrl.clear();

                    def_label(label_start);

                    blocks.push_back({"if", node->varCount});
                    alloc_stack(node->varCount);
                    writer<<"@ --- if code start ---"<<endl;
                    _generateAsm(node->code);
                    writer<<"@ --- if code end ---"<<endl;
                    free_stack(node->varCount);
                    blocks.pop_back();

                    def_label(end_label);
                }

                break;

            }
            case elseExpr:{
                auto*node=(ElseAST*) (astNode);
                throw CompileERROR("Compiler inner error: unexpected analyzing else branch.");
                break;
            }
            case whileExpr:{
                /**
                 label_while_start
                 condition---
                 and ax,ax
                 jz label_end
                 code---
                 jmp label_while_start
                 label_end
                 */
                auto*node=(WhileAST*) (astNode);

                int label_while_start=getLabelID();
                int label_start=getLabelID();
                int label_end=getLabelID();

                regCtrl.clear();

                def_label(label_while_start);
                writer<<"@ ---- while condition start --- "<<endl;
                deal_conditions(node->expression, label_start, label_end);
                writer<<"@ ---- while condition end --- "<<endl;
                regCtrl.clear();

                loop_start_labels.push(label_while_start);
                loop_ending_labels.push(label_end);

                def_label(label_start);
                blocks.push_back({"while", node->varCount});
                alloc_stack(node->varCount);
                writer<<"@ --- while code start ---"<<endl;
                _generateAsm(node->code);
                writer<<"@ --- while code end ---"<<endl;
                free_stack(node->varCount);
                blocks.pop_back();

                loop_start_labels.pop();
                loop_ending_labels.pop();

                jmp(label_while_start);
                def_label(label_end);

                break;
            }
            case funcExpr:{
                auto*node=(FuncPrototypeAST*) (astNode);

                func_declare(tokenizer[node->name]);

                // register func
                if(FuncTable.find(node->name)!=FuncTable.end()){
                    throw CompileERROR("Function "+tokenizer[node->name]+" re-defined.");
                } else{
                    FuncTable[node->name]=node;
                }

                int label_ret=getLabelID();
                lastFuncRetLabel=label_ret;

                // BP+ & arguments are dealt by caller
                _generateAsm(node->code);
                def_label(label_ret);
                // BP- are dealt by caller

                func_declare_ends(tokenizer[node->name]);

                break;
            }
            case funcArg:{
                auto*node=(FuncArgAST*) (astNode);
                // seems nothing to do
                break;
            }
            case returnExpr:{
                auto*node=(ReturnAST*) (astNode);

                writer<<"@ -------- start RET "<<endl;

                if(node->res){
                    // with return val
                    _generateAsm(node->res);
                }
                regCtrl.clear(true);
                jmp(lastFuncRetLabel);

                break;
            }
            case callExpr:{
                auto*node=(CallAST*) (astNode);

                // detect built-in functions
                // #####################################################
                if(node->name==tokenizer("getch")){
                    if(node->argsCount!=0){
                        throw CompileERROR("built-in function getchar: arguments' count not match");
                    }
                    getChar();
                } else if(node->name==tokenizer("putch")){
                    if(node->argsCount!=1){
                        throw CompileERROR("built-in function putchar: arguments' count not match");
                    }
                    ExprAST*argSource=node->args;
                    _generateAsm(argSource, r0, true);
                    putChar();
                } else if(node->name==tokenizer("putarray")){
                    if(node->argsCount!=2){
                        throw CompileERROR("built-in function getint: arguments' count not match");
                    }
                    ExprAST*argSource=node->args;
                    _generateAsm(argSource->next, r0, true);
                    writer<<"mov r1,r0"<<endl;
                    push_r1(); // push r1
                    _generateAsm(argSource, r0, true);
                    pop_r1(); // pop r1
                    writer<<"push {r1,r2,r3,r4,lr}"<<endl;
                    writer<<"bl putarray"<<endl;
                    writer<<"pop {r1,r2,r3,r4,lr}"<<endl;
                } else if(node->name==tokenizer("getarray")){
                    if(node->argsCount!=1){
                        throw CompileERROR("built-in function getint: arguments' count not match");
                    }
                    ExprAST*argSource=node->args;
                    _generateAsm(argSource, r0, true);
                    writer<<"push {r1,r2,r3,r4,lr}"<<endl;
                    writer<<"bl getarray"<<endl;
                    writer<<"pop {r1,r2,r3,r4,lr}"<<endl;
                } else if(node->name==tokenizer("getint")){
                    if(node->argsCount!=0){
                        throw CompileERROR("built-in function getint: arguments' count not match");
                    }
                    getint();
                } else if(node->name==tokenizer("putint")){
                    if(node->argsCount!=1){
                        throw CompileERROR("built-in function putint: arguments' count not match");
                    }
                    ExprAST*argSource=node->args;
                    _generateAsm(argSource, r0, true);
                    putint();
                } else if(node->name==tokenizer("_sysy_starttime")){
                    if(node->argsCount!=1){
                        throw CompileERROR("built-in function _sysy_starttime: arguments' count not match");
                    }
                    ExprAST*argSource=node->args;
                    _generateAsm(argSource, r0, true);
                    _sysy_starttime();
                } else if(node->name==tokenizer("_sysy_stoptime")){
                    if(node->argsCount!=1){
                        throw CompileERROR("built-in function _sysy_stoptime: arguments' count not match");
                    }
                    ExprAST*argSource=node->args;
                    _generateAsm(argSource, r0, true);
                    _sysy_stoptime();
                } else if(node->name==tokenizer("_memset")){
                    if(node->argsCount!=3){
                        throw CompileERROR("built-in function _memset: arguments' count not match");
                    }
                    ExprAST*argSource=node->args;

                    push_r1();

                    _generateAsm(argSource->next->next, r1, true); // size=r1
                    _generateAsm(argSource, r0, true); // variable=r0

                    writer<<"push {r1,r2,r3,r4,lr}"<<endl;
                    writer<<"mov r2,r1"<<endl;
                    writer<<"mov r1,#0"<<endl;
                    writer<<"bl memset"<<endl;
                    writer<<"pop {r1,r2,r3,r4,lr}"<<endl;

                    pop_r1();
                }else{
                    // ###################################################

                    // find func def
                    FuncPrototypeAST*func;
                    if(FuncTable.find(node->name)==FuncTable.end()){
                        exit(123);
                        throw CompileERROR("Function "+tokenizer[node->name]+" un-defined.");
                    } else{
                        func=FuncTable[node->name];
                    }
                    if(func->argsCount!=node->argsCount){
                        exit(456);
                        throw CompileERROR("Func "+tokenizer[node->name]+" caller: arguments' count not match");
                    }

                    // BP+ & place arguments & call & BP-

                    FuncArgAST*argDest=(FuncArgAST*) func->args;
                    ExprAST*argSource=node->args;
                    // copy arguments
                    vector<int> args_addr;
                    bool hasFuncCall=false;
                    while(argDest){
                        if(argSource->type==callExpr){
                            hasFuncCall=true;
                            break;
                        }
                        argDest=(FuncArgAST*) (argDest->next);
                        argSource=argSource->next;
                    }

                    argDest=(FuncArgAST*) func->args;
                    argSource=node->args;

                    if(hasFuncCall){
                        while(argDest){
                            _generateAsm(argSource, r0, true);
                            push_r0();
                            args_addr.push_back(argDest->addr+func->varCount*UNIT_SIZE);
                            // list-like operation
                            argDest=(FuncArgAST*) (argDest->next);
                            argSource=argSource->next;
                        }
                        for(auto iter=args_addr.rbegin();iter!=args_addr.rend();iter++){
                            pop_r0();
                            writer<<"ldr r12,="<<*iter<<endl;
                            writer<<"str r0,[r9,r12]"<<endl;
                        }
                    }else{
                        while(argDest){
                            _generateAsm(argSource, r0, true);
                            writer<<"ldr r12,="<<(argDest->addr+func->varCount*UNIT_SIZE)<<endl;
                            writer<<"str r0,[r9,r12]"<<endl;
                            // list-like operation
                            argDest=(FuncArgAST*) (argDest->next);
                            argSource=argSource->next;
                        }
                    }
                    call_func(func->varCount, tokenizer[func->name]);
                }
                writer<<"mov "<<target<<",r0"<<endl;
                break;
            }
            case arrayPrototypeExpr:{
                auto*node=(ArrayPrototypeAST*) (astNode);
                // assign array address to pointer
                if(((VariableAST*) node->variable)->isGlobal){
                    writer<<"ldr r0,="<<((VariableAST*) node->variable)->addr<<endl;
                    writer<<"add r0,r10"<<endl;
                    writer<<"add r12,r0,#"<<UNIT_SIZE<<endl;
                    writer<<"str r12,[r0]"<<endl;
                } else{
                    writer<<"ldr r0,="<<((VariableAST*) node->variable)->addr<<endl;
                    writer<<"add r0,r9"<<endl;
                    writer<<"add r12,r0,#"<<UNIT_SIZE<<endl;
                    writer<<"str r12,[r0]"<<endl;
                }
                // -------- init list --------
                if(node->initValues){
                    auto&list=initialization_lists[node->initialization_list_id];
                    writer<<"@ start memset ---------"<<endl;
                    // memset
                    _generateAsm(((VariableAST*) node->variable));
                    writer<<"push {r0,r1,r2,r3,r4,lr}\n"
                            "ldr r1,=0\n"
                            "ldr r2,="<<list.size()*UNIT_SIZE<<
                          "\n"
                          "bl memset\n"
                          "pop {r0,r1,r2,r3,r4,lr}\n";
                    writer<<"@ memset finished ---------"<<endl;
                    writer<<"@ Initial List: start assigning initial values ---------"<<endl;
                    // assign init values
                    _generateAsm(((VariableAST*) node->variable));
                    writer<<"MOV r1,r0"<<endl; // BX is addr
                    for(int i=0; i<list.size(); i++){
                        if(list[i]){
                            list[i]->next=nullptr;
                            if(list[i]->value.with_value&&i*4<256){
                                writer<<"ldr r0,="<<list[i]->value.value<<endl;
                                writer<<"str r0,[r1,#"<<i*4<<"]"<<endl;
                                continue;
                            }
                            writer<<"ldr r12,="<<i*4<<endl; // get offset
                            _generateAsm(list[i]);
                            writer<<"str r0,[r1,r12]"<<endl;
                        }
                    }
                    writer<<"@ Initial List: finished ---------"<<endl;
                }
                break;
            }
            case arrayCallExpr:{
                // DEBUG ------------------------->
                auto node=(ArrayCallAST*) astNode;
                cout<<"debug: Array Call : dim sizes are ";
                for(auto i:node->dim_diff){
                    cout<<i<<" ";
                }
                cout<<endl;
                // <------------------------- DEBUG

                // todo: array call asm
                if(((ArrayCallAST*) node)->dim_diff.size()==1){
                    // deal single-dim array
                    auto cnt=(ArrayCallAST*) node;
                    if(cnt->indexes[0]->value.with_value){
                        if(cnt->indexes[0]->value.value*4<256){
                            auto reg_var=loadVar(cnt->variable);
                            writer<<"ldr "<<target<<",["<<reg_var<<",#"<<cnt->indexes[0]->value.value*4<<"]"<<endl;
                        }else{
                            writer<<"ldr r0,="<<cnt->indexes[0]->value.value*4<<endl;
                            auto reg_var=loadVar(cnt->variable);
                            writer<<"ldr "<<target<<",["<<reg_var<<",r0]"<<endl;
                        }
                    } else if(cnt->indexes[0]->type==varExpr){
                        auto reg=loadVar(cnt->indexes[0]);
                        writer<<"mov r0,"<<reg<<",lsl #"<<UNIT_BITS<<endl;
                        auto reg_var=loadVar(cnt->variable);
                        writer<<"ldr "<<target<<",["<<reg_var<<",r0]"<<endl;
                    } else{
                        _generateAsm(cnt->indexes[0], r0);
                        writer<<"mov r0,r0,lsl #"<<UNIT_BITS<<endl;
                        auto reg_var=loadVar(cnt->variable);
                        writer<<"ldr "<<target<<",["<<reg_var<<",r0]"<<endl;
                    }
                } else{
                    if(target!=r1)push_r1();
                    // calculate offset --------------- start
                    auto cnt=(ArrayCallAST*) node;
                    _generateAsm(cnt->indexes[cnt->dimSizes.size()-1],r1);
                    for(int i=cnt->dimSizes.size()-2; i>=0; i--){
                        if(cnt->dim_diff[i]!=1){
                            if(cnt->indexes[i]->value.with_value){
                                if(cnt->indexes[i]->value.value*cnt->dim_diff[i]>256){
                                    writer<<"ldr r12,="<<cnt->indexes[i]->value.value*cnt->dim_diff[i]<<endl;
                                    writer<<"add r1,r12"<<endl;
                                }else{
                                    writer<<"add r1,#"<<cnt->indexes[i]->value.value*cnt->dim_diff[i]<<endl;
                                }
                            }if(__builtin_popcount(cnt->dim_diff[i])==1){
                                _generateAsm(cnt->indexes[i]); // index -> AX
                                writer<<"mov r0,r0,lsl #"<<get_lowest_bit(cnt->dim_diff[i])<<endl;
                                writer<<"add r1,r0"<<endl;
                            }else if(cnt->indexes[i]->type==varExpr){
                                auto reg=loadVar(cnt->indexes[i]);
                                writer<<"ldr r12,="<<cnt->dim_diff[i]<<endl;
                                writer<<"mla r1,"<<reg<<",r12,r1"<<endl;
                            } else{
                                _generateAsm(cnt->indexes[i]); // index -> AX
                                writer<<"ldr r12,="<<cnt->dim_diff[i]<<endl;
                                writer<<"mla r1,r0,r12,r1"<<endl;
                            }
                        } else{
                            _generateAsm(cnt->indexes[i]); // index -> AX
                            writer<<"ADD r1,r0"<<endl;
                        }
                    }
                    // bx is offset

                    // calculate offset --------------- end
                    writer<<"mov r1,r1,lsl #"<<UNIT_BITS<<endl;
                    auto reg_var=loadVar(cnt->variable);
                    writer<<"ldr "<<target<<",[r1,"<<reg_var<<"]"<<endl;
                    if(target!=r1)pop_r1();
                }
                break;
            }
            case breakExpr:{
                int diff=0;
                for(int i=blocks.size()-1; i>=0; i--){
                    if(blocks[i].first=="while"){
                        diff+=blocks[i].second;
                        break;
                    }
                    diff+=blocks[i].second;
                }
                free_stack(diff);
                jmp(loop_ending_labels.top());
                writer<<"@ --- ↑ loop break "<<endl;
                break;
            }
            case continueExpr:{
                int diff=0;
                for(int i=blocks.size()-1; i>=0; i--){
                    if(blocks[i].first=="while"){
                        diff+=blocks[i].second;
                        break;
                    }
                    diff+=blocks[i].second;
                }
                free_stack(diff);
                jmp(loop_start_labels.top());
                writer<<"@ --- ↑ loop continue "<<endl;
                break;
            }
            case initialListExpr:
            {
                throw CompileERROR("Unexpected parsing initialListExpr.");
            }
        }
        if(astNode->buff.flag==BUFF_PUSH){
            auto reg=regCtrl.load(astNode->buff.name, astNode->buff.addr, true);
            writer<<"mov "<<reg<<","<<target<<endl;
            regCtrl.mark_update(astNode->buff.name);
        }
        astNode=astNode->next;
        if(once)astNode=nullptr;
    }
}
