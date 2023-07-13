/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <optional>
#include <iterator>
#include <string>
#include <vector>
#include <functional>
#include <nix/eval-cache.hh>
#include <list>
#include <queue>
#include "flox/types.hh"
#include <ranges>


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

namespace detail {

/* -------------------------------------------------------------------------- */

struct cursor_iterator_closure {

  using MaybeCursor = resolve::MaybeCursor;
  using Cursor      = resolve::Cursor;
  using Symbol      = nix::Symbol;
  using State       = std::shared_ptr<nix::EvalState>;

  static const std::vector<Symbol>     empty_path;
  static const cursor_iterator_closure empty_cl;

  // Enable if you `recurseIntoAttrs' later
  //const struct {} sentinel = {};

  const std::vector<nix::Symbol> & _attrs =
    cursor_iterator_closure::empty_path;

  State       _state  = nullptr;
  MaybeCursor _parent = nullptr;


  cursor_iterator_closure() = default;

  cursor_iterator_closure( State state, Cursor parent )
    : _state( state ), _parent( parent ), _attrs( parent->getAttrs() )
  {}

  using iterator_t        = std::ranges::iterator_t<std::vector<Symbol>>;
  using range_reference_t = std::ranges::range_reference_t<std::vector<Symbol>>;

  struct iterator : iterator_t {

    using base      = iterator_t;
    using reference = std::pair<Symbol, Cursor>;

    iterator() = default;

  };


  constexpr auto begin()        { return this->_attrs.begin();  }
  constexpr auto begin()  const { return this->_attrs.begin();  }
  constexpr auto cbegin() const { return this->_attrs.cbegin(); }
  constexpr auto end()          { return this->_attrs.end();    }
  constexpr auto end()    const { return this->_attrs.end();    }
  constexpr auto cend()   const { return this->_attrs.cend();   }
  constexpr auto size()   const { return this->_attrs.size();   }
  constexpr auto empty()  const { return this->_attrs.empty();  }

};  /* End class `CursorIteratorClosure' */


const std::vector<nix::Symbol> cursor_iterator_closure::empty_path = {};
const cursor_iterator_closure  cursor_iterator_closure::empty_cl   = {};

static_assert( std::ranges::forward_range<cursor_iterator_closure> );


/* -------------------------------------------------------------------------- */

}  /* End namespace `flox::detail' */


/* -------------------------------------------------------------------------- */

}  /* End Namespace `flox' */

/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
