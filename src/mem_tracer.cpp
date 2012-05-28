// MicroMath+ - (c) Ugo Varetto

/// @file mem_tracer.cpp implementation of MemTracer class used to for tracking memory usage

#include <iostream>

#include "mem_tracer.h"

bool MemTracer::Ready = false;

//----------------------------------------------------------------------------
void MemTracer::Add( void* p, char const * file, int line, const char* fun, std::size_t s )
{
    if( lockCount_ > 0 ) return;
    MemTracer::Lock lock( *this );
    map_[ p ] = Entry( file, line, fun, s );
    amem_ += s;
}

//----------------------------------------------------------------------------
void MemTracer::Remove ( void* p )
{
    if( lockCount_ > 0 ) return;

    MemTracer::Lock lock( *this );

    iterator it = map_.find (p);
    if( it != map_.end() )
    {
    	dmem_ += it->second.Size();
        map_.erase( it );
    }
    
}

//----------------------------------------------------------------------------
MemTracer::MemTracer ( std::ostream& os ) 
: lockCount_(0), os_( os ), amem_( 0 ), dmem_( 0 )
{
    Ready = true;
}

//----------------------------------------------------------------------------
MemTracer::~MemTracer ()
{
    Ready = false;
    Dump();
}

//----------------------------------------------------------------------------
void MemTracer::Dump ()
{
	typedef unsigned long ulong;// *issue with MinGW gcc 3.4.2
							    // << unsigned long( i ) causes an error
							    // *issue with MS VS .NET 2003 no overload
							    // for operator <<( ostream&, std::size_t ) ???  
	os_ << "\nAllocated Memory:   " << ulong( amem_ ) << " bytes";
	os_ << "\nDeallocated Memory: " << ulong( dmem_ ) << " bytes" << std::endl;
    if( map_.size () != 0 )
    {
        os_ << ulong( map_.size () ) << " memory leaks detected\n";
        for( iterator it = map_.begin (); it != map_.end (); ++it )
        {
            char const * file = it->second.File ();
            const int line = it->second.Line ();
            const char* fun = it->second.Fun();
            const std::size_t size = it->second.Size();
            
            os_ << "File: " << file << ','
                << " Line: " << line << ','
                << " Size: " << ulong( size );
            if( fun ) os_ <<  ", Function: " << fun;
            os_ << std::endl;
            
        }
        os_.flush(); 
    }    
}

