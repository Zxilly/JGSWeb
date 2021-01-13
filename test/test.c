//
// Created by 12009 on 2020/12/1.
//

#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <regex.h>
#include <unistd.h>

char timestr[60];

char *time2str(int sec) {
    memset(timestr, 0, sizeof(timestr));
    if (sec <= 60) {
        sprintf(timestr, "%d seconds", sec);
    } else if (sec <= 3600) {
        sprintf(timestr, "%d minutes and %d seconds", sec / 60, sec % 60);
    } else if (sec <= 86400) {
        sprintf(timestr, "%d hours, %d minutes and %d seconds", sec / 3600, sec % 3600 / 60, sec % 60);
    } else {
        sprintf(timestr, "%d days, %d hours, %d minutes and %d seconds", sec / 86400, sec % 86400 / 3600, sec % 3600 / 60,
                sec % 60);
    }
    return timestr;
}


int main(int argc, char *argv[]) {
    struct tm *tmptime = NULL;
    while (tmptime->tm_sec >= 0 && tmptime->tm_sec <= 30) {
        sleep(1);
        time_t tmp = 0;
        printf("")
        tmptime = localtime(&tmp);
    }

}