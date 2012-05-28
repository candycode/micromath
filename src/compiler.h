#ifndef COMPILER_H__
#define COMPILER_H__

// MicroMath+ - (c) Ugo Varetto

/// @file compiler.h definition of compiler class

#include <sstream>
#include "execution.h"
#include "math_parser.h"
#include "exception.h"

#include "shared_ptr.h"

#ifdef MMP_DEBUG_MEMORY
#include "dbgnew.h"
#define new new( __FILE__, __LINE__, __FUNCTION__ )
#endif

namespace mmath_plus {

  
  //===========================================================================

  static const bool CREATE_VARS      = true;

  static const bool DONT_CREATE_VARS = !CREATE_VARS;

  //---------------------------------------------------------------------------
  /// Creates an instruction array(program) given a list of tokens.
  template < class T > class compiler {
  public:
    
    /// Utility constant.
    static const bool CREATE_VARS = true;
    
    /// Utility constant.
    static const bool COUNT_ARGS = true;

    /// Utility constant.
    static const bool DONT_CREATE_VARS = !CREATE_VARS;
    
    /// Utility constant.
    static const bool DONT_COUNT_ARGS = !COUNT_ARGS;

    /// Class name.
    static const std::string CLS_NAME;

    //--------------------------------------------------------------------------
    /// Base class for exceptions.
    class exception : public exception_base {
    public:
      /// Constructor.
      /// @param fun function throwing exception
      /// @param lineno line number at which exception is thrown
      /// @param data message
      exception( const std::string& fun,
                 unsigned long lineno,
                 const std::string& data = "" )
        : exception_base( NS_NAME, compiler::CLS_NAME, fun, lineno, data )
      {}
    };

    //--------------------------------------------------------------------------
    /// Thrown when null token found.
    class null_token : public exception {
    public:
      /// Constructor.
      /// @param fun function throwing exception
      /// @param lineno line number at which exception is thrown
      /// @param data message 
      null_token( const std::string& fun,
                  unsigned long lineno,
                  const std::string& data = "" )
        : exception( fun, lineno, data )
      {}
    };

    //--------------------------------------------------------------------------
    /// Thrown when unknown token found.
    class unknown_token : public exception {
    public:
      /// Constructor.
      /// @param fun function throwing exception
      /// @param lineno line number at which exception is thrown
      /// @param data message
      unknown_token( const std::string& fun,
                     unsigned long lineno,
                     const std::string& data = "" )
      : exception( fun, lineno, data )
      {}
    };
    
    //--------------------------------------------------------------------------
    /// Constructor.
    /// @param count_args if true selects function given name and number
    /// of arguments; if false only function name is used
    /// @param create_vars if true creates a new variable and initializes it
    /// to zero when a new name is found 
    compiler( bool count_args = false, bool create_vars = false )
      : create_variables_( create_vars ), count_args_( count_args )
    {}
    
    //--------------------------------------------------------------------------
    /// Returns instruction array given token list and run-time environment.
    /// @param tokens const reference to token pointers
    /// @param rt const reference to run-time environment
    /// @return compiled instruction array
    /// @todo use iterator instead of vector
    typename rte< T >::prog_type
    compile( const std::vector<  math_parser::TokenPtr >& tokens, rte< T >& rt )
    {
      typename rte< T >::prog_type program;
      for( std::vector< math_parser::TokenPtr >::const_iterator i = tokens.begin();
            i != tokens.end();
            ++i )
      {
        typename rte< T >::prog_type::value_type inst( compile( *i, rt ) );
        program.push_back( inst );
      }
      return program;
    }

	/// Returns value of <code>create_variables</code> variable.
	/// If <code>create_variables</code> is true then a new variable
	/// is crated in case the compilers finds a name not found in 
	/// the variable array.
	bool create_variables()	const { return create_variables_; }
	/// Sets value of <code>create_variables</code> variable.
	/// If <code>create_variables</code> is true then a new variable
	/// is crated in case the compilers finds a name not found in 
	/// the variable array.
	/// @param cv create varables
	void create_variables( bool cv ) { create_variables_ = cv; }
	/// Returns value of <code>count_args</code> variable.
	/// If <code>count_args</code> is true then the compiler looks
	/// up a function using both name and number of parameters; if
	/// false it looks up a function using the name only.
	/// It may be useful to set this paramater to false in e.g.
	/// the following cases:
	//// - function accepting a variable number of parameters:
	///    f(4, 1.1, 2.3, 1.0, 3.45) first parameter contains the
	///    number of passed parameters not including the first parameter
	///  - function is called with a multidimensional parameter:
	///    f((1,2,3)) is parsed as 1 2 3 f[1] NOT 1 2 3 f[3]
	bool count_args() const { return count_args_; }
	/// Sets the value of <code>count_args</code> variable.
	/// If <code>count_args</code> is true then the compiler looks
	/// up a function using both name and number of parameters; if
	/// false it looks up a function using the name only.
	/// It may be useful to set this paramater to false in e.g.
	/// the following cases:
	///  - function accepting a variable number of parameters:
	///    f(4, 1.1, 2.3, 1.0, 3.45) first parameter contains the
	///    number of passed parameters not including the first parameter
	///  - function is called with a multidimensional parameter:
	///    f((1,2,3)) is parsed as 1 2 3 f[1] NOT 1 2 3 f[3]
	void count_args( bool ca ) { count_args_ = ca; }

	
  private:

    /// If true it creates a new variable when a name which does not match
    /// any function, variable or constant is found.
    bool create_variables_;

    /// Take into account number of arguments when compiling a function ?
    bool count_args_;

    //--------------------------------------------------------------------------
    /// Return instruction given token and run-time environment.
    /// @param t pointer to token
    /// @param rt const reference to run-time environment
    instruction< T >* compile( math_parser::TokenPtr t, rte< T >& rt ) 
    {
      if( !t )
      {
        throw null_token( "compile", __LINE__, "" );
        return 0;
      }
              
      switch( t->type )
      {
      case math_parser::UNKNOWN:
        {
          throw unknown_token( "compile", __LINE__, t->str );
          break;
        }          
      case math_parser::VALUE:
        {
          const math_parser::value_token* v_p =
            static_cast< const math_parser::value_token* >( ptr( t ) );
          return new load_val< T >( std::atof( v_p->str.c_str() ) );
          break;
        }
      case math_parser::FUNCTION:
        {

			typedef shared_ptr< const function_i< T > > FPtr;
			const math_parser::function_token* ft =
             static_cast< const math_parser::function_token* >( ptr( t ) ); 
          	const FPtr f( rt.function_p( ft->str,
										count_args_ ? ft->args : -1 ) );
            if( f ) return new call_fun< T >( f );
            break;
        }
      case math_parser::OPERATOR:
        {
          const math_parser::operator_token* o_p =
            static_cast< const math_parser::operator_token* >( ptr( t ) );
          typedef shared_ptr< const function_i< T > > FPtr;
          const FPtr f( rt.function_p( o_p->str,
                                        o_p->rargs,
                                        o_p->largs ) );
          if( f ) return new call_fun< T >( f );
          break;
        }
      case math_parser::NAME:
        {
        	  typedef shared_ptr< const function_i< T > > FPtr;
          if( !count_args_ )
          { 
             // check if name is a function
             const FPtr f( rt.function_p( t->str ) );
             if( f ) return new call_fun< T >( f );
          }
          // check if name is a variable
          typedef shared_ptr< value< T > > VPtr;
          VPtr v( rt.variable_p( t->str ) );
          if( v ) return new load_var< T >( v );
          // check if name is a constant
          typedef shared_ptr< const value< T > > CPtr;
          const CPtr c( rt.constant_p( t->str ) );
          if( c ) return new load_val< T >( c->val );
          // name is not a name nor a constant, if  requested create new variable.
          if( create_variables_ )
          {
          	typedef typename rte< T >::ValPtrT ptype;
          	ptype new_var( new value< T >( t->str, T() ) );
            rt.var_tab.push_back( new_var );
            return new load_var< T >( new_var );
          }
          break;
        }
      default:
          break;
      }
      // no known token found
      throw unknown_token( "compile", __LINE__, t->str );
      return 0;
    }
  };

  /// Definition of class name variable.
  template < class T >
  const std::string compiler< T >::CLS_NAME( "compiler" );

  //===========================================================================

} // namespace mmath_plus

#ifdef MMP_DEBUG_MEMORY
#undef new
#endif

#endif // COMPILER_H__  
