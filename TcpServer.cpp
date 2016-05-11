#include "TcpServer.h"


TcpServer::TcpServer()
{
	InitializeCriticalSection(&m_timeCS);
	FD_ZERO(&m_fdReads);
	m_client_socket.clear();
	m_data.clear();
	m_operaFunc = 0;
	m_pidAccept = 0;
	m_pidManage = 0;
}

TcpServer::~TcpServer()
{
	FD_ZERO(&m_fdReads);
	m_client_socket.clear();
	m_data.clear();
	m_operaFunc = NULL;

}

bool TcpServer::SendData(const char * szBuf, size_t nLen, int socket)
{
	if (szBuf == NULL)
	{
		return false;
	}
	int res = send(socket,szBuf,nLen,0);
	if (res == -1)
	{
		return false;
	}
	return true;
}

//���ݴ����߳�
void * TcpServer::OperatorThread(void * pParam)
{
	if(!pParam)
	{
		return 0;
	}
	TcpServer * pThis = (TcpServer*)pParam;
	EnterCriticalSection(&pThis->m_timeCS);
	if(!pThis->m_data.empty())
	{
		ServerData data = pThis->m_data.front();
		pThis->m_data.pop_front();
		if(pThis->m_operaFunc)
		{
			//�����ݽ����ص���������
			pThis->m_operaFunc((char *)&data, sizeof(data), data.socket);
		}
	}
	LeaveCriticalSection(&pThis->m_timeCS);
	return 0;
}

bool TcpServer::Initialize(unsigned int nPort, unsigned long recvFunc)
{
	if(0 != recvFunc)
	{
		//���ûص�����
		m_operaFunc = (pCallBack)recvFunc;
	}
	//�ȷ���ʼ��
	UnInitialize();
	//����socket
	m_server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(-1 == m_server_socket)
	{
		TRACE("socket error:%m\n");
		closesocket(m_server_socket);
		shutdown(m_server_socket,2);
		return false;
	}

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(nPort);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	int res = bind(m_server_socket, (sockaddr*)&serverAddr, sizeof(serverAddr));
	if(-1 == res)
	{
		TRACE("bind error:%d\n",GetLastError());
		closesocket(m_server_socket);
		shutdown(m_server_socket,2);
		return false;
	}
	//����
	res = listen(m_server_socket, MAX_LISTEN);
	if(-1 == res)
	{
		TRACE("listen error:%m\n");
		closesocket(m_server_socket);
		shutdown(m_server_socket,2);
		return false;
	}
	//�����߳̽���socket����
	DWORD		dwThreadID;
	m_pidAccept = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)AcceptThread, PVOID(this), 0, &dwThreadID);

	//���������߳�
	DWORD		dwThreadID1;
	m_pidManage = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ManageThread, PVOID(this), 0, &dwThreadID1);

	return true;
}

//����socket�����߳�
void * TcpServer::AcceptThread(void * pParam)
{
	if(!pParam)
	{
		TRACE("param is null\n");
		return 0;
	}
	TcpServer * pThis = (TcpServer*)pParam;
	int nMax_fd = 0;
	int fd ;
	int i = 0;
	while(1)
	{
		FD_ZERO(&pThis->m_fdReads);
		FD_SET(pThis->m_server_socket, &pThis->m_fdReads);
		
		nMax_fd = nMax_fd > pThis->m_server_socket ? nMax_fd : pThis->m_server_socket;
		std::set<int>::iterator iter = pThis->m_client_socket.begin();
		
		for(; iter != pThis->m_client_socket.end(); ++iter)
		{
			nMax_fd = nMax_fd > (*iter) ? nMax_fd : (*iter);
			FD_SET(*iter, &pThis->m_fdReads);
		}
		
	//	if(!pThis->m_client_socket.empty())
	//	{
	//		--iter;

	//		nMax_fd = nMax_fd > (*iter) ? nMax_fd : (*iter);
	//	}
		
		int res = select(nMax_fd + 1, &pThis->m_fdReads, 0, 0, NULL);
		if(-1 == res)
		{
			TRACE("select error:%m\n");
			continue;
		}
		TRACE("select success\n");
		
		if(FD_ISSET(pThis->m_server_socket, &pThis->m_fdReads))
		{
			//�����µ�����
			fd = accept(pThis->m_server_socket, 0,0);
			if(-1 == fd)
			{
				TRACE("accept error:%m\n");
				continue;
			}
			//��������ӵĿͻ�
			pThis->m_client_socket.insert(fd);
			TRACE("���ӳɹ�\n");
		}
		
		for(iter = pThis->m_client_socket.begin();iter != pThis->m_client_socket.end();)
		{
			//�жϿͻ��Ƿ�ɶ�
			if(-1 != *iter && FD_ISSET(*iter, &pThis->m_fdReads))	
			{
				char buf[MAX_DATA_LEN] = {0};
				res = recv(*iter, buf, sizeof(buf), 0);
				if(res > 0)
				{
					ServerData serverData = {0};
					memcpy(serverData.buf, buf, res);
					serverData.nLen = res;
					serverData.socket = *iter;
					pThis->GetAddressBySocket(serverData.socket,serverData.clientaddr);
					pThis->SendData((const char*)serverData.buf,serverData.nLen,serverData.socket);
					EnterCriticalSection(&pThis->m_timeCS);
					pThis->m_data.push_back(serverData);
					LeaveCriticalSection(&pThis->m_timeCS);
				}
				else if(0 == res)
				{
					TRACE("�ͻ����˳�\n");
					pThis->m_client_socket.erase(iter++);
					
					
				}
				else
				{
					TRACE("recv error\n");
				}	
			}
			else
			{
				iter++;
			}
				
		
			
		}
		
	}
}

//�����̣߳����ڴ��������߳�
void * TcpServer::ManageThread(void * pParam)
{
	if(!pParam)
	{
		return 0;
	}
	int pid;
	TcpServer * pThis = (TcpServer *)pParam;
	while(1)
	{

		EnterCriticalSection(&pThis->m_timeCS);
		int nCount = pThis->m_data.size();
		LeaveCriticalSection(&pThis->m_timeCS);
		if(nCount > 0)
		{
			HANDLE pid = 0;
			DWORD dwThreadId;
			//���������߳�
			pid = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)AcceptThread,  pParam,0, &dwThreadId);
			if( pid < 0 )
			{
				TRACE("creae operator thread failed\n");
			}
		}
		Sleep(100);
	}
}

bool TcpServer::UnInitialize()
{
	closesocket(m_server_socket);;
	shutdown(m_server_socket,2);
	for(std::set<int>::iterator iter = m_client_socket.begin(); iter != m_client_socket.end(); ++iter)
	{
		closesocket(*iter);
	}
	m_client_socket.clear();
	if(0 != m_pidAccept)
	{
		WaitForSingleObject(m_pidAccept,0);
	}
	if(0 != m_pidManage)
	{
		WaitForSingleObject(m_pidManage,0);
	}
	return true;
}

bool TcpServer::GetAddressBySocket(SOCKET m_socket,SOCKADDR_IN &m_address)
{
	memset(&m_address, 0, sizeof(m_address));
	int nAddrLen = sizeof(m_address);

	if(::getpeername(m_socket, (SOCKADDR*)&m_address, &nAddrLen) != 0)
	{
		TRACE("Get IP address by socket failed!n");
		return false;
	}

	//��ȡIP��Port
	//cout<<"IP: "<<::inet_ntoa(m_address.sin_addr)<<"  PORT: "<<ntohs(m_address.sin_port)<<endl;
	return true;
}