#ifndef _SYSOCKET_H_
#define _SYSOCKET_H_

#include <winsock2.h>
#pragma comment (lib, "Ws2_32.lib")
//[Thread lib]
#include <Windows.h>
#include <process.h>    /* _beginthread, _endthread */

#include <map>
#include <vector>
#ifdef BCB
#include <Windows.h>
#else
#endif


//////////////////////////////////////////////////////////////////////////
#define SYTCPSOCKET_BUFFER_LENGTH	1024
enum SYTCPSocketEventStatus
{
	SYTCPSOCKET_CLOSE = 0,
	SYTCPSOCKET_CONNECTFAULT,
	SYTCPSOCKET_CONNECTED,
	SYTCPSOCKET_DISCONNECT,
	SYTCPSOCKET_LISTENED,
	SYTCPSOCKET_RECVDATA
};

struct SYTCPEvent
{
	SYTCPSocketEventStatus	Status;
	wchar_t					wszMsg[256];
	char					szData[SYTCPSOCKET_BUFFER_LENGTH];
	USHORT					Port;
	int						iLen;
};


struct SYTCPConnectParam
{
	PVOID   pSYSocket;
	SOCKET  Socket;
	HANDLE  hRecvProcThread;
	sockaddr_in addrClient;
	bool    bClose;
	char	recv_data[SYTCPSOCKET_BUFFER_LENGTH]; //for Client
};
//////////////////////////////////////////////////////////////////////////

DWORD WINAPI ListenProcThread(void *pParam);
DWORD WINAPI ServerRecvProcThread(void *pParam);
DWORD WINAPI ClientConnectProcThread(void *pParam);
DWORD WINAPI ClientRecvProcThread(void *pParam);
DWORD WINAPI SendProcThread(void *pParam);
DWORD WINAPI CheckProcThread(void *pParam);
//////////////////////////////////////////////////////////////////////////

class SYTCPSocket
{
public:
	//Server
	USHORT  LocalPort;
	HANDLE _hListenProcThread;
	std::map<USHORT, SYTCPConnectParam*>_mapClientList; 
	//Client
	char*   RemoteHost;
	USHORT	RemotePort;
	HANDLE	_hConnectProcThread;
	HANDLE	_hClientRecvProcThread;
	//Common
	SOCKET  Socket;
	HANDLE	_hSendProcThread;
	std::vector<char*>* _pVecSendData;
	//Event
	void (*OnEvent)(SYTCPSocket *sender, SYTCPEvent e);
	//////////////////////////////////////////////////////////////////////////
	SYTCPSocket()
	{
		LocalPort = 0;
		_hListenProcThread	= NULL;

		RemoteHost = 0;
		RemotePort = 0;
		_hConnectProcThread = NULL;
		_hClientRecvProcThread = NULL;

		Socket = NULL;
		_hSendProcThread	= NULL;
		_pVecSendData = new std::vector<char*>;
		OnEvent = NULL;
	}
	~SYTCPSocket()
	{
		this->Close();

		delete _pVecSendData;
	}

	void Close()
	{
		if (/*_hListenProcThread == NULL && _hConnectProcThread == NULL &&*/ Socket == NULL && _hSendProcThread == NULL) return;
		
		
		
		//Server
		if (_hListenProcThread)
		{
			TerminateThread(_hListenProcThread, 0);
			_hListenProcThread = NULL;

			std::map<USHORT, SYTCPConnectParam*>::iterator iter;
			for ( iter=_mapClientList.begin() ; iter != _mapClientList.end(); iter++ )
			{
				SYTCPConnectParam *item = (SYTCPConnectParam*)iter->second;		
				
				shutdown(item->Socket, SD_BOTH);		
				closesocket(item->Socket);				
				WSACleanup();
				item->Socket = NULL;
				TerminateThread(item->hRecvProcThread, 0);
				item->hRecvProcThread = NULL;
				//free
				free(item);
			}//end for
			_mapClientList.clear();

		}

		//Client
		if (_hConnectProcThread)
		{
			TerminateThread(_hConnectProcThread, 0);
			_hConnectProcThread = NULL;
		}

		//Client
		if (_hClientRecvProcThread)
		{
			TerminateThread(_hClientRecvProcThread, 0);
			_hClientRecvProcThread = NULL;		
		}
			
		//Server & Client
		if (_hSendProcThread)
		{
			TerminateThread(_hSendProcThread, 0);
			_hSendProcThread = NULL;		
		}

		//Server
		while(!_pVecSendData->empty())
		{
			free(_pVecSendData->back());
			_pVecSendData->pop_back();
		}

		
		//Server & Client
		if (Socket)
		{
			shutdown(Socket, SD_BOTH);		
			closesocket(Socket);
			WSACleanup();
			Socket = NULL;		
		}

		//Server & Client
		SYTCPEvent e;
		ZeroMemory(&e, sizeof(e));
		e.Status = SYTCPSOCKET_CLOSE;
		this->OnEvent(this, e);
	}

	void Listen(void)
	{
		this->Close();
		//Listen
		_hListenProcThread = CreateThread(NULL, 0, ListenProcThread, this, 0, NULL);
		
	}

	void Connect(void)
	{
		this->Close();
		//Connect
		_hConnectProcThread = CreateThread(NULL, 0, ClientConnectProcThread, this, 0, NULL);		
	}


	void Send(const char* pData)
	{

		if (_hSendProcThread == NULL) return; //還未連線

		int iLen = (int)strlen(pData)+1;
		char *pBuffer = (char *)malloc(sizeof(char)*(iLen));
		memcpy(pBuffer, pData, sizeof(char)*(iLen));
		_pVecSendData->push_back(pBuffer);
	}


	void GetNowOnlineClient()
	{

		WCHAR wszBuf[256];

		if (_hListenProcThread)
		{
			OutputDebugString(L"=======Online List=======\n");
		
			for(std::map<USHORT, SYTCPConnectParam*>::iterator iter=_mapClientList.begin(); iter!=_mapClientList.end(); iter++) 
			{  
				memset(wszBuf, 0, sizeof(wszBuf));
				SYTCPConnectParam *pClient = (*iter).second;
				wsprintf(wszBuf, L"Server had a connection from (%S, %d)\n", inet_ntoa(pClient->addrClient.sin_addr), ntohs(pClient->addrClient.sin_port));
				OutputDebugString(wszBuf);				
			}//end for
			
		}//end if (_hListenProcThread)
	}

	SOCKET GetSocketWithPort(USHORT port)
	{
		USHORT key = port;
		std::map<USHORT, SYTCPConnectParam*>::iterator iter = _mapClientList.find(key);		
		if (iter == _mapClientList.end())
		{
			///No find
		}else{			
			return ((SYTCPConnectParam*)iter->second)->Socket;			
		}
		return NULL;
	}

	void CloseSocketWithPort(USHORT port)
	{
		USHORT key = port;
		std::map<USHORT, SYTCPConnectParam*>::iterator iter = _mapClientList.find(key);		
		if (iter == _mapClientList.end())
		{
			///No find
		}else{	
			SYTCPConnectParam *item = (SYTCPConnectParam*)iter->second;		
			item->bClose = true;			
		}
		
	}
};
#endif



