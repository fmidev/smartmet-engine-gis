#include "Normalize.h"
#include <boost/algorithm/string/replace.hpp>

namespace SmartMet
{
namespace Engine
{
namespace Gis
{
void normalize_string(std::string& str)
{
  // convert to lower case and skandinavian characters
  std::transform(str.begin(), str.end(), str.begin(), ::tolower);
  boost::algorithm::replace_all(str, "Ä", "a");
  boost::algorithm::replace_all(str, "ä", "a");
  boost::algorithm::replace_all(str, "Å", "a");
  boost::algorithm::replace_all(str, "å", "a");
  boost::algorithm::replace_all(str, "Ö", "o");
  boost::algorithm::replace_all(str, "ö", "o");
}
}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet
