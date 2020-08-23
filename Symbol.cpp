#include "Symbol.h"

const int UNIT_BITS=2;
const int UNIT_SIZE=1<<UNIT_BITS;
int MAIN_FUNC_SHIFTING;

stack<int> loop_ending_labels;
stack<int> loop_start_labels;

stack<map<int, int>> tiny_constant_table;

map<int, vector<ExprAST*>> initialization_lists;

SymTable::SymTable(){
    // init
    data.push({true, 0,});
}

bool SymTable::exists(int id){
    return data.top().map.find(id)!=data.top().map.end();
}

void SymTable::alloc(int id, int addr){
    if(data.top().newlyDefined.find(id)!=data.top().newlyDefined.end())
        throw CompileERROR("var "+tokenizer[id]+" re-defined.");
    data.top().newlyDefined.insert(id);
    data.top().map[id]=addr;
}

void SymTable::into(int diff){
    data.push(data.top());
    data.top().isGlobal=false;
    data.top().newlyDefined.clear();
    for(auto&i:data.top().map){
        if(i.second<50)i.second-=diff;
    }
}

void SymTable::pop(){
    data.pop();
}

int SymTable::operator[](int id){
    if(!exists(id)){
        throw CompileERROR("var "+tokenizer[id]+" not defined");
    }
    return data.top().map[id];
}

SymTable symTable;

stack<pair<int, int>> addrLenStack;

map<int, set<int>> func_reliably;

map<int, FuncPrototypeAST*> FuncTable;

bool _isFuncArgTable=false;

int getFuncCodeLines(ExprAST*node){
    int count=0;
    while(node){
        count++;
        node=node->next;
    }
    return count;
}

void parse_init_list(ExprAST*node, vector<ExprAST*>&list, vector<int>&diff_unit, int&pos, int floor){
    // do align
    if(floor>=1&&pos%diff_unit[floor-1]){
        pos+=(diff_unit[floor-1]-pos%diff_unit[floor-1]);
    }

    while(node){
        switch(node->type){
            case initialListExpr:
                parse_init_list(((InitialListAST*) node)->head, list, diff_unit, pos, floor+1);
                break;
            case nullExpr:
                break;
            default:
                list[pos++]=node;
                break;
        }
        node=node->next;
    }

    // do align
    if(floor>=1&&pos%diff_unit[floor-1]){
        pos+=(diff_unit[floor-1]-pos%diff_unit[floor-1]);
    }
}

int getBlockVarInfo(ExprAST*node){
    int count=0;
    while(node){
        switch(node->type){
            case varExpr:{
                if(_isFuncArgTable||((VariableAST*) node)->isNewVar){
                    count+=1;
                }
                auto iter=tiny_constant_table.top().find(((VariableAST*) node)->name);
                if(iter!=tiny_constant_table.top().end()){
                    ((VariableAST*) node)->value.with_value=true;
                    ((VariableAST*) node)->value.value=iter->second;
                }
                break;
            }
            case ifExpr:{
                tiny_constant_table.push(tiny_constant_table.top());
                ((IfAST*) node)->varCount=getBlockVarInfo(((IfAST*) node)->code);
                tiny_constant_table.pop();

                getBlockVarInfo(((IfAST*) node)->expression);

                auto cnt=(IfAST*) node;
                if(cnt->elseCode){
                    tiny_constant_table.push(tiny_constant_table.top());
                    ((ElseAST*) (cnt->elseCode))->varCount=getBlockVarInfo(((ElseAST*) (cnt->elseCode))->code);
                    tiny_constant_table.pop();
                }

                break;
            }
            case blockExpr:
                tiny_constant_table.push(tiny_constant_table.top());
                ((BlockAST*) node)->varCount=getBlockVarInfo(((BlockAST*) node)->code);
                tiny_constant_table.pop();
                break;
            case elseExpr:

                break;
            case whileExpr:
                tiny_constant_table.push(tiny_constant_table.top());
                ((WhileAST*) node)->varCount=getBlockVarInfo(((WhileAST*) node)->code);
                tiny_constant_table.pop();

                getBlockVarInfo(((WhileAST*) node)->expression);
                // count+=((WhileAST*)node)->varCount;
                break;
            case funcExpr:
                // getFuncReliably(((FuncPrototypeAST*) node)->code,((FuncPrototypeAST*) node)->name);
                ((FuncPrototypeAST*) node)->codeLines=getFuncCodeLines(((FuncPrototypeAST*) node)->code);
                ((FuncPrototypeAST*) node)->argsCount=getBlockVarInfo(((FuncPrototypeAST*) node)->args);

                tiny_constant_table.push(tiny_constant_table.top());
                ((FuncPrototypeAST*) node)->varCount=getBlockVarInfo(((FuncPrototypeAST*) node)->args)+
                                                     getBlockVarInfo(((FuncPrototypeAST*) node)->code);
                tiny_constant_table.pop();

                if(tokenizer[((FuncPrototypeAST*) node)->name]=="main"){
                    MAIN_FUNC_SHIFTING=((FuncPrototypeAST*) node)->varCount;
                }
                // count+=((FuncPrototypeAST*)node)->varCount;
                break;
            case callExpr:
                _isFuncArgTable=true;
                ((CallAST*) node)->argsCount=getNodeCount(((CallAST*) node)->args);
                getBlockVarInfo(((CallAST*) node)->args);
                _isFuncArgTable=false;
                break;
            case funcArg:
                count+=1;
                break;
            case constExpr:
                if(_isFuncArgTable)count+=1;
                break;
            case nullExpr:
                break;
            case binExpr:{
                auto cnt=(BinaryOpAST*) node;
                if(cnt->op==OP_DECLARE){
                    if(((VariableAST*) cnt->LHS)->value.is_const){
                        tiny_constant_table.top()[((VariableAST*) cnt->LHS)->name]=((VariableAST*) cnt->RHS)->value.value;
                    }else{
                        auto iter=tiny_constant_table.top().find(((VariableAST*) node)->name);
                        if(iter!=tiny_constant_table.top().end()){
                            tiny_constant_table.top().erase(iter);
                        }
                    }
                }
                count+=getBlockVarInfo(((BinaryOpAST*) node)->LHS);
                count+=getBlockVarInfo(((BinaryOpAST*) node)->RHS);
                break;
            }
            case returnExpr:
                getBlockVarInfo(((ReturnAST*) node)->res);
                break;
            case arrayPrototypeExpr:{
                auto cnt=(ArrayPrototypeAST*) node;
                count+=getBlockVarInfo(cnt->variable);
                vector<int> dims;
                int size=1;
                for(auto i:cnt->dimensions){
                    auto cnt_dimSize=calculate_constant(i, 0);
                    dims.push_back(cnt_dimSize);
                    size*=dims.back();
                }
                count+=size;
            }
                break;
            case arrayCallExpr:{
                auto cnt=(ArrayCallAST*) node;
                for(auto i:cnt->indexes){
                    getBlockVarInfo(i);
                }
            }
                break;
            case breakExpr:
                break;
            case continueExpr:
                break;
            case initialListExpr:
                break;
        }
        node=node->next;
    }
    return count;
}

void distributeVars(ExprAST*node){
    while(node){
        if(node->buff.flag!=BUFF_NO_EFFECT){
            node->buff.addr=symTable[node->buff.name]*UNIT_SIZE;
#ifdef DISTRIBUTE_DEBUG
            cout<<"Addr of temp var: "<<tokenizer[node->buff.name]<<" = "<<node->buff.addr<<endl;
#endif
        }
        switch(node->type){
            case varExpr:{
                if(((VariableAST*) node)->isNewVar){
                    addrLenStack.top().second++;
                    if(symTable.data.top().isGlobal){
                        symTable.alloc(((VariableAST*) node)->name,
                                       addrLenStack.top().second);
                    } else{
                        symTable.alloc(((VariableAST*) node)->name,
                                       addrLenStack.top().second-addrLenStack.top().first);
                    }
                }
                ((VariableAST*) node)->addr=symTable[((VariableAST*) node)->name]*UNIT_SIZE;
                ((VariableAST*) node)->isGlobal=(((VariableAST*) node)->addr>50);

#ifdef DISTRIBUTE_DEBUG
                cout<<"Addr of "<<tokenizer[((VariableAST*) node)->name]<<" = "<<((VariableAST*) node)->addr<<endl;
#endif
                break;
            }
            case funcArg:{
                addrLenStack.top().second++;
                symTable.alloc(((FuncArgAST*) node)->name,
                               addrLenStack.top().second-addrLenStack.top().first);
                ((FuncArgAST*) node)->addr=symTable[((FuncArgAST*) node)->name]*UNIT_SIZE;
                {
                    auto cnt=(FuncArgAST*) node;
                    if(cnt->array.isArray){
                        vector<int> dims;
                        int size=1;
                        for(auto i:cnt->array.dimensions){
                            int cnt_dimSize;
                            if(i){
                                cnt_dimSize=calculate_constant(i, 1);
                            } else{
                                cnt_dimSize=1;
                            }
                            dims.push_back(cnt_dimSize);
                            size*=dims.back();
                        }
                        symTable.data.top().arrMap[cnt->name]={0, (int) dims.size(), dims};
                        symTable.data.top().arrMap[cnt->name].getDiff();
                    }
                }
                break;
            }
            case blockExpr:{
                auto cnt=(BlockAST*) node;

                symTable.into(cnt->varCount);
                addrLenStack.push({cnt->varCount, 0});

                distributeVars(cnt->code);

                addrLenStack.pop();
                symTable.pop();
                break;
            }
            case ifExpr:{
                auto cnt=(IfAST*) node;
                distributeVars(((IfAST*) node)->expression);

                symTable.into(((IfAST*) node)->varCount);
                addrLenStack.push({((IfAST*) node)->varCount, 0});

                distributeVars(((IfAST*) node)->code);

                addrLenStack.pop();
                symTable.pop();

                if(cnt->elseCode){
                    symTable.into(((ElseAST*) cnt->elseCode)->varCount);
                    addrLenStack.push({((ElseAST*) cnt->elseCode)->varCount, 0});

                    distributeVars(((ElseAST*) cnt->elseCode)->code);

                    addrLenStack.pop();
                    symTable.pop();
                }
                break;
            }
            case elseExpr:{
                break;
            }
            case whileExpr:{
                distributeVars(((WhileAST*) node)->expression);

                symTable.into(((WhileAST*) node)->varCount);
                addrLenStack.push({((WhileAST*) node)->varCount, 0});

                distributeVars(((WhileAST*) node)->code);

                addrLenStack.pop();
                symTable.pop();
                break;
            }
            case funcExpr:{
                symTable.into(((FuncPrototypeAST*) node)->varCount);
                addrLenStack.push({((FuncPrototypeAST*) node)->varCount, 0});

                distributeVars(((FuncPrototypeAST*) node)->args);
                distributeVars(((FuncPrototypeAST*) node)->code);

                addrLenStack.pop();
                symTable.pop();
                break;
            }
            case binExpr:{
                auto cnt=(BinaryOpAST*) node;
                if(cnt->op==OP_DECLARE){
                    if(((VariableAST*) cnt->LHS)->value.is_const){
                        symTable.data.top().constant[((VariableAST*) cnt->LHS)->name]=((VariableAST*) cnt->RHS)->value.value;
                    }
                }
                distributeVars(((BinaryOpAST*) node)->LHS);
                distributeVars(((BinaryOpAST*) node)->RHS);
                break;
            }
            case returnExpr:{
                distributeVars(((ReturnAST*) node)->res);
                break;
            }
            case callExpr:{
                distributeVars(((CallAST*) node)->args);
                break;
            }
            case nullExpr:{
                break;
            }
            case constExpr:{
                break;
            }
            case arrayPrototypeExpr:{
                auto cnt=(ArrayPrototypeAST*) node;

                if(cnt->initValues){
                    distributeVars(cnt->initValues);
                }

                vector<int> dims;
                int size=1;
                for(auto i:cnt->dimensions){
                    auto cnt_dimSize=calculate_constant(i, 1);
                    dims.push_back(cnt_dimSize);
                    size*=dims.back();
                }
                symTable.data.top().arrMap[((VariableAST*) cnt->variable)->name]={0, (int) dims.size(), dims};
                symTable.data.top().arrMap[((VariableAST*) cnt->variable)->name].getDiff();
                distributeVars(cnt->variable);
                addrLenStack.top().second+=(size);

                if(cnt->initValues){
                    int list_id=initialization_lists.size();
                    cnt->initialization_list_id=list_id;
                    initialization_lists[list_id].resize(size);
                    auto&list=initialization_lists[list_id];
                    int pos=0;
                    parse_init_list(cnt->initValues, list,
                                    symTable.data.top().arrMap[((VariableAST*) cnt->variable)->name].diff_unit, pos,
                                    -1);
                }

#ifdef DISTRIBUTE_DEBUG
                cout<<"Detected array of length "<<size<<endl;
#endif
                break;
            }
            case arrayCallExpr:{
                distributeVars(((ArrayCallAST*) node)->variable);
                auto cnt=(ArrayCallAST*) node;
                auto iter=symTable.data.top().arrMap.find(((VariableAST*) cnt->variable)->name);
                if(iter==symTable.data.top().arrMap.end()){
                    throw CompileERROR("Array "+tokenizer[((VariableAST*) cnt->variable)->name]+
                                       " : size is not recorded in symTable.");
                }
                if(cnt->indexes.size()!=iter->second.dims.size()){
                    throw CompileERROR("When calling array "+tokenizer[((VariableAST*) cnt->variable)->name]+
                                       " : dims' count not fit.");
                }
                cnt->dimSizes=iter->second.dims;
                cnt->dim_diff=iter->second.diff_unit;
                for(auto i:cnt->indexes){
                    distributeVars(i);
                }
                break;
            }
            case breakExpr:{
                break;
            }
            case continueExpr:{
                break;
            }
            case initialListExpr:{
                auto cnt=(InitialListAST*) node;
                distributeVars(cnt->head);
                break;
            }
        }
        node=node->next;
    }
}

