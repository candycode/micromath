#ifndef MATH_PARSER_H__
#define MATH_PARSER_H__

// MicroMath+ - (c) Ugo Varetto

/// @file math_parser.h declaration of math_parser class and related types

#include <vector>
#include <string>
#include <sstream>
#include <utility>
#include <algorithm>
#include <iostream>
#include <list>

#include "mmp_algorithm.h"

#include "text_utility.h"
#include "exception.h"

#include "shared_ptr.h"

#ifdef MMP_DEBUG_MEMORY
#include "dbgnew.h"
#define new new( __FILE__, __LINE__, __FUNCTION__ )
#endif

//==============================================================================

namespace mmath_plus {

  /// Namespace name.
  static const std::string NS_NAME( "mmath_plus" );

  //============================================================================

  //----------------------------------------------------------------------------
  /// Holds operator-related information.
  /// Can be changed to specify left and right number of
  /// operands independently to support expressions like
  /// (1, 2, 3) * 1.
  /// A field can be added to add information about the number of returned values
  /// to support operators returning a number of values different from the size
  /// of the operands e.g. scalar product.
  class operator_type {
  private:  
	/// Operator name e.g. +.
    std::string name_;

	/// Number of operands.
    int operands_;

	/// Dimension of left operand.
	int largs_;
	
	/// Dimension of right operand.
	int rargs_;
	
	/// Dimension of returned value.
	int outvals_;
	
    /// Swap left and right operands ?.
    bool swap_;

  public:
    /// Constructor.
    /// @param n name
    /// @param o number of operands
    /// @param l number of operands on the left side
    /// @param r number of operands on the right side
    /// @param out number of returned values
    /// @param s swap operands ?
    operator_type( const std::string& n, int o, int l = 1, int r = 1,
				   int out = 1, bool s = false )
      : name_( n ), operands_( o ), largs_( l ), rargs_( r ),
	    outvals_( out ), swap_( s )
    {}

	/// Operator name e.g. +.
    const std::string& name() const { return name_; }

	/// Number of operands.
    int operands() const { return operands_; }

   	/// Dimension (number of components) of left operand.
	int largs() const { return largs_; }
	
	/// Dimension (number of components) of right operand.
	int rargs() const { return rargs_; }
	
	/// Dimension (number of components) of returned value.
	int outvals() const { return outvals_; }
	
    /// Swap left and right operands ?.
    bool swap() const { return swap_; }

   };

  //============================================================================

  //----------------------------------------------------------------------------
  /// Math parser: extracts tokens associated to a mathematical expression.
  /// The flow of operations is:
  ///   - error checking
  ///   - conversion of operators into functions (optional)
  ///   - conversion to rpn
  ///   - extraction of tokens.
  /// The parser can optionally compute the number of arguments for operators
  /// and functions and make the information available in the output std::string.
  /// IN: x + 1.E-3 - atan2( y, z )
  /// OUT:
  ///   x 1.E-3 +[1 1] y z atan2[2] -[1 1]
  /// OR
  ///   x 1.E-3 + y z atan2 -.
  class math_parser {

  // Public interface. //
  public:

    /// Class name.
    static const std::string CLS_NAME;

    //--------------------------------------------------------------------------
    /// Base class for math_parser exceptions.
    class exception : public exception_base {
    public:
      /// Constructor.
      /// @param fun name of function throwing exception
      /// @param lineno line number at which exception is thrown
      /// @param data message
      exception( const std::string& fun,
                 unsigned long lineno,
                 const std::string& data = "" )
        : exception_base( NS_NAME, math_parser::CLS_NAME, fun, lineno, data )
      {}
    };

    //--------------------------------------------------------------------------
    /// Thrown when unmatched opening parenthesis found.
    class unmatched_opening_par : public exception {
    public:
      /// Constructor.
      /// @param fun name of function throwing exception
      /// @param lineno line number at which exception is thrown
      /// @param data message
      unmatched_opening_par( const std::string& fun,
                             unsigned long lineno,
                             const std::string& data = "" )
        : exception( fun, lineno, data )
      {}
    };

    //--------------------------------------------------------------------------
    /// Thrown when unmatched closing parenthesis found.
    class unmatched_closing_par : public exception {
    public:
      /// Constructor.
      /// @param fun name of function throwing exception
      /// @param lineno line number at which exception is thrown
      /// @param data message
      unmatched_closing_par( const std::string& fun,
                             unsigned long lineno,
                             const std::string& data = "" )
        : exception( fun, lineno, data )
      {}
    };

    //--------------------------------------------------------------------------
    /// Thrown when unknown symbol found. E.g. '$'
    class unknown_symbol : public exception {
    public:
      /// Constructor.
      /// @param fun name of function throwing exception
      /// @param lineno line number at which exception is thrown
      /// @param data message
      unknown_symbol( const std::string& fun,
                      unsigned long lineno,
                      const std::string& data = "" )
        : exception( fun, lineno, data )
      {}
    };

    //--------------------------------------------------------------------------
    /// Thrown when invalid name found. E.g. '2.x'
    class invalid_name : public exception {
    public:
      /// Constructor.
      /// @param fun name of function throwing exception
      /// @param lineno line number at which exception is thrown
      /// @param data message
      invalid_name( const std::string& fun,
                    unsigned long lineno,
                    const std::string& data = "" )
                  : exception( fun, lineno, data )
      {}
    };

    /// Utility constant.
    static const bool DEBUG             = true;
    /// Utility constant.
    static const bool REPLACE_OPERATORS = true;
    /// Utility constant.
    static const bool SWAP_ARGS         = true;
    /// Utility constant.
    static const bool COUNT_ARGS        = true;

    /// Utility constant.
    static const bool DONT_DEBUG             = !DEBUG;
    /// Utility constant.
    static const bool DONT_SWAP_ARGS         = !SWAP_ARGS;
    /// Utility constant.
    static const bool DONT_COUNT_ARGS        = !COUNT_ARGS;

    /// Token type.
    enum token_type { UNKNOWN, VALUE, NAME, FUNCTION, OPERATOR };

    //--------------------------------------------------------------------------
    /// Base class for tokens.
    struct token {
      /// Token type.
      const token_type type;
      /// Token std::string.
      const std::string str;
      /// Constructor.
      /// @param s token std::string
      /// @param t token type
      token( const std::string& s, token_type t = UNKNOWN ) : type( t ), str( s )
      {}
    };

    //--------------------------------------------------------------------------
    /// Value e.g. 2.
    struct value_token : token {
      /// Constructor.
      /// @param s token std::string
      value_token( const std::string& s ) : token( s, VALUE ) {}
    };

    //--------------------------------------------------------------------------
    /// Name e.g. x.
    struct name_token : token {
      /// Constructor.
      /// @param s token std::string
      name_token( const std::string& s ) : token( s, NAME ) {}
    };

    //--------------------------------------------------------------------------
    /// Function e.g. sin[1].
    struct function_token : token {
      /// Number of arguments.
      const int args;
	  /// Number of returned values.
	  const int outvalues;
      /// Constructor.
      /// @param s std::string
      /// @param a number of arguments
	  /// @param o number of returned values
      function_token( const std::string& s, int a, int o ) :
													token( s, FUNCTION ),
													args( a ),
													outvalues( o ) {}
    };

    //--------------------------------------------------------------------------
    /// Operator e.g. +[1 1].
    struct operator_token : token {
      /// Number of left arguments.
      const int largs;
      /// Number of right arguments.
      const int rargs;
	  /// Numbet of generated values.
	  const int outvalues;
      /// Constructor.
      /// @param s token std::string
      /// @param l number of left arguments
      /// @param r number of right arguments
	  /// @param o numer of generated values
      operator_token( const std::string& s, int l, int r, int o )
                : token( s, OPERATOR ), largs( l ), rargs( r ), outvalues( o )
				 {}
    };

