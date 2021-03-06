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

#ifndef XCSOAR_DEVICE_PARSER_HPP
#define XCSOAR_DEVICE_PARSER_HPP

#include "Math/fixed.hpp"

struct NMEAInfo;
struct BrokenDateTime;
class NMEAInputLine;
struct GeoPoint;
struct BrokenDate;

class NMEAParser
{
  bool ignore_checksum;
  static int start_day;
  fixed last_time;

public:
  bool real;

  bool use_geoid;

public:
  NMEAParser(bool ignore_checksum = false);

  /**
   * Resets the NMEAParser
   */
  void Reset();

  void SetReal(bool _real) {
    real = _real;
  }

  void SetIgnoreChecksum(bool _ignore_checksum) {
    ignore_checksum = _ignore_checksum;
  }

  void DisableGeoid() {
    use_geoid = true;
  }

  /**
   * Parses a provided NMEA String into a NMEA_INFO struct
   * @param line NMEA string
   * @param info NMEA_INFO output struct
   * @return Parsing success
   */
  bool ParseLine(const char *line, NMEAInfo &info);

public:
  /**
   * Calculates the checksum of the provided NMEA string and
   * compares it to the provided checksum
   * @param String NMEA string
   * @return True if checksum correct
   */
  static bool NMEAChecksum(const char *string);

  /**
   * Checks whether time has advanced since last call and
   * updates the last_time reference if necessary
   * @return True if time has advanced since last call
   */
  static bool TimeHasAdvanced(fixed this_time, fixed &last_time, NMEAInfo &info);

  /**
   * Calculates a seconds-based FixTime and corrects it
   * in case over passing the UTC midnight mark
   * @param FixTime NMEA format fix time (HHMMSS)
   * @param info NMEA_INFO struct to parse into
   * @return Seconds-based FixTime
   */
  static fixed TimeModify(fixed fix_time, BrokenDateTime &date_time,
                          bool date_available);

  static bool ReadGeoPoint(NMEAInputLine &line, GeoPoint &value_r);

  static bool ReadDate(NMEAInputLine &line, BrokenDate &date);

private:

  /**
   * Verifies the given fix time.  If it is smaller than LastTime, but
   * within a certain tolerance, the LastTime is returned, otherwise
   * the specified time is returned without modification.
   *
   * This is used to reduce quirks when the time stamps in GPGGA and
   * GPRMC are off by a second.  Without this workaround, XCSoar loses
   * the GPS fix every now and then, because GPRMC is ignored most of
   * the time.
   */
  gcc_pure
  fixed TimeAdvanceTolerance(fixed time) const;

  /**
   * Checks whether time has advanced since last call and
   * updates the GPS_info if necessary
   * @param ThisTime Current time
   * @param info NMEA_INFO struct to update
   * @return True if time has advanced since last call
   */
  bool TimeHasAdvanced(fixed this_time, NMEAInfo &info);

  /**
   * Parses a GLL sentence
   *
   * @param line A NMEAInputLine instance that can be used for parsing
   * @param info NMEA_INFO struct to parse into
   * @return Parsing success
   */
  bool GLL(NMEAInputLine &line, NMEAInfo &info);

  /**
   * Parses a GGA sentence
   *
   * @param line A NMEAInputLine instance that can be used for parsing
   * @param info NMEA_INFO struct to parse into
   * @return Parsing success
   */
  bool GGA(NMEAInputLine &line, NMEAInfo &info);

  /**
   * Parses a GSA sentence
   *
   * @param line A NMEAInputLine instance that can be used for parsing
   * @param info NMEA_INFO struct to parse into
   * @return Parsing success
   */
  bool GSA(NMEAInputLine &line, NMEAInfo &info);

  /**
   * Parses a RMC sentence
   *
   * @param line A NMEAInputLine instance that can be used for parsing
   * @param info NMEA_INFO struct to parse into
   * @return Parsing success
   */
  bool RMC(NMEAInputLine &line, NMEAInfo &info);

  /**
   * Parses a PGRMZ sentence (Garmin proprietary).
   *
   * @param line A NMEAInputLine instance that can be used for parsing
   * @param info NMEA_INFO struct to parse into
   * @return Parsing success
   */
  bool RMZ(NMEAInputLine &line, NMEAInfo &info);

  /**
   * Parses a PTAS1 sentence (Tasman Instruments proprietary).
   *
   * @param line A NMEAInputLine instance that can be used for parsing
   * @param info NMEA_INFO struct to parse into
   * @return Parsing success
   */
  static bool PTAS1(NMEAInputLine &line, NMEAInfo &info);
};

#endif
