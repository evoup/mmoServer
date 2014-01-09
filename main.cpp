/*-
 * Project
 *
 * Copyright (c) 2007-2008 by Company.
 * All rights reserved.
 * Author: evoup <webmaster@akata2.vicp.net>
 *
 * $Id: main.cpp,v 1.15 2010/11/16 10:26:45 root Exp $
 */
/******* 服务器程序守护进程多线程版本 (main.cpp) ************/
#include "stdio.h"
#include "stdlib.h"
#include "errno.h"
#include "sys/types.h"
#include "sys/socket.h"
#include "unistd.h"
#include <unistd.h>
#include "netinet/in.h"
#include "netdb.h"
//#include <regex.h>
#include <boost/regex.hpp>
#include <syslog.h>
#include <signal.h>
#include <sys/stat.h>
//c++ precomplier header
#ifndef __cplusplus
#define __cplusplus
#endif
#define __debug
#include "main.h"
#include <arpa/inet.h>
#include <pthread.h>
#include "threadpool.h"
#include <fcntl.h>
#include "config.h"
//#include <json.h>
#define MAXFD 64
#define USE_DAEMON 1
using namespace std;
using namespace boost; 
typedef struct criticalArea{//互斥锁结构
    pthread_mutex_t mutex; 
}criticalArea_t; 
//全局变量
criticalArea_t criticalArea; 
CONNECTION *conn; 
CONFIG config;
void * function(void *);
void skip_rest_head(FILE *);
namespace std{
    struct ARG{//线程参数结构
        int newfd;
        struct sockaddr_in client; 
    };
    int * fdPtr;//传给线程函数的fd函数参数
    class evp_socket {
        int listen_sockfd, new_fd;//侦听套接字描述符，每客户端套接字描述符
        int numreq;//请求的次数
        struct sockaddr_in server_addr;
        struct sockaddr_in client_addr;
        int sin_size, portnumber;
        char hello[];
        pthread_t tid;//线程id
        struct ARG * arg;
        public:
        evp_socket ();
        ~evp_socket ();
        int init (int ac,char* av[]);
        int create_sfd();//创建socket描述符
        int fill_server_struct();//服务端填充结构
        int bind_sfd();//捆绑socket描述符
        int listen_sfd();//监听socket描述符
        int do_serv();//开始服务
        int close_server();
    };
}
evp_socket::evp_socket(){
}
    int evp_socket::init(int ac,char* av[]){
        if (ac < 2 )
        {
            fprintf(stderr, "程序需要参数   Usage:%s portnumbera\n ", av[0]);
            exit(1);
        }

        //printf("共有%d个参数\n",ac-1);
#if USE_DAEMON 
        if (ac == 3) {
            if (!strcmp(av[2],"-dmn") ){
                //fprintf(stdout,"\033[33;40mIpWriteServer running as daemon!write by evoup\033[0;40m");
                pid_t pid;
                if ((pid=fork())!= 0){
                    exit(0);/*父进程终止*/
                }
                setsid();//成为新会话
                fprintf(stdout,"\033[33;40mIpWriteServer running as daemon!write by evoup\n\033[0;40m");
                signal(SIGHUP,SIG_IGN);
                if ( (pid = fork())!=0){//终止第一个子进程
                    exit(0);
                }
                //第二子进程继续
                chdir("/");
                umask(0);
                for(int i=0;i<MAXFD;i++){
                    close(i);
                }
                openlog(av[0],LOG_PID,0);
                //syslog(LOG_NOTICE|LOG_LOCAL0,"server started!");
            }
        }
#endif
#if 0
        if(ac==4) {
            if (!strcmp(av[3],"-pid") ){
                fprintf(stdout,"Todo：记录pid，之后如此关闭，$ kill -TERM `cat /tmp/daemonthredserver.pid`\n");
            }
        }
#endif
        if((portnumber=atoi(av[1]))<0) {
            fprintf(stderr,"Usage:%s portnumbera ",av[0]);
            exit(1);
        }
		//if(0 != config_read()) fprintf(stderr,"Read configure file failed!");
		char serverInfoStr[200];
		sprintf(serverInfoStr,"server max connection:%d\n",config.server_maxconn);
		fprintf(stdout,serverInfoStr);
		conn=(CONNECTION *)calloc(config.server_maxconn, sizeof(CONNECTION));
        return 1;
    }
int evp_socket::create_sfd(){
    /* 服务器端开始建立socket描述符 */
    if ((listen_sockfd = socket(AF_INET, SOCK_STREAM, 0)) ==  - 1){
        fprintf(stdout, "创建socket描述符失败！");
        fprintf(stderr, "Socket error:%s a", strerror(errno));
        exit(1);
    }
    else{
#ifdef __debug
        fprintf(stdout, "创建socket描述符OK！\t\n");
#endif
        //设置套接字选项SO_REUSEADDR
        int opt = SO_REUSEADDR;
        setsockopt(this->new_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
		/**** Todo设置TIME_WAIT for accept过多{{{ ****/
		int nNetTimeout=1000;//1秒
		//发送时限
		setsockopt(this->new_fd,SOL_SOCKET,SO_SNDTIMEO,(char *)&nNetTimeout,sizeof(int));
		//接收时限
		setsockopt(this->new_fd,SOL_SOCKET,SO_RCVTIMEO,(char *)&nNetTimeout,sizeof(int));
		/**** }}} ****/
        return 1;
    }
}
int evp_socket::fill_server_struct(){
    /* 服务器端填充 sockaddr结构 */
    bzero(&server_addr, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(portnumber);
    return 1;
}
int evp_socket::bind_sfd(){
    /* 捆绑sockfd描述符 */
    if (bind(listen_sockfd, (struct sockaddr*)(&server_addr), sizeof(struct sockaddr))
            ==  - 1)
    {
        fprintf(stderr, "Bind error:%s a", strerror(errno));
        exit(1);
    }
    else{
#ifdef __debug
        fprintf(stdout, "捆绑sockfd描述符OK！\t\n");
#endif
        return 1;
    }
}
int evp_socket::listen_sfd(){
    /* 监听sockfd描述符 */
    if(listen(listen_sockfd,5)==-1) {
        fprintf(stderr,"Listen error:%s a",strerror(errno));
        exit(1);
    }
    else{
#ifdef __debug
        fprintf(stdout, "开始监听！\t\n");
#endif
        return 1;
    }
}
int evp_socket::do_serv(){
    while(1) {
		int i, fromlen;
		fprintf(stderr,"准备接收连接\n");
		/* 检查最大连接数 */
		for (i=0;;i++) {
			if (i>=config.server_maxconn) {
				//sleep(1);
				i=0;
				//continue;
				break;//对于长连接到达服务器连接上限不accept
			}
			if (conn[i].socket_fd==0) break;
		}
        /* 服务器阻塞,直到客户程序建立连接 */
        sin_size=sizeof(struct sockaddr_in);
		//主线程循环里调用accept等待客户端连接上来
		memset((char *)&conn[i], 0, sizeof(conn[i]));
		fromlen=sizeof(conn[i].ClientAddr);
        if((conn[i].socket_fd=accept(listen_sockfd,(struct sockaddr *)&conn[i].ClientAddr,(socklen_t *)&fromlen))==-1) {
            fprintf(stderr,"Accept error:%s a",strerror(errno));
            exit(1);
        }
            fprintf(stderr,"已经接收连接\n");
        fdPtr=(int *)malloc(sizeof(int *));//Todo:注意Free
        *fdPtr=conn[i].socket_fd;
        numreq++;
        fprintf(stderr,"Server get connection from %s reqnum %d\r\n",inet_ntoa(client_addr.sin_addr),numreq);//这里有问题为什么是stderr
        pthread_t tid;
#if 1//创建线程
		fprintf(stderr,"准备创建线程\n");
		/** from nullhttp server 属性 start {{{ **/
		pthread_attr_t thr_attr;
		if (pthread_attr_init(&thr_attr)) {
			//logerror("pthread_attr_init()");
			fprintf(stderr,"分配线程属性失败\n");
			exit(1);
		}
		if (pthread_attr_setstacksize(&thr_attr, 65536L)) {
			//logerror("pthread_attr_setstacksize()");
			fprintf(stderr,"设置线程属性失败\n");
			exit(1);
		}
		/** from nullhttp server 属性 end }}} **/
            fprintf(stderr,"准备创建线程step 2\n");
			//防止SIGPIPE导致主线程退出
#ifndef WIN32
			sigset_t signal_mask;
			sigemptyset (&signal_mask);
			sigaddset (&signal_mask, SIGPIPE);
			int rc = pthread_sigmask (SIG_BLOCK, &signal_mask, NULL);
			if (rc != 0)
			{
				printf("block sigpipe error\n");
			}
#endif
        if (pthread_create(&(tid),&thr_attr,function,(void *)fdPtr)){
            fprintf(stderr,"创建线程失败\n");
            exit(1);
        }
            fprintf(stderr,"创建线程完成\n");
#endif
	/** 注释掉！聊天室版本的接受字符串 start {{{ **/
#if 0
        char buff[256];//存放"服务器"返回的信息，相对于telnet键入字符发送过来，这个服务器程序从功能上也可以说是个客户端
        memset(buff,0,sizeof(buff));
#ifdef __debug
        sleep(1);//等待1.X秒，调试用，现在好了
#endif
#if 1
        long flags = fcntl(new_fd,F_GETFD);
        //long flags = fcntl(new_fd,F_GETFL);
        flags |= O_NONBLOCK;
        fcntl(new_fd,F_SETFD,flags);
#endif
        if (!recv(new_fd,buff,sizeof(buff),0)){
            fprintf(stderr,"还没有接受到或者不是信任主机！\n");
        }
        close(new_fd);
#endif
	/** 注释掉！聊天室版本的接受字符串 end }}} **/
    }//end while
}//end evp_socket::do_serv()
int evp_socket::close_server(){
    close(listen_sockfd);
    pthread_mutex_destroy(&criticalArea.mutex);
    exit(0);
}

int main(int argc, char *argv[]){
    pthread_mutex_init(&criticalArea.mutex,NULL);
    evp_socket *obj=new evp_socket();
    obj->init(argc,argv);
    obj->create_sfd();//create socket describe sign
    obj->fill_server_struct();//fill server struct
    obj->bind_sfd();//bind socket describe sign
    obj->listen_sfd();//listen socket descibe sign
    obj->do_serv();//run serv poll
    obj->close_server();
}

	/** 注释掉！废弃的线程函数 start {{{ **/
#if 0//线程函数
void * function(void * arg){
    struct ARG * info;
    info=(struct ARG * )arg;
    //处理客户端细节
    //Todo要把recv加入进来以处理每个客户端
    //
    fprintf(stderr,"thread function run");
//   printf("got a connection form %s",inet_ntoa((info->client).sin_addr));
    //int num;
    free(arg);
    pthread_exit(NULL);
}
#endif

#if 0 
void * function(void * arg){
///    pthread_mutex_lock(&criticalArea.mutex);
    //fprintf(stdout,"i am a thread\n");
///    pthread_mutex_unlock(&criticalArea.mutex);
    int fd;//本线程套接字描述符
    fd=*(int *)arg;//Todo:??相当于把(int *)arg赋值给&fd
    free(arg);//释放死循环里的内存分配
    FILE * fp_in;
    fp_in=fdopen(fd,"r");//将客户端套接字临时存到一文件指针
    char request[255];
    fgets(request,255,fp_in);
    //printf("got a conn to a thread %d req=%s",fd,request);
    //守护进程上面这句要写到syslog里面了
    skip_rest_head(fp_in);
    //处理请求开始
    char msg[200]="OK";
    struct stat bufStat;
    if (fstat(fd,&bufStat)==-1){
        fclose(fp_in);
        return (void *)(-1);
    }
    if (bufStat.st_mode & S_IWRITE) {
        FILE *fp=fdopen(fd,"w");
        if (fp==0 || fp==NULL){
            fclose(fp);
            fclose(fp_in);
            return (void *)(-1);
        }
        fflush(fp);
        fprintf(fp,"HTTP/1.1 200 %s\r\n",msg);
        //fprintf(fp,"Date: Sun, 15 Aug 2010 12:03:38 GMT\r\n");
        fprintf(fp,"Server: evpServer\r\n");
        //fprintf(fp,"Expires: Thu, 19 Nov 1981 08:52:00 GMT\r\n");
        //fprintf(fp,"Vary: Accept-Encoding\r\n");
        fprintf(fp,"Keep-Alive: timeout=15, max=98\r\n");
        //fprintf(fp,"Connection: Keep-Alive\r\n");
        //fprintf(fp,"Transfer-Encoding: chunked\r\n");
        fprintf(fp,"Content-Type: text/html\r\n\r\n");
        fprintf(fp,"<html>hehe<h3>hello thread!</h3></html>\r\n");
        fflush(fp);
        fclose(fp);
    }
    //处理请求结束
    fclose(fp_in);
    pthread_detach(pthread_self());
    return (void *)1;
}
#endif
	/** 注释掉！废弃的线程函数 end }}} **/

void skip_rest_head(FILE *fp){
char buf[255]={0};
while(fgets(buf,255,fp)!=NULL && strcmp(buf,"\r\n")!=0) ;
}

#if 1
void * function(void * arg){
	//pthread_mutex_lock(&criticalArea.mutex);
    int iret = pthread_detach(pthread_self());//改变线程属性为非joinable,这样可以等等pthread_exit自己释放
	fprintf(stderr,"准备创建线程step 3\n");
    int fd;//本线程套接字描述符
    fd=((ARG *)arg)->newfd;
    FILE * fp_in;
	/** 线程循环开始 {{{ **/
	while(1){
		fp_in=fdopen(fd,"r");//将客户端套接字临时存到一文件指针
		char request[255];
		fgets(request,255,fp_in);
		//skip_rest_head(fp_in);
		fprintf(stderr,"准备创建线程step 4\n");
		//处理请求开始
		//char msg[200]="OK";
		struct stat bufStat;
		if (fstat(fd,&bufStat)==-1){
			fclose(fp_in);
			return (void *)(-1);
		}
#if 1
		if (bufStat.st_mode & S_IWRITE) {
#if 0 
			FILE *fp=fdopen(fd,"w");
#if 0 
			if (fp==0 || fp==NULL){
				fclose(fp);
				fclose(fp_in);
				return (void *)(-1);
			}
			fflush(fp);
			fprintf(fp,"HTTP/1.1 200 %s\r\n",msg);
			fprintf(fp,"Server: evpServer\r\n");
			fprintf(fp,"Keep-Alive: timeout=15, max=98\r\n");
			fprintf(fp,"Content-Type: text/html\r\n\r\n");
			fprintf(fp,"<html>hehe<h3>hello thread!</h3></html>\r\n");
			fflush(fp);
#endif
			fclose(fp);
#endif
			fputs(request,stdout);
			char header[60];//define header by 60 bytes
			char body[200];//TODO dynamic accloc
			const char * buf="HTTP/1.1 200 \r\nContent-Type: text/html\r\n\r\n<html>hehe<h3>hello thread!</h3></html>\r\n";
			if(strncasecmp(request,"GOTOP",5)==0)
				buf="y-=1\r\n";
			else if(strncasecmp(request,"GOLEFT",6)==0)
				buf="x-=1\r\n";
			else if (strncasecmp(request,"GOBOTTOM",8)==0)
				buf="y+=1\r\n";
			else if (strncasecmp(request,"GORIGHT",7)==0)
				buf="x+=1\r\n";
			else if (strncasecmp(request,"GM_SAYTOALL",11)==0){
				createHeader(_PROTOCOL_TYPE_UI,gm_saytoall,header);
				createUiBody(gm_saytoall,body);
				int len=strlen(header)+strlen(body);
				char *msg=(char *)malloc(len*sizeof(char));
				msg=strncat(header,body,len);
				//buf=header;
				buf=msg;
				//connect message header & body
			}
			else if (strncasecmp(request,"GM_SAYTOME",10)==0){
				createHeader(_PROTOCOL_TYPE_UI,gm_saytome,header);
				buf=header;
			}
			else 
				buf="";//不用发送\0?
			fputs(buf,stdout);
			int ret;
			ret=send(fd,buf,strlen(buf),0);
			fprintf(stderr,"ret%d",ret);
			if (ret <= 0)
				//fclose(fp_in);
				break;
		}
    }
	/** 线程循环结束 }}} **/
#endif
    //处理请求结束
#if 1//mmoerver不关闭连接，除非出现hack等问题,直接while里面长连接
    fclose(fp_in);
#endif
    free(arg);//释放死循环里的内存分配
    //int iret = pthread_detach(pthread_self());
    if(iret!=0)
    {
        printf( "Can 't   detach   at   my   thread\n ");
        pthread_exit(NULL);   //   显式退出
    } 
    //pthread_mutex_unlock(&criticalArea.mutex);
    return (void *)1;
}
#endif

/**
 *read message header
 **/
/*
 *void readHeader(){
 *    char line[2048];
 *    strncpy(conn[sid].dat->in_RemoteAddr, inet_ntoa(conn[sid].ClientAddr.sin_addr), sizeof(conn[sid].dat->in_RemoteAddr)-1);
 *}
 */

/**
 *创建消息头
 **/
int createHeader(int type,int cmd,char * str){
//消息头：类型\n协议版本\n
switch(type){
	case _PROTOCOL_TYPE_ROLE:
		strncpy(str, (char *)_MESSAGE_ROLE_HEADER,  sizeof(_MESSAGE_ROLE_HEADER));
		break;
	case _PROTOCOL_TYPE_SCENE:
		strncpy(str, (char *)_MESSAGE_SCENE_HEADER, sizeof(_MESSAGE_SCENE_HEADER));
		break;
	case _PROTOCOL_TYPE_WAIT:
		strncpy(str, (char *)_MESSAGE_WAIT_HEADER,  sizeof(_MESSAGE_WAIT_HEADER));
		break;
	case _PROTOCOL_TYPE_UI:
		strncpy(str, (char *)_MESSAGE_UI_HEADER,    sizeof(_MESSAGE_UI_HEADER));
		break;
	default:
		break;
}
strncat(str, "\n", sizeof("\n"));
strncat(str, _MESSAGE_VERSION, sizeof(_MESSAGE_VERSION));
strncat(str, "\n", sizeof("\n"));
char bcmd[4];
sprintf(bcmd,"%d",cmd);
strncat(str, bcmd, sizeof(bcmd));
strncat(str, "\n", sizeof("\n"));
return 0;
}

/**
 *创建界面消息体
 **/
int createUiBody(int cmd,char * str){
	strncat(str, (char *)"<SID>evoup</SID>",sizeof("<SID>evoup</SID>"));
	//strncpy(str, (char *)"<CMD>", sizeof("<CMD>"));
	strncat(str, (char *)"<TContent>time=20&word=",sizeof("<TContent>time=20&word=") );
	strncat(str,(char *)"WELCOME_NEWBIE",sizeof("WELCOME_NEWBIE"));
	strncat(str,(char *)"</TContent>",sizeof("</TContent>"));
	return 0;
}

/**
 *创建消息尾，crc32校验位
 */
int createTail(char * msgStr){
	return 0;
}

/**
 *获取消息总长度的CRC32码(包括CRC32自己的10位数)
 **/
int getMessageCRC32(){

	return 0;
}

