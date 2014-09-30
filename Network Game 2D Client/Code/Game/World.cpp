#include "World.hpp"
#include "../Engine/Time.hpp"
#include "../Engine/DeveloperConsole.hpp"
#include "../Engine/NewMacroDef.hpp"

//-----------------------------------------------------------------------------------------------
World::World( float worldWidth, float worldHeight )
	: m_size( worldWidth, worldHeight )
	, m_playerTexture( nullptr )
	, m_isConnectedToServer( false )
	, m_isConnectedToGame( false )
	, m_hasInitializedGame( false )
	, m_hasFlag( false )
	, m_nextPacketNumber( 0 )
	, m_flagPosition( worldWidth, worldHeight )
{

}


//-----------------------------------------------------------------------------------------------
void World::Initialize()
{
	InitializeTime();
	m_client.ConnectToServer( IP_ADDRESS, PORT_NUMBER );
	m_playerTexture = Texture::CreateOrGetTexture( PLAYER_TEXTURE_FILE_PATH );
	m_flagTexture = Texture::CreateOrGetTexture( FLAG_TEXTURE_FILE_PATH );
	m_secondsSinceLastInitSend = GetCurrentTimeSeconds();

	m_mainPlayer = new Player;
	m_players.push_back( m_mainPlayer );
}


//-----------------------------------------------------------------------------------------------
void World::Destruct()
{
	m_client.DisconnectFromServer();
}


//-----------------------------------------------------------------------------------------------
void World::ChangeIPAddress( const std::string& ipAddrString )
{
	//unsigned short currentServerPortNumber = m_client.GetServerPortNumber();
	//m_client.DisconnectFromServer();
	//m_client.ConnectToServer( ipAddrString, currentServerPortNumber );
	m_client.SetServerIPAddress( ipAddrString );

	m_isConnectedToServer = false;
}


//-----------------------------------------------------------------------------------------------
void World::ChangePortNumber( unsigned short portNumber )
{
	//std::string currentServerIPAddress = m_client.GetServerIPAddress();
	//m_client.DisconnectFromServer();
	//m_client.ConnectToServer( currentServerIPAddress, portNumber );
	m_client.SetServerPortNumber( portNumber );

	m_isConnectedToServer = false;
}


//-----------------------------------------------------------------------------------------------
void World::ShowLobbyGames()
{
	if( !m_isConnectedToServer )
	{
		ConsoleLogLine logLine( "Not connected to server\n", Color::Red );
		g_developerConsole.m_consoleLogLines.push_back( logLine );
		return;
	}

	if( m_lobbyGames.size() == 0 )
	{
		ConsoleLogLine logLine( "No games found\n", Color::Blue );
		g_developerConsole.m_consoleLogLines.push_back( logLine );
		return;
	}

	for( unsigned int gameIndex = 0; gameIndex < m_lobbyGames.size(); ++gameIndex )
	{
		const GameInfo& game = m_lobbyGames[ gameIndex ];

		ConsoleLogLine logLine0( "Game ID: " + ConvertNumberToString( game.m_id ), Color::Blue );
		g_developerConsole.m_consoleLogLines.push_back( logLine0 );

		ConsoleLogLine logLine1( "    Game Owner: " + game.m_ownerName, Color::Blue );
		g_developerConsole.m_consoleLogLines.push_back( logLine1 );

		ConsoleLogLine logLine2( "    Players In Game: " + ConvertNumberToString( game.m_numPlayersInGame ), Color::Blue );
		g_developerConsole.m_consoleLogLines.push_back( logLine2 );
	}
}


//-----------------------------------------------------------------------------------------------
void World::CreateLobbyGame()
{
	if( !m_isConnectedToServer )
	{
		ConsoleLogLine logLine( "Not connected to server\n", Color::Red );
		g_developerConsole.m_consoleLogLines.push_back( logLine );
		return;
	}

	LobbyPacket createPacket;
	createPacket.packetNumber = m_nextPacketNumber;
	createPacket.packetType = LOBBY_TYPE_CreateGame;
	createPacket.timestamp = GetCurrentTimeSeconds();
	
	SendPacket( createPacket, true );
}


