#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <sys/types.h>
#include <syslog.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <curl/curl.h>

static CURL *checksession, *loginsession;
static CURLcode checkcode;
static int logincount = 0;
static int oldlogincount = 0;
static int hourcount = 0;
static pid_t startup_status, sid;
static int checkflag=0;
regex_t compR;
regmatch_t regAns[1];


static _Bool login();

static struct MemoryStruct {
    char *memory;
    size_t size;
} mem_a;

static size_t rboutput(const char *d, size_t n, size_t l, void *p)
{
    //syslog(LOG_DEBUG,"%s",d);
    //printf("%s\n",d);
    (void)d;
    (void)p;
    return n*l;
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

void creatCheckSession(){
    checksession = curl_easy_init();
    if(!checkflag){
        curl_easy_setopt(checksession, CURLOPT_URL, "http://dnet.mb.qq.com/rsp204");
        //printf("switch to Tencent\n");
        checkflag=!checkflag;
    } else {
        curl_easy_setopt(checksession, CURLOPT_URL, "http://connect.rom.miui.com/generate_204");
        //printf("switch to Xiaomi\n");
        checkflag=!checkflag;
    }

    curl_easy_setopt(checksession, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(checksession, CURLOPT_TCP_KEEPIDLE, 120L);
    curl_easy_setopt(checksession, CURLOPT_TCP_KEEPINTVL, 60L);
    curl_easy_setopt(checksession, CURLOPT_WRITEFUNCTION, rboutput);
    curl_easy_setopt(checksession, CURLOPT_USERAGENT, "FFFFFFFFFFFFFFFFF");
}

static _Bool check() {
    checkcode = curl_easy_perform(checksession);
    long http_code = 0;
    curl_easy_getinfo(checksession, CURLINFO_RESPONSE_CODE, &http_code);
    //printf("http code is %ld\n",http_code);
    //printf("check code is %d\n",checkcode);
    switch (http_code) {
        case 204:
            //printf("Check Success.\n");
            sleep(5);
            //exit(0);
            return true;
        case 000:
            //printf("get 000\n");
            sleep(1);
            return check();
        case 302:
        case 200:
            //sleep(1);
            syslog(LOG_WARNING, "Check Failed, Error Code %ld", http_code);
            return login();
        default:
            syslog(LOG_WARNING, "Uncaught Error %ld", http_code);
            //sleep(1);
            return login();
    }
}

static _Bool login() {
    syslog(LOG_INFO,"Checked network lost. Will try to recover in 13s.");
    sleep(13);
    //exit(EXIT_SUCCESS);
    checkcode = curl_easy_perform(loginsession);
    int drcom_num = 0;
    if (checkcode != CURLE_OK) {
        //printf("Login curl Not OK\n");
        sleep(1);
        return login();
    } else {
        //printf("Login curl OK\n");
        if(!regexec(&compR,mem_a.memory,1,regAns,0)){
            sscanf(mem_a.memory+regAns[0].rm_so, "<!--Dr.COMWebLoginID_%d.htm-->", &drcom_num);
            //TODO: implement get error UL
            //("drcom is %d\n",drcom_num);
            if (drcom_num == 2) {
                syslog(LOG_ERR, "Login Failed, Retry in 13s.");
                sleep(13);
                return login();
            } else if(drcom_num==3){
                logincount++;
                //printf("Login Success.\n");
                syslog(LOG_NOTICE, "Login Success.");
                sleep(2);
                curl_easy_cleanup(checksession);
                creatCheckSession();
                return true;
            } else {
                syslog(LOG_ERR, "Login Failed with unknown error (Dr.com Code is %d), Retry in 13s.",drcom_num);
                sleep(13);
                //printf("%s",mem_a.memory);
                return login();
            }
        } else {
            return login();
        }
    }
}

static void creatDaemon(){
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
}



int main(int argc, char *argv[]) {
    char checkurl[] = "http://connect.rom.miui.com/generate_204";
    char loginurl[] = "http://192.168.167.46/a70.htm";
    char *account;
    char *password;
    char loginpostfield[200];
    char logincookie[200];
    char *IP = NULL;
    time_t count_t = time(NULL);

    mem_a.memory = malloc(1);  /* will be grown as needed by the realloc above */
    mem_a.size = 0;    /* no data at this point */

    regcomp(&compR,"<!--Dr.COMWebLoginID_[0-9].htm-->",REG_EXTENDED|REG_ICASE);


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

    sprintf(loginpostfield,
            "DDDDD=%s&upass=%s&R1=0&R2=&R6=0&para=00&0MKKey=123456&buttonClicked=&redirect_url=&err_flag=&username=&password=&user=&cmd=&Login=&R7=0",
            account, password);

    sprintf(logincookie, "Cookie: program=ip; vlan=0; md5_login2=%s%%7C%s; ip=%s", account, password, IP);
    // 自定义检查头部


    creatCheckSession();

    loginsession = curl_easy_init();

    curl_easy_setopt(loginsession, CURLOPT_URL, loginurl);
    curl_easy_setopt(loginsession, CURLOPT_POST, 1L);
    curl_easy_setopt(loginsession, CURLOPT_USERAGENT, "FFFFFFFFFFFFFFFFF");
    /* send all data to this function  */
    curl_easy_setopt(loginsession, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    /* we pass our 'chunk' struct to the callback function */
    curl_easy_setopt(loginsession, CURLOPT_WRITEDATA, (void *) &mem_a);
    curl_easy_setopt(loginsession, CURLOPT_COOKIE, logincookie);
    curl_easy_setopt(loginsession, CURLOPT_POSTFIELDS, loginpostfield);

    syslog(LOG_NOTICE, "Curl inited.");


    check();

//    for (;;) {
//        if (check()) {
//            if (difftime(time(NULL), count_t) >= 3600) {
//                count_t = time(NULL);
//                if (logincount-oldlogincount>0){
//                    syslog(LOG_INFO,"Normal time in the past is %d hours",hourcount);
//                    syslog(LOG_INFO,"Login %d time(s) in the past 1 hour, has login %d times in total.",logincount-oldlogincount,logincount);
//                    oldlogincount = logincount;
//                    hourcount = 0;
//                } else {
//                    syslog(LOG_INFO,"Running normal in the past %d hour(s).",++hourcount);
//                }
//            }
//        } else {
//            syslog(LOG_INFO,"Checked network lost. Will try to recover in 13s.");
//            sleep(13);
//            while (!login()) {}
//        }
//    }

    while (check()){
        if (difftime(time(NULL),count_t)>=3600) {
            count_t = time(NULL);
            if (logincount-oldlogincount>0){
                    syslog(LOG_INFO,"Normal time in the past is %d hours",hourcount);
                    syslog(LOG_INFO,"Login %d time(s) in the past 1 hour, has login %d times in total.",logincount-oldlogincount,logincount);
                    oldlogincount = logincount;
                    hourcount = 0;
                } else {
                    syslog(LOG_INFO,"Running normal in the past %d hour(s).",++hourcount);
                }
        }
    }


    closelog();
    curl_easy_cleanup(checksession);
    curl_easy_cleanup(loginsession);
    return EXIT_SUCCESS;
}
