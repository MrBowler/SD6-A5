#ifndef include_Lobby
#define include_Lobby
#pragma once

//-----------------------------------------------------------------------------------------------
#include <map>
#include <set>
#include "Player.hpp"
#include "GameServer.hpp"
#include "LobbyPacket.hpp"


//-----------------------------------------------------------------------------------------------
const double SECONDS_BEFORE_SEND_LOBBY_UPDATE = 5.0;


//-----------------------------------------------------------------------------------------------
class Lobby
{
public:
	void Initalize();
	void Update();

private:
	void SendPacketToClient( const LobbyPacket& pkt, const ClientInfo& info, bool requireAck );
	void SendPacketToAllClients( const LobbyPacket& pkt, bool requireAck );
	std::string ConvertNumberToString( int number );
	void UpdateGames();
	void GetPackets();
	void SendLobbyUpdates();
	void RemovePlayerFromLobby( const ClientInfo& info );
	void AcknowledgeConnection( const LobbyPacket& packet, const ClientInfo& info );
	void ProcessAckPackets( const LobbyPacket& ackPacket, const ClientInfo& info );
	void CreateGame( const LobbyPacket& createPacket, const ClientInfo& gameOwner );
	void AddPlayersToLobby( const GameServer* game );
	void AddPlayerToGame( const LobbyPacket& joinPacket, const ClientInfo& info );
	void ResendAckPackets();

	UDPServer											m_server;
	unsigned int										m_nextPacketNumber;
	unsigned int										m_nextGameID;
	unsigned short										m_nextPortNumber;
	double												m_lastUpdateTime;
	std::set< ClientInfo >								m_lobbyPlayers;
	std::map< int, GameServer* >						m_games;
	std::map< ClientInfo, std::vector< LobbyPacket > >	m_sendPacketsPerClient;
};


#endif // include_Lobby