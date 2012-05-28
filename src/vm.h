#ifndef VM_H__
#define VM_H__

// MicroMath+ - (c) Ugo Varetto

/// @file vm.h definition of simple virtual machine

#include "execution.h"

#ifdef MMP_DEBUG_MEMORY
#include "dbgnew.h"
#define new new( __FILE__, __LINE__, __FUNCTION__ )
#endif
//==============================================================================

namespace mmath_plus {

  //============================================================================

  //----------------------------------------------------------------------------
  /// Simple virtual machine implementation.
  /// Run-time value stack is accessible through the stack() function.
  template < class RteT > class vm : public executor< RteT > {
  public:

    /// Type alias for program.
	typedef typename executor< RteT >::prog_type prog_type;

	/// Constructor.
    /// @param rt reference to run-time environment.
    vm( const RteT& rt ) : rte_( rt ) {}

    /// Destructor.
    virtual ~vm() {}

    /// Returns reference to run-time environment.
    const RteT& rte()  const { return rte_; }

    /// Returns reference to run-time environment.
    /// @todo not safe - remove ASAP used only in
    /// procedure< T >::operator()( rte< T >& ),
    /// to remove values from stack and to access rte::release_resources.
    virtual RteT& rte() { return rte_; }

    /// Returns constant reference to instruction array.
    const prog_type* prog() const { return rte_.prog_p; }

    /// Sets run-time environment.
    void rte( const RteT& rt) { rte_ = rt; }

    /// Sets instruction array.
    void prog( prog_type* pr ) { rte_.prog_p = pr; }

    /// Iterates through instruction array and execute each instruction.
    /// The run-time environment's instruction pointer is incremented
    /// after each instruction has been dereferenced but before the
    /// instruction is executed.
    /// exec() function is called on each instruction.
	///
	/// @param i entry point: this is the start value assigned to the
	/// instruction pointer.
    void run( typename prog_type::size_type i = 0 )
    {
      const prog_type& prog = *rte_.prog_p;
      const typename prog_type::size_type end = prog.size();
      rte_.ip = i;
      while( rte_.ip != end )
      {
        prog[ rte_.ip ]->exec( rte_ );
        ++rte_.ip;
      }
    }

  private:

    /// Run-time environment.
    RteT rte_;
  };

  //============================================================================

} // namespace mmath_plus

//==============================================================================
#ifdef MMP_DEBUG_MEMORY
#undef new
#endif

#endif //VM_H__
