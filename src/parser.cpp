#include <vector>
#include <string>
#include <stack>
#include <iostream>
#include "parser.hh"
#include "common.hh"
using namespace std;
void logerr(string message, errType type, string position){
    cout << message << endl;
    if(position.length() > 0)
        cout << "\tAt:" << position << endl;
    exit(1);
}
//Returns if the string can be converted into a number
//By nasm later
bool isValidNumber(const std::string& str) {
    try {std::stod(str);return true;}
    catch (const std::exception& e) {return false;}
}
//Returns index of a keyword stored as token
int keyword(string word){
    for(int i =0; i < keywords_size;i++){
        if(word == keywords[i])
            return i;
    }return -1;
}
string parse_number(string token, string name){
    if(isValidNumber(token))return token;
    if(token[0] == '\\')return name + "_" + token.replace(0,1,"");
    if(token[0] == '~')return token.replace(0,1,"");
    return "dword["  + name + "_" + token + "]";
}
//Parses if operators
string parse_operator(string input){
    if(input == "=")return "je";
    if(input == "!")return "jne";
    if(input == ">")return "jg";
    if(input == "<")return "jl";
    return "jnz";
}
string parse(vector<string> tokens,stack<string> *usingsptr){
    string result = "section .text\n";
    stack<string> bss, data;
    string pipe = "";
    for(size_t i =0; i < tokens.size();i++){
        //Detector for an object
        //This adds the target library behind the executable
        if(tokens[i] == "use"){
            ++i;
            string fileName = tokens[i]
                .replace(0,1,"")
                .replace(tokens[i].size()-1,1,"");
            usingsptr->push(fileName);
        //Parses function
        }else if(tokens[i] == "arr"){
            string s1 = tokens[++i], s2 = tokens[++i];
            data.push(s1 + " dd " + s2+" dup(0)");
        }else if(tokens[i] == "func"){
            //Getting the functions name, arugments and return type
            string name = tokens[++i];
            int conditionCounter = 0;   //Condition counter counts the number of open conditions(nested if)
            stack<int> labelStack;
            result += name + ":\n";     //Makes the label for it
            stack<string> args;
            if(tokens[++i] != "(")  logerr("( token not found in function define!",errType::syntax,name);
            //Gets arguments for the function
            while(++i < tokens.size()){
                     if(tokens[i] == ")") break;
                else if(tokens[i] != ","){args.push(tokens[i]);
                     if(tokens[i] != "void" && tokens[i] != "int"){
                        bss.push(name + "_" + tokens[i]);
                        result +=
                            "\tpop ebx\n\tpop eax\n\tpush ebx\n\tmov dword[" + name + "_" + tokens[i] + "], eax\n";
                       }
                    }
            }
            if(tokens[i] != ")")    logerr(") token not found in function define!",errType::syntax,name);
            if(tokens[++i] != "{")  logerr("Funcion opening not detected!", errType::syntax,name);
            //Getting the actual content
            //Level, amount of conditions at once
            int level = 0, last_label = 0;
            bool redirected = false;
            while(++i < tokens.size()){
                string line_result = "";
                if(tokens[i] == "{") level++;
                else if(tokens[i] == "}"){
                    if(level-- == 0)break;
                    else{
                        if(redirected){
                            redirected = 0;
                            labelStack.pop();
                            replaceAll(result,";lbl" + to_string(last_label),pipe);
                            pipe = "";
                        }else{
                            //Result of this: main10a:
                            result += name + to_string(labelStack.top()) + "a:\n";
                            last_label = labelStack.top();
                            labelStack.pop();
                        }
                        continue;
                    }
                }
                switch(keyword(tokens[i])){
                    //Call
                    case 0:{
                        string fname = tokens[++i];i++;
                        while(++i < tokens.size()){
                            if(tokens[i] == ")")break;
                            line_result += "\tmov eax, " + parse_number(tokens[i],name) + "\n" +
                                           "\tpush eax\n";
                        }
                        line_result += "\tcall " + fname + "\n";
                        i++;
                        }break;
                    //Defines a vairable
                    case 1:
                        bss.push(name + "_" + tokens[++i]);
                        if(tokens[++i] == "="){
                            line_result +=
                                    "\tmov eax, " + parse_number(tokens[++i],name) + "\n" +
                                    "\tmov dword[" + bss.top() + "], eax\n";
                            i++;
                        }
                        break;
                    //Return
                    case 2:
                        if(tokens[++i] == ";") line_result += "\tmov ebx, 0\n\tret\n";
                        else{
                            line_result +=
                                "\tmov eax, " + parse_number(tokens[i],name)+ "\n" +
                                "\tmov ebx, 1\n" +
                                "\tret\n";
                            i++;
                        }break;
                    //If - Used for conditions
                    case 3:{
                        cout << "a\n";
                        if(tokens[++i] != "(") logerr("Broken syntax after the IF word",errType::syntax,name);
                        string num0 = parse_number(tokens[++i],name);
                        string op = parse_operator(tokens[++i]);
                        string num1 = parse_number(tokens[++i],name);
                        string label = name + to_string(conditionCounter++);
                        cout << labelStack.size() << " " << conditionCounter-1 << "\n";
                        labelStack.push((conditionCounter-1));
                        line_result +=
                            "\tmov eax, " + num0 + "\n" +
                            "\tmov ebx, " + num1 + "\n" +
                            "\tcmp eax, ebx\n\t" +
                            op + " " + label + "\n" + 
                            ";lbl" + to_string(conditionCounter-1) + "\n" +
                            "\tjmp " + label + "a\n"+
                            label + ":\n";

                        i+=2;
                        level++;
                        }break;
                    //set - Moves number to each register and does interupt 
                    case 4:{
                        string
                            numb1 = parse_number(tokens[++i],name),
                            numb2 = parse_number(tokens[++i],name),
                            numb3 = parse_number(tokens[++i],name),
                            numb4 = parse_number(tokens[++i],name),
                            inter = tokens[++i];
                            line_result +=
                                "\tmov eax, " + numb1 + "\n"+
                                "\tmov ebx, " + numb2 + "\n"+
                                "\tmov ecx, " + numb3 + "\n"+
                                "\tmov edx, " + numb4 + "\n"+
                                "\tint 0x"    + inter + "\n";
                            i++;
                        }break;
                    //setp - Used for moving and getting from arrays
                    case 5:
                        if(tokens[++i] == "~"){
                            ++i;
                            line_result +=
                                "\tmov eax, " + parse_number(tokens[i],name) + "\n" +
                                "\tmov ebx, dword[eax]\n"+
                                "\tmov "+ parse_number(tokens[i+1],name) + ",ebx\n";
                            i++;
                        }
                        else{
                            ++i;
                            line_result +=
                                "\tmov eax, " + parse_number(tokens[i],name) + "\n" +
                                "\tmov dword[eax], " + parse_number(tokens[i+1],name) + "\n";
                            i++;
                        }
                        break;
                    //getret - Gets return value of a function
                    case 6:
                        ++i;
                        line_result += 
                            "\tcmp ebx, 1\n" + (string)("")+
                            "\tjne " + name + to_string(i-1) + "\n" +
                            "\tmov " + parse_number(tokens[i],name) + ",eax\n" +
                            name + to_string(i-1) + ":\n";
                        i++;
                        break;
                    //setr - Insterts custom code
                    case 7:
                        i+=2;
                        if(tokens[i-1] == "i")line_result += "\tint 0x" + tokens[i] + "\n";
                        else if(tokens[i-1] == "r")line_result += "\t" + tokens[i].replace(0,1,"").replace(tokens[i].size()-1,1,"") + "\n";
                        break;
                    case 8:
                        i++;level++;
                        pipe = "";
                        redirected = true;
                        break;
                    default:{
                            string thing0 = parse_number(tokens[i],name);
                            string thing1 = tokens[++i];
                            if(thing1 == "="){
                                string num = parse_number(tokens[++i],name);
                                if(tokens[i] == num)line_result +=
                                    "\tmov " + thing0 + ", " + num + "\n";
                                else line_result +=
                                    "\tmov eax, " + num + "\n" +
                                    "\tmov " + thing0 + ", eax\n";
                                i++;
                            }else if(thing1 == "++"){
                                line_result +=
                                    "\tmov eax, " + thing0 + "\n" +
                                    "\tadd eax, 1\n" +
                                    "\tmov " + thing0 + ", eax\n";
                                    i++;
                            }else if(thing1 == "--"){
                                line_result +=
                                    "\tmov eax, " + thing0 + "\n" +
                                    "\tsub eax, 1\n" +
                                    "\tmov " + thing0 + ", eax\n";
                                i++;
                            }else if((thing1 == "+" || thing1 == "-" ||
                                      thing1 == "*")&&tokens[++i] == "="){
                                if(thing1 == "+")thing1 = "add";
                                else if(thing1 == "-") thing1 = "sub";
                                else if(thing1 == "*") thing1 = "imul";
                                line_result +=
                                    "\tmov eax, " + thing0 + "\n\t" +
                                    thing1 + " eax, " + parse_number(tokens[++i],name) +
                                    "\n\tmov " + thing0 + ", eax\n";
                                i++;
                            }else if(thing1 == "/" && tokens[++i] == "="){
                                line_result +=
                                    "\tmov eax, " + thing0 + "\n\t" +
                                    "\tmov ebx, " + parse_number(tokens[++i],name) +
                                    "\n\tdiv ebx"
                                    "\n\tmov " + thing0 + ", eax\n";
                                i++;
                            }
                        }break;
                }
                if(redirected){pipe += line_result;}
                else result += line_result;
            }
        }
    }result += "section .bss\n";
    while(bss.size()){
        result += "\t" + bss.top() + " resd 1\n";
        bss.pop();
    }result += "section .data\n";
    while(data.size()){
        result += "\t" + data.top() + "\n";
        data.pop();
    }return result;
}