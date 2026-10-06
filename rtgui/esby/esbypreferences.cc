/*
 *  This file is part of RawTherapee.
 *
 *  RawTherapee is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  RawTherapee is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RawTherapee.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "esbypreferences.h"
#include "multilangmgr.h"
#include "windows/preferences.h"
#include "soundman.h" //needed for testing sound.

// sound test buttons of the sounds tab (Preferences methods, the buttons are created in getSoundsPanel())
void  Preferences::btn_test_batchqueuedone_clicked (GdkEventButton* event)
{
  SoundManager::playSoundAsync(txtSndBatchQueueDone->get_text ());
}

void  Preferences::btn_test_lngedit_clicked (GdkEventButton* event)
{
  SoundManager::playSoundAsync(txtSndLngEditProcDone->get_text ());
}

Gtk::Widget* EsbyPreferencesPanel::getTTPanel ()
{
  Gtk::VBox* pTTP = Gtk::manage(new Gtk::VBox ());

  themeBox1 = Gtk::manage(new Gtk::HBox());
  themeBox1->set_spacing(4);
  themeBox2 = Gtk::manage(new Gtk::HBox());
  themeBox2->set_spacing(4);
  themeBox3 = Gtk::manage(new Gtk::HBox());
  themeBox3->set_spacing(4);
  themeBox4 = Gtk::manage(new Gtk::HBox());
  themeBox4->set_spacing(4);
  themeBox5 = Gtk::manage(new Gtk::HBox());
  themeBox5->set_spacing(4);
  themeBox6 = Gtk::manage(new Gtk::HBox());
  themeBox6->set_spacing(4);
  themeBox7 = Gtk::manage(new Gtk::HBox());
  themeBox7->set_spacing(4);
  themeBox8 = Gtk::manage(new Gtk::HBox());
  themeBox8->set_spacing(4);
  themeBox9 = Gtk::manage(new Gtk::HBox());
  themeBox9->set_spacing(4);
  themeBox10 = Gtk::manage(new Gtk::HBox());
  themeBox10->set_spacing(4);
  themeBox11 = Gtk::manage(new Gtk::HBox());
  themeBox11->set_spacing(4);


  lbHideFavorite = Gtk::manage(new Gtk::Label(M("TP_THEMETOOL_HIDE_FAVORITE")));
  cbHideFavorite = Gtk::manage(new Gtk::CheckButton());

  lbHideExposure = Gtk::manage(new Gtk::Label(M("TP_THEMETOOL_HIDE_EXPOSURE")));
  cbHideExposure = Gtk::manage(new Gtk::CheckButton());

  lbHideDetails = Gtk::manage(new Gtk::Label(M("TP_THEMETOOL_HIDE_DETAIL")));
  cbHideDetails = Gtk::manage(new Gtk::CheckButton());

  lbHideColor = Gtk::manage(new Gtk::Label(M("TP_THEMETOOL_HIDE_COLOR")));
  cbHideColor = Gtk::manage(new Gtk::CheckButton());

  lbHideAdvanced = Gtk::manage(new Gtk::Label(M("TP_THEMETOOL_HIDE_ADVANCED")));
  cbHideAdvanced = Gtk::manage(new Gtk::CheckButton());

  lbHideLocal = Gtk::manage(new Gtk::Label(M("TP_THEMETOOL_HIDE_LOCAL")));
  cbHideLocal = Gtk::manage(new Gtk::CheckButton());

  lbHideTransform = Gtk::manage(new Gtk::Label(M("TP_THEMETOOL_HIDE_TRANSFORM")));
  cbHideTransform = Gtk::manage(new Gtk::CheckButton());

  lbHideRaw = Gtk::manage(new Gtk::Label(M("TP_THEMETOOL_HIDE_RAW")));
  cbHideRaw = Gtk::manage(new Gtk::CheckButton());

  lbHideMetadata = Gtk::manage(new Gtk::Label(M("TP_THEMETOOL_HIDE_METADATA")));
  cbHideMetadata = Gtk::manage(new Gtk::CheckButton());

  lbHideUseful = Gtk::manage(new Gtk::Label(M("TP_THEMETOOL_HIDE_USEFUL")));
  cbHideUseful = Gtk::manage(new Gtk::CheckButton());

  lbHideTrash = Gtk::manage(new Gtk::Label(M("TP_THEMETOOL_HIDE_TRASH")));
  cbHideTrash = Gtk::manage(new Gtk::CheckButton());

  themeBox1->pack_start(*cbHideFavorite, Gtk::PACK_SHRINK, 0);
  themeBox1->pack_start(*lbHideFavorite, Gtk::PACK_SHRINK, 0);
  themeBox2->pack_start(*cbHideExposure, Gtk::PACK_SHRINK, 0);
  themeBox2->pack_start(*lbHideExposure, Gtk::PACK_SHRINK, 0);
  themeBox3->pack_start(*cbHideDetails, Gtk::PACK_SHRINK, 0);
  themeBox3->pack_start(*lbHideDetails, Gtk::PACK_SHRINK, 0);
  themeBox4->pack_start(*cbHideColor, Gtk::PACK_SHRINK, 0);
  themeBox4->pack_start(*lbHideColor, Gtk::PACK_SHRINK, 0);
  themeBox5->pack_start(*cbHideAdvanced, Gtk::PACK_SHRINK, 0);
  themeBox5->pack_start(*lbHideAdvanced, Gtk::PACK_SHRINK, 0);
  themeBox6->pack_start(*cbHideLocal, Gtk::PACK_SHRINK, 0);
  themeBox6->pack_start(*lbHideLocal, Gtk::PACK_SHRINK, 0);
  themeBox7->pack_start(*cbHideTransform, Gtk::PACK_SHRINK, 0);
  themeBox7->pack_start(*lbHideTransform, Gtk::PACK_SHRINK, 0);
  themeBox8->pack_start(*cbHideRaw, Gtk::PACK_SHRINK, 0);
  themeBox8->pack_start(*lbHideRaw, Gtk::PACK_SHRINK, 0);
  themeBox9->pack_start(*cbHideMetadata, Gtk::PACK_SHRINK, 0);  
  themeBox9->pack_start(*lbHideMetadata, Gtk::PACK_SHRINK, 0);
  themeBox10->pack_start(*cbHideUseful, Gtk::PACK_SHRINK, 0);  
  themeBox10->pack_start(*lbHideUseful, Gtk::PACK_SHRINK, 0);
  themeBox11->pack_start(*cbHideTrash, Gtk::PACK_SHRINK, 0);
  themeBox11->pack_start(*lbHideTrash, Gtk::PACK_SHRINK, 0);

  pTTP->pack_start(*themeBox1, Gtk::PACK_SHRINK, 0);
  pTTP->pack_start(*themeBox2, Gtk::PACK_SHRINK, 0);
  pTTP->pack_start(*themeBox3, Gtk::PACK_SHRINK, 0);
  pTTP->pack_start(*themeBox4, Gtk::PACK_SHRINK, 0);
  pTTP->pack_start(*themeBox5, Gtk::PACK_SHRINK, 0);
  pTTP->pack_start(*themeBox6, Gtk::PACK_SHRINK, 0);
  pTTP->pack_start(*themeBox7, Gtk::PACK_SHRINK, 0);
  pTTP->pack_start(*themeBox8, Gtk::PACK_SHRINK, 0);
  pTTP->pack_start(*themeBox9, Gtk::PACK_SHRINK, 0);
  pTTP->pack_start(*themeBox10, Gtk::PACK_SHRINK, 0);
  pTTP->pack_start(*themeBox11, Gtk::PACK_SHRINK, 0);

  pTTP->set_border_width (4);

  return pTTP;
}

void EsbyPreferencesPanel::fillPreferences(const EsbySettings& settings)
{
   //todo define TTP code
    cbHideFavorite->set_active (settings.TTPHideFavorite);
    cbHideExposure->set_active (settings.TTPHideExposure);
    cbHideDetails->set_active (settings.TTPHideDetails);
    cbHideColor->set_active (settings.TTPHideColor);
    cbHideAdvanced->set_active (settings.TTPHideAdvanced);
    cbHideLocal->set_active (settings.TTPHideLocal);
    cbHideTransform->set_active (settings.TTPHideTransform);
    cbHideRaw->set_active (settings.TTPHideRaw);
    cbHideMetadata->set_active (settings.TTPHideMetadata);
    cbHideUseful->set_active (settings.TTPHideUseful);
    // was missing: the trash checkbox was always unchecked, and saving the preferences showed the trash tab again
    cbHideTrash->set_active (settings.TTPHideTrash);
}

void EsbyPreferencesPanel::storePreferences(EsbySettings& settings)
{
   //todo define TTP code here.
     settings.TTPHideFavorite =  cbHideFavorite->get_active ();
     settings.TTPHideExposure =  cbHideExposure->get_active ();
     settings.TTPHideDetails =  cbHideDetails->get_active ();
     settings.TTPHideColor =  cbHideColor->get_active ();
     settings.TTPHideAdvanced =  cbHideAdvanced->get_active ();
     settings.TTPHideLocal =  cbHideLocal->get_active ();
     settings.TTPHideTransform =  cbHideTransform->get_active ();
     settings.TTPHideRaw =  cbHideRaw->get_active ();
     settings.TTPHideMetadata =  cbHideMetadata->get_active ();
     settings.TTPHideUseful =  cbHideUseful->get_active ();
     settings.TTPHideTrash =  cbHideTrash->get_active ();
}
