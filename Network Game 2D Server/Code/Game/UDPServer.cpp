#include "UDPServer.hpp"


//-----------------------------------------------------------------------------------------------
bool UDPServer::StartServer( unsigned short desiredPortNumber )
{
	if( WSAStartup( 0x202, &m_wsaData ) != 0 )
	{
		return false;
	}

	m_socket = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP );
	if( m_socket == INVALID_SOCKET )
	{
		return false;
	}

	m_serverAddr.sin_family = AF_INET;
	m_serverAddr.sin_addr.s_addr = htonl( INADDR_ANY );
	m_serverAddr.sin_port = htons( desiredPortNumber );

	u_long mode = 1;
	if( ioctlsocket( m_socket, FIONBIO, &mode ) == SOCKET_ERROR )
	{
		return false;
	}

	if( bind( m_socket, (struct sockaddr *) &m_serverAddr, sizeof( m_serverAddr ) ) < 0 )
	{
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------------------------
void UDPServer::EndServer()
{
	closesocket( m_socket );
	WSACleanup();
}


//-----------------------------------------------------------------------------------------------
bool UDPServer::SendPacketToClient( const char* packetInfo, int packetLength, const struct sockaddr_in& clientAddr )
{
	if( sendto( m_socket, packetInfo, packetLength, 0, (struct sockaddr*) &clientAddr, sizeof( clientAddr ) ) < 0 )
	{
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------------------------
bool UDPServer::ReceivePacketFromClient( char* out_packetInfo, int packetLength, struct sockaddr_in& out_clientAddr, int& out_clientLen )
{
	if( recvfrom( m_socket, out_packetInfo, packetLength, 0, (struct sockaddr*) &out_clientAddr, &out_clientLen ) < 0 )
	{
		return false;
	}

	return true;
}