//-----------------------------------------------------------------------------------------------
void World::JoinLobbyGame( unsigned int gameID )
{
	if( !m_isConnectedToServer )
	{
		ConsoleLogLine logLine( "Not connected to server\n", Color::Red );
		g_developerConsole.m_consoleLogLines.push_back( logLine );
		return;
	}

	LobbyPacket joinPacket;
	joinPacket.packetNumber = m_nextPacketNumber;
	joinPacket.packetType = LOBBY_TYPE_JoinGame;
	joinPacket.timestamp = GetCurrentTimeSeconds();
	joinPacket.data.join.gameID = gameID;

	SendPacket( joinPacket, true );
}


//-----------------------------------------------------------------------------------------------
void World::Update( float deltaSeconds, const Keyboard& keyboard, const Mouse& mouse )
{
	UpdateFromInput( keyboard, mouse, deltaSeconds );
	CheckForFlagCapture();
	ReceivePackets();
	ApplyDeadReckoning();
	SendUpdate();
	ResendAckPackets();
	RemoveTimedOutLobbyGames();
	RemoveTimedOutPlayers();
}


//-----------------------------------------------------------------------------------------------
void World::RenderObjects3D()
{
	
}


//-----------------------------------------------------------------------------------------------
void World::RenderObjects2D()
{
	if( !m_hasInitializedGame )
		return;

	RenderPlayers();
	RenderFlag();
}


//-----------------------------------------------------------------------------------------------
void World::SendPacket( const CS6Packet& packet, bool requireAck )
{
	m_client.SendPacketToServer( (const char*) &packet, sizeof( packet ) );
	++m_nextPacketNumber;

	if( requireAck )
	{
		m_sentGamePackets.push_back( packet );
	}
}


//-----------------------------------------------------------------------------------------------
void World::SendPacket( const LobbyPacket& packet, bool requireAck )
{
	m_client.SendPacketToServer( (const char*) &packet, sizeof( packet ) );
	++m_nextPacketNumber;

	if( requireAck )
	{
		m_sentLobbyPackets.push_back( packet );
	}
}


//-----------------------------------------------------------------------------------------------
void World::SendJoinGamePacket()
{
	LobbyPacket joinPacket;
	joinPacket.packetNumber = m_nextPacketNumber;
	joinPacket.packetType = LOBBY_TYPE_Acknowledge;
	joinPacket.timestamp = GetCurrentTimeSeconds();
	joinPacket.data.acknowledged.packetType = LOBBY_TYPE_Acknowledge;

	SendPacket( joinPacket, false );
}


//-----------------------------------------------------------------------------------------------
void World::ProcessAckPackets( const CS6Packet& ackPacket )
{
	for( unsigned int packetIndex = 0; packetIndex < m_sentGamePackets.size(); ++packetIndex )
	{
		CS6Packet packet = m_sentGamePackets[ packetIndex ];
		if( packet.packetNumber == ackPacket.data.acknowledged.packetNumber )
		{
			m_sentGamePackets.erase( m_sentGamePackets.begin() + packetIndex );
			break;
		}
	}
}


//-----------------------------------------------------------------------------------------------
void World::ProcessAckPackets( const LobbyPacket& ackPacket )
{
	if( ackPacket.data.acknowledged.packetType == LOBBY_TYPE_Acknowledge )
	{
		m_isConnectedToServer = true;
	}
	else if( ackPacket.data.acknowledged.packetType == LOBBY_TYPE_JoinGame || ackPacket.data.acknowledged.packetType == LOBBY_TYPE_CreateGame )
	{
		ChangePortNumber( ackPacket.data.acknowledged.portNumber );
		m_isConnectedToServer = true;
		m_isConnectedToGame = true;
	}

	for( unsigned int packetIndex = 0; packetIndex < m_sentLobbyPackets.size(); ++packetIndex )
	{
		LobbyPacket packet = m_sentLobbyPackets[ packetIndex ];
		if( packet.packetNumber == ackPacket.data.acknowledged.packetNumber )
		{
			m_sentLobbyPackets.erase( m_sentLobbyPackets.begin() + packetIndex );
			break;
		}
	}
}


//-----------------------------------------------------------------------------------------------
void World::ResetGame( const CS6Packet& resetPacket )
{
	m_isConnectedToServer = true;
	m_hasInitializedGame = true;
	m_hasFlag = false;

	m_mainPlayer->m_color.r = resetPacket.data.reset.playerColorAndID[0];
	m_mainPlayer->m_color.g = resetPacket.data.reset.playerColorAndID[1];
	m_mainPlayer->m_color.b = resetPacket.data.reset.playerColorAndID[2];
	m_mainPlayer->m_currentPosition.x = resetPacket.data.reset.playerXPosition;
	m_mainPlayer->m_currentPosition.y = resetPacket.data.reset.playerYPosition;
	m_mainPlayer->m_lastUpdatePosition = m_mainPlayer->m_currentPosition;
	m_mainPlayer->m_currentVelocity = Vector2( 0.f, 0.f );
	m_mainPlayer->m_orientationDegrees = 0.f;
	m_flagPosition.x = resetPacket.data.reset.flagXPosition;
	m_flagPosition.y = resetPacket.data.reset.flagYPosition;

	CS6Packet ackPacket;
	ackPacket.packetNumber = m_nextPacketNumber;
	ackPacket.packetType = TYPE_Acknowledge;
	ackPacket.playerColorAndID[0] = m_mainPlayer->m_color.r;
	ackPacket.playerColorAndID[1] = m_mainPlayer->m_color.g;
	ackPacket.playerColorAndID[2] = m_mainPlayer->m_color.b;
	ackPacket.timestamp = GetCurrentTimeSeconds();
	ackPacket.data.acknowledged.packetNumber = resetPacket.packetNumber;
	ackPacket.data.acknowledged.packetType = TYPE_Reset;

	SendPacket( ackPacket, false );
}


//-----------------------------------------------------------------------------------------------
void World::UpdatePlayer( const CS6Packet& updatePacket )
{
	if( !m_hasInitializedGame )
		return;

	for( unsigned int playerIndex = 0; playerIndex < m_players.size(); ++playerIndex )
	{
		Player* player = m_players[ playerIndex ];
		if( player->m_color.r == updatePacket.playerColorAndID[0]
		&& player->m_color.g == updatePacket.playerColorAndID[1]
		&& player->m_color.b == updatePacket.playerColorAndID[2] )
		{
			if( player == m_mainPlayer )
				return;

			player->m_lastUpdatePosition.x = updatePacket.data.updated.xPosition;
			player->m_lastUpdatePosition.y = updatePacket.data.updated.yPosition;
			player->m_lastUpdateVelocity = player->m_currentVelocity;
			player->m_currentVelocity.x = updatePacket.data.updated.xVelocity;
			player->m_currentVelocity.y = updatePacket.data.updated.yVelocity;
			player->m_orientationDegrees = updatePacket.data.updated.yawDegrees;
			player->m_timeOfLastUpdate = GetCurrentTimeSeconds();

			return;
		}
	}

	Player* player = new Player();
	player->m_color.r = updatePacket.playerColorAndID[0];
	player->m_color.g = updatePacket.playerColorAndID[1];
	player->m_color.b = updatePacket.playerColorAndID[2];
	player->m_currentPosition.x = updatePacket.data.updated.xPosition;
	player->m_currentPosition.y = updatePacket.data.updated.yPosition;
	player->m_lastUpdatePosition = player->m_currentPosition;
	player->m_currentVelocity.x = updatePacket.data.updated.xVelocity;
	player->m_currentVelocity.y = updatePacket.data.updated.yVelocity;
	player->m_orientationDegrees = updatePacket.data.updated.yawDegrees;
	player->m_timeOfLastUpdate = GetCurrentTimeSeconds();

	m_players.push_back( player );
}


//-----------------------------------------------------------------------------------------------
void World::UpdateFromInput( const Keyboard& keyboard, const Mouse&, float deltaSeconds )
{
	if( g_developerConsole.m_drawConsole )
		return;

	Vector2 velocity;

	bool isEast = keyboard.IsKeyPressedDown( KEY_D );
	bool isNorth = keyboard.IsKeyPressedDown( KEY_W );
	bool isWest = keyboard.IsKeyPressedDown( KEY_A );
	bool isSouth = keyboard.IsKeyPressedDown( KEY_S );

	bool isNorthEast = isEast & isNorth;
	bool isNorthWest = isWest & isNorth;
	bool isSouthWest = isWest & isSouth;
	bool isSouthEast = isEast & isSouth;

	if( isNorthEast )
	{
		velocity = Vector2( 1.f, 1.f );
		m_mainPlayer->m_orientationDegrees = 45.f;
	}
	else if( isNorthWest )
	{
		velocity = Vector2( -1.f, 1.f );
		m_mainPlayer->m_orientationDegrees = 135.f;
	}
	else if( isSouthWest )
	{
		velocity = Vector2( -1.f, -1.f );
		m_mainPlayer->m_orientationDegrees = 225.f;
	}
	else if( isSouthEast )
	{
		velocity = Vector2( 1.f, -1.f );
		m_mainPlayer->m_orientationDegrees = 315.f;
	}
	else if( isEast )
	{
		velocity = Vector2( 1.f, 0.f );
		m_mainPlayer->m_orientationDegrees = 0.f;
	}
	else if( isNorth )
	{
		velocity = Vector2( 0.f, 1.f );
		m_mainPlayer->m_orientationDegrees = 90.f;
	}
	else if( isWest )
	{
		velocity = Vector2( -1.f, 0.f );
		m_mainPlayer->m_orientationDegrees = 180.f;
	}
	else if( isSouth )
	{
		velocity = Vector2( 0.f, -1.f );
		m_mainPlayer->m_orientationDegrees = 270.f;
	}

	velocity.Normalize();
	m_mainPlayer->m_currentVelocity = velocity * SPEED_PIXELS_PER_SECOND;
	m_mainPlayer->m_currentPosition = m_mainPlayer->m_currentPosition + m_mainPlayer->m_currentVelocity * deltaSeconds;

	m_mainPlayer->m_currentPosition.x = ClampFloat( m_mainPlayer->m_currentPosition.x, 0.f, m_size.x );
	m_mainPlayer->m_currentPosition.y = ClampFloat( m_mainPlayer->m_currentPosition.y, 0.f, m_size.y );

	m_mainPlayer->m_timeOfLastUpdate = GetCurrentTimeSeconds();
}


//-----------------------------------------------------------------------------------------------
void World::SendUpdate()
{
	if( !m_isConnectedToServer && ( m_secondsSinceLastInitSend > SECONDS_BEFORE_RESEND_INIT_PACKET ) )
	{
		SendJoinGamePacket();
		m_secondsSinceLastInitSend = GetCurrentTimeSeconds();
		return;
	}

	if( !m_hasInitializedGame )
		return;
	
	CS6Packet updatePacket;
	updatePacket.packetType = TYPE_Update;
	updatePacket.playerColorAndID[0] = m_mainPlayer->m_color.r;
	updatePacket.playerColorAndID[1] = m_mainPlayer->m_color.g;
	updatePacket.playerColorAndID[2] = m_mainPlayer->m_color.b;
	updatePacket.timestamp = GetCurrentTimeSeconds();
	updatePacket.data.updated.xPosition = m_mainPlayer->m_currentPosition.x;
	updatePacket.data.updated.yPosition = m_mainPlayer->m_currentPosition.y;
	updatePacket.data.updated.xVelocity = m_mainPlayer->m_currentVelocity.x;
	updatePacket.data.updated.yVelocity = m_mainPlayer->m_currentVelocity.y;
	updatePacket.data.updated.yawDegrees = m_mainPlayer->m_orientationDegrees;

	SendPacket( updatePacket, false );
}


//-----------------------------------------------------------------------------------------------
void World::SendVictory()
{
	CS6Packet victoryPacket;
	victoryPacket.packetNumber = m_nextPacketNumber;
	victoryPacket.packetType = TYPE_Victory;
	victoryPacket.playerColorAndID[0] = m_mainPlayer->m_color.r;
	victoryPacket.playerColorAndID[1] = m_mainPlayer->m_color.g;
	victoryPacket.playerColorAndID[2] = m_mainPlayer->m_color.b;
	victoryPacket.timestamp = GetCurrentTimeSeconds();
	victoryPacket.data.victorious.playerColorAndID[0] = m_mainPlayer->m_color.r;
	victoryPacket.data.victorious.playerColorAndID[1] = m_mainPlayer->m_color.g;
	victoryPacket.data.victorious.playerColorAndID[2] = m_mainPlayer->m_color.b;

	SendPacket( victoryPacket, true );
}


//-----------------------------------------------------------------------------------------------
void World::CheckForFlagCapture()
{
	if( !m_isConnectedToServer || m_hasFlag )
		return;

	Vector2 flagPositionDifference = m_flagPosition - m_mainPlayer->m_currentPosition;
	float distanceToFlag = flagPositionDifference.GetLength();
	if( distanceToFlag <= DISTANCE_FROM_FLAG_FOR_PICKUP_PIXELS )
	{
		SendVictory();
		m_hasFlag = true;
	}
}


//-----------------------------------------------------------------------------------------------
void World::AcknowledgeGameOver( const CS6Packet& gameOverPacket )
{
	CS6Packet ackPacket;
	ackPacket.packetNumber = m_nextPacketNumber;
	ackPacket.packetType = TYPE_Acknowledge;
	ackPacket.playerColorAndID[0] = m_mainPlayer->m_color.r;
	ackPacket.playerColorAndID[1] = m_mainPlayer->m_color.g;
	ackPacket.playerColorAndID[2] = m_mainPlayer->m_color.b;
	ackPacket.timestamp = GetCurrentTimeSeconds();
	ackPacket.data.acknowledged.packetNumber = gameOverPacket.packetNumber;
	ackPacket.data.acknowledged.packetType = TYPE_GameOver;

	SendPacket( ackPacket, false );

	for( unsigned int playerIndex = 0; playerIndex < m_players.size(); ++playerIndex )
	{
		Player* player = m_players.back();
		if( player != m_mainPlayer )
		{
			delete player;
			m_players.pop_back();
			--playerIndex;
		}
	}

	ChangePortNumber( PORT_NUMBER );

	m_isConnectedToGame = false;
	m_hasInitializedGame = false;
}


//-----------------------------------------------------------------------------------------------
void World::UpdateLobbyGames( const LobbyPacket& updatePacket )
{
	for( unsigned int gameIndex = 0; gameIndex < m_lobbyGames.size(); ++gameIndex )
	{
		GameInfo& game = m_lobbyGames[ gameIndex ];
		if( game.m_id == updatePacket.data.update.gameID )
		{
			game.m_numPlayersInGame = updatePacket.data.update.numPlayersInGame;
			game.m_ownerName = updatePacket.data.update.gameOwner;
			game.m_lastUpdateTime = GetCurrentTimeSeconds();
			return;
		}
	}

	GameInfo game;
	game.m_id = updatePacket.data.update.gameID;
	game.m_numPlayersInGame = updatePacket.data.update.numPlayersInGame;
	game.m_ownerName = updatePacket.data.update.gameOwner;
	game.m_lastUpdateTime = GetCurrentTimeSeconds();
	m_lobbyGames.push_back( game );
}


//-----------------------------------------------------------------------------------------------
void World::ResendAckPackets()
{
	for( unsigned int packetIndex = 0; packetIndex < m_sentGamePackets.size(); ++packetIndex )
	{
		CS6Packet* packet = &m_sentGamePackets[ packetIndex ];
		if( ( GetCurrentTimeSeconds() - packet->timestamp ) > SECONDS_BEFORE_RESEND_INIT_PACKET )
		{
			packet->packetNumber = m_nextPacketNumber;
			packet->timestamp = GetCurrentTimeSeconds();
			SendPacket( *packet, false );
		}
	}

	for( unsigned int packetIndex = 0; packetIndex < m_sentLobbyPackets.size(); ++packetIndex )
	{
		LobbyPacket* packet = &m_sentLobbyPackets[ packetIndex ];
		if( ( GetCurrentTimeSeconds() - packet->timestamp ) > SECONDS_BEFORE_RESEND_INIT_PACKET )
		{
			packet->packetNumber = m_nextPacketNumber;
			packet->timestamp = GetCurrentTimeSeconds();
			SendPacket( *packet, false );
		}
	}
}


//-----------------------------------------------------------------------------------------------
void World::ApplyDeadReckoning()
{
	for( unsigned int playerIndex = 0; playerIndex < m_players.size(); ++playerIndex )
	{
		Player* player = m_players[ playerIndex ];
		if( player == m_mainPlayer )
			continue;

		float deltaSeconds = (float) ( GetCurrentTimeSeconds() - player->m_timeOfLastUpdate );
		player->m_currentPosition = player->m_lastUpdatePosition + player->m_currentVelocity * deltaSeconds;

		player->m_currentPosition.x = ClampFloat( player->m_currentPosition.x, 0.f, m_size.x );
		player->m_currentPosition.y = ClampFloat( player->m_currentPosition.y, 0.f, m_size.y );
	}
}


//-----------------------------------------------------------------------------------------------
void World::ReceivePackets()
{
	if( m_isConnectedToGame )
		ReceiveGamePackets();
	else
		ReceiveLobbyPackets();
}


//-----------------------------------------------------------------------------------------------
void World::ReceiveLobbyPackets()
{
	LobbyPacket packet;
	std::set< LobbyPacket > recvPackets;

	while( m_client.ReceivePacketFromServer( (char*) &packet, sizeof( packet ) ) )
	{
		recvPackets.insert( packet );
	}

	std::set< LobbyPacket >::iterator setIter;
	for( setIter = recvPackets.begin(); setIter != recvPackets.end(); ++setIter )
	{
		LobbyPacket orderedPacket = *setIter;

		if( orderedPacket.packetType == LOBBY_TYPE_Update )
		{
			UpdateLobbyGames( orderedPacket );
		}
		if( orderedPacket.packetType == LOBBY_TYPE_Acknowledge )
		{
			ProcessAckPackets( orderedPacket );
		}
	}
}


//-----------------------------------------------------------------------------------------------
void World::ReceiveGamePackets()
{
	CS6Packet packet;
	std::set< CS6Packet > recvPackets;

	while( m_client.ReceivePacketFromServer( (char*) &packet, sizeof( packet ) ) )
	{
		recvPackets.insert( packet );
	}

	std::set< CS6Packet >::iterator setIter;
	for( setIter = recvPackets.begin(); setIter != recvPackets.end(); ++setIter )
	{
		CS6Packet orderedPacket = *setIter;

		if( orderedPacket.packetType == TYPE_Update )
		{
			UpdatePlayer( orderedPacket );
		}
		else if( orderedPacket.packetType == TYPE_Reset )
		{
			ResetGame( orderedPacket );
		}
		else if( orderedPacket.packetType == TYPE_Acknowledge )
		{
			ProcessAckPackets( orderedPacket );
		}
		else if( orderedPacket.packetType == TYPE_GameOver )
		{
			AcknowledgeGameOver( orderedPacket );
		}
	}
}


//-----------------------------------------------------------------------------------------------
void World::RemoveTimedOutLobbyGames()
{
	for( unsigned int gameIndex = 0; gameIndex < m_lobbyGames.size(); ++gameIndex )
	{
		GameInfo game = m_lobbyGames[ gameIndex ];
		if( ( GetCurrentTimeSeconds() - game.m_lastUpdateTime ) > SECONDS_BEFORE_TIMEOUT_REMOVE )
		{
			m_lobbyGames.erase( m_lobbyGames.begin() + gameIndex );
			--gameIndex;
		}
	}
}


//-----------------------------------------------------------------------------------------------
void World::RemoveTimedOutPlayers()
{
	for( unsigned int playerIndex = 0; playerIndex < m_players.size(); ++playerIndex )
	{
		Player* player = m_players[ playerIndex ];
		if( ( GetCurrentTimeSeconds() - player->m_timeOfLastUpdate ) > SECONDS_BEFORE_TIMEOUT_REMOVE )
		{
			m_players.erase( m_players.begin() + playerIndex );
			delete player;
			--playerIndex;
		}
	}
}


//-----------------------------------------------------------------------------------------------
void World::RenderPlayers()
{
	OpenGLRenderer::EnableTexture2D();
	OpenGLRenderer::BindTexture2D( m_playerTexture->m_openglTextureID );

	for( unsigned int playerIndex = 0; playerIndex < m_players.size(); ++playerIndex )
	{
		Player* player = m_players[ playerIndex ];
		float colorR = player->m_color.r * ONE_OVER_TWO_HUNDRED_TWENTY_FIVE;
		float colorG = player->m_color.g * ONE_OVER_TWO_HUNDRED_TWENTY_FIVE;
		float colorB = player->m_color.b * ONE_OVER_TWO_HUNDRED_TWENTY_FIVE;

		OpenGLRenderer::PushMatrix();

		OpenGLRenderer::SetColor3f( colorR, colorG, colorB );
		OpenGLRenderer::Translatef( player->m_currentPosition.x, player->m_currentPosition.y, 0.f );
		OpenGLRenderer::Rotatef( -player->m_orientationDegrees, 0.f, 0.f, 1.f );

		OpenGLRenderer::BeginRender( QUADS );
		{
			OpenGLRenderer::SetTexCoords2f( 0.f, 1.f );
			OpenGLRenderer::SetVertex2f( -ONE_HALF_POINT_SIZE_PIXELS, -ONE_HALF_POINT_SIZE_PIXELS );

			OpenGLRenderer::SetTexCoords2f( 1.f, 1.f );
			OpenGLRenderer::SetVertex2f( ONE_HALF_POINT_SIZE_PIXELS, -ONE_HALF_POINT_SIZE_PIXELS );

			OpenGLRenderer::SetTexCoords2f( 1.f, 0.f );
			OpenGLRenderer::SetVertex2f( ONE_HALF_POINT_SIZE_PIXELS, ONE_HALF_POINT_SIZE_PIXELS );

			OpenGLRenderer::SetTexCoords2f( 0.f, 0.f );
			OpenGLRenderer::SetVertex2f( -ONE_HALF_POINT_SIZE_PIXELS, ONE_HALF_POINT_SIZE_PIXELS );
		}
		OpenGLRenderer::EndRender();

		OpenGLRenderer::PopMatrix();
	}

	OpenGLRenderer::BindTexture2D( 0 );
	OpenGLRenderer::DisableTexture2D();
}


//-----------------------------------------------------------------------------------------------
void World::RenderFlag()
{
	OpenGLRenderer::EnableTexture2D();
	OpenGLRenderer::BindTexture2D( m_flagTexture->m_openglTextureID );
	OpenGLRenderer::SetColor3f( 1.f, 1.f, 1.f );

	OpenGLRenderer::BeginRender( QUADS );
	{
		OpenGLRenderer::SetTexCoords2f( 0.f, 1.f );
		OpenGLRenderer::SetVertex2f( m_flagPosition.x - ONE_HALF_POINT_SIZE_PIXELS, m_flagPosition.y - ONE_HALF_POINT_SIZE_PIXELS );

		OpenGLRenderer::SetTexCoords2f( 1.f, 1.f );
		OpenGLRenderer::SetVertex2f( m_flagPosition.x + ONE_HALF_POINT_SIZE_PIXELS, m_flagPosition.y - ONE_HALF_POINT_SIZE_PIXELS );

		OpenGLRenderer::SetTexCoords2f( 1.f, 0.f );
		OpenGLRenderer::SetVertex2f( m_flagPosition.x + ONE_HALF_POINT_SIZE_PIXELS, m_flagPosition.y + ONE_HALF_POINT_SIZE_PIXELS );

		OpenGLRenderer::SetTexCoords2f( 0.f, 0.f );
		OpenGLRenderer::SetVertex2f( m_flagPosition.x - ONE_HALF_POINT_SIZE_PIXELS, m_flagPosition.y + ONE_HALF_POINT_SIZE_PIXELS );
	}
	OpenGLRenderer::EndRender();

	OpenGLRenderer::BindTexture2D( 0 );
	OpenGLRenderer::DisableTexture2D();
}