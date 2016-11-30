#pragma once

#include <boost/date_time/posix_time/posix_time.hpp>

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
};

struct MetaData
{
  // timesteps
  std::vector<boost::posix_time::ptime> timesteps;

  // bounding box of geometries
  double xmin;
  double ymin;
  double xmax;
  double ymax;

  MetaData() : xmin(0.0), ymin(0.0), xmax(0.0), ymax(0.0) {}
};

}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet
