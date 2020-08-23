#include "AST.h"
#include "Symbol.h"
#include "Assembly.h"

using namespace std;

int main(int argc, char*argv[]){

    // in function test, argc=5, else 6
    if(argc==6){
        largeMem=true;
        totalStackSize=10;
    } else{
        totalStackSize=100000;
        largeMem=false;
    }
    tokenizer.lineNumber=0;

    ifstream in(argv[4]);
    string line;
    while(!in.eof()){
        getline(in, line);
        tokenizer<<line;
    }
    in.close();
    tokenizer<<"EOF";
    cout<<tokenizer[tokenizer.forward(2)]<<endl;

    Parser parser;
    auto*head=(BlockAST*) parser.ParseStart()->next;

    BuffOptimizer buffOptimizer;
    buffOptimizer.startFromHead(head);

    eraseEmpty(head, nullptr);

    tiny_constant_table.push({});
    int count=getBlockVarInfo(((BlockAST*) head)->code);
    getBlockVarInfo(((BlockAST*) head)->next);

    addrLenStack.push({count, 500000});
    distributeVars(((BlockAST*) head)->code);
    distributeVars(((BlockAST*) head)->next);

    writer.open(argv[3]);
    generateAsm(head);
    writer.close();

    ifstream ifs(argv[3]);
    list<string> output={""};
    while(getline(ifs, line)){
        if(line=="mov r0,r0")continue;
        if(line.length()>3&&output.back().length()>3){
            if(line.substr(0, 3)=="pop"&&output.back().substr(0, 4)=="push"){
                if(line.substr(4)==output.back().substr(5)){
                    output.pop_back();
                    continue;
                }
            }
            if(line.substr(0, 4)=="push"&&output.back().substr(0, 3)=="pop"){
                if(line.substr(5)==output.back().substr(4)){
                    output.pop_back();
                    continue;
                }
            }
        }
        output.push_back(line);
    }
    ifs.close();
    writer.open(argv[3]);
    for(auto&s:output){
        writer<<s<<endl;
    }
    writer.close();

    cout<<"\n----------- Compile OK -----------\n";
    return 0;
}
