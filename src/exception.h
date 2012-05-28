#ifndef EXCEPTION_H__
#define EXCEPTION_H__

// MicroMath+ - (c) Ugo Varetto

/// @file exception.h exception base class

#include <string>
#include <ostream>

#ifdef MMP_DEBUG_MEMORY
#include "dbgnew.h"
#define new new( __FILE__, __LINE__, __FUNCTION__ )
#endif

//==============================================================================

namespace mmath_plus {

  
  //============================================================================

  //----------------------------------------------------------------------------
  /// Base class for exceptions.
  class exception_base {
  public:
    /// Constructor.
    /// @param ins namespace
    /// @param icls class throwing exception 
    /// @param ifun member function throwing exception
    /// @param ilineno line number at which exception is thrown
    /// @param idata message
    exception_base( const std::string& ins,
                    const std::string& icls,
                    const std::string& ifun,
                    unsigned long ilineno,
                    const std::string& idata = "" )
      : ns( ins ), cls( icls ), fun( ifun ), lineno( ilineno ), data( idata )
    {}
    /// Name space.
    const std::string ns;
    /// Class name.
    const std::string cls;
    /// Member function name.
    const std::string fun;
    /// Line number at which exception is thrown.
    const unsigned long lineno;
    /// Message.
    const std::string data;
  };

  /// Helper function to print exception.
  /// @param os reference to output stream
  /// @param eb reference to exception
  inline std::ostream& operator<<( std::ostream& os, const exception_base& eb )
  {
    return os << '\t' << eb.ns << "::" << eb.cls << "::" << eb.fun << '\n' 
              << '\t' << "Line #: " << eb.lineno << '\n'
              << '\t' << eb.data << '\n';
  }

  //============================================================================

} // namespace mmath_plus

//==============================================================================
#ifdef MMP_DEBUG_MEMORY
#undef new
#endif
#endif //EXCEPTION_H__
