/* ========================================================================== *
 *
 * A generic iterator for `nix' attribute sets.
 *
 * This uses the list of symbols returned by
 * `nix::eval_cache::Cursor::getAttrs()' to create a fancier iterator that lets
 * the caller process a pair of `( ATTR-PATH, CURSOR )' pairs which is often
 * more ergonomic and avoids ugly boilerplate normally required to convert
 * symbols to attribute paths.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <string>
#include <nix/eval-inline.hh>
#include "flox/types.hh"
#include "flox/util.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace util {

/* -------------------------------------------------------------------------- */

/* `nix::ref<nix::eval_cache::AttrCursor>' and
 * `std::shared_ptr<nix::eval_cache::AttrCursor>'  */
using resolve::Cursor, resolve::MaybeCursor;


/* -------------------------------------------------------------------------- */

class AttrSetIterClosure {

  private:

    std::shared_ptr<nix::flake::LockedFlake> _flake = nullptr;
    std::shared_ptr<nix::EvalState>          _state = nullptr;
    std::list<std::string>                   _path  = {};
    MaybeCursor                              _cur   = nullptr;

      nix::ref<nix::eval_cache::EvalCache>
    openEvalCache() const
    {
      nix::flake::Fingerprint fingerprint = this->getFingerprint();
      return nix::make_ref<nix::eval_cache::EvalCache>(
        ( nix::evalSettings.useEvalCache && nix::evalSettings.pureEval )
        ? std::optional { std::cref( fingerprint ) }
        : std::nullopt
      , * this->_state
      , [&]()
        {
          nix::Value * vFlake = this->_state->allocValue();
          nix::flake::callFlake( * this->_state, * this->_flake, * vFlake );
          this->_state->forceAttrs(
            * vFlake, nix::noPos, "while parsing cached flake data"
          );
          nix::Attr * aOutputs = vFlake->attrs->get(
            this->_state->symbols.create( "outputs" )
          );
          assert( aOutputs != nullptr );
          return aOutputs->value;
        }
      );
    }


/* -------------------------------------------------------------------------- */

  public:

    AttrSetIterClosure(
      nix::ref<nix::EvalState>                 state
    , std::shared_ptr<nix::flake::LockedFlake> flake
    , Cursor                                   cur
    ) : _state( (std::shared_ptr<nix::EvalState>) state )
      , _flake( flake )
      , _cur( (MaybeCursor) cur )
    {
      for ( const auto & str :
              this->_state->symbols.resolve( cur->getAttrPath() )
          )
        {
          this->_path.emplace_back( str );
        }
    }

/* -------------------------------------------------------------------------- */

    AttrSetIterClosure(
            nix::ref<nix::EvalState>                   state
    ,       std::shared_ptr<nix::flake::LockedFlake>   flake
    , const std::list<std::string>                   & path
    ) : _state( (std::shared_ptr<nix::EvalState>) state )
      , _flake( flake )
      , _path( path )
    {
      nix::ref<nix::eval_cache::EvalCache> cache = this->openEvalCache();
      this->_cur = (MaybeCursor) cache->getRoot();
      for ( const auto & p : this->_path )
        {
          this->_cur = this->_cur->getAttr( p );
        }
    }


/* -------------------------------------------------------------------------- */

      std::shared_ptr<nix::flake::LockedFlake>
    getFlake() const
    {
      return this->_flake;
    }

      nix::flake::Fingerprint
    getFingerprint() const
    {
      return this->_flake->getFingerprint();
    }

      resolve::FloxFlakeRef
    getRef() const {
      return this->_flake->flake.lockedRef;
    }

      std::list<std::string_view>
    getPath() const
    {
      std::list<std::string_view> rsl;
      for ( const auto & p : this->_path ) { rsl.emplace_back( p ); }
      return std::move( rsl );
    }


/* -------------------------------------------------------------------------- */

    /* Empty struct to use as end of iterator marker. */
    struct sentinel {};

/* -------------------------------------------------------------------------- */

    struct iterator
    {
      using key_type  = std::list<std::string_view>;
      // using elem_type = std::shared_ptr<nix::eval_cache::AttrCursor>;
      using elem_type = MaybeCursor;

      using value_type = std::pair<key_type, elem_type>;
      using reference  = value_type;

      using todo_queue   = std::queue<Cursor, std::list<Cursor>>;
      using symbol_queue = std::queue<nix::Symbol, std::list<nix::Symbol>>;

      private:
        const AttrSetIterClosure & _cl;
        symbol_queue               _syms  = {};
        nix::Symbol                _attr  = {};
        elem_type                  _ptr   = nullptr;

      public:
        iterator( const AttrSetIterClosure & cl )
          : _cl( cl )
        {
          for ( auto & key : this->_cl._cur->getAttrs() )
            {
              this->_syms.push( key );
            }
          if ( this->_syms.empty() ) { return; }
          this->_attr = this->_syms.front();
          this->_syms.pop();
          this->_ptr = this->_cl._cur->getAttr( this->_attr );
        }

          iterator &
        operator++()
        {
          if ( this->_syms.empty() )
            {
              this->_ptr = nullptr;
            }
          else
            {
              this->_attr = this->_syms.front();
              this->_syms.pop();
              this->_ptr = this->_cl._cur->getAttr( this->_attr );
            }
          return * this;
        }

          iterator
        operator++( int )
        {
          iterator tmp = * this;
          ++( * this );
          return tmp;
        }

          bool
        operator==( const iterator & other ) const
        {
          return this->_ptr == other._ptr;
        }

          bool
        operator!=( const iterator & other ) const
        {
          return this->_ptr != other._ptr;
        }

          bool
        operator==( const sentinel & other ) const
        {
          return this->_ptr == nullptr;
        }

          bool
        operator!=( const sentinel & other ) const
        {
          return this->_ptr != nullptr;
        }

          reference
        operator*()
        {
          std::list<std::string_view> key = this->_cl.getPath();
          key.emplace_back( this->_cl._state->symbols[this->_attr] );
          return std::make_pair( std::move( key ), this->_ptr );
        }

    };  /* End struct `AttrsIterClosure::iterator' */


/* -------------------------------------------------------------------------- */

    iterator begin()  const { return iterator( * this ); }
    sentinel end()    const { return sentinel();         }
    iterator cbegin() const { return this->begin();      }
    sentinel cend()   const { return this->end();        }

    std::size_t size() const { return this->_cur->getAttrs().size(); }


/* -------------------------------------------------------------------------- */

};  /* End class `AttrSetIterClosure' */


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::util' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
