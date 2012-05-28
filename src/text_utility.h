#ifndef TEXT_UTILITY_H__
#define TEXT_UTILITY_H__

// MicroMath+ - (c) Ugo Varetto

/// @file text_utility.h  definition of text processing functions

#include <functional>
#include <string>
#include <vector>
#include <utility>
#include <cstdlib>

#include "mmp_algorithm.h"

#ifdef MMP_DEBUG_MEMORY
#include "dbgnew.h"
#define new new( __FILE__, __LINE__, __FUNCTION__ )
#endif

//==============================================================================

namespace mmath_plus {

  //============================================================================

  /// Character range values e.g. ['a', 'z'].
  typedef std::pair< std::string::value_type, std::string::value_type > val_range_type;

  /// Ierator range values. e.g. [std::string::begin(), std::string::end()].
  typedef std::pair< std::string::const_iterator, std::string::const_iterator > range_type;

  //----------------------------------------------------------------------------
  /// Returns iterator range for which the predicate holds true.
  template < class Pred >
  range_type search_range( std::string::const_iterator begin,
                           std::string::const_iterator end,
                           Pred p )
  {
    std::string::const_iterator b = mmath_plus::find_if( begin, end, p );
    if( b == end ) return range_type( end, end );
    std::string::const_iterator e =
								mmath_plus::find_if( b, end, std::not1( p ) );
    return range_type( b, e );
  }

  //----------------------------------------------------------------------------
  /// Counts number of arguments.
  class argument {
    /// Opening parenthesis.
    const std::string::value_type OPENPAR;
    /// Closing parenthesis.
    const std::string::value_type CLOSEPAR;
    /// Argument separator e.g. ','
    const std::string::value_type ARGS_SEPARATOR;
    /// Incremented each time an open parenthesis is found.
    mutable int _p;
  public:
    /// Constructor.
    /// @param op character identifying opening parenthesis
    /// @param cp character identifying closing parenthesis
    /// @param as character identifying argument separator
    argument( std::string::value_type op,
              std::string::value_type cp,
              std::string::value_type as )
              : OPENPAR( op ), CLOSEPAR( cp ), ARGS_SEPARATOR( as ), _p( 0 )
    {}
    /// Returns true if <code> ch </code> is part of an argument,
    /// false otherwise.
    /// @param ch character
    bool operator()( const std::string::value_type ch )
    {

      if( ch == OPENPAR ) { ++_p; return false; }
      if( ch == CLOSEPAR ) { --_p; return false; }
      if( ch == ARGS_SEPARATOR && !_p )  return true;

      return false;
    }
  };

  //----------------------------------------------------------------------------
  /// Stores argument ranges into provided collection.
  template < class C > class extract_argument {

    /// Iterator pointing to current last character read.
    range_type::first_type cnt_;
    /// Iterator pointing to first character of argument.
    range_type::first_type start_;
    /// Iterator pointing one past last character of input sequence.
    range_type::first_type end_;
    /// Function object returning true if character is part of argument.
    argument is_arg_;
    /// Collection into which extracted arguments are stored.
    C& _c;

  public:
    /// Constructor.
    /// @param c reference to collection
    /// @param r start-end points
    /// @param op opening parenthesis character
    /// @param cp closing parenthesis character
    /// @param as argument separator character
    extract_argument( C& c, // collection
                      range_type r, // start - end points
                      std::string::value_type op,
                      std::string::value_type cp,
                      std::string::value_type as )
                      : cnt_( r.first ), start_( r.first ), end_( r.second ),
                        is_arg_(  op, cp, as ), _c( c )

    {}

    /// If character is part of argument increment counter, if not
    /// all characters from start to one before the last character
    /// read represent an argument to be added to the collection.
    void operator()(const std::string::value_type ch )
    {

      if( cnt_ == end_ )
      {
        _c.push_back( range_type( start_, end_ ) );
        return;
      }

      if( is_arg_( ch ) )
      {
        _c.push_back( range_type( start_, --cnt_ ) );
        ++cnt_; ++cnt_;
        start_ = cnt_;
        return;
      }
      ++cnt_;
    }

  };

  //----------------------------------------------------------------------------
  /// Matches opening parenthesis. Counter is initialized to zero since
  /// iterator is assumed to point to current open parenthesis.
  class match_opening {
    /// Incremented (decremented) each time an opening (closing) parenthesis
    /// is found.
    int cnt_;
    /// Opening parenthesis.
    const std::string::value_type OPENPAR;
    /// Closing parenthesis.
    const std::string::value_type CLOSEPAR;
  public:
    /// Constructor.
    match_opening( const std::string::value_type opening,
                   const std::string::value_type closing )
                   : cnt_( 0 ), OPENPAR( opening ), CLOSEPAR( closing )
    {}
    /// Returns true if matching parenthesis found, false otherwise.
    bool operator()( const std::string::value_type c )
    {
      if( c == OPENPAR ) ++cnt_;
      if( c == CLOSEPAR ) --cnt_;
      if( !cnt_ ) return true;
      return false;
    }
  };

  //----------------------------------------------------------------------------
  /// Matches closing parenthesis. Counter is initialized to one
  /// in this case because reverse iterator points one position
  /// before current parenthesis.
  class match_closing {
    /// Incremented (decremented) each time a closing (opening) parenthesis
    /// is found.
    int cnt_;
    /// Opening parenthesis.
    const std::string::value_type OPENPAR;
    /// Closing parenthesis.
    const std::string::value_type CLOSEPAR;
  public:
    /// Constructor.
    match_closing( const std::string::value_type opening,
                   const std::string::value_type closing )
                   : cnt_( 1 ), OPENPAR( opening ), CLOSEPAR( closing )
    {}
    /// Returns true if matching parenthesis found.
    bool operator()( const std::string::value_type c )
    {
      if( c == OPENPAR ) --cnt_;
      if( c == CLOSEPAR ) ++cnt_;
      if( !cnt_ ) return true;
      return false;
    }
  };

  //----------------------------------------------------------------------------
  /// Finds forward matching parenthesis given a couple of iterators.
  inline std::string::const_iterator forward_parenthesis_match(
                                              std::string::const_iterator begin,
                                              std::string::const_iterator end,
                                              const std::string::value_type opening,
                                              const std::string::value_type closing )
  {
    return mmath_plus::find_if(begin, end, match_opening( opening, closing ) );
  }

  //----------------------------------------------------------------------------
  /// Finds forward matching parenthesis given a couple of iterators.
  inline std::string::iterator forward_parenthesis_get(
                                              std::string::iterator begin,
                                              std::string::iterator end,
                                              const std::string::value_type opening,
                                              const std::string::value_type closing )
  {
    return mmath_plus::find_if(begin, end, match_opening( opening, closing ) );
  }

	

  //----------------------------------------------------------------------------
  /// Finds backward matching parenthesis given a couple of operators.
  inline std::string::const_reverse_iterator backward_parenthesis_match(
                                          std::string::const_reverse_iterator begin,
                                          std::string::const_reverse_iterator end,
                                          const std::string::value_type opening,
                                          const std::string::value_type closing )
  {
    return mmath_plus::find_if(begin, end, match_closing( opening, closing ) );
  }

  //----------------------------------------------------------------------------
  /// Finds forward matching parenthesis given std::string and start position.
  inline std::string::size_type forward_parenthesis_match(
                                              const std::string& s,
                                              std::string::size_type i,
                                              const std::string::value_type opening,
                                              const std::string::value_type closing )
  {

    std::string::const_iterator it =
					mmath_plus::find_if(
						std::string::const_iterator( s.begin() + i ), s.end(),
                        match_opening( opening, closing ) );
    if( it == s.end() ) return std::string::npos;
    return ( it - s.begin() );
  }

  //----------------------------------------------------------------------------
  /// Finds backward matching parenthesis given std::string and start position.
  inline std::string::size_type backward_parenthesis_match(
                                              const std::string& s,
                                              std::string::size_type i,
                                              const std::string::value_type opening,
                                              const std::string::value_type closing )
  {
    std::string::const_reverse_iterator ri =
        mmath_plus::find_if( std::string::const_reverse_iterator( s.begin() + i ),
                 s.rend(),
                 match_closing( opening, closing ) );
    if( ri == s.rend() ) return std::string::npos;
    return ( --ri.base() - s.begin() );
  }

  //----------------------------------------------------------------------------
  /// std::find next character different from ch.
  inline std::string::const_iterator next_not_ch(
                                           const std::string::const_iterator begin,
                                           const std::string::const_iterator end,
                                           const std::string::value_type ch )
  {
    return mmath_plus::find_if( begin, end,
                    std::bind2nd(
						std::not_equal_to< std::string::value_type >(), ch ) );
  }

  //----------------------------------------------------------------------------
  /// std::find next character different from ch.
  inline std::string::const_iterator next_ch( const std::string::const_iterator begin,
                                         const std::string::const_iterator end,
                                         const std::string::value_type ch )
  {
    return std::find( begin, end, ch );
  }


  //----------------------------------------------------------------------------
  /// Matches number in the format 1.2E-3 or 1 or 1.2 .
  /// @warning: matches as well 1.2E and 1.2E- .
  class match_number : public std::unary_function< std::string::value_type, bool > {
    /// Found decimal point ?.
    mutable bool   dot_found_;
    /// Position of 'E'.
    mutable int    E_pos_;
    /// Number of matched characters.
    mutable int    cnt_;
    /// [ 0 - 9 ].
    val_range_type range_;
  public:
    /// Decimal separator
    static const std::string::value_type DOT   = '.';
    /// Exponent
    static const std::string::value_type E     = 'E';
    /// Plus
    static const std::string::value_type PLUS  = '+';
    /// Minus
    static const std::string::value_type MINUS = '-';
  public:

    /// Constructor.
    match_number( /*const std::string::value_type dot = '.',
                  const std::string::value_type e = 'E'*/ )
                  : dot_found_( false ), E_pos_( -1 ), cnt_( -1 ),
                    range_( val_range_type( '0', '9' ) )
    {}

    /// Reset values.
    void reset() const { dot_found_ = false; E_pos_ = -1; cnt_ = -1; }

    /// Returns true if character is part of a floating point number
    bool operator()( const std::string::value_type& ch ) const
    {
      ++cnt_;
      // is digit ?
      if( ch >= range_.first && ch <= range_.second ) return true;

      // decimal point
      if( ch == DOT )
      {
        // decimal point after 'E' ?
        if( E_pos_ >= 0 )
        {
          reset();
          return false;
        }

        // if decimal point not in number yet then return true, false
        // otherwise
        if( !dot_found_ ) { dot_found_ = true; return true; }
        else { reset(); return false; }
      }

      // 'E' ?
      if( toupper( ch ) == E )
      {
        // 'E' cannot be the first char in a number
        if( cnt_ == 0 )
        {
          reset();
          return false;
        }

        // if 'E' not found yet record position and return true
        if( E_pos_ < 0 )
        {
          E_pos_ = cnt_;
          return true;
        }
      }

      // if '+' or '-' found then return true only if '+' or '-' is
      // the first character after 'E'
      if( ( ch == PLUS || ch == MINUS ) && E_pos_ >= 0 && E_pos_ == cnt_ - 1 )
      {
        return true;
      }

      // no match
      reset();
      return false;
    }
  };

  //----------------------------------------------------------------------------
  /// Matches identifiers in the format 23abcd_ a_b_c_d_ abcd NOT 2abcd; stops
  /// at the first non matching character.
  class match_name : public std::unary_function< std::string::value_type, bool > {

    /// [0 - 9].
    val_range_type num_range_;
    /// [a - z]
    val_range_type lower_range_;
    /// [A - Z]
    val_range_type upper_range_;
    /// '_'
    const std::string::value_type UNDER_BAR;

    /// Number of matched characters; set to zero each time a non matching
    /// character is found.
    mutable int cnt_;

    /// Total number of characters read;
    mutable int n_cnt_;

    /// Buffer holding the last two characters read. XXX
    mutable std::string::value_type tmp_buf_[ 2 ];

    match_number match_number_;

  public:
    /// Constructor.
    match_name() : num_range_( val_range_type( '0', '9' ) ),
                   lower_range_( val_range_type( 'a', 'z' ) ),
                   upper_range_( val_range_type( 'A', 'Z' ) ),
                   UNDER_BAR( '_' ), cnt_( -1 ), n_cnt_( -1 )

    {
      tmp_buf_[ 0 ] = tmp_buf_[ 1 ] = ' ';
    }

    /// Resets counter.
    void reset() const { cnt_ = -1; }

    /// Returns true if character is member of a name.
    bool operator()( const std::string::value_type& ch ) const
    {
      ++cnt_;
      ++n_cnt_;
      // store char in buffer
      tmp_buf_[ n_cnt_ % 2 ] = ch;

      // if 'E' is found check that last two characters read are not numbers
      // or '.';
      if( !cnt_ && toupper( ch ) == match_number::E
          &&  ( match_number_( tmp_buf_[ 0 ] )
          || match_number_( tmp_buf_[ 1 ] ) ) )
      {
        reset();
        return false;
      }

      // [a - z] ?
      if( ch >= lower_range_.first && ch <= lower_range_.second ) return true;

      // [A - Z] ?
      if( ch >= upper_range_.first && ch <= upper_range_.second ) return true;

      // '_' ?
      if( ch == UNDER_BAR ) return true;

      // if number and char is not in first position return true
      if( ch >= num_range_.first && ch <= num_range_.second
          && cnt_ > 0) return true;

      // no match
      reset();
      return false;
    }
  };

  //----------------------------------------------------------------------------



  //============================================================================

} // namespace mmath_plus

//==============================================================================
#ifdef MMP_DEBUG_MEMORY
#undef new
#endif

#endif // TEXT_UTILITY_H__