//////////////////////////////////////////////////////////////////////////
DWORD WINAPI ListenProcThread(void *pParam)
{
	SYTCPSocket *pSYSocket = (SYTCPSocket*)pParam;	


	wchar_t wszBuf[256];

	//----------------------
	// Initialize Winsock
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != NO_ERROR) {
		//OutputDebugString(L"Error at WSAStartup()\n");
		SYTCPEvent e;
		ZeroMemory(&e, sizeof(e));
		e.Status = SYTCPSOCKET_CONNECTFAULT;
		wsprintf(e.wszMsg, L"%s", L"Error at WSAStartup()\n");
		pSYSocket->OnEvent(pSYSocket, e);
		return false;
	}			

	//----------------------
	// Create a SOCKET for listening for
	// incoming connection requests.		
	pSYSocket->Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (pSYSocket->Socket == INVALID_SOCKET) 
	{
		memset(wszBuf, 0, sizeof(wszBuf));
		wsprintf(wszBuf, L"Error at socket(): %ld\n", WSAGetLastError());
		//OutputDebugString(wszBuf);		
		WSACleanup();
		SYTCPEvent e;
		ZeroMemory(&e, sizeof(e));
		e.Status = SYTCPSOCKET_CONNECTFAULT;
		wsprintf(e.wszMsg, L"%s", wszBuf);
		pSYSocket->OnEvent(pSYSocket, e);
		return false;
	}


	//----------------------
	// The sockaddr_in structure specifies the address family,
	// IP address, and port for the socket that is being bound.	
	sockaddr_in service_addr;
	service_addr.sin_family = AF_INET;
	service_addr.sin_addr.s_addr = INADDR_ANY;
	service_addr.sin_port = htons(pSYSocket->LocalPort);



	if (bind( pSYSocket->Socket, (SOCKADDR*) &service_addr, sizeof(service_addr)) == SOCKET_ERROR) 
	{				
		//OutputDebugString(L"bind() failed.\n");
		closesocket(pSYSocket->Socket);
		WSACleanup();
		SYTCPEvent e;
		ZeroMemory(&e, sizeof(e));
		e.Status = SYTCPSOCKET_CONNECTFAULT;
		wsprintf(e.wszMsg, L"%s", L"bind() failed.\n");
		pSYSocket->OnEvent(pSYSocket, e);
		return false;
	}


	//----------------------
	// Listen for incoming connection requests 
	// on the created socket
	if (listen( pSYSocket->Socket, 1 ) == SOCKET_ERROR)
	{
		//OutputDebugString(L"Error listening on socket.\n");
		closesocket(pSYSocket->Socket);
		WSACleanup();
		SYTCPEvent e;
		ZeroMemory(&e, sizeof(e));
		e.Status = SYTCPSOCKET_CONNECTFAULT;
		wsprintf(e.wszMsg, L"%s", L"Error listening on socket.\n");
		pSYSocket->OnEvent(pSYSocket, e);
		return false;
	}


	memset(wszBuf, 0, sizeof(wszBuf));
	wsprintf(wszBuf, L"Listening on socket....Port:%d\n", pSYSocket->LocalPort);
	//OutputDebugString(wszBuf);	

	//Send
	pSYSocket->_hSendProcThread = CreateThread(NULL, 0, SendProcThread, pSYSocket, 0, NULL);

	SYTCPEvent e;
	ZeroMemory(&e, sizeof(e));
	e.Status = SYTCPSOCKET_LISTENED;
	wsprintf(e.wszMsg, L"%s", wszBuf);
	pSYSocket->OnEvent(pSYSocket, e);

	int sin_size = sizeof(struct sockaddr_in);

	while(true)
	{  
		
		sockaddr_in client_addr;
		SOCKET clientSocket = accept(pSYSocket->Socket, (struct sockaddr *)&client_addr, &sin_size);
	
		SYTCPConnectParam *pRecvParam = (SYTCPConnectParam*)malloc(sizeof(SYTCPConnectParam));
		memset(pRecvParam, 0, sizeof(SYTCPConnectParam));
		pRecvParam->pSYSocket = pSYSocket; 
		pRecvParam->Socket = clientSocket;
		pRecvParam->addrClient = client_addr;



		//memset(wszBuf, 0, sizeof(wszBuf));
		//wsprintf(wszBuf, L"Server got a connection from (%S, %d)\n", inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
		//OutputDebugString(wszBuf);
				
		pRecvParam->hRecvProcThread = CreateThread(NULL, 0, ServerRecvProcThread, pRecvParam, 0, NULL);	

		USHORT key = pRecvParam->addrClient.sin_port;
		/* 
		//一定不會重複，因為每次連線Server 會給一個port
		std::map<USHORT, SYTCPConnectParam*>::iterator iter = pSYSocket->_mapClientList.find(key);		
		if (iter == pSYSocket->_mapClientList.end())
		{
			///No find
		}else{
			//free
			free((SYTCPConnectParam*)iter->second);
			//remove
			pSYSocket->_mapClientList.erase(iter);
		}*/

		pSYSocket->_mapClientList.insert(std::map<USHORT, SYTCPConnectParam*>::value_type(key, pRecvParam)); 

		SYTCPEvent e;
		ZeroMemory(&e, sizeof(e));
		e.Status = SYTCPSOCKET_CONNECTED;
		wsprintf(e.wszMsg, L"Server got a connection from (%S, %d)\n", inet_ntoa(pRecvParam->addrClient.sin_addr),ntohs(pRecvParam->addrClient.sin_port));
		pSYSocket->OnEvent(pSYSocket, e);
	
		
		//delete pRecvParam;
		//closesocket(subSocket);
		
	}

	pSYSocket->Close();
	
}

//////////////////////////////////////////////////////////////////////////
DWORD WINAPI ServerRecvProcThread(void *pParam)
{
	SYTCPConnectParam *pRecvParm = (SYTCPConnectParam *)pParam;
	SYTCPSocket *pThis = (SYTCPSocket *)pRecvParm->pSYSocket;
	SOCKET Socket = pRecvParm->Socket;

	//set Timeout
	
	struct timeval tv;
	tv.tv_sec = 2;  //2s
	tv.tv_usec = 0; //500000 = 0.5s
	setsockopt(Socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
	pRecvParm->bClose = false;
	

	while (true)
	{

		//char recv_data[1024]; 		      
		
		int bytes_recieved = recv(Socket, pRecvParm->recv_data, SYTCPSOCKET_BUFFER_LENGTH, 0);		
		pRecvParm->recv_data[bytes_recieved] = '\0';	
		if (bytes_recieved >0)
		{	
			//Event
			SYTCPEvent e;
			ZeroMemory(&e, sizeof(e));
			e.Status = SYTCPSOCKET_RECVDATA;
			e.Port = pRecvParm->addrClient.sin_port;
			e.iLen = bytes_recieved;
			memcpy(e.szData, pRecvParm->recv_data, sizeof(pRecvParm->recv_data) );				
			pThis->OnEvent(pThis, e);				
		}	
		//OutputDebugString(L"online\n");
		if (bytes_recieved == 0 || pRecvParm->bClose ==true)
		{		
			
			shutdown(Socket, SD_BOTH);		
			closesocket(Socket);
			//WSACleanup(); 不要加因為Server還在運行
			Socket = NULL;	

			//Event
			SYTCPEvent e;
			ZeroMemory(&e, sizeof(e));
			e.Status = SYTCPSOCKET_DISCONNECT;
			memset(e.szData, 0, sizeof(e.szData));				
			pThis->OnEvent(pThis, e);

			//OutputDebugString(L"Socket Is Close\n");
			USHORT key = pRecvParm->addrClient.sin_port;

			std::map<USHORT, SYTCPConnectParam*>::iterator iter = pThis->_mapClientList.find(key);		
			if (iter == pThis->_mapClientList.end())
			{
				///No find
			}else{

				//free
				free((SYTCPConnectParam*)iter->second);
				//remove
				pThis->_mapClientList.erase(iter);
			}

			break;
		}		
		
	}

	
	//delete pRecvParm; no need free((SYTCPConnectParam*)iter->second);
	return 0;
}

//////////////////////////////////////////////////////////////////////////
DWORD WINAPI ClientConnectProcThread(void *pParam)
{
	SYTCPSocket *pSYSocket = (SYTCPSocket*)pParam;

	wchar_t wszBuf[256];
	//----------------------
	// Initialize Winsock
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != NO_ERROR){
		//OutputDebugString(L"Error at WSAStartup()\n");
		SYTCPEvent e;
		ZeroMemory(&e, sizeof(e));
		e.Status = SYTCPSOCKET_CONNECTFAULT;
		wsprintf(e.wszMsg, L"%s", L"Error at WSAStartup()\n");
		pSYSocket->OnEvent(pSYSocket, e);
		return false;
	}			


	//----------------------
	// Create a SOCKET for connecting to server		
	pSYSocket->Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (pSYSocket->Socket == INVALID_SOCKET) 
	{			
		memset(wszBuf, 0, sizeof(wszBuf));
		wsprintf(wszBuf, L"Error at socket(): %ld\n", WSAGetLastError());
		//OutputDebugString(wszBuf);	
		WSACleanup();
		SYTCPEvent e;
		ZeroMemory(&e, sizeof(e));
		e.Status = SYTCPSOCKET_CONNECTFAULT;
		wsprintf(e.wszMsg, L"%s", wszBuf);
		pSYSocket->OnEvent(pSYSocket, e);
		return false;
	}

	//----------------------
	// The sockaddr_in structure specifies the address family,
	// IP address, and port of the server to be connected to.		
	sockaddr_in client_addr;
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = inet_addr( pSYSocket->RemoteHost );
	client_addr.sin_port = htons( pSYSocket->RemotePort );

	//----------------------
	// Connect to server.
	if ( connect( pSYSocket->Socket, (SOCKADDR*) &client_addr, sizeof(client_addr) ) == SOCKET_ERROR) {
		//OutputDebugString(L"Failed to connect.\n");
		WSACleanup();
		SYTCPEvent e;
		ZeroMemory(&e, sizeof(e));
		e.Status = SYTCPSOCKET_CONNECTFAULT;
		wsprintf(e.wszMsg, L"%s", L"Failed to connect.\n");
		pSYSocket->OnEvent(pSYSocket, e);
		return false;
	}

	//OutputDebugString(L"Client connected to server.\n");	

	//Recv
	pSYSocket->_hClientRecvProcThread = CreateThread(NULL, 0, ClientRecvProcThread, pSYSocket, 0, NULL);
	//Send
	pSYSocket->_hSendProcThread = CreateThread(NULL, 0, SendProcThread, pSYSocket, 0, NULL);

	SYTCPEvent e;
	ZeroMemory(&e, sizeof(e));
	e.Status = SYTCPSOCKET_CONNECTED;
	wsprintf(e.wszMsg, L"%s", L"Client connected to server.\n");
	pSYSocket->OnEvent(pSYSocket, e);

	return true;
	//WSACleanup();
}
//////////////////////////////////////////////////////////////////////////
DWORD WINAPI ClientRecvProcThread(void *pParam)
{

	SYTCPSocket *pThis = (SYTCPSocket *)pParam; 
	SOCKET Socket = pThis->Socket;

	
	CreateThread(NULL, 0, CheckProcThread, pThis, 0, NULL);	

	while (true)
	{
		char recv_data[SYTCPSOCKET_BUFFER_LENGTH];       
		int bytes_recieved = recv(Socket, recv_data, SYTCPSOCKET_BUFFER_LENGTH, 0);
		recv_data[bytes_recieved] = '\0';
		if (bytes_recieved >0)
		{	
			//Event
			SYTCPEvent e;
			ZeroMemory(&e, sizeof(e));
			e.Status = SYTCPSOCKET_RECVDATA;
			e.iLen = bytes_recieved;
			memcpy(e.szData, recv_data, sizeof(recv_data) );				
			pThis->OnEvent(pThis, e);				
		}	

		if (bytes_recieved == 0 ){
			break;
		}
		/*
		if (bytes_recieved == 0 )
		{
			pThis->Close();
			//OutputDebugString(L"Socket Is Close\n");
			//Event
			SYTCPEvent e;
			ZeroMemory(&e, sizeof(e));
			e.Status = SYTCPSOCKET_DISCONNECT;
			memset(e.szData, 0, sizeof(e.szData));				
			pThis->OnEvent(pThis, e);
			break;
		}*/	


	}
	//delete pRecvParm;

	return true;
}
//////////////////////////////////////////////////////////////////////////
DWORD WINAPI SendProcThread(void *pParam)
{

	SYTCPSocket *pThis = (SYTCPSocket *)pParam;

	while(true)
	{
		if (pThis->_pVecSendData->size() > 0)
		{

			if (pThis->_hListenProcThread)
			{
				char *pData = pThis->_pVecSendData->at(0);
				
				std::map<USHORT, SYTCPConnectParam*>::iterator iter;
				for ( iter=pThis->_mapClientList.begin() ; iter != pThis->_mapClientList.end(); iter++ )
				{
					SYTCPConnectParam *item = (SYTCPConnectParam*)iter->second;				

					int iResult = send(item->Socket, (const char *)pData, (int)strlen(pData), 0); 
					OutputDebugString(L"Server has send data\n");
				}				
			}

			if (pThis->_hConnectProcThread)
			{
				char *pData = pThis->_pVecSendData->at(0);
				int iResult = send(pThis->Socket, (const char *)pData, (int)strlen(pData), 0); 
				OutputDebugString(L"Client has send data\n");

			}

			std::vector<char *>::iterator iter = pThis->_pVecSendData->begin();
			free(*iter);
			pThis->_pVecSendData->erase(iter);
		}//end if (pThis->_pVecSendData->size())

	}//end while

	return true;
}

//////////////////////////////////////////////////////////////////////////
DWORD WINAPI CheckProcThread(void *pParam)
{
	SYTCPSocket *pThis = (SYTCPSocket *)pParam; 
	SOCKET Socket = pThis->Socket;

	int optVal = 1;
	int optLen = sizeof(optVal);

	while(true)
	{
		int res = getsockopt(Socket, SOL_SOCKET, SO_KEEPALIVE, (char*)&optVal, &optLen);

		if(res==SOCKET_ERROR) 
		{
			OutputDebugString(L"client offline\n");		
			break;
		}else{
			//OutputDebugString(L"online\n");
		}
		Sleep(100);
	}


	pThis->Close();
	//OutputDebugString(L"Socket Is Close\n");
	//Event
	SYTCPEvent e;
	ZeroMemory(&e, sizeof(e));
	e.Status = SYTCPSOCKET_DISCONNECT;
	memset(e.szData, 0, sizeof(e.szData));				
	pThis->OnEvent(pThis, e);

	return 0;
}
