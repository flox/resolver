/* ========================================================================== *
 *
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
        AttrSetIterClosure & _cl;
        todo_queue           _todo  = {};
        symbol_queue         _syms  = {};
        key_type             _key   = {};
        elem_type            _ptr   = nullptr;
        bool                 _recur = false;

      public:
        iterator( AttrSetIterClosure & cl, bool recur = false )
          : _cl( cl ), _recur( recur )
        {
          for ( auto & p : this->_cl._path ) { this->_key.push_back( p ); }
          for ( auto & key : this->_cl._cur->getAttrs() )
            {
              this->_syms.push( key );
            }
          if ( this->_syms.empty() ) { return; }
          nix::Symbol s = this->_syms.front();
          this->_syms.pop();
          this->_key.push_back( std::string( this->_cl._state->symbols[s] ) );
          this->_ptr = this->_cl._cur->getAttr( s );
        }

          iterator &
        operator++()
        {
          this->_key.pop_back();
          if ( this->_syms.empty() )
            {
              this->_ptr = nullptr;
            }
          else
            {
              nix::Symbol s = this->_syms.front();
              this->_syms.pop();
              this->_key.push_back(
                std::string( this->_cl._state->symbols[s] )
              );
              this->_ptr = this->_cl._cur->getAttr( s );
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
          return std::make_pair( this->_key, this->_ptr );
        }

    };  /* End struct `AttrsIterClosure::iterator' */


/* -------------------------------------------------------------------------- */

    sentinel end()   const { return sentinel();         }
    iterator begin()       { return iterator( * this ); }


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
