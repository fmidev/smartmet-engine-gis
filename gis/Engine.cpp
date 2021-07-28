#include "Engine.h"
#include "Config.h"
#include "Normalize.h"
#include <boost/algorithm/string/join.hpp>
#include <gis/Box.h>
#include <gis/CoordinateTransformation.h>
#include <gis/Host.h>
#include <gis/OGR.h>
#include <gis/PostGIS.h>
#include <macgyver/Exception.h>
#include <macgyver/StringConversion.h>
#include <gdal_version.h>
#include <memory>
#include <ogrsf_frmts.h>
#include <stdexcept>

const auto featuredeleter = [](OGRFeature* p) { OGRFeature::DestroyFeature(p); };
using SafeFeature = std::unique_ptr<OGRFeature, decltype(featuredeleter)>;

namespace SmartMet
{
namespace Engine
{
namespace Gis
{
namespace
{
int getEpsgCode(const GDALDataPtr& connection,
                const std::string& schema,
                const std::string& table,
                const std::string& geometry_column,
                const SmartMet::Engine::Gis::Config& config)
{
  try
  {
    auto layerdeleter = [&](OGRLayer* p) { connection->ReleaseResultSet(p); };
    using SafeLayer = std::unique_ptr<OGRLayer, decltype(layerdeleter)>;

    std::string sqlStmt =
        "select st_srid(" + geometry_column + ") from " + schema + "." + table + " limit 1;";

    SafeLayer pLayer(connection->ExecuteSQL(sqlStmt.c_str(), nullptr, nullptr), layerdeleter);

    if (!pLayer)
      throw Fmi::Exception(BCP, "Gis-engine: PostGIS metadata query failed: '" + sqlStmt + "'");

    SafeFeature pFeature(pLayer->GetNextFeature(), featuredeleter);

    if (pFeature)
      return pFeature->GetFieldAsInteger(0);

    if (config.getDefaultEPSG())
    {
      if (!config.quiet())
        std::cerr << "Warning: sqlStmt returned null. Setting EPSG to default value "
                  << *config.getDefaultEPSG() << std::endl;
      return *config.getDefaultEPSG();
    }

    throw Fmi::Exception(BCP, "Gis-engine: Null feature received from " + sqlStmt);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

GDALDataPtr db_connection(const Config& config, const std::string& pgname)
{
  try
  {
    // Read it from the database
    const postgis_connection_info& pgci = config.getPostGISConnectionInfo(pgname);

    Fmi::Host host(pgci.host, pgci.database, pgci.username, pgci.password, pgci.port);

    return host.connect();
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}
}  // namespace

// ----------------------------------------------------------------------
/*!
 * \brief Create cache-keys for the map options
 */
// ----------------------------------------------------------------------

std::pair<std::string, std::string> cache_keys(const MapOptions& theOptions,
                                               const Fmi::SpatialReference* theSR)
{
  try
  {
    std::string wkt = "NULL";
    if (theSR)
      wkt = Fmi::OGR::exportToWkt(*theSR);

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
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

OGREnvelope Engine::getTableEnvelope(const GDALDataPtr& connection,
                                     const std::string& schema,
                                     const std::string& table,
                                     const std::string& geometry_column,
                                     bool quiet) const
{
  try
  {
    // we assume there is just one geometry column
    auto fixed_bbox = itsConfig->getTableBBox(schema, table);
    if (fixed_bbox)
    {
      OGREnvelope bbox;
      bbox.MinX = fixed_bbox->west;
      bbox.MinY = fixed_bbox->south;
      bbox.MaxX = fixed_bbox->east;
      bbox.MaxY = fixed_bbox->north;
      return bbox;
    }

#if 0
    std::string sqlStmt = "SELECT ST_EstimatedExtent('" + schema + "', '" + table + "', '" +
                          geometry_column + "')::geometry as extent";
#else
    std::string sqlStmt = "SELECT ST_Extent(" + geometry_column + ")::geometry as extent FROM " +
                          schema + "." + table;
#endif

    auto layerdeleter = [&](OGRLayer* p) { connection->ReleaseResultSet(p); };
    using SafeLayer = std::unique_ptr<OGRLayer, decltype(layerdeleter)>;

    SafeLayer pLayer(connection->ExecuteSQL(sqlStmt.c_str(), nullptr, nullptr), layerdeleter);

    if (!pLayer)
      throw Fmi::Exception(BCP, "Gis-engine: PostGIS metadata query failed: '" + sqlStmt + "'");

    SafeFeature pFeature(pLayer->GetNextFeature(), featuredeleter);

    if (!pFeature)
      throw Fmi::Exception(BCP, "Gis-engine: PostGIS feature query failed: '" + sqlStmt + "'");

    // get geometry
    OGRGeometry* pGeometry = pFeature->GetGeometryRef();

    if (!pGeometry)
    {
      Fmi::Exception ex(BCP, "Gis-engine: PostGIS feature query failed: '" + sqlStmt + "'");
      if (quiet)
        ex.disableLogging();
      throw ex;
    }

    OGREnvelope bbox;
    pGeometry->getEnvelope(&bbox);

    return bbox;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief The only permitted constructor requires a configfile
 */
// ----------------------------------------------------------------------

Engine::Engine(const std::string& theFileName) : itsConfigFile(theFileName) {}

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
    itsEnvelopeCache.resize(itsConfig->getMaxCacheSize());

    // Register all drivers just once

#if GDAL_VERSION_MAJOR < 2
    OGRRegisterAll();
#else
    GDALAllRegister();
#endif
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
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

Spine::CRSRegistry& Engine::getCRSRegistry()
{
  try
  {
    return itsConfig->getCRSRegistry();
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Fetch a shape from the database
 *
 * \param theOptions query options
 */
// ----------------------------------------------------------------------

OGRGeometryPtr Engine::getShape(const Fmi::SpatialReference* theSR,
                                const MapOptions& theOptions) const
{
  try
  {
    // Validate options
    if (theOptions.schema.empty())
      throw Fmi::Exception(BCP, "PostGIS database name missing from map query");
    if (theOptions.table.empty())
      throw Fmi::Exception(BCP, "PostGIS table name missing from map query");

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
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

Fmi::Features Engine::getFeatures(const MapOptions& theOptions) const
{
  return getFeatures(nullptr, theOptions);
}

Fmi::Features Engine::getFeatures(const Fmi::SpatialReference& theSR,
                                  const MapOptions& theOptions) const
{
  return getFeatures(&theSR, theOptions);
}

Fmi::Features Engine::getFeatures(const Fmi::SpatialReference* theSR,
                                  const MapOptions& theOptions) const
{
  try
  {
    // Validate options
    if (theOptions.schema.empty())
      throw Fmi::Exception(BCP, "PostGIS database name missing from map query");
    if (theOptions.table.empty())
      throw Fmi::Exception(BCP, "PostGIS table name missing from map query");

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
    {
      ret = *obj;
    }
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
      Fmi::FeaturePtr newfeature;
      if (theOptions.minarea)
        newfeature->geom.reset(Fmi::OGR::despeckle(*(feature->geom), *theOptions.minarea));

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
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
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

    // Get time always in UTC
    connection->ExecuteSQL("SET TIME ZONE UTC", nullptr, nullptr);

    auto layerdeleter = [&](OGRLayer* p) { connection->ReleaseResultSet(p); };
    using SafeLayer = std::unique_ptr<OGRLayer, decltype(layerdeleter)>;

    // 1) Get timesteps
    if (theOptions.time_column)
    {
      auto timestep = itsConfig->getTableTimeStep(theOptions.schema, theOptions.table);
      if (!timestep)
      {
        // List all available times
        std::string sqlStmt = "SELECT DISTINCT(" + *theOptions.time_column + ")" + " FROM " +
                              theOptions.schema + "." + theOptions.table + " WHERE " +
                              *theOptions.time_column + " IS NOT NULL ORDER by " +
                              *theOptions.time_column;

        SafeLayer pLayer(connection->ExecuteSQL(sqlStmt.c_str(), nullptr, nullptr), layerdeleter);

        if (!pLayer)
          throw Fmi::Exception(BCP, "Gis-engine: PostGIS metadata query failed: '" + sqlStmt + "'");

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
            std::cout << "Reading values from '" << theOptions.schema << "." << theOptions.table
                      << "." << *theOptions.time_column << "' failed!" << std::endl;
            break;
          }

          timeinfo.tm_year -= 1900;  // years after 1900
          timeinfo.tm_mon -= 1;      // months 0..11

          metadata.timesteps.push_back(boost::posix_time::ptime_from_tm(timeinfo));
        }
      }

      else
      {
        // Establish starttime and endtime
        std::string sqlStmt = "SELECT min(" + *theOptions.time_column + ") as mintime, max(" +
                              *theOptions.time_column + ") as maxtime FROM " + theOptions.schema +
                              "." + theOptions.table + " WHERE " + *theOptions.time_column +
                              " IS NOT NULL";

        SafeLayer pLayer(connection->ExecuteSQL(sqlStmt.c_str(), nullptr, nullptr), layerdeleter);

        if (!pLayer)
          throw Fmi::Exception(BCP, "Gis-engine: PostGIS metadata query failed: '" + sqlStmt + "'");

        SafeFeature pFeature(pLayer->GetNextFeature(), featuredeleter);

        if (!pFeature)
          throw Fmi::Exception(BCP, "Gis-engine: PostGIS feature query failed: '" + sqlStmt + "'");

        tm start_tm;
        tm end_tm;

        bool ret1 = pFeature->GetFieldAsDateTime(0,
                                                 &start_tm.tm_year,
                                                 &start_tm.tm_mon,
                                                 &start_tm.tm_mday,
                                                 &start_tm.tm_hour,
                                                 &start_tm.tm_min,
                                                 &start_tm.tm_sec,
                                                 &start_tm.tm_isdst);
        bool ret2 = pFeature->GetFieldAsDateTime(1,
                                                 &end_tm.tm_year,
                                                 &end_tm.tm_mon,
                                                 &end_tm.tm_mday,
                                                 &end_tm.tm_hour,
                                                 &end_tm.tm_min,
                                                 &end_tm.tm_sec,
                                                 &end_tm.tm_isdst);

        if (!ret1 || !ret2)
        {
          std::cout << "Reading values from '" << theOptions.schema << "." << theOptions.table
                    << "." << *theOptions.time_column << "' failed!" << std::endl;
        }
        else
        {
          start_tm.tm_year -= 1900;  // years after 1900
          start_tm.tm_mon -= 1;      // months 0..11

          end_tm.tm_year -= 1900;  // years after 1900
          end_tm.tm_mon -= 1;      // months 0..11

          auto starttime = boost::posix_time::ptime_from_tm(start_tm);
          auto endtime = boost::posix_time::ptime_from_tm(end_tm);
          metadata.timeinterval = TimeInterval{starttime, endtime, *timestep};
        }
      }
    }

    // 2) Get bounding box
    metadata.xmin = 0.0;
    metadata.ymin = 0.0;
    metadata.xmax = 0.0;
    metadata.ymax = 0.0;

    // Cache envelopes. We assume no need to reduce envelope sizes when old data is deleted
    auto hash = theOptions.hash_value();
    if (theOptions.time_column && !metadata.timesteps.empty())
    {
      const auto& last_time = metadata.timesteps.back();
      boost::hash_combine(hash, boost::hash_value(Fmi::to_iso_string(last_time)));
    }

    auto obj = itsEnvelopeCache.find(hash);
    if (obj)
    {
      metadata.xmin = obj->MinX;
      metadata.ymin = obj->MinY;
      metadata.xmax = obj->MaxX;
      metadata.ymax = obj->MaxY;
      return metadata;
    }

    OGREnvelope table_envelope = getTableEnvelope(connection,
                                                  theOptions.schema,
                                                  theOptions.table,
                                                  theOptions.geometry_column,
                                                  itsConfig->quiet());

    // convert source spatial reference system to EPSG:4326
    int epsg = getEpsgCode(
        connection, theOptions.schema, theOptions.table, theOptions.geometry_column, *itsConfig);

    Fmi::SpatialReference source(epsg);
    Fmi::SpatialReference target("WGS84");
    Fmi::CoordinateTransformation transformation(source, target);

    bool ok = (transformation.transform(table_envelope.MinX, table_envelope.MinY) &&
               transformation.transform(table_envelope.MaxX, table_envelope.MaxY));
    if (ok)
    {
      metadata.xmin = table_envelope.MinX;
      metadata.ymin = table_envelope.MinY;
      metadata.xmax = table_envelope.MaxX;
      metadata.ymax = table_envelope.MaxY;
    }

    itsEnvelopeCache.insert(hash, table_envelope);

    return metadata;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
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
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Return EPSG information
 */
// ----------------------------------------------------------------------

boost::optional<EPSG> Engine::getEPSG(int theEPSG) const
{
  try
  {
    return itsConfig->getEPSG(theEPSG);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

void Engine::populateGeometryStorage(const PostGISIdentifierVector& thePostGISIdentifiers,
                                     GeometryStorage& theGeometryStorage) const
{
  try
  {
    // If geometries with same name and type came from the same table (like roads) they are merged
    // into one. Geometries with same name and type came are not merged if they are from different
    // tables, for example same city can be in deffirent tables (like esri.europe_cities_eureffin
    // and natural_earth.world_populated_places)
    std::map<std::string, std::string> geomid_pgkey_map;

    // This set is to prevent multiple readings of the same data source
    // 'pgname:schema:table:field'
    std::set<std::string> pgKeys;

    for (auto pgId : thePostGISIdentifiers)
    {
      if (itsShutdownRequested)
        return;

      std::string pgKey = pgId.key();

      if (pgKeys.find(pgKey) != pgKeys.end())
        continue;

      pgKeys.insert(pgKey);

      SmartMet::Engine::Gis::MapOptions mo;
      mo.pgname = pgId.pgname;
      mo.schema = pgId.schema;
      mo.table = pgId.table;
      mo.fieldnames.insert(pgId.field);

      Fmi::SpatialReference srs("WGS84");

      Fmi::Features features = getFeatures(srs, mo);

      for (const auto& feature : features)
      {
        const OGRGeometry* geom = feature->geom.get();

        if (geom != nullptr)
        {
          Fmi::Attribute attribute = feature->attributes.at(pgId.field);
          AttributeToString ats;
          std::string geomName = boost::apply_visitor(ats, attribute);
          geomName += pgId.source_name;

          // Geometry name can not be empty, since it is used for example in timesries queries
          // like 'place=helsinki'
          if (geomName.empty())
            continue;

          // Convert to lower case
          normalize_string(geomName);

          OGRwkbGeometryType geomType = geom->getGeometryType();

          std::string geom_id = (geomName + Fmi::to_string(static_cast<int>(geomType)));
          if (geomid_pgkey_map.find(geom_id) != geomid_pgkey_map.end())
          {
            if (geomid_pgkey_map.at(geom_id) != pgKey)
            {
              // Geom of the same type and source_name processed already
              continue;
            }

            if (geomType == wkbPoint)
            {
              // Skip POINT-geometry with the same name and type. For example city 'Brest' can be
              // found both in France and in Belarus. Other than POINT-geometries are
              // merged below, for example roads are stored in small sections and must be merged
              continue;
            }
          }
          else
          {
            // Geometry (name+type) encountered first time
            geomid_pgkey_map[geomName + Fmi::to_string(static_cast<int>(geomType))] = pgKey;
          }

          // If same name and type found
          if (theGeometryStorage.itsGeometries.find(geomType) ==
              theGeometryStorage.itsGeometries.end())
          {
            // If that type of geometries not found, add new one
            NameOGRGeometryMap nameOGRGeometryMap;
            nameOGRGeometryMap.insert(
                make_pair(geomName, boost::shared_ptr<OGRGeometry>(geom->clone())));
            theGeometryStorage.itsGeometries.insert(std::make_pair(geomType, nameOGRGeometryMap));
          }
          else
          {
            // That type of geometries found
            NameOGRGeometryMap& nameOGRGeometryMap = theGeometryStorage.itsGeometries[geomType];
            // If named area not found, add new one
            if (nameOGRGeometryMap.find(geomName) == nameOGRGeometryMap.end())
            {
              nameOGRGeometryMap.insert(
                  std::make_pair(geomName, boost::shared_ptr<OGRGeometry>(geom->clone())));
            }
            else
            {
              // Do the merge with the new and old one
              boost::shared_ptr<OGRGeometry>& previousGeom = nameOGRGeometryMap[geomName];
              if (geomType == wkbMultiLineString)
              {
                // Multilinestrings are merged with addGeometryDirectly-function
                auto* geom_tmp = dynamic_cast<OGRMultiLineString*>(previousGeom->clone());
                const auto* new_geom = dynamic_cast<const OGRMultiLineString*>(geom);
                // Iterate the LINESTRINGS inside Multilinestring and add them to old one
                for (int i = 0; i < new_geom->getNumGeometries(); i++)
                {
                  geom_tmp->addGeometryDirectly(new_geom->getGeometryRef(i)->clone());
                }
                previousGeom.reset(geom_tmp);
              }
              else
              {
                // For other geometries use Union-function
                previousGeom.reset(previousGeom->Union(geom));
              }
            }
          }

          std::string svgString = Fmi::OGR::exportToSvg(*geom, Fmi::Box::identity(), 6);

          if (geomType == wkbPolygon || geomType == wkbMultiPolygon)
          {
#ifdef MYDEBUG
            cout << "POLYGON in SVG format: " << svgString << endl;
#endif
            if (theGeometryStorage.itsPolygons.find(geomName) !=
                theGeometryStorage.itsPolygons.end())
              theGeometryStorage.itsPolygons[geomName] = svgString;
            else
              theGeometryStorage.itsPolygons.insert(std::make_pair(geomName, svgString));
          }
          else if (geomType == wkbLineString || geomType == wkbMultiLineString)
          {
#ifdef MYDEBUG
            std::cout << "LINE in SVG format: " << svgString << std::endl;
#endif
            if (theGeometryStorage.itsLines.find(geomName) != theGeometryStorage.itsLines.end())
            {
              theGeometryStorage.itsLines[geomName].append(svgString);
            }
            else
              theGeometryStorage.itsLines.insert(std::make_pair(geomName, svgString));
          }
          else if (geomType == wkbPoint)
          {
            const auto* ogrPoint = reinterpret_cast<const OGRPoint*>(geom);
            if (theGeometryStorage.itsPoints.find(geomName) != theGeometryStorage.itsPoints.end())
              theGeometryStorage.itsPoints[geomName] =
                  std::make_pair(ogrPoint->getX(), ogrPoint->getY());
            else
              theGeometryStorage.itsPoints.insert(
                  std::make_pair(geomName, std::make_pair(ogrPoint->getX(), ogrPoint->getY())));
          }
        }
      }
    }
    // Add quotation mark in the beginning and in the end
    for (auto& svg : theGeometryStorage.itsLines)
    {
      if (!svg.second.empty() && svg.second[0] != '"')
      {
        svg.second.insert(0, "\"");
        svg.second.append("\"");
      }
    }
    for (auto& svg : theGeometryStorage.itsPolygons)
    {
      if (!svg.second.empty() && svg.second[0] != '"')
      {
        svg.second.insert(0, "\"");
        svg.second.append("\"");
      }
    }
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
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
