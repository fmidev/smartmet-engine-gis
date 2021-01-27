#include "MetaData.h"
#include <boost/functional/hash.hpp>

namespace SmartMet
{
namespace Engine
{
namespace Gis
{
std::size_t MetaDataQueryOptions::hash_value() const
{
  auto hash = boost::hash_value(pgname);
  boost::hash_combine(hash, boost::hash_value(schema));
  boost::hash_combine(hash, boost::hash_value(table));
  boost::hash_combine(hash, boost::hash_value(geometry_column));
  if (time_column)
    boost::hash_combine(hash, boost::hash_value(*time_column));
  return hash;
}

}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet
