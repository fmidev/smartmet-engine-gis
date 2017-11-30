// ======================================================================

#pragma once

#include "BBox.h"
#include "CRSRegistry.h"
#include <boost/utility.hpp>
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

class Config : private boost::noncopyable
{
 public:
  Config() = delete;
  Config(const std::string& theFileName);

  // return the CRS registry
  CRSRegistry& getCRSRegistry();

  const postgis_connection_info& getPostGISConnectionInfo(const std::string thePGName) const;

  int getMaxCacheSize() const { return itsMaxCacheSize; }
  BBox getBBox(int theEPSG) const;

 private:
  libconfig::Config itsConfig;
  CRSRegistry itsCRSRegistry;

  // PostGIS settings
  postgis_connection_info itsDefaultConnectionInfo;
  std::map<std::string, postgis_connection_info> itsConnectionInfo;

  // cache settings
  int itsMaxCacheSize;

  // EPSG bounding boxes
  typedef std::map<int, BBox> BBoxMap;
  BBoxMap itsBBoxes;
};

}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet
