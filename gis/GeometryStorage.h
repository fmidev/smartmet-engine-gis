// ======================================================================
/*!
 * \file
 * \brief Interface of class
 */
// ======================================================================

#pragma once

#include <macgyver/DateTime.h>
#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>
#include <gis/OGR.h>
#include <macgyver/StringConversion.h>
#include <list>
#include <map>
#include <string>

namespace SmartMet
{
namespace Engine
{
namespace Gis
{
class Engine;
struct postgis_identifier
{
  std::string source_name;
  std::string pgname;
  std::string schema;
  std::string table;
  std::string field;
  std::string key() const
  {
    return (source_name + ";" + pgname + ";" + schema + ";" + table + ";" + field);
  }
  bool allFieldsDefined() const
  {
    return (!pgname.empty() && !schema.empty() && !table.empty() && !field.empty());
  }
};

struct AttributeToString : public boost::static_visitor<std::string>
{
  std::string operator()(const std::string& s) const { return s; }
  std::string operator()(double d) const { return Fmi::to_string(d); }
  std::string operator()(int i) const { return Fmi::to_string(i); }
  std::string operator()(const Fmi::DateTime& t) const
  {
    return Fmi::date_time::to_iso_string(t);
  }
};

using PostGISIdentifierVector = std::vector<postgis_identifier>;
using NameOGRGeometryMap = std::map<std::string, boost::shared_ptr<OGRGeometry> >;

class GeometryStorage
{
 public:
  bool geoObjectExists(const std::string& name) const;
  bool isPolygon(const std::string& name) const;
  bool isLine(const std::string& name) const;
  bool isPoint(const std::string& name) const;
  std::string getSVGPath(const std::string& name) const;
  std::pair<double, double> getPoint(const std::string& name) const;
  std::list<std::string> areaNames() const;

  const OGRGeometry* getOGRGeometry(const std::string& name, int type) const;

 private:
  std::map<std::string, std::string> itsPolygons;
  std::map<std::string, std::string> itsLines;
  std::map<std::string, std::pair<double, double> > itsPoints;
  // OGR geometries are mapped by type and name
  // name is not unambiguous: e.g. there can be a Point and Polygon for Helsinki

  std::map<int, NameOGRGeometryMap> itsGeometries;  // int == OGRwkbGeometryType
  std::map<std::string, int> itsQueryParameters;

  friend class SmartMet::Engine::Gis::Engine;
};  // class GeometryStorage

}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet

// ======================================================================
