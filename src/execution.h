#ifndef EXECUTION_H__
#define EXECUTION_H__

// MicroMath+ - (c) Ugo Varetto

/// @file execution.h declaration and definition of execution-related types and functions.

#include <vector>
#include <stack>
#include <valarray>
#include <string>
#include <algorithm>

#include "shared_ptr.h"

#ifdef MMP_DEBUG_MEMORY
#include "dbgnew.h"
#define new new( __FILE__, __LINE__, __FUNCTION__ )
#endif
//=============================================================================

namespace mmath_plus {


  /// Forward declaration of run-time environment class.
  template < class T > struct rte;

  //===========================================================================

  //---------------------------------------------------------------------------
  /// Executor interface.
  /// Runs a program using std::stack, instructions, variables and functions
  /// found in the supplied run-time environment structure.
  template < class RteT >  struct executor {
    /// Type alias for run-time environment data
    typedef RteT rt_type;
    /// Stack type
    typedef typename rt_type::stack_type             stack_type;
    /// Program type 
    typedef typename rt_type::prog_type              prog_type;
    /// Value type of stack elements
    typedef typename rt_type::stack_type::value_type value_type;

    /// Returns const reference run-time environment.
    virtual const RteT& rte()  const            = 0;

    /// Returns reference to run-time environment.
    /// @todo not safe - remove ASAP used only in
    /// procedure< T >::operator()( rte< T >& ). And
    /// to remove values from std::stack.
    virtual RteT& rte()                         = 0;

    /// Returns const reference to instruction array.
    virtual const prog_type* prog() const       = 0;

    /// Sets run-time environment.
    virtual void rte( const RteT& rt)           = 0;

    /// Sets instruction array.
    virtual void prog( prog_type* pr )          = 0;

    /// Executes instructions in instruction array.
    /// @param i first instruction to execute
    virtual void run( typename prog_type::size_type i = 0 ) = 0;

    /// Destructor. @warning review
    virtual ~executor() {}
  };

  //===========================================================================

  //---------------------------------------------------------------------------
  /// Holds information about value types.
  template < class T >
  struct value {
    /// Value name.
    const std::string name;
    /// Value.
    T  val;
    /// Constructor.
    /// @param n name
    /// @param v value
    value( const std::string& n, T v = T() ) : name( n ), val( v )
    {}
  };

  //---------------------------------------------------------------------------
  /// Base interface for functions.
  template < class T >
  struct function_i {
   
	/// Name.
    const std::string name;
	
	/// Number of input values read from the std::stack.
    const int values_in;
	
	/// Number of output value placed on the std::stack.
    const int values_out;
	
	/// Number of left input values; used for operators.
	const int lvalues_in;
	
   	/// Number of right input values; used for operators.
	const int rvalues_in;

    /// Constructor.
    /// @param in number of arguments
    /// @param out number of returned values
    /// @param n name
	/// @param lin number of parameters on the left side, used for operators.
	/// @todo add try/catch block in parameter initialization and throw
	/// exception in case left parameters # > in parameters #.
    function_i( const std::string& n, int in, int out, int lin = 0 )
                : name( n ), values_in( in ), values_out( out ),
				  lvalues_in( lin ), rvalues_in( in - lin )
    {}

    /// Operator() called when function invoked.
    /// @param rt reference to run-time environment.
    virtual void operator()( rte< T >& rt ) const = 0;

    /// Virtual destructor.
    virtual ~function_i() {}
  };
  //===========================================================================

  //---------------------------------------------------------------------------
  /// Base interface for instructions.
  template < class T >
  struct instruction {
    /// Called when instruction executed.
    /// @param rt reference to run-time environment
    virtual void exec( rte< T >& rt ) = 0;
    /// Virtual destructor.
    virtual ~instruction()
    {}
  };

  //---------------------------------------------------------------------------
  /// Loads value on top of std::stack.
  template < class T >
  struct load_val : instruction< T > {
    /// Value.
    const T val;
    /// Executes instruction: loads value on top of std::stack.
    void exec( rte< T >& rt );
    /// Constructor.
    /// @param v values
    load_val( const T v ) : val( v ) {}
  };

  //---------------------------------------------------------------------------
  /// Loads variable value on top of std::stack.
  template < class T >
  struct load_var : instruction< T > {
    /// Pointer to value type in variable table.
    shared_ptr< value< T > > val_p;
    /// Loads value on top of std::stack.
    void exec( rte< T >& );
    /// Constructor.
    /// @param vp pointer to value type.
    load_var( const shared_ptr< value< T > >& vp ) : val_p( vp ) {}
  };

  //---------------------------------------------------------------------------
  /// Calls function.
  template < class T >
  struct call_fun : instruction< T > {
    /// Pointer to function object to be invoked.
    const shared_ptr< const function_i< T > > fun_p;
    /// Executes function by inkoking operator() on function object.
    /// @param rt reference to run-time environment
    void exec( rte< T >& rt ) { ( *fun_p )( rt ); }
    /// Constructor.
    /// @param fp pointer to function object.
    call_fun( const shared_ptr< const function_i< T > >& fp ) : fun_p( fp ) {}
  };

  //===========================================================================

  //---------------------------------------------------------------------------
  /// Utility class to construct a function_i object from a function object or
  /// function pointer.
  template < class FunT, class T >
  struct function : function_i< T > {
    /// Function to be called.
    const FunT fun;
    /// Constructor.
    /// @param f function pointer or function object
    /// @param n function name
    /// @param in number of input parameters
    /// @param out number of output values
    /// @param lin number of parameters on the left side, used for operators
    function( FunT f, const std::string& n, int in, int out, int lin = 0 )
              : function_i< T >( n, in, out, lin ), fun( f )
    {}
    /// Invokes function.
    /// @param rt reference to run-time environment
    void operator()( rte< T >& rt ) const
    {
      fun( rt );
    }
  };

  //===========================================================================

  //---------------------------------------------------------------------------
  /// Run-time environment.
  /// Used to store:
  ///   - functions
  ///   - variables
  ///   - constants
  ///   - program (could be stored outside the RTE)
  ///   - value std::stack
  ///   - execution std::stack
  ///   - instruction pointer
  template < class ValT > struct rte {
    /// Type alias for pointer to function objects
	typedef shared_ptr< function_i< ValT > > FunPtrT;
    /// Type alias for pointer to value objects
	typedef shared_ptr< value< ValT > >      ValPtrT;
    /// Type alias for pointer to instructions
	typedef shared_ptr< instruction< ValT> > InstrPtrT;
	/// Function table type
    typedef std::vector< FunPtrT >   fun_p_tab_type;
    /// Value table type
    typedef std::vector< ValPtrT >   val_p_tab_type;
    /// Program type
    typedef std::vector< InstrPtrT > prog_type;
    /// Value stack type
    typedef std::stack< ValT, std::vector< ValT > > stack_type;
    /// Execution stack type 
    typedef std::stack< typename prog_type::size_type,
						std::vector< typename prog_type::size_type > >
						exe_stack_type;
    /// Functions.
    fun_p_tab_type fun_tab;

    /// Variables.
    val_p_tab_type var_tab;

    /// Constants.
    val_p_tab_type const_tab;

    /// Program.
    prog_type*     prog_p;

    /// Value std::stack.
    /// Used to store values and results of computation.
    stack_type     stack;

    /// Execution std::stack.
    /// Used to store instruction addresses.
    exe_stack_type exe_stack;

    /// Instruction pointer.
    /// Index in instruction array.
    typename prog_type::size_type ip;

	/// @warning XXX REVIEW DO WE NEED A DEFAULT CONSTRUCTOR ???
	rte() : prog_p( 0 ), ip( 0 )
	{}

    /// Constructor.
    /// @param functions functions
    /// @param vars variables
    /// @param constants constants
    rte( fun_p_tab_type functions, val_p_tab_type vars,
         val_p_tab_type constants )
         : fun_tab( functions ), var_tab( vars ),
           const_tab( constants ), prog_p( 0 ), ip( 0 )
    {}

    /// Returns pointer to function given function name and number of
    /// arguments.
    /// @param s function name
    /// @param rargs number of arguments on the right side if args == -1 doesn't check number of
    /// arguments
    /// @param largs number of arguments on the left side
    /// @return function pointer
    typename fun_p_tab_type::value_type 
    function_p( const std::string& s, int rargs = -1,
				int largs = 0 ) const
    {
      typename fun_p_tab_type::const_iterator i;
      if( rargs < 0 )
      {
        for( i = fun_tab.begin(); i != fun_tab.end(); ++i )
        {
          if( ( *i )->name == s ) return *i;
        }
      }
      else
      {
        for( i = fun_tab.begin(); i != fun_tab.end(); ++i )
        {
          if( ( *i )->name == s && (*i)->rvalues_in == rargs
							  && (*i)->lvalues_in == largs ) return *i;
        }
      }
      typedef typename fun_p_tab_type::value_type v;
      return v();
    }

    /// Returns pointer to variable.
    /// @param name variable's name
    /// @return pointer to variable
    typename val_p_tab_type::value_type variable_p( const std::string& name ) const
    {
      typename val_p_tab_type::const_iterator i;
      for( i = var_tab.begin(); i != var_tab.end(); ++i )
      {
        if( (*i)->name == name ) return *i;
      }
	  typedef typename val_p_tab_type::value_type v;
      return v();
    }

    /// Returns pointer to constant.
    /// @param name constant's name
    /// @return pointer to constant
    typename val_p_tab_type::value_type constant_p( const std::string& name ) const
    {
      typename val_p_tab_type::const_iterator i;
      for( i = const_tab.begin(); i != const_tab.end(); ++i )
      {
        if( ( *i )->name == name ) return *i;
      }
	  typedef typename val_p_tab_type::value_type v;	
      return v();
    }	
	
  };


  //===========================================================================

  //---------------------------------------------------------------------------

  /// Loads value on top of std::stack.
  template < class T >
  inline void load_val< T >::exec( rte< T >& rt ) { rt.stack.push( val ); }

  /// Loads variable's value on top of std::stack.
  template < class T >
  void load_var< T >::exec( rte< T >& rt ) { rt.stack.push( val_p->val ); }

  //===========================================================================

} // namespace mmath_plus

//=============================================================================
#ifdef MMP_DEBUG_MEMORY
#undef new
#endif

#endif // EXECUTION_H__
