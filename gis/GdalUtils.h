#pragma once

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <newbase/NFmiPoint.h>
#include <newbase/NFmiRect.h>
#include <gdal_version.h>
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

class GeometryConv : public OGRCoordinateTransformation
{
 public:
  GeometryConv(boost::function1<NFmiPoint, NFmiPoint> conv);

  virtual ~GeometryConv();

#if GDAL_VERSION_MAJOR < 3
  virtual int Transform(int nCount, double *x, double *y, double *z = nullptr);

  virtual int TransformEx(
      int nCount, double *x, double *y, double *z = nullptr, int *pabSuccess = nullptr);
#else
  int Transform(int nCount, double *x, double *y, double *z, double *t, int *pabSuccess) override;

#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR  == 3 && GDAL_VERSION_MINOR >= 1)
  OGRCoordinateTransformation *Clone() const override;
#endif

#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR  == 3 && GDAL_VERSION_MINOR >= 3)
  int TransformWithErrorCodes(
      int nCount, double *x, double *y, double *z, double *t, int *panErrorCodes) override;

    OGRCoordinateTransformation* GetInverse() const override;
#endif

#endif

 private:
  OGRSpatialReference *GetSourceCS() override { return nullptr; }
  OGRSpatialReference *GetTargetCS() override { return nullptr; }

  boost::function1<NFmiPoint, NFmiPoint> conv;
};

}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet
