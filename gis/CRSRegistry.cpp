#include "CRSRegistry.h"
#include <boost/foreach.hpp>
#include <boost/spirit/include/qi.hpp>
#include <gdal/ogr_srs_api.h>
#include <macgyver/StringConversion.h>
#include <macgyver/TypeName.h>
#include <spine/Exception.h>
#include <stdexcept>

namespace ba = boost::algorithm;
namespace qi = boost::spirit::qi;
namespace ac = boost::spirit::ascii;

namespace SmartMet
{
namespace Engine
{
namespace Gis
{
/**
 *   @brief [INTERNAL] Identity CRS transformation
 */
class CRSRegistry::IdentityTransformation : public CRSRegistry::Transformation
{
  const std::string crs_name;

 public:
  IdentityTransformation(const std::string& crs_name);

  virtual ~IdentityTransformation();

  virtual std::string get_src_name() const;

  virtual std::string get_dest_name() const;

  virtual NFmiPoint transform(const NFmiPoint& src);

  virtual boost::array<double, 3> transform(const boost::array<double, 3>& src);

  virtual void transform(OGRGeometry& geometry);
};

/**
 *   @brief [INTERNAL] CRS transformation implementation
 */
class CRSRegistry::TransformationImpl : public CRSRegistry::Transformation
{
  const std::string from_name;
  const std::string to_name;
  bool swap1, swap2;
  OGRCoordinateTransformation* conv;

 public:
  TransformationImpl(const CRSRegistry::MapEntry& from, const CRSRegistry::MapEntry& to);

  virtual ~TransformationImpl();

  virtual std::string get_src_name() const;

  virtual std::string get_dest_name() const;

  virtual NFmiPoint transform(const NFmiPoint& src);

  virtual boost::array<double, 3> transform(const boost::array<double, 3>& src);

