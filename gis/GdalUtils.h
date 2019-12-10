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

// class GeometryConv : public OGRCoordinateTransformation
class GeometryConv
{
 public:
  GeometryConv(boost::function1<NFmiPoint, NFmiPoint> conv);

  virtual ~GeometryConv();

  virtual int Transform(int nCount, double *x, double *y, double *z = nullptr);

  virtual int TransformEx(
      int nCount, double *x, double *y, double *z = nullptr, int *pabSuccess = nullptr);

 private:
  virtual OGRSpatialReference *GetSourceCS() { return nullptr; }
  virtual OGRSpatialReference *GetTargetCS() { return nullptr; }

 private:
  boost::function1<NFmiPoint, NFmiPoint> conv;
};

}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet
