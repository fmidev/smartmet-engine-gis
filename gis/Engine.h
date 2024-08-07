// ======================================================================

#pragma once

#include "Config.h"
#include "GeometryStorage.h"
#include "MapOptions.h"
#include "MetaData.h"
#include <memory>
#include <gis/SpatialReference.h>
#include <gis/Types.h>
#include <macgyver/Cache.h>
#include <spine/CRSRegistry.h>
#include <spine/SmartMetEngine.h>
#include <libconfig.h++>
#include <ogr_geometry.h>
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

  Engine() = delete;
  explicit Engine(std::string theFileName);

  // return the CRS registry
  Spine::CRSRegistry& getCRSRegistry();

  // fetch a shape

  OGRGeometryPtr getShape(const Fmi::SpatialReference* theSR, const MapOptions& theOptions) const;

  Fmi::Features getFeatures(const MapOptions& theOptions) const;
  Fmi::Features getFeatures(const Fmi::SpatialReference& theSR, const MapOptions& theOptions) const;

  MetaData getMetaData(const MetaDataQueryOptions& theOptions) const;

  void populateGeometryStorage(const PostGISIdentifierVector& thePostGISIdentifiers,
                               GeometryStorage& theGeometryStorage) const;

 protected:
  void init() override;
  void shutdown() override;

 private:
  Fmi::Features getFeatures(const Fmi::SpatialReference* theSR, const MapOptions& theOptions) const;

  OGREnvelope getTableEnvelope(const GDALDataPtr& connection,
                               const std::string& schema,
                               const std::string& table,
                               const std::string& geometry_column,
                               bool quiet) const;

  Fmi::Cache::CacheStatistics getCacheStats() const override;

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

};  // class Engine

}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet

// ======================================================================
