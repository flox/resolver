/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <cstddef>
#include <string>
#include <cstring>
#include <vector>
#include <nlohmann/json.hpp>
#include <ranges>


/* -------------------------------------------------------------------------- */

namespace flox {


/* -------------------------------------------------------------------------- */

struct AttrPath {

  private:
    static const std::array<std::string> _common;
    std::vector<std::string>             _data    = {};
    std::vector<const char *>            _path    = {};

  public:
    AttrPath() = default;

      template<std::ranges::input_range R>
    AttrPath( const R<std::string_view> & path )
    {
      size_t i = 0;
      for ( auto p = std::begin( path ); p != std::end( path ); ++p, ++i )
        {
          if ( ( * p ) == std::string_view( "{{system}}" ) )
            {
              _path[i] = nullptr;
            }
          else if ( auto c = std::find( AttrPath::_common, * p );
                    c != std::end( AttrPath::_common )
                  )
            {
              _path[i] = c->data();
            }
          else
            {
              _data.push_back( * p );
              _path[i] = _data.back().data();
            }
        }
    }

};  /* End class `AttrPath' */

static const std::array<std::string> AttrPath::_common = {
  "packages", "legacyPackages", "catalog", "evalCatalog"
, "x86_64-linux", "aarch64-linux", "x86_64-darwin", "aarch64-darwin"
, "stable", "staging", "unstable"
// TODO: extend with other common package sets.
, "nodePackages", "pythonPackages", "haskellPackages", "xorg"
};


/* -------------------------------------------------------------------------- */

}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
