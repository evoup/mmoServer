/*-
 * Project
 *
 * Copyright (c) 2007-2008 by Company.
 * All rights reserved.
 * Author: evoup <webmaster@akata2.vicp.net>
 *
 * $Id: main.h,v 1.6 2010/11/16 10:26:45 root Exp $
 */
#ifndef INCLUDE_MAIN_H
#define INCLUDE_MAIN_H
//
#ifdef __cplusplus//for c++
#include <string.h>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#endif
//#include "word.h"
#define MAX_TEXT 2048 
#define _MESSAGE_ROLE_HEADER  "EVP::EVP_HEADER_ROLE_FAMILY"
#define _MESSAGE_SCENE_HEADER "EVP::EVP_HEADER_SCENE_FAMILY"
#define _MESSAGE_WAIT_HEADER  "EVP::EVP_HEADER_WAIT_FAMILY"
#define _MESSAGE_UI_HEADER    "EVP::EVP_HEADER_UI_FAMILY"
#define _MESSAGE_VERSION      "EVP::EVP_SERVER_VERSION_07DA"
/* 消息类型 */
#define _PROTOCOL_TYPE_ROLE   0
#define _PROTOCOL_TYPE_SCENE  1
#define _PROTOCOL_TYPE_WAIT   2
#define _PROTOCOL_TYPE_UI     3

//#define _CMD_GM_SAYTOALL      0
//typedef enum{
	//ROLE  = 0,
	//SCENE = 1,
	//WAIT  = 2,
	//UI    = 3
//}PROTOCOL_TYPE;
/* GM消息 */
enum gm_cmd {
gm_saytoall    = 800,
gm_saytome     = 801,
gm_kickplayer  = 802,
gm_addplayerhp = 803,
gm_killplayer  = 804
};
/*UI消息*/
typedef enum{
	SERVER_SAY_TO_ME  = 0,
	SERVER_RAIN_SMALL = 1,
	SERVER_RAIN_HEAVY = 2
}UI_MESSAGE_TYPE;
//from nullhttpd src , connect struct
typedef struct {
	// incoming data
	char in_Connection[16];
	int  in_ContentLength;
	char in_ContentType[128];
	//char in_Cookie[1024];
	char in_Host[64];
	//char in_IfModifiedSince[64];
	char in_PathInfo[128];
	char in_Protocol[16];
	//char in_QueryString[1024];
	//char in_Referer[128];
	char in_RemoteAddr[16];
	int  in_RemotePort;
	//char in_RequestMethod[8];
	//char in_RequestURI[1024];
	//char in_ScriptName[128];
	char in_UserAgent[128];
	// outgoing data
	short int out_status;
	//char out_CacheControl[16];
	char out_Connection[16];
	int  out_ContentLength;
	char out_Date[64];
	char out_Expires[64];
	//char out_LastModified[64];
	char out_Pragma[16];
	char out_Protocol[16];
	char out_Server[128];
	//char out_ContentType[128];
	//char out_ReplyData[MAX_REPLYSIZE];
	short int out_headdone;
	short int out_bodydone;
	short int out_flushed;
	// user data
	char envbuf[8192];
	//added for mmoserver start
	//user_t *ob; /* 指向处理服务器端逻辑的结构 */
	int fd; /* socket连接 */
	struct sockaddr_in addr; /* 连接的地址信息 */
	char text[MAX_TEXT]; /* 接收的消息缓冲 */
	int text_end; /* 接收消息缓冲的尾指针 */
	int text_start; /* 接收消息缓冲的头指针 */
	int last_time; /* 上一条消息是什么时候接收到的 */
	struct timeval latency; /* 客户端本地时间和服务器本地时间的差值 */
	struct timeval last_confirm_time; /* 上一次验证的时间 */
	short is_confirmed; /* 该连接是否通过验证过 */
	int ping_num; /* 该客户端到服务器端的ping值 */
	int ping_ticker; /* 多少个IO周期处理更新一次ping值 */
	int message_length; /* 发送缓冲消息长度 */
	char message_buf[MAX_TEXT]; /* 发送缓冲区 */
	int iflags; /* 该连接的状态 */
	//added for mmoserver end 
} CONNDATA;

typedef struct {
	pthread_t handle;
	unsigned long int id;
	short int socket_fd;
	struct sockaddr_in ClientAddr;
	time_t ctime; // Creation time
	time_t atime; // Last Access time
	//char *PostData;
	CONNDATA *dat;
} CONNECTION;

typedef struct {
	char config_filename[255];
	char server_base_dir[255];
	char server_bin_dir[255];
	//char server_cgi_dir[255];
	char server_etc_dir[255];
	//char server_htdocs_dir[255];
	char server_hostname[64];
	short int server_port;
	short int server_loglevel;
	short int server_maxconn;
	short int server_maxidle;
} CONFIG;
//CONFIG config;移到config.h了
//CONNECTION *conn;移到main.c里了
int createHeader(int, int, char *);
int createUiBody(int cmd,char * str);
int createTail(char * msgStr);
/* 即时消息 */
typedef struct {
int messageId;
char * text;
}liveMessage;
/**/
typedef struct {
int messageId;
int sid;
}unBoardedLm;
typedef struct {
int messageId;
int sid;
}boardedLm;
//
#endif
