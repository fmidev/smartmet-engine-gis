#include "GdalUtils.h"
#include <spine/Exception.h>

namespace SmartMet
{
namespace Engine
{
namespace Gis
{
boost::shared_ptr<OGRPolygon> bbox2polygon(const NFmiRect &rect)
{
  try
  {
    OGRLinearRing boundary;
    boundary.addPoint(rect.Left(), rect.Bottom());
    boundary.addPoint(rect.Left(), rect.Top());
    boundary.addPoint(rect.Right(), rect.Top());
    boundary.addPoint(rect.Right(), rect.Bottom());
    boundary.closeRings();

    boost::shared_ptr<OGRPolygon> result(new OGRPolygon);
    result->addRing(&boundary);

    return result;
  }
  catch (...)
  {
    throw Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

boost::shared_ptr<OGRPolygon> bbox2polygon(double xmin, double ymin, double xmax, double ymax)
{
  try
  {
    OGRLinearRing boundary;
    boundary.addPoint(xmin, ymin);
    boundary.addPoint(xmin, ymax);
    boundary.addPoint(xmax, ymax);
    boundary.addPoint(xmax, xmin);
    boundary.closeRings();

    boost::shared_ptr<OGRPolygon> result(new OGRPolygon);
    result->addRing(&boundary);

    return result;
  }
  catch (...)
  {
    throw Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

boost::shared_ptr<OGRPolygon> bbox2polygon(
    double xmin, double ymin, double xmax, double ymax, int num_side_points)
{
  try
  {
    double xstep = (xmax - xmin) / (num_side_points - 1);
    double ystep = (ymax - ymin) / (num_side_points - 1);

    OGRLinearRing boundary;

    for (int i = 0; i < num_side_points; i++)
      boundary.addPoint(xmin, ymin + i * ystep);

    for (int i = 1; i < num_side_points; i++)
      boundary.addPoint(xmin + i * xstep, ymax);

    for (int i = 1; i < num_side_points; i++)
      boundary.addPoint(xmax, ymax - i * ystep);

    for (int i = 1; i < num_side_points; i++)
      boundary.addPoint(xmax - i * xstep, ymin);

    boundary.closeRings();

    boost::shared_ptr<OGRPolygon> result(new OGRPolygon);
    result->addRing(&boundary);

    return result;
  }
  catch (...)
  {
    throw Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

std::string WKT(const OGRGeometry &geometry)
{
  try
  {
    char *wkt = nullptr;
    geometry.exportToWkt(&wkt);
    std::string result = wkt;
    CPLFree(wkt);
    return result;
  }
  catch (...)
  {
    throw Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet
