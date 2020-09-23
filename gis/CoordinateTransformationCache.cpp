#include "CoordinateTransformationCache.h"
#include "Engine.h"
#include <boost/functional/hash.hpp>

// 20 data projections times 20 output projections already implies 400 combinations
// Note that we do a linear search for an integer in a list, but it is much
// faster than creating a coordinate transformation. Also, the most popular
// transformations tend to be at the start of the list.

const int MaxCacheSize = 50 * 50;

namespace SmartMet
{
namespace Engine
{
namespace Gis
{
CoordinateTransformationCache::Deleter::Deleter(
    std::size_t theHash, std::weak_ptr<CoordinateTransformationCache *> theCache)
    : itsHash(theHash), itsCache(theCache)
{
}

void CoordinateTransformationCache::Deleter::operator()(OGRCoordinateTransformation *ptr)
{
  // Return the object to the pool if the cache still exists, otherwise delete it
  if (auto cache_ptr = itsCache.lock())
    (*cache_ptr.get())->add(itsHash, std::unique_ptr<OGRCoordinateTransformation>{ptr});
  else
    delete ptr;
}

CoordinateTransformationCache::CoordinateTransformationCache()
    : itsPointer(new CoordinateTransformationCache *(this))
{
}

bool CoordinateTransformationCache::empty() const
{
  Spine::ReadLock lock(itsMutex);
  return itsPool.empty();
}

std::size_t CoordinateTransformationCache::size() const
{
  Spine::ReadLock lock(itsMutex);
  return itsPool.size();
}

void CoordinateTransformationCache::add(std::size_t hash,
                                        std::unique_ptr<OGRCoordinateTransformation> obj)
{
  Spine::WriteLock lock(itsMutex);

  // Add as the most recently used to the start
  itsPool.push_front(std::make_pair(hash, std::move(obj)));

  // Pop the least recently used if the cache is too big
  if (itsPool.size() > MaxCacheSize)
    itsPool.pop_back();
}

CoordinateTransformationCache::Ptr CoordinateTransformationCache::get(const std::string &theSource,
                                                                      const std::string &theTarget,
                                                                      const Engine &theEngine)
{
  auto hash = boost::hash_value(theSource);
  boost::hash_combine(hash, boost::hash_value(theTarget));

  // Return transformation object from the pool if there is one
  {
    Spine::WriteLock lock(itsMutex);

    auto pos = std::find_if(itsPool.begin(), itsPool.end(), [hash](const CacheElement &element) {
      return hash == element.first;
    });

    if (pos != itsPool.end())
    {
      // std::cout << "\n" << theSource << " ---> " << theTarget << " from cache\n";

      // Take ownership from the CacheElement and return it to the user
      Ptr ret(pos->second.release(),
              Deleter(hash, std::weak_ptr<CoordinateTransformationCache *>{itsPointer}));
      itsPool.erase(pos);
      return std::move(ret);
    }
  }

  // Lock no longer needed since we create a new coordinate transformation object and return it
  // to the user directly.

  // std::cout << "\n" << theSource << " ---> " << theTarget << " created\n";

  auto src = theEngine.getSpatialReference(theSource);
  auto tgt = theEngine.getSpatialReference(theTarget);

  auto *ptr = OGRCreateCoordinateTransformation(src.get(), tgt.get());

  if (ptr == nullptr)
  {
    throw Fmi::Exception(BCP, "Failed to create coordinate transformation")
        .addParameter("Source", theSource)
        .addParameter("Target", theTarget);
  }

  return Ptr(ptr, Deleter(hash, std::weak_ptr<CoordinateTransformationCache *>{itsPointer}));
}

}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet

// ======================================================================
