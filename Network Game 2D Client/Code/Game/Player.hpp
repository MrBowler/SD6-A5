#ifndef include_Player
#define include_Player
#pragma once

//-----------------------------------------------------------------------------------------------
#include "Color3b.hpp"
#include "../Engine/Vector2.hpp"


//-----------------------------------------------------------------------------------------------
struct Player
{
	Color3b		m_color;
	Vector2		m_currentPosition;
	Vector2		m_lastUpdatePosition;
	Vector2		m_currentVelocity;
	Vector2		m_lastUpdateVelocity;
	float		m_orientationDegrees;
	double		m_timeOfLastUpdate;
};



#endif // include_Player