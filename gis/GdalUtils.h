#pragma once

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <newbase/NFmiPoint.h>
#include <newbase/NFmiRect.h>
#include <limits>
#include <ogr_geometry.h>
#include <ogr_spatialref.h>

namespace SmartMet
{
namespace Engine
{
namespace Gis
{
boost::shared_ptr<OGRPolygon> bbox2polygon(const NFmiRect &rect);

boost::shared_ptr<OGRPolygon> bbox2polygon(double xmin, double ymin, double xmax, double ymax);

boost::shared_ptr<OGRPolygon> bbox2polygon(
    double xmin, double ymin, double xmax, double ymax, int num_side_points = 10);

std::string WKT(const OGRGeometry &geometry);

}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet
