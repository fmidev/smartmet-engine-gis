// ======================================================================

#include "Engine.h"

#include <spine/Exception.h>
#include <macgyver/StringConversion.h>

#include <gis/Host.h>
#include <gis/OGR.h>
#include <gis/PostGIS.h>

#include <gdal/ogrsf_frmts.h>

#include <boost/algorithm/string/join.hpp>

#include <memory>
#include <stdexcept>

auto featuredeleter = [](OGRFeature* p)
{
  OGRFeature::DestroyFeature(p);
};
using SafeFeature = std::unique_ptr<OGRFeature, decltype(featuredeleter)>;

namespace
{
int getEpsgCode(const OGRDataSourcePtr& connection,
                const std::string& schema,
                const std::string& table,
                const std::string& geometry_column)
{
  try
  {
    auto layerdeleter = [&](OGRLayer* p)
    {
      connection->ReleaseResultSet(p);
    };
    using SafeLayer = std::unique_ptr<OGRLayer, decltype(layerdeleter)>;

    std::string sqlStmt =
        "select st_srid(" + geometry_column + ") from " + schema + "." + table + " limit 1;";

    SafeLayer pLayer(connection->ExecuteSQL(sqlStmt.c_str(), NULL, NULL), layerdeleter);

    if (!pLayer)
      throw SmartMet::Spine::Exception(
          BCP, "Gis-engine: PostGIS metadata query failed: '" + sqlStmt + "'");

    SafeFeature pFeature(pLayer->GetNextFeature(), featuredeleter);

    if (!pFeature)
      throw SmartMet::Spine::Exception(BCP, "Gis-engine: Null feature received from " + sqlStmt);

    int epsg = pFeature->GetFieldAsInteger(0);

    return epsg;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

OGREnvelope getTableEnvelope(const OGRDataSourcePtr& connection,
                             const std::string& schema,
                             const std::string& table,
                             const std::string& geometry_column)
{
  try
  {
    std::string sqlStmt = "SELECT ST_EstimatedExtent('" + schema + "', '" + table + "', '" +
                          geometry_column + "')::geometry as extent";

    auto layerdeleter = [&](OGRLayer* p)
    {
      connection->ReleaseResultSet(p);
    };
    using SafeLayer = std::unique_ptr<OGRLayer, decltype(layerdeleter)>;

    SafeLayer pLayer(connection->ExecuteSQL(sqlStmt.c_str(), NULL, NULL), layerdeleter);

    if (!pLayer)
      throw SmartMet::Spine::Exception(
          BCP, "Gis-engine: PostGIS metadata query failed: '" + sqlStmt + "'");

    SafeFeature pFeature(pLayer->GetNextFeature(), featuredeleter);

    if (!pFeature)
      throw SmartMet::Spine::Exception(
          BCP, "Gis-engine: PostGIS feature query failed: '" + sqlStmt + "'");

    // get geometry
    OGRGeometry* pGeometry = pFeature->GetGeometryRef();

    if (!pGeometry)
      throw SmartMet::Spine::Exception(
          BCP, "Gis-engine: PostGIS feature query failed: '" + sqlStmt + "'");

    OGREnvelope bbox;
    pGeometry->getEnvelope(&bbox);

    return bbox;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}
}

namespace SmartMet
{
namespace Engine
{
namespace Gis
{
namespace
{
OGRDataSourcePtr db_connection(const Config& config, const std::string& pgname)
{
  try
  {
    // Read it from the database
    const postgis_connection_info& pgci = config.getPostGISConnectionInfo(pgname);

    Fmi::Host host(
        pgci.itsHost, pgci.itsDatabase, pgci.itsUsername, pgci.itsPassword, pgci.itsPort);

    return host.connect();
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}
}

// ----------------------------------------------------------------------
/*!
 * \brief Create cache-keys for the map options
 */
// ----------------------------------------------------------------------

std::pair<std::string, std::string> cache_keys(const MapOptions& theOptions,
                                               OGRSpatialReference* theSR)
{
  try
  {
    std::string wkt = "NULL";
    if (theSR)
    {
      char* tmp;
      theSR->exportToWkt(&tmp);
      wkt = tmp;
      OGRFree(tmp);  // Note: NOT delete!
    }

    std::string basic = theOptions.schema;
    basic += '|';
    basic += theOptions.table;
    basic += '|';
    basic += boost::algorithm::join(theOptions.fieldnames, "|");
    basic += '|';
    basic += (theOptions.where ? *theOptions.where : "");
    basic += '|';
    basic += wkt;

    std::string key = basic;
    key += '|';
    key += Fmi::to_string(theOptions.minarea ? *theOptions.minarea : 0.0);
    key += '|';
    key += Fmi::to_string(theOptions.mindistance ? *theOptions.mindistance : 0.0);

    return std::make_pair(basic, key);
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief The only permitted constructor requires a configfile
 */
// ----------------------------------------------------------------------

Engine::Engine(const std::string& theFileName) : itsConfigFile(theFileName), itsConfig()
{
}

// ----------------------------------------------------------------------
/*!
 * \brief Initialize the engine
 *
 * Note: We do not wish to delay the initialization of other engines.
 * init() is done in its own thread, hence we read the configuration
 * files here, and hence itsConfig is a pointer instead of an object.
 */
// ----------------------------------------------------------------------

void Engine::init()
{
  try
  {
    itsConfig.reset(new Config(itsConfigFile));

    itsCache.resize(itsConfig->getMaxCacheSize());

    itsFeaturesCache.resize(itsConfig->getMaxCacheSize());

    // Register all drivers just once

    OGRRegisterAll();
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Shutdown the engine
 */
// ----------------------------------------------------------------------

void Engine::shutdown()
{
  std::cout << "  -- Shutdown requested (gis)\n";
}

// ----------------------------------------------------------------------
/*!
 * \brief Return the CRS registry
 */
// ----------------------------------------------------------------------

CRSRegistry& Engine::getCRSRegistry()
{
  try
  {
    return itsConfig->getCRSRegistry();
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Fetch a shape from the database
 *
 * \param theOptions query options
 */
// ----------------------------------------------------------------------

OGRGeometryPtr Engine::getShape(OGRSpatialReference* theSR, const MapOptions& theOptions) const
{
  try
  {
    // Validate options
    if (theOptions.schema.empty())
      throw SmartMet::Spine::Exception(BCP, "PostGIS database name missing from map query");
    if (theOptions.table.empty())
      throw SmartMet::Spine::Exception(BCP, "PostGIS table name missing from map query");

    // Find simplified map from the cache

    auto keys = cache_keys(theOptions, theSR);
    const auto& basic_key = keys.first;
    const auto& full_key = keys.second;

    auto obj = itsCache.find(full_key);
    if (obj)
      return *obj;

    // Find full map from the cache

    OGRGeometryPtr geom;

    obj = itsCache.find(basic_key);
    if (obj)
      geom = *obj;
    else
    {
      // Read it from the database
      auto connection = db_connection(*itsConfig, theOptions.pgname);

      std::string name = theOptions.schema + "." + theOptions.table;
      geom = Fmi::PostGIS::read(theSR, connection, name, theOptions.where);

      // Cache the result if it's not empty
      if (geom)
        itsCache.insert(basic_key, geom);
    }

    // If no simplification was requested we're done
    if (!theOptions.minarea && !theOptions.mindistance)
      return geom;

    // Apply simplification options

    if (theOptions.minarea)
      geom.reset(Fmi::OGR::despeckle(*geom, *theOptions.minarea));

    if (theOptions.mindistance && geom)
    {
      const double kilometers_to_degrees = 1.0 / 110.0;  // one degree latitude =~ 110 km
      const double kilometers_to_meters = 1000;

      OGRSpatialReference* crs = geom->getSpatialReference();
      bool geographic = (crs ? crs->IsGeographic() : false);
      if (!geographic)
        geom.reset(
            geom->SimplifyPreserveTopology(kilometers_to_meters * (*theOptions.mindistance)));
      else
        geom.reset(
            geom->SimplifyPreserveTopology(kilometers_to_degrees * (*theOptions.mindistance)));
    }

    // Cache the result

    if (geom)
      itsCache.insert(full_key, geom);

    return geom;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

Fmi::Features Engine::getFeatures(OGRSpatialReference* theSR, const MapOptions& theOptions) const
{
  try
  {
    // Validate options
    if (theOptions.schema.empty())
      throw SmartMet::Spine::Exception(BCP, "PostGIS database name missing from map query");
    if (theOptions.table.empty())
      throw SmartMet::Spine::Exception(BCP, "PostGIS table name missing from map query");

    // Find simplified map from the cache
    auto keys = cache_keys(theOptions, theSR);
    const auto& basic_key = keys.first;
    const auto& full_key = keys.second;

    auto obj = itsFeaturesCache.find(full_key);
    if (obj)
      return *obj;

    Fmi::Features ret;

    // Find full map from the cache
    obj = itsFeaturesCache.find(basic_key);
    if (obj)
      ret = *obj;
    else
    {
      // Read it from the database
      auto connection = db_connection(*itsConfig, theOptions.pgname);

      std::string name = theOptions.schema + "." + theOptions.table;
      ret = Fmi::PostGIS::read(theSR, connection, name, theOptions.fieldnames, theOptions.where);

      // Cache the result if it's not empty
      if (!ret.empty())
        itsFeaturesCache.insert(basic_key, ret);
    }

    // If no simplification was requested we're done
    if (!theOptions.minarea && !theOptions.mindistance)
      return ret;

    // Apply simplification options

    Fmi::Features newfeatures;
    for (const auto& feature : ret)
    {
      Fmi::FeaturePtr newfeature = feature;
      if (theOptions.minarea)
        newfeature->geom.reset(Fmi::OGR::despeckle(*(newfeature->geom), *theOptions.minarea));

      if (theOptions.mindistance && newfeature->geom)
      {
        const double kilometers_to_degrees = 1.0 / 110.0;  // one degree latitude =~ 110 km
        const double kilometers_to_meters = 1000;

        OGRSpatialReference* crs = newfeature->geom->getSpatialReference();
        bool geographic = (crs ? crs->IsGeographic() : false);

        if (!geographic)
          newfeature->geom.reset(newfeature->geom->SimplifyPreserveTopology(
              kilometers_to_meters * (*theOptions.mindistance)));
        else
          newfeature->geom.reset(newfeature->geom->SimplifyPreserveTopology(
              kilometers_to_degrees * (*theOptions.mindistance)));
      }

      if (newfeature->geom)
        newfeatures.push_back(newfeature);
    }

    // Cache the result
    if (!newfeatures.empty())
      itsFeaturesCache.insert(full_key, newfeatures);

    return newfeatures;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Fetch meta data (timesteps and bounding box) from db
 *
 * \param theOptions query options
 */
// ----------------------------------------------------------------------

MetaData Engine::getMetaData(const MetaDataQueryOptions& theOptions) const
{
  try
  {
    MetaData metadata;

    auto connection = db_connection(*itsConfig, theOptions.pgname);

    auto layerdeleter = [&](OGRLayer* p)
    {
      connection->ReleaseResultSet(p);
    };
    using SafeLayer = std::unique_ptr<OGRLayer, decltype(layerdeleter)>;

    // 1) Get timesteps

    std::string sqlStmt;
    if (theOptions.time_column)
    {
      sqlStmt = "SELECT DISTINCT(" + *theOptions.time_column + ")" + " FROM " + theOptions.schema +
                "." + theOptions.table + " ORDER by " + *theOptions.time_column;

      SafeLayer pLayer(connection->ExecuteSQL(sqlStmt.c_str(), NULL, NULL), layerdeleter);

      if (!pLayer)
        throw SmartMet::Spine::Exception(
            BCP, "Gis-engine: PostGIS metadata query failed: '" + sqlStmt + "'");

      while (true)
      {
        SafeFeature pFeature(pLayer->GetNextFeature(), featuredeleter);
        if (!pFeature)
          break;

        tm timeinfo;

        bool ret = pFeature->GetFieldAsDateTime(0,
                                                &timeinfo.tm_year,
                                                &timeinfo.tm_mon,
                                                &timeinfo.tm_mday,
                                                &timeinfo.tm_hour,
                                                &timeinfo.tm_min,
                                                &timeinfo.tm_sec,
                                                &timeinfo.tm_isdst);

        if (!ret)
        {
          std::cout << "Reading values from time column '" << theOptions.time_column << "' failed!"
                    << std::endl;
          break;
        }

        timeinfo.tm_year -= 1900;  // years after 1900
        timeinfo.tm_mon -= 1;      // months 0..11

        metadata.timesteps.push_back(boost::posix_time::ptime_from_tm(timeinfo));
      }
    }

    // 2) Get bounding box

    // set default values
    metadata.xmin = 0.0;
    metadata.ymin = 0.0;
    metadata.xmax = 0.0;
    metadata.ymax = 0.0;

    int epsg =
        ::getEpsgCode(connection, theOptions.schema, theOptions.table, theOptions.geometry_column);
    OGRSpatialReference oSourceSRS;
    oSourceSRS.importFromEPSGA(epsg);
    OGRSpatialReference oTargetSRS;
    oTargetSRS.importFromEPSGA(4326);

    OGREnvelope table_envelope = ::getTableEnvelope(
        connection, theOptions.schema, theOptions.table, theOptions.geometry_column);
    //-------------------

    // convert source spatial reference system to EPSG:4326

    std::unique_ptr<OGRCoordinateTransformation> poCT(
        OGRCreateCoordinateTransformation(&oSourceSRS, &oTargetSRS));

    if (!poCT)
      throw SmartMet::Spine::Exception(BCP,
                                       "OGRCreateCoordinateTransformation function call failed");

    bool transformation_ok = (poCT->Transform(1, &(table_envelope.MinX), &(table_envelope.MinY)) &&
                              poCT->Transform(1, &(table_envelope.MaxX), &(table_envelope.MaxY)));
    if (transformation_ok)
    {
      metadata.xmin = table_envelope.MinX;
      metadata.ymin = table_envelope.MinY;
      metadata.xmax = table_envelope.MaxX;
      metadata.ymax = table_envelope.MaxY;
    }

    return metadata;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Return the bounding box for a EPSG
 */
// ----------------------------------------------------------------------

BBox Engine::getBBox(int theEPSG) const
{
  try
  {
    return itsConfig->getBBox(theEPSG);
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet

// DYNAMIC MODULE CREATION TOOLS

extern "C" void* engine_class_creator(const char* configfile, void* /* user_data */)
{
  return new SmartMet::Engine::Gis::Engine(configfile);
}

extern "C" const char* engine_name()
{
  return "Gis";
}
// ======================================================================
