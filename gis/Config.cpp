// ======================================================================

#include "Config.h"
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <macgyver/Exception.h>
#include <macgyver/TimeParser.h>
#include <spine/ConfigBase.h>
#include <cpl_conv.h>  // For configuring GDAL
#include <stdexcept>

namespace SmartMet
{
namespace Engine
{
namespace Gis
{
void Config::read_crs_settings()
{
  std::string crs_dir;
  itsConfig.lookupValue("crsDefinitionDir", crs_dir);
  if (!crs_dir.empty() && crs_dir[0] != '/')
  {
    // Handle relative paths assuming they are relative to the config itself
    boost::filesystem::path p(itsFileName);
    crs_dir = p.parent_path().string() + "/" + crs_dir;
  }
  itsCRSRegistry.read_crs_dir(crs_dir);
}

void Config::require_postgis_settings() const
{
  if (!itsConfig.exists("postgis.host"))
  {
    throw Fmi::Exception(BCP, "The 'postgis.host' attribute not set!")
        .addParameter("Configuration file", itsFileName);
  }

  if (!itsConfig.exists("postgis.port"))
  {
    throw Fmi::Exception(BCP, "The 'postgis.port' attribute not set!")
        .addParameter("Configuration file", itsFileName);
  }

  if (!itsConfig.exists("postgis.database"))
  {
    throw Fmi::Exception(BCP,
                         "The 'postgis.database' attribute not set!"
                         "")
        .addParameter("Configuration file", itsFileName);
  }

  if (!itsConfig.exists("postgis.username"))
  {
    throw Fmi::Exception(BCP, "The 'postgis.username' attribute not set!")
        .addParameter("Configuration file", itsFileName);
  }

  if (!itsConfig.exists("postgis.password"))
  {
    throw Fmi::Exception(BCP, "The 'postgis.password' attribute not set!")
        .addParameter("Configuration file", itsFileName);
  }

  if (!itsConfig.exists("postgis.encoding"))
  {
    throw Fmi::Exception(BCP, "The 'postgis.encoding' attribute not set!")
        .addParameter("Configuration file", itsFileName);
  }
}

void Config::read_postgis_settings()
{
  require_postgis_settings();

  itsConfig.lookupValue("postgis.host", itsDefaultConnectionInfo.host);
  itsConfig.lookupValue("postgis.port", itsDefaultConnectionInfo.port);
  itsConfig.lookupValue("postgis.database", itsDefaultConnectionInfo.database);
  itsConfig.lookupValue("postgis.username", itsDefaultConnectionInfo.username);
  itsConfig.lookupValue("postgis.password", itsDefaultConnectionInfo.password);
  itsConfig.lookupValue("postgis.encoding", itsDefaultConnectionInfo.encoding);

  libconfig::Setting& pg_sett = itsConfig.lookup("postgis");
  int n_pgsett = pg_sett.getLength();
  for (int i = 0; i < n_pgsett; i++)
  {
    libconfig::Setting& sett = pg_sett[i];

    if (sett.isGroup())
    {
      std::string sett_name(sett.getName());
      postgis_connection_info pgci;

      itsConfig.lookupValue("postgis." + sett_name + ".host", pgci.host);
      itsConfig.lookupValue("postgis." + sett_name + ".port", pgci.port);
      itsConfig.lookupValue("postgis." + sett_name + ".database", pgci.database);
      itsConfig.lookupValue("postgis." + sett_name + ".username", pgci.username);
      itsConfig.lookupValue("postgis." + sett_name + ".password", pgci.password);
      itsConfig.lookupValue("postgis." + sett_name + ".encoding", pgci.encoding);

      itsConnectionInfo.insert(make_pair(sett_name, pgci));
    }
  }
}

void Config::read_postgis_info()
{
  if (!itsConfig.exists("info"))
    return;

  const auto& info = itsConfig.lookup("info");
  if (!info.isGroup())
    throw Fmi::Exception(BCP, "info setting must be a group");

  for (int i = 0; i < info.getLength(); i++)
  {
    const auto& schema = info[i];

    if (!schema.isGroup())
      throw Fmi::Exception(BCP, "info settings must be groups");

    std::string schema_name = schema.getName();

    for (int j = 0; j < schema.getLength(); j++)
    {
      const auto& table = schema[j];
      if (!table.isGroup())
        throw Fmi::Exception(BCP, "info schema settings must be groups");

      std::string table_name = table.getName();

      if (table.exists("bbox"))
      {
        auto bbox = read_bbox(table["bbox"]);
        std::string key = schema_name + '.' + table_name;
        itsPostGisBBoxMap.insert(std::make_pair(key, bbox));
      }

      if (table.exists("timestep"))
      {
        auto timestep = Fmi::TimeParser::parse_iso_duration(table["timestep"]);
        std::string key = schema_name + '.' + table_name;
        itsPostGisTimeStepMap.insert(std::make_pair(key, timestep));
      }
    }
  }
}

void Config::read_cache_settings()
{
  if (!itsConfig.exists("cache.max_size"))
  {
    throw Fmi::Exception(BCP, "The 'cache.max_size' attribute not set!")
        .addParameter("Configuration file", itsFileName);
  }

  itsConfig.lookupValue("cache.max_size", itsMaxCacheSize);
}

void Config::read_gdal_settings()
{
  if (!itsConfig.exists("gdal"))
    return;

  const auto& settings = itsConfig.lookup("gdal");
  if (!settings.isGroup())
  {
    throw Fmi::Exception(BCP, "The 'gdal' parameter must be a group of GDAL settings!")
        .addParameter("Configuration file", itsFileName);
  }

  for (int i = 0; i < settings.getLength(); i++)
  {
    const auto& value = settings[i];
    const char* name = value.getName();
    CPLSetConfigOption(name, value);
  }
}

BBox Config::read_bbox(const libconfig::Setting& theSetting) const
{
  if (!theSetting.isArray())
  {
    throw Fmi::Exception(BCP, "The 'bbox' element must contain arrays!")
        .addParameter("Configuration file", itsFileName);
  }

  if (theSetting.getLength() != 4)
  {
    throw Fmi::Exception(BCP, "The 'bbox' elements must be arrays of size 4!")
        .addDetail("Found an array of length " + std::to_string(theSetting.getLength()))
        .addParameter("Configuration file", itsFileName);
  }

  double w = theSetting[0];
  double e = theSetting[1];
  double s = theSetting[2];
  double n = theSetting[3];
  return BBox{w, e, s, n};
}

EPSG Config::read_epsg(const libconfig::Setting& theSetting) const
{
  if (!theSetting.isGroup())
  {
    throw Fmi::Exception(BCP, "The 'epsg' elements must be groups!")
        .addParameter("Configuration file", itsFileName);
  }

  if (!theSetting.exists("id"))
    throw Fmi::Exception(BCP, "The 'epsg' elements must have an 'id' field");
  if (!theSetting.exists("bbox"))
    throw Fmi::Exception(BCP, "The 'epsg' elements must have a 'bbox' field");

  EPSG epsg;
  epsg.bbox = read_bbox(theSetting["bbox"]);
  theSetting.lookupValue("id", epsg.number);
  theSetting.lookupValue("name", epsg.name);
  theSetting.lookupValue("scope", epsg.scope);
  theSetting.lookupValue("source", epsg.source);
  theSetting.lookupValue("deprecated", epsg.deprecated);
  return epsg;
}

// TODO(mheiskan) Deprecated
void Config::read_bbox_settings()
{
  std::cerr << "Warning: gis-engine bbox setting is deprecated, please use epsg setting instead"
            << std::endl;

  const auto& bboxes = itsConfig.lookup("bbox");
  if (!bboxes.isGroup())
  {
    throw Fmi::Exception(BCP, "The 'bbox' attribute must be a group!")
        .addParameter("Configuration file", itsFileName);
  }

  for (int i = 0; i < bboxes.getLength(); i++)
  {
    const auto& value = bboxes[i];
    std::string field_name = value.getName();
    int id = boost::lexical_cast<int>(field_name.substr(5));

    EPSG epsg;
    epsg.bbox = read_bbox(value);
    epsg.number = id;

    itsEPSGMap.insert(std::make_pair(id, std::move(epsg)));
  }
}

void Config::read_epsg_settings()
{
  const bool has_bbox = itsConfig.exists("bbox");
  const bool has_epsg = itsConfig.exists("epsg");

  if (has_bbox && has_epsg)
    throw Fmi::Exception(BCP,
                         "The config has both bbox and epsg settings, the former are deprecated")
        .addParameter("Configuration file", itsFileName);

  // TODO(mheiskan) deprecated settings
  if (has_bbox)
    read_bbox_settings();
  else
  {
    // New style settings

    const auto& epsg_list = itsConfig.lookup("epsg");

    if (!epsg_list.isList())
    {
      throw Fmi::Exception(BCP, "The 'epsg' attribute must be a list of groups!")
          .addParameter("Configuration file", itsFileName);
    }

    for (int i = 0; i < epsg_list.getLength(); i++)
    {
      EPSG epsg = read_epsg(epsg_list[i]);
      itsEPSGMap.insert(std::make_pair(epsg.number, std::move(epsg)));
    }
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Construct from configuration file name
 */
// ----------------------------------------------------------------------

Config::Config(std::string theFileName) : itsFileName(std::move(theFileName))
{
  try
  {
    if (itsFileName.empty())
      return;

    try
    {
      // Enable sensible relative include paths
      boost::filesystem::path p = theFileName;
      p.remove_filename();
      itsConfig.setIncludeDir(p.c_str());

      itsConfig.readFile(itsFileName.c_str());

      itsConfig.lookupValue("quiet", itsQuiet);

      read_crs_settings();
      read_postgis_settings();
      read_cache_settings();
      read_gdal_settings();
      read_epsg_settings();
      read_postgis_info();
    }
    catch (const libconfig::SettingException& err)
    {
      throw Fmi::Exception(BCP, "Configuration file setting error!")
          .addParameter("Configuration file", itsFileName)
          .addParameter("Path", err.getPath())
          .addParameter("Error description", err.what());
    }
    catch (const libconfig::ParseException& err)
    {
      throw Fmi::Exception(BCP, "Configuration file parsing failed!")
          .addParameter("Configuration file", itsFileName)
          .addParameter("Error line", std::to_string(err.getLine()))
          .addParameter("Error description", err.getError());
    }
    catch (const libconfig::ConfigException& err)
    {
      throw Fmi::Exception(BCP, "Configuration exception!")
          .addParameter("Configuration file", itsFileName)
          .addParameter("Error description", err.what());
    }
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

Spine::CRSRegistry& Config::getCRSRegistry()
{
  return itsCRSRegistry;
}

BBox Config::getBBox(int theEPSG) const
{
  try
  {
    const auto it = itsEPSGMap.find(theEPSG);
    if (it != itsEPSGMap.end())
      return it->second.bbox;
    return BBox{-180, 180, -90, 90};
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

boost::optional<EPSG> Config::getEPSG(int theEPSG) const
{
  try
  {
    const auto it = itsEPSGMap.find(theEPSG);
    if (it != itsEPSGMap.end())
      return it->second;
    return {};
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

const postgis_connection_info& Config::getPostGISConnectionInfo(const std::string thePGName) const
{
  try
  {
    // THIS IS VERY QUESTIONABLE!!!!! PGNAME SHOULD BE MANDATORY ARGUMENT!!!!!!!
    if (thePGName.empty())
    {
      // If pgname is empty try default
      if (!itsDefaultConnectionInfo.host.empty())
        return itsDefaultConnectionInfo;
      auto it = itsConnectionInfo.find("gis");
      if (it == itsConnectionInfo.end())
        throw Fmi::Exception(BCP, "Default postgis setting missing: \"gis\"");
      return it->second;
    }

    if (itsConnectionInfo.find(thePGName) == itsConnectionInfo.end())
      throw Fmi::Exception(BCP, "No postgis settings found for '" + thePGName + "'");

    return itsConnectionInfo.find(thePGName)->second;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Get the default EPSG for tables whose SRID is missing
 */
// ----------------------------------------------------------------------

boost::optional<int> Config::getDefaultEPSG() const
{
  return itsDefaultEPSG;
}

// ----------------------------------------------------------------------
/*!
 * \brief Return precomputed/fixed bbox if there is one
 */
// ----------------------------------------------------------------------

boost::optional<BBox> Config::getTableBBox(const std::string& theSchema,
                                           const std::string& theTable) const
{
  std::string key = theSchema + "." + theTable;
  auto pos = itsPostGisBBoxMap.find(key);
  if (pos == itsPostGisBBoxMap.end())
    return {};
  return pos->second;
}

// ----------------------------------------------------------------------
/*!
 * \brief Return configured timestep for PostGIS data
 */
// ----------------------------------------------------------------------

boost::optional<boost::posix_time::time_duration> Config::getTableTimeStep(
    const std::string& theSchema, const std::string& theTable) const
{
  std::string key = theSchema + "." + theTable;
  auto pos = itsPostGisTimeStepMap.find(key);
  if (pos == itsPostGisTimeStepMap.end())
    return {};
  return pos->second;
}

// ----------------------------------------------------------------------
/*!
 * \brief Return true for quiet mode
 */
// ----------------------------------------------------------------------

bool Config::quiet() const
{
  return itsQuiet;
}

}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet
