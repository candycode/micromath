// MicroMath+ - (c) Ugo Varetto

/// @file math_parser.cpp implementation of math_parser class


#include <sstream>

#include "math_parser.h"

#ifdef MMP_DEBUG_MEMORY
#include "dbgnew.h"
#define new new( __FILE__, __LINE__, __FUNCTION__ )
#endif

//==============================================================================

namespace mmath_plus {

  namespace mm = mmath_plus;
  using std::sort;
  using std::equal;
  using std::unique;
  using std::back_insert_iterator;
  using std::back_inserter;
  using std::remove_copy;
  using std::string;
  using std::vector;
  using std::not_equal_to;
  using std::pair;
  
  //============================================================================

  /// Definition of class name.
  const string math_parser::CLS_NAME( "math_parser" );

  //----------------------------------------------------------------------------

    /// Opening parenthesis.
	const std::string::value_type math_parser::OPENPAR = '(';

	/// Closing parenthesis.
	const std::string::value_type math_parser::CLOSEPAR = ')';

	/// Argument separator.
	const std::string::value_type math_parser::ARGS_SEPARATOR = ',';

	/// Whitespace.
	const std::string::value_type math_parser::BLANK = ' ';

		/// Separator in RPN expression.
	const std::string::value_type math_parser::RPN_SEPARATOR = ' ';

	/// Opening parenthesis.
	const std::string::value_type math_parser::OPEN_ARG_PAR = '[';

	/// Closing parenthesis.
	const std::string::value_type math_parser::CLOSE_ARG_PAR = ']';

  //----------------------------------------------------------------------------
  /// Number to string conversion
  template < class T > string ltoa( T n, string::size_type s = 4 )
  {
	std::stringstream ss;
	string ls( s, '0' );
	ss << n;
	ss >> ls;
	return ls;
  }
    
  //----------------------------------------------------------------------------

  //============================================================================

  //----------------------------------------------------------------------------
   
  vector< math_parser::TokenPtr > math_parser::parse( const string& expr )
  {
    tokens_.clear();
    
    expr_ = expr;
    
    if( !validate() ) return tokens_;

    wrap();
    
    remove_blanks();
       
    to_rpn();
    
    create_tokens();
    
    return tokens_;
  }

  //----------------------------------------------------------------------------
   
  void math_parser::remove_blanks()
  {
    if( debug_ ) *os_p_ << "remove_blanks {" << '\n' << " " << expr_ << '\n'; 
        
    tmp_expr_.resize( expr_.size() );
    string::iterator end =
      remove_copy( expr_.begin(), expr_.end(), tmp_expr_.begin(), BLANK );

    expr_.assign( tmp_expr_.begin(), end );
    

    if( debug_ ) *os_p_ << "} remove_blanks" << '\n' << " " << expr_ << '\n';
  }

  //----------------------------------------------------------------------------
   
  bool math_parser::validate()
  {
    // Check for unmatched parentheses.
    for( string::const_iterator i = expr_.begin(); i != expr_.end(); ++i )
    {
      if( *i == OPENPAR )
      {
        if( forward_parenthesis_match( i, expr_.end(), OPENPAR, CLOSEPAR )
            == expr_.end() )
        {
          throw unmatched_opening_par( "validate", __LINE__, 
                  string( string::const_iterator( expr_.begin() ), ++i ) );
          return false; // <-- in case exception handling is disabled
        }
        continue;
      }
      if( *i == CLOSEPAR )
      {
        if( backward_parenthesis_match( 
                  string::const_reverse_iterator( i ),
                  expr_.rend(),
                  OPENPAR, CLOSEPAR ) ==
                             string::const_reverse_iterator( expr_.rend() ) )
        {
          throw unmatched_closing_par( "validate", __LINE__, 
                     string( string::const_iterator( expr_.begin() ), ++i ) );
          return false; // <-- in case exception handling is disabled
          break;
        }
        continue;
      }
    }

    tmp_expr_ = expr_;
    
    // replace numbers with blanks
    range_type r = search_number( tmp_expr_.begin(), tmp_expr_.end(), tmp_expr_.begin() );
    while( r.first != tmp_expr_.end() )
    {
      tmp_expr_.replace( r.first - tmp_expr_.begin(),
                         r.second - r.first + 1, r.second - r.first + 1, BLANK );
       r = search_number( tmp_expr_.begin(), tmp_expr_.end(), tmp_expr_.begin() );
    }

    // replace operators with blanks
    for( vector< operator_type >::const_iterator it =operators_.begin();
         it != operators_.end();
         ++it )
    {
        string::size_type b;
        while( ( b = tmp_expr_.find( it->name() ) ) != string::npos )
        {
            tmp_expr_.replace( b, it->name().size(), it->name().size(), BLANK );
        }     
    }
    
    // replace function names with blanks
    r = search_function( tmp_expr_.begin(), tmp_expr_.end() );
    while( r.first != tmp_expr_.end() )
    {
      const string::size_type start = r.first - tmp_expr_.begin();
      const string::size_type p = tmp_expr_.find( OPENPAR, start );
      const string::size_type size = p - start;
      tmp_expr_.replace( start, size, size, BLANK );                         
       r = search_function( tmp_expr_.begin(), tmp_expr_.end() );
    }

    // replace variables and constants with blanks
    r = search_name( tmp_expr_.begin(), tmp_expr_.end() );
    while( r.first != tmp_expr_.end() )
    {
      tmp_expr_.replace( r.first - tmp_expr_.begin(),
                         r.second - r.first + 1, r.second - r.first + 1, BLANK );
       r = search_name( tmp_expr_.begin(), tmp_expr_.end() );
    }

       
    // replace parentheses and argument separator with blanks
    for( string::iterator si = tmp_expr_.begin(); si != tmp_expr_.end(); ++si )
    {
      if( *si == OPENPAR  || *si == CLOSEPAR || *si == ARGS_SEPARATOR )
      {
        *si = BLANK;
      }
    }

    // check if there is anything that is not a blank
    if( mm::find_if(
          tmp_expr_.begin(), tmp_expr_.end(), 
          bind2nd( not_equal_to< string::value_type >(), BLANK ) )
          != tmp_expr_.end() )
    {
      throw unknown_symbol( "validate", __LINE__, tmp_expr_ );
      return false; 
    }
    
    return true; 
  }

  //----------------------------------------------------------------------------
   
  void math_parser::wrap()
  {
    if( debug_ ) *os_p_ << "wrap {" << '\n' << " " << expr_ << '\n';
    
    vector< range_type > range_vec;

    // wrap numbers
    range_type r = search_number( expr_.begin(), expr_.end(), expr_.begin() );
    while( r.second != expr_.end() )
    {
      if( (r.first == expr_.begin()) || (r.second == expr_.end() - 1) )
	  {
        string::const_iterator it = add_parentheses( r );
		r = search_number( it, expr_.end(), expr_.begin() );
		continue;
	  }
      // (1.2)
      if( *( r.first - 1 ) == OPENPAR && *( r.second + 1 ) == CLOSEPAR )
      {
        r = search_number( r.second + 1, expr_.end(), expr_.begin() );
        continue;
      }

      // (1.2,
      if( *( r.first - 1 ) == OPENPAR && *( r.second + 1 ) == ARGS_SEPARATOR )
      {
        r = search_number( r.second + 1, expr_.end(), expr_.begin() );
        continue;
      }

      // ,1.2)
      if( *( r.first - 1 ) == ARGS_SEPARATOR && *( r.second + 1 ) == CLOSEPAR )
      {
        r = search_number( r.second + 1, expr_.end(), expr_.begin() );
        continue;
      }

      if( debug_ ) *os_p_ << "number " << *r.first << " " << *r.second << '\n';
      string::const_iterator it = add_parentheses( r );
      r = search_number( it, expr_.end(), expr_.begin() );
    }

    // wrap constants and variables
    r = search_name( expr_.begin(), expr_.end() );
    while( r.second != expr_.end() )
    {
    
	 if( r.first == expr_.begin() || r.second == expr_.end() - 1 )
	  {
        string::const_iterator it = add_parentheses( r );
		r = search_name( expr_.begin(), expr_.end() );
		continue;
	  }
      // (x)
      if( *( r.first - 1 ) == OPENPAR && *( r.second + 1 ) == CLOSEPAR )
      {
        r = search_name( r.second + 1, expr_.end() );
        continue;
      }

      // (x,
      if( *( r.first - 1 ) == OPENPAR && *( r.second + 1 ) == ARGS_SEPARATOR )
      {
        r = search_name( r.second + 1, expr_.end() );
        continue;
      }

      // ,x)
      if( *( r.first - 1 ) == ARGS_SEPARATOR && *( r.second + 1 ) == CLOSEPAR )
      {
        r = search_name( r.second + 1, expr_.end() );
        continue;
      }

      if( debug_ ) *os_p_ << "variable " << *r.first << " " << *r.second << '\n';
      string::const_iterator it = add_parentheses( r );
      r = search_name( it, expr_.end() );
    }

    // wrap functions
    string fname;
    r = search_function( expr_.begin(), expr_.end() );
     	  
    while( r.second != expr_.end() )
    {    
    		    
	  if( r.first == expr_.begin() || r.second == expr_.end() - 1 )
	  {
        string::const_iterator it = add_parentheses( r );
		r = search_function( expr_.begin(), expr_.end() );
		continue;
	  }	
      // (sin(x))
      if( *( r.first - 1 ) == OPENPAR && *( r.second + 1 ) == CLOSEPAR )
      {
		string::size_type op = expr_.find( OPENPAR, r.first - expr_.begin() );
        r = search_function( expr_.begin() + op, expr_.end() );
        continue;
      }

      // (sin(x),
      if( *( r.first - 1 ) == OPENPAR && *( r.second + 1 ) == ARGS_SEPARATOR )
      {
		string::size_type op = expr_.find( OPENPAR, r.first - expr_.begin() );
        r = search_function( expr_.begin() + op, expr_.end() );
        continue;
      }

      // ,sin(x))
      if( *( r.first - 1 ) == ARGS_SEPARATOR && *( r.second + 1 ) == CLOSEPAR )
      {
        string::size_type op = expr_.find( OPENPAR, r.first - expr_.begin() );
        r = search_function( expr_.begin() + op, expr_.end() );
        continue;
      }

      if( debug_ ) *os_p_ << "function " << *r.first << " " << *r.second << '\n';
      string::const_iterator it = add_parentheses( r );
      r = search_function( expr_.begin(), expr_.end() );
    }

    if( debug_ ) *os_p_ << "} wrap" << '\n';
      
  }
  
  //----------------------------------------------------------------------------
  namespace {  
  /*
   * 1) find first character that is not ')', if character not ']' return
   * 2) find matching opening '['
   * 3) read arguments between '[' and ']'
   * 4) return last argument read
   */  
  int get_out_values( const std::string& s,
  					  std::string::size_type e,
  					  const std::string::value_type CLOSE_PAR,
  					  const std::string::value_type OPEN_ARG_PAR,
  					  const std::string::value_type CLOSE_ARG_PAR )
  {
   	
  	while( s[ e ] == CLOSE_PAR ) --e;
  	if( s[ e ] != CLOSE_ARG_PAR ) return -1;
  	const std::string::size_type c = s.rfind( OPEN_ARG_PAR, e );
  	if( c == std::string::npos ) return -1;
  	
  	std::istringstream is( std::string( s.begin() + c + 1, s.begin() + e ) );
  	int i = -1;
  	while( is ) is >> i;
  	return i;
  
  }  
  }
  
  //----------------------------------------------------------------------------
   
  void math_parser::postfix_operators()
  {
    if( debug_ ) *os_p_ << "postfix_operators {" << '\n' << " " << expr_ << '\n';

    typedef string::size_type s_t;
    
    vector< operator_type >::const_iterator op_it = operators_.begin();
    
    const s_t invalid = string::npos;
      

    for( ; op_it != operators_.end(); ++op_it )
    {
      pair< s_t, s_t > op( invalid, invalid );
      s_t start = 0;
      
	 
      // find operator
      op.first = expr_.find( op_it->name() );
      
      while( op.first != string::npos )
      {
	  
		pair< s_t, s_t > left_operand( invalid, invalid );
        pair< s_t, s_t > right_operand( invalid, invalid );
                
        // last char of operator name
        op.second = op.first + op_it->name().size() - 1;

        if( op.second  == expr_.size() - 1 ) break;
        
        start = op.second + 1;

        if( expr_[ op.second + 1 ] == CLOSEPAR
            || expr_[ op.second + 1 ] == OPEN_ARG_PAR )
        {
          op.first = expr_.find( op_it->name(), start ); 
          if( op.first == string::npos ) break;
          continue;
        }
        // find left operand
        if( op.first != 0 && expr_[ op.first - 1 ] == CLOSEPAR )
        {
          // find matching opening parenthesis
          left_operand.first =
              backward_parenthesis_match( expr_, op.first - 1, OPENPAR, CLOSEPAR );
          left_operand.second = op.first - 1;
          /// @warning NO check, parentheses MUST already have been checked
        }

        // find right operand
        if(  ( op.second <expr_.size() - 1 ) &&
            expr_[ op.second + 1 ] == OPENPAR )
        {
          // find matching closing parenthesis
          right_operand.second =
              forward_parenthesis_match( expr_, op.second + 1, OPENPAR, CLOSEPAR );
          right_operand.first = op.second + 1;
          /// @warning NO check, parentheses MUST already have been checked         
        }


		// Problem (?) : ((1,2)) is assumed to be a single argument.

        int args_num = 0;
        const bool has_left_operand  = left_operand.first != invalid;
        const bool has_right_operand = right_operand.first != invalid;
        
        if( has_left_operand  ) ++args_num;
        if( has_right_operand ) ++args_num;
         
        // check number of arguments supported by operator
        if( op_it->operands() != args_num )
        {
          op.first = expr_.find( op_it->name(), start );
          if( op.first == string::npos ) break;
          continue;
        }

	    string::size_type op_expr_size = op.second - op.first + 1;
        
        // record width of operator expression:
        // "(left)+(right)"
        //  ^----span----^
        pair< s_t, s_t > span( op );

        // create new string (x)+(y) --> ((x)(y)+) 
        if( has_left_operand )
        {
          span.first = left_operand.first;
          op_expr_size += left_operand.second - left_operand.first + 1;
        }

        if( has_right_operand )
        {
          span.second = right_operand.second;
          op_expr_size += right_operand.second - right_operand.first + 1;
        }

        string op_expr( "" );

        string op_str = expr_.substr( op.first, op.second - op.first + 1 );
        
        // count number of left and right arguments
        if( count_args_ )
        {
          std::iterator_traits<
              string::const_iterator >::difference_type rargs = 0;
          std::iterator_traits<
              string::const_iterator >::difference_type largs = 0;
					

          if( has_left_operand )
		  {
		  	
		  	largs = get_out_values( expr_,
		  						   left_operand.second,
		  						   CLOSEPAR, OPEN_ARG_PAR, CLOSE_ARG_PAR );
		  		   
		  	if( largs < 0 )
		  	{
		  		largs = mm::count_if(
                          expr_.begin() + left_operand.first + 1,
                          expr_.begin() + left_operand.second,
                          argument( OPENPAR, CLOSEPAR, ARGS_SEPARATOR ) ) + 1;
		  	}					    

		  }
		  
		  if( has_right_operand )
          {
          	rargs = get_out_values( expr_,
		  						   right_operand.second,
		  						   CLOSEPAR, OPEN_ARG_PAR, CLOSE_ARG_PAR );
		  						   
  			if( rargs < 0 )
  			{
  				rargs = mm::count_if(
							expr_.begin() + right_operand.first + 1,
							expr_.begin() + right_operand.second,
							argument( OPENPAR, CLOSEPAR, ARGS_SEPARATOR ) ) + 1;
  			}			   

          }

		  vector< operator_type >::const_iterator oi = operators_.begin();
		  int ret_values = -1;
		  for( ; oi != operators_.end(); ++oi )
		  {
			if( oi->name() == op_it->name() && oi->largs() == largs
				&& oi->rargs() == rargs && oi->outvals() >= 0  )
			{
				ret_values = oi->outvals();
				break;
			}
		  } 
		
		  if( ret_values < 0 )
		  {
			struct ex : public std::exception {
			
				std::string msg;
				ex( const std::string& s ) : msg( s ) {} 
				const char* what() const throw()
				{
					return msg.c_str();
				}
				~ex() throw() {}
			};
			
			std::ostringstream m;
			typedef unsigned long ulong; // issue with gcc 3.4.2 MinGW
										// << unsigned long( i ) causes an error
										// issue with MS VS .NET 2003 no overload
							            // for operator <<( ostream&, std::size_t ) ???
			m << "operator " << op_str
			  << OPEN_ARG_PAR << ulong( largs ) << ' ' 
			  << ulong( rargs ) << ' ' << '?' << CLOSE_ARG_PAR
			  << " not found";
			throw ex( m.str() );
		  
		  }			
		  std::ostringstream os;
		  typedef unsigned long ulong; // issue with gcc 3.4.2 MinGW
									  // << unsigned long( i ) causes an error
									  // issue with MS VS .NET 2003 no overload
							          // for operator <<( ostream&, std::size_t ) ???
		  os << op_str << OPEN_ARG_PAR << ' ' << ulong( largs ) << ' ' 
			 << ulong( rargs ) << ' ' << ret_values << ' ' << CLOSE_ARG_PAR;
		  op_str = os.str();
		}
		 	
        op_expr += OPENPAR; // "("

	    if( debug_ ) *os_p_ << " op_expr: " << op_expr << '\n';
        
        string  left_operand_str( "" );
        string right_operand_str( "" );

        if( has_left_operand ) // (x)+(y) -> left = (x)
        {
          left_operand_str =
                expr_.substr( left_operand.first,
                              left_operand.second - left_operand.first + 1 );
		      if( debug_ ) *os_p_ << " op_expr: " << op_expr << '\n';
			  if( debug_ ) *os_p_ << " left op: " << left_operand_str << '\n';
        }

        if( has_right_operand ) // (x)+(y) -> right = (y)
        {
          right_operand_str =
                expr_.substr( right_operand.first,
                              right_operand.second - right_operand.first + 1 );
		      if( debug_ ) *os_p_ << " op_expr: "  << op_expr << '\n';
			  if( debug_ ) *os_p_ << " right op: " << right_operand_str << '\n';
        }

        // swap left and right operands
        const bool swap_args = op_it->swap();
        if( swap_args )
        {
          op_expr = right_operand_str + ',' + left_operand_str + ' ' + op_str;
                    
        }
        else
        {
           op_expr += left_operand_str + ',' + right_operand_str + ' ' + op_str;                     
        }

	    if( debug_ ) *os_p_ << " op_expr: " << op_expr << '\n';

        // "((x)(y))" or "((y)(x))"
        op_expr += CLOSEPAR;
	      if( debug_ ) *os_p_ << " op_expr: " << op_expr << '\n';
        
		
	    // replace operator expression inside expr_
        expr_.replace( span.first, span.second - span.first + 1, op_expr );
		       
        // search for next opearator with same name
        op.first = expr_.find( op_it->name(), start );
        if( op.first == string::npos ) break;  
      }
    }
    if( debug_ ) *os_p_ << " } postfix_operators" << '\n' << " " << expr_ << '\n';
  }


    
  //----------------------------------------------------------------------------
  void math_parser::postfix_functions()
  {
    if( debug_ ) *os_p_ << "postfix_functions {" << '\n' << " " << expr_ << '\n';

    typedef string::size_type s_t;

    range_type r = search_function( expr_.begin(), expr_.end() );
    while( r.second != expr_.end() )
    {
      const pair< s_t, s_t > span( r.first - expr_.begin(),
                                   r.second -  expr_.begin() );  

      pair< s_t, s_t > name( span );
      pair< s_t, s_t > parentheses( span );
      s_t open_par = find( expr_.begin() + span.first, expr_.end(),
                           OPENPAR ) - expr_.begin();

      name.second = open_par - 1;
      parentheses.first = open_par;

      string fun =
        expr_.substr( parentheses.first,
                      parentheses.second - parentheses.first + 1 );

      if( swap_args_ )
      {
        swap_function_args( fun, range_type( fun.begin(), --fun.end() ));
      }


	  const string fun_name =
					expr_.substr( name.first, name.second - name.first + 1 ); 	
      fun += fun_name;

      // count number of arguments
      if( count_args_ )
      {
        std::iterator_traits<
            string::const_iterator >::difference_type args = 0;
        if( parentheses.second - parentheses.first > 1 )
        {
		
		  args = get_out_values( expr_, span.second, CLOSEPAR,
								 OPEN_ARG_PAR, CLOSE_ARG_PAR );
		  if( args < 0 )
		  {	
			args = mm::count_if( expr_.begin() + open_par + 1,
								 expr_.begin() + span.second,
								 argument( OPENPAR, CLOSEPAR, ARGS_SEPARATOR ) )
								 + 1;
		  }
        }
		
        fun += OPEN_ARG_PAR;
        fun += ltoa( int( args ) );
		fun += CLOSE_ARG_PAR;
      }

      expr_.replace( span.first, span.second - span.first + 1, fun );

      r = search_function( expr_.begin(), expr_.end() );
    }
    if( debug_ ) *os_p_ << "} postfix_functions" << '\n' << " " << expr_ << '\n';
  }

  //----------------------------------------------------------------------------
  namespace {
  /// Check if two adjacent characters are equal to specified character.
  class equals {
	  string::value_type _v;
  public:
	  equals( string::value_type v ) : _v( v ) 
	  {}
	  bool operator()( string::value_type v1, string::value_type v2 )
	  {
		  return v1 == v2 && v1 == _v;
	  }
  };

  /// Return true if character is parenthesis or separator (e.g. ',').
  class to_remove {
    const string::value_type OPENPAR;
    const string::value_type CLOSEPAR;
    const string::value_type ARGS_SEPARATOR;
  public:
    to_remove( string::value_type op,
               string::value_type cp,
               string::value_type as )
               : OPENPAR( op ), CLOSEPAR( cp ), ARGS_SEPARATOR( as )
    {}
    bool operator ()( string::value_type ch ) const
    {
      return ch == OPENPAR || ch == CLOSEPAR || ch == ARGS_SEPARATOR;
    }  
  };
  }
  
  void math_parser::to_rpn()
  {
    if( debug_ ) *os_p_ << "to_rpn {" << '\n' << " " << expr_ << '\n';

	postfix_operators();
	
	postfix_functions(); 
	
	mm::replace_if( expr_.begin(), expr_.end(),
                to_remove( OPENPAR, CLOSEPAR, ARGS_SEPARATOR),
                           RPN_SEPARATOR );
    string::iterator end = unique( expr_.begin(), expr_.end(),
									                 equals( RPN_SEPARATOR ) );
    expr_.erase( end, expr_.end() );
    if( debug_ ) *os_p_ << "} to_rpn" << '\n' << " " << expr_ << '\n';
  }
 
  //============================================================================

} // namespace mmath_plus
//==============================================================================
