#ifndef include_GameServer
#define include_GameServer
#pragma once

//-----------------------------------------------------------------------------------------------
#include <map>
#include <set>
#include <string>
#include <time.h>
#include <vector>
#include <sstream>
#include <iostream>
#include "Player.hpp"
#include "Color3b.hpp"
#include "CS6Packet.hpp"
#include "UDPServer.hpp"
#include "ClientInfo.hpp"
#include "../Engine/Time.hpp"


//-----------------------------------------------------------------------------------------------
const unsigned short PORT_NUMBER = 5000;
const int MAP_SIZE_WIDTH = 500;
const int MAP_SIZE_HEIGHT = 500;
const double SECONDS_BEFORE_SEND_UPDATE = 0.1;
const double SECONDS_BEFORE_RESEND_RELIABLE_PACKETS = 0.25;
const double SECONDS_BEFORE_TIMEOUT_REMOVE = 5.0;


//-----------------------------------------------------------------------------------------------
class GameServer
{
public:
	GameServer() {}
	void Initalize();
	void Update();
	void AddPlayer( const ClientInfo& info );
	unsigned int GetNumberOfPlayers();

	bool								m_isGameOver;
	bool								m_addedPlayersToLobby;
	unsigned int						m_gameID;
	unsigned short						m_portNumber;
	std::string							m_ownerName;
	std::map< ClientInfo, Player* >		m_players;

private:
	void SendPacketToClient( const CS6Packet& pkt, const ClientInfo& info, bool requireAck );
	void SendPacketToAllClients( const CS6Packet& pkt, bool requireAck );
	std::string ConvertNumberToString( int number );
	Color3b GetPlayerColorForID( unsigned int playerID );
	Vector2 GetRandomPosition();
	void ProcessAckPackets( const CS6Packet& ackPacket, const ClientInfo& info );
	void RemovePlayer( const ClientInfo& info );
	void CheckForTimeOutPlayers();
	void ResetGame( const CS6Packet& victoryPacket, const ClientInfo& info );
	void UpdatePlayer( const CS6Packet& updatePacket, const ClientInfo& info );
	void SendUpdatesToClients();
	void SendGameOverToClients();
	void GetPackets();
	void ResendAckPackets();

	UDPServer											m_server;
	unsigned int										m_nextPacketNumber;
	double												m_lastUpdateTime;
	int													m_numFlagsCaptured;
	Vector2												m_flagPosition;
	std::map< ClientInfo, std::vector< CS6Packet > >	m_sendPacketsPerClient;
};


#endif // include_GameServer