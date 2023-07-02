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

      MaybeCursor
    openCursor() const
    {
      MaybeCursor curr = this->openEvalCache()->getRoot();
      if ( this->_subtree == ST_PACKAGES )
        {
          curr = curr->maybeGetAttr( "packages" );
          if ( curr == nullptr ) { return nullptr; }
          curr = curr->maybeGetAttr( this->_system );
        }
      else
        {
          curr = curr->maybeGetAttr( subtreeTypeToString( this->_subtree ) );
          if ( curr == nullptr ) { return nullptr; }
          curr = curr->maybeGetAttr( this->_system );
          if ( this->_stability.has_value() )
            {
              if ( curr == nullptr ) { return nullptr; }
              curr = curr->maybeGetAttr( this->_stability.value() );
            }
        }
      return curr;
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
        nix::SymbolTable                         * _symtab;
        subtree_type                               _subtree;
        todo_queue                                 _todo;
        std::vector<nix::Symbol>::const_iterator   _end;
        std::vector<nix::Symbol>::iterator         _it;
        std::shared_ptr<FlakePackage>              _ptr;

        /**
         * Evaluate a package at the current cursor position.
         * Return true/false if the current cursor position allowed a package
         * to be evaluated.
         * When the return value is `false', `_ptr' is set to `nullptr', and
         * the iterator likely needs to "seek" the next package
         * using `++( * this )'.
         */
        bool evalPackage();

        /* Clear iterator fields. Also used as a sentinel value. */
          const_iterator &
        clear()
        {
          this->_subtree = ST_NONE;
          this->_symtab  = nullptr;
          this->_end     = std::vector<nix::Symbol>().cend();
          this->_it      = std::vector<nix::Symbol>().begin();
          this->_ptr     = nullptr;
          return * this;
        }

      public:
        explicit const_iterator( subtree_type       subtree
                               , nix::SymbolTable * symtab
                               , todo_queue         todo
                               )
          : _subtree( subtree ), _todo( todo ), _symtab( symtab )
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
              /* Try loading a package from the current position.
               * On failure seek until we find a package attribute. */
              if ( ! this->evalPackage() ) { ++( * this ); }
            }
        }

        const_iterator( subtree_type subtree = ST_NONE )
          : _symtab( nullptr )
          , _subtree( subtree )
          , _todo( todo_queue() )
          , _ptr( nullptr )
        {
          _end = std::vector<nix::Symbol>().cend();
          _it  = std::vector<nix::Symbol>().begin();
        }

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
          return this->_it != other._it;
        }

        reference operator*() { return * this->_ptr;                      }
        pointer  operator->() { return (nix::ref<value_type>) this->_ptr; }

    };  /* End struct `FlakePackageSet::const_iterator' */


/* -------------------------------------------------------------------------- */

    const_iterator begin();
    const_iterator end()   const { return const_iterator(); }


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
