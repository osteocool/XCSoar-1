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

#include "CirclingComputer.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "ComputerSettings.hpp"
#include "Math/LowPassFilter.hpp"

static const fixed MIN_TURN_RATE(4);
static const fixed CRUISE_CLIMB_SWITCH(15);
static const fixed CLIMB_CRUISE_SWITCH(10);

void
CirclingComputer::Reset()
{
  turn_rate_delta_time.Reset();
  turning_delta_time.Reset();
  percent_delta_time.Reset();

  ResetStats();
}

void
CirclingComputer::ResetStats()
{
  min_altitude = fixed_zero;
}

void
CirclingComputer::TurnRate(CirclingInfo &circling_info,
                           const NMEAInfo &basic,
                           const FlyingState &flight)
{
  if (!basic.time_available || !flight.flying) {
    circling_info.turn_rate = fixed_zero;
    circling_info.turn_rate_heading = fixed_zero;
    circling_info.turn_rate_smoothed = fixed_zero;
    return;
  }

  const fixed dt = turn_rate_delta_time.Update(basic.time,
                                               fixed_third, fixed_ten);
  if (negative(dt)) {
    circling_info.turn_rate = fixed_zero;
    circling_info.turn_rate_heading = fixed_zero;
    circling_info.turn_rate_smoothed = fixed_zero;
  }

  if (!positive(dt))
    return;

  circling_info.turn_rate =
    (basic.track - last_track).AsDelta().Degrees() / dt;
  circling_info.turn_rate_heading =
    (basic.attitude.heading - last_heading).AsDelta().Degrees() / dt;

  last_track = basic.track;
  last_heading = basic.attitude.heading;

  // JMW limit rate to 50 deg per second otherwise a big spike
  // will cause spurious lock on circling for a long time
  fixed turn_rate = max(fixed(-50), min(fixed(50), circling_info.turn_rate));

  // Make the turn rate more smooth using the LowPassFilter
  turn_rate = LowPassFilter(circling_info.turn_rate_smoothed,
                            turn_rate, fixed(0.3));
  circling_info.turn_rate_smoothed = turn_rate;
}

void
CirclingComputer::Turning(CirclingInfo &circling_info,
                          const MoreData &basic,
                          const FlyingState &flight,
                          const CirclingSettings &settings)
{
  // You can't be circling unless you're flying
  if (!basic.time_available || !flight.flying)
    return;

  const fixed dt = turning_delta_time.Update(basic.time,
                                             fixed_zero, fixed_zero);
  if (!positive(dt))
    return;

  circling_info.turning =
    fabs(circling_info.turn_rate_smoothed) >= MIN_TURN_RATE;

  // Force cruise or climb mode if external device says so
  bool force_cruise = false;
  bool force_circling = false;
  if (settings.external_trigger_cruise_enabled && !basic.gps.replay) {
    switch (basic.switch_state.flight_mode) {
    case SwitchInfo::FlightMode::UNKNOWN:
      force_circling = false;
      force_cruise = false;
      break;

    case SwitchInfo::FlightMode::CIRCLING:
      force_circling = true;
      force_cruise = false;
      break;

    case SwitchInfo::FlightMode::CRUISE:
      force_circling = false;
      force_cruise = true;
      break;
    }
  }

  switch (circling_info.turn_mode) {
  case CirclingMode::CRUISE:
    // If (in cruise mode and beginning of circling detected)
    if (circling_info.turning || force_circling) {
      // Remember the start values of the turn
      turn_start_time = basic.time;
      turn_start_location = basic.location;
      turn_start_altitude = basic.nav_altitude;
      turn_start_energy_height = basic.energy_height;
      circling_info.turn_mode = CirclingMode::POSSIBLE_CLIMB;
    }
    if (!force_circling)
      break;

  case CirclingMode::POSSIBLE_CLIMB:
    if (force_cruise) {
      circling_info.turn_mode = CirclingMode::CRUISE;
      break;
    }
    if (circling_info.turning || force_circling) {
      if (((basic.time - turn_start_time) > CRUISE_CLIMB_SWITCH)
          || force_circling) {
        // yes, we are certain now that we are circling
        circling_info.circling = true;

        // JMW Transition to climb
        circling_info.turn_mode = CirclingMode::CLIMB;

        // Remember the start values of the climbing period
        circling_info.climb_start_location = turn_start_location;
        circling_info.climb_start_altitude = turn_start_altitude;
        circling_info.climb_start_altitude_te =
          turn_start_altitude + turn_start_energy_height;
        circling_info.climb_start_time = turn_start_time;
      }
    } else {
      // nope, not turning, so go back to cruise
      circling_info.turn_mode = CirclingMode::CRUISE;
    }
    break;

  case CirclingMode::CLIMB:
    if (!circling_info.turning || force_cruise) {
      // Remember the end values of the turn
      turn_start_time = basic.time;
      turn_start_location = basic.location;
      turn_start_altitude = basic.nav_altitude;
      turn_start_energy_height = basic.energy_height;

      // JMW Transition to cruise, due to not properly turning
      circling_info.turn_mode = CirclingMode::POSSIBLE_CRUISE;
    }
    if (!force_cruise)
      break;

  case CirclingMode::POSSIBLE_CRUISE:
    if (force_circling) {
      circling_info.turn_mode = CirclingMode::CLIMB;
      break;
    }

    if (!circling_info.turning || force_cruise) {
      if (basic.time - turn_start_time > CLIMB_CRUISE_SWITCH || force_cruise) {
        // yes, we are certain now that we are cruising again
        circling_info.circling = false;

        // Transition to cruise
        circling_info.turn_mode = CirclingMode::CRUISE;
        circling_info.cruise_start_location = turn_start_location;
        circling_info.cruise_start_altitude = turn_start_altitude;
        circling_info.cruise_start_time = turn_start_time;
      }
    } else {
      // nope, we are circling again
      // JMW Transition back to climb, because we are turning again
      circling_info.turn_mode = CirclingMode::CLIMB;
    }
    break;
  }
}

void
CirclingComputer::PercentCircling(const MoreData &basic,
                                  CirclingInfo &circling_info)
{
  if (!basic.time_available)
    return;

  // JMW circling % only when really circling,
  // to prevent bad stats due to flap switches and dolphin soaring

  const fixed dt = percent_delta_time.Update(basic.time,
                                             fixed_zero, fixed_zero);
  if (!positive(dt))
    return;

  // if (Circling)
  if (circling_info.circling && circling_info.turning) {
    // Add one second to the circling time
    // timeCircling += (Basic->Time-LastTime);
    circling_info.time_climb += dt;

    // Add the Vario signal to the total climb height
    circling_info.total_height_gain += basic.gps_vario;
  } else {
    // Add one second to the cruise time
    // timeCruising += (Basic->Time-LastTime);
    circling_info.time_cruise += dt;
  }

  // Calculate the circling percentage
  if (circling_info.time_cruise + circling_info.time_climb > fixed_one)
    circling_info.circling_percentage = 100 * circling_info.time_climb /
        (circling_info.time_cruise + circling_info.time_climb);
  else
    circling_info.circling_percentage = fixed_minus_one;
}

void
CirclingComputer::MaxHeightGain(const MoreData &basic,
                                const FlyingState &flight,
                                CirclingInfo &circling_info)
{
  if (!basic.NavAltitudeAvailable() || !flight.flying)
    return;

  if (positive(min_altitude)) {
    fixed height_gain = basic.nav_altitude - min_altitude;
    circling_info.max_height_gain = max(height_gain, circling_info.max_height_gain);
  } else {
    min_altitude = basic.nav_altitude;
  }

  min_altitude = min(basic.nav_altitude, min_altitude);
}
