#include <iostream>
#include "Net.h"

#ifdef WIN32
#pragma comment(lib,"wsock32")
#define close closesocket  
#endif

int NetRequest::Connect(int port, const char* address)
{
    struct sockaddr_in ser_addr;

	_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (_sockfd < 0)
	{
		printf("error2!\n");
		close(_sockfd);
		return -1;
	}
	//构建服务器套接字地址  
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons(port);
#ifdef WIN32
	ser_addr.sin_addr.S_un.S_addr = inet_addr(address);
	//inet_pton(AF_INET, address, (void *)&ser_addr.sin_addr.s_addr);
#else
	inet_pton(AF_INET, address, (void *)&ser_addr.sin_addr.s_addr);
#endif
	int flag = connect(_sockfd, (struct sockaddr *)&ser_addr, sizeof(ser_addr));

	return flag;
}

int NetRequest::SetServer(const char *ipAddr)
{
    strncpy(ip_address, ipAddr, sizeof(ip_address));
	ip_address[sizeof(ip_address) - 1] = '\0';
	return 0;
}

int NetRequest::Request(char * receivebuf, int buffer_size, const char * sendtos)
{
	int savepoint = 0;
	int recvnum = 0;
	char buf[1024];
	fd_set read_set;
	struct timeval t_o;
	
    // std::cout << sendtos << std::endl;
	t_o.tv_sec  = 1; 
	t_o.tv_usec = 0;

	int _port = 50660;
	if (Connect(_port, ip_address) < 0)
	{
		std::cout << "failed to connect to " << ip_address << std::endl;
		close(_sockfd);
        return 0;
	}
	FD_ZERO(&read_set);
	FD_SET(_sockfd, &read_set);
	send(_sockfd, sendtos, strlen(sendtos), 0);
	if (buffer_size == 0) //no need to receive data
	{
		close(_sockfd);
        return 0;
	}	
	while (select(_sockfd + 1, &read_set, NULL, NULL, &t_o) > 0)
	{
		if (FD_ISSET(_sockfd, &read_set))
		{
			recvnum = recv(_sockfd, receivebuf+savepoint, 4096, 0);
			if (recvnum <= 0)//当系统重新启动的时候可能触发
			{
				break;
			}
			savepoint += recvnum;
		}
		else
		{
			std::cout << "timeout!\n";
			break;
		}
	}
	close(_sockfd);
	return savepoint;
}