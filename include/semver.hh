/* ========================================================================== *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <string>
#include <regex>
#include <optional>
#include <list>
#include <nix/util.hh>


/* -------------------------------------------------------------------------- */

bool isSemver( const std::string & version );
bool isSemver( std::string_view version );

bool isDate( const std::string & version );
bool isDate( std::string_view version );

bool isCoercibleToSemver( const std::string & version );
bool isCoercibleToSemver( std::string_view version );

/* -------------------------------------------------------------------------- */

std::optional<std::string> coerceSemver( std::string_view version );


/* -------------------------------------------------------------------------- */

/* Pair of error-code and output string. */
std::pair<int, std::string> runSemver( const std::list<std::string> & args );

std::list<std::string> semverSat( const std::string            & range
                                , const std::list<std::string> & versions
                                );


/* -------------------------------------------------------------------------- *
 *
 *
 * ========================================================================== */
