//
// Created by 12009 on 2020/12/1.
//

#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <regex.h>

int main(int argc, char *argv[]){
    int tmp;
    regex_t compR;
    regmatch_t regAns[1];
    regoff_t a;
    char *test = "aaa<!--Dr.COMWebLoginID_9.htm-->";
    regcomp(&compR,"<!--Dr.COMWebLoginID_[0-9].htm-->",REG_EXTENDED|REG_ICASE);
    tmp = regexec(&compR,test,1,regAns,0);
    sscanf(test+regAns[0].rm_so,"<!--Dr.COMWebLoginID_%d.htm-->",&tmp);
    printf("%d\n",tmp);
    printf("%d,%d",regAns[0].rm_so,regAns[0].rm_eo);

}