/*
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2004-2010 Gabor Horvath <hgabor@rawtherapee.com>
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
#include "multilangmgr.h"
#include "toolpanelcoord.h"
#include "options.h"
#include "../rtengine/imagesource.h"
#include "../rtengine/dfmanager.h"
#include "../rtengine/ffmanager.h"
#include "../rtengine/improcfun.h"
#include "../rtengine/procevents.h"
#include "../rtengine/refreshmap.h"
#include "../rtexif/rtexif.h"

using namespace rtengine::procparams;

ToolPanelCoordinator::ToolPanelCoordinator (bool batch, bool benchmark) : ipc (nullptr), hasChanged (false), editDataProvider (nullptr)
{

    exposurePanel   = Gtk::manage (new ToolVBox ());
    detailsPanel    = Gtk::manage (new ToolVBox ());
    colorPanel      = Gtk::manage (new ToolVBox ());
    transformPanel  = Gtk::manage (new ToolVBox ());
    rawPanel        = Gtk::manage (new ToolVBox ());
    advancedPanel    = Gtk::manage (new ToolVBox ());
    favoritePanel   = Gtk::manage (new ToolVBox ());
    trashPanel      = Gtk::manage (new ToolVBox ());
    usefulPanel     = Gtk::manage (new ToolVBox ());

    favoritePanel->setBoxName(PANEL_NAME_FAVORITE);
    exposurePanel->setBoxName(PANEL_NAME_EXPOSURE);
    detailsPanel->setBoxName (PANEL_NAME_DETAILS);
    colorPanel->setBoxName(PANEL_NAME_COLOR);
    advancedPanel->setBoxName(PANEL_NAME_WAVELET);
    transformPanel->setBoxName(PANEL_NAME_TRANSFORM);
    rawPanel->setBoxName(PANEL_NAME_RAW);
    trashPanel->setBoxName(PANEL_NAME_TRASH);
    usefulPanel->setBoxName(PANEL_NAME_USEFUL);

    env =  new Environment(toolPanels, expList);
    isReaction = false;
    printf("environment created: #%i \n",env->getEnvRef());
    if (benchmark)
      env->setVar("benchmark", 1);

    favoritePanel->setEnvironment(env);
    exposurePanel->setEnvironment(env);
    detailsPanel->setEnvironment(env);
    colorPanel->setEnvironment(env);
    advancedPanel->setEnvironment(env);
    transformPanel->setEnvironment(env);
    rawPanel->setEnvironment(env);

    trashPanel->setEnvironment(env);
    usefulPanel->setEnvironment(env);
 
    env->setFavoritePanel(favoritePanel);
    env->setTrashPanel(trashPanel);

    coarse              = Gtk::manage (new CoarsePanel ());
    toneCurve           = Gtk::manage (new ToneCurve ());
    shadowshighlights   = Gtk::manage (new ShadowsHighlights ());
    impulsedenoise      = Gtk::manage (new ImpulseDenoise ());
    defringe            = Gtk::manage (new Defringe ());
    dirpyrdenoise       = Gtk::manage (new DirPyrDenoise ());
    epd                 = Gtk::manage (new EdgePreservingDecompositionUI ());
    sharpening          = Gtk::manage (new Sharpening ());
    localContrast       = Gtk::manage(new LocalContrast());
    sharpenEdge         = Gtk::manage (new SharpenEdge ());
    sharpenMicro        = Gtk::manage (new SharpenMicro ());
    lcurve              = Gtk::manage (new LCurve ());
    rgbcurves           = Gtk::manage (new RGBCurves ());
    colortoning         = Gtk::manage (new ColorToning ());
    lensgeom            = Gtk::manage (new LensGeometry ());
    lensProf            = Gtk::manage (new LensProfilePanel ());
    distortion          = Gtk::manage (new Distortion ());
    rotate              = Gtk::manage (new Rotate ());
    vibrance            = Gtk::manage (new Vibrance ());
    colorappearance     = Gtk::manage (new ColorAppearance ());
    whitebalance        = Gtk::manage (new WhiteBalance ());
    vignetting          = Gtk::manage (new Vignetting ());
    retinex             = Gtk::manage (new Retinex ());
    gradient            = Gtk::manage (new Gradient ());
    pcvignette          = Gtk::manage (new PCVignette ());
    perspective         = Gtk::manage (new PerspCorrection ());
    cacorrection        = Gtk::manage (new CACorrection ());
    chmixer             = Gtk::manage (new ChMixer ());
    blackwhite          = Gtk::manage (new BlackWhite ());
    resize              = Gtk::manage (new Resize ());
    prsharpening        = Gtk::manage (new PrSharpening());
    crop                = Gtk::manage (new Crop ());
    icm                 = Gtk::manage (new ICMPanel ());
    metadata            = Gtk::manage(new MetaDataPanel());
    wavelet             = Gtk::manage (new Wavelet ());
    dirpyrequalizer     = Gtk::manage (new DirPyrEqualizer ());
    hsvequalizer        = Gtk::manage (new HSVEqualizer ());
    filmSimulation      = Gtk::manage (new FilmSimulation ());
    softlight           = Gtk::manage(new SoftLight());
    dehaze              = Gtk::manage(new Dehaze());
    sensorbayer         = Gtk::manage (new SensorBayer ());
    sensorxtrans        = Gtk::manage (new SensorXTrans ());
    bayerprocess        = Gtk::manage (new BayerProcess ());
    xtransprocess       = Gtk::manage (new XTransProcess ());
    bayerpreprocess     = Gtk::manage (new BayerPreProcess ());
    preprocess          = Gtk::manage (new PreProcess ());
    darkframe           = Gtk::manage (new DarkFrame ());
    flatfield           = Gtk::manage (new FlatField ());
    rawcacorrection     = Gtk::manage (new RAWCACorr ());
    rawexposure         = Gtk::manage (new RAWExposure ());
    bayerrawexposure    = Gtk::manage (new BayerRAWExposure ());
    xtransrawexposure   = Gtk::manage (new XTransRAWExposure ());
    fattal              = Gtk::manage (new FattalToneMapping ());

    // So Demosaic, Line noise filter, Green Equilibration, Ca-Correction (garder le nom de section identique!) and Black-Level will be moved in a "Bayer sensor" tool,
    // and a separate Demosaic and Black Level tool will be created in an "X-Trans sensor" tool

    // X-Trans demozaic methods: "3-pass (best), 1-pass (medium), fast"
    // Mettre  jour les profils fournis pour inclure les nouvelles section Raw, notamment pour "Default High ISO"
    // Valeurs par dfaut:
    //     Best -> low ISO
    //     Medium -> High ISO

    addPanel (colorPanel, whitebalance);
    addPanel (exposurePanel, toneCurve);
    addPanel (colorPanel, vibrance);
    addPanel (colorPanel, chmixer);
    addPanel (colorPanel, blackwhite);
    addPanel (exposurePanel, shadowshighlights);
    addPanel (detailsPanel, sharpening);
    addPanel (detailsPanel, localContrast);
    addPanel (detailsPanel, sharpenEdge);
    addPanel (detailsPanel, sharpenMicro);
    addPanel (colorPanel, hsvequalizer);
    addPanel (colorPanel, filmSimulation);
    addPanel (colorPanel, softlight);
    addPanel (colorPanel, rgbcurves);
    addPanel (colorPanel, colortoning);
    addPanel (exposurePanel, epd);
    addPanel (exposurePanel, fattal);
    addPanel (advancedPanel, retinex);
    addPanel (exposurePanel, pcvignette);
    addPanel (exposurePanel, gradient);
    addPanel (exposurePanel, lcurve);
    addPanel (advancedPanel, colorappearance);
    addPanel (detailsPanel, impulsedenoise);
    addPanel (detailsPanel, dirpyrdenoise);
    addPanel (detailsPanel, defringe);
    addPanel (detailsPanel, dirpyrequalizer);
    addPanel (detailsPanel, dehaze);
    addPanel (advancedPanel, wavelet);
    addPanel (transformPanel, crop);
    addPanel (transformPanel, resize);
    addPanel (resize->getPackBox(), prsharpening, 2);
    addPanel (transformPanel, lensgeom);
    addPanel (lensgeom->getPackBox(), rotate, 2);
    addPanel (lensgeom->getPackBox(), perspective, 2);
    addPanel (lensgeom->getPackBox(), lensProf, 2);
    addPanel (lensgeom->getPackBox(), distortion, 2);
    addPanel (lensgeom->getPackBox(), cacorrection, 2);
    addPanel (lensgeom->getPackBox(), vignetting, 2);
    addPanel (colorPanel, icm);
    addPanel (rawPanel, sensorbayer);
    addPanel (sensorbayer->getPackBox(), bayerprocess, 2);
    addPanel (sensorbayer->getPackBox(), bayerrawexposure, 2);
    addPanel (sensorbayer->getPackBox(), bayerpreprocess, 2);
    addPanel (sensorbayer->getPackBox(), rawcacorrection, 2);
    addPanel (rawPanel, sensorxtrans);
    addPanel (sensorxtrans->getPackBox(), xtransprocess, 2);
    addPanel (sensorxtrans->getPackBox(), xtransrawexposure, 2);
    addPanel (rawPanel, rawexposure);
    addPanel (rawPanel, preprocess);
    addPanel (rawPanel, darkframe);
    addPanel (rawPanel, flatfield);

    toolPanels.push_back (coarse);
    toolPanels.push_back(metadata);
    coarse->setToolName("coarse"); // coase does not have a name.

  // new panels are registered a bit diffently
    addPanel (usefulPanel, Gtk::manage (new TTSaver()));
    addPanel (usefulPanel, Gtk::manage (new TTIsoProfiler()));
    addPanel (usefulPanel, Gtk::manage (new TTTabHider()));
    addPanel (usefulPanel, Gtk::manage (new TTFavoriteColorChooser()));
    addPanel (usefulPanel, Gtk::manage (new TTPanelColorChooser()));
    addPanel (usefulPanel, Gtk::manage (new TTUDLRHider()));
    addPanel (usefulPanel, Gtk::manage (new TTTweaker()));
    addPanel (usefulPanel, Gtk::manage (new TTVarDisplayer()));

    toolPanelNotebook = new Gtk::Notebook ();
    toolPanelNotebook->set_name ("ToolPanelNotebook");
    env->setToolPanelNotebook(toolPanelNotebook);


    exposurePanelSW    = Gtk::manage (new MyScrolledWindow ());
    detailsPanelSW     = Gtk::manage (new MyScrolledWindow ());
    colorPanelSW       = Gtk::manage (new MyScrolledWindow ());
    transformPanelSW   = Gtk::manage (new MyScrolledWindow ());
    rawPanelSW         = Gtk::manage (new MyScrolledWindow ());
    advancedPanelSW     = Gtk::manage (new MyScrolledWindow ());
    favoritePanelSW    = Gtk::manage (new MyScrolledWindow ()); 
    trashPanelSW       = Gtk::manage (new MyScrolledWindow ());
    usefulPanelSW      = Gtk::manage (new MyScrolledWindow ());
    updateVScrollbars (options.hideTPVScrollbar);

    // load panel endings
    for (int i=0; i< NB_PANEL; i++) {
        vbPanelEnd[i] = Gtk::manage (new Gtk::VBox ());
        imgPanelEnd[i] = Gtk::manage (new RTImage ("ornament1.png"));
        imgPanelEnd[i]->show ();
        vbPanelEnd[i]->pack_start (*imgPanelEnd[i], Gtk::PACK_SHRINK);
        vbPanelEnd[i]->show_all();
        hsPanelEnd[i] = Gtk::manage(new Gtk::HSeparator);
    }

    int panelIter = 0;
    handlePanel(favoritePanel, favoritePanelSW, panelIter++, 4);
    handlePanel(exposurePanel, exposurePanelSW, panelIter++, 4);
    handlePanel(detailsPanel, detailsPanelSW, panelIter++, 4);
    handlePanel(colorPanel, colorPanelSW, panelIter++, 4);
    handlePanel(advancedPanel, advancedPanelSW, panelIter++,4);
    handlePanel(transformPanel, transformPanelSW, panelIter++, 4);
    handlePanel(rawPanel, rawPanelSW, panelIter++, 0);
    handlePanel(usefulPanel, usefulPanelSW, panelIter++, 4);
    handlePanel(trashPanel, trashPanelSW, panelIter++, 4);

    if( options.rtSettings.verbose )
      printf("panel handling performed. \n");

    for (int i=PANEL_SWITCHABLE_START; i< PANEL_SWITCHABLE_START + NB_PANEL_SWITCHABLE; i++) { //last panel is trash thus ignored
      int modOp  = NB_PANEL_SWITCHABLE ; //-2 because we ignore first panel and trash panel
      // I really don't want to know how modulo negative number is h@ndled 
      // so i am adding nbPanel to the values
      // -- esby
      ToolVBox* box1 =  static_cast<ToolVBox*> (vbPanel[PANEL_SWITCHABLE_START + ((i+modOp-2) % (modOp))]);
      ToolVBox* box2 =  static_cast<ToolVBox*> (vbPanel[i]);
      box2->setPrevBox(box1);
      box1->setNextBox(box2);
      env->addVBox(box1);
    }

  //allowing those filters to be extracted from their entity

    ToolVBox* box =  static_cast<ToolVBox*> (lensgeom->getPackBox());

    box->setBoxName("lensgeom");
    box->setNextBox(transformPanel);
    box->setPrevBox(transformPanel);
    env->addVBox(box);

    toiE = Gtk::manage (new TextOrIcon ("exposure.png", M ("MAIN_TAB_EXPOSURE"), M ("MAIN_TAB_EXPOSURE_TOOLTIP")));
    toiD = Gtk::manage (new TextOrIcon ("detail.png", M ("MAIN_TAB_DETAIL"), M ("MAIN_TAB_DETAIL_TOOLTIP")));
    toiC = Gtk::manage (new TextOrIcon ("color-circles.png", M ("MAIN_TAB_COLOR"), M ("MAIN_TAB_COLOR_TOOLTIP")));
    toiW = Gtk::manage (new TextOrIcon ("atom.png", M ("MAIN_TAB_ADVANCED"), M ("MAIN_TAB_ADVANCED_TOOLTIP")));
    toiT = Gtk::manage (new TextOrIcon ("transform.png", M ("MAIN_TAB_TRANSFORM"), M ("MAIN_TAB_TRANSFORM_TOOLTIP")));
    toiR = Gtk::manage (new TextOrIcon ("bayer.png", M ("MAIN_TAB_RAW"), M ("MAIN_TAB_RAW_TOOLTIP")));
    toiM = Gtk::manage (new TextOrIcon ("metadata.png", M ("MAIN_TAB_METADATA"), M ("MAIN_TAB_METADATA_TOOLTIP")));
    toiF = Gtk::manage (new TextOrIcon ("favorite.png" , M ("MAIN_TAB_FAVORITE") , M ("MAIN_TAB_FAVORITE_TOOLTIP") ));
    toiP = Gtk::manage (new TextOrIcon ("trash.png"    , M ("MAIN_TAB_TRASH") ,    M ("MAIN_TAB_TRASH_TOOLTIP") ));
    toiU = Gtk::manage (new TextOrIcon ("useful.png"   , M ("MAIN_TAB_USEFUL") ,   M ("MAIN_TAB_USEFUL_TOOLTIP") ));

    box =  static_cast<ToolVBox*>(sensorbayer->getPackBox());
    box->setBoxName("sensorbayer");
    box->setPrevBox(rawPanel);
    box->setNextBox(rawPanel);
    env->addVBox(box);

    box =  static_cast<ToolVBox*>(sensorxtrans->getPackBox());
    box->setBoxName("sensorxtrans");
    box->setPrevBox(rawPanel);
    box->setNextBox(rawPanel);
    env->addVBox(box);

    box =  static_cast<ToolVBox*>(resize->getPackBox());
    box->setBoxName("resize");
    box->setPrevBox(transformPanel);
    box->setNextBox(transformPanel);
    env->addVBox(box);

    toolPanelNotebook->append_page (*favoritePanelSW,  *toiF);
    toolPanelNotebook->append_page (*exposurePanelSW,  *toiE);
    toolPanelNotebook->append_page (*detailsPanelSW,   *toiD);
    toolPanelNotebook->append_page (*colorPanelSW,     *toiC);
    toolPanelNotebook->append_page (*advancedPanelSW,   *toiW);
    toolPanelNotebook->append_page (*transformPanelSW, *toiT);
    toolPanelNotebook->append_page (*rawPanelSW,       *toiR);
    toolPanelNotebook->append_page (*metadata,    *toiM);
    toolPanelNotebook->append_page (*usefulPanelSW,    *toiU);
    toolPanelNotebook->append_page (*trashPanelSW,     *toiP);

    toolPanelNotebook->set_current_page (0);

    toolPanelNotebook->set_scrollable ();
    toolPanelNotebook->show_all ();

//    if( options.rtSettings.verbose )
      printf("Starting toolpanel deployment\n");
    doDeploy();
//    if( options.rtSettings.verbose )
      printf("Panel deployment finished\n");

    for (auto toolPanel : toolPanels) 
        toolPanel->setListener (this);
    whitebalance->setWBProvider (this);
    whitebalance->setSpotWBListener (this);
    darkframe->setDFProvider (this);
    flatfield->setFFProvider (this);
    lensgeom->setLensGeomListener (this);
    rotate->setLensGeomListener (this);
    distortion->setLensGeomListener (this);
    crop->setCropPanelListener (this);
    icm->setICMPanelListener (this);

    toolBar = new ToolBar ();
    toolBar->setToolBarListener (this);
    toolPanelNotebook->signal_switch_page().connect (sigc::mem_fun(*this,  &ToolPanelCoordinator::on_notebook_switch_page) );
}

void ToolPanelCoordinator::addPanel (Gtk::Box* where, FoldableToolPanel* panel, int level)
{
    env->registerPanel (where, panel, level);
}

ToolPanelCoordinator::~ToolPanelCoordinator ()
{
    idle_register.destroy();

    closeImage ();

    env->disableSwitchPageReaction = true;
    delete toolPanelNotebook;
    delete toolBar;
    delete env;
}

void ToolPanelCoordinator::doDeploy()
{
    for (auto toolPanel : toolPanels) 
    {
//      printf("panel nb=%i \n",  i);
        toolPanel->deploy();
    }
}

void ToolPanelCoordinator::doDeployLate()
{
//    if( options.rtSettings.verbose )
      printf("late panel deployment for env=#%i \n", env->getEnvRef());
    for (auto toolPanel : toolPanels) 
    {   
//      printf("panel nb=%i \n",  i);
      toolPanel->deployLate();
    }
    env->disableSwitchPageReaction = false;

}

void ToolPanelCoordinator::doReact(FakeProcEvent ev)
{
    if( options.rtSettings.verbose ) 
      printf("enabling panel reac for envId=%i\n", env->getId());

    if (!isReaction)
    {
      isReaction = true;

      for (auto toolPanel : toolPanels) 
        toolPanel->setReacted(false);

      for (auto toolPanel : toolPanels) 
      {
//      printf("panel nb=%i \n",  i);
        if (!toolPanel->getReacted())
        {
          toolPanel->setReacted(true);
          toolPanel->react(ev);
        }
      }
      isReaction = false;
    }
}

void ToolPanelCoordinator::handlePanel(Gtk::VBox* vbox, Gtk::ScrolledWindow* panelSW, int panelIterator, int spacing) {
   panelSW->add (*vbox);
   vbox->pack_start (*hsPanelEnd[panelIterator], Gtk::PACK_SHRINK,0);
   vbox->pack_start (*vbPanelEnd[panelIterator], Gtk::PACK_SHRINK,spacing);
   vbPanel[panelIterator] = vbox;
   ToolVBox* box =  static_cast<ToolVBox*>(vbox);
   box->setParent(toolPanelNotebook);
   box->setParentSW(panelSW);
}


void ToolPanelCoordinator::imageTypeChanged (bool isRaw, bool isBayer, bool isXtrans, bool isMono)
{
    if (isRaw) {
        if (isBayer) {
            const auto func = [](gpointer data) -> gboolean {
                ToolPanelCoordinator* const self = static_cast<ToolPanelCoordinator*>(data);

                self->rawPanelSW->set_sensitive (true);
                self->sensorxtrans->FoldableToolPanel::hide();
                self->sensorbayer->FoldableToolPanel::show();
                self->preprocess->FoldableToolPanel::show();
                self->flatfield->FoldableToolPanel::show();
                self->retinex->FoldableToolPanel::setGrayedOut(false);

                return FALSE;
            };
            idle_register.add(func, this);
        }
        else if (isXtrans) {
            const auto func = [](gpointer data) -> gboolean {
                ToolPanelCoordinator* const self = static_cast<ToolPanelCoordinator*>(data);

                self->rawPanelSW->set_sensitive (true);
                self->sensorxtrans->FoldableToolPanel::show();
                self->sensorbayer->FoldableToolPanel::hide();
                self->preprocess->FoldableToolPanel::show();
                self->flatfield->FoldableToolPanel::show();
                self->retinex->FoldableToolPanel::setGrayedOut(false);

                return FALSE;
            };
            idle_register.add(func, this);
        }
        else if (isMono) {
            const auto func = [](gpointer data) -> gboolean {
                ToolPanelCoordinator* const self = static_cast<ToolPanelCoordinator*>(data);

                self->rawPanelSW->set_sensitive (true);
                self->sensorbayer->FoldableToolPanel::hide();
                self->sensorxtrans->FoldableToolPanel::hide();
                self->preprocess->FoldableToolPanel::hide();
                self->flatfield->FoldableToolPanel::show();
                self->retinex->FoldableToolPanel::setGrayedOut(false);

                return FALSE;
            };
            idle_register.add(func, this);
        } else {
            const auto func = [](gpointer data) -> gboolean {
                ToolPanelCoordinator* const self = static_cast<ToolPanelCoordinator*>(data);

                self->rawPanelSW->set_sensitive (true);
                self->sensorbayer->FoldableToolPanel::hide();
                self->sensorxtrans->FoldableToolPanel::hide();
                self->preprocess->FoldableToolPanel::hide();
                self->flatfield->FoldableToolPanel::hide();
                self->retinex->FoldableToolPanel::setGrayedOut(false);

                return FALSE;
            };
            idle_register.add(func, this);
        }
    } else {
        const auto func = [](gpointer data) -> gboolean {
            ToolPanelCoordinator* const self = static_cast<ToolPanelCoordinator*>(data);

            self->rawPanelSW->set_sensitive (false);
            self->retinex->FoldableToolPanel::setGrayedOut(true);

            return FALSE;
        };
        idle_register.add(func, this);
    }

}


void ToolPanelCoordinator::panelChanged(const rtengine::ProcEvent& event, const Glib::ustring& descr)
{
    if (!ipc) {
        return;
    }

    int changeFlags = rtengine::RefreshMapper::getInstance()->getAction(event);

    ProcParams* params = ipc->beginUpdateParams ();

    for (auto toolPanel : toolPanels) 
        toolPanel->write (params);

    // Compensate rotation on flip
    if (event == rtengine::EvCTHFlip || event == rtengine::EvCTVFlip) {
        if (fabs (params->rotate.degree) > 0.001) {
            params->rotate.degree *= -1;
            changeFlags |= rtengine::RefreshMapper::getInstance()->getAction(rtengine::EvROTDegree);
            rotate->read (params);
        }
    }

    int tr = TR_NONE;

    if (params->coarse.rotate == 90) {
        tr = TR_R90;
    } else if (params->coarse.rotate == 180) {
        tr = TR_R180;
    } else if (params->coarse.rotate == 270) {
        tr = TR_R270;
    }

    // Update "on preview" geometry
    if (event == rtengine::EvPhotoLoaded || event == rtengine::EvProfileChanged || event == rtengine::EvHistoryBrowsed || event == rtengine::EvCTRotate) {
        // updating the "on preview" geometry
        int fw, fh;
        ipc->getInitialImage()->getImageSource()->getFullSize (fw, fh, tr);
        gradient->updateGeometry (params->gradient.centerX, params->gradient.centerY, params->gradient.feather, params->gradient.degree, fw, fh);
    }

    // some transformations make the crop change for convenience
    if (event == rtengine::EvCTHFlip) {
        crop->hFlipCrop ();
        crop->write (params);
    } else if (event == rtengine::EvCTVFlip) {
        crop->vFlipCrop ();
        crop->write (params);
    } else if (event == rtengine::EvCTRotate) {
        crop->rotateCrop (params->coarse.rotate, params->coarse.hflip, params->coarse.vflip);
        crop->write (params);
        resize->update (params->crop.enabled, params->crop.w, params->crop.h, ipc->getFullWidth(), ipc->getFullHeight());
        resize->write (params);
    } else if (event == rtengine::EvCrop) {
        resize->update (params->crop.enabled, params->crop.w, params->crop.h);
        resize->write (params);
    }

    ipc->endUpdateParams (changeFlags);   // starts the IPC processing

    hasChanged = true;

    for (auto paramcListener : paramcListeners) {
        paramcListener->procParamsChanged (params, event, descr);
    }
}

void ToolPanelCoordinator::profileChange(
    const PartialProfile* nparams,
    const rtengine::ProcEvent& event,
    const Glib::ustring& descr,
    const ParamsEdited* paramsEdited,
    bool fromLastSave
)
{
    int fw, fh, tr;

    if (!ipc) {
        return;
    }

    ProcParams *params = ipc->beginUpdateParams ();
    ProcParams *mergedParams = new ProcParams();

    // Copy the current params as default values for the fusion
    *mergedParams = *params;

    // Reset IPTC values when switching procparams from the History
    if (event == rtengine::EvHistoryBrowsed) {
        mergedParams->iptc.clear();
        mergedParams->exif.clear();
    }

    // And apply the partial profile nparams to mergedParams
    nparams->applyTo (mergedParams, fromLastSave);

    // Derive the effective changes, if it's a profile change, to prevent slow RAW rerendering if not necessary
    bool filterRawRefresh = false;

    if (event != rtengine::EvPhotoLoaded) {
        ParamsEdited pe (true);
        std::vector<rtengine::procparams::ProcParams> lParams (2);
        lParams[0] = *params;
        lParams[1] = *mergedParams;
        pe.initFrom (lParams);

        filterRawRefresh = pe.raw.isUnchanged() && pe.lensProf.isUnchanged() && pe.retinex.isUnchanged();
    }

    *params = *mergedParams;
    delete mergedParams;

    tr = TR_NONE;

    if (params->coarse.rotate == 90) {
        tr = TR_R90;
    } else if (params->coarse.rotate == 180) {
        tr = TR_R180;
    } else if (params->coarse.rotate == 270) {
        tr = TR_R270;
    }

    // trimming overflowing cropped area
    ipc->getInitialImage()->getImageSource()->getFullSize (fw, fh, tr);
    crop->trim (params, fw, fh);

    // updating the GUI with updated values
    for (auto toolPanel : toolPanels) {
        toolPanel->read (params);

        if (event == rtengine::EvPhotoLoaded || event == rtengine::EvProfileChanged) {
            toolPanel->autoOpenCurve();
        }
    }

    if (event == rtengine::EvPhotoLoaded || event == rtengine::EvProfileChanged || event == rtengine::EvHistoryBrowsed || event == rtengine::EvCTRotate) {
        // updating the "on preview" geometry
        gradient->updateGeometry (params->gradient.centerX, params->gradient.centerY, params->gradient.feather, params->gradient.degree, fw, fh);
    }

    // start the IPC processing
    if (filterRawRefresh) {
        ipc->endUpdateParams ( rtengine::RefreshMapper::getInstance()->getAction(event) & ALLNORAW );
    } else {
        ipc->endUpdateParams (event);
    }

    hasChanged = event != rtengine::EvProfileChangeNotification;

    for (auto paramcListener : paramcListeners) {
        paramcListener->procParamsChanged (params, event, descr);
    }
}

void ToolPanelCoordinator::setDefaults(const ProcParams* defparams)
{
    if (defparams)
       for (auto toolPanel : toolPanels) 
            toolPanel->setDefaults(defparams);
}

CropGUIListener* ToolPanelCoordinator::getCropGUIListener ()
{

    return crop;
}

void parseDirectory(rtexif::TagDirectory* d, Glib::ustring prefix, Environment* env)
{
  if (d)
    for (int i = 0; i < d->getCount(); ++i)
    {
       rtexif::Tag* t = d->getTagByIndex (i);

       Glib::ustring prefixedName = prefix + ':' + t->nameToString();
       if (prefix=="")
         prefixedName = t->nameToString();

//       printf("trying to parse tag name=%s:%s \n",prefixedName.c_str(), t->nameToString().c_str());

       if (t->getDirectory())
       {
//         printf("parsing directory= %s \n",prefixedName.c_str());
         parseDirectory(t->getDirectory(),prefixedName,env);
       }
       else
       {
//         printf("... value=%s \n",t->valueToString().c_str());
         env->setVar(prefixedName, t->valueToString());
       }
   }
}

void ToolPanelCoordinator::initImage (rtengine::StagedImageProcessor* ipc_, bool raw)
{

    ipc = ipc_;
    toneCurve->disableListener ();
    toneCurve->enableAll ();
    toneCurve->enableListener ();

    if (ipc) {
        const rtengine::FramesMetaData* pMetaData = ipc->getInitialImage()->getMetaData();
        metadata->setImageData(pMetaData);

       const rtengine::FramesMetaData* idata = ipc->getInitialImage()->getMetaData();
       printf("transmiting some data via rtvar \n");
       //todo add idata to rtvar?
        env->setVar("Iso", idata->getISOSpeed());
        env->setVar("Fnum",  Glib::ustring(idata->apertureToString(idata->getFNumber())));
        env->setVar("Speed", Glib::ustring(idata->shutterToString(idata->getShutterSpeed())));
        env->setVar("FLen", idata->getFocalLen());
        env->setVar("Ecomp", Glib::ustring(idata->expcompToString(idata->getExpComp(),true)));
//        env->setVar("Fname", openThm->getFileName());
//todo search another way of loading it
        env->setVar("Width", ipc->getFullWidth());
        env->setVar("Height", ipc->getFullHeight());
        // note: we will react later, on pp3 version tranmission
        printf("partial exif values transmitted by variables \n");

        printf("transmitting full exif data via rtvar \n");
        if (pMetaData->hasExif()) 
        {
          rtexif::TagDirectory* root = pMetaData->getRootExifData() ;
          parseDirectory(root,ROOT_EXIF_PREFIX,env);
          printf("full exif values tranmitted by variables \n");
          doReact(FakeEvFullExifTransmitted);
        }

        ipc->setAutoExpListener (toneCurve);
        ipc->setAutoCamListener (colorappearance);
        ipc->setAutoBWListener (blackwhite);
        ipc->setFrameCountListener (bayerprocess);
        ipc->setFlatFieldAutoClipListener (flatfield);
        ipc->setBayerAutoContrastListener (bayerprocess);
        ipc->setXtransAutoContrastListener (xtransprocess);
        ipc->setAutoWBListener (whitebalance);
        ipc->setAutoColorTonListener (colortoning);
        ipc->setAutoChromaListener (dirpyrdenoise);
        ipc->setWaveletListener (wavelet);
        ipc->setRetinexListener (retinex);
        ipc->setSizeListener (crop);
        ipc->setSizeListener (resize);
        ipc->setImageTypeListener (this);
        flatfield->setShortcutPath (Glib::path_get_dirname (ipc->getInitialImage()->getFileName()));

        icm->setRawMeta (raw, (const rtengine::FramesData*)pMetaData);
        lensProf->setRawMeta (raw, pMetaData);
    }


    toneCurve->setRaw (raw);
    hasChanged = true;
}


void ToolPanelCoordinator::closeImage ()
{

    if (ipc) {
        ipc->stopProcessing ();
        ipc = nullptr;
    }
}

void ToolPanelCoordinator::closeAllTools()
{

    for (size_t i = 0; i < options.tpOpen.size(); i++)
        if (i < expList.size()) {
            expList.at (i)->set_expanded (false);
        }
}

void ToolPanelCoordinator::openAllTools()
{

    for (size_t i = 0; i < options.tpOpen.size(); i++)
        if (i < expList.size()) {
            expList.at (i)->set_expanded (true);
        }
}

void ToolPanelCoordinator::updateToolState()
{

    for (size_t i = 0; i < options.tpOpen.size(); i++)
        if (i < expList.size()) {
            expList.at (i)->set_expanded (options.tpOpen.at (i));
        }

    if (options.tpOpen.size() > expList.size()) {
        size_t sizeWavelet = options.tpOpen.size() - expList.size();
        std::vector<int> temp;

        for (size_t i = 0; i < sizeWavelet; i++) {
            temp.push_back (options.tpOpen.at (i + expList.size()));
        }

        wavelet->updateToolState (temp);
        retinex->updateToolState (temp);
    }
}

void ToolPanelCoordinator::readOptions ()
{

    crop->readOptions ();
}

void ToolPanelCoordinator::writeOptions ()
{

    crop->writeOptions ();

    if (options.autoSaveTpOpen) {
        writeToolExpandedStatus (options.tpOpen);
    }
}


void ToolPanelCoordinator::writeToolExpandedStatus (std::vector<int> &tpOpen)
{
    tpOpen.clear ();

    for (size_t i = 0; i < expList.size(); i++) {
        tpOpen.push_back (expList.at (i)->get_expanded ());
    }

    wavelet->writeOptions (tpOpen);
    retinex->writeOptions (tpOpen);
}


void ToolPanelCoordinator::spotWBselected(int x, int y, Thumbnail* thm)
{
    if (!ipc) {
        return;
    }

//    toolBar->setTool (TOOL_HAND);
    int rect = whitebalance->getSize ();
    int ww = ipc->getFullWidth();
    int hh = ipc->getFullHeight();

    if (x - rect > 0 && y - rect > 0 && x + rect < ww && y + rect < hh) {
        double temp;
        double green;
        ipc->getSpotWB (x, y, rect, temp, green);
        whitebalance->setWB (temp, green);
    }
}

void ToolPanelCoordinator::sharpMaskSelected(bool sharpMask)
{
    if (!ipc) {
        return;
    }
    ipc->beginUpdateParams ();
    ipc->setSharpMask(sharpMask);
    ipc->endUpdateParams (rtengine::EvShrEnabled);
}

int ToolPanelCoordinator::getSpotWBRectSize() const
{
    return whitebalance->getSize();
}

void ToolPanelCoordinator::cropSelectionReady()
{
    toolBar->setTool (TMHand);

    if (!ipc) {
        return;
    }
}

void ToolPanelCoordinator::rotateSelectionReady(double rotate_deg, Thumbnail* thm)
{
    toolBar->setTool (TMHand);

    if (!ipc) {
        return;
    }

    if (rotate_deg != 0.0) {
        rotate->straighten (rotate_deg);
    }
}

ToolBar* ToolPanelCoordinator::getToolBar() const
{
    return toolBar;
}

CropGUIListener* ToolPanelCoordinator::startCropEditing(Thumbnail* thm)
{
    return crop;
}

void ToolPanelCoordinator::autoCropRequested ()
{

    if (!ipc) {
        return;
    }

    int x1, y1, x2, y2, w, h;
    ipc->getAutoCrop (crop->getRatio(), x1, y1, w, h);
    x2 = x1 + w - 1;
    y2 = y1 + h - 1;
    crop->cropInit (x1, y1, w, h);
    crop->cropResized (x1, y1, x2, y2);
    crop->cropManipReady ();
}

rtengine::RawImage* ToolPanelCoordinator::getDF()
{
    if (!ipc) {
        return nullptr;
    }

    const rtengine::FramesMetaData *imd = ipc->getInitialImage()->getMetaData();

    if (imd) {
        int iso = imd->getISOSpeed();
        double shutter = imd->getShutterSpeed();
        std::string maker ( imd->getMake()  );
        std::string model ( imd->getModel() );
        time_t timestamp = imd->getDateTimeAsTS();

        return rtengine::dfm.searchDarkFrame ( maker, model, iso, shutter, timestamp);
    }

    return nullptr;
}

rtengine::RawImage* ToolPanelCoordinator::getFF()
{
    if (!ipc) {
        return nullptr;
    }

    const rtengine::FramesMetaData *imd = ipc->getInitialImage()->getMetaData();

    if (imd) {
        // int iso = imd->getISOSpeed();              temporarilly removed because unused
        // double shutter = imd->getShutterSpeed();   temporarilly removed because unused
        double aperture = imd->getFNumber();
        double focallength = imd->getFocalLen();
        std::string maker ( imd->getMake()  );
        std::string model ( imd->getModel() );
        std::string lens (  imd->getLens()  );
        time_t timestamp = imd->getDateTimeAsTS();

        return rtengine::ffm.searchFlatField ( maker, model, lens, focallength, aperture, timestamp);
    }

    return nullptr;
}

Glib::ustring ToolPanelCoordinator::GetCurrentImageFilePath()
{
    if (!ipc) {
        return "";
    }

    return ipc->getInitialImage()->getFileName();
}

void ToolPanelCoordinator::straightenRequested ()
{

    if (!ipc) {
        return;
    }

    toolBar->setTool (TMStraighten);
}

double ToolPanelCoordinator::autoDistorRequested ()
{
    if (!ipc) {
        return 0.0;
    }

    return rtengine::ImProcFunctions::getAutoDistor (ipc->getInitialImage()->getFileName(), 400);
}

void ToolPanelCoordinator::spotWBRequested (int size)
{

    if (!ipc) {
        return;
    }

    toolBar->setTool (TMSpotWB);
}

void ToolPanelCoordinator::cropSelectRequested ()
{

    if (!ipc) {
        return;
    }

    toolBar->setTool (TMCropSelect);
}

void ToolPanelCoordinator::saveInputICCReference(const Glib::ustring& fname, bool apply_wb)
{
    if (ipc) {
        ipc->saveInputICCReference (fname, apply_wb);
    }
}

void ToolPanelCoordinator::updateCurveBackgroundHistogram(
    const LUTu& histToneCurve,
    const LUTu& histLCurve,
    const LUTu& histCCurve,
    const LUTu& histLCAM,
    const LUTu& histCCAM,
    const LUTu& histRed,
    const LUTu& histGreen,
    const LUTu& histBlue,
    const LUTu& histLuma,
    const LUTu& histLRETI
)
{
    colorappearance->updateCurveBackgroundHistogram(histToneCurve, histLCurve, histCCurve, histLCAM,  histCCAM, histRed, histGreen, histBlue, histLuma, histLRETI);
    toneCurve->updateCurveBackgroundHistogram(histToneCurve, histLCurve, histCCurve,histLCAM,  histCCAM, histRed, histGreen, histBlue, histLuma, histLRETI);
    lcurve->updateCurveBackgroundHistogram(histToneCurve, histLCurve, histCCurve, histLCAM, histCCAM, histRed, histGreen, histBlue, histLuma, histLRETI);
    rgbcurves->updateCurveBackgroundHistogram(histToneCurve, histLCurve, histCCurve, histLCAM, histCCAM, histRed, histGreen, histBlue, histLuma, histLRETI);
    retinex->updateCurveBackgroundHistogram(histToneCurve, histLCurve, histCCurve, histLCAM, histCCAM, histRed, histGreen, histBlue, histLuma, histLRETI);
}

void ToolPanelCoordinator::foldAllButOne (Gtk::Box* parent, FoldableToolPanel* openedSection)
{

    for (auto toolPanel : toolPanels) {
        if (toolPanel->getParent() != nullptr) {
            ToolPanel* currentTP = toolPanel;

            if ((env->state == ENV_STATE_IN_FAV)
            || ((currentTP->getParent() == parent) 
            && (!currentTP->canBeIgnored()))) {
                // Section in the same tab, we unfold it if it's not the one that has been clicked
                if (currentTP != openedSection) {
                    currentTP->setExpanded (false);
                } else {
                    if (!currentTP->getExpanded()) {
                        currentTP->setExpanded (true);
                    }
                }
            }
        }
    }
}

bool ToolPanelCoordinator::handleShortcutKey (GdkEventKey* event)
{

    //bool ctrl = event->state & GDK_CONTROL_MASK;  temporarily removed because unused
    //bool shift = event->state & GDK_SHIFT_MASK;   temporarily removed because unused
    bool alt = event->state & GDK_MOD1_MASK;

    if (alt) {
        switch (event->keyval) {
            case GDK_KEY_e:
                toolPanelNotebook->set_current_page (toolPanelNotebook->page_num (*exposurePanelSW));
                return true;

            case GDK_KEY_d:
                toolPanelNotebook->set_current_page (toolPanelNotebook->page_num (*detailsPanelSW));
                return true;

            case GDK_KEY_c:
                toolPanelNotebook->set_current_page (toolPanelNotebook->page_num (*colorPanelSW));
                return true;

            case GDK_KEY_t:
                toolPanelNotebook->set_current_page (toolPanelNotebook->page_num (*transformPanelSW));
                return true;

            case GDK_KEY_r:
                toolPanelNotebook->set_current_page (toolPanelNotebook->page_num (*rawPanelSW));
                return true;

            case GDK_KEY_w:
                toolPanelNotebook->set_current_page (toolPanelNotebook->page_num (*advancedPanelSW));
                return true;

            case GDK_KEY_m:
                toolPanelNotebook->set_current_page (toolPanelNotebook->page_num (*metadata));
                return true;

            case GDK_KEY_p:
                toolPanelNotebook->set_current_page (toolPanelNotebook->page_num(*trashPanelSW));
                return true;

            case GDK_KEY_f:
                toolPanelNotebook->set_current_page (toolPanelNotebook->page_num(*favoritePanelSW));
                return true;

            case GDK_KEY_u:
                 toolPanelNotebook->set_current_page (toolPanelNotebook->page_num(*usefulPanelSW));
                 return true;
        }
    }

    return false;
}

void ToolPanelCoordinator::updateVScrollbars (bool hide)
{
    GThreadLock lock; // All GUI access from idle_add callbacks or separate thread HAVE to be protected
    Gtk::PolicyType policy = hide ? Gtk::POLICY_NEVER : Gtk::POLICY_AUTOMATIC;
    exposurePanelSW->set_policy     (Gtk::POLICY_AUTOMATIC, policy);
    detailsPanelSW->set_policy      (Gtk::POLICY_AUTOMATIC, policy);
    colorPanelSW->set_policy        (Gtk::POLICY_AUTOMATIC, policy);
    transformPanelSW->set_policy    (Gtk::POLICY_AUTOMATIC, policy);
    rawPanelSW->set_policy          (Gtk::POLICY_AUTOMATIC, policy);
    advancedPanelSW->set_policy      (Gtk::POLICY_AUTOMATIC, policy);
    favoritePanelSW->set_policy     (Gtk::POLICY_AUTOMATIC, policy);
    trashPanelSW->set_policy        (Gtk::POLICY_AUTOMATIC, policy);
    usefulPanelSW->set_policy       (Gtk::POLICY_AUTOMATIC, policy);

    for (auto currExp : expList) {
        currExp->updateVScrollbars (hide);
    }
}

void ToolPanelCoordinator::updateTPVScrollbar (bool hide)
{
    updateVScrollbars (hide);
}

void ToolPanelCoordinator::toolSelected (ToolMode tool)
{
    GThreadLock lock; // All GUI access from idle_add callbacks or separate thread HAVE to be protected

    switch (tool) {
        case TMCropSelect:
            crop->setExpanded (true);
            toolPanelNotebook->set_current_page (toolPanelNotebook->page_num (*transformPanelSW));
            break;

        case TMSpotWB:
            whitebalance->setExpanded (true);
            toolPanelNotebook->set_current_page (toolPanelNotebook->page_num (*colorPanelSW));
            break;

        case TMStraighten:
            lensgeom->setExpanded (true);
            rotate->setExpanded (true);
            toolPanelNotebook->set_current_page (toolPanelNotebook->page_num (*transformPanelSW));
            break;

        default:
            break;
    }
}

void ToolPanelCoordinator::editModeSwitchedOff ()
{
    if (editDataProvider) {
        editDataProvider->switchOffEditMode();
    }
}

void ToolPanelCoordinator::dirSelected (const Glib::ustring& dirname, const Glib::ustring& openfile)
{

    flatfield->setShortcutPath (dirname);
}

void ToolPanelCoordinator::setEditProvider (EditDataProvider *provider)
{
    editDataProvider = provider;

    for (size_t i = 0; i < toolPanels.size(); i++) {
        toolPanels.at (i)->setEditProvider (provider);
    }
}
void ToolPanelCoordinator::on_notebook_switch_page(Gtk::Widget* /* page */, guint page_num){

  if (!env->disableSwitchPageReaction)
    {

    env->prevState = env->state;
    if (options.rtSettings.verbose)     
      printf("notebook switch page");
    if (toolPanelNotebook->get_current_page() == toolPanelNotebook->page_num(*favoritePanelSW))
    {
       if (options.rtSettings.verbose)
         printf(" -> favorite panel\n");
       env->state = ENV_STATE_IN_FAV;      
    }
    else 
    if(toolPanelNotebook->get_current_page() == toolPanelNotebook->page_num(*trashPanelSW)) 
    {
       if (options.rtSettings.verbose)
         printf(" -> trash panel\n");
       env->state = ENV_STATE_IN_TRASH;
    }else
    {
      if (options.rtSettings.verbose)
        printf(" -> normal panel\n");
      env->state = ENV_STATE_IN_NORM;
    }

   // we only checks outside of fav <> fav or trash <> trash interactions
   if (!((env->state == env->prevState)
    && (env->state != ENV_STATE_IN_NORM)))
   {

     // todo: fix the performance issue.
     // most of the time is spend on this loop
     // there is probably an optimization that needs to be performed at some point.

   // dc determine the tabs that was displayed before the tab switch 
    // 11 for favorite to favorite tabs.
    // 12 for favorite to normal tabs.
    // 13 for favorite to trash tabs.
    // 21 for normal to favorite tabs.
    // 22 n -> n
    // 23 n -> t
    // 31 t -> f
    // 32 t -> n
    // 33 t -> t

    int dc=0;
    if (env->prevState == ENV_STATE_IN_FAV) dc+= 10;
    if (env->prevState == ENV_STATE_IN_TRASH) dc+= 30;
    if (env->prevState == ENV_STATE_IN_NORM) dc+= 20;

    if (env->state == ENV_STATE_IN_FAV) dc+= 1;
    if (env->state == ENV_STATE_IN_TRASH) dc+= 3;
    if (env->state == ENV_STATE_IN_NORM) dc+= 2;

    for (auto toolPanel : toolPanels) 
        toolPanel->favorite_others_tabs_switch(dc);

    //putting the ending panels and separator to the end
    for (int i=0; i< NB_PANEL; i++){
      int pos = toolPanels.size()-1;
      Gtk::Widget* w = static_cast<Gtk::Widget*>(vbPanelEnd[i]);
      vbPanel[i]->reorder_child (*w, pos);
      w = (Gtk::Widget*)hsPanelEnd[i];
      vbPanel[i]->reorder_child (*w, pos-1);
     }
   }
    // we update label info all the time
    // updating the label info (currently the position number)
    for (auto toolPanel : toolPanels) 
        toolPanel->updateLabelInfo();
  }
}
