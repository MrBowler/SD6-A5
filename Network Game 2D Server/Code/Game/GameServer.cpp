#include "GameServer.hpp"


//-----------------------------------------------------------------------------------------------
void GameServer::Initalize()
{
	srand( (unsigned int) time( NULL ) );

	InitializeTime();
	m_server.StartServer( m_portNumber );

	m_nextPacketNumber = 0;
	m_numFlagsCaptured = 0;
	m_isGameOver = false;
	m_addedPlayersToLobby = false;
	m_flagPosition = GetRandomPosition();
	m_lastUpdateTime = GetCurrentTimeSeconds();

	std::cout << "Game is up and running\n";
}


//-----------------------------------------------------------------------------------------------
void GameServer::Update()
{
	GetPackets();
	CheckForTimeOutPlayers();
	SendUpdatesToClients();
	ResendAckPackets();
}


//-----------------------------------------------------------------------------------------------
void GameServer::AddPlayer( const ClientInfo& info )
{
	Player* player;

	std::map< ClientInfo, Player* >::iterator playerIter = m_players.find( info );
	if( playerIter == m_players.end() )
	{
		player = new Player();
	}
	else
	{
		player = playerIter->second;
	}

	player->m_color = GetPlayerColorForID( m_players.size() );
	player->m_position = GetRandomPosition();
	player->m_velocity = Vector2( 0.f, 0.f );
	player->m_orientationDegrees = 0.f;
	player->m_lastUpdateTime = GetCurrentTimeSeconds();

	m_players[ info ] = player;

	CS6Packet resetPacket;
	resetPacket.packetNumber = m_nextPacketNumber;
	resetPacket.packetType = TYPE_Reset;
	resetPacket.playerColorAndID[0] = player->m_color.r;
	resetPacket.playerColorAndID[1] = player->m_color.g;
	resetPacket.playerColorAndID[2] = player->m_color.b;
	resetPacket.timestamp = GetCurrentTimeSeconds();
	resetPacket.data.reset.playerXPosition = player->m_position.x;
	resetPacket.data.reset.playerYPosition = player->m_position.y;
	resetPacket.data.reset.flagXPosition = m_flagPosition.x;
	resetPacket.data.reset.flagYPosition = m_flagPosition.y;
	resetPacket.data.reset.playerColorAndID[0] = player->m_color.r;
	resetPacket.data.reset.playerColorAndID[1] = player->m_color.g;
	resetPacket.data.reset.playerColorAndID[2] = player->m_color.b;

	SendPacketToClient( resetPacket, info, true );
}


//-----------------------------------------------------------------------------------------------
unsigned int GameServer::GetNumberOfPlayers()
{
	return m_players.size();
}


//-----------------------------------------------------------------------------------------------
void GameServer::SendPacketToClient( const CS6Packet& pkt, const ClientInfo& info, bool requireAck )
{
	struct sockaddr_in clientAddr;
	clientAddr.sin_family = AF_INET;
	clientAddr.sin_addr.s_addr = inet_addr( info.m_ipAddress );
	clientAddr.sin_port = info.m_portNumber;
	m_server.SendPacketToClient( (const char*) &pkt, sizeof( pkt ), clientAddr );
	++m_nextPacketNumber;

	if( requireAck )
	{
		std::vector< CS6Packet > sentPackets;
		std::map< ClientInfo, std::vector< CS6Packet > >::iterator vecIter = m_sendPacketsPerClient.find( info );
		if( vecIter != m_sendPacketsPerClient.end() )
		{
			sentPackets = vecIter->second;
		}

		sentPackets.push_back( pkt );
		m_sendPacketsPerClient[ info ] = sentPackets;
	}
}


//-----------------------------------------------------------------------------------------------
void GameServer::SendPacketToAllClients( const CS6Packet& pkt, bool requireAck )
{
	std::map< ClientInfo, Player* >::iterator playerIter;
	for( playerIter = m_players.begin(); playerIter != m_players.end(); ++playerIter )
	{
		SendPacketToClient( pkt, playerIter->first, requireAck );
	}
}


//-----------------------------------------------------------------------------------------------
std::string GameServer::ConvertNumberToString( int number )
{
	return static_cast< std::ostringstream* >( &( std::ostringstream() << number ) )->str();
}


//-----------------------------------------------------------------------------------------------
Color3b GameServer::GetPlayerColorForID( unsigned int playerID )
{
	if( playerID == 0 )
		return Color3b( 255, 0, 0 );
	if( playerID == 1 )
		return Color3b( 0, 255, 0 );
	if( playerID == 2 )
		return Color3b( 0, 0, 255 );
	if( playerID == 3 )
		return Color3b( 255, 255, 0 );
	if( playerID == 4 )
		return Color3b( 255, 0, 255 );
	if( playerID == 5 )
		return Color3b( 0, 255, 255 );
	if( playerID == 6 )
		return Color3b( 255, 165, 0 );
	if( playerID == 7 )
		return Color3b( 128, 0, 128 );

	return Color3b( 255, 255, 255 );
}


