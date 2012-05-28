// MicroMath+ - (c) Ugo Varetto

/// @file test.cpp implementation of console program to test MicroMath+


#include <string>
#include <iostream>
#include <exception>
#include <iomanip>
#include <iterator>
#include <algorithm>
#include <functional>
#include <fstream>

#include "compiler.h"
#include "execution.h"
#include "vm.h"
#include "def_rte.h"
#include "math_parser.h"

#ifdef MMP_DEBUG_MEMORY
#include "dbgnew.h"
#define new new( __FILE__, __LINE__, __FUNCTION__ )

/// Global instance of MemTracer class; it prints by default to std::clog stream
MemTracer NewTrace;
#endif

//-----------------------------------------------------------------------------

using namespace mmath_plus;  
using std::vector;
using std::cout;
using std::string;
using std::cin;
using std::getline;
using std::exception;
using std::boolalpha;
using std::endl;
using std::istream_iterator;
using std::copy;


//-----------------------------------------------------------------------------
/// Prefix of command strings.
static const string::value_type COMMAND_CHAR = '@';
/// Switch argument count on/off.
static const string TOGGLE_COUNT_ARGS        = "count";
/// Switch argument count on/off for functions in compiler.
static const string TOGGLE_COUNT_FUN_ARGS    = "countfun";
/// Switch argument reversal on/off.
static const string TOGGLE_REVERSE_ARGS      = "reverse";
/// Switch debug on/off.
static const string TOGGLE_DEBUG             = "debug";
/// Show flag values.
static const string PRINT_STATUS             = "status";
/// Enable/Disable substitution of operators with function.
static const string TOGGLE_REPLACE_OPS       = "replace";
/// Define new function.
static const string DEFINE_FUNCTION          = "defun";
/// Print list of supported functions
static const string LIST                     = "list";
/// Print list of supported functions
static const string VALUES                   = "vals";
/// Print list of supported functions
static const string QUIT                     = "quit";

/// Functor to print content of function_i*; used to print the content of
/// a vector of function_i* elements to an output stream.
struct opfun2str
{
    /// Overload for function objects
    /// @param pf function object
    /// Returns a string representation of function objects with
    /// # name
    /// # number of left operands
    /// # number of right operands
    /// # swap flag
    string operator()( const rte< double >::FunPtrT& pf ) const
    {
        std::ostringstream os;
        os << pf->name << "\tLEFT: " << pf->lvalues_in
            << "\tRIGHT: " << pf->rvalues_in 
            << "\tOUT: " << pf->values_out;
        return os.str();
    }
    /// Overload for operators
    /// @param op operator
    /// Returns a string representation of operators with:
    /// # name
    /// # number of left operands
    /// # number of right operands
    /// # swap flag
    string operator()( const mmath_plus::operator_type& op ) const
    {
        std::ostringstream os;
        os << op.name() << "\tLEFT: " << op.largs()
            << "\tRIGHT: " << op.rargs() 
            << "\tOUT: " << op.outvals() << "\tSWAP: " <<
            ( op.swap() ? 'Y' : 'N' );
        return os.str();
    }
    /// Returns name and value of value objects
    string operator()( const rte< double >::ValPtrT& vp ) const
    {
        std::ostringstream os;
        os << vp->name << " = " << vp->val;          
        return os.str();
    }           
};


//-----------------------------------------------------------------------------
/// Adds user defined function.
/// @param mp reference to math parser
/// @param r reference to run-time environment to which function is added
/// @param expr function source code
/// @param n function name
/// @param args arguments
/// @param out number of output values placed on the stack
/// @param largs number of values on the left side
template < class T >
void add_user_def_function( math_parser& mp,
                            rte< T >& r,
                            const string& expr,
                            const string& n,
                            const vector< string >& args,
                            int out,
						    int largs = 0 )                            
{
  typename rte< T >::fun_p_tab_type functions = generate_def_functions< T >();
  typename rte< T >::val_p_tab_type constants = generate_def_constants< T >();
  typename rte< T >::val_p_tab_type variables;
  typedef typename rte< T >::val_p_tab_type::value_type vptype;
  typedef typename rte< T >::fun_p_tab_type::value_type fptype;
  
  for( vector< string >::const_iterator i = args.begin();
       i != args.end();
       ++i )
  {
    // make sure parameters are well formed
    if( mmath_plus::find_if(
				i->begin(), i->end(), match_name() ) != i->begin() )
    { 
		struct ex : public std::exception {
            std::string msg;    
            ex( const std::string& m ) : msg( m ) {}
            const char* what() const throw()
            {
                return msg.c_str();
            }
            ~ex() throw() {}                
        };        
       
		throw ex( "wrong parameter: " + *i );
    }
    variables.push_back( vptype( new value< T >( *i ) ) );
  }
  
  // create local run-time environment
  rte< T > rt( functions, variables, constants );
  
  // use the pointer defined inside procedure
  typedef typename procedure<T>::executor_ptr_type exptype;
  exptype ex_p( new vm< rte< T > >( rt ) ); 
  
  // declare program type which will hold the sequence of instructions
  // generated by the compiler
  typename rte< T >::prog_type program;
  
  // parse 
  vector< math_parser::TokenPtr > vt = mp.parse( expr );
  
  // compile
  compiler< T > c( compiler< T >::DONT_COUNT_ARGS,
                   compiler< T >::CREATE_VARS );       
  program = c.compile( vt, rt );

  // add function to run-time environment
  r.fun_tab.push_back( fptype( new procedure<T>( program, ex_p,
                               n, int ( args.size() ), out, largs ) ) );  
}

//forward declaration

void print_usage();


//-----------------------------------------------------------------------------
/// Test parser, compiler and VM.
void test_vm() {
    
    
  typedef mmath_plus::operator_type op_t;
 
  // define operators for parser
  op_t ops_array[] = { // example of function accepting 6 parameters and returning
  	                   // 3 values
  					   op_t( "cross3", 1, 0, 6, 3 ),
					   op_t( "^", 2 ), op_t( "*", 2, 3, 3, 1 ),
					   op_t( "*", 2 ), op_t( "/", 2 ),
					   op_t( "-", 1, 0, 1, 1 ), op_t( "-", 2 ),
					   op_t( "-", 2, 3, 3, 3 ),
					   op_t( "+", 2, 3, 3, 3 ), op_t( "+", 2 ),
				       op_t( "=", 2, 1, 1, 1, true ),  
					   op_t( "=", 2, 3, 3, 3, true ) // 2 arguments, swap arguments
													 // to have variable name just
													 // before assignment operator
													 // x = 2 --> 2 x =
													 // This makes it easy to find
													 // the variable when the assignment
													 // operator is found. 
					 }; 	
 
  // anything in the form 'function_name( x1, x2,..., xn)' is assumed to be
  // a function accepting n arguments and returning 1 value.
  // It is possible to define functions accepting N arguments and returning M
  // values as operators; in this case the functions will have to be declared together with
  // the operators list if parse-time argument check is required, check the cross3 function
  // above.
  
  vector< op_t > ops( ops_array, ops_array + sizeof( ops_array ) / sizeof( op_t ) );

  // generate default run-time environment
  // the default run time environment supports all the standard C math functions.
  rte< double > rt = generate_default_rte< double >(); // Check memory allocation here
  
                                                         
  // build parser
  math_parser mp( ops,
                  math_parser::DONT_SWAP_ARGS,
                  math_parser::COUNT_ARGS,
                  math_parser::DEBUG,
                  &cout );
  
  // create compiler and virtual machine for execution
  compiler< double > c( compiler< double >::COUNT_ARGS,
                        compiler< double >::CREATE_VARS );
  vm< rte< double > > m( rt );
    
  // expression
  string expr;
  
  cout << "==============================================" << '\n';
  
  // read input; parse, compile and execute each entered line
  while( getline( cin, expr ) )
  {
	try
    {
      if( expr[ 0 ] == COMMAND_CHAR )
      {
        string command = expr.substr( 1, string::npos );
        if( command == TOGGLE_COUNT_ARGS )
        {
          mp.count_args( !mp.count_args() );
        }
        else if( command == QUIT )
        {
          break;
        }
        else if( command == TOGGLE_COUNT_FUN_ARGS )
        {
            c.count_args( !c.count_args() );
        }
        else if( command == TOGGLE_REVERSE_ARGS )
        {
          mp.rpn_swap( !mp.rpn_swap() );
        }
        else if( command == TOGGLE_DEBUG )
        {
          mp.debug( !mp.debug() );
        }
        else if( command == PRINT_STATUS )
        {
          cout << boolalpha;
          cout << "REVERSE ARGUMENTS   " << mp.rpn_swap()   << endl;
          cout << "COUNT ARGUMENTS     " << mp.count_args() << endl;
          cout << "COUNT FUN ARGUMENTS " << c.count_args()  << endl;
          cout << "DEBUG               " << mp.debug()      << endl;
        }
        else if( command == DEFINE_FUNCTION )
        {
          cout << "DEFINE FUNCTION "
               << "Enter <# of out values> <name> <list of input values>"
               << endl << " example: 1 myfun x y" << endl;
          getline( cin, expr );
		  std::istringstream is( expr.c_str() );
          int out_args;
          string fname;
          vector< string > in_args;
          
          is >> out_args >> fname;
          
          istream_iterator< string > ii( is );
          copy( ii, istream_iterator< string >(), back_inserter( in_args ) );
          cout << "TYPE BODY OF FUNCTION ON NEXT LINE" << endl;          
          getline( cin, expr );
          add_user_def_function( mp, rt, expr, fname, in_args, out_args );
        }
        else if( command == LIST )
        {
            cout <<  "==========================" << '\n';
            cout << "FUNCTIONS" << '\n' << "==========================" << '\n';
            std::transform( rt.fun_tab.begin(), rt.fun_tab.end(),
                            std::ostream_iterator< std::string >( cout, "\n" ),
                            opfun2str() );
            cout << "==========================" << '\n';
            cout << "OPERATORS" << '\n' << "==========================" << '\n';
            std::transform( ops.begin(), ops.end(),
                            std::ostream_iterator< std::string >( cout, "\n" ),
                            opfun2str() );
        }
        else if( command == VALUES )
        {
            cout <<  "==========================" << '\n';
            cout << "VARIABLES" << '\n' << "==========================" << '\n';
            std::transform( rt.var_tab.begin(), rt.var_tab.end(),
                            std::ostream_iterator< std::string >( cout, "\n" ),
                            opfun2str() );
            cout << "==========================" << '\n';
            cout << "CONSTANTS" << '\n' << "==========================" << '\n';
            std::transform( rt.const_tab.begin(), rt.const_tab.end(),
                            std::ostream_iterator< std::string >( cout, "\n" ),
                            opfun2str() );
        }
        else
        {
          cout << "UNKNOWN COMMAND; VALID COMMANDS: " << endl;
          print_usage();
        }
		cout << "==============================================" << '\n';
		continue;
      }

      // program
      rte< double >::prog_type program;

      // parse
      math_parser::Tokens vt = mp.parse( expr );
      
      // compile
      program = c.compile( vt, rt);
      
      // run program
      m.prog( &program );
      m.run();
      
      if( !m.rte().stack.empty() )
      {
        // print result i.e. value on top of stack
        cout << '\n' << "RESULT: "; 
		while( !m.rte().stack.empty() )
		{
			cout << m.rte().stack.top() << ' ';
			// remove value on top of stack
			m.rte().stack.pop();
		}
		cout << endl;
      }
      cout << "==============================================" << '\n';
    }
    catch( math_parser::unmatched_opening_par& uo_p )
    {
        cout << "unmatched opening parenthesis" << '\n';
        cout << uo_p << '\n';
        continue;
    }
    catch( math_parser::invalid_name& in_p )
    {
        cout << "invalid name" << '\n';
        cout << in_p << '\n';
        continue;
    }
    catch( math_parser::unmatched_closing_par& uc_p )
    {
        cout << "unmatched closing parenthesis" << '\n';
        cout << uc_p << '\n';
        continue;
    }
    catch( math_parser::unknown_symbol& us_p )
    {
        cout << "unknown symbol" << '\n';
        cout << us_p << '\n';
        continue;
    }
    catch( compiler< double >::unknown_token& ut_p )
    {
        cout << "unknown token" << '\n';
        cout << ut_p << '\n';
        continue;
    }
	catch( compiler< double >::null_token& nt_p )
    {
        cout << "null token" << '\n';
        cout << nt_p << '\n';
        continue;
    }
	catch( string& s )
	{
		cout << s << '\n';
	}
    catch( exception& e )
    {
      cout << e.what() << '\n';
      continue;
    }
    catch( ... )
    {
        cout << "unknown exception caught" << '\n';
        break;
    }    
  }

}

//-----------------------------------------------------------------------------

/// Prints usage.
void print_usage()
{
    cout << COMMAND_CHAR << TOGGLE_COUNT_ARGS   
         << "\t\ttoggle count arguments"   << endl;
    cout << COMMAND_CHAR << TOGGLE_COUNT_FUN_ARGS   
         << "\t\ttoggle count arguments for functions" << endl;     
    cout << COMMAND_CHAR << TOGGLE_REVERSE_ARGS
         << "\ttoggle reverse arguments" << endl;
    cout << COMMAND_CHAR << TOGGLE_DEBUG
         << "\t\ttoggle debug" << endl;
    cout << COMMAND_CHAR << PRINT_STATUS
         << "\t\tprint status" << endl;
    cout << COMMAND_CHAR << DEFINE_FUNCTION
         << "\t\tdefine new function" << endl;
    cout << COMMAND_CHAR << LIST
        << "\t\tlist supported operators & functions" << endl;
    cout << COMMAND_CHAR << VALUES
        << "\t\tlist variables and constants" << endl;    
    cout << COMMAND_CHAR << QUIT << "\t\tquit" << endl;      
}

//-----------------------------------------------------------------------------

/// Entry point.
int main( int, char** )
{
  print_usage();
  test_vm();
  cout << "\nbye\n";
  return 0;
}

//-----------------------------------------------------------------------------
