//
// Created by 12009 on 2020/12/1.
//

#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]){
    char *account = "1909102345";
    printf("%lu\n",strlen(account));
    printf("%d\n",argc);
    printf("%s\n",argv[0]);
    printf("%s\n",account);
    account = argv[1];
    printf("%s\n",account);
}