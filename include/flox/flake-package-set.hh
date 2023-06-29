/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <string>
#include "flox/package-set.hh"
#include "flox/util.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

class FlakePackageSet : public PackageSet {

  private:

    subtree_type                             _subtree;
    std::string                              _system;
    std::optional<std::string>               _stability;
    std::shared_ptr<nix::flake::LockedFlake> _flake;
    nix::ref<nix::EvalState>                 _state;

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

    // TODO: `readonly' flag for `db'

    FlakePackageSet(
            nix::ref<nix::EvalState>                   state
    ,       std::shared_ptr<nix::flake::LockedFlake>   flake
    , const subtree_type                             & subtree
    ,       std::string_view                           system
    , const std::optional<std::string_view>          & stability = std::nullopt
    ) : _state( state )
      , _subtree( subtree )
      , _system( system )
      , _stability( stability )
      , _flake( flake )
    {}

    FlakePackageSet(
            nix::ref<nix::EvalState>                   state
    , const FloxFlakeRef                             & flakeRef
    , const subtree_type                             & subtree
    ,       std::string_view                           system
    , const std::optional<std::string_view>          & stability = std::nullopt
    ,       bool                                       trace     = false
    ) : FlakePackageSet(
          state
        , std::make_shared<nix::flake::LockedFlake>(
            nix::flake::lockFlake( * state, flakeRef, floxFlakeLockFlags )
          )
        , subtree
        , system
        , stability
        )
    {}


/* -------------------------------------------------------------------------- */

      nix::flake::Fingerprint
    getFingerprint() const
    {
      return this->_flake->getFingerprint();
    }


/* -------------------------------------------------------------------------- */

    std::string_view getType()    const override { return "flake";        }
    subtree_type     getSubtree() const override { return this->_subtree; }
    std::string_view getSystem()  const override { return this->_system;  }

      std::optional<std::string_view>
    getStability() const override
    {
      if ( this->_stability.has_value() ) { return this->_stability; }
      else                                { return std::nullopt;     }
    }

      FloxFlakeRef
    getRef() const override
    {
      return this->_flake->flake.lockedRef;
    }


/* -------------------------------------------------------------------------- */

    bool        hasRelPath( const std::list<std::string_view> & path ) override;
    std::size_t size() override;


/* -------------------------------------------------------------------------- */

    template<bool IS_CONST> struct flake_iterator_impl;
    using flake_iterator       = flake_iterator_impl<false>;
    using flake_const_iterator = flake_iterator_impl<true>;

      template<bool IS_CONST>
    struct flake_iterator_impl : iterator_impl<IS_CONST>
    {
      private:
        todo_queue                               _todo;
        std::vector<nix::Symbol>::const_iterator _end;
        std::vector<nix::Symbol>::iterator       _it;

      public:
        explicit flake_iterator_impl( todo_queue todo )
          : _todo( todo )
        {
          if ( todo.empty() )
            {
              std::vector<nix::Symbol> e;
              this->_end = e.end();
              this->_it  = e.begin();
            }
          else
            {
              this->_end = this->_todo.front()->getAttrs().end();
              this->_it  = this->_todo.front()->getAttrs().begin();
            }
        }
        flake_iterator_impl() : flake_iterator_impl( todo_queue() ) {}

        std::string_view getType() const { return "flake"; }

        flake_iterator_impl & operator++();

          flake_iterator_impl
        operator++( int )
        {
          flake_iterator_impl tmp = * this;
          ++( * this );
          return tmp;
        }

        bool operator==( const iterator       & other ) const { return false; }
        bool operator==( const const_iterator & other ) const { return false; }
          bool
        operator==( const flake_iterator & other ) const
        {
          return this->_it == other._it;
        }
          bool
        operator==( const flake_const_iterator & other ) const
        {
          return this->_it == other._it;
        }

        bool operator!=( const iterator       & other ) const { return true; }
        bool operator!=( const const_iterator & other ) const { return true; }
          bool
        operator!=( const flake_iterator & other ) const
        {
          return ! ( ( * this ) == other );
        }
          bool
        operator!=( const flake_const_iterator & other ) const
        {
          return ! ( ( * this ) == other );
        }

      friend flake_const_iterator;
      friend flake_iterator;

    };  /* End struct `PackageSet::iterator_impl' */


/* -------------------------------------------------------------------------- */

    iterator       begin()       override;
    iterator       end()         override;
    const_iterator begin() const override;
    const_iterator end()   const override;


/* -------------------------------------------------------------------------- */

};  /* End class `FlakePackageSet' */


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
