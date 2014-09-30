#include "NewDeleteFunctions.hpp"
#include <malloc.h>
#include "EngineCommon.hpp"
#include "MemoryManager.hpp"
#include "ErrorWarningAssertions.hpp"


//-----------------------------------------------------------------------------------------------
#undef new
#define UNUSED(x) (void)(x);
#define USING_MEMORY_MANAGER


//-----------------------------------------------------------------------------------------------
void* operator new( size_t size )
{
#ifdef USING_MEMORY_MANAGER
	if( !MemoryManager::IsMemoryManagerAvailable() )
		MemoryManager::Initialize( POOL_MEMORY_IN_BYTES );
		
	void* data = MemoryManager::AllocateMemory( size );
	if( data == nullptr )
	{
		static const std::bad_alloc nomem;
		_RAISE(nomem);
	}

	return data;
#else
	return malloc( size );
#endif
}


//-----------------------------------------------------------------------------------------------
void operator delete( void* data )
{
#ifdef USING_MEMORY_MANAGER
	MemoryManager::FreeMemory( data );
#else
	free( data );
#endif
}


//-----------------------------------------------------------------------------------------------
void* operator new[]( size_t size )
{
	return ( operator new( size ) );
}


//-----------------------------------------------------------------------------------------------
void operator delete[]( void* data )
{
	operator delete( data );
}


//-----------------------------------------------------------------------------------------------
void* operator new( size_t size, const char* file, unsigned int line )
{
#ifdef USING_MEMORY_MANAGER
	if( !MemoryManager::IsMemoryManagerAvailable() )
		MemoryManager::Initialize( POOL_MEMORY_IN_BYTES );

	void* data = MemoryManager::AllocateMemory( size, file, line );
	if( data == nullptr )
	{
		static const std::bad_alloc nomem;
		_RAISE(nomem);
	}

	return data;
#else
	UNUSED( file );
	UNUSED( line );
	return malloc( size );
#endif
}


//-----------------------------------------------------------------------------------------------
void operator delete( void* data, const char*, unsigned int )
{
	operator delete( data );
}


//-----------------------------------------------------------------------------------------------
void* operator new[]( size_t size, const char* file, unsigned int line )
{
	return ( operator new( size, file, line ) );
}


//-----------------------------------------------------------------------------------------------
void operator delete[]( void* data, const char*, unsigned int )
{
	operator delete( data );
}