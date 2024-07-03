// ======================================================================
/*!
 * \brief The PostGIS shape readed options
 */
// ======================================================================

#pragma once

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
};

}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet
