#ifndef include_GameInfo
#define include_GameInfo
#pragma once

//-----------------------------------------------------------------------------------------------
#include <string>


//-----------------------------------------------------------------------------------------------
struct GameInfo
{
	unsigned int	m_id;
	unsigned char	m_numPlayersInGame;
	std::string		m_ownerName;
	double			m_lastUpdateTime;
};


#endif // include_GameInfo