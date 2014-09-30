#ifndef include_UDPClient
#define include_UDPClient
#pragma once

//-----------------------------------------------------------------------------------------------
#include <string>
#include <WinSock2.h>
#pragma comment(lib,"ws2_32.lib")


//-----------------------------------------------------------------------------------------------
class UDPClient
{
public:
	UDPClient() {}
	bool ConnectToServer( const std::string& serverIPAddress, unsigned short serverPortNumber );
	void DisconnectFromServer();
	bool ReceivePacketFromServer( char* out_packetInfo, int packetLength );
	bool SendPacketToServer( const char* packetInfo, int packetLength );
	std::string GetServerIPAddress();
	unsigned short GetServerPortNumber();
	void SetServerIPAddress( const std::string ipAddress );
	void SetServerPortNumber( unsigned short portNumber );

private:
	WSADATA					m_wsaData;
	SOCKET					m_socket;
	struct sockaddr_in		m_serverAddr;
	std::string				m_serverIPAddress;
	unsigned short			m_serverPortNumber;
};


#endif // include_UDPClient