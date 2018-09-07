// ======================================================================

#include "Config.h"
#include "CRSRegistry.h"
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <gdal/cpl_conv.h>  // For configuring GDAL
#include <spine/ConfigBase.h>
#include <spine/Exception.h>
#include <stdexcept>

namespace fs = boost::filesystem;
namespace ba = boost::algorithm;

namespace SmartMet
{
namespace Engine
{
namespace Gis
{
// ----------------------------------------------------------------------
/*!
 * \brief Parse a single CRS definition
 */
// ----------------------------------------------------------------------

void parse_single_crs_def(Spine::ConfigBase& theConfig,
                          libconfig::Setting& theEntry,
                          CRSRegistry& theRegistry)
{
  try
  {
    theConfig.assert_is_group(theEntry);

    const std::string name = theConfig.get_mandatory_config_param<std::string>(theEntry, "name");
    bool swap_coord = theConfig.get_optional_config_param<bool>(theEntry, "swapCoord", false);
    bool show_height = theConfig.get_optional_config_param<bool>(theEntry, "showHeight", false);
    const std::string axis_labels =
        theConfig.get_mandatory_config_param<std::string>(theEntry, "axisLabels");
    const std::string proj_epoch_uri =
        theConfig.get_mandatory_config_param<std::string>(theEntry, "projEpochUri");
    std::string proj_uri;

    if (theEntry.exists("epsg"))
    {
      using boost::format;
      int epsg = theConfig.get_mandatory_config_param<int>(theEntry, "epsg");
      const std::string def_regex = str(format("(?:urn:ogc:def:crs:|)EPSG:{1,2}%04u") % epsg);
      const std::string def_proj_uri =
          str(format("http://www.opengis.net/def/crs/EPSG/0/%04u") % epsg);
      std::string regex =
          theConfig.get_optional_config_param<std::string>(theEntry, "regex", def_regex);
      proj_uri =
          theConfig.get_optional_config_param<std::string>(theEntry, "projUri", def_proj_uri);
      theRegistry.register_epsg(name, epsg, regex, swap_coord);
    }
    else
    {
      const std::string proj4 =
          theConfig.get_mandatory_config_param<std::string>(theEntry, "proj4");
      const std::string regex =
          theConfig.get_mandatory_config_param<std::string>(theEntry, "regex");
      proj_uri = theConfig.get_mandatory_config_param<std::string>(theEntry, "projUri");
      theRegistry.register_proj4(name, proj4, regex, swap_coord);
      if (theEntry.exists("epsgCode"))
      {
        int epsg = theConfig.get_mandatory_config_param<int>(theEntry, "epsgCode");
        theRegistry.set_attribute(name, "epsg", epsg);
      }
    }

    theRegistry.set_attribute(name, "showHeight", show_height);
    theRegistry.set_attribute(name, "projUri", proj_uri);
    theRegistry.set_attribute(name, "projEpochUri", proj_epoch_uri);
    theRegistry.set_attribute(name, "axisLabels", axis_labels);
  }
  catch (...)
  {
    throw Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Read CRS definitions from a directory
 */
// ----------------------------------------------------------------------

void read_crs_dir(const fs::path& theDir, CRSRegistry& theRegistry)
{
  try
  {
    if (not fs::exists(theDir))
      throw Spine::Exception(BCP, "Gis config: CRS directory '" + theDir.string() + "' not found");

    if (not fs::is_directory(theDir))
      throw Spine::Exception(
          BCP, "Gis config: CRS directory '" + theDir.string() + "' is not a directory");

    for (auto it = fs::directory_iterator(theDir); it != fs::directory_iterator(); ++it)
    {
      const fs::path entry = *it;
      const auto fn = entry.filename().string();
      if (fs::is_regular_file(entry) and not ba::starts_with(fn, ".") and
          not ba::starts_with(fn, "#") and ba::ends_with(fn, ".conf"))
      {
        boost::shared_ptr<Spine::ConfigBase> desc(
            new Spine::ConfigBase(entry.string(), "CRS description"));

        try
        {
          auto& root = desc->get_root();
          parse_single_crs_def(*desc, root, theRegistry);
        }
        catch (...)
        {
          throw Spine::Exception::Trace(BCP, "Invalid CRS description!")
              .addParameter("File", entry.string());
        }
      }
    }
  }
  catch (...)
  {
    throw Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

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
  read_crs_dir(crs_dir, itsCRSRegistry);
}

void Config::require_postgis_settings() const
{
  if (!itsConfig.exists("postgis.host"))
  {
    throw Spine::Exception(BCP, "The 'postgis.host' attribute not set!")
        .addParameter("Configuration file", itsFileName);
  }

  if (!itsConfig.exists("postgis.port"))
  {
    throw Spine::Exception(BCP, "The 'postgis.port' attribute not set!")
        .addParameter("Configuration file", itsFileName);
  }

  if (!itsConfig.exists("postgis.database"))
  {
    throw Spine::Exception(BCP,
                           "The 'postgis.database' attribute not set!"
                           "")
        .addParameter("Configuration file", itsFileName);
  }

  if (!itsConfig.exists("postgis.username"))
  {
    throw Spine::Exception(BCP, "The 'postgis.username' attribute not set!")
        .addParameter("Configuration file", itsFileName);
  }

  if (!itsConfig.exists("postgis.password"))
  {
    throw Spine::Exception(BCP, "The 'postgis.password' attribute not set!")
        .addParameter("Configuration file", itsFileName);
  }

  if (!itsConfig.exists("postgis.encoding"))
  {
    throw Spine::Exception(BCP, "The 'postgis.encoding' attribute not set!")
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
  unsigned int n_pgsett = pg_sett.getLength();
  for (unsigned int i = 0; i < n_pgsett; i++)
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

void Config::read_cache_settings()
{
  if (!itsConfig.exists("cache.max_size"))
  {
    throw Spine::Exception(BCP, "The 'cache.max_size' attribute not set!")
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
    throw Spine::Exception(BCP, "The 'gdal' parameter must be a group of GDAL settings!")
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
    throw Spine::Exception(BCP, "The 'bbox' element must contain arrays!")
        .addParameter("Configuration file", itsFileName);
  }

  if (theSetting.getLength() != 4)
  {
    throw Spine::Exception(BCP, "The 'bbox' elements must be arrays of size 4!")
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
    throw Spine::Exception(BCP, "The 'epsg' elements must be groups!")
        .addParameter("Configuration file", itsFileName);
  }

  if (!theSetting.exists("id"))
    throw Spine::Exception(BCP, "The 'epsg' elements must have an 'id' field");
  if (!theSetting.exists("bbox"))
    throw Spine::Exception(BCP, "The 'epsg' elements must have a 'bbox' field");

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
    throw Spine::Exception(BCP, "The 'bbox' attribute must be a group!")
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
    throw Spine::Exception(BCP,
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
      throw Spine::Exception(BCP, "The 'epsg' attribute must be a list of groups!")
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
      itsConfig.readFile(itsFileName.c_str());

      itsConfig.lookupValue("quiet", itsQuiet);

      read_crs_settings();
      read_postgis_settings();
      read_cache_settings();
      read_gdal_settings();
      read_epsg_settings();
    }
    catch (const libconfig::SettingException& err)
    {
      throw Spine::Exception(BCP, "Configuration file setting error!")
          .addParameter("Configuration file", itsFileName)
          .addParameter("Path", err.getPath())
          .addParameter("Error description", err.what());
    }
    catch (const libconfig::ParseException& err)
    {
      throw Spine::Exception(BCP, "Configuration file parsing failed!")
          .addParameter("Configuration file", itsFileName)
          .addParameter("Error line", std::to_string(err.getLine()))
          .addParameter("Error description", err.getError());
    }
    catch (const libconfig::ConfigException& err)
    {
      throw Spine::Exception(BCP, "Configuration exception!")
          .addParameter("Configuration file", itsFileName)
          .addParameter("Error description", err.what());
    }
  }
  catch (...)
  {
    throw Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

CRSRegistry& Config::getCRSRegistry()
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
    throw Spine::Exception::Trace(BCP, "Operation failed!");
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
    throw Spine::Exception::Trace(BCP, "Operation failed!");
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
        throw Spine::Exception(BCP, "Default postgis setting missing: \"gis\"");
      else
        return it->second;
    }

    if (itsConnectionInfo.find(thePGName) == itsConnectionInfo.end())
      throw Spine::Exception(BCP, "No postgis settings found for '" + thePGName + "'");

    return itsConnectionInfo.find(thePGName)->second;
  }
  catch (...)
  {
    throw Spine::Exception::Trace(BCP, "Operation failed!");
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
