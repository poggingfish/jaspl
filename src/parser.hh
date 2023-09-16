#ifndef __PARSER_HH
#define __PARSER_HH
#include <string>
#include <stack>
using namespace std;
enum errType{syntax, warn, common, info};
void logerr(string message, errType type, string position);
string parse(vector<string> tokens,stack<string> *usingsptr);
#endif