    /// Constructor.
    /// @param operators const reference to operator table
    /// @param swap_args swap function arguments ?
    /// @param count_args count arguments  and operands ?
    /// @param debug if debug is true then log messages are printed to the given
    ///        stream
    /// @param out_p pointer to stream used for logging
    math_parser( const std::vector< operator_type >& operators,
                 bool swap_args = false,
                 bool count_args = true,
                 bool debug = false, std::ostream* out_p = &std::clog )
                 : os_p_( out_p ), operators_( operators ),
                   debug_( debug ), swap_args_( swap_args ),
                   count_args_( count_args )

    {}

    /// Alias for pointer to token type
	typedef shared_ptr< token > TokenPtr;
    /// Alias for token sequence type
	typedef std::vector< TokenPtr > Tokens;
	
    /// Parsing function.
    /// @param expr const reference to expression to parse
    /// @return instruction array
    Tokens parse( const std::string& expr );
	
    /// Get value of debug_ flag.
    bool debug() const { return debug_; }

    /// Set value of debug_ flag.
    void debug( bool dbg ) { debug_ = dbg;  }

    /// Get value of swap_args_ flag.
    bool rpn_swap() const { return swap_args_; }

    /// Set value of swap_args_ flag.
    void rpn_swap( bool rpn_swap ) { swap_args_ = rpn_swap;  }

    /// Get value of count_args_ flag.
    bool count_args() const { return count_args_; }

    /// Set value of count_args_ flag.
    void count_args( bool count_args ) { count_args_ = count_args;  }

    /// Get reference to expression.
    const std::string& expr() const { return expr_; }

  private:

    /// Debug stream.
    std::ostream* os_p_;

    /// Expression.
    std::string expr_;

    /// Temporary buffer.
    std::string tmp_expr_;

    /// Mapping between operators and functions e.g. + --> add().
    const std::vector< operator_type >& operators_;

    /// Tokens.
    std::vector< TokenPtr > tokens_;

    /// Debug enabled when debug == true.
    bool   debug_;

    /// If swap_args_ == true arguments of each operator are swapped and
    /// function arguments are reverse ordered.
    /// a + b/c --> a b c / + if swap_args_ == false; c b / a + otherwise.
    /// f( x, y ) --> x y f if swap_args_ == false; y x f otherwise.
    bool   swap_args_;

    /// Add information about how many arguments an operator or funtion has.
    /// e.g. (1, 2, 3) + (4, 5, 6) - f(x,y) --> 1 2 3 4 5 6 +[3 3] x y f[2] -
    bool count_args_;

    /// Opening parenthesis.
    static const std::string::value_type OPENPAR;// = '(';

    /// Closing parenthesis.
    static const std::string::value_type CLOSEPAR;// = ')';

    /// Argument separator.
    static const std::string::value_type ARGS_SEPARATOR;// = ',';

    /// Whitespace.
    static const std::string::value_type BLANK;// = ' ';

	  /// Separator in RPN expression.
    static const std::string::value_type RPN_SEPARATOR;// = ' ';

    /// Opening parenthesis.
    static const std::string::value_type OPEN_ARG_PAR;// = '[';

    /// Closing parenthesis.
    static const std::string::value_type CLOSE_ARG_PAR;// = ']';

  // Internal functions. //
  private:

    /// Remove blanks from expression.
    void remove_blanks();

    /// Validate expression.
    bool validate();

    /// (a)+(b) --> ((a)(b)+).
    void postfix_operators();

    /// (sin(x)) --> ((x)sin).
    void postfix_functions();

    /// Convert to RPN.
    void to_rpn();

    //--------------------------------------------------------------------------
    /// Creates token array by splitting the RPN expression into an array of
    /// strings.
    void create_tokens()
    {
      std::istringstream is( expr_.data() );
      std::string s;
      while( is )
      {
        is >> s;
        if( s.empty() ) break;
        if( count_args_
            && s.find( OPEN_ARG_PAR ) != std::string::npos
            && s[ s.size() - 1 ] != CLOSE_ARG_PAR )
        {
		 
		  std::vector< std::string > sv;
		  std::string s2 = " ";
		  while( is && s2[ s2.size() - 1 ] != CLOSE_ARG_PAR ) 
		  {
			is >> s2;
			if( is.good() ) sv.push_back( s2 );			
		  }
		  for( size_t i = 0; i < sv.size(); ++i )
		  {
			s += BLANK;
			s += sv[ i ];
		  } 	
        }
        TokenPtr t( create_token( s ) );
        if( t != 0 ) tokens_.push_back( t );
        if( debug_ )
        {
          *os_p_ << s << "\t\t";
          switch( t->type )
          {
          case UNKNOWN:  *os_p_ << "UNKNOWN";
                         break;
          case VALUE:    *os_p_ << "VALUE";
                         break;
          case NAME:     *os_p_ << "NAME";
                         break;
          case FUNCTION: *os_p_ << "FUNCTION";
                         break;
          case OPERATOR: *os_p_ << "OPERATOR";
                         break;
          default:       break;
        }
          *os_p_ << '\n';
        }
        s.clear();
      }
    }

    //--------------------------------------------------------------------------
    /// Creates token from std::string.
    /// @param s input std::string
    /// @return token pointer
    token* create_token( const std::string& s )
    {

      if( count_args_ )
      {
		const std::string::size_type i = s.find( OPEN_ARG_PAR );
		if(  i != std::string::npos )
		{
			const std::string name = s.substr( 0, i );
			std::string::size_type c = s.find( CLOSE_ARG_PAR, i );
			if( c == std::string::npos ) return new token( s );
			
			std::istringstream is( std::string( s.begin() + i + 1, s.begin() + c ) );
			std::vector< int > values; values.reserve( 3 );
			int v;
			while( is ) 
			{
				is >> v;
				if( is ) values.push_back( v ); 
			}
			
			switch( values.size() )
			{
				case 1:
					{
						return new function_token( name, values[ 0 ], -1 );
					}
					break;
				case 2:
					{
						return new function_token( 
											name, values[ 0 ], values[ 1 ] );
					}
					break;
				case 3:
					{
						return new operator_token( name, values[ 0 ],
											   values[ 1 ], values[ 2 ] );
					}
					break;
				default:
					return 0; 				
			}
			
		}

	  }
      else
      {
        std::vector< operator_type >::const_iterator i;
        for( i = operators_.begin(); i != operators_.end(); ++i )
        {
          if( i->name() == s ) return new operator_token( s, -1 , -1, -1 );
        }
      }

      range_type r = search_number( s.begin(), s.end(), s.begin() );
      if( r.first != s.end() ) return new value_token( s );

      r = search_name( s.begin(), s.end() );
      if( r.first != s.end() ) return new name_token( s );

      if( !count_args_ )
      {
        for( std::vector< operator_type >::const_iterator i =operators_.begin();
             i != operators_.end(); ++i )
        {
          if( i->name() == s ) return new operator_token( s, -1, -1, -1 );
        }
      }

      return new token( s );

    }

    /// Wraps every number, identifier and function with parentheses.
    void wrap();

	/// Is RPN separator ?.
    /// @param v1 character
    /// @param v2 character
	bool is_rpn_separator( std::string::value_type v1, std::string::value_type v2 );

    //--------------------------------------------------------------------------
    /// Wraps characters between r.first and r.second with parentheses.
    /// @param r identifying part of expression to wrap
    /// @return iterator pointing to first element after closing parenthesis
    std::string::const_iterator add_parentheses( const range_type& r )
    {
      // range starts after end of sequence return ( end, end )
      if( r.first == expr_.end()  || r.second == expr_.end() )
      {
        return expr_.end();
      }

      // return expr_.end() ? this happens when parenthesis is added at last
      // position in sequence
      bool ret_end = false;
      // if closing parenthesis to be added at last position then
      // return expr_.end() after adding parentheses
      if( r.second == expr_.end() - 1 ) ret_end = true;
      // "("
      std::string op_expr( 1, OPENPAR );
      // "(x"
      op_expr.append( r.first, r.second + 1 );
      // "(x)"
      op_expr += CLOSEPAR;
      // compute position of first character after replaced expression
      const std::string::difference_type offset =
                                       r.first - expr_.begin() + op_expr.size();
      // replace expression with wrapped version:  "x" -->"(x)"
      expr_ =expr_.replace(
                  r.first - expr_.begin(), r.second - r.first + 1, op_expr );
      // right parenthesis == last character in std::string
      if( ret_end ) return expr_.end();
      // return position of character after closing parenthesis
      return ( expr_.begin() + offset );
    }

