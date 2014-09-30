#include "UDPClient.hpp"
#include "../Engine/NewMacroDef.hpp"


//-----------------------------------------------------------------------------------------------
bool UDPClient::ConnectToServer( const std::string& serverIPAddress, unsigned short serverPortNumber )
{
	if( WSAStartup( 0x202, &m_wsaData ) != 0 )
	{
		return false;
	}

	m_socket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
	if( m_socket == INVALID_SOCKET )
	{
		return false;
	}

	m_serverAddr.sin_family = AF_INET;
	m_serverAddr.sin_addr.s_addr = inet_addr( serverIPAddress.c_str() );
	m_serverAddr.sin_port = htons( serverPortNumber );

	u_long mode = 1;
	if( ioctlsocket( m_socket, FIONBIO, &mode ) == SOCKET_ERROR )
	{
		return false;
	}

	m_serverIPAddress = serverIPAddress;
	m_serverPortNumber = serverPortNumber;

	return true;
}


//-----------------------------------------------------------------------------------------------
void UDPClient::DisconnectFromServer()
{
	closesocket( m_socket );
	WSACleanup();
}


//-----------------------------------------------------------------------------------------------
bool UDPClient::ReceivePacketFromServer( char* out_packetInfo, int packetLength )
{
	struct sockaddr_in clientAddr;
	int clientLen = sizeof( clientAddr );

	if( recvfrom( m_socket, out_packetInfo, packetLength, 0, (struct sockaddr*) &clientAddr, &clientLen ) < 0 )
	{
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------------------------
bool UDPClient::SendPacketToServer( const char* packetInfo, int packetLength )
{
	if( sendto( m_socket, packetInfo, packetLength, 0, (struct sockaddr*) &m_serverAddr, sizeof( m_serverAddr ) ) < 0 )
	{
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------------------------
std::string UDPClient::GetServerIPAddress()
{
	return m_serverIPAddress;
}


//-----------------------------------------------------------------------------------------------
unsigned short UDPClient::GetServerPortNumber()
{
	return m_serverPortNumber;
}


//-----------------------------------------------------------------------------------------------
void UDPClient::SetServerIPAddress( const std::string ipAddress )
{
	m_serverAddr.sin_addr.s_addr = inet_addr( ipAddress.c_str() );
}


//-----------------------------------------------------------------------------------------------
void UDPClient::SetServerPortNumber( unsigned short portNumber )
{
	m_serverAddr.sin_port = htons( portNumber );
}