// ======================================================================

#pragma once

#include "BBox.h"
#include "Config.h"
#include "EPSG.h"
#include "GeometryStorage.h"
#include "MapOptions.h"
#include "MetaData.h"
#include <boost/shared_ptr.hpp>
#include <gdal/ogr_geometry.h>
#include <gdal/ogr_spatialref.h>
#include <gis/Types.h>
#include <macgyver/Cache.h>
#include <spine/CRSRegistry.h>
#include <spine/SmartMetEngine.h>
#include <libconfig.h++>
#include <string>

namespace SmartMet
{
namespace Engine
{
namespace Gis
{
using Spine::CRSRegistry;

class Engine : public SmartMet::Spine::SmartMetEngine
{
 public:
  // constructor is available only with a libconfig configuration file

  Engine(const std::string& theFileName);

  // return the CRS registry
  Spine::CRSRegistry& getCRSRegistry();

  // fetch a shape

  OGRGeometryPtr getShape(OGRSpatialReference* theSR, const MapOptions& theOptions) const;

  Fmi::Features getFeatures(OGRSpatialReference* theSR, const MapOptions& theOptions) const;

  std::shared_ptr<OGRSpatialReference> getSpatialReference(const std::string& theSR) const;

  BBox getBBox(int theEPSG) const;
  boost::optional<EPSG> getEPSG(int theEPSG) const;

  MetaData getMetaData(const MetaDataQueryOptions& theOptions) const;

  void populateGeometryStorage(const PostGISIdentifierVector& thePostGISIdentifiers,
                               GeometryStorage& theGeometryStorage) const;

 protected:
  virtual void init();
  void shutdown();

 private:
  Engine();

  OGREnvelope getTableEnvelope(const GDALDataPtr& connection,
                               const std::string& schema,
                               const std::string& table,
                               const std::string& geometry_column,
                               bool quiet) const;

  std::string itsConfigFile;
  std::unique_ptr<Config> itsConfig;  // ptr for delayed initialization

  // Cached contents
  using GeometryCache = Fmi::Cache::Cache<std::string, OGRGeometryPtr>;
  mutable GeometryCache itsCache;

  // cache for geometries with attributes
  using FeaturesCache = Fmi::Cache::Cache<std::string, Fmi::Features>;
  mutable FeaturesCache itsFeaturesCache;

  // cache for envelopes
  using EnvelopeCache = Fmi::Cache::Cache<std::size_t, OGREnvelope>;
  mutable EnvelopeCache itsEnvelopeCache;

  // cache for spatial references
  using SpatialReferenceCache =
      Fmi::Cache::Cache<std::string, std::shared_ptr<OGRSpatialReference>>;
  mutable SpatialReferenceCache itsSpatialReferenceCache;

};  // class Engine

}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet

// ======================================================================
