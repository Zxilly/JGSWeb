#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <curl/curl.h>
#include <sys/types.h>
#include <syslog.h>
#include <signal.h>
#include <unistd.h>

static CURL *checksession,*loginsession;
static CURLcode checkcode;
struct curl_slist *checkheaders,*loginheaders;

static void check(){

}

int main(int argc, char *argv[]) {
    pid_t startup_status,sid;
    char checkurl[] = "http://connect.rom.miui.com/generate_204";
    char loginurl[] = "http://192.168.167.46/a70.htm";
    char *account;
    char *password;

    //处理参数
    if (argc != 3){
        perror("Argument num");
        exit(EXIT_FAILURE);
    }

    if (strlen(argv[1])!=10){
        perror("Account error");
        exit(EXIT_FAILURE);
    } else {
        account = argv[1];
    }

    if (strlen(argv[2])!=6){ // 介于我压根没找到改密码的地方，姑且认为这玩意儿一定 6 位
        perror("Password error");
        exit(EXIT_FAILURE);
    } else {
        password = argv[2];
    };


    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN); // ignore 父进程挂掉的关闭信号
    startup_status = fork();
    if (startup_status < 0) {
        perror("Creat Daemon");
        closelog();
        exit(EXIT_FAILURE);
    } else if (startup_status == 0) {
        syslog(LOG_NOTICE, "JGSWeb parent daemon start at [%d].", getpid());
        sid = setsid();
        if (sid < 0) {
            perror("Second Dameon Claim");
            exit(EXIT_FAILURE);
        } else if (sid > 0) {
            syslog(LOG_NOTICE, "JGSWeb parent daemon set sid at [%d].", sid);
            startup_status = fork(); // 第二次fork，派生出一个孤儿
            if (startup_status < 0) {
                perror("Second Daemon Fork");
                exit(EXIT_FAILURE);
            } else if (startup_status > 0) {
                syslog(LOG_NOTICE, "JGSWeb true daemon will start at [%d], daemon parent suicide.", startup_status);
                exit(EXIT_SUCCESS);
            } else {
                syslog(LOG_NOTICE, "JGSWeb true daemon start at [%d].", getpid());
            }
        }
    } else {
        syslog(LOG_NOTICE, "JGSWeb try to start daemon parent at [%d], parent process will suicide.", startup_status);
        printf("JGSWeb try to start daemon parent at [%d], parent process will suicide.\n", startup_status);
        exit(EXIT_SUCCESS);
    }



    openlog("JGSWeb", LOG_PID, LOG_SYSLOG);
    curl_global_init(CURL_GLOBAL_ALL);

    // 自定义检查头部

    curl_easy_setopt(checksession, CURLOPT_URL, checkurl);
    curl_easy_setopt(checksession, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(checksession, CURLOPT_TCP_KEEPIDLE, 120L);
    curl_easy_setopt(checksession, CURLOPT_TCP_KEEPINTVL, 60L);



    curl_easy_setopt(loginsession,CURLOPT_URL,loginurl);



}
