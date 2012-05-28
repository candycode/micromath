#ifndef DEF_RTE_H__ 
#define DEF_RTE_H__

// MicroMath+ - (c) Ugo Varetto

/// @file def_rte.h sample run-time environment: pre-defined functions, variables, operators and constants

#include <cmath>
#include <exception>

#include "execution.h"
#include "adaptors.h"

#include "shared_ptr.h"

#ifdef MMP_DEBUG_MEMORY
#include "dbgnew.h"
#define new new( __FILE__, __LINE__, __FUNCTION__ )
#endif


//==============================================================================

namespace mmath_plus {

  //============================================================================

  using std::abs;  using std::acos;  using std::asin;
  using std::atan; using std::atan2; using std::ceil;
  using std::cos;  using std::cosh;  using std::exp;
  using std::fabs; using std::floor; using std::fmod;
  using std::log;  using std::log10; using std::pow;
  using std::sin;  using std::sinh;  using std::sqrt;
  using std::tan;  using std::tanh;

  //----------------------------------------------------------------------------
  /// Unary function object.
  template < class T >
  struct unary_function_t {
    const char* name; ///< Function name
    T (*f)( T );      ///< Function pointer
	const int left_params; ///< Number of parameters on the left side
  };

  /// Binary function object.
  template < class T >
  struct binary_function_t {
    const char* name; ///< Function name
    T (*f)( T, T );   ///< Function pointer
	const int left_params; ///< Number of parameters on the left side
  };

  /// Value type.
  template < class T >
  struct value_t {
    const char* name; ///< Value name
    T val; ///< Value data
  };

  //----------------------------------------------------------------------------
  /// Negate.
  template < class T > T neg( T v ) { return - v;  }
  /// Inverse.
  template < class T > T inv( T v ) { return 1 / v; }

  /// Add.
  template < class T > T add( T v1, T v2) { return v1 + v2; }
  /// Subtract.
  template < class T > T sub( T v1, T v2) { return v1 - v2; }
  /// Multiply.
  template < class T > T mul( T v1, T v2) { return v1 * v2; }
  /// Divide.
  template < class T > T div( T v1, T v2) { return v1 / v2; }

  /// Default unary function table.
  unary_function_t< double > unary_functions[] =
  {
    { "abs",   fabs,  0 }, { "acos", acos, 0 }, { "asin",  asin,  0 }, { "atan", atan, 0 },
    { "ceil",  ceil,  0 }, { "cos",  cos,  0 }, { "cosh",  cosh,  0 }, { "exp",  exp,  0 },
    { "floor", floor, 0 }, { "log",  log,  0 }, { "log10", log10, 0 }, { "sin",  sin,  0 },
    { "sinh",  sinh,  0 }, { "sqrt", sqrt, 0 }, { "tan",   tan,   0 }, { "inv",  inv,  0 },
    { "-",     neg,   1 }
  };

  /// Default binary function table.
  binary_function_t< double > binary_functions[] =
  {
    { "^",   pow, 1 }, { "*",     mul,   1 }, { "/", div, 1 },   { "+", add, 1 },
    { "-",   sub, 1 }, { "%",     fmod,  1 },
    { "add", add, 0 }, { "sub",   sub,   0 }, { "div", div, 0 }, { "mul", mul, 0 },
    { "pow", pow, 0 }, { "atan2", atan2, 0 }
  };

  /// Default constants.
  value_t< double > constants[] =
  {
    { "e", 2.71828182845904523536 }, { "log2e", 1.44269504088896340736 },
    { "Pi", 3.14159265358979323846 }
  };

  /// Default variables.
  value_t< double > variables[] =
  {
    { "x", 0.0 }, { "y", 0.0 }, { "z", 0.0 }, { "w", 0.0 }
  };

  //----------------------------------------------------------------------------
  /// Thrown when invalid assignment detected.
  class invalid_assign : public std::exception {
    const char *what( ) const throw( )
    {
      return "invalid assignment";
    }
  };

  //----------------------------------------------------------------------------
  /// Assignment function.
  /// Assigns value to variable and loads value on top of stack.
  /// @warning Operands need to be swapped: x = 2 --> 2 x = NOT x 2 =
  /// @warning RTTI must be enabled.
  template < class T >
  struct scalar_assign : public function_i< T > {

    /// Constructor.
    scalar_assign()
      : function_i< T >( "=", 2, 1, 1 )
    {}

    /// Invoked when assignment function is called.
    void operator()( rte< T >& rt ) const
    {
      //
      // |stack| |        program           | |IP|
      // -----------------------------------------
      // |  0  | | load value               | | 0|
      // |  1  | | load value of variable X | | 1|
      // |     | | call assignment function | | 2|
      //

      // rt.ip points to current instruction

      // 2 x =
      const typename rte< T >::prog_type& prog = *rt.prog_p;
      // remove value of 'x' from stack. 
	  rt.stack.pop();
      // get pointer to load_var('x') instruction: current instruction pointed
	  // at by Instruction Pointer is assignment (this), load_var is the
	  // previous instruction i.e. IP - 1.
	  load_var< T >* lv_p = dynamic_cast< load_var< T >* >( ptr( prog[ rt.ip - 1 ] ) );
      // if the previous instruction is not a load_var then assignment
	  // expression is wrong.
	  if( !lv_p ) throw invalid_assign();
	  // got a pointer to load_var; next: retrieve pointer to variable value
	  // and set it to value currently on top of stack.
	  // value is not removed from stack i.e. assignment returns the value
	  // assigned to variable.
      lv_p->val_p->val = rt.stack.top();
    }
	
  };

  //----------------------------------------------------------------------------		
  /// Multi-dimensional assign: (x,y,z)=(1,2,3).
  template < class T, int N >
  struct vector_assign : public function_i< T > {
   
   /// Constructor.
   vector_assign()
      : function_i< T >( "=", 2 * N, N, N )
    {}
    
   /// Invoked when assignment function is called.		
   void operator()( rte< T >& rt ) const
   {
	  //
      // |stack| |        program           | | IP  |
      // --------------------------------------------
      // |  0  | | load value               | |  0  |
	  // |  1  | | load value               | |  1  |
	  // |  .  | | load value               | |  .  |
	  // |  .  | | load value               | |  .  |
	  // |  .  | | load value               | |  .  |
	  // |  N  | | load value               | |  N  |
      // |     | | load value of variable X1| | N+1 |
	  // |     | | load value of variable X2| | N+2 |
	  // |     | |             .            | |  .  |
	  // |     | |             .            | |  .  |
	  // |     | |             .            | |  .  |
	  // |     | | load value of variable Xn| | N+n |
      // |     | | call assignment function | |N+n+1|
      
      // rt.ip points to current instruction

      // 1 2... x1 x2... =
      const typename rte< T >::prog_type& prog = *rt.prog_p;
      
	  T v[ N ];	  
	  
	  // remove values of variables from stack
	  if( !rt.stack.empty() ) for( int i = 0; i != N && !rt.stack.empty(); ++i ) rt.stack.pop();
	  
	  // get values, remove values from stack and assign values to variables
	  for( int i = 0; i != N && !rt.stack.empty(); ++i )
	  {
		 load_var< T >* lv_p = dynamic_cast< load_var< T >* >(
						ptr( prog[ rt.ip - ( 1 + i ) ] ) );
		 if( !lv_p ) throw invalid_assign();
		 v[ i ] = rt.stack.top(); rt.stack.pop();
		 lv_p->val_p->val = v[ i ];		  	
	  }
	  
	  // push values back on stack
	  for( int i = ( N - 1 ); i >= 0; --i ) rt.stack.push( v[ i ] );	 	
   }
        	  
  };			
	

  //----------------------------------------------------------------------------
  /// Dot product R^3-->R.
  /// (1,2,3)*(1,2,3)=1*1+2*2+3*3. 
  template < class T >
  struct dotprod3 : public function_i< T > {

    /// Constructor.
    dotprod3()
      : function_i< T >( "*", 6, 1, 3 )
    {}

    /// Invoked when sum function is called: 4 parameters are read and removed
	/// from the stack, two placed on the stack.
    void operator()( rte< T >& rt ) const
    {
		const T z2 = rt.stack.top(); rt.stack.pop();
		const T y2 = rt.stack.top(); rt.stack.pop();
		const T x2 = rt.stack.top(); rt.stack.pop();
		const T z1 = rt.stack.top(); rt.stack.pop();
		const T y1 = rt.stack.top(); rt.stack.pop();
		const T x1 = rt.stack.top(); rt.stack.pop();
		rt.stack.push( x1*x2 + y1*y2 + z1*z2 );
    }
  };
  
  
  //----------------------------------------------------------------------------
  /// Cross product R^3-->R^3.
  /// (1,2,3)^(4,5,6)=(2*6-3*5,-(1*6-3*4),1*5-2*4) 
  template < class T >
  struct crossprod3 : public function_i< T > {

    /// Constructor.
    crossprod3()
      : function_i< T >( "cross3", 6, 3, 0 ) // 6 parameters in
                                             // 3 values out
                                             // 0 parameters on the left side		
    {}

    /// Invoked when sum function is called: 4 parameters are read and removed
	/// from the stack, two placed on the stack.
    void operator()( rte< T >& rt ) const
    {
		const T z2 = rt.stack.top(); rt.stack.pop();
		const T y2 = rt.stack.top(); rt.stack.pop();
		const T x2 = rt.stack.top(); rt.stack.pop();
		const T z1 = rt.stack.top(); rt.stack.pop();
		const T y1 = rt.stack.top(); rt.stack.pop();
		const T x1 = rt.stack.top(); rt.stack.pop();
		rt.stack.push( y1*z2 - y2*z1 );
		rt.stack.push( x2*z1 - x1*z2 );
		rt.stack.push( x1*y2 - x2*y1 );
    }
  };

  
				     	
  //----------------------------------------------------------------------------		
  /// Adaptor that creates a function accepting N arguments and returning
  /// N values from an R^1-->R^1 function.
  /// + --> Adaptor 3 --> (1,2,3) + (4,5,6) == (1+4,2+5,3+6).
  template < class T, int N >
  class vector_op_apply : public function_i< T > {
  	typedef shared_ptr< function_i< T > > FunPtr;
  private:
	FunPtr fp_;	;
  public:   
   /// Constructor vector_apply DOES own the memory by default.
   vector_op_apply( FunPtr fp )
      : function_i< T >( fp->name, 2 * N, N, N ), fp_( fp )
    {
		if( fp->lvalues_in != 1 || fp->rvalues_in != 1 )
		{
			struct ex : public std::exception {
				const char* what() const throw()
				{	return "ONLY BINARY OPERATORS SUPPORTED"; }			
			};
			throw ex(); 
		}
	}

	/// Applies the R^1-->R^1 function to each element.
   void operator()( rte< T >& rt ) const
   {
	  T v1[ N ];
	  T v2[ N ];
	  T vo[ N ]	  ;
	  for( int i = 0; i < N; ++i )
	  {
		v2[ i ] = rt.stack.top(); rt.stack.pop();
	  }
	  for( int i = 0; i < N; ++i )
	  {
		v1[ i ] = rt.stack.top(); rt.stack.pop();
	  }
	  for( int i = 0; i < N; ++i )
	  {
		 rt.stack.push( v1[ i ] );
		 rt.stack.push( v2[ i ] );
		 ( *fp_ )( rt );
		 vo[ i ] = rt.stack.top(); rt.stack.pop();		   
	  }
		  
	  for( int i = ( N - 1 ); i >= 0; --i ) rt.stack.push( vo[ i ] );	 	
   }  
   				    	  
  };			
			

  //----------------------------------------------------------------------------
  /// Procedure: Executes a compiled program.
  /// @todo share run-time environments using pointers; as of now each function
  /// has its own separated execution environment.
  /// @todo add constructor parameter to free resources if procedure owns memory
  template < class T >
  class procedure : public function_i< T > {      
  public:
    /// Pointer to executor type.
    typedef shared_ptr< executor< rte< T > > > executor_ptr_type;  
    /// Constructor.
    procedure( const typename executor< rte< T > >::prog_type& prog,
               executor_ptr_type vm_p,
			   const std::string& name,
               int in,
               int out,
			   int lin = 0 )
      : function_i< T >( name, in, out, lin ),
        vm_p_( vm_p ), proc_( prog ),
        in_( in ), out_( out ), lin_( lin ), rin_( in - lin )
    {
      vm_p_->prog( &proc_ );
    }

    /// invoked when function called.
    void operator()( rte< T >& rt ) const
    {
      // get reference to local run-time environment
      rte< T >& r = vm_p_->rte();

      // read values from stack
      typename rte< T >::val_p_tab_type::iterator vi = r.var_tab.begin();
      const typename rte< T >::val_p_tab_type::iterator end	= r.var_tab.end();
      
      // copy values from external run-time environment into local variables
      for( int i = 0; ( i < in_ ) && ( vi != end ); ++i, ++vi )
      {
        T v = rt.stack.top(); rt.stack.pop();
        (*vi)->val = v;
      }

      // execute new procedure
      vm_p_->run();

      // std::copy values onto stack
      for( int o = 0; o < out_; ++o )
      {
        rt.stack.push( r.stack.top() );
        r.stack.pop();
      }
    }
	
  private:
	executor_ptr_type vm_p_;
    typename rte< T >::prog_type proc_;
    int in_;
    int out_;
	int lin_;
	int rin_;
  };


  //----------------------------------------------------------------------------
  /// Generates unary function table.
  /// @param functions array of unary functions
  /// @param n array size
  /// @return table of unary functions
  template < class T >
  typename rte< T >::fun_p_tab_type generate_unary_functions(
                                              unary_function_t< T > functions[],
                                              size_t n )
  {
    typename rte< T >::fun_p_tab_type fun_tab( n );
    typename rte< T >::fun_p_tab_type::iterator i;
    size_t j;
    for( i = fun_tab.begin(), j = 0; i != fun_tab.end(); ++i, ++j )
    {
      unary_function< T > uf( functions[ j ].f );
      typedef typename rte< T >::fun_p_tab_type::value_type pointer_type;
	  *i = pointer_type( new function< unary_function< T >, T >
										( uf , functions[ j ].name,
										  1, 1,
										  functions[ j ].left_params ) );
    }

    return fun_tab;
  }

  //----------------------------------------------------------------------------
  /// Generates binary function table.
  /// @param functions array of binary functions
  /// @param n array size
  /// @return table of binary functions
  template < class T >
  typename rte< T >::fun_p_tab_type generate_binary_functions(
                                              binary_function_t< T > functions[],
                                              size_t n )
  {
    typename rte< T >::fun_p_tab_type fun_tab( n );
    typename rte< T >::fun_p_tab_type::iterator i;
    size_t j;
    for( i = fun_tab.begin(), j = 0; i != fun_tab.end(); ++i, ++j )
    {
      binary_function< T > bf( functions[ j ].f );
      typedef typename rte< T >::fun_p_tab_type::value_type pointer_type;
	  *i = pointer_type( new function< binary_function< T >, T >(
												bf, functions[ j ].name,
												2, 1,
												functions[ j ].left_params ) );
    }

    return fun_tab;
  }

  //----------------------------------------------------------------------------
  /// Generates variable table.
  /// @param vars array of variables
  /// @param n array size
  /// @return table of variables
  template < class T >
  typename rte< T >::val_p_tab_type generate_variables( value_t< T > vars[], size_t n )
  {
    typename rte< T >::val_p_tab_type var_tab( n );
    typename rte< T >::val_p_tab_type::iterator i;
    size_t j;
    for( i = var_tab.begin(), j = 0; i != var_tab.end(); ++i, ++j )
    {
	  typedef typename rte< T >::val_p_tab_type::value_type pointer_type;	
      *i = pointer_type( new value< T >( vars[ j ].name, vars[ j ].val ) );
    }

    return var_tab;
  }

  //----------------------------------------------------------------------------
  /// Generates constant table.
  /// @param constants array of constants
  /// @param n array size
  /// @return table of constants
  template < class T >
  typename rte< T >::val_p_tab_type generate_constants( value_t< T > constants[],
                                               size_t n )
  {
    typename rte< T >::val_p_tab_type const_tab( n );
    typename rte< T >::val_p_tab_type::iterator i;
    size_t j;
    for( i = const_tab.begin(), j = 0; i != const_tab.end(); ++i, ++j )
    {
	  typedef typename rte< T >::val_p_tab_type::value_type pointer_type;
      *i = pointer_type( new value< T >(
							constants[ j ].name, constants[ j ].val ) );
    }

    return const_tab;
  }

  //----------------------------------------------------------------------------
  /// Generates default function table.
  /// @return table of functions
  template < class T >
  typename rte< T >::fun_p_tab_type generate_def_functions()
  {
    typename rte< T >::fun_p_tab_type ft =
      generate_unary_functions< T >(
         unary_functions,
         sizeof( unary_functions ) / sizeof( unary_functions[ 0 ] ) );

    typename rte< T >::fun_p_tab_type bf =
      generate_binary_functions< T >(
         binary_functions,
         sizeof( binary_functions ) / sizeof( binary_functions[ 0 ] ) );

    std::copy( bf.begin(), bf.end(), std::back_inserter( ft ) );

	typedef typename rte< T >::fun_p_tab_type::value_type pointer_type;
	
	ft.push_back( pointer_type( new vector_assign< double, 4 >() ) );
	ft.push_back( pointer_type( new vector_assign< double, 3 >() ) );
	ft.push_back( pointer_type( new vector_assign< double, 2 >() ) );
	ft.push_back( pointer_type( new crossprod3< double >() ) );
    
	ft.push_back( pointer_type( new dotprod3< double >() ) );
	
	for( size_t i = 0; i < sizeof( binary_functions ) / sizeof( binary_functions[ 0 ] ); ++i )
	{
		if( binary_functions[ i ].left_params == 1 )
		{
			binary_function< T > f( binary_functions[ i ].f );
			pointer_type f3d( new function< binary_function< T >, T >(
													f, binary_functions[ i ].name,
													2, 1,
													binary_functions[ i ].left_params ) );
			ft.push_back( pointer_type( new vector_op_apply< double, 3 >( f3d ) ) );
		}
	}
	
	ft.push_back( pointer_type( new scalar_assign< double >() ) );
	
    return ft;
  }

  //----------------------------------------------------------------------------
  /// Generates default constant table.
  /// @return table of constants
  template < class T >
  typename rte< T >::val_p_tab_type generate_def_constants()
  {
    const size_t cs = sizeof( constants ) / sizeof( constants[ 0 ] );
    return generate_constants< T >( constants, cs );
  }

  //----------------------------------------------------------------------------
  /// Generates default variable table.
  /// @return table of variables
  template < class T >
  typename rte< T >::val_p_tab_type generate_def_variables()
  {
    const size_t vs = sizeof( variables ) / sizeof( variables[ 0 ] );
    return generate_variables< T >( variables, vs );
  }

  //----------------------------------------------------------------------------
  /// Generates default run-time environment object.
  /// @return default run-time environment
  template < class T >
  rte< T > generate_default_rte()
  {

    return rte< T >( generate_def_functions< T >(),
                     generate_def_variables< T >(),
                     generate_def_constants< T >() );
  }

  //============================================================================

} // namespace mmath_plus

//==============================================================================
#ifdef MMP_DEBUG_MEMORY
#undef new
#endif

#endif // DEF_RTE_H__
