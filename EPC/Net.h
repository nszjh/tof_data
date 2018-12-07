#ifndef  NET__H
#define  NET__H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#ifdef WIN32

//#include <winsock.h>
//#include <arpa/inet.h>
//#include <windows.h>
#include <WS2tcpip.h>
//#define WIN32_LEAN_AND_MEAN 
//#include <windows.h>
#else
//socket
#include <sys/types.h>  
#include <sys/socket.h>  
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <unistd.h>
#endif



class NetRequest
{
public:
    NetRequest(){
        memset(ip_address, 0, sizeof(ip_address));
    };
    int Request(char *receivebuf, int buffer_size, const char *sendtos);
    int SetServer(const char *ipAddr);
protected:
    int Connect(int port, const char* address);
#ifdef WIN32
	SOCKET _sockfd;
#else
	int _sockfd;
#endif
	char ip_address[20];
};

#endif