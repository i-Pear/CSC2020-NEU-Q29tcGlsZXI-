#include "Tokenizer.h"

bool Tokenizer::is_num(char c) {
    return c >= '0' && c <= '9';
}

string Tokenizer::ctos(char c) {
    string str;
    str += c;
    return str;
}

void Tokenizer::init() {
    lineNumber = 0;

    this->inLineComment = false;
    // 读取关键字，界符，并先初始化界符
    //ifstream in("../config/keywords.txt");
    string line="int";
    key_words.insert(line);
    for (int i = 0; i < TABLES_NUM; i++) {
        words[i] = Allocator<string>(LIMIT * i);
    }
    int i=0;
    string deli[25]={"+","-","*","/",";","=","(",")","{","}",",","[","]","%",">","<",">=","<=","==","!","!=","&","|","&&","||"};
    while (i<25) {
        line=deli[i];
        delimiters.insert(line);
        (*this)(line);
        i++;
    }
    delimiters.insert(" ");
}

Tokenizer::Tokenizer() {
    init();
}

Token_Type Tokenizer::type(int id) {
    return Token_Type(id/LIMIT);
}

bool Tokenizer::operator>>(int &i) {
    if (res.empty())return false;
    i = res.front();
    res.pop_front();
#ifdef DEBUG
    cout << "Received:\t" << i << "\t" << tokenizer[i] << endl;
#endif
    return true;
}

bool is_alpha(char c){
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_');
}

Tokenizer &Tokenizer::operator<<(const string &cod) {
    lineNumber++;
    string code = trim(cod);
    code += "     ";
    int start_time = code.find("starttime()");
    if(start_time != string::npos){
        code = code.substr(0, start_time) + "_sysy_starttime("+to_string(lineNumber)+")" + code.substr(start_time + 11, code.length() - start_time - 11);
    }
    int end_time = code.find("stoptime()");
    if(end_time != string::npos){
        code = code.substr(0, end_time) + "_sysy_stoptime("+to_string(lineNumber)+")" + code.substr(end_time + 10, code.length() - end_time - 10);
    }
    int i = 0;
    char ch;
    int length = code.length();
    while (i < length) {
        while (isEmpty(code[i]))i++;
        if(i >= length)break;
        string buff;
        if(inLineComment){
            goto LINE_COMMENT;
        }
        if (is_alpha(code[i])) {
            buff += code[i], i++;
            while (!delimiters.count(ctos(code[i])))buff += code[i], i++;
            res.push_back((*this)(buff));
        } else if (is_num(code[i])) {
            buff += code[i], i++;
            while (is_num(code[i]))buff += code[i], i++;
            if (code[i] == '.') {
                buff += code[i], i++;
                if (delimiters.count(ctos(code[i])))
                    throw CompileERROR("Additional characters are required after dot");
            } else if (code[i] == 'e') {
                buff += code[i], i++;
                if (code[i] == '+' || code[i] == '-')buff += code[i], i++;
                else if (delimiters.count(ctos(code[i])))
                    throw CompileERROR("Additional characters are required after e");
                if (delimiters.count(ctos(code[i])))
                    throw CompileERROR("Additional characters are required after +/-");
            }
            while (!delimiters.count(ctos(code[i])))buff += code[i], i++;
            res.push_back((*this)(buff));
        } else if (code[i] == '\'' || code[i] == '\"') {
            ch = code[i], i++;
            buff += ch;
            while (code[i] != ch)buff += code[i], i++;
            buff += ch;
            res.push_back((*this)(buff));
            i++;
        } else if (delimiters.count(ctos(code[i]))) {
            LINE_COMMENT:
            if(code[i] == '/' && code[i + 1] == '*' && !inLineComment){
                inLineComment = true;
                i += 2;
                continue;
            }else if(code[i] == '*' && code[i + 1] == '/'){
                inLineComment = false;
                i += 2;
                continue;
            }else if(inLineComment){
                i++;
                continue;
            }
            if(code[i] == '/' && code[i + 1] == '/')break;
            string check = ctos(code[i]);
            check += code[i + 1];
            if (delimiters.count(check) && check.length() == 2) {
                res.push_back((*this)(check)), i += 2;
            } else {
                buff += code[i];
                res.push_back((*this)(buff)), i++;
            }
        } else {
            if(code[i] == '#')break;
            else {
//                cout << code << endl;
//                cout << i << " " << code[i] << endl;
                throw CompileERROR("using disallowed character '" + ctos(code[i]) + "'");
            }
        }
    }
    return *this;
}

void operator >> (ifstream& pre,Tokenizer& tk){
    string line;
    while(!pre.eof()){
        getline(pre, line);
        tokenizer<<line;
    }
}

const string &Tokenizer::operator[](int i) {
    return words[type(i)][i];
}

int Tokenizer::operator()(const string &str) {
    Token_Type i;
    if (delimiters.count(str))i = TYPE_DELIMITER;
    else if (key_words.count(str))i = TYPE_KEYWORD;
    else if (is_num(str[0]))i = TYPE_NUM;
    else if (str[0] == '\'')i = TYPE_CHARACTER;
    else if (str[0] == '\"')i = TYPE_STRING;
    else i = TYPE_SYMBOL;
    return words[i](str);
}

void Tokenizer::push_back(int id) {
    res.push_front(id);
}

int Tokenizer::forward(int num) {
    if(num <= 0)throw CompileERROR("need a num > 0");
    int ans;
    stack<int> tmp;
    for (int i = 0; i < num; ++i) {
        tmp.push(res.front());
        res.pop_front();
        if(i == num - 1){
            ans = tmp.top();
        }
    }
    while (!tmp.empty()){
        res.push_front(tmp.top());
        tmp.pop();
    }
    return ans;
}


Tokenizer tokenizer;