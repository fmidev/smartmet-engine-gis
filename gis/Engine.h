// ======================================================================

#pragma once

#include "BBox.h"
#include "Config.h"
#include "EPSG.h"
#include "GeometryStorage.h"
#include "MapOptions.h"
#include "MetaData.h"
#include <boost/shared_ptr.hpp>
#include <gis/SpatialReference.h>
#include <gis/Types.h>
#include <macgyver/Cache.h>
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
class CRSRegistry;

class Engine : public SmartMet::Spine::SmartMetEngine
{
 public:
  // constructor is available only with a libconfig configuration file

  Engine(const std::string& theFileName);

  // return the CRS registry
  CRSRegistry& getCRSRegistry();

  // fetch a shape

  OGRGeometryPtr getShape(const Fmi::SpatialReference& theSR, const MapOptions& theOptions) const;

  Fmi::Features getFeatures(const Fmi::SpatialReference& theSR, const MapOptions& theOptions) const;

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
  typedef Fmi::Cache::Cache<std::string, OGRGeometryPtr> GeometryCache;
  mutable GeometryCache itsCache;

  // cache for geometries with attributes
  typedef Fmi::Cache::Cache<std::string, Fmi::Features> FeaturesCache;
  mutable FeaturesCache itsFeaturesCache;

  // cache for envelopes
  typedef Fmi::Cache::Cache<std::size_t, OGREnvelope> EnvelopeCache;
  mutable EnvelopeCache itsEnvelopeCache;

};  // class Engine

}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet

// ======================================================================
