#include "Lobby.hpp"


//-----------------------------------------------------------------------------------------------
void Lobby::Initalize()
{
	InitializeTime();
	m_server.StartServer( PORT_NUMBER );
	m_nextPacketNumber = 0;
	m_nextGameID = 0;
	m_nextPortNumber = PORT_NUMBER + 1;
	m_lastUpdateTime = GetCurrentTimeSeconds();

	std::cout << "Server is up and running\n";
}


//-----------------------------------------------------------------------------------------------
void Lobby::Update()
{
	UpdateGames();
	GetPackets();
	SendLobbyUpdates();
}


//-----------------------------------------------------------------------------------------------
void Lobby::SendPacketToClient( const LobbyPacket& pkt, const ClientInfo& info, bool requireAck )
{
	struct sockaddr_in clientAddr;
	clientAddr.sin_family = AF_INET;
	clientAddr.sin_addr.s_addr = inet_addr( info.m_ipAddress );
	clientAddr.sin_port = info.m_portNumber;
	m_server.SendPacketToClient( (const char*) &pkt, sizeof( pkt ), clientAddr );
	++m_nextPacketNumber;

	if( requireAck )
	{
		std::vector< LobbyPacket > sentPackets;
		std::map< ClientInfo, std::vector< LobbyPacket > >::iterator vecIter = m_sendPacketsPerClient.find( info );
		if( vecIter != m_sendPacketsPerClient.end() )
		{
			sentPackets = vecIter->second;
		}

		sentPackets.push_back( pkt );
		m_sendPacketsPerClient[ info ] = sentPackets;
	}
}


//-----------------------------------------------------------------------------------------------
void Lobby::SendPacketToAllClients( const LobbyPacket& pkt, bool requireAck )
{
	std::set< ClientInfo >::iterator playerIter;
	for( playerIter = m_lobbyPlayers.begin(); playerIter != m_lobbyPlayers.end(); ++playerIter )
	{
		SendPacketToClient( pkt, *playerIter, requireAck );
	}
}


//-----------------------------------------------------------------------------------------------
std::string Lobby::ConvertNumberToString( int number )
{
	return static_cast< std::ostringstream* >( &( std::ostringstream() << number ) )->str();
}


//-----------------------------------------------------------------------------------------------
void Lobby::UpdateGames()
{
	std::map< int, GameServer* >::iterator gameIter;
	std::vector< std::map< int, GameServer* >::iterator > gamesToRemove;

	for( gameIter = m_games.begin(); gameIter != m_games.end(); ++gameIter )
	{
		GameServer* game = gameIter->second;
		game->Update();
		if( game->m_isGameOver )
		{
			if( !game->m_addedPlayersToLobby )
			{
				AddPlayersToLobby( game );
				game->m_addedPlayersToLobby = true;
			}

			if( game->m_players.size() == 0 )
			{
				gamesToRemove.push_back( gameIter );
			}
		}
	}

	for( unsigned int removeIndex = 0; removeIndex < gamesToRemove.size(); ++removeIndex )
	{
		m_games.erase( gamesToRemove[ removeIndex ] );
	}
}


//-----------------------------------------------------------------------------------------------
void Lobby::GetPackets()
{
	LobbyPacket pkt;
	struct sockaddr_in clientAddr;
	clientAddr.sin_family = AF_INET;
	int clientLen = sizeof( clientAddr );

	std::set< LobbyPacket > recvPackets;
	std::map< LobbyPacket, ClientInfo > infoByPacket;
	while( m_server.ReceivePacketFromClient( (char*) &pkt, sizeof( pkt ), clientAddr, clientLen ) )
	{
		ClientInfo info;
		info.m_ipAddress = inet_ntoa( clientAddr.sin_addr );
		info.m_portNumber = clientAddr.sin_port;

		recvPackets.insert( pkt );
		infoByPacket[ pkt ] = info;
	}

	std::set< LobbyPacket >::iterator setIter;
	for( setIter = recvPackets.begin(); setIter != recvPackets.end(); ++setIter )
	{
		LobbyPacket orderedPacket = *setIter;
		ClientInfo info = infoByPacket[ orderedPacket ];

		if( orderedPacket.packetType == LOBBY_TYPE_Acknowledge )
		{
			ProcessAckPackets( orderedPacket, info );
		}
		else if( orderedPacket.packetType == LOBBY_TYPE_CreateGame )
		{
			CreateGame( orderedPacket, info );
		}
		else if( orderedPacket.packetType == LOBBY_TYPE_JoinGame )
		{
			AddPlayerToGame( orderedPacket, info );
		}
	}
}