    //--------------------------------------------------------------------------
    /// Returns iterators pointing to first and last character of function
    /// including opening and closing parentheses.
    /// cos(x) --> first iterator points to 'c' second iterator points to ')'.
    /// A function can have the same format as variables but must have an
    /// opening parenthesis immediately after the function name; the opening
    /// parenthesis must have a matching closing parenthesis.
    /// @param b start position
    /// @param e one past end position
    /// @return iterators pointing to first and last character of function
    range_type search_function( const std::string::const_iterator& b,
                                const std::string::const_iterator& e ) const
    {
      // search for first and last character of sequence matching a name i.e.
      range_type r = search_range( b, e, match_name() );
      // if not found return range_type( e, e ) as returned by function
      // search_range
      if( r.first == e ) return r;
      // range found and second character == opening parenthesis
      if( r.second != e && *r.second == OPENPAR )
      {
      	
    		std::string fname( r.first, r.second ); 
  		if( mmath_plus::find_if( operators_.begin(), operators_.end(), findop( fname ) ) != operators_.end() )
  		{
  			return search_function( r.second, e );	
  		}
        // get position of matching closing parenthesis
        r.second = forward_parenthesis_match( r.second, e,  OPENPAR, CLOSEPAR );
        return r;
      }
      // a name was found but no opening parenthesis after it so continue
      // searching
	  return search_function( r.second, e );
    }

	
    //--------------------------------------------------------------------------
    /// Returns iterators pointing to first and last character of variable name.
    /// var_name --> first iterator points to 'v' second iterator points to 'e'.
    /// A variable name can start with '_' or a letter and can have any
    /// number of letters, digits or underbars.
    /// @param b start position
    /// @param e one past end position
    /// @return iterators pointing to first and last character of variable
    range_type search_name( const std::string::const_iterator& b,
                            const std::string::const_iterator& e ) const
    {
      // search for first and last character of name
      range_type r = search_range( b, e, match_name() );
      // it nothing found return ( e, e )
      if( r.first == e ) return r;

      // if name found and there is no opening parenthesis after it
      if( r.second == e || *r.second != OPENPAR )
      {
      	std::string fname( r.first, r.second ); 
  		if( mmath_plus::find_if( operators_.begin(), operators_.end(), findop( fname ) ) != operators_.end() )
  		{
  			return search_name( r.second, e );	
  		}
        // return range
        return range_type( r.first, --r.second );
      }

      // a name was found but with a parenthesis after it
	    return search_name( r.second, e );
    }

    //--------------------------------------------------------------------------
    /// Returns iterators pointing to first and last character of number.
    /// 1.3E-08 --> first iterator points to '1' second iterator points to '8'.
    /// A number must:
    ///     - start with a digit or decimal point
    ///     - end with a digit
    ///     - have one and only one 'E'
    ///     - have one and only one decimal point before 'E' but not
    ///       adjacent to 'E'
    ///     - have at least one digit after 'E'
    ///     - have at most one '+' or '-' character just after 'E'
    /// @param b start position
    /// @param e one past end position
    /// @param start position of first element of expression
    /// @return iterators pointing to first and last character of number
    range_type search_number( const std::string::const_iterator& b,
                              const std::string::const_iterator& e,
                              const std::string::const_iterator& start ) const
    {
      if( b == e ) return range_type( e, e );
      range_type r = search_range( b, e, match_number() );
      // nothing found
      if( r.first == e ) return r;
      // check if name after number
      range_type rn = search_name( r.second, e );
      if(  rn.first == r.second && rn.first != e )
      {
        // e.g. 2x
        throw invalid_name( "search_number", __LINE__,
                            std::string( r.first, ++rn.second ) );
        return range_type( e, e );
      }
      // first character in start position
      if( r.first == start ) return range_type( r.first, --r.second );

      // match_number can validate numbers like 1.2E or 1.2E+
      // check if name before number e.g. x2
      range_type nr = search_range( r.first - 1, r.second, match_name() );
      // if number is within name perform new search
      if( nr.first != r.second ) return search_number( r.second, e, start );

      // get last character validated by match_number
      const std::string::value_type last = *( r.second - 1 );
      // if last character validated by match_number is not a digit
      // i.e. it is 'E', '+' or '-'
      if( ( r.first != r.second ) && !isdigit( last ) )
      {
        // if number of digits > 1 then if last validated character is not a
        // number continue searching; if only one character has bee validated
        // by match_number then it has to be a single digit number
        return search_range( r.second, e , match_number() );
      }
      // if last character is a digit or a decimal point
      if( isdigit(last) || last == match_number::DOT )
      {
        // part of a name ?
        // number could be part of a variable name e.g. x2x
        // if first character non at beginning of sequence
        if( r.first != start )
        {
          // search forward for name starting one character before starting
          // of number: x2x --> start from x
          range_type v = search_name( r.first - 1, e );
          // if there is a name starting at first - 1 then the matched
          // number is part of a variable: x2x --> found '2' which is part
          // of variable 'x2x'
          if( v.first < r.second )
          {
            return search_number( r.second, e, start );
          }
        }
        // number is not part of a name
        return range_type( r.first, --r.second );
      }
      return search_number( r.second, e, start );
    }

    /// Function object used to find operators
	struct findop {
                /// Operator name
	    		const std::string& name;
	    		/// Constructor
                /// @param s operator name
                findop( const std::string& s ) : name( s ) {}
	    		/// @param o reference to operator instance
                /// @return <code>true</code> if operator name matches name of passed operator instance
                bool operator()( const operator_type& o ) { return o.name() == name; }    
    };

public:

    /// Reverses std::list of function arguments.
    /// @param expr_ expression
    /// @param pr range containing std::list of parameters to reverse
    void swap_function_args( std::string& expr_, range_type pr )
    {
      if( *pr.first == OPENPAR && *pr.second == CLOSEPAR )
      {
        ++pr.first; --pr.second;
        swap_function_args( expr_, pr );
        return;
      }
      // find argument range list
      std::list< range_type > ri;
      std::for_each( pr.first,
                pr.second + 1,
                extract_argument< std::list< range_type> >(
                                        ri, range_type( pr.first, pr.second ),
                                        OPENPAR, CLOSEPAR, ARGS_SEPARATOR ) );

      // if argument range std::list size == 1 return
      if( ri.size() < 2 ) return;

      // call function on every argument range
      for( std::list< range_type >::iterator i = ri.begin();
           i != ri.end();
           ++i )
      {
        swap_function_args( expr_, *i );
      }

      tmp_expr_.clear();
      tmp_expr_ = "";
      // create std::string with arguments in reverse order
      for( std::list< range_type >::reverse_iterator j = ri.rbegin();
           j != ri.rend();
           ++j )
      {
        if( j != ri.rbegin() ) tmp_expr_ += ARGS_SEPARATOR;
        tmp_expr_ += expr_.substr( j->first - expr_.begin(),
                                   j->second - j->first + 1 );
      }

      expr_.replace( pr.first - expr_.begin(),
                     pr.second - pr.first + 1, tmp_expr_ );

      if( debug_ ) *os_p_ << expr_ << '\n';
    }
  };


  //----------------------------------------------------------------------------

  /// Function object used to free memory
  template < class Tptr >
  struct free {
    /// Frees memory
    /// @param p pointer to object
    /// @return NULL 
    Tptr operator()( Tptr p )
      { delete p; return 0; }
  };

  /// Utility function to delete each pointer stored inside a collection.
  template < class Collection >
  void free_ptr_collection( Collection& c )
  {
    typedef typename Collection::value_type val_t;
    transform( c.begin(), c.end(), c.begin(), free< val_t >() );
    c.clear();
  }

  //----------------------------------------------------------------------------


  //============================================================================

} //namespase mmath_plus

//==============================================================================
#ifdef MMP_DEBUG_MEMORY
#undef new
#endif

#endif // MATH_PARSER_H__
