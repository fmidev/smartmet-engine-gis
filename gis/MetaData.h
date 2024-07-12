#pragma once

#include <macgyver/DateTime.h>
#include <optional>
#include <vector>

namespace SmartMet
{
namespace Engine
{
namespace Gis
{
// ----------------------------------------------------------------------
/*!
 *   GIS database metadata
 *
 */
// ----------------------------------------------------------------------

struct MetaDataQueryOptions
{
  std::string pgname;
  std::string schema;
  std::string table;
  std::string geometry_column;
  std::optional<std::string> time_column;
  std::size_t hash_value() const;
};

struct TimeInterval
{
  Fmi::DateTime starttime;
  Fmi::DateTime endtime;
  Fmi::TimeDuration timestep;
};

struct MetaData
{
  // list of steps or fixed steps
  std::vector<Fmi::DateTime> timesteps;
  std::optional<TimeInterval> timeinterval;

  // bounding box of geometries
  double xmin = 0;
  double ymin = 0;
  double xmax = 0;
  double ymax = 0;

  MetaData() = default;
};

}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet
