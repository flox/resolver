/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <string>
#include "flox/package.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

    struct AttrPathGlob;

    namespace predicates {

/* -------------------------------------------------------------------------- */

struct PkgPred {
  using pred_fn = std::function<bool( const Package & )>;
  pred_fn pred;

  PkgPred() : pred( []( const Package & ) { return true; } ) {}
  PkgPred( const pred_fn & f ) : pred( f ) {}
  PkgPred( const PkgPred & p ) : pred( p.pred ) {}

  PkgPred( PkgPred && p ) : pred( std::move( p.pred ) ) {}

    PkgPred &
  operator=( const PkgPred & other )
  {
    this->pred = other.pred;
    return * this;
  }

    PkgPred &
  operator=( PkgPred && other )
  {
    this->pred = std::move( other.pred );
    return * this;
  }

    PkgPred
  operator&&( const PkgPred & other ) const
  {
    pred_fn t = this->pred;
    pred_fn o = other.pred;
    return PkgPred( [t, o]( const Package & p )
    {
      return t( p ) && o( p );
    } );
  }

    PkgPred
  operator||( const PkgPred & other ) const
  {
    pred_fn t = this->pred;
    pred_fn o = other.pred;
    return PkgPred( [t, o]( const Package & p )
    {
      return t( p ) || o( p );
    } );
  }

    PkgPred
  operator!()
  {
    pred_fn t = this->pred;
    return PkgPred( [t]( const Package & p )
    {
      return ! t( p );
    } );
  }

  bool operator()( const Package & p ) const { return this->pred( p ); }
};


/* -------------------------------------------------------------------------- */

PkgPred hasName(          const std::string              & name      );
PkgPred hasFullName(      const std::string              & name      );
PkgPred hasPname(         const std::string              & name      );
PkgPred hasPkgAttrName(   const std::string              & name      );
PkgPred hasVersion(       const std::string              & version   );
PkgPred satisfiesSemver(  const std::string              & range     );
PkgPred hasLicense(       const std::string              & license   );
PkgPred hasLicense(       const std::vector<std::string> & licenses  );
PkgPred hasSubtree(       const std::string              & subtree   );
PkgPred hasSubtree(             subtree_type               subtree   );
PkgPred hasStability(     const std::string              & stability );
PkgPred hasOutput(        const std::string              & output    );
PkgPred hasOutputs(       const std::vector<std::string> & outputs   );
PkgPred depthLE(                size_t                     max       );
PkgPred hasUnfree(              bool                       value     );
PkgPred hasBroken(              bool                       value     );
PkgPred hasRelPathPrefix( const std::vector<std::string> & prefix    );
PkgPred hasAbsPathPrefix( const std::vector<std::string> & prefix    );
PkgPred hasAbsPathPrefix( const AttrPathGlob             & prefix    );


/* -------------------------------------------------------------------------- */

static const PkgPred predTrue =
  PkgPred( []( const Package & ) { return true; } );

static const PkgPred predFalse =
  PkgPred( []( const Package & ) { return false; } );

static const PkgPred hasMeta =
  PkgPred( []( const Package & p ) { return p.hasMetaAttr(); } );

static const PkgPred isFree = PkgPred( []( const Package & p ) {
  return ! p.isUnfree().value_or( false );
} );

static const PkgPred notBroken = PkgPred( []( const Package & p ) {
  return ! p.isBroken().value_or( false );
} );


/* -------------------------------------------------------------------------- */

    }  /* End Namespace `flox::resolve::predicates' */
  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
