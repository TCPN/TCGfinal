#include "ClientSocket.h"

ClientSocket::ClientSocket( char* ip,  int port)
{
	while(!InitSocket(ip,port))
	{ 
		printf("protocol port init failed!\n");
	}
}

ClientSocket::ClientSocket()
{
}

ClientSocket::~ClientSocket(void)
{
	CloseSocket();
}

void ClientSocket::CloseSocket()
{
#ifdef _WIN32
     closesocket(this->m_Socket);
     WSACleanup();
#else
     close(m_Socket);
#endif
}

bool ClientSocket::InitSocket(const char*ip, const int port)
{
     
#ifdef _WIN32
    SOCKET socket;
    SOCKADDR_IN servAddr;
	WSADATA wsaData ;
    int retVal;
	if ( ::WSAStartup( MAKEWORD( 2, 0 ), &wsaData ) != 0 ) // minorVer:函式庫檔案的副版本(高位元組), majorVer:主版本(低位元組)
	{
		printf( "Socket WSAStartup() failed. \n" ) ;
		ShowErrorMsg() ;
		return false ;
	}
	// 建立Socket
	socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if( socket == INVALID_SOCKET )
	{
		printf("Socket Initial failed.");
		WSACleanup();
		ShowErrorMsg() ;
		return false ;
	}
	// set sockaddr_in
	servAddr.sin_family = AF_INET ;
	servAddr.sin_addr.s_addr = inet_addr(ip) ;
	servAddr.sin_port = htons(port) ;

	// connect to Server
	retVal = ::connect( socket, (LPSOCKADDR)&servAddr, sizeof(servAddr) ) ;
	if ( SOCKET_ERROR == retVal )
	{
		puts("//Connect Error.  please call the  Administrator.\n//");
		return  false ;
	}
	this->m_Socket = socket;
#else     
    struct sockaddr_in servAddr;
    m_Socket = socket(AF_INET , SOCK_STREAM , 0);
    if (m_Socket == -1)
    {
        printf("Could not create socket");
        return false;
    }
    puts("Socket created");

    servAddr.sin_addr.s_addr = inet_addr(ip);
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons( port );

    //Connect to remote server
    if (connect(m_Socket , (struct sockaddr *)&servAddr , sizeof(servAddr)) < 0)
    {
        perror("connect failed. Error");
        return false;
    }
    puts("Connected\n");
#endif
	return true;

}

void ClientSocket::ShowErrorMsg(void)
{
#ifdef _WIN32
	int nErrCode = WSAGetLastError();//get error code

	HLOCAL hlocal = NULL;

	// error string
	BOOL fOk = FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL, nErrCode, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		(PTSTR)&hlocal, 0, NULL);

	// show error message
	if ( hlocal != NULL )
	{
		//MessageBox(NULL, (WCHAR*)LocalLock(hlocal), L"CLIENT ERROR", MB_OK);
		LocalFree(hlocal);
	}
#endif
} // end ShowErrorMsg()

bool ClientSocket::Recieve(char **recvbuf)
{
	int bytesRecv=0;
	int lastLen=0;
	char len[3]=""; 
	bytesRecv = recv( this->m_Socket, len, 2, 0 ); // 收長度
	len[2]='\0';
	if(bytesRecv<2)
	{
		if(bytesRecv == 0 )
			puts("*Disconnect to server!!\n");
		puts("****recv error(bytesRecv)****");
		return false;
	}
	else
	{
		const int totalLen = (len[1])*128 + (len[0]); 
		lastLen = totalLen;
		char* packet = (char*)malloc(sizeof(char)*(totalLen+1));
		char tmpBuffer[1025];
		int index = 0;
		while( lastLen >= 1024 )
		{   
			bytesRecv = recv( this->m_Socket, tmpBuffer, 1024, 0 ); // 不確定 recv 多少
			if(bytesRecv < 0 ){
				puts("****recv error(lastBuffer)****");
				return false;
			}
			for(int i=0; i<bytesRecv;i++) packet[index++] = tmpBuffer[i];
			lastLen -= bytesRecv;
		}
		while( lastLen > 0 )
		{  
			bytesRecv = recv( this->m_Socket, tmpBuffer, lastLen, 0 ); // 不確定會 recv 多少
			if(bytesRecv < 0 ){
				return false;
			}
			for(int i=0; i<bytesRecv;i++) packet[index++] = tmpBuffer[i];
			lastLen -= bytesRecv;
		}
		packet[totalLen]='\0';
		*recvbuf = packet;
	} 
	return true;
} // end Recieve(char **recvbuf)

bool ClientSocket::Send(const char* sendbuf)
{
	int packetlength = strlen(sendbuf) ;
	char *sendstr = (char*)malloc(sizeof(char)*(packetlength+3));   
	sendstr[0] = packetlength % 128 ;
	sendstr[1] = packetlength / 128 ;
	for ( int i = 2 ; i < packetlength + 2 ; i ++ ) 
		sendstr[i] = sendbuf[i - 2] ; 
	sendstr[packetlength + 2] = '\0'; 
    int nBufSize = strlen(sendbuf);
	int bytesSend = send(this->m_Socket,sendstr,packetlength+2,0);
    if( bytesSend < nBufSize )
    {
		puts("*****send error*****");
		return false;
    }
	if(sendstr!=NULL) free(sendstr); sendstr = NULL;
	return true;
} // end Send(const char* sendbuf)


