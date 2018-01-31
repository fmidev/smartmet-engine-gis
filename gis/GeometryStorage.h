// ======================================================================
/*!
 * \file
 * \brief Interface of class
 */
// ======================================================================

#pragma once

#include <boost/date_time/posix_time/posix_time.hpp>
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
  std::string host;
  std::string port;
  std::string database;
  std::string username;
  std::string password;
  std::string schema;
  std::string table;
  std::string field;
  std::string encoding;
  std::string key()
  {
    return (host + ";" + port + ";" + database + ";" + username + ";" + password + ";" + schema +
            ";" + table + ";" + field + ";" + encoding);
  }
  bool allFieldsDefined()
  {
    return (!host.empty() && !port.empty() && !database.empty() && !username.empty() &&
            !password.empty() && !schema.empty() && !table.empty() && !field.empty() &&
            !encoding.empty());
  }
};

struct AttributeToString : public boost::static_visitor<std::string>
{
  std::string operator()(const std::string& s) const { return s; }
  std::string operator()(double d) const { return Fmi::to_string(d); }
  std::string operator()(int i) const { return Fmi::to_string(i); }
  std::string operator()(const boost::posix_time::ptime& t) const
  {
    return boost::posix_time::to_iso_string(t);
  }
};

typedef std::vector<postgis_identifier> PostGISIdentifierVector;
typedef std::map<std::string, boost::shared_ptr<OGRGeometry> > NameOGRGeometryMap;

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

  const OGRGeometry* getOGRGeometry(const std::string& name, OGRwkbGeometryType type) const;

 private:
  std::map<std::string, std::string> itsPolygons;
  std::map<std::string, std::string> itsLines;
  std::map<std::string, std::pair<double, double> > itsPoints;
  // OGR geometries are mapped by type and name
  // name is not unambiguous: e.g. there can be a Point and Polygon for Helsinki
  std::map<OGRwkbGeometryType, NameOGRGeometryMap> itsGeometries;
  std::map<std::string, int> itsQueryParameters;

  friend class SmartMet::Engine::Gis::Engine;
};  // class GeometryStorage

}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet

// ======================================================================