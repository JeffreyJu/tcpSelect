#include "command_define.h"
#include <set>
#include <list>
#include <stdio.h>
#include <string>
using namespace std;

class TcpServer
{
public:
	TcpServer();
	virtual ~TcpServer();
	bool Initialize(unsigned int nPort, unsigned long recvFunc);
	bool SendData(const char * szBuf, size_t nLen, int socket);
	bool GetAddressBySocket(SOCKET m_socket,SOCKADDR_IN &m_address);
	bool UnInitialize();

private:
	static void * AcceptThread(void * pParam);
	static void * OperatorThread(void * pParam);	
	static void * ManageThread(void * pParam);
private:
	int m_server_socket;
	FD_SET m_fdReads;
	CRITICAL_SECTION m_timeCS;
	pCallBack m_operaFunc;
	//int m_client_socket[MAX_LISTEN];
	std::set<int> m_client_socket;
	std::list<ServerData> m_data;
	HANDLE m_pidAccept;
	HANDLE m_pidManage;
};
