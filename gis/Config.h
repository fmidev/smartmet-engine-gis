// ======================================================================

#pragma once

#include "BBox.h"
#include "CRSRegistry.h"
#include <libconfig.h++>
#include <boost/utility.hpp>
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
  int itsPort;
  std::string itsHost;
  std::string itsDatabase;
  std::string itsUsername;
  std::string itsPassword;
  std::string itsEncoding;
};

class Config : private boost::noncopyable
{
 public:
  Config() = delete;
  Config(const std::string& theFileName);

  // return the CRS registry
  CRSRegistry& getCRSRegistry();

  int getPort() const { return itsPort; }
  const std::string& getHost() const { return itsHost; }
  const std::string& getDatabase() const { return itsDatabase; }
  const std::string& getUsername() const { return itsUsername; }
  const std::string& getPassword() const { return itsPassword; }
  const std::string& getEncoding() const { return itsEncoding; }
  const postgis_connection_info& getPostGISConnectionInfo(const std::string thePGName) const;

  int getMaxCacheSize() const { return itsMaxCacheSize; }
  BBox getBBox(int theEPSG) const;

 private:
  libconfig::Config itsConfig;
  CRSRegistry itsCRSRegistry;

  // postgis settings
  int itsPort;
  std::string itsHost;
  std::string itsDatabase;
  std::string itsUsername;
  std::string itsPassword;
  std::string itsEncoding;

  std::map<std::string, postgis_connection_info> postgis_settings;

  // cache settings
  int itsMaxCacheSize;

  // EPSG bounding boxes
  typedef std::map<int, BBox> BBoxMap;
  BBoxMap itsBBoxes;
};

}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet
