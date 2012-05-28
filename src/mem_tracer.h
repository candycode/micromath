#ifndef TRACER_H_
#define TRACER_H_

// MicroMath+ - (c) Ugo Varetto

/// @file mem_tracer.h declaration of MemTracer class used to for tracking memory usage


#include <map>
#include <iostream>
/// Memory tracer. Modified version of code found on <em>www.relisoft.com</em>
/// Usage: store each pointer into map when memory allocated;
/// remove pointer from map when memory deallocated;
/// at destruction time it checks if map size is zero i.e. number of allocations ==
/// number of deallocations in the destructor.
/// In case map size not zero it prints a list of lines where non-deallocated memory was allocated.
class MemTracer {
private:
    /// Memory allocation entry
    class Entry
    {
    public:
        /// Constructor
        /// @param file source file where allocation occurred
        /// @param line line at which allocation occurred
        /// @param fun function in which allocation occurred
        /// @size number of allocated bytes 
        Entry( char const* file, int line, const char* fun, std::size_t size )
            : file_ (file), line_ (line), fun_( fun ), size_( size )
        {}
        /// Default constructor
        Entry ()
            : file_ (0), line_ (0), fun_( 0 ), size_( 0 )
        {}
        /// Returns the name of the file where allocation occurred 
        char const* File () const { return file_; }
        /// Returns the line at which allocation occurred
        int Line () const { return line_; }
        /// Returns the name of the function in which allocation occurred
        const char* Fun() const { return fun_; }
        /// Returns the number of allocated bytes
        std::size_t Size () const { return size_; }
    private:
        /// Name of file where allocation occurred
        char const * file_;
        /// Line at which allocation occurred
        int line_;
        /// Name of function in which allocation occurred
        const char* fun_;
        /// Number of allocated bytes
        std::size_t size_;
    };
    /// Locker class used to serialize access to tracer
    class Lock
    {
    public:
        /// Constructor: lock tracer upon construction
        /// @param tracer controlled tracer
        Lock (MemTracer& tracer)
            : tracer_ (tracer)
        {
            tracer_.lock ();
        }
        /// Destructor: release tracer upon destruction
        ~Lock ()
        {
            tracer_.unlock ();
        }
    private:
        /// Reference to controlled tracer object
        MemTracer& tracer_;
    };
    /// Entry Map iterator type
    typedef std::map< void*, Entry >::iterator iterator;
    friend class Lock;
public:
    /// Constructor
    /// @param os output stream used to log the results
    MemTracer ( std::ostream& os = std::clog );
    /// Default destructor: reports memory usage
    ~MemTracer ();
    /// Add memory allocation data into memory usage map
    /// @param p address of first byte of allocated memory
    /// @param file name of source file where allocation occurred
    /// @param line line at which allocation occurred
    /// @param fun name of function in which allocation occurred
    /// @param size number of allocated bytes
    void Add ( void* p, char const * file, int line, const char* fun, std::size_t size );
    /// Remove memory allocation data from map
    void Remove (void* p);
    /// Print memory usage report
    void Dump ();
    /// Set to true after initialization
    static bool Ready;
private:
    /// Temporary non-thread-safe implementation of lock method
    void lock()   { lockCount_++; }
    /// Temporary non-thread-safe implementation of unlock method
    void unlock() { lockCount_--; }
private:
    /// Memory address to memory usage entry map
    std::map< void*, Entry > map_;
    /// Lock count: used by MemTracer::lock() and MemTracer::unlock() methods
    /// to serialize access to instances of MemTracer class
    int lockCount_;
    /// Output stream
    std::ostream& os_;
    /// Allocated memory in bytes
    std::size_t amem_;
    /// Deallocated memory in bytes
    std::size_t dmem_;
};
#endif /*TRACER_H_*/
