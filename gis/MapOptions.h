// ======================================================================
/*!
 * \brief The PostGIS shape readed options
 */
// ======================================================================

#pragma once

#include <gis/GeometryAmalgamator.h>
#include <gis/GeometrySimplifier.h>
#include <optional>
#include <set>
#include <string>

namespace SmartMet
{
namespace Engine
{
namespace Gis
{
struct MapOptions
{
  std::string pgname;
  std::string schema;
  std::string table;
  std::set<std::string> fieldnames;
  std::optional<std::string> where;
  std::optional<double> minarea;
  std::optional<double> mindistance;

  // Optional polygon amalgamator and simplifier. Both are inactive in their
  // default-constructed state and can be activated by setting the relevant
  // limits / type. Tolerance values in the simplifier are expected to be in
  // CRS coordinate units (the caller may use GeometrySimplifier::bbox() to
  // convert from pixel tolerances).
  Fmi::GeometryAmalgamator amalgamator;
  Fmi::GeometrySimplifier simplifier;
};

}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet
