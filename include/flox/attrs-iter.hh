/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <string>
#include <nix/eval-eval-inline.hh>
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
      , _path( this->_state->_symbols.resolve( cur->getAttrPath() ) )
    {}

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
        todo_queue           _todo;
        symbol_queue         _syms;
        key_type             _key;
        elem_type            _ptr;

      public:
        // TODO
        iterator( AttrSetIterClosure & cl )
          : _cl( cl )
        {}

        iterator & operator++();

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

        reference operator*() { return * this->_ptr;                      }

    };  /* End struct `FlakePackageSet::iterator' */


/* -------------------------------------------------------------------------- */

    iterator begin();
    iterator end()   const { return const_iterator(); }

    std::size_t size();


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
