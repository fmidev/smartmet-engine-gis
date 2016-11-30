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
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
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
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
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
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

std::string WKT(const OGRGeometry &geometry)
{
  try
  {
    char *wkt = NULL;
    geometry.exportToWkt(&wkt);
    std::string result = wkt;
    OGRFree(wkt);
    return result;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

GeometryConv::GeometryConv(boost::function1<NFmiPoint, NFmiPoint> theConv) : conv(theConv)
{
}

GeometryConv::~GeometryConv()
{
}

// BUG?? nCount is unused
int GeometryConv::Transform(int /* nCount */, double *x, double *y, double *z)
{
  try
  {
    NFmiPoint src(*x, *y);
    NFmiPoint dest = conv(src);
    *x = dest.X();
    *y = dest.Y();
    if (z)
      *z = 0.0;
    return TRUE;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

int GeometryConv::TransformEx(int nCount, double *x, double *y, double *z, int *pabSuccess)
{
  try
  {
    for (int i = 0; i < nCount; i++)
    {
      NFmiPoint src(x[i], y[i]);
      NFmiPoint dest = conv(src);
      x[i] = dest.X();
      y[i] = dest.Y();
      if (z)
        z[i] = 0.0;
      pabSuccess[i] = TRUE;
    }
    return TRUE;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet
