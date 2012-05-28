#ifndef REFCOUNT_PTR_H_
#define REFCOUNT_PTR_H_

// MicroMath+ - (c) Ugo Varetto

/// @file shared_ptr.h implementation of reference counted pointer

/// @todo add unspecified-bool idiom implementation to shared_ptr class

#include <cassert>

#ifdef MMP_DEBUG_MEMORY
#include "dbgnew.h"
#define new new( __FILE__, __LINE__, __FUNCTION__ )
#endif

namespace mmath_plus {

/// Increment integer; kept as an external function for future support of
/// atomic increment. 
inline int increment( int* pi ) { assert( pi ); return ++*pi; }

/// Decrement integer; kept as an external function for future support of
/// atomic increment. 
inline int decrement( int* pi ) { assert( pi ); return --*pi; }


/// Implementation of simple ref counted smart pointer with the minimal amount of functionality
/// needed from within MicroMath.    
template <class T> class shared_ptr
{
public:

    /// Value type
    typedef T element_type;
    /// Pointer type
    typedef T* pointer_type;

	/// Explicit constructor; allocates a new counter if required.
    explicit shared_ptr( T* p = 0 ) 
        : countPtr_(0), ptr_( p )
    { if (p) countPtr_ = new int( 1 ); }
    
    /// Copy constructor.
    shared_ptr(const shared_ptr& r) 
    { acquire(r); }
    
    /// Destructor.
    ~shared_ptr() 
    { release(); }
    
   	/// Assignment.   
    shared_ptr& operator=( const shared_ptr& r )
    {
        if (this != &r)
        {
            release();
            acquire( r );
        }
        return *this;
    }

	/// Constructor from shared pointer holding a pointer to an object of different
	/// type.
    template <class Y> shared_ptr(const shared_ptr<Y>& r) 
    {
        acquire( r );
    }

	/// Assignment from shared pointer holding a pointer to an object of different
	/// type.
    template <class Y> shared_ptr& operator=(const shared_ptr<Y>& r)
    {
        //if (this != &r)
		//{
            release();
            acquire(r);
        //}
        return *this;
    }

	/// Dereference operator.
    T& operator*()  const  {return *ptr_;}
    /// Member access operator.
    T* operator->() const  {return ptr_; }
    
    /// Allows for "if( !sharedPtr ) ...".    
    bool operator!() const { return ptr_ == 0; }

private:    
    /// Returned by operator Tester*, operator delete is not defined
    /// to prevent expressions like 'delete mySmartPtr;' to work.
    class Tester {    
    		void operator delete( void* );
    };
public:
    /// Returns 0 or pointer to static instance of inner class Tester; done
    /// to allow expressions as if( mySmartPointer ) ...
    operator Tester*() const
    { 
    		if( ptr_ == 0 ) return 0;
    		static Tester t;
    		return &t;
    } 
    
    /// Returns internal pointer to data. 
	friend inline pointer_type ptr( const shared_ptr& p ) { return p.ptr_; }
	
	/// Returns internal pointer to ref counter.
	friend inline int* ptrcount( const shared_ptr& p ) { return p.countPtr_; }   
    
    /// Equality shared_ptr == T*, used only in checks e.g. if( sp == 0 )... 
    friend inline bool operator==( const shared_ptr& sp, pointer_type p )
    { return sp.ptr_ == p; }
    
    /// Inequality shared_ptr != T*, used in checks only.
    friend inline bool operator!=( const shared_ptr& sp, pointer_type p )
    { return !operator==( sp, p ); }
    
    /// Equality T* == shared_ptr (for consistency).
    friend inline bool operator==( pointer_type p, const shared_ptr& sp )
    { return sp.ptr_ == p; }
    
    /// Equality T* != shared_ptr (for consistency).
    friend inline bool operator!=( pointer_type p, const shared_ptr& sp )
    { return !operator==( sp, p ); }  
     
    
private:
	    
 	/// Reference counter shared among different instances of shared_ptr
 	/// pointing to the same memory address.
    int* countPtr_;
    
    /// Pointer to data.
    T* ptr_;    
    
    /// Acquires new pointer and increments reference count.  	      
    template < class O >
	void acquire( const O& o) throw()
    { // increment the count
        ptr_ = ptr( o );
        countPtr_ = ptrcount( o );
        if (countPtr_) increment( countPtr_ );
    }

	/// Decrements ref counter and releases memory if counter == 0;
	/// delete is used to release memory, no custom deallocator provided. 
    void release()
    { // decrement the count, delete if it is 0
        if (countPtr_) {
            if ( decrement( countPtr_ ) == 0 ) {
                ///@warning assuming that memory was allocated with
				/// operator new, custom deallocator not supported.
				delete ptr_;
                delete countPtr_;
            }
            ptr_ = 0;
            countPtr_ = 0;
        }
    }
};

/// Implementation of pointer conversion 
template < class T, class U >
shared_ptr< T > static_pointer_cast( const shared_ptr< U >& p )
{
		shared_ptr< T > tp( static_cast< T* >( ptr( p ) ) );
		const int i = *( ptrcount( tp ) ) + *( ptrcount( p ) );
		*( ptrcount( tp ) ) = i;
		return tp;		
}


} // namespace mmath_plus

#ifdef MMP_DEBUG_MEMORY
#undef new
#endif

#endif /*REFCOUNT_PTR_H_*/
