/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "AltitudeInfo.hpp"
#include "Interface.hpp"
#include "Units/Units.hpp"
#include "Simulator.hpp"
#include "Dialogs/XML.hpp"
#include "Dialogs/dlgTools.h"
#include "Dialogs/dlgInfoBoxAccess.hpp"
#include "Form/TabBar.hpp"
#include "Form/Edit.hpp"

static bool
PnlInfoUpdate()
{
  const DerivedInfo &calculated = CommonInterface::Calculated();
  const NMEAInfo &basic = CommonInterface::Basic();
  TCHAR sTmp[32];

  if (!calculated.altitude_agl_valid) {
    ((WndProperty *)dlgInfoBoxAccess::GetWindowForm()->FindByName(_T("prpAltAGL")))->SetText(_("N/A"));
  } else {
    // Set Value
    _stprintf(sTmp, _T("%.0f %s"), (double)Units::ToUserAltitude(calculated.altitude_agl),
                                   Units::GetAltitudeName());

    ((WndProperty *)dlgInfoBoxAccess::GetWindowForm()->FindByName(_T("prpAltAGL")))->SetText(sTmp);
  }

  if (!basic.baro_altitude_available) {
    ((WndProperty *)dlgInfoBoxAccess::GetWindowForm()->FindByName(_T("prpAltBaro")))->SetText(_("N/A"));
  } else {
    // Set Value
    _stprintf(sTmp, _T("%.0f %s"), (double)Units::ToUserAltitude(basic.baro_altitude),
                                       Units::GetAltitudeName());

    ((WndProperty *)dlgInfoBoxAccess::GetWindowForm()->FindByName(_T("prpAltBaro")))->SetText(sTmp);
  }

  if (!basic.gps_altitude_available) {
    ((WndProperty *)dlgInfoBoxAccess::GetWindowForm()->FindByName(_T("prpAltGPS")))->SetText(_("N/A"));
  } else {
    // Set Value
     _stprintf(sTmp, _T("%.0f %s"), (double)Units::ToUserAltitude(basic.gps_altitude),
                                         Units::GetAltitudeName());

      ((WndProperty *)dlgInfoBoxAccess::GetWindowForm()->FindByName(_T("prpAltGPS")))->SetText(sTmp);
  }

  if (!calculated.terrain_valid){
    ((WndProperty *)dlgInfoBoxAccess::GetWindowForm()->FindByName(_T("prpTerrain")))->SetText(_("N/A"));
  } else {
    // Set Value
     _stprintf(sTmp, _T("%.0f %s"), (double)Units::ToUserAltitude(calculated.terrain_altitude),
                                         Units::GetAltitudeName());

      ((WndProperty *)dlgInfoBoxAccess::GetWindowForm()->FindByName(_T("prpTerrain")))->SetText(sTmp);
  }

  return true;
}

static void
OnTimerNotify(gcc_unused WndForm &Sender)
{
  PnlInfoUpdate();
}

bool
AltitudeInfoPreShow()
{
  return PnlInfoUpdate();
}

Window *
LoadAltitudeInfoPanel(SingleWindow &parent, TabBarControl *wTabBar,
                      WndForm *wf, int id)
{
  assert(wTabBar);
  assert(wf);

  Window *wInfoBoxAccessInfo =
    LoadWindow(NULL, wf, wTabBar->GetClientAreaWindow(),
               _T("IDR_XML_INFOBOXALTITUDEINFO"));
  assert(wInfoBoxAccessInfo);

  wf->SetTimerNotify(OnTimerNotify);

  return wInfoBoxAccessInfo;
}