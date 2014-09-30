#ifndef include_Color3b
#define include_Color3b
#pragma once

//-----------------------------------------------------------------------------------------------
struct Color3b
{
	Color3b() : r( 0 ), g( 0 ), b( 0 ) {}
	Color3b( unsigned char initialR, unsigned char initialG, unsigned char initialB ) : r( initialR ), g( initialG ), b( initialB ) {}
	bool operator==( const Color3b& lhv ) const;

	unsigned char r;
	unsigned char g;
	unsigned char b;
};


//-----------------------------------------------------------------------------------------------
inline bool Color3b::operator==( const Color3b& lhv ) const
{
	if( lhv.r == this->r && lhv.g == this->g && lhv.b == this->b )
		return true;

	return false;
}


#endif // include_Color3b