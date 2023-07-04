/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include "flox/types.hh"
#include "flox/cached-package-set.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

  bool
CachedPackageSet::hasRelPath( const std::list<std::string_view> & path )
{
  if ( this->_populateDb ) { return this->_fps->hasRelPath( path );  }
  else                     { return this->_dbps->hasRelPath( path ); }
}


/* -------------------------------------------------------------------------- */

  std::shared_ptr<Package>
CachedPackageSet::maybeGetRelPath( const std::list<std::string_view> & path )
{
  if ( this->_populateDb ) { return this->_fps->maybeGetRelPath( path );  }
  else                     { return this->_dbps->maybeGetRelPath( path ); }
}


/* -------------------------------------------------------------------------- */

  std::size_t
CachedPackageSet::size()
{
  if ( this->_populateDb ) { return this->_fps->size();  }
  else                     { return this->_dbps->size(); }
}


/* -------------------------------------------------------------------------- */

  CachedPackageSet::const_iterator
CachedPackageSet::begin() const
{
  if ( this->_populateDb )
    {
      return const_iterator(
        this->_populateDb
      , this->_fps
      , nullptr
      , this->_db
      );
    }
  else
    {
      return const_iterator( this->_populateDb, nullptr, this->_dbps, nullptr );
    }
}


/* -------------------------------------------------------------------------- */

  CachedPackageSet::const_iterator &
CachedPackageSet::const_iterator::operator++()
{
  const Package * p = nullptr;
  if ( this->_populateDb )
    {
      ++( * this->_fi );
      p = this->_fi->operator->().get_ptr().get();
    }
  else
    {
      ++( * this->_di );
      p = this->_di->operator->().get_ptr().get();
    }

  std::vector<std::string_view> pathS;
  for ( const auto & s : p->getPathStrs() ) { pathS.push_back( s ); }

  std::vector<std::string_view> outputs;
  for ( const auto & s : p->getOutputs() ) { outputs.push_back( s ); }

  std::vector<std::string_view> outputsToInstall;
  for ( const auto & s : p->getOutputsToInstall() )
    {
      outputsToInstall.push_back( s );
    }

  this->_ptr = std::make_shared<RawPackage>(
    pathS
  , p->getFullName()
  , p->getPname()
  , p->getVersion()
  , p->getSemver()
  , p->getLicense()
  , outputs
  , outputsToInstall
  , p->isBroken()
  , p->isUnfree()
  , p->hasMetaAttr()
  , p->hasPnameAttr()
  , p->hasVersionAttr()
  );

  if ( this->_populateDb ) { this->_db->setDrvInfo( * this->_ptr ); }

  return * this;
}  /* End `CachedPackageSet::const_iterator::operator++()' */


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
