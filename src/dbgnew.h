#ifndef DEBUGNEW_H__
#define DEBUGNEW_H__

// MicroMath+ - (c) Ugo Varetto

/// @file dbgnew.h overloaded new & delete operators used to tace memory allocation in conjunction with MemTracer class

#include <cstdlib>

#include "mem_tracer.h"

/// Declaration of global instance of MemTracer class used to store
/// memory usage information
extern MemTracer NewTrace;

/// Overloaded operator new.
/// @param size of memory chunk to allocate
/// @param file __FILE__
/// @param line __LINE__
/// @param fun __FUNCTION__
inline void * operator new ( size_t size, char const * file, int line, const char* fun )
{
    void* p = std::malloc( size );
    if( MemTracer::Ready ) NewTrace.Add( p, file, line, fun, size );
    return p;
}

/// Called in case an exception is thrown in the constructor of an object allocated
/// through the new (size_t size, char const * file, int line, const char* fun)
/// operator.
inline void operator delete( void * p, char const * file, int line, const char* fun )
{
    if( MemTracer::Ready ) NewTrace.Remove( p );
    std::free( p );
}

/// Overloaded operator new called from outside mmath_plus.
/// @param size byte size of memory chunk to allocate
inline void* operator new( size_t size )
{
	void* p = std::malloc( size );
    if( MemTracer::Ready ) NewTrace.Add( p, "?", 0, 0, size );
    return p;
}

/// Overloaded delete operator. 
inline void operator delete( void* p )
{
    if( MemTracer::Ready ) NewTrace.Remove ( p );
    free( p );
}

// #define the following to use overloaded new(size, file, line, function)
//#define new new( __FILE__, __LINE__, __FUNCTION__ )

#endif // DEBUGNEW_H__
