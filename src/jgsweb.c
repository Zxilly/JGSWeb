#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <syslog.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <curl/curl.h>

static CURL *checksession, *loginsession;
static CURLcode checkcode;
static int logincount = 0;
static time_t starttime = 0;
static time_t lastsuccesstime = 0;
static time_t starttimelength = 0;
static time_t normaltimelength = 0;
static time_t errortimelength = 0;
static pid_t startup_status, sid;
static int checkflag = 0;

static bool duplicate_flag = false;
static bool first_flag = false;

regex_t compR;
regmatch_t regAns[1];
static char timestr[60];


static _Bool login();

static struct MemoryStruct {
    char *memory;
    size_t size;
} mem_a;

static size_t rboutput(const char *d, size_t n, size_t l, void *p) {
    //syslog(LOG_DEBUG,"%s",d);
    //printf("%s\n",d);
    (void) d;
    (void) p;
    return n * l;
}


char *time2str(time_t sec) {
    memset(timestr, 0, sizeof(timestr));
    if (sec <= 60) {
        sprintf(timestr, "%ld seconds", sec);
    } else if (sec <= 3600) {
        sprintf(timestr, "%ld minutes and %ld seconds", sec / 60, sec % 60);
    } else if (sec <= 86400) {
        sprintf(timestr, "%ld hours, %ld minutes and %ld seconds", sec / 3600, sec % 3600 / 60, sec % 60);
    } else {
        sprintf(timestr, "%ld days, %ld hours, %ld minutes and %ld seconds", sec / 86400, sec % 86400 / 3600,
                sec % 3600 / 60,
                sec % 60);
    }
    return timestr;
}

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *) userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL) {
        /* out of memory! */
        syslog(LOG_ERR, "not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

void setupCheckServer() {
    curl_easy_setopt(checksession, CURLOPT_URL, "http://connect.rom.miui.com/generate_204");
}

void creatCheckSession() {
    checksession = curl_easy_init();

    setupCheckServer();

    curl_easy_setopt(checksession, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(checksession, CURLOPT_TCP_KEEPIDLE, 120L);
    curl_easy_setopt(checksession, CURLOPT_TCP_KEEPINTVL, 60L);
    curl_easy_setopt(checksession, CURLOPT_WRITEFUNCTION, rboutput);
    curl_easy_setopt(checksession, CURLOPT_TIMEOUT, 2L);
    curl_easy_setopt(checksession, CURLOPT_USERAGENT, "FFFFFFFFFFFFFFFFF");
}

static bool check() {
    checkcode = curl_easy_perform(checksession);

    if (checkcode != CURLE_OK) {
        if (checkcode == CURLE_OPERATION_TIMEDOUT) {
            syslog(LOG_WARNING, "Check Timeout. Potential Connection lost.");
            return login();
        }
    }

    long http_code = 0;
    curl_easy_getinfo(checksession, CURLINFO_RESPONSE_CODE, &http_code);

    switch (http_code) {
        case 204:
            sleep(1);
            return true;
        case 000:
            syslog(LOG_WARNING, "Checked DNS Error.");
            sleep(1);
            return check();
        case 302:
        case 200:
            syslog(LOG_WARNING, "Check Failed, Error Code %ld", http_code);
            return login();
        default:
            syslog(LOG_WARNING, "Uncaught Error %ld", http_code);
            return login();
    }
}

static bool login() {
    int hour = 0, weekday = 0;
    time_t tmp = time(NULL);
    struct tm *tmptime = localtime(&tmp);
    hour = tmptime->tm_hour;
    weekday = tmptime->tm_wday;
    if (weekday >= 1 && weekday <= 5) {
        while (hour >= 0 && hour <= 6) {
            errortimelength += 1200;
            tmp = time(NULL);
            tmptime = localtime(&tmp);
            hour = tmptime->tm_hour;
            syslog(LOG_DEBUG, "%d:%d:%d, wait 1200s and another loop.", tmptime->tm_hour, tmptime->tm_min, tmptime->tm_sec);
            sleep(1200);
            if (!first_flag) {
                first_flag = true;
            }
        } // FIXME: endless loop
    }
    syslog(LOG_INFO, "Wait for 13s.");
    sleep(13);
    errortimelength += 13;
    checkcode = curl_easy_perform(loginsession);
    int drcom_num = 0;
    if (checkcode != CURLE_OK) {
        sleep(1);
        errortimelength += 1;
        syslog(LOG_ERR, "Meet curl error.");
        return login();
    } else {
//        if (!regexec(&compR, mem_a.memory, 1, regAns, 0)) {
//            sscanf(mem_a.memory + regAns[0].rm_so, "<!--Dr.COMWebLoginID_%d.htm-->", &drcom_num);
//            //TODO: implement get error UL
//            if (drcom_num == 2) {
//                if (duplicate_flag) {
//                    syslog(LOG_ERR, "Duplicate Login Checked, Retry in 17s.");
//                    errortimelength += 17;
//                    sleep(17);
//                } else {
//                    duplicate_flag = true;
//                    syslog(LOG_ERR, "Duplicate Login Checked, Retry in 13s.");
//                    errortimelength += 13;
//                    sleep(13);
//                }
//                return login();
//            } else if (drcom_num == 3) {
//                logincount++;
//                starttimelength = difftime(time(NULL), starttime);
//                normaltimelength = starttimelength - errortimelength;
//                syslog(LOG_NOTICE, "Login Success");
//                syslog(LOG_NOTICE, "Have logined %d time(s) in %s", logincount,
//                       time2str(difftime(time(NULL), starttime)));
//                syslog(LOG_NOTICE, "Have started %s.", time2str(starttimelength));
//                syslog(LOG_NOTICE, "Running normal %s.", time2str(normaltimelength));
//                syslog(LOG_NOTICE, "Network Lost %s.", time2str(errortimelength));
//                syslog(LOG_NOTICE, "SLA is %.5f",
//                       (double) normaltimelength / (double) starttimelength);
//
//                if (lastsuccesstime == 0 || first_flag) {
//                    lastsuccesstime = time(NULL);
//                    syslog(LOG_NOTICE, "This is first time login.");
//                    first_flag = false;
//                } else {
//                    syslog(LOG_NOTICE, "This part normal time is %ld seconds", time(NULL) - lastsuccesstime);
//                    lastsuccesstime = time(NULL);
//                    syslog(LOG_NOTICE, "Average normal time is %s", time2str(normaltimelength / logincount));
//                }
//
//                sleep(2);
//                curl_easy_cleanup(checksession);
//                creatCheckSession();
//                return true;
//            } else {
//                syslog(LOG_ERR, "Login Failed with unknown error (Dr.com Code is %d), Retry in 13s.", drcom_num);
//                errortimelength += 13;
//                sleep(13);
//                return login();
//            }
//        } else {
//            return login();
//        }

        logincount++;
        starttimelength = difftime(time(NULL), starttime);
        normaltimelength = starttimelength - errortimelength;
        syslog(LOG_NOTICE, "Login Success");
        syslog(LOG_NOTICE, "Have logined %d time(s) in %s", logincount,
               time2str(difftime(time(NULL), starttime)));
        syslog(LOG_NOTICE, "Have started %s.", time2str(starttimelength));
        syslog(LOG_NOTICE, "Running normal %s.", time2str(normaltimelength));
        syslog(LOG_NOTICE, "Network Lost %s.", time2str(errortimelength));
        syslog(LOG_NOTICE, "SLA is %.5f",
               (double) normaltimelength / (double) starttimelength);

        if (lastsuccesstime == 0 || first_flag) {
            lastsuccesstime = time(NULL);
            syslog(LOG_NOTICE, "This is first time login.");
            first_flag = false;
        } else {
            syslog(LOG_NOTICE, "This part normal time is %ld seconds", time(NULL) - lastsuccesstime);
            lastsuccesstime = time(NULL);
            syslog(LOG_NOTICE, "Average normal time is %s", time2str(normaltimelength / logincount));
        }

        sleep(2);
        curl_easy_cleanup(checksession);
        creatCheckSession();
        return true;
    }
}

static void creatDaemon() {
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
            perror("Second Daemon Claim");
            exit(EXIT_FAILURE);
        } else {
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
        // printf("JGSWeb try to start daemon parent at [%d], parent process will suicide.\n", startup_status);
        exit(EXIT_SUCCESS);
    }
}


int main(int argc, char *argv[]) {
//    char loginurl[] = "http://192.168.167.46/";
    char loginurl[200];
    char *account;
    char *password;
//    char loginpostfield[200];
    char logincookie[200];
    char *IP = NULL;
    time_t count_t = time(NULL);

    mem_a.memory = malloc(1);  /* will be grown as needed by the realloc above */
    mem_a.size = 0;    /* no data at this point */

    regcomp(&compR, "<!--Dr.COMWebLoginID_[0-9].htm-->", REG_EXTENDED | REG_ICASE);


    //处理参数
    if (argc != 3) {
        perror("Argument num");
        exit(EXIT_FAILURE);
    }

    if (strlen(argv[1]) != 10) {
        perror("Account error");
        exit(EXIT_FAILURE);
    } else {
        account = argv[1];
    }

    if (strlen(argv[2]) != 6) { // 介于我压根没找到改密码的地方，姑且认为这玩意儿一定 6 位
        perror("Password error");
        exit(EXIT_FAILURE);
    } else {
        password = argv[2];
    }

    FILE *p_file = NULL;

    p_file = popen(
            "ubus call network.interface.wan status | grep \\\"address\\\" | grep -oE '[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}'",
            "r");
    if (!p_file) {
        perror("GetIP");
    }
    fscanf(p_file, "%s", IP);
    pclose(p_file); //TEST
//    IP = "10.73.44.192";
//    printf("%s",IP);


    openlog("JGSWeb", LOG_PID, LOG_SYSLOG);

    // 程序启动时获取 IP，但是程序本身是设计作为长期运行的，也就是说，在程序运行中，wan 口可能会 DHCP 刷新地址，这个 IP 失效
    // 程序接下来将会作为 daemon 运行， system 隐式调用的 fork() 会被禁止， 所以无法重复这个操作
    // 理想的操作应该是使用 uci 或者访问 ubus 获取 IP，同时也可以在 IP 变更时得到通知
    // TODO: implement ubus client

    creatDaemon();
// 开发中不作为daemon运行



    curl_global_init(CURL_GLOBAL_ALL);



    //设置login参数串

//    sprintf(loginpostfield,
//            "DDDDD=%s&upass=%s&R1=0&R2=&R6=0&para=00&0MKKey=123456&buttonClicked=&redirect_url=&err_flag=&username=&password=&user=&cmd=&Login=&R7=0",
//            account, password);

    sprintf(logincookie, "Cookie: program=ip; vlan=0; md5_login2=%s%%7C%s; ip=%s", account, password, IP);
    // 自定义检查头部

    sprintf(loginurl,
            "http://192.168.167.46/drcom/login?callback=dr1003&DDDDD=%s&upass=%s&0MKKey=123456&R1=0&R2=&R3=0&R6=0&para=00&v6ip=&terminal_type=1&lang=zh-cn&jsVersion=4.1&v=7912&lang=zh",
            account, password);

    creatCheckSession();

    loginsession = curl_easy_init();

    curl_easy_setopt(loginsession, CURLOPT_URL, loginurl);
//    curl_easy_setopt(loginsession, CURLOPT_POST, 1L);
    curl_easy_setopt(loginsession, CURLOPT_USERAGENT, "FFFFFFFFFFFFFFFFF");
    /* send all data to this function  */
    curl_easy_setopt(loginsession, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    /* we pass our 'chunk' struct to the callback function */
    curl_easy_setopt(loginsession, CURLOPT_WRITEDATA, (void *) &mem_a);
    curl_easy_setopt(loginsession, CURLOPT_COOKIE, logincookie);
//    curl_easy_setopt(loginsession, CURLOPT_POSTFIELDS, loginpostfield);

    syslog(LOG_NOTICE, "Curl inited.");

    starttime = (int) time(NULL);


//    while (check()) {
//        if (difftime(time(NULL), count_t) >= 3600) {
//            count_t = time(NULL);
//            if (logincount - oldlogincount <= 0) {
//                // syslog(LOG_INFO, "Normal time in the past is %d s", hourcount);
////                syslog(LOG_INFO, "Login %d time(s) in the past 1 hour, has login %d times in total.",
////                       logincount - oldlogincount, logincount);
////                oldlogincount = logincount;
////                hourcount = 0;
//                syslog(LOG_INFO, "Running normal in the past %d hour(s).", ++hourcount);
//            }/* else {
//                syslog(LOG_INFO, "Running normal in the past %d hour(s).", ++hourcount);
//            }*/
//        }
//    }

    for (;;) {
        check();
    }


    closelog();
    curl_easy_cleanup(checksession);
    curl_easy_cleanup(loginsession);
    return EXIT_SUCCESS;
}
