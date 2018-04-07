// ======================================================================
/*!
 * \brief The PostGIS shape readed options
 */
// ======================================================================

#pragma once

#include <boost/optional.hpp>
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
  boost::optional<std::string> where;
  boost::optional<double> minarea;
  boost::optional<double> mindistance;
};

}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet
