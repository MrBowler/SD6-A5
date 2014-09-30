#ifndef include_UDPServer
#define include_UDPServer
#pragma once

//-----------------------------------------------------------------------------------------------
#include <WinSock2.h>
#pragma comment(lib,"ws2_32.lib")


//-----------------------------------------------------------------------------------------------
class UDPServer
{
public:
	UDPServer() {}
	bool StartServer( unsigned short desiredPortNumber );
	void EndServer();
	bool SendPacketToClient( const char* packetInfo, int packetLength, const struct sockaddr_in& clientAddr );
	bool ReceivePacketFromClient( char* out_packetInfo, int packetLength, struct sockaddr_in& out_clientAddr, int& out_clientLen );

private:
	WSADATA				m_wsaData;
	SOCKET				m_socket;
	struct sockaddr_in	m_serverAddr;
};


#endif // include_UDPServer