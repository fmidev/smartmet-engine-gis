#include "GdalUtils.h"
#include <macgyver/Exception.h>
#include <gdal_version.h>

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
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
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
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
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
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

std::string WKT(const OGRGeometry &geometry)
{
  try
  {
    char *wkt = nullptr;
    geometry.exportToWkt(&wkt);
    std::string result = wkt;
#if GDAL_VERSION_MAJOR < 2
    OGRFree(wkt);
#else
    CPLFree(wkt);
#endif
    return result;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

GeometryConv::GeometryConv(boost::function1<NFmiPoint, NFmiPoint> theConv) : conv(theConv) {}

GeometryConv::~GeometryConv() = default;

#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)
OGRCoordinateTransformation *GeometryConv::Clone() const
{
  throw Fmi::Exception(BCP, "Attempt to clone GeometryConv");
}
#endif

#if GDAL_VERSION_MAJOR < 3
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
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}
#endif

#if GDAL_VERSION_MAJOR >= 3
int GeometryConv::Transform(int nCount, double *x, double *y, double *z, double *t, int *pabSuccess)
{
  try
  {
    (void)t;
    for (int i = 0; i < nCount; i++)
    {
      NFmiPoint src(x[i], y[i]);
      NFmiPoint dest = conv(src);
      x[i] = dest.X();
      y[i] = dest.Y();
      if (z)
        z[i] = 0.0;
      if (pabSuccess != nullptr)
        pabSuccess[i] = TRUE;
    }
    return TRUE;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}
#endif

#if GDAL_VERSION_MAJOR < 3
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
      if (pabSuccess != nullptr)
        pabSuccess[i] = TRUE;
    }
    return TRUE;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}
#endif

#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR  == 3 && GDAL_VERSION_MINOR >= 3)
int GeometryConv::TransformWithErrorCodes(
    int, double *, double *, double *, double *, int *)
{
  throw Fmi::Exception(BCP, "Attempt to call GeometryConv::TransformWithErrorCodes");
}

OGRCoordinateTransformation* GeometryConv::GetInverse() const
{
  throw Fmi::Exception(BCP, "Attempt to call GeometryConv::GetInverse");
}
#endif


}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet
