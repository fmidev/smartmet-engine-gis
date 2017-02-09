#pragma once

#include <limits>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <gdal/ogr_geometry.h>
#include <gdal/ogr_spatialref.h>
#include <newbase/NFmiRect.h>
#include <newbase/NFmiPoint.h>

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

  virtual int Transform(int nCount, double *x, double *y, double *z = NULL);

  virtual int TransformEx(
      int nCount, double *x, double *y, double *z = NULL, int *pabSuccess = NULL);

 private:
  virtual OGRSpatialReference *GetSourceCS() { return NULL; }
  virtual OGRSpatialReference *GetTargetCS() { return NULL; }
 private:
  boost::function1<NFmiPoint, NFmiPoint> conv;
};

}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet
