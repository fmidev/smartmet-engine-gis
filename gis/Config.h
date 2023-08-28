// ======================================================================

#pragma once

#include <boost/optional.hpp>
#include <gis/BBox.h>
#include <spine/CRSRegistry.h>
#include <libconfig.h++>
#include <string>

namespace SmartMet
{
namespace Engine
{
namespace Gis
{
// postgis settings
struct postgis_connection_info
{
  int port;
  std::string host;
  std::string database;
  std::string username;
  std::string password;
  std::string encoding;
};

class Config
{
 public:
  ~Config() = default;
  explicit Config(std::string theFileName);

  Config() = delete;
  Config(const Config& other) = delete;
  Config& operator=(const Config& other) = delete;
  Config(Config&& other) = delete;
  Config& operator=(Config&& other) = delete;

  // return the CRS registry
  Spine::CRSRegistry& getCRSRegistry();

  const postgis_connection_info& getPostGISConnectionInfo(const std::string& thePGName) const;

  int getMaxCacheSize() const { return itsMaxCacheSize; }

  boost::optional<int> getDefaultEPSG() const;
  boost::optional<Fmi::BBox> getTableBBox(const std::string& theSchema,
                                          const std::string& theTable) const;
  boost::optional<boost::posix_time::time_duration> getTableTimeStep(
      const std::string& theSchema, const std::string& theTable) const;

  bool quiet() const;

 private:
  void read_crs_settings();
  void require_postgis_settings() const;
  void read_postgis_settings();
  void read_postgis_info();
  void read_cache_settings();
  void read_gdal_settings();
  void read_bbox_settings();
  Fmi::BBox read_bbox(const libconfig::Setting& theSetting) const;

  libconfig::Config itsConfig;
  std::string itsFileName;
  Spine::CRSRegistry itsCRSRegistry;

  // PostGIS settings
  postgis_connection_info itsDefaultConnectionInfo;
  std::map<std::string, postgis_connection_info> itsConnectionInfo;

  // cache settings
  int itsMaxCacheSize = 0;

  // Default EPSG for PostGIS geometries which have no SRID
  boost::optional<int> itsDefaultEPSG;

  // Quiet mode
  bool itsQuiet = true;

  // Precomputed/fixed PostGIS information
  using PostGisBBoxMap = std::map<std::string, Fmi::BBox>;
  PostGisBBoxMap itsPostGisBBoxMap;

  using PostGisTimeStepMap = std::map<std::string, boost::posix_time::time_duration>;
  PostGisTimeStepMap itsPostGisTimeStepMap;
};

}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet
