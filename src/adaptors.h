#ifndef ADAPTORS_H__
#define ADAPTORS_H__

// MicroMath+ - (c) Ugo Varetto

/// @file adaptors.h definition of helper functions to adapt standard functions to function< T >_i derived function objects.

#include "execution.h"  

#ifdef MMP_DEBUG_MEMORY
#include "dbgnew.h"
#define new new( __FILE__, __LINE__, __FUNCTION__ )
#endif


//==============================================================================

namespace mmath_plus {

  //============================================================================

  //----------------------------------------------------------------------------
  /// Utility class to construct a function object from a function of type:
  /// T fun(T).
  template < class T = double >
  struct unary_function {
    /// Accepted parameter value type
    typedef T val_type;
    /// Function pointer
    typedef val_type ( *fun_type )( val_type );
    /// Pointer to function of type <code> T fun( T ) </code>
    const  fun_type f;
    /// Constructor.
    /// @param f1 pointer to function of type <code> T fun( T ) </code>
    unary_function( fun_type f1 ) : f( f1 ) {}
    /// Puts value returned by function <code> f </code> on top of stack.
    /// @param rt reference to run-time environment
    void operator()( rte< T >& rt ) const
    {
      val_type& st = rt.stack.top();
      st = f( st );      
    }
    /// Returns value returned by function <code> f </code>
    /// @param v value type e.g. floating point number
    /// @returns value type
    val_type operator()( val_type v ) const { return f( v ); }
  };

  //----------------------------------------------------------------------------
  /// Utility class to construct a function object from a function of type:
  /// T fun(T, T).
  template < class T = double, class RteT = rte< T > >
  struct binary_function {
    /// Type of accepted values
    typedef T val_type;
    /// Function pointer
    typedef val_type ( *fun_type )( val_type, val_type );
    /// Pointer to function of type <code> T fun( T, T ) </code>
    const  fun_type f;
    /// Constructor.
    /// @param f1 pointer to function of type <code> T fun( T, T ) </code>
    binary_function( fun_type f1 ) : f( f1 ) {}
    /// Puts value returned by function <code> f </code> on top of stack.
    /// @param rt reference to run-time environment
    void operator()( RteT& rt ) const
    {
      const val_type op2  = rt.stack.top();
      rt.stack.pop();
      val_type& op1 = rt.stack.top();
      op1 = f( op1, op2 );      
    }
    /// Returns value returned by function <code> f </code>
    /// @param v1 value type e.g. floating point number
    /// @param v2 value type e.g. floating point number
    /// @returns value type
    val_type operator()( val_type v1, val_type v2 ) const
    { 
      return f( v1, v2 );
    }

  };

  //============================================================================

} // namespace mmath_plus

//==============================================================================
#ifdef MMP_DEBUG_MEMORY
#undef new
#endif

#endif // ADAPTORS_H__
