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

#ifndef XCSOAR_REACH_RESULT_HPP
#define XCSOAR_REACH_RESULT_HPP

#include "Rough/RoughAltitude.hpp"

#include <stdint.h>
#include <stdlib.h>

/**
 * The result of a reach calculation.
 */
struct ReachResult {
  enum class Validity : uint8_t {
    INVALID,
    VALID,
    UNREACHABLE,
  };

  /**
   * The arrival altitude for straight glide, ignoring terrain
   * obstacles.
   */
  RoughAltitude direct;

  /**
   * The arrival altitude considering detour to avoid terrain
   * obstacles.  This attribute may only be used if #terrain_valid is
   * #VALID.
   */
  RoughAltitude terrain;

  /**
   * This attribute describes whether the #terrain attribute is valid.
   */
  Validity terrain_valid;

  void Clear() {
    direct = terrain = -1;
    terrain_valid = Validity::INVALID;
  }

  bool IsReachableDirect() const {
    return direct.IsPositive();
  }

  bool IsReachableTerrain() const {
    return terrain_valid == Validity::VALID && terrain.IsPositive();
  }

  bool IsDeltaConsiderable() const {
    if (terrain_valid != Validity::VALID)
      return false;

    const int delta = abs((int)direct - (int)terrain);
    return delta >= 10 && delta * 100 / (int)direct > 5;
  }

  bool IsReachRelevant() const {
    return terrain_valid == Validity::VALID && terrain != direct;
  }

  void Add(RoughAltitude delta) {
    direct += delta;
    terrain += delta;
  }

  void Subtract(RoughAltitude delta) {
    direct -= delta;
    terrain -= delta;
  }
};

#endif
