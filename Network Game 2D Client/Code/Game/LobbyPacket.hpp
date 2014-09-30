#ifndef include_LobbyPacket
#define include_LobbyPacket
#pragma once

//-----------------------------------------------------------------------------------------------
typedef unsigned char PacketType;
static const PacketType LOBBY_TYPE_Acknowledge = 20;
static const PacketType LOBBY_TYPE_Update = 21;
static const PacketType LOBBY_TYPE_CreateGame = 22;
static const PacketType LOBBY_TYPE_JoinGame = 23;


//-----------------------------------------------------------------------------------------------
struct AckPacketLobby
{
	PacketType packetType;
	unsigned short portNumber;
	unsigned int packetNumber;
};


//-----------------------------------------------------------------------------------------------
struct UpdatePacketLobby
{
	unsigned int gameID;
	unsigned char numPlayersInGame;
	char gameOwner[20];
};


//-----------------------------------------------------------------------------------------------
struct JoinGamePacketLobby
{
	unsigned int gameID;
};


//-----------------------------------------------------------------------------------------------
struct LobbyPacket
{
	bool operator<( const LobbyPacket& other ) const;

	PacketType packetType;
	unsigned int packetNumber;
	double timestamp;
	union PacketData
	{
		AckPacketLobby acknowledged;
		UpdatePacketLobby update;
		JoinGamePacketLobby join;
	} data;
};


//-----------------------------------------------------------------------------------------------
inline bool LobbyPacket::operator<( const LobbyPacket& other ) const
{
	return this->packetNumber < other.packetNumber;
}


#endif // include_LobbyPacket