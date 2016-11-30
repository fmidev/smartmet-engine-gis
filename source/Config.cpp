// ======================================================================

#include "Config.h"
#include "CRSRegistry.h"
#include <spine/ConfigBase.h>
#include <spine/Exception.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <gdal/cpl_conv.h>  // For configuring GDAL
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

void parse_single_crs_def(SmartMet::Spine::ConfigBase& theConfig,
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
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
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
      throw SmartMet::Spine::Exception(
          BCP, "Gis config: CRS directory '" + theDir.string() + "' not found");

    if (not fs::is_directory(theDir))
      throw SmartMet::Spine::Exception(
          BCP, "Gis config: CRS directory '" + theDir.string() + "' is not a directory");

    for (auto it = fs::directory_iterator(theDir); it != fs::directory_iterator(); ++it)
    {
      const fs::path entry = *it;
      const auto fn = entry.filename().string();
      if (fs::is_regular_file(entry) and not ba::starts_with(fn, ".") and
          not ba::starts_with(fn, "#") and ba::ends_with(fn, ".conf"))
      {
        boost::shared_ptr<SmartMet::Spine::ConfigBase> desc(
            new SmartMet::Spine::ConfigBase(entry.string(), "CRS description"));

        try
        {
          auto& root = desc->get_root();
          parse_single_crs_def(*desc, root, theRegistry);
        }
        catch (...)
        {
          SmartMet::Spine::Exception exception(BCP, "Invalid CRS description!", NULL);
          exception.addParameter("File", entry.string());
          throw exception;
        }
      }
    }
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Construct from configuration file name
 */
// ----------------------------------------------------------------------

Config::Config(const std::string& theFileName) : itsConfig(), itsCRSRegistry()
{
  try
  {
    if (theFileName.empty())
      return;

    try
    {
      itsConfig.readFile(theFileName.c_str());

      std::string crs_dir;
      itsConfig.lookupValue("crsDefinitionDir", crs_dir);
      read_crs_dir(crs_dir, itsCRSRegistry);

      // Read postgis settings

      if (!itsConfig.exists("postgis.host"))
      {
        SmartMet::Spine::Exception exception(BCP, "The 'postgis.host' attribute not set!");
        exception.addParameter("Configuration file", theFileName);
        throw exception;
      }

      if (!itsConfig.exists("postgis.port"))
      {
        SmartMet::Spine::Exception exception(BCP, "The 'postgis.port' attribute not set!");
        exception.addParameter("Configuration file", theFileName);
        throw exception;
      }

      if (!itsConfig.exists("postgis.database"))
      {
        SmartMet::Spine::Exception exception(BCP,
                                             "The 'postgis.database' attribute not set!"
                                             "");
        exception.addParameter("Configuration file", theFileName);
        throw exception;
      }

      if (!itsConfig.exists("postgis.username"))
      {
        SmartMet::Spine::Exception exception(BCP, "The 'postgis.username' attribute not set!");
        exception.addParameter("Configuration file", theFileName);
        throw exception;
      }

      if (!itsConfig.exists("postgis.password"))
      {
        SmartMet::Spine::Exception exception(BCP, "The 'postgis.password' attribute not set!");
        exception.addParameter("Configuration file", theFileName);
        throw exception;
      }

      if (!itsConfig.exists("postgis.encoding"))
      {
        SmartMet::Spine::Exception exception(BCP, "The 'postgis.encoding' attribute not set!");
        exception.addParameter("Configuration file", theFileName);
        throw exception;
      }

      itsConfig.lookupValue("postgis.host", itsHost);
      itsConfig.lookupValue("postgis.port", itsPort);
      itsConfig.lookupValue("postgis.database", itsDatabase);
      itsConfig.lookupValue("postgis.username", itsUsername);
      itsConfig.lookupValue("postgis.password", itsPassword);
      itsConfig.lookupValue("postgis.encoding", itsEncoding);

      libconfig::Setting& pg_sett = itsConfig.lookup("postgis");
      unsigned int n_pgsett = pg_sett.getLength();
      for (unsigned int i = 0; i < n_pgsett; i++)
      {
        libconfig::Setting& sett = pg_sett[i];

        if (sett.isGroup())
        {
          std::string sett_name(sett.getName());
          postgis_connection_info pgci;

          itsConfig.lookupValue("postgis." + sett_name + ".host", pgci.itsHost);
          itsConfig.lookupValue("postgis." + sett_name + ".port", pgci.itsPort);
          itsConfig.lookupValue("postgis." + sett_name + ".database", pgci.itsDatabase);
          itsConfig.lookupValue("postgis." + sett_name + ".username", pgci.itsUsername);
          itsConfig.lookupValue("postgis." + sett_name + ".password", pgci.itsPassword);
          itsConfig.lookupValue("postgis." + sett_name + ".encoding", pgci.itsEncoding);

          postgis_settings.insert(make_pair(sett_name, pgci));
        }
      }

      /*
      std::cout << "ANSSI:\n";

      //	  typedef std::map<std::string, postgis_id> pg_setting_map;
      for(std::map<std::string, postgis_connection_info>::const_iterator iter =
      postgis_settings.begin();
              iter != postgis_settings.end(); ++iter)
            {
              std::cout << iter->first << std::endl
                                    << iter->second.itsHost << ","
                                    << iter->second.itsPort << ","
                                    << iter->second.itsDatabase << ","
                                    << iter->second.itsUsername << ","
                                    << iter->second.itsPassword << ","
                                    << iter->second.itsEncoding << std::endl;
            }
      */

      if (!itsConfig.exists("cache.max_size"))
      {
        SmartMet::Spine::Exception exception(BCP, "The 'cache.max_size' attribute not set!");
        exception.addParameter("Configuration file", theFileName);
        throw exception;
      }

      itsConfig.lookupValue("cache.max_size", itsMaxCacheSize);

      // EPSG bounding boxes

      const auto& bboxes = itsConfig.lookup("bbox");
      if (!bboxes.isGroup())
      {
        SmartMet::Spine::Exception exception(BCP, "The 'bbox' attribute must be a group!");
        exception.addParameter("Configuration file", theFileName);
        throw exception;
      }

      for (int i = 0; i < bboxes.getLength(); i++)
      {
        const auto& value = bboxes[i];
        if (!value.isArray())
        {
          SmartMet::Spine::Exception exception(BCP, "The 'bbox' element must contain arrays!");
          exception.addParameter("Configuration file", theFileName);
          throw exception;
        }

        if (value.getLength() != 4)
        {
          SmartMet::Spine::Exception exception(BCP,
                                               "The 'bbox' elements must be arrays of size 4!");
          exception.addDetail("Found an array of length " + std::to_string(value.getLength()));
          exception.addParameter("Configuration file", theFileName);
          throw exception;
        }
        std::string name = value.getName();
        int epsg = boost::lexical_cast<int>(name.substr(5));
        double w = value[0];
        double e = value[1];
        double s = value[2];
        double n = value[3];
        itsBBoxes.insert(std::make_pair(epsg, BBox(w, e, s, n)));
      }

      // GDAL settings

      if (itsConfig.exists("gdal"))
      {
        const auto& settings = itsConfig.lookup("gdal");
        if (!settings.isGroup())
        {
          SmartMet::Spine::Exception exception(
              BCP, "The 'gdal' parameter must be a group of GDAL settings!");
          exception.addParameter("Configuration file", theFileName);
          throw exception;
        }

        for (int i = 0; i < settings.getLength(); i++)
        {
          const auto& value = settings[i];
          const char* name = value.getName();
          CPLSetConfigOption(name, value);
        }
      }
    }
    catch (const libconfig::SettingException& err)
    {
      SmartMet::Spine::Exception exception(BCP, "Configuration file setting error!");
      exception.addParameter("Configuration file", theFileName);
      exception.addParameter("Path", err.getPath());
      exception.addParameter("Error description", err.what());
      throw exception;
    }
    catch (libconfig::ParseException& err)
    {
      SmartMet::Spine::Exception exception(BCP, "Configuration file parsing failed!");
      exception.addParameter("Configuration file", theFileName);
      exception.addParameter("Error line", std::to_string(err.getLine()));
      exception.addParameter("Error description", err.getError());
      throw exception;
    }
    catch (const libconfig::ConfigException& err)
    {
      SmartMet::Spine::Exception exception(BCP, "Configuration exception!");
      exception.addParameter("Configuration file", theFileName);
      exception.addParameter("Error description", err.what());
      throw exception;
    }
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Return the CRS registry
 */
// ----------------------------------------------------------------------

CRSRegistry& Config::getCRSRegistry()
{
  return itsCRSRegistry;
}

// ----------------------------------------------------------------------
/*!
 * \brief Return bounding box for given EPSG
 */
// ----------------------------------------------------------------------

BBox Config::getBBox(int theEPSG) const
{
  try
  {
    const auto it = itsBBoxes.find(theEPSG);
    if (it != itsBBoxes.end())
      return it->second;
    else
      return BBox(-180, 180, -90, 90);
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

const postgis_connection_info& Config::getPostGISConnectionInfo(const std::string thePGName) const
{
  try
  {
    // THIS IS VERY QUESTIONABLE!!!!! PGNAME SHOULD BE MANDATORY ARGUMENT!!!!!!!
    if (thePGName.empty())
    {
      auto it = postgis_settings.find("gis");
      if (it == postgis_settings.end())
        throw SmartMet::Spine::Exception(BCP, "Default postgis setting missing: \"gis\"");
      else
        return it->second;
    }

    if (postgis_settings.find(thePGName) == postgis_settings.end())
      throw SmartMet::Spine::Exception(BCP, "No postgis settings found for '" + thePGName + "'");

    return postgis_settings.find(thePGName)->second;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet
