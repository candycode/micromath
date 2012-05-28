#ifndef MMP_ALGORITHM_H__
#define MMP_ALGORITHM_H__

// MicroMath+ - Ugo Varetto

/// @file mmp_algorithm.h re-implemented STL algorithms to guarantee sequential in-order traversal of sequences, not guaranteed by ISO C++ 14882 standard  

#include <algorithm>

#ifdef MMP_DEBUG_MEMORY
#include "dbgnew.h"
#define new new( __FILE__, __LINE__, __FUNCTION__ )
#endif

namespace mmath_plus {

	//------------------------------------------------------------------------------
	/// find_if implementation
    template<class InputIterator, class Predicate>
	inline InputIterator find_if( InputIterator first,
								  InputIterator last,
								  Predicate predicate)
	{	
		// find first satisfying predicate
		for ( ; first != last; ++first)
		{
			if( predicate( *first ) ) break;
		}
		return first;
	}

	//------------------------------------------------------------------------------
	/// count_if implementation
    template< class InputIterator, class Predicate >
	inline typename std::iterator_traits< InputIterator >::difference_type
	count_if( InputIterator first, InputIterator last, Predicate predicate )
	{
		// count elements satisfying predicate
		typename std::iterator_traits< InputIterator >::difference_type cnt = 0;

		for ( ; first != last; ++first)
		{
			if ( predicate( *first ) ) ++cnt;
		}
		return cnt;
	}

	//------------------------------------------------------------------------------
	/// replace_if implementation
    template< class ForwardIterator,
			  class Predicate,
			  class T >
	inline void replace_if( ForwardIterator first,
							ForwardIterator last,
							Predicate predicate,
							const T& val )
	{
		// replace each value satisfying predicate with new value
		for( ; first != last; ++first )
		{
			if( predicate( *first ) ) *first = val;
		}
	}
} // namespace mmath_plus

#ifdef MMP_DEBUG_MEMORY
#undef new
#endif

#endif // MMP_ALGORITHM_H__
