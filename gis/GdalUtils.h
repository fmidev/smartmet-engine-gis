#pragma once

#include <boost/function.hpp>
#include <memory>
#include <newbase/NFmiPoint.h>
#include <newbase/NFmiRect.h>
#include <gdal_version.h>
#include <limits>
#include <ogr_geometry.h>
#include <ogr_spatialref.h>

#undef GDAL_VERSION_ID
#define GDAL_VERSION_ID (100*GDAL_VERSION_MAJOR + GDAL_VERSION_MINOR)

namespace SmartMet
{
namespace Engine
{
namespace Gis
{
std::shared_ptr<OGRPolygon> bbox2polygon(const NFmiRect &rect);

std::shared_ptr<OGRPolygon> bbox2polygon(double xmin, double ymin, double xmax, double ymax);

std::shared_ptr<OGRPolygon> bbox2polygon(
    double xmin, double ymin, double xmax, double ymax, int num_side_points = 10);

std::string WKT(const OGRGeometry &geometry);

class GeometryConv : public OGRCoordinateTransformation
{
 public:
  explicit GeometryConv(boost::function1<NFmiPoint, NFmiPoint> conv);

  ~GeometryConv() override;

#if GDAL_VERSION_MAJOR < 3
  virtual int Transform(int nCount, double *x, double *y, double *z = nullptr);

  virtual int TransformEx(
      int nCount, double *x, double *y, double *z = nullptr, int *pabSuccess = nullptr);
#else
  int Transform(int nCount, double *x, double *y, double *z, double *t, int *pabSuccess) override;

#if GDAL_VERSION_ID >= 301
  OGRCoordinateTransformation *Clone() const override;
#endif

#if GDAL_VERSION_ID >= 303
  int TransformWithErrorCodes(
      int nCount, double *x, double *y, double *z, double *t, int *panErrorCodes) override;

  OGRCoordinateTransformation *GetInverse() const override;
#endif

#endif

 private:
#if GDAL_VERSION_ID >= 307
  const OGRSpatialReference *GetSourceCS() const override { return nullptr; }
  const OGRSpatialReference *GetTargetCS() const override { return nullptr; }
#else
  OGRSpatialReference *GetSourceCS() override { return nullptr; }
  OGRSpatialReference *GetTargetCS() override { return nullptr; }
#endif

  boost::function1<NFmiPoint, NFmiPoint> conv;
};

}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet
