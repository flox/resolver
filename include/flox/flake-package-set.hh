/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <string>
#include "flox/util.hh"
#include "flox/package-set.hh"


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
            nix::ref<nix::EvalState>          state
    , const FloxFlakeRef                    & flakeRef
    , const subtree_type                    & subtree
    ,       std::string_view                  system
    , const std::optional<std::string_view> & stability = std::nullopt
    ,       bool                              trace     = false
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
      if ( this->_stability.has_value() ) { return this->_stability.value(); }
      else                                { return std::nullopt;             }
    }

      FloxFlakeRef
    getRef() const override {
      return this->_flake->flake.lockedRef;
    }


/* -------------------------------------------------------------------------- */

    bool        hasRelPath( const std::list<std::string_view> & path ) override;
    std::size_t size() override;


/* -------------------------------------------------------------------------- */

    struct const_iterator
    {
      using value_type = const FlakePackage;
      using reference  = value_type &;
      using pointer    = nix::ref<value_type>;

      private:
        subtree_type                             _subtree = ST_NONE;
        todo_queue                               _todo;
        std::vector<nix::Symbol>::const_iterator _end;
        std::vector<nix::Symbol>::iterator       _it;

      public:
        explicit const_iterator( subtree_type subtree, todo_queue todo )
          : _subtree( subtree ), _todo( todo )
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
        const_iterator( subtree_type subtree = ST_NONE )
          : const_iterator( subtree, todo_queue() )
        {}

        std::string_view getType() const { return "flake"; }

        const_iterator & operator++();

          const_iterator
        operator++( int )
        {
          const_iterator tmp = * this;
          ++( * this );
          return tmp;
        }

          bool
        operator==( const const_iterator & other ) const
        {
          return this->_it == other._it;
        }

          bool
        operator!=( const const_iterator & other ) const
        {
          return ! ( ( * this ) == other );
        }

    };  /* End struct `FlakePackageSet::const_iterator' */


/* -------------------------------------------------------------------------- */

    const_iterator begin();
    const_iterator end()   const;


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