//-----------------------------------------------------------------------------------------------
void Lobby::SendLobbyUpdates()
{
	if( ( GetCurrentTimeSeconds() - m_lastUpdateTime ) < SECONDS_BEFORE_SEND_UPDATE )
		return;

	std::map< int, GameServer* >::iterator gameIter;
	for( gameIter = m_games.begin(); gameIter != m_games.end(); ++gameIter )
	{
		GameServer* game = gameIter->second;

		LobbyPacket updatePacket;
		updatePacket.packetType = LOBBY_TYPE_Update;
		updatePacket.timestamp = GetCurrentTimeSeconds();
		updatePacket.data.update.gameID = gameIter->first;
		updatePacket.data.update.numPlayersInGame = (unsigned char) game->GetNumberOfPlayers();
		strcpy_s( updatePacket.data.update.gameOwner, game->m_ownerName.c_str() );

		std::set< ClientInfo >::iterator playerIter;
		for( playerIter = m_lobbyPlayers.begin(); playerIter != m_lobbyPlayers.end(); ++playerIter )
		{
			updatePacket.packetNumber = m_nextPacketNumber;
			SendPacketToClient( updatePacket, *playerIter, false );
		}
	}

	m_lastUpdateTime = GetCurrentTimeSeconds();
}


//-----------------------------------------------------------------------------------------------
void Lobby::RemovePlayerFromLobby( const ClientInfo& info )
{
	std::set< ClientInfo >::iterator playerIter;
	for( playerIter = m_lobbyPlayers.begin(); playerIter != m_lobbyPlayers.end(); ++playerIter )
	{
		ClientInfo playerInfo = *playerIter;
		if( playerInfo.m_ipAddress == info.m_ipAddress && playerInfo.m_portNumber == info.m_portNumber )
		{
			m_lobbyPlayers.erase( playerIter );
			return;
		}
	}
}


//-----------------------------------------------------------------------------------------------
void Lobby::AcknowledgeConnection( const LobbyPacket& packet, const ClientInfo& info )
{
	LobbyPacket ackPacket;
	ackPacket.packetNumber = m_nextPacketNumber;
	ackPacket.packetType = LOBBY_TYPE_Acknowledge;
	ackPacket.timestamp = GetCurrentTimeSeconds();
	ackPacket.data.acknowledged.packetNumber = packet.packetNumber;
	ackPacket.data.acknowledged.packetType = LOBBY_TYPE_Acknowledge;

	SendPacketToClient( ackPacket, info, false );
}


//-----------------------------------------------------------------------------------------------
void Lobby::ProcessAckPackets( const LobbyPacket& ackPacket, const ClientInfo& info )
{
	if( ackPacket.data.acknowledged.packetType == LOBBY_TYPE_Acknowledge )
	{
		m_lobbyPlayers.insert( info );
		AcknowledgeConnection( ackPacket, info );
	}

	std::map< ClientInfo, std::vector< LobbyPacket > >::iterator vecIter = m_sendPacketsPerClient.find( info );
	if( vecIter != m_sendPacketsPerClient.end() )
	{
		std::vector< LobbyPacket > sentPackets = vecIter->second;
		for( unsigned int packetIndex = 0; packetIndex < sentPackets.size(); ++packetIndex )
		{
			LobbyPacket packet = sentPackets[ packetIndex ];
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
void Lobby::CreateGame( const LobbyPacket& createPacket, const ClientInfo& gameOwner )
{
	GameServer* game = new GameServer();
	game->m_gameID = m_nextGameID;
	game->m_portNumber = m_nextPortNumber;
	game->m_ownerName = std::string( gameOwner.m_ipAddress ) + ":" + ConvertNumberToString( gameOwner.m_portNumber );
	game->Initalize();
	game->AddPlayer( gameOwner );
	m_games[ m_nextGameID ] = game;

	LobbyPacket ackPacket;
	ackPacket.packetNumber = m_nextPacketNumber;
	ackPacket.packetType = LOBBY_TYPE_Acknowledge;
	ackPacket.timestamp = GetCurrentTimeSeconds();
	ackPacket.data.acknowledged.packetNumber = createPacket.packetNumber;
	ackPacket.data.acknowledged.portNumber = m_nextPortNumber;
	ackPacket.data.acknowledged.packetType = LOBBY_TYPE_CreateGame;

	SendPacketToClient( ackPacket, gameOwner, false );
	RemovePlayerFromLobby( gameOwner );

	++m_nextGameID;
	++m_nextPortNumber;
}


//-----------------------------------------------------------------------------------------------
void Lobby::AddPlayersToLobby( const GameServer* game )
{
	std::map< ClientInfo, Player* > players = game->m_players;
	std::map< ClientInfo, Player* >::iterator playerIter;
	for( playerIter = players.begin(); playerIter != players.end(); ++playerIter )
	{
		m_lobbyPlayers.insert( playerIter->first );
	}
}


//-----------------------------------------------------------------------------------------------
void Lobby::AddPlayerToGame( const LobbyPacket& joinPacket, const ClientInfo& info )
{
	std::map< int, GameServer* >::iterator gameIter;
	for( gameIter = m_games.begin(); gameIter != m_games.end(); ++gameIter )
	{
		GameServer* game = gameIter->second;
		if( game->m_gameID == joinPacket.data.join.gameID )
		{
			game->AddPlayer( info );

			LobbyPacket ackPacket;
			ackPacket.packetNumber = m_nextPacketNumber;
			ackPacket.packetType = LOBBY_TYPE_Acknowledge;
			ackPacket.timestamp = GetCurrentTimeSeconds();
			ackPacket.data.acknowledged.packetNumber = joinPacket.packetNumber;
			ackPacket.data.acknowledged.portNumber = game->m_portNumber;
			ackPacket.data.acknowledged.packetType = LOBBY_TYPE_JoinGame;

			SendPacketToClient( ackPacket, info, false );
			RemovePlayerFromLobby( info );

			return;
		}
	}

	LobbyPacket ackPacket;
	ackPacket.packetNumber = m_nextPacketNumber;
	ackPacket.packetType = LOBBY_TYPE_Acknowledge;
	ackPacket.timestamp = GetCurrentTimeSeconds();
	ackPacket.data.acknowledged.packetNumber = joinPacket.packetNumber;
	ackPacket.data.acknowledged.portNumber = PORT_NUMBER;
	ackPacket.data.acknowledged.packetType = LOBBY_TYPE_JoinGame;

	SendPacketToClient( ackPacket, info, false );
}


//-----------------------------------------------------------------------------------------------
void Lobby::ResendAckPackets()
{
	std::map< ClientInfo, std::vector< LobbyPacket > >::iterator vecIter;
	for( vecIter = m_sendPacketsPerClient.begin(); vecIter != m_sendPacketsPerClient.end(); ++vecIter )
	{
		std::vector< LobbyPacket > sentPackets = vecIter->second;
		for( unsigned int packetIndex = 0; packetIndex < sentPackets.size(); ++packetIndex )
		{
			LobbyPacket* packet = &sentPackets[ packetIndex ];
			if( ( GetCurrentTimeSeconds() - packet->timestamp ) > SECONDS_BEFORE_RESEND_RELIABLE_PACKETS )
			{
				packet->packetNumber = m_nextPacketNumber;
				packet->timestamp = GetCurrentTimeSeconds();
				SendPacketToClient( *packet, vecIter->first, false );
			}
		}
	}
}