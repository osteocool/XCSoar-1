/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "LiftDatabaseComputer.hpp"
#include "Engine/Navigation/TraceHistory.hpp"
#include "NMEA/LiftDatabase.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/CirclingInfo.hpp"

void
LiftDatabaseComputer::Clear(LiftDatabase &lift_database,
                            TraceVariableHistory &circling_average_trace)
{
  lift_database.Clear();

  circling_average_trace.clear();
}

void
LiftDatabaseComputer::Reset(LiftDatabase &lift_database,
                            TraceVariableHistory &circling_average_trace)
{
  last_circling = false;
  last_heading = Angle::Zero();

  Clear(lift_database, circling_average_trace);
}

/**
 * This function converts a heading into an unsigned index for the LiftDatabase.
 *
 * This is calculated with Angles to deal with the 360 degree limit.
 *
 * 357 = 0
 * 4 = 0
 * 5 = 1
 * 14 = 1
 * 15 = 2
 * ...
 * @param heading The heading to convert
 * @return The index for the LiftDatabase array
 */
static unsigned
heading_to_index(Angle &heading)
{
  static const Angle afive = Angle::Degrees(fixed(5));

  unsigned index = (unsigned)
      floor((heading + afive).AsBearing().Degrees() / 10);

  return std::max(0u, std::min(35u, index));
}

void
LiftDatabaseComputer::Compute(LiftDatabase &lift_database,
                              TraceVariableHistory &circling_average_trace,
                              const MoreData &basic,
                              const CirclingInfo &circling_info)
{
  // If we just started circling
  // -> reset the database because this is a new thermal
  if (!circling_info.circling && last_circling)
    Clear(lift_database, circling_average_trace);

  // Determine the direction in which we are circling
  bool left = circling_info.TurningLeft();

  // Depending on the direction set the step size sign for the
  // following loop
  Angle heading_step = Angle::Degrees(fixed(left ? -10 : 10));

  const Angle heading = basic.attitude.heading;

  // Start at the last heading and add heading_step until the current heading
  // is reached. For each heading save the current lift value into the
  // LiftDatabase. Last and current heading are included since they are
  // a part of the ten degree interval most of the time.
  //
  // This is done with Angles to deal with the 360 degrees limit.
  // e.g. last heading 348 degrees, current heading 21 degrees
  //
  // The loop condition stops until the current heading is reached.
  // Depending on the circling direction the current heading will be
  // smaller or bigger then the last one, because of that negative() is
  // tested against the left variable.
  for (Angle h = last_heading;
       left == negative((heading - h).AsDelta().Degrees());
       h += heading_step) {
    unsigned index = heading_to_index(h);
    lift_database[index] = basic.brutto_vario;
  }

  // detect zero crossing
  if ((heading < Angle::QuarterCircle() &&
       last_heading.Degrees() > fixed_270) ||
      (last_heading < Angle::QuarterCircle() &&
       heading.Degrees() > fixed_270)) {

    fixed h_av = fixed_zero;
    for (auto i : lift_database)
      h_av += i;

    h_av /= lift_database.size();
    circling_average_trace.push(h_av);
  }

  last_circling = circling_info.circling;
  last_heading = basic.attitude.heading;
}