  virtual void transform(OGRGeometry& geometry);
};

#define CHECK_NAME(name)   \
  if (crs_map.count(name)) \
    throw SmartMet::Spine::Exception(BCP, "Duplicate name of coordinate system (" + name + ")!");

CRSRegistry::CRSRegistry()
{
}

CRSRegistry::~CRSRegistry()
{
}

void CRSRegistry::register_epsg(const std::string& name,
                                int epsg_code,
                                boost::optional<std::string> regex,
                                bool swap_coord)
{
  try
  {
    SmartMet::Spine::WriteLock lock(rw_lock);
    const std::string nm = Fmi::ascii_tolower_copy(name);
    CHECK_NAME(nm);

    MapEntry entry(name, regex);
    if (entry.cs->importFromEPSG(epsg_code) != OGRERR_NONE)
    {
      throw SmartMet::Spine::Exception(BCP, "Failed to register projection EPSG:" + epsg_code);
    }
    entry.swap_coord = swap_coord;

    entry.attrib_map["epsg"] = epsg_code;
    entry.attrib_map["swapCoord"] = swap_coord;

    crs_map.insert(std::make_pair(nm, entry));
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

void CRSRegistry::register_proj4(const std::string& name,
                                 const std::string& proj4_def,
                                 boost::optional<std::string> regex,
                                 bool swap_coord)
{
  try
  {
    SmartMet::Spine::WriteLock lock(rw_lock);

    const std::string nm = Fmi::ascii_tolower_copy(name);
    CHECK_NAME(nm);

    MapEntry entry(name, regex);
    if (entry.cs->importFromProj4(proj4_def.c_str()) != OGRERR_NONE)
    {
      throw SmartMet::Spine::Exception(BCP,
                                       "Failed to parse PROJ.4 definition '" + proj4_def + "'!");
    }

    entry.swap_coord = swap_coord;

    entry.attrib_map["swapCoord"] = swap_coord;

    crs_map.insert(std::make_pair(nm, entry));
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

void CRSRegistry::register_wkt(const std::string& name,
                               const std::string& wkt_def,
                               boost::optional<std::string> regex,
                               bool swap_coord)
{
  try
  {
    SmartMet::Spine::WriteLock lock(rw_lock);

    const std::string nm = Fmi::ascii_tolower_copy(name);
    CHECK_NAME(nm);

    MapEntry entry(name, regex);
    char* str = const_cast<char*>(wkt_def.c_str());

    int ret = entry.cs->importFromWkt(&str);

    if (ret == OGRERR_NONE)
    {
      // Check whether there is some garbage after WKT
      for (; *str and std::isspace(*str); str++)
      {
      }

      if (*str == 0)
      {
        crs_map.insert(std::make_pair(nm, entry));
        return;
      }
    }

    entry.attrib_map["swapCoord"] = swap_coord;

    entry.swap_coord = swap_coord;

    throw SmartMet::Spine::Exception(BCP, "Failed to parse PROJ.4 definition '" + wkt_def + "'!");
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

boost::shared_ptr<CRSRegistry::Transformation> CRSRegistry::create_transformation(
    const std::string& from, const std::string& to)
{
  try
  {
    const MapEntry& e_from = get_entry(from);
    const MapEntry& e_to = get_entry(to);
    boost::shared_ptr<Transformation> result;

    if (&e_from == &e_to)
    {
      result.reset(new IdentityTransformation(e_from.name));
    }
    else
    {
      result.reset(new TransformationImpl(e_from, e_to));
    }

    return result;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

SmartMet::Spine::BoundingBox CRSRegistry::convert_bbox(const SmartMet::Spine::BoundingBox& src,
                                                       const std::string& to)
{
  try
  {
    const MapEntry& e_from = get_entry(src.crs);
    const MapEntry& e_to = get_entry(to);

    if (&e_from == &e_to)
    {
      return src;
    }

    TransformationImpl transformation(e_from, e_to);

    const int NPOINTS = 10;
    OGRLinearRing boundary;

    double xStep = (src.xMax - src.xMin) / (NPOINTS - 1);
    double yStep = (src.yMax - src.yMin) / (NPOINTS - 1);

    for (int i = 0; i < NPOINTS; i++)
      boundary.addPoint(src.xMin, src.yMin + i * yStep);
    for (int i = 1; i < NPOINTS; i++)
      boundary.addPoint(src.xMin + i * xStep, src.yMax);
    for (int i = 1; i < NPOINTS; i++)
      boundary.addPoint(src.xMax, src.yMax - i * yStep);
    for (int i = 1; i < NPOINTS; i++)
      boundary.addPoint(src.xMax - i * xStep, src.yMin);

    boundary.closeRings();

    OGRPolygon polygon;
    polygon.addRing(&boundary);

    transformation.transform(polygon);

    OGREnvelope envelope;
    polygon.getEnvelope(&envelope);

    SmartMet::Spine::BoundingBox result;
    result.xMin = envelope.MinX;
    result.yMin = envelope.MinY;
    result.xMax = envelope.MaxX;
    result.yMax = envelope.MaxY;
    result.crs = to;

    return result;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

std::string CRSRegistry::get_proj4(const std::string& name)
{
  try
  {
    char* tmp = NULL;
    auto& entry = get_entry(name);
    entry.cs->exportToProj4(&tmp);
    std::string result(tmp);
    OGRFree(tmp);
    return result;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

void CRSRegistry::dump_info(std::ostream& output)
{
  try
  {
    BOOST_FOREACH (const auto& item, crs_map)
    {
      output << "CRSRegistry: name='" << item.second.name << "' regex='" << item.second.regex
             << "' proj4='" << get_proj4(item.second.name) << "'" << std::endl;
    }
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

std::vector<std::string> CRSRegistry::get_crs_keys() const
{
  try
  {
    std::vector<std::string> result;
    BOOST_FOREACH (const auto& item, crs_map)
    {
      result.push_back(item.first);
    }
    return result;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

CRSRegistry::MapEntry& CRSRegistry::get_entry(const std::string& name)
{
  try
  {
    SmartMet::Spine::ReadLock lock(rw_lock);
    const std::string name_lower = Fmi::ascii_tolower_copy(name);
    auto it = crs_map.find(name_lower);

    if (it != crs_map.end())
    {
      // Found entry by name ==> return it
      return it->second;
    }
    else
    {
      const std::string opengisPrefix = "http://www.opengis.net/def/crs/epsg/";
      bool isOpengis = qi::phrase_parse(
          name_lower.begin(),
          name_lower.end(),
          qi::string(opengisPrefix) >> qi::ushort_ >> '/' >> qi::ushort_ >> qi::eps,
          ac::space);
      std::string epsgCode = "EPSG::";

      if (isOpengis)
      {
        std::string::size_type pos = name_lower.find_last_of('/');
        if (pos != std::string::npos)
        {
          epsgCode.append(name_lower.substr(pos + 1));
        }
        else
          isOpengis = false;
      }

      // Not found by name: try searching using regex match
      BOOST_FOREACH (auto& item, crs_map)
      {
        if (not item.second.regex.empty())
        {
          if (isOpengis and boost::regex_match(epsgCode, item.second.regex))
          {
            return item.second;
          }
          else if (boost::regex_match(name, item.second.regex))
          {
            return item.second;
          }
        }
      }

      // Still not found ==> throw an error
      throw SmartMet::Spine::Exception(BCP, "Coordinate system '" + name + "' not found!");
    }
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

void CRSRegistry::handle_get_attribute_error(const std::string& crs_name,
                                             const std::string& attrib_name,
                                             const std::type_info& actual_type,
                                             const std::type_info& expected_type)
{
  try
  {
    throw SmartMet::Spine::Exception(
        BCP, "Type mismatch of attribute '" + attrib_name + "' for CRS '" + crs_name + "'!")
        .addParameter("Expected", Fmi::demangle_cpp_type_name(expected_type.name()))
        .addParameter("Found", Fmi::demangle_cpp_type_name(actual_type.name()));
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

CRSRegistry::Transformation::Transformation()
{
}

CRSRegistry::Transformation::~Transformation()
{
}

CRSRegistry::MapEntry::MapEntry(const std::string& theName, boost::optional<std::string> text)
    : name(theName), cs(new OGRSpatialReference)
{
  try
  {
    if (text)
    {
      try
      {
        regex.assign(*text, boost::regex_constants::perl | boost::regex_constants::icase);
      }
      catch (...)
      {
        std::cerr << METHOD_NAME << ": failed to parse PERL regular expression '" << text << "'"
                  << std::endl;

        SmartMet::Spine::Exception exception(BCP, "Failed to parse PERL regular expression");
        if (text)
          exception.addParameter("text", *text);
        throw exception;
      }
    }
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

CRSRegistry::IdentityTransformation::IdentityTransformation(const std::string& theCrsName)
    : crs_name(theCrsName)
{
}

CRSRegistry::IdentityTransformation::~IdentityTransformation()
{
}
std::string CRSRegistry::IdentityTransformation::get_src_name() const
{
  return crs_name;
}

std::string CRSRegistry::IdentityTransformation::get_dest_name() const
{
  return crs_name;
}

NFmiPoint CRSRegistry::IdentityTransformation::transform(const NFmiPoint& src)
{
  return src;
}

boost::array<double, 3> CRSRegistry::IdentityTransformation::transform(
    const boost::array<double, 3>& src)
{
  return src;
}

void CRSRegistry::IdentityTransformation::transform(OGRGeometry& geometry)
{
  (void)geometry;
}

CRSRegistry::TransformationImpl::TransformationImpl(const CRSRegistry::MapEntry& from,
                                                    const CRSRegistry::MapEntry& to)
    : from_name(from.name),
      to_name(to.name),
      swap1(from.swap_coord),
      swap2(to.swap_coord),
      conv(OGRCreateCoordinateTransformation(from.cs.get(), to.cs.get()))
{
  try
  {
    if (not conv)
    {
      throw SmartMet::Spine::Exception(
          BCP,
          "Failed to create coordinate transformation '" + from_name + "' to '" + to_name + "'!");
    }
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

CRSRegistry::TransformationImpl::~TransformationImpl()
{
  try
  {
    if (conv)
    {
      OGRCoordinateTransformation::DestroyCT(conv);
    }
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

std::string CRSRegistry::TransformationImpl::get_src_name() const
{
  return from_name;
}

std::string CRSRegistry::TransformationImpl::get_dest_name() const
{
  return to_name;
}

NFmiPoint CRSRegistry::TransformationImpl::transform(const NFmiPoint& src)
{
  try
  {
    double x = swap1 ? src.Y() : src.X();
    double y = swap1 ? src.X() : src.Y();
    if (conv->Transform(1, &x, &y, NULL))
    {
      NFmiPoint result(swap2 ? y : x, swap2 ? x : y);
      return result;
    }
    else
    {
      std::ostringstream msg;
      msg << "Coordinate transformatiom from " << from_name << " to " << to_name << " failed for ("
          << src.X() << ", " << src.Y() << ")";
      throw SmartMet::Spine::Exception(BCP, msg.str());
    }
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

boost::array<double, 3> CRSRegistry::TransformationImpl::transform(
    const boost::array<double, 3>& src)
{
  try
  {
    double x = swap1 ? src[1] : src[0];
    double y = swap1 ? src[0] : src[1];
    double z = src[2];
    if (conv->Transform(1, &x, &y, &z))
    {
      boost::array<double, 3> result;
      result[0] = swap2 ? y : x;
      result[1] = swap2 ? x : y;
      result[2] = z;
      return result;
    }
    else
    {
      std::ostringstream msg;
      msg << "Coordinate transformatiom from " << from_name << " to " << to_name << " failed for ("
          << src[0] << ", " << src[1] << ", " << src[1] << ")";
      throw SmartMet::Spine::Exception(BCP, msg.str());
    }
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

void CRSRegistry::TransformationImpl::transform(OGRGeometry& geometry)
{
  try
  {
    OGRErr err = geometry.transform(conv);
    if (err != OGRERR_NONE)
    {
      std::ostringstream msg;
      char* gText = NULL;
      geometry.exportToWkt(&gText);
      msg << "Failed to transform geometry " << gText << " to " << to_name;
      OGRFree(gText);
      throw SmartMet::Spine::Exception(BCP, msg.str());
    }
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet
