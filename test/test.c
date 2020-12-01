//
// Created by 12009 on 2020/12/1.
//

#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

int main(int argc, char *argv[]){
    static CURL *checksession, *loginsession;
    curl_global_init(CURL_GLOBAL_ALL);
    checksession = curl_easy_init();
    curl_easy_setopt(checksession, CURLOPT_URL, "https://www.baidu.com");
    //curl_easy_setopt(checksession, CURLOPT_TCP_KEEPALIVE, 1L);
    //curl_easy_setopt(checksession, CURLOPT_TCP_KEEPIDLE, 120L);
    //curl_easy_setopt(checksession, CURLOPT_TCP_KEEPINTVL, 60L);
    //curl_easy_setopt(checksession, CURLOPT_USERAGENT, "FFFFFFFFFFFFFFFFF");
    int a = curl_easy_perform(checksession);
    printf("%d\n",a);
}