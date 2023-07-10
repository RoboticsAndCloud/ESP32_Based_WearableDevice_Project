#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<syslog.h>
#include<netinet/tcp.h>
#include<sys/types.h>

#include<unistd.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>


#include "esp_wifi.h"
#include "app_camera.h"
#include "app_tcp.h"


//PacketType
#define COM_PACKET_TYPE_REQUEST 0
#define COM_PACKET_TYPE_RESPONSE 1
#define COM_PACKET_TYPE_NOTICE 1

// MSG_TYPE
#define COM_MSG_TYPE_TAKE_PHOTO 0x0401
#define COM_MSG_TYPE_WEARABLE_DEVICE 0X0402

#define COM_HEAD_LEN 4 // 13 Byte


#define PROGRAM "SmartHome_ESP_Client"

//#define Test_ip "127.0.0.1"
//#define Test_ip "10.227.234.19"
#define Test_ip "192.168.1.138"

#define SERVER_PORT 3333
#define MAX_BUFFER 1000




int id = 1;

typedef struct
{
    unsigned char PacketType;
    unsigned short MsgType;
    unsigned char Other;

//    unsigned int UserID;
//    unsigned short MsgLength;
//    unsigned int TransactionID;
}__attribute__((packed)) st_com_head; /*sockets communicate struct */

typedef struct
{
    st_com_head comHead;
    char msg[0];/**/
}__attribute__((packed)) st_com_msg; /*sockets communicate struct */


int createTransactionID()
{
    return 100;
}
int test_send(int socket,const st_com_msg *msg,int len)
{
    int sizeByte;
    int sendSocket = socket;
    int sendLen = len;
    char sendBuf[MAX_BUFFER];
    memcpy(sendBuf, msg,sendLen);
    sizeByte = send(socket, sendBuf,sendLen, 0);
	if (sizeByte == -1)
	{
	    printf("In test_send, status = -1 \n");
	    return -1;
	}
    #ifdef DEBUG
	    printf("test_send to socket %d is ok..and msg sMsgType  %d\n",socket,msg.comHead.MsgType);
	#endif
	return sizeByte;
}

st_com_head createMsgComHead(unsigned char packetType,unsigned short MsgType,unsigned int UserID,unsigned short len,unsigned int TransactionID)
{
	st_com_head comHead;
	//int temp_g_userID = 1;//g_userID need
	comHead.PacketType = packetType;
	comHead.MsgType = htons(MsgType);
    comHead.Other = packetType;

//	comHead.UserID = htonl(UserID);
//	comHead.MsgLength = htons(len);
//    comHead.TransactionID = htonl(TransactionID);
	return comHead;
}
void prt_com_head(st_com_head comHead)
{
    unsigned char PacketType;
    unsigned short MsgType;
//    unsigned int UserID;
//    unsigned short MsgLength;
//    unsigned int TransactionID;
	uint8_t tag;
	uint8_t len;

    PacketType = comHead.PacketType;
    MsgType = ntohs(comHead.MsgType);//¿¿¿¿¿
//	UserID = ntohl(comHead.UserID);
//    MsgLength = ntohs(comHead.MsgLength);
//	TransactionID = ntohl(comHead.TransactionID);

	printf("----------------------------------\n");

    printf("--------in print info start-----------------------\n");
	printf("network endian: PacketType %d \n",comHead.PacketType);
	printf("network endian: MsgType %d \n",comHead.MsgType);
//	printf("network endian: UserID %d \n",comHead.UserID);
//	printf("network endian: MsgLength %d \n",comHead.MsgLength);
//	printf("network endian: TransactionID %d \n",comHead.TransactionID);

	printf("----------------------------------\n");

    printf("host endian: PacketType %d \n",PacketType);
	printf("host endian: MsgType %x \n",MsgType);
//	printf("host endian: UserID %d \n",UserID);
//	printf("host endian: MsgLength %d \n",MsgLength);
//	printf("host endian: TransactionID %d \n",TransactionID);
	printf("--------in print info end-------------------------\n");
	printf("----------------------------------\n");
}

int device_request(int socket,unsigned short MsgType)
{
	st_com_msg *send_msg, *send_msg_temp;

    int TransactionID = createTransactionID();
    unsigned short  len = (COM_HEAD_LEN);
	send_msg = (st_com_msg *)malloc(len);
	send_msg->comHead = createMsgComHead(0,MsgType,1,len,TransactionID);


	char *msg = "Just for test";
	len = sizeof(msg);


	int sizeByte;
    int sendSocket = socket;
    int sendLen = len;
    char sendBuf[MAX_BUFFER];
    memcpy(sendBuf, msg, sendLen);
    sizeByte = send(socket, sendBuf,sendLen, 0);


	if(sizeByte>0)
    {
        printf("-----------------------------------\n");
        printf("----test_send invalid_request  success-----------------\n");
        printf("-----------------------------------\n");
    }
    else
    {
        printf("-----------------------------------\n");
        printf("----test_send invalid_request error-----------------\n");
        printf("-----------------------------------\n");
        return -1;
    }

    sleep(5);

    sizeByte = send(socket, sendBuf,sendLen, 0);
	if(sizeByte>0)
    {
        printf("-----------------------------------\n");
        printf("----test_send second invalid_request  success-----------------\n");
        printf("-----------------------------------\n");
    }
    else
    {
        printf("-----------------------------------\n");
        printf("----test_send second invalid_request error-----------------\n");
        printf("-----------------------------------\n");
        return -1;
    }

    free(send_msg);

    return 0;

}

int take_photo_request(int socket,unsigned short MsgType)
{
	st_com_msg *send_msg, *send_msg_temp;

    int TransactionID = createTransactionID();
    unsigned short  len = (COM_HEAD_LEN);
	send_msg = (st_com_msg *)malloc(len);
	send_msg->comHead = createMsgComHead(0,MsgType,1,len,TransactionID);

	prt_com_head(send_msg->comHead);

	if(test_send(socket,send_msg,len)>0)
    {
        printf("-----------------------------------\n");
        printf("----test_send take_photo_request  success-----------------\n");
        printf("-----------------------------------\n");
    }
    else
    {
        printf("-----------------------------------\n");
        printf("----test_send take_photo_request error-----------------\n");
        printf("-----------------------------------\n");
        return -1;
    }

    free(send_msg);

    return 0;

}

int connectRemoteSocket(const char host[], const unsigned short port, int *this_m_sock, struct sockaddr_in *this_m_addr)
{
	int on = 1;
	int status;
	//create
	*this_m_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(*this_m_sock == -1)
	{
		printf("Socket invalid\n");
		return -1;
	}

	if(setsockopt(*this_m_sock, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on)) == -1)
	{
		printf("Failed at setsockopt.\n");
		return -1;
	}

	//connect
	this_m_addr->sin_family = AF_INET;
	this_m_addr->sin_port = htons(port);

	status = inet_pton(AF_INET, host, &(this_m_addr->sin_addr));

	if(errno == EAFNOSUPPORT)
	{
		printf("EAFNOSUPPORT\n");
		return -1;
	}

	status = connect(*this_m_sock, (struct sockaddr *)this_m_addr, sizeof(*this_m_addr));

	if(status != 0)
	{
		printf("In connect, status = %d, errno = %d\n", status, errno);
		return -1;
	}

	return 0;
}

void func_deal_msg(int sockfd)
{
	char buff[MAX_BUFFER];
	void *imageSize = malloc(sizeof(uint32_t));

	FILE *jpg;
	const char *currtime_string;

	int flag = 0;
	int pic_count = 0;

	char *packet_msg;
    st_com_head comHead;
	int i;
    unsigned char PacketType;
    unsigned short MsgType;
    unsigned int UserID;
    unsigned short MsgLength;
    unsigned int TransactionID;
	int reply;

	char recvBuf[MAX_BUFFER];
    int valread;

	while(1) {

	valread = recv(sockfd, recvBuf, MAX_BUFFER, 0);

	packet_msg = recvBuf;

	printf("received msg valread =%d \n", valread);
	// robot send command
	// or recvBuf == 'TAKE_PHOTO'
	if (valread <= 0 ) {
	    break;
	}
	if (valread >= COM_HEAD_LEN) {
	    // todo send capture command to wearable device
	    memcpy(&comHead,recvBuf,sizeof(st_com_head));
	    PacketType = comHead.PacketType;
        MsgType = ntohs(comHead.MsgType);

	    // todo if MsgType ==COM_MSG_TYPE_TAKE_PHOTO
	    printf("received msg from server ===================\n");

	    prt_com_head(comHead);

	    if (MsgType == COM_MSG_TYPE_TAKE_PHOTO && PacketType == 0) {
	        // take photos
	        app_tcp_main();
	        break;
	    }
	}

	} // end while
}


int client_task(int argc, char *argv[])
{
	struct sockaddr_in address;
    int userSocket;
 	socklen_t addrlen;
	int state;
    int i;
    char mng_name[8];


   	state = connectRemoteSocket(Test_ip, SERVER_PORT, &userSocket, &address);

    char buf[1024];
	int bufsize;
	socklen_t opt = sizeof(int);
    int ret = getsockopt(userSocket,SOL_SOCKET,SO_SNDBUF,&bufsize,&opt);
	printf("buf size %d \n",bufsize);

	while (1) {
	    func_deal_msg(userSocket);
	}

    return userSocket;
}