int getNodeCount(ExprAST*node){
    int count=0;
    while(node){
        count++;
        node=node->next;
    }
    return count;
}

int calculate_constant(ExprAST*astNode, int mode){
    if(astNode->type==binExpr){
        auto node=(BinaryOpAST*) astNode;
        switch(node->op){
            case OP_ADD:
                return calculate_constant(node->LHS, mode)+calculate_constant(node->RHS, mode);
            case OP_MINUS:
                return calculate_constant(node->LHS, mode)-calculate_constant(node->RHS, mode);
            case OP_MUL:
                return calculate_constant(node->LHS, mode)*calculate_constant(node->RHS, mode);
            case OP_DIV:
                return calculate_constant(node->LHS, mode)/calculate_constant(node->RHS, mode);
            case OP_MOD:
                return calculate_constant(node->LHS, mode)%calculate_constant(node->RHS, mode);
            case OP_SINGLE_MINUS:
                return -calculate_constant(node->RHS, mode);
        }
    } else if(astNode->type==varExpr){
        auto node=(VariableAST*) astNode;
        if(mode==0){
            auto iter=tiny_constant_table.top().find(node->name);
            if(iter!=tiny_constant_table.top().end()){
                return iter->second;
            } else{
                throw CompileERROR("Factor is not a constant when calculating constant value.");
            }
        } else{
            auto iter=symTable.data.top().constant.find(node->name);
            if(iter!=symTable.data.top().constant.end()){
                return iter->second;
            } else{
                throw CompileERROR("Factor is not a constant when calculating constant value.");
            }
        }
    } else if(astNode->type==constExpr){
        return ((ConstExprAST*) astNode)->value.value;
    }
    throw CompileERROR("In\"calculate_constant\" : neither switch branch is excuted.");
}