#include "StdAfx.h"

#include <map>
using namespace std;

const int MAX_LISTEN = 3000;
const int SERVER_LISTEN_PORT = 20685;
const int MAX_DATA_LEN = 4096;

typedef struct
{
	unsigned char buf[MAX_DATA_LEN];
	size_t nLen;
	int socket;
	SOCKADDR_IN clientaddr;
}ServerData, *pServerData;

typedef void (*pCallBack)(const char * szBuf, size_t nLen, int socket);