//-----------------------------------------------------------------------------------------------
Vector2 GameServer::GetRandomPosition()
{
	Vector2 returnVec;
	returnVec.x = (float) ( rand() % MAP_SIZE_WIDTH );
	returnVec.y = (float) ( rand() % MAP_SIZE_HEIGHT );

	return returnVec;
}


//-----------------------------------------------------------------------------------------------
void GameServer::ProcessAckPackets( const CS6Packet& ackPacket, const ClientInfo& info )
{
	if( ackPacket.data.acknowledged.packetType == TYPE_Acknowledge )
	{
		AddPlayer( info );
	}
	else if( ackPacket.data.acknowledged.packetType == TYPE_GameOver )
	{
		RemovePlayer( info );
	}

	std::map< ClientInfo, std::vector< CS6Packet > >::iterator vecIter = m_sendPacketsPerClient.find( info );
	if( vecIter != m_sendPacketsPerClient.end() )
	{
		std::vector< CS6Packet > sentPackets = vecIter->second;
		for( unsigned int packetIndex = 0; packetIndex < sentPackets.size(); ++packetIndex )
		{
			CS6Packet packet = sentPackets[ packetIndex ];
			if( packet.packetNumber == ackPacket.data.acknowledged.packetNumber )
			{
				sentPackets.erase( sentPackets.begin() + packetIndex );
				m_sendPacketsPerClient[ info ] = sentPackets;
				break;
			}
		}
	}
}


//-----------------------------------------------------------------------------------------------
void GameServer::RemovePlayer( const ClientInfo& info )
{
	std::string playerString = std::string( info.m_ipAddress ) + ":" + ConvertNumberToString( info.m_portNumber );
	if( playerString == m_ownerName )
		m_isGameOver = true;

	std::map< ClientInfo, Player* >::iterator playerIter = m_players.find( info );
	if( playerIter != m_players.end() )
	{
		Player* player = playerIter->second;
		m_players.erase( playerIter );
		delete player;
	}

	std::map< ClientInfo, std::vector< CS6Packet > >::iterator listIter = m_sendPacketsPerClient.find( info );
	if( listIter != m_sendPacketsPerClient.end() )
	{
		m_sendPacketsPerClient.erase( listIter );
	}
}


//-----------------------------------------------------------------------------------------------
void GameServer::CheckForTimeOutPlayers()
{
	std::map< ClientInfo, Player* >::iterator playerIter;
	for( playerIter = m_players.begin(); playerIter != m_players.end(); ++playerIter )
	{
		Player* player = playerIter->second;
		if( ( GetCurrentTimeSeconds() - player->m_lastUpdateTime ) > SECONDS_BEFORE_TIMEOUT_REMOVE )
		{
			RemovePlayer( playerIter->first );
			SendGameOverToClients();
			return;
		}
	}
}


//-----------------------------------------------------------------------------------------------
void GameServer::ResetGame( const CS6Packet& victoryPacket, const ClientInfo& info )
{
	CS6Packet ackPacket;
	ackPacket.packetNumber = m_nextPacketNumber;
	ackPacket.packetType = TYPE_Acknowledge;
	ackPacket.timestamp = GetCurrentTimeSeconds();
	ackPacket.data.acknowledged.packetNumber = victoryPacket.packetNumber;
	ackPacket.data.acknowledged.packetType = TYPE_Victory;

	SendPacketToClient( ackPacket, info, false );

	m_flagPosition = GetRandomPosition();
	++m_numFlagsCaptured;
	if( m_numFlagsCaptured >= 3 )
	{
		m_isGameOver = true;
		SendGameOverToClients();
		return;
	}

	std::map< ClientInfo, Player* >::iterator playerIter;
	for( playerIter = m_players.begin(); playerIter != m_players.end(); ++playerIter )
	{
		Player* player = playerIter->second;
		Vector2 resetPlayerPos = GetRandomPosition();

		CS6Packet resetPacket;
		resetPacket.packetNumber = m_nextPacketNumber;
		resetPacket.packetType = TYPE_Reset;
		resetPacket.playerColorAndID[0] = player->m_color.r;
		resetPacket.playerColorAndID[1] = player->m_color.g;
		resetPacket.playerColorAndID[2] = player->m_color.b;
		resetPacket.timestamp = GetCurrentTimeSeconds();
		resetPacket.data.reset.playerColorAndID[0] = player->m_color.r;
		resetPacket.data.reset.playerColorAndID[1] = player->m_color.g;
		resetPacket.data.reset.playerColorAndID[2] = player->m_color.b;
		resetPacket.data.reset.playerXPosition = resetPlayerPos.x;
		resetPacket.data.reset.playerYPosition = resetPlayerPos.y;
		resetPacket.data.reset.flagXPosition = m_flagPosition.x;
		resetPacket.data.reset.flagYPosition = m_flagPosition.y;
		resetPacket.data.reset.playerColorAndID[0] = player->m_color.r;
		resetPacket.data.reset.playerColorAndID[1] = player->m_color.g;
		resetPacket.data.reset.playerColorAndID[2] = player->m_color.b;

		SendPacketToClient( resetPacket, playerIter->first, true );
	}
}


//-----------------------------------------------------------------------------------------------
void GameServer::UpdatePlayer( const CS6Packet& updatePacket, const ClientInfo& info )
{
	std::map< ClientInfo, Player* >::iterator playerIter = m_players.find( info );
	if( playerIter != m_players.end() )
	{
		Player* player = playerIter->second;
		player->m_position.x = updatePacket.data.updated.xPosition;
		player->m_position.y = updatePacket.data.updated.yPosition;
		player->m_velocity.x = updatePacket.data.updated.xVelocity;
		player->m_velocity.y = updatePacket.data.updated.yVelocity;
		player->m_orientationDegrees = updatePacket.data.updated.yawDegrees;
		player->m_lastUpdateTime = GetCurrentTimeSeconds();
	}
}


//-----------------------------------------------------------------------------------------------
void GameServer::SendUpdatesToClients()
{
	if( m_isGameOver )
		return;

	if( ( GetCurrentTimeSeconds() - m_lastUpdateTime ) < SECONDS_BEFORE_SEND_UPDATE )
		return;

	std::map< ClientInfo, Player* >::iterator playerIter;
	for( playerIter = m_players.begin(); playerIter != m_players.end(); ++playerIter )
	{
		Player* player = playerIter->second;
		CS6Packet updatePacket;
		updatePacket.packetNumber = m_nextPacketNumber;
		updatePacket.packetType = TYPE_Update;
		updatePacket.playerColorAndID[0] = player->m_color.r;
		updatePacket.playerColorAndID[1] = player->m_color.g;
		updatePacket.playerColorAndID[2] = player->m_color.b;
		updatePacket.timestamp = GetCurrentTimeSeconds();
		updatePacket.data.updated.xPosition = player->m_position.x;
		updatePacket.data.updated.yPosition = player->m_position.y;
		updatePacket.data.updated.xVelocity = player->m_velocity.x;
		updatePacket.data.updated.yVelocity = player->m_velocity.y;
		updatePacket.data.updated.yawDegrees = player->m_orientationDegrees;

		SendPacketToAllClients( updatePacket, false );
	}

	m_lastUpdateTime = GetCurrentTimeSeconds();
}


//-----------------------------------------------------------------------------------------------
void GameServer::SendGameOverToClients()
{
	CS6Packet gameOverPacket;
	gameOverPacket.packetNumber = m_nextPacketNumber;
	gameOverPacket.packetType = TYPE_GameOver;
	gameOverPacket.timestamp = GetCurrentTimeSeconds();

	SendPacketToAllClients( gameOverPacket, true );
}


//-----------------------------------------------------------------------------------------------
void GameServer::GetPackets()
{
	CS6Packet pkt;
	struct sockaddr_in clientAddr;
	clientAddr.sin_family = AF_INET;
	int clientLen = sizeof( clientAddr );

	std::set< CS6Packet > recvPackets;
	std::map< CS6Packet, ClientInfo > infoByPacket;
	while( m_server.ReceivePacketFromClient( (char*) &pkt, sizeof( pkt ), clientAddr, clientLen ) )
	{
		ClientInfo info;
		info.m_ipAddress = inet_ntoa( clientAddr.sin_addr );
		info.m_portNumber = clientAddr.sin_port;

		recvPackets.insert( pkt );
		infoByPacket[ pkt ] = info;
	}

	std::set< CS6Packet >::iterator setIter;
	for( setIter = recvPackets.begin(); setIter != recvPackets.end(); ++setIter )
	{
		CS6Packet orderedPacket = *setIter;
		ClientInfo info = infoByPacket[ orderedPacket ];

		if( orderedPacket.packetType == TYPE_Acknowledge )
		{
			ProcessAckPackets( orderedPacket, info );
		}
		else if( orderedPacket.packetType == TYPE_Update )
		{
			UpdatePlayer( orderedPacket, info );
		}
		else if( orderedPacket.packetType == TYPE_Victory )
		{
			ResetGame( orderedPacket, info );
		}
	}
}


//-----------------------------------------------------------------------------------------------
void GameServer::ResendAckPackets()
{
	std::map< ClientInfo, std::vector< CS6Packet > >::iterator vecIter;
	for( vecIter = m_sendPacketsPerClient.begin(); vecIter != m_sendPacketsPerClient.end(); ++vecIter )
	{
		std::vector< CS6Packet > sentPackets = vecIter->second;
		for( unsigned int packetIndex = 0; packetIndex < sentPackets.size(); ++packetIndex )
		{
			CS6Packet* packet = &sentPackets[ packetIndex ];
			if( ( GetCurrentTimeSeconds() - packet->timestamp ) > SECONDS_BEFORE_RESEND_RELIABLE_PACKETS )
			{
				packet->packetNumber = m_nextPacketNumber;
				packet->timestamp = GetCurrentTimeSeconds();
				SendPacketToClient( *packet, vecIter->first, false );
			}
		}

		m_sendPacketsPerClient[ vecIter->first ] = sentPackets;
	}
}