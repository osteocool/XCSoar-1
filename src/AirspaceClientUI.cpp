#include "AirspaceClientUI.hpp"
#include "Airspace/Airspaces.hpp"
#include "Airspace/AirspaceWarningManager.hpp"
#include "AirspaceParser.hpp"
#include "RasterTerrain.h"

const AirspacesInterface::AirspaceVector 
AirspaceClientUI::scan_range(const GEOPOINT location,
                             const fixed range,
                             const AirspacePredicate &condition) const
{
  Poco::ScopedRWLock lock(mutex);
  return airspaces.scan_range(location, range, condition);
}

void 
AirspaceClientUI::visit_warnings(AirspaceWarningVisitor& visitor) const
{
  Poco::ScopedRWLock lock(mutex);
  return airspace_warning.visit_warnings(visitor);
}

void 
AirspaceClientUI::visit_within_range(const GEOPOINT &loc, 
                                     const fixed range,
                                     AirspaceVisitor& visitor) const
{
  Poco::ScopedRWLock lock(mutex);
  return airspaces.visit_within_range(loc, range, visitor);
}

void 
AirspaceClientUI::visit_intersecting(const GEOPOINT &loc, 
                                     const GeoVector &vec,
                                     AirspaceIntersectionVisitor& visitor) const
{
  Poco::ScopedRWLock lock(mutex);
  return airspaces.visit_intersecting(loc, vec, visitor);
}

void
AirspaceClientUI::clear()
{
  Poco::ScopedRWLock lock(mutex, true);
  airspace_warning.clear();
  airspaces.clear();
}

bool
AirspaceClientUI::read(char* path)
{
  Poco::ScopedRWLock lock(mutex, true);
  return ReadAirspace(airspaces, path);
}


void
AirspaceClientUI::finalise_after_loading(RasterTerrain* terrain,
                                         const AtmosphericPressure &press)
{
  Poco::ScopedRWLock lock(mutex, true);

  airspaces.optimise();
  airspaces.set_flight_levels(press);
    
  if (terrain != NULL) {
    terrain->Lock();
    airspaces.set_ground_levels(*terrain);
    terrain->Unlock();
  }
}


void 
AirspaceClientUI::lock() const
{
  mutex.readLock();
}

void 
AirspaceClientUI::unlock() const
{
  mutex.unlock();
}

unsigned 
AirspaceClientUI::size() const
{
  Poco::ScopedRWLock lock(mutex);
  return airspaces.size();
}


AirspacesInterface::AirspaceTree::const_iterator 
AirspaceClientUI::begin() const
{
  return airspaces.begin();
}

AirspacesInterface::AirspaceTree::const_iterator 
AirspaceClientUI::end() const
{
  return airspaces.end();
}

AirspaceWarning& 
AirspaceClientUI::get_warning(const AbstractAirspace& airspace)
{
  Poco::ScopedRWLock lock(mutex, true);
  return airspace_warning.get_warning(airspace);
}

const AirspaceWarning* 
AirspaceClientUI::get_warning(const unsigned index) const
{
  Poco::ScopedRWLock lock(mutex);
  return airspace_warning.get_warning(index);
}

size_t 
AirspaceClientUI::warning_size() const
{
  Poco::ScopedRWLock lock(mutex);
  return airspace_warning.size();
}

bool 
AirspaceClientUI::warning_empty() const
{
  Poco::ScopedRWLock lock(mutex);
  return airspace_warning.empty();
}

int 
AirspaceClientUI::get_warning_index(const AbstractAirspace& airspace) const
{
  Poco::ScopedRWLock lock(mutex);
  return airspace_warning.get_warning_index(airspace);
}

void 
AirspaceClientUI::acknowledge_day(const AbstractAirspace& airspace,
                                  const bool set)
{
  Poco::ScopedRWLock lock(mutex, true);
  airspace_warning.acknowledge_day(airspace, set);
}

void 
AirspaceClientUI::acknowledge_warning(const AbstractAirspace& airspace,
                                  const bool set)
{
  Poco::ScopedRWLock lock(mutex, true);
  airspace_warning.acknowledge_warning(airspace, set);
}

void 
AirspaceClientUI::acknowledge_inside(const AbstractAirspace& airspace,
                                  const bool set)
{
  Poco::ScopedRWLock lock(mutex, true);
  airspace_warning.acknowledge_inside(airspace, set);
}

bool 
AirspaceClientUI::airspace_empty() const
{
  Poco::ScopedRWLock lock(mutex);
  return airspaces.empty();
}
