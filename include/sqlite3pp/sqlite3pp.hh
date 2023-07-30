/* ========================================================================== *
 *
 * sqlite3pp.cpp
 *
 * The MIT License
 *
 * Copyright (c) 2015 Wongoo Lee (iwongu at gmail dot com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 *
 * -------------------------------------------------------------------------- */

#ifndef SQLITE3PP_H
#define SQLITE3PP_H

#define SQLITE3PP_VERSION "1.0.8"
#define SQLITE3PP_VERSION_MAJOR 1
#define SQLITE3PP_VERSION_MINOR 0
#define SQLITE3PP_VERSION_PATCH 8

#include <functional>
#include <iterator>
#include <stdexcept>
#include <string>
#include <tuple>

#ifdef SQLITE3PP_LOADABLE_EXTENSION
#include <sqlite3ext.h>
SQLITE_EXTENSION_INIT1
#else
#  include <sqlite3.h>
#endif

namespace sqlite3pp
{
  class database;

  namespace ext
  {
    class function;
    class aggregate;
    database borrow( sqlite3 * pdb );
  }

  template <class T> struct convert { using to_int = int; };

  class null_type {};

  class noncopyable
  {
   protected:
    noncopyable()  = default;
    ~noncopyable() = default;

    noncopyable(       noncopyable && ) = default;
    noncopyable( const noncopyable &  ) = delete;

    noncopyable & operator=(       noncopyable && ) = default;
    noncopyable & operator=( const noncopyable &  ) = delete;
  };

  class database : noncopyable
  {
    friend class statement;
    friend class database_error;
    friend class ext::function;
    friend class ext::aggregate;
    friend database ext::borrow( sqlite3 * pdb );

   public:
    using busy_handler     = std::function<int (int)>;
    using commit_handler   = std::function<int ()>;
    using rollback_handler = std::function<void ()>;
    using backup_handler   = std::function<void ( int, int, int )>;
    using update_handler   =
      std::function<void ( int, const char *, const char *, long long int )>;
    using authorize_handler = std::function<int (       int
                                                , const char *
                                                , const char *
                                                , const char *
                                                , const char *
                                                )>;

    explicit database(
      const char * dbname = nullptr
    ,       int    flags  = SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE
    , const char * vfs    = nullptr
    );

    database( database && db );

    database & operator=( database && db );

    ~database();

    int connect( const char * dbname, int flags, const char * vfs = nullptr );
    int disconnect();

    int attach( const char * dbname, const char * name );
    int detach( const char * name );

    int backup( database & destdb, backup_handler h = {} );
    int backup( const char           * dbname
              ,       database       & destdb
              , const char           * destdbname
              ,       backup_handler   h
              , int step_page = 5
              );

    long long int last_insert_rowid() const;

    int enable_foreign_keys(          bool enable = true );
    int enable_triggers(              bool enable = true );
    int enable_extended_result_codes( bool enable = true );

    int changes() const;

    int error_code() const;
    int extended_error_code() const;

    const char * error_msg() const;

    int execute(  const char * sql );
    int executef( const char * sql, ... );

    int set_busy_timeout( int ms );

    void set_busy_handler(      busy_handler      h );
    void set_commit_handler(    commit_handler    h );
    void set_rollback_handler(  rollback_handler  h );
    void set_update_handler(    update_handler    h );
    void set_authorize_handler( authorize_handler h );

   private:
    database(sqlite3* pdb) : db_( pdb ), borrowing_( true ) {}

   private:
    sqlite3 * db_;
    bool      borrowing_;

    busy_handler      bh_;
    commit_handler    ch_;
    rollback_handler  rh_;
    update_handler    uh_;
    authorize_handler ah_;
  };

  class database_error : public std::runtime_error
  {
   public:
    explicit database_error( const char * msg );
    explicit database_error( database & db );
  };

  enum copy_semantic { copy, nocopy };

  class statement : noncopyable
  {
   public:
    int prepare( const char * stmt );
    int finish();

    int bind( int idx, int value );
    int bind( int idx, double value );
    int bind( int idx, long long int value );
    int bind( int idx, const char * value, copy_semantic fcopy );
    int bind( int idx, const void * value, int n, copy_semantic fcopy );
    int bind( int idx, std::string const & value, copy_semantic fcopy );
    int bind( int idx );
    int bind( int idx, null_type );

    int bind( const char * name );
    int bind( const char * name, int           value );
    int bind( const char * name, double        value );
    int bind( const char * name, long long int value );
    int bind( const char * name, null_type );
    int bind( const char * name, const char * value, copy_semantic fcopy );
    int bind( const char          * name
            , const std::string   & value
            ,       copy_semantic   fcopy
            );
    int bind( const char * name
            , const void          * value
            ,       int             n
            ,       copy_semantic   fcopy
            );

    int step();
    int reset();

   protected:
    explicit statement( database & db, const char * stmt = nullptr );
    ~statement();

    int prepare_impl( const char         * stmt );
    int finish_impl(        sqlite3_stmt * stmt );

   protected:
          database     & db_;
          sqlite3_stmt * stmt_;
    const char         * tail_;
  };

  class command : public statement
  {
   public:
    class bindstream
    {
     public:
      bindstream( command & cmd, int idx );

        template <class T> bindstream &
      operator<<( T value )
      {
        auto rc = cmd_.bind( idx_, value );
        if ( rc != SQLITE_OK ) { throw database_error(cmd_.db_); }
        ++idx_;
        return * this;
      }

        bindstream &
      operator<<( char const * value )
      {
        auto rc = cmd_.bind( idx_, value, copy );
        if ( rc != SQLITE_OK) { throw database_error( cmd_.db_ ); }
        ++idx_;
        return * this;
      }

        bindstream &
      operator<<( std::string const & value )
      {
        auto rc = cmd_.bind( idx_, value, copy );
        if ( rc != SQLITE_OK ) { throw database_error( cmd_.db_ ); }
        ++idx_;
        return * this;
      }

     private:
      command & cmd_;
      int       idx_;
    };

    explicit command( database & db, const char * stmt = nullptr );

    bindstream binder( int idx = 1 );

    int execute();
    int execute_all();
  };

  class query : public statement
  {
   public:
    class rows
    {
     public:
      class getstream
      {
       public:
        getstream( rows * rws, int idx );

          template <class T> getstream &
        operator>>( T & value )
        {
          value = rws_->get( idx_, T() );
          ++idx_;
          return * this;
        }

       private:
        rows * rws_;
        int    idx_;
      };

      explicit rows( sqlite3_stmt* stmt );

      int data_count()            const;
      int column_type(  int idx ) const;
      int column_bytes( int idx ) const;

      template <class T> T get( int idx ) const { return get( idx, T() ); }

        template <class... Ts> std::tuple<Ts...>
      get_columns( typename convert<Ts>::to_int... idxs ) const
      {
        return std::make_tuple( get( idxs, Ts() )... );
      }

      getstream getter(int idx = 0);

     private:
      int           get( int idx, int           ) const;
      double        get( int idx, double        ) const;
      long long int get( int idx, long long int ) const;
      std::string   get( int idx, std::string   ) const;
      null_type     get( int idx, null_type     ) const;

      const char * get( int idx, const char * ) const;
      const void * get( int idx, const void * ) const;

     private:
      sqlite3_stmt * stmt_;
    };

    class query_iterator
//      : public std::iterator<std::input_iterator_tag, rows>
    {
     public:
      using iterator_category = std::input_iterator_tag;
      using value_type        = rows;
      using difference_type   = ptrdiff_t;
      using pointer           = rows *;
      using reference         = rows &;

      query_iterator();
      explicit query_iterator( query * cmd );

      bool operator==( query_iterator const & ) const;
      bool operator!=( query_iterator const & ) const;

      query_iterator & operator++();

        query_iterator
      operator++( int )
      {
        query_iterator tmp = * this;
        this->operator++();
        return tmp;
      }

      value_type operator*() const;

     private:
      query * cmd_;
      int     rc_;
    };

    explicit query( database & db, char const* stmt = nullptr );

    int column_count() const;

    char const * column_name( int idx ) const;
    char const * column_decltype( int idx ) const;

    using iterator = query_iterator;

    iterator begin();
    iterator end();
  };

  class transaction : noncopyable
  {
   public:
    explicit transaction( database & db
                        , bool       fcommit  = false
                        , bool       freserve = false
                        );
    ~transaction();

    int commit();
    int rollback();

   private:
    database * db_;
    bool       fcommit_;
  };

} // namespace sqlite3pp

#include "sqlite3pp.ipp"

#endif
