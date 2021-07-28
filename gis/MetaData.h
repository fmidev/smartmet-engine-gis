#pragma once

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/optional.hpp>
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
  boost::optional<std::string> time_column;
  std::size_t hash_value() const;
};

struct TimeInterval
{
  boost::posix_time::ptime starttime;
  boost::posix_time::ptime endtime;
  boost::posix_time::time_duration timestep;
};

struct MetaData
{
  // list of steps or fixed steps
  std::vector<boost::posix_time::ptime> timesteps;
  boost::optional<TimeInterval> timeinterval;

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
