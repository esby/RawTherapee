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
 *  along with RawTherapee.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "multilangmgr.h"
#include "toolpanelcoord.h"
#include "metadatapanel.h"
#include "options.h"
#include "rtimage.h"

#include "../rtengine/imagesource.h"
#include "../rtengine/dfmanager.h"
#include "../rtengine/ffmanager.h"
#include "../rtengine/improcfun.h"
#include "../rtengine/perspectivecorrection.h"
#include "../rtengine/procevents.h"
#include "../rtengine/refreshmap.h"
#include "../rtexif/rtexif.h"
#include "ttdep.h"

using namespace rtengine::procparams;

ToolPanelCoordinator::ToolPanelCoordinator(bool batch, bool benchmark) : ipc(nullptr), favoritePanelSW(nullptr), hasChanged(false), editDataProvider(nullptr)
{
    useRtFav = false;

    favoritePanel   = Gtk::manage(new ToolVBox());
    exposurePanel   = Gtk::manage(new ToolVBox());
    detailsPanel    = Gtk::manage(new ToolVBox());
    colorPanel      = Gtk::manage(new ToolVBox());
    transformPanel  = Gtk::manage(new ToolVBox());
    rawPanel        = Gtk::manage(new ToolVBox());
    advancedPanel   = Gtk::manage(new ToolVBox());
    locallabPanel   = Gtk::manage(new ToolVBox());
    trashPanel      = Gtk::manage(new ToolVBox());
    usefulPanel     = Gtk::manage(new ToolVBox());

    favoritePanel->setBoxName(PANEL_NAME_FAVORITE);
    exposurePanel->setBoxName(PANEL_NAME_EXPOSURE);
    detailsPanel->setBoxName(PANEL_NAME_DETAILS);
    colorPanel->setBoxName(PANEL_NAME_COLOR);
    advancedPanel->setBoxName(PANEL_NAME_WAVELET);
    locallabPanel->setBoxName(PANEL_NAME_LOCALLAB);
    transformPanel->setBoxName(PANEL_NAME_TRANSFORM);
    rawPanel->setBoxName(PANEL_NAME_RAW);
    usefulPanel->setBoxName(PANEL_NAME_USEFUL);
    trashPanel->setBoxName(PANEL_NAME_TRASH);

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

    coarse              = Gtk::manage(new CoarsePanel());
    toneCurve           = Gtk::manage(new ToneCurve());
    shadowshighlights   = Gtk::manage(new ShadowsHighlights());
    impulsedenoise      = Gtk::manage(new ImpulseDenoise());
    defringe            = Gtk::manage(new Defringe());
    spot                = Gtk::manage(new Spot());
    dirpyrdenoise       = Gtk::manage(new DirPyrDenoise());
    epd                 = Gtk::manage(new EdgePreservingDecompositionUI());
    sharpening          = Gtk::manage(new Sharpening());
    localContrast       = Gtk::manage(new LocalContrast());
    sharpenEdge         = Gtk::manage(new SharpenEdge());
    sharpenMicro        = Gtk::manage(new SharpenMicro());
    lcurve              = Gtk::manage(new LCurve());
    rgbcurves           = Gtk::manage(new RGBCurves());
    colortoning         = Gtk::manage(new ColorToning());
    lensgeom            = Gtk::manage(new LensGeometry());
    lensProf            = Gtk::manage(new LensProfilePanel());
    distortion          = Gtk::manage(new Distortion());
    rotate              = Gtk::manage(new Rotate());
    vibrance            = Gtk::manage(new Vibrance());
    colorappearance     = Gtk::manage(new ColorAppearance());
    whitebalance        = Gtk::manage(new WhiteBalance());
    vignetting          = Gtk::manage(new Vignetting());
    retinex             = Gtk::manage(new Retinex());
    gradient            = Gtk::manage(new Gradient());
    locallab            = Gtk::manage(new Locallab());
    pcvignette          = Gtk::manage(new PCVignette());
    perspective         = Gtk::manage(new PerspCorrection());
    cacorrection        = Gtk::manage(new CACorrection());
    chmixer             = Gtk::manage(new ChMixer());
    blackwhite          = Gtk::manage(new BlackWhite());
    resize              = Gtk::manage(new Resize());
    prsharpening        = Gtk::manage(new PrSharpening());
    crop                = Gtk::manage(new Crop());
    icm                 = Gtk::manage(new ICMPanel());
    metadata            = Gtk::manage(new MetaDataPanel());
    wavelet             = Gtk::manage(new Wavelet());
    dirpyrequalizer     = Gtk::manage(new DirPyrEqualizer());
    hsvequalizer        = Gtk::manage(new HSVEqualizer());
    filmSimulation      = Gtk::manage(new FilmSimulation());
    softlight           = Gtk::manage(new SoftLight());
    dehaze              = Gtk::manage(new Dehaze());
    sensorbayer         = Gtk::manage(new SensorBayer());
    sensorxtrans        = Gtk::manage(new SensorXTrans());
    bayerprocess        = Gtk::manage(new BayerProcess());
    xtransprocess       = Gtk::manage(new XTransProcess());
    bayerpreprocess     = Gtk::manage(new BayerPreProcess());
    preprocess          = Gtk::manage(new PreProcess());
    darkframe           = Gtk::manage(new DarkFrame());
    flatfield           = Gtk::manage(new FlatField());
    rawcacorrection     = Gtk::manage(new RAWCACorr());
    rawexposure         = Gtk::manage(new RAWExposure());
    preprocessWB        = Gtk::manage(new PreprocessWB());
    bayerrawexposure    = Gtk::manage(new BayerRAWExposure());
    xtransrawexposure   = Gtk::manage(new XTransRAWExposure());
    fattal              = Gtk::manage(new FattalToneMapping());
    filmNegative        = Gtk::manage(new FilmNegative());
    pdSharpening        = Gtk::manage(new PdSharpening());
    // So Demosaic, Line noise filter, Green Equilibration, Ca-Correction(garder le nom de section identique!) and Black-Level will be moved in a "Bayer sensor" tool,
    // and a separate Demosaic and Black Level tool will be created in an "X-Trans sensor" tool

    // X-Trans demozaic methods: "3-pass(best), 1-pass(medium), fast"
    // Mettre  jour les profils fournis pour inclure les nouvelles section Raw, notamment pour "Default High ISO"
    // Valeurs par dfaut:
    //     Best -> low ISO
    //     Medium -> High ISO
    if (useRtFav)
      favorites.resize(options.favorites.size(), nullptr);

    addfavoritePanel(colorPanel, whitebalance);
    addfavoritePanel(exposurePanel, toneCurve);
    addfavoritePanel(colorPanel, vibrance);
    addfavoritePanel(colorPanel, chmixer);
    addfavoritePanel(colorPanel, blackwhite);
    addfavoritePanel(exposurePanel, shadowshighlights);
    addfavoritePanel(detailsPanel, spot);
    addfavoritePanel(detailsPanel, sharpening);
    addfavoritePanel(detailsPanel, localContrast);
    addfavoritePanel(detailsPanel, sharpenEdge);
    addfavoritePanel(detailsPanel, sharpenMicro);
    addfavoritePanel(colorPanel, hsvequalizer);
    addfavoritePanel(colorPanel, filmSimulation);
    addfavoritePanel(colorPanel, filmNegative);
    addfavoritePanel(colorPanel, softlight);
    addfavoritePanel(colorPanel, rgbcurves);
    addfavoritePanel(colorPanel, colortoning);
    addfavoritePanel(exposurePanel, epd);
    addfavoritePanel(exposurePanel, fattal);
    addfavoritePanel(advancedPanel, retinex);
    addfavoritePanel(exposurePanel, pcvignette);
    addfavoritePanel(exposurePanel, gradient);
    addfavoritePanel(exposurePanel, lcurve);
    addfavoritePanel(advancedPanel, colorappearance);
    addfavoritePanel(detailsPanel, impulsedenoise);
    addfavoritePanel(detailsPanel, dirpyrdenoise);
    addfavoritePanel(detailsPanel, defringe);
    addfavoritePanel(detailsPanel, dirpyrequalizer);
    addfavoritePanel(detailsPanel, dehaze);
    addfavoritePanel(advancedPanel, wavelet);
    addfavoritePanel(locallabPanel, locallab);

    addfavoritePanel(transformPanel, crop);
    addfavoritePanel(transformPanel, resize);
    addPanel(resize->getPackBox(), prsharpening, 2); // todo why addPanel and not addfavorite?
    addfavoritePanel(transformPanel, lensgeom);
    addfavoritePanel(lensgeom->getPackBox(), rotate, 2);
    addfavoritePanel(lensgeom->getPackBox(), perspective, 2);
    addfavoritePanel(lensgeom->getPackBox(), lensProf, 2);
    addfavoritePanel(lensgeom->getPackBox(), distortion, 2);
    addfavoritePanel(lensgeom->getPackBox(), cacorrection, 2);
    addfavoritePanel(lensgeom->getPackBox(), vignetting, 2);
    addfavoritePanel(colorPanel, icm);
    addfavoritePanel(rawPanel, sensorbayer);
    addfavoritePanel(sensorbayer->getPackBox(), bayerprocess, 2);
    addfavoritePanel(sensorbayer->getPackBox(), bayerrawexposure, 2);
    addfavoritePanel(sensorbayer->getPackBox(), bayerpreprocess, 2);
    addfavoritePanel(sensorbayer->getPackBox(), rawcacorrection, 2);
    addfavoritePanel(rawPanel, sensorxtrans);
    addfavoritePanel(sensorxtrans->getPackBox(), xtransprocess, 2);
    addfavoritePanel(sensorxtrans->getPackBox(), xtransrawexposure, 2);
    addfavoritePanel(rawPanel, rawexposure);
    addfavoritePanel(rawPanel, preprocessWB);
    addfavoritePanel(rawPanel, preprocess);
    addfavoritePanel(rawPanel, darkframe);
    addfavoritePanel(rawPanel, flatfield);
    addfavoritePanel(rawPanel, pdSharpening);

    favoriteCount = 0;
    if (useRtFav){
       for(auto it = favorites.begin(); it != favorites.end(); ++it) {
         if (*it) {
            addPanel(favoritePanel, *it);
            ++favoriteCount;
         }
       }
    }

    toolPanels.push_back(coarse);
    toolPanels.push_back(metadata);
    coarse->setToolName("coarse"); // coarse does not have a name.

    toolPanelNotebook = new Gtk::Notebook();
    toolPanelNotebook->set_name("ToolPanelNotebook");

  // new panels are registered a bit diffently
    addPanel(usefulPanel, Gtk::manage(new TTSaver()));
    addPanel(usefulPanel, Gtk::manage(new TTIsoProfiler()));
    addPanel(usefulPanel, Gtk::manage(new TTTabHider()));
    addPanel(usefulPanel, Gtk::manage(new TTFavoriteColorChooser()));
    addPanel(usefulPanel, Gtk::manage(new TTPanelColorChooser()));
    addPanel(usefulPanel, Gtk::manage(new TTUDLRHider()));
    addPanel(usefulPanel, Gtk::manage(new TTTweaker()));
    addPanel(usefulPanel, Gtk::manage(new TTVarDisplayer()));

    toolPanelNotebook = new Gtk::Notebook();
    toolPanelNotebook->set_name("ToolPanelNotebook");
    env->setToolPanelNotebook(toolPanelNotebook);

    favoritePanelSW    = Gtk::manage(new MyScrolledWindow()); 
    exposurePanelSW    = Gtk::manage(new MyScrolledWindow());
    detailsPanelSW     = Gtk::manage(new MyScrolledWindow());
    colorPanelSW       = Gtk::manage(new MyScrolledWindow());
    transformPanelSW   = Gtk::manage(new MyScrolledWindow());
    rawPanelSW         = Gtk::manage(new MyScrolledWindow());
    advancedPanelSW    = Gtk::manage(new MyScrolledWindow());
    locallabPanelSW    = Gtk::manage(new MyScrolledWindow());    
    
    trashPanelSW       = Gtk::manage(new MyScrolledWindow());
    usefulPanelSW      = Gtk::manage(new MyScrolledWindow());
    updateVScrollbars(options.hideTPVScrollbar);

    // load panel endings
    for(int i = 0; i < NB_PANEL; i++) {
        vbPanelEnd[i] = Gtk::manage(new Gtk::VBox());
        imgPanelEnd[i] = Gtk::manage(new RTImage("ornament1.png"));
        imgPanelEnd[i]->show();
        vbPanelEnd[i]->pack_start(*imgPanelEnd[i], Gtk::PACK_SHRINK);
        vbPanelEnd[i]->show_all();
        hsPanelEnd[i] = Gtk::manage(new Gtk::HSeparator);
    }
    if (useRtFav)
    {
      if (favoriteCount > 0) {
        favoritePanelSW = Gtk::manage(new MyScrolledWindow());
        favoritePanelSW->add(*favoritePanel);
        favoritePanel->pack_start(*vbPanelEnd[0], Gtk::PACK_SHRINK, 4);     
      }
    }
    updateVScrollbars(options.hideTPVScrollbar);

    int panelIter = 0;
    if ((!useRtFav) || favoriteCount > 0 )
      handlePanel(favoritePanel, favoritePanelSW, panelIter++, 4); //0
    handlePanel(exposurePanel, exposurePanelSW, panelIter++, 4);   //1
    handlePanel(detailsPanel, detailsPanelSW, panelIter++, 4);     //2
    handlePanel(colorPanel, colorPanelSW, panelIter++, 4);         //3
    handlePanel(advancedPanel, advancedPanelSW, panelIter++,4);    //4
    handlePanel(transformPanel, transformPanelSW, panelIter++, 4); //5
    handlePanel(locallabPanel, locallabPanelSW, panelIter++, 4);   //6
    handlePanel(rawPanel, rawPanelSW, panelIter++, 4);             //7
    handlePanel(usefulPanel, usefulPanelSW, panelIter++, 4);       //8
    handlePanel(trashPanel, trashPanelSW, panelIter++, 4);         //9

    if ( options.rtSettings.verbose )
      printf("panel handling performed. \n");

     printf("panel");

    for(int i=PANEL_SWITCHABLE_START; i< PANEL_SWITCHABLE_START + NB_PANEL_SWITCHABLE; i++) { //last panel is trash thus ignored
      printf(" %i",i);
      int modOp  = NB_PANEL_SWITCHABLE ; //-2 because we ignore first panel and trash panel
      // I really don't want to know how modulo negative number is h@ndled 
      // so i am adding nbPanel to the values
      // -- esby
      ToolVBox* box1 =  static_cast<ToolVBox*>(vbPanel[PANEL_SWITCHABLE_START +((i+modOp-2) %(modOp))]);
      ToolVBox* box2 =  static_cast<ToolVBox*>(vbPanel[i]);
      box2->setPrevBox(box1);
      box1->setNextBox(box2);
      env->addVBox(box1);
    }
    env->doLog=true;
    printf(" done.\n");

  //allowing those filters to be extracted from their entity

    ToolVBox* box =  static_cast<ToolVBox*>(lensgeom->getPackBox());

    box->setBoxName("lensgeom");
    box->setNextBox(transformPanel);
    box->setPrevBox(transformPanel);
    env->addVBox(box);

    toiF = Gtk::manage(new TextOrIcon("star.png", M("MAIN_TAB_FAVORITES"), M("MAIN_TAB_FAVORITES_TOOLTIP")));
    toiE = Gtk::manage(new TextOrIcon("exposure.png", M("MAIN_TAB_EXPOSURE"), M("MAIN_TAB_EXPOSURE_TOOLTIP")));
    toiD = Gtk::manage(new TextOrIcon("detail.png", M("MAIN_TAB_DETAIL"), M("MAIN_TAB_DETAIL_TOOLTIP")));
    toiC = Gtk::manage(new TextOrIcon("color-circles.png", M("MAIN_TAB_COLOR"), M("MAIN_TAB_COLOR_TOOLTIP")));
    toiW = Gtk::manage(new TextOrIcon("atom.png", M("MAIN_TAB_ADVANCED"), M("MAIN_TAB_ADVANCED_TOOLTIP")));
    toiL = Gtk::manage(new TextOrIcon("hand-open.png", M("MAIN_TAB_LOCALLAB"), M("MAIN_TAB_LOCALLAB_TOOLTIP")));
    toiT = Gtk::manage(new TextOrIcon("transform.png", M("MAIN_TAB_TRANSFORM"), M("MAIN_TAB_TRANSFORM_TOOLTIP")));
    toiR = Gtk::manage(new TextOrIcon("bayer.png", M("MAIN_TAB_RAW"), M("MAIN_TAB_RAW_TOOLTIP")));
    toiM = Gtk::manage(new TextOrIcon("metadata.png", M("MAIN_TAB_METADATA"), M("MAIN_TAB_METADATA_TOOLTIP")));
    toiF = Gtk::manage(new TextOrIcon("favorite.png" , M("MAIN_TAB_FAVORITE") , M("MAIN_TAB_FAVORITE_TOOLTIP") ));
    toiP = Gtk::manage(new TextOrIcon("trash.png"    , M("MAIN_TAB_TRASH") ,    M("MAIN_TAB_TRASH_TOOLTIP") ));
    toiU = Gtk::manage(new TextOrIcon("useful.png"   , M("MAIN_TAB_USEFUL") ,   M("MAIN_TAB_USEFUL_TOOLTIP") ));


    if ((!useRtFav) || (favoriteCount>0 ))
        toolPanelNotebook->append_page(*favoritePanelSW,  *toiF);

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

    toolPanelNotebook->append_page(*exposurePanelSW,  *toiE);
    toolPanelNotebook->append_page(*detailsPanelSW,   *toiD);
    toolPanelNotebook->append_page(*colorPanelSW,     *toiC);
    toolPanelNotebook->append_page(*advancedPanelSW,   *toiW);

    // Locallab notebook is hidden in batch mode
    if (!batch) {
        toolPanelNotebook->append_page(*locallabPanelSW,   *toiL);
    }

    toolPanelNotebook->append_page(*transformPanelSW, *toiT);
    toolPanelNotebook->append_page(*rawPanelSW,       *toiR);
    toolPanelNotebook->append_page(*metadata,         *toiM);
    toolPanelNotebook->append_page(*usefulPanelSW,    *toiU);
    toolPanelNotebook->append_page(*trashPanelSW,     *toiP);

    toolPanelNotebook->set_current_page(0);

    toolPanelNotebook->set_scrollable();
    toolPanelNotebook->show_all();

    notebookconn = toolPanelNotebook->signal_switch_page().connect(
                       sigc::mem_fun(*this, &ToolPanelCoordinator::notebookPageChanged));

    if ( options.rtSettings.verbose )
      printf("Starting toolpanel deployment\n");
    doDeploy();
    if ( options.rtSettings.verbose )
      printf("Panel deployment finished\n");

    // In batch mode, notebookPageChanged method is blocked because it's useless to display spots
    if (batch) {
        notebookconn.block(true);
    }

    for (auto toolPanel : toolPanels) {
        toolPanel->setListener(this);
    }

    whitebalance->setWBProvider(this);
    whitebalance->setSpotWBListener(this);
    darkframe->setDFProvider(this);
    flatfield->setFFProvider(this);
    lensgeom->setLensGeomListener(this);
    rotate->setLensGeomListener(this);
    perspective->setLensGeomListener(this);
    perspective->setPerspCorrectionPanelListener(this);
    distortion->setLensGeomListener(this);
    crop->setCropPanelListener(this);
    icm->setICMPanelListener(this);
    filmNegative->setFilmNegProvider(this);

    toolBar = new ToolBar();
    toolBar->setToolBarListener(this);
    toolPanelNotebook->signal_switch_page().connect(sigc::mem_fun(*this,  &ToolPanelCoordinator::on_notebook_switch_page) );
}

void ToolPanelCoordinator::notebookPageChanged(Gtk::Widget* page, guint page_num)
{
    // Locallab spot curves are set visible if at least one photo has been loaded (to avoid
    // segfault) and locallab panel is active
    if (photoLoadedOnce) {
        if (page == locallabPanelSW) {
            toolBar->blockEditDeactivation(); // Avoid edit tool deactivation when Locallab page is active (except if pressing other tools button)
            locallab->subscribe();
        }

        if (prevPage == locallabPanelSW) { // To deactivate Locallab only when switching from Locallab page
            toolBar->blockEditDeactivation(false);
            locallab->unsubscribe();
        }

        prevPage = page;
    }
}

void ToolPanelCoordinator::addPanel(Gtk::Box* where, FoldableToolPanel* panel, int level)
{
    env->registerPanel(where, panel, level);

    panel->setParent(where);
    panel->setLevel(level);
/*  
    // this is performed by handle panel
    expList.push_back(panel->getExpander());
    where->pack_start(*panel->getExpander(), false, false);
    toolPanels.push_back(panel);
*/
}
void ToolPanelCoordinator::addfavoritePanel(Gtk::Box* where, FoldableToolPanel* panel, int level)
{
  if (useRtFav){
    auto name = panel->getToolName();
    auto it = std::find(options.favorites.begin(), options.favorites.end(), name);
    if (it != options.favorites.end()) {
        int index = std::distance(options.favorites.begin(), it);
        favorites[index] = panel;
    } else {
        addPanel(where, panel, level);
    }
  }
  else
    addPanel(where, panel, level);
}

ToolPanelCoordinator::~ToolPanelCoordinator ()
{
    idle_register.destroy();

    closeImage();

    // When deleting toolPanelNotebook, pages removal activates notebookPageChanged function
    // which is responsible of segfault if listener isn't deactivated before
    notebookconn.block(true);

    env->disableSwitchPageReaction = true;
    delete toolPanelNotebook;
    delete toolBar;
    delete env;
}

void ToolPanelCoordinator::doDeploy()
{
    for(auto toolPanel : toolPanels) 
    {
//      printf("panel nb=%i \n",  i);
        toolPanel->deploy();
    }
}

void ToolPanelCoordinator::doDeployLate()
{
//    if ( options.rtSettings.verbose )
      printf("late panel deployment for env=#%i \n", env->getEnvRef());
    for(auto toolPanel : toolPanels) 
    {   
//      printf("panel nb=%i \n",  i);
      toolPanel->deployLate();
    }
    env->disableSwitchPageReaction = false;

}

void ToolPanelCoordinator::doReact(FakeProcEvent ev)
{
    if ( options.rtSettings.verbose ) 
      printf("enabling panel reac for envId=%i\n", env->getId());

    if (!isReaction)
    {
      isReaction = true;

      for(auto toolPanel : toolPanels) 
        toolPanel->setReacted(false);

      for(auto toolPanel : toolPanels) 
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
   panelSW->add(*vbox);
   vbox->pack_start(*hsPanelEnd[panelIterator], Gtk::PACK_SHRINK,0);
   vbox->pack_start(*vbPanelEnd[panelIterator], Gtk::PACK_SHRINK,spacing);
   vbPanel[panelIterator] = vbox;
   ToolVBox* box =  static_cast<ToolVBox*>(vbox);
   box->setParent(toolPanelNotebook);
   box->setParentSW(panelSW);
}


void ToolPanelCoordinator::imageTypeChanged(bool isRaw, bool isBayer, bool isXtrans, bool isMono)
{
    if (isRaw) {
        if (isBayer) {
            idle_register.add(
                [this]() -> bool
                {
                    rawPanelSW->set_sensitive(true);
                    sensorxtrans->FoldableToolPanel::hide();
                    xtransprocess->FoldableToolPanel::hide();
                    bayerrawexposure->FoldableToolPanel::show();
                    xtransrawexposure->FoldableToolPanel::hide();
                    sensorbayer->FoldableToolPanel::show();
                    bayerprocess->FoldableToolPanel::show();
                    bayerpreprocess->FoldableToolPanel::show();
                    rawcacorrection->FoldableToolPanel::show();
                    preprocessWB->FoldableToolPanel::show();
                    preprocess->FoldableToolPanel::show();
                    flatfield->FoldableToolPanel::show();
                    pdSharpening->FoldableToolPanel::show();
                    retinex->FoldableToolPanel::setGrayedOut(false);
                    return false;
                }
            );
        } else if (isXtrans) {
            idle_register.add(
                [this]() -> bool
                {
                    rawPanelSW->set_sensitive(true);
                    sensorxtrans->FoldableToolPanel::show();
                    xtransprocess->FoldableToolPanel::show();
                    xtransrawexposure->FoldableToolPanel::show();
                    bayerrawexposure->FoldableToolPanel::hide();
                    sensorbayer->FoldableToolPanel::hide();
                    bayerprocess->FoldableToolPanel::hide();
                    bayerpreprocess->FoldableToolPanel::hide();
                    rawcacorrection->FoldableToolPanel::hide();
                    preprocessWB->FoldableToolPanel::show();
                    preprocess->FoldableToolPanel::show();
                    flatfield->FoldableToolPanel::show();
                    pdSharpening->FoldableToolPanel::show();
                    retinex->FoldableToolPanel::setGrayedOut(false);
                    return false;
                }
            );
        } else if (isMono) {
            idle_register.add(
                [this]() -> bool
                {
                    rawPanelSW->set_sensitive(true);
                    sensorbayer->FoldableToolPanel::hide();
                    bayerprocess->FoldableToolPanel::hide();
                    bayerpreprocess->FoldableToolPanel::hide();
                    rawcacorrection->FoldableToolPanel::hide();
                    sensorxtrans->FoldableToolPanel::hide();
                    xtransprocess->FoldableToolPanel::hide();
                    xtransrawexposure->FoldableToolPanel::hide();
                    preprocessWB->FoldableToolPanel::hide();
                    preprocess->FoldableToolPanel::hide();
                    flatfield->FoldableToolPanel::show();
                    pdSharpening->FoldableToolPanel::show();
                    retinex->FoldableToolPanel::setGrayedOut(false);
                    return false;
                }
            );
        } else {
            idle_register.add(
                [this]() -> bool
                {
                    rawPanelSW->set_sensitive(true);
                    sensorbayer->FoldableToolPanel::hide();
                    bayerprocess->FoldableToolPanel::hide();
                    bayerpreprocess->FoldableToolPanel::hide();
                    rawcacorrection->FoldableToolPanel::hide();
                    sensorxtrans->FoldableToolPanel::hide();
                    xtransprocess->FoldableToolPanel::hide();
                    xtransrawexposure->FoldableToolPanel::hide();
                    preprocessWB->FoldableToolPanel::hide();
                    preprocess->FoldableToolPanel::hide();
                    flatfield->FoldableToolPanel::hide();
                    pdSharpening->FoldableToolPanel::hide();
                    retinex->FoldableToolPanel::setGrayedOut(false);
                    return false;
                }
            );
        }
    } else {
        idle_register.add(
            [this]() -> bool
            {
                rawPanelSW->set_sensitive(false);
                sensorbayer->FoldableToolPanel::hide();
                bayerprocess->FoldableToolPanel::hide();
                bayerpreprocess->FoldableToolPanel::hide();
                rawcacorrection->FoldableToolPanel::hide();
                sensorxtrans->FoldableToolPanel::hide();
                xtransprocess->FoldableToolPanel::hide();
                xtransrawexposure->FoldableToolPanel::hide();
                preprocessWB->FoldableToolPanel::hide();
                preprocess->FoldableToolPanel::hide();
                flatfield->FoldableToolPanel::hide();
                pdSharpening->FoldableToolPanel::hide();
                retinex->FoldableToolPanel::setGrayedOut(true);
                return false;
            }
        );
    }

}

void ToolPanelCoordinator::setTweakOperator (rtengine::TweakOperator *tOperator)
{
    if (ipc && tOperator) {
        ipc->setTweakOperator(tOperator);
    }
}

void ToolPanelCoordinator::unsetTweakOperator (rtengine::TweakOperator *tOperator)
{
    if (ipc && tOperator) {
        ipc->unsetTweakOperator(tOperator);
    }
}

void ToolPanelCoordinator::refreshPreview (const rtengine::ProcEvent& event)
{
    if (!ipc) {
        return;
    }

    ProcParams* params = ipc->beginUpdateParams ();
    for (auto toolPanel : toolPanels) {
        toolPanel->write (params);
    }

    ipc->endUpdateParams (event);   // starts the IPC processing
}

void ToolPanelCoordinator::panelChanged(const rtengine::ProcEvent& event, const Glib::ustring& descr)
{
    if (!ipc) {
        return;
    }

    int changeFlags = rtengine::RefreshMapper::getInstance()->getAction(event);

    ProcParams* params = ipc->beginUpdateParams();

    for (auto toolPanel : toolPanels) {
        toolPanel->write(params);
    }

    // Compensate rotation on flip
    if (event == rtengine::EvCTHFlip || event == rtengine::EvCTVFlip) {
        if (fabs(params->rotate.degree) > 0.001) {
            params->rotate.degree *= -1;
            changeFlags |= rtengine::RefreshMapper::getInstance()->getAction(rtengine::EvROTDegree);
            rotate->read(params);
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
        ipc->getInitialImage()->getImageSource()->getFullSize(fw, fh, tr);
        gradient->updateGeometry(params->gradient.centerX, params->gradient.centerY, params->gradient.feather, params->gradient.degree, fw, fh);
    }

    // some transformations make the crop change for convenience
    if (event == rtengine::EvCTHFlip) {
        crop->hFlipCrop();
        crop->write(params);
    } else if (event == rtengine::EvCTVFlip) {
        crop->vFlipCrop();
        crop->write(params);
    } else if (event == rtengine::EvCTRotate) {
        crop->rotateCrop(params->coarse.rotate, params->coarse.hflip, params->coarse.vflip);
        crop->write(params);
        resize->update(params->crop.enabled, params->crop.w, params->crop.h, ipc->getFullWidth(), ipc->getFullHeight());
        resize->write(params);
    } else if (event == rtengine::EvCrop) {
        resize->update(params->crop.enabled, params->crop.w, params->crop.h);
        resize->write(params);
    }

    /*
     * Manage Locallab mask visibility:
     * - Mask preview is updated when choosing a mask preview method
     * - Mask preview is also updated when modifying (to avoid hiding a potentially visible mask combobox):
     *   - Color&Light invers
     *   - Exposure inversex
     *   - Shadow Highlight inverssh
     *   - Soft Light softMethod
     * - Mask preview is stopped when creating, deleting or selecting a spot
     * - Mask preview is also stopped when removing a spot or resetting all mask visibility
     */
    if (event == rtengine::EvlocallabshowmaskMethod) {
        const Locallab::llMaskVisibility maskStruc = locallab->getMaskVisibility();
        ipc->setLocallabMaskVisibility(maskStruc.previewDeltaE, maskStruc.colorMask, maskStruc.colorMaskinv, maskStruc.expMask, maskStruc.expMaskinv,
                maskStruc.SHMask, maskStruc.SHMaskinv, maskStruc.vibMask, maskStruc.softMask,
                maskStruc.blMask, maskStruc.tmMask, maskStruc.retiMask, maskStruc.sharMask,
                maskStruc.lcMask, maskStruc.cbMask, maskStruc.logMask, maskStruc.maskMask, maskStruc.cieMask);
    } else if (event == rtengine::EvLocallabSpotCreated || event == rtengine::EvLocallabSpotSelectedWithMask ||
            event == rtengine::EvLocallabSpotDeleted || event == rtengine::Evlocallabshowreset ||
            event == rtengine::EvlocallabToolRemovedWithRefresh) {
        locallab->resetMaskVisibility();
        ipc->setLocallabMaskVisibility(false, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }

    ipc->endUpdateParams(changeFlags);    // starts the IPC processing

    hasChanged = true;

    for (auto paramcListener : paramcListeners) {
        paramcListener->procParamsChanged(params, event, descr);
    }

    // Locallab spot curves are set visible if at least one photo has been loaded (to avoid
    // segfault) and locallab panel is active
    // When a new photo is loaded, Locallab spot curves need to be set visible again
const auto func =
    [this]() -> bool
    {
        if (photoLoadedOnce && (toolPanelNotebook->get_nth_page(toolPanelNotebook->get_current_page()) == locallabPanelSW)) {
            locallab->subscribe();
       }

        return false;
    };

if (event == rtengine::EvPhotoLoaded) {
    idle_register.add(func);
}

    photoLoadedOnce = true;

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

    ProcParams *params = ipc->beginUpdateParams();
    ProcParams *mergedParams = new ProcParams();

    // Copy the current params as default values for the fusion
    *mergedParams = *params;

    // Reset IPTC values when switching procparams from the History
    if (event == rtengine::EvHistoryBrowsed) {
        mergedParams->iptc.clear();
        mergedParams->exif.clear();
    }

    // And apply the partial profile nparams to mergedParams
    nparams->applyTo(mergedParams, fromLastSave);

    // Derive the effective changes, if it's a profile change, to prevent slow RAW rerendering if not necessary
    bool filterRawRefresh = false;

    if (event != rtengine::EvPhotoLoaded) {
        ParamsEdited pe(true);
        std::vector<rtengine::procparams::ProcParams> lParams(2);
        lParams[0] = *params;
        lParams[1] = *mergedParams;
        pe.initFrom(lParams);

        filterRawRefresh = pe.raw.isUnchanged() && pe.lensProf.isUnchanged() && pe.retinex.isUnchanged() && pe.pdsharpening.isUnchanged();
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
    ipc->getInitialImage()->getImageSource()->getFullSize(fw, fh, tr);
    crop->trim(params, fw, fh);

    // updating the GUI with updated values
    for (auto toolPanel : toolPanels) {
        toolPanel->read(params);

        if (event == rtengine::EvPhotoLoaded || event == rtengine::EvProfileChanged) {
            toolPanel->autoOpenCurve();

            // For Locallab, reset tool expanders visibility only when a photo or profile is loaded
            locallab->openAllTools();
        }
    }

    if (event == rtengine::EvPhotoLoaded || event == rtengine::EvProfileChanged || event == rtengine::EvHistoryBrowsed || event == rtengine::EvCTRotate) {
        // updating the "on preview" geometry
        gradient->updateGeometry(params->gradient.centerX, params->gradient.centerY, params->gradient.feather, params->gradient.degree, fw, fh);
    }

    // Reset Locallab mask visibility
    locallab->resetMaskVisibility();
    ipc->setLocallabMaskVisibility(false, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    // start the IPC processing
    if (filterRawRefresh) {
        ipc->endUpdateParams(rtengine::RefreshMapper::getInstance()->getAction(event) & ALLNORAW);
    } else {
        ipc->endUpdateParams(event);
    }

    hasChanged = event != rtengine::EvProfileChangeNotification;

    for (auto paramcListener : paramcListeners) {
        paramcListener->procParamsChanged(params, event, descr);
    }

    // Locallab spot curves are set visible if at least one photo has been loaded (to avoid
    // segfault) and locallab panel is active
    // When a new photo is loaded, Locallab spot curves need to be set visible again
const auto func =
    [this]() -> bool
    {
        if (photoLoadedOnce && (toolPanelNotebook->get_nth_page(toolPanelNotebook->get_current_page()) == locallabPanelSW)) {
            locallab->subscribe();
        }

        return false;
    };

if (event == rtengine::EvPhotoLoaded) {
    idle_register.add(func);
}

    photoLoadedOnce = true;
}

void ToolPanelCoordinator::setDefaults(const ProcParams* defparams)
{
    if (defparams) {
        for (auto toolPanel : toolPanels) {
            toolPanel->setDefaults(defparams);
        }
    }
}

CropGUIListener* ToolPanelCoordinator::getCropGUIListener()
{

    return crop;
}

void parseDirectory(rtexif::TagDirectory* d, Glib::ustring prefix, Environment* env)
{
  if (d)
    for(int i = 0; i < d->getCount(); ++i)
    {
       rtexif::Tag* t = d->getTagByIndex(i);

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

void ToolPanelCoordinator::initImage(rtengine::StagedImageProcessor* ipc_, bool raw)
{

    ipc = ipc_;
    toneCurve->disableListener();
    toneCurve->enableAll();
    toneCurve->enableListener();

    if (ipc) {
        const rtengine::FramesMetaData* pMetaData = ipc->getInitialImage()->getMetaData();
        metadata->setImageData(pMetaData);

       const rtengine::FramesMetaData* idata = ipc->getInitialImage()->getMetaData();
       if( options.rtSettings.verbose ) 
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
        if (pMetaData->hasExif ()) 
        {
          rtexif::TagDirectory* root = pMetaData->getRootExifData() ;
          parseDirectory(root,ROOT_EXIF_PREFIX,env);
          printf("full exif values tranmitted by variables \n");
          doReact(FakeEvFullExifTransmitted);
        }

        ipc->setAutoExpListener(toneCurve);
        ipc->setAutoCamListener(colorappearance);
        ipc->setAutoBWListener(blackwhite);
        ipc->setFrameCountListener(bayerprocess);
        ipc->setFlatFieldAutoClipListener (flatfield);
        ipc->setBayerAutoContrastListener (bayerprocess);
        ipc->setXtransAutoContrastListener (xtransprocess);
        ipc->setpdSharpenAutoContrastListener (pdSharpening);
        ipc->setpdSharpenAutoRadiusListener (pdSharpening);
        ipc->setAutoWBListener(whitebalance);
        ipc->setAutoColorTonListener(colortoning);
        ipc->setAutoprimListener(icm);
        ipc->setAutoChromaListener(dirpyrdenoise);
        ipc->setWaveletListener(wavelet);
        ipc->setRetinexListener(retinex);
        ipc->setSizeListener(crop);
        ipc->setSizeListener(resize);
        ipc->setLocallabListener(locallab);
        ipc->setImageTypeListener(this);
        ipc->setFilmNegListener(filmNegative);
        flatfield->setShortcutPath(Glib::path_get_dirname(ipc->getInitialImage()->getFileName()));

        icm->setRawMeta(raw, (const rtengine::FramesData*)pMetaData);
        lensProf->setRawMeta(raw, pMetaData);
        perspective->setMetadata(pMetaData);
    }


    toneCurve->setRaw(raw);
    hasChanged = true;
}


void ToolPanelCoordinator::closeImage()
{

    if (ipc) {
        ipc->stopProcessing();
        ipc = nullptr;
    }
}

void ToolPanelCoordinator::closeAllTools()
{
    for (size_t i = 0; i < options.tpOpen.size(); ++i) {
        if (i < expList.size()) {
            expList[i]->set_expanded(false);
        }
    }
}

void ToolPanelCoordinator::openAllTools()
{
    for (size_t i = 0; i < options.tpOpen.size(); ++i) {
        if (i < expList.size()) {
            expList[i]->set_expanded(true);
        }
    }
}

void ToolPanelCoordinator::updateToolState()
{
    if (options.tpOpen.empty()) {
        for (auto expander : expList) {
            expander->set_expanded(false);
        }

        wavelet->updateToolState({});
        retinex->updateToolState({});

        return;
    }

    for (size_t i = 0; i < options.tpOpen.size(); ++i) {
        if (i < expList.size()) {
            expList[i]->set_expanded(options.tpOpen[i]);
        }
    }

    if (options.tpOpen.size() > expList.size()) {
        const size_t sizeWavelet = options.tpOpen.size() - expList.size();

        std::vector<int> temp;

        for (size_t i = 0; i < sizeWavelet; ++i) {
            temp.push_back(options.tpOpen[i + expList.size()]);
        }

        wavelet->updateToolState(temp);
        retinex->updateToolState(temp);
    }
}

void ToolPanelCoordinator::readOptions()
{

    crop->readOptions();
}

void ToolPanelCoordinator::writeOptions()
{

    crop->writeOptions();

    if (options.autoSaveTpOpen) {
        writeToolExpandedStatus(options.tpOpen);
    }
}


void ToolPanelCoordinator::writeToolExpandedStatus(std::vector<int> &tpOpen)
{
    tpOpen.clear();

    for (size_t i = 0; i < expList.size(); i++) {
        tpOpen.push_back(expList.at(i)->get_expanded());
    }

    wavelet->writeOptions(tpOpen);
    retinex->writeOptions(tpOpen);

}


void ToolPanelCoordinator::updateShowtooltipVisibility (bool showtooltip)
{
    locallab->updateShowtooltipVisibility(showtooltip);
}


void ToolPanelCoordinator::spotWBselected(int x, int y, Thumbnail* thm)
{
    if (!ipc) {
        return;
    }

//    toolBar->setTool (TOOL_HAND);
    int rect = whitebalance->getSize();
    int ww = ipc->getFullWidth();
    int hh = ipc->getFullHeight();

    if (x - rect > 0 && y - rect > 0 && x + rect < ww && y + rect < hh) {
        double temp;
        double green;
        ipc->getSpotWB(x, y, rect, temp, green);
        whitebalance->setWB(temp, green);
    }
}

void ToolPanelCoordinator::sharpMaskSelected(bool sharpMask)
{
    if (!ipc) {
        return;
    }

    ipc->beginUpdateParams();
    ipc->endUpdateParams (ipc->setSharpMask(sharpMask));
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

void ToolPanelCoordinator::autoCropRequested()
{

    if (!ipc) {
        return;
    }

    int x1, y1, x2, y2, w, h;
    ipc->getAutoCrop(crop->getRatio(), x1, y1, w, h);
    x2 = x1 + w - 1;
    y2 = y1 + h - 1;
    crop->cropInit(x1, y1, w, h);
    crop->cropResized(x1, y1, x2, y2);
    crop->cropManipReady();
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
        std::string maker(imd->getMake());
        std::string model(imd->getModel());
        time_t timestamp = imd->getDateTimeAsTS();

        return rtengine::dfm.searchDarkFrame(maker, model, iso, shutter, timestamp);
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
        // int iso = imd->getISOSpeed();              temporarily removed because unused
        // double shutter = imd->getShutterSpeed();   temporarily removed because unused
        double aperture = imd->getFNumber();
        double focallength = imd->getFocalLen();
        std::string maker(imd->getMake());
        std::string model(imd->getModel());
        std::string lens(imd->getLens());
        time_t timestamp = imd->getDateTimeAsTS();

        return rtengine::ffm.searchFlatField(maker, model, lens, focallength, aperture, timestamp);
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

void ToolPanelCoordinator::straightenRequested()
{

    if (!ipc) {
        return;
    }

    toolBar->setTool(TMStraighten);
}

void ToolPanelCoordinator::autoPerspRequested (bool corr_pitch, bool corr_yaw, double& rot, double& pitch, double& yaw, const std::vector<rtengine::ControlLine> *lines)
{
    if (!(ipc && (corr_pitch || corr_yaw))) {
        return;
    }

    rtengine::ImageSource *src = dynamic_cast<rtengine::ImageSource *>(ipc->getInitialImage());
    if (!src) {
        return;
    }

    rtengine::procparams::ProcParams params;
    ipc->getParams(&params);

    auto res = rtengine::PerspectiveCorrection::autocompute(src, corr_pitch, corr_yaw, &params, src->getMetaData(), lines);
    rot = res.angle;
    pitch = res.pitch;
    yaw = res.yaw;
}

double ToolPanelCoordinator::autoDistorRequested()
{
    if (!ipc) {
        return 0.0;
    }

    return rtengine::ImProcFunctions::getAutoDistor(ipc->getInitialImage()->getFileName(), 400);
}

void ToolPanelCoordinator::spotWBRequested(int size)
{

    if (!ipc) {
        return;
    }

    toolBar->setTool(TMSpotWB);
}

void ToolPanelCoordinator::cropSelectRequested()
{

    if (!ipc) {
        return;
    }

    toolBar->setTool(TMCropSelect);
}

void ToolPanelCoordinator::controlLineEditModeChanged(bool active)
{
    if (!ipc) {
        return;
    }

    if (active) {
        toolBar->setTool(TMPerspective);
    }
}

void ToolPanelCoordinator::saveInputICCReference(const Glib::ustring& fname, bool apply_wb)
{
    if (ipc) {
        ipc->saveInputICCReference(fname, apply_wb);
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

void ToolPanelCoordinator::foldAllButOne(Gtk::Box* parent, FoldableToolPanel* openedSection)
{

    for (auto toolPanel : toolPanels) {
        if (toolPanel->getParent() != nullptr) {
            ToolPanel* currentTP = toolPanel;

            if ((env->state == ENV_STATE_IN_FAV)
            ||((currentTP->getParent() == parent) 
            &&(!currentTP->canBeIgnored()))) {
                // Section in the same tab, we unfold it if it's not the one that has been clicked
                if (currentTP != openedSection) {
                    currentTP->setExpanded(false);
                } else {
                    if (!currentTP->getExpanded()) {
                        currentTP->setExpanded(true);
                    }
                }
            }
        }
    }
}

bool ToolPanelCoordinator::handleShortcutKey(GdkEventKey* event)
{

    //bool ctrl = event->state & GDK_CONTROL_MASK;  temporarily removed because unused
    //bool shift = event->state & GDK_SHIFT_MASK;   temporarily removed because unused
    bool alt = event->state & GDK_MOD1_MASK;

    if (alt) {
        switch (event->keyval) {
            case GDK_KEY_f:
                if ((!useRtFav) || (favoriteCount > 0 )) {
                toolPanelNotebook->set_current_page (toolPanelNotebook->page_num(*favoritePanelSW));
                }
                return true;

            case GDK_KEY_e:
                toolPanelNotebook->set_current_page(toolPanelNotebook->page_num(*exposurePanelSW));
                return true;

            case GDK_KEY_d:
                toolPanelNotebook->set_current_page(toolPanelNotebook->page_num(*detailsPanelSW));
                return true;

            case GDK_KEY_c:
                toolPanelNotebook->set_current_page(toolPanelNotebook->page_num(*colorPanelSW));
                return true;

            case GDK_KEY_t:
                toolPanelNotebook->set_current_page(toolPanelNotebook->page_num(*transformPanelSW));
                return true;

            case GDK_KEY_r:
                toolPanelNotebook->set_current_page(toolPanelNotebook->page_num(*rawPanelSW));
                return true;

            case GDK_KEY_a:
                toolPanelNotebook->set_current_page(toolPanelNotebook->page_num(*advancedPanelSW));
                return true;

            case GDK_KEY_o:
                toolPanelNotebook->set_current_page(toolPanelNotebook->page_num(*locallabPanelSW));
                return true;

            case GDK_KEY_m:
                toolPanelNotebook->set_current_page(toolPanelNotebook->page_num(*metadata));
                return true;

            case GDK_KEY_p:
                toolPanelNotebook->set_current_page(toolPanelNotebook->page_num(*trashPanelSW));
                return true;

            case GDK_KEY_u:
                 toolPanelNotebook->set_current_page(toolPanelNotebook->page_num(*usefulPanelSW));
                 return true;
        }
    }

    return false;
}

void ToolPanelCoordinator::updateVScrollbars(bool hide)
{
    GThreadLock lock; // All GUI access from idle_add callbacks or separate thread HAVE to be protected
    Gtk::PolicyType policy = hide ? Gtk::POLICY_NEVER : Gtk::POLICY_AUTOMATIC;
    if ((!useRtFav) || (favoriteCount > 0)) {
          favoritePanelSW->set_policy    (Gtk::POLICY_AUTOMATIC, policy);
    }
    exposurePanelSW->set_policy    (Gtk::POLICY_AUTOMATIC, policy);
    detailsPanelSW->set_policy     (Gtk::POLICY_AUTOMATIC, policy);
    colorPanelSW->set_policy       (Gtk::POLICY_AUTOMATIC, policy);
    transformPanelSW->set_policy   (Gtk::POLICY_AUTOMATIC, policy);
    rawPanelSW->set_policy         (Gtk::POLICY_AUTOMATIC, policy);
    advancedPanelSW->set_policy    (Gtk::POLICY_AUTOMATIC, policy);
    locallabPanelSW->set_policy    (Gtk::POLICY_AUTOMATIC, policy);
    trashPanelSW->set_policy       (Gtk::POLICY_AUTOMATIC, policy);
    usefulPanelSW->set_policy      (Gtk::POLICY_AUTOMATIC, policy);


    for (auto currExp : expList) {
        currExp->updateVScrollbars(hide);
    }
}

void ToolPanelCoordinator::updateTPVScrollbar(bool hide)
{
    updateVScrollbars(hide);
}

void ToolPanelCoordinator::toolDeselected(ToolMode tool)
{
    if (tool == TMPerspective) {
        perspective->requestApplyControlLines();
    }
}

void ToolPanelCoordinator::toolSelected(ToolMode tool)
{
    GThreadLock lock; // All GUI access from idle_add callbacks or separate thread HAVE to be protected
    notebookconn.block(true); // "signal_switch_page" event is blocked to avoid unsubscribing Locallab (allows a correct behavior when switching to another tool using toolbar)

    auto checkFavorite = [this](FoldableToolPanel* tool) {
    if (!useRtFav) {
        for (auto fav : favorites) {
            if (fav == tool) {
                return true;
            }
        }
        return false;
      }
    };

   //todo: the code is never called ?
   
    switch (tool) {
        case TMCropSelect: {
            toolBar->blockEditDeactivation(false); // To allow deactivating Locallab when switching to another tool using toolbar
            crop->setExpanded(true);
            toolPanelNotebook->set_current_page(toolPanelNotebook->page_num(checkFavorite(crop) ? *favoritePanelSW : *transformPanelSW));
            prevPage = toolPanelNotebook->get_nth_page(toolPanelNotebook->get_current_page()); // Updating prevPage as "signal_switch_page" event
            break;
        }

        case TMSpotWB: {
            toolBar->blockEditDeactivation(false); // To allow deactivating Locallab when switching to another tool using toolbar
            whitebalance->setExpanded(true);
            toolPanelNotebook->set_current_page(toolPanelNotebook->page_num(checkFavorite(whitebalance) ? *favoritePanelSW : *colorPanelSW));
            prevPage = toolPanelNotebook->get_nth_page(toolPanelNotebook->get_current_page()); // Updating prevPage as "signal_switch_page" event
            break;
        }

        case TMStraighten: {
            toolBar->blockEditDeactivation(false); // To allow deactivating Locallab when switching to another tool using toolbar
            rotate->setExpanded(true);
            bool isFavorite = checkFavorite(rotate);
            if (!isFavorite) {
                isFavorite = checkFavorite(lensgeom);
                lensgeom->setExpanded(true);
            }
            toolPanelNotebook->set_current_page(toolPanelNotebook->page_num(isFavorite ? *favoritePanelSW : *transformPanelSW));
            prevPage = toolPanelNotebook->get_nth_page(toolPanelNotebook->get_current_page()); // Updating prevPage as "signal_switch_page" event
            break;
        }

        case TMPerspective: {
            toolBar->blockEditDeactivation(false); // To allow deactivating Locallab when switching to another tool using toolbar
            perspective->setControlLineEditMode(true);
            perspective->setExpanded(true);
            bool isFavorite = checkFavorite(perspective);
            if (!isFavorite) {
                isFavorite = checkFavorite(lensgeom);
                lensgeom->setExpanded(true);
            }
            toolPanelNotebook->set_current_page(toolPanelNotebook->page_num(isFavorite ? *favoritePanelSW : *transformPanelSW));
            prevPage = toolPanelNotebook->get_nth_page(toolPanelNotebook->get_current_page()); // Updating prevPage as "signal_switch_page" event
            break;
        }

        default:
            break;
    }

    notebookconn.block(false);
}

void ToolPanelCoordinator::editModeSwitchedOff()
{
    if (editDataProvider) {
        editDataProvider->switchOffEditMode();
    }
}

void ToolPanelCoordinator::dirSelected(const Glib::ustring& dirname, const Glib::ustring& openfile)
{

    flatfield->setShortcutPath(dirname);
}

void ToolPanelCoordinator::setEditProvider(EditDataProvider *provider)
{
    editDataProvider = provider;

    for (size_t i = 0; i < toolPanels.size(); i++) {
        toolPanels.at(i)->setEditProvider(provider);
    }
}

bool ToolPanelCoordinator::getFilmNegativeSpot(rtengine::Coord spot, int spotSize, RGB &refInput, RGB &refOutput)
{
    return ipc && ipc->getFilmNegativeSpot(spot.x, spot.y, spotSize, refInput, refOutput);
}

void ToolPanelCoordinator::on_notebook_switch_page(Gtk::Widget* /* page */, guint page_num){
  if (options.rtSettings.verbose)
    printf(" on_notebook_switch_page called\n");
  if (!env->disableSwitchPageReaction)
    {

    env->prevState = env->state;
    if (options.rtSettings.verbose)     
      printf("notebook switch page %c-> ", env->prevState);
    if (toolPanelNotebook->get_current_page() == toolPanelNotebook->page_num(*favoritePanelSW))
    {
       env->state = ENV_STATE_IN_FAV;      
       if (options.rtSettings.verbose)
         printf("%c -> favorite panel\n", env->state);
    }
    else 
    if (toolPanelNotebook->get_current_page() == toolPanelNotebook->page_num(*trashPanelSW)) 
    {
       env->state = ENV_STATE_IN_TRASH;
       if (options.rtSettings.verbose)
         printf("%c -> trash panel\n", env->state);
    }else
    {
      env->state = ENV_STATE_IN_NORM;
      if (options.rtSettings.verbose)
        printf("%c -> normal panel\n", env->state);
    }

   // we only checks outside of fav <> fav or trash <> trash interactions
   if (!((env->state == env->prevState)
    &&(env->state != ENV_STATE_IN_NORM)))
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

// done: this was splitted into three parts
// as it was causing positionning issues with favorite / normal panels, with position not loaded correctly
/*
    for(auto toolPanel : toolPanels) 
        toolPanel->favorite_others_tabs_switch(dc);
*/

    // first: favorite
    // then each normal panel in order
    // then trash panel

    // handling favorite panels
    std::vector<ToolPanel*> panels = env->getToolPanels();

    std::sort (panels.begin(), panels.end(), sortByFav);

    for (std::vector<ToolPanel*>::iterator it = panels.begin() ; it != panels.end(); ++it)
    {
     ToolPanel* p = static_cast<ToolPanel*>(*it);  

      if (p->getFavoriteButton()->get_active() == true)
      {
//        printf("Parsing VBox switch todo FAV %s pos=%i \n", p->getToolName().c_str(), p->getPosFav());
        p->favorite_others_tabs_switch(dc);
      }
    }

    panels = env->getToolPanels();
    std::sort (panels.begin(), panels.end(), sortByFav);
 
    /* - unused debug code
    for (std::vector<ToolPanel*>::iterator it1 = panels.begin() ; it1 != panels.end(); ++it1)
    {
      ToolPanel* p1 = static_cast<ToolPanel*>(*it1);
//      printf("%s.getPLocation= %i \n",p1->getToolName().c_str(),p1->getPLocation());

      for (std::vector<ToolPanel*>::iterator it2 = it1+1 ; it2 != panels.end(); ++it2)
      {
        ToolPanel* p2 = static_cast<ToolPanel*>(*it2);
        if ((p1->getFavoriteButton()->get_active() == true)
        && (p2->getFavoriteButton()->get_active() == true))
        {
          int pp1 = favoritePanel->getPos(p1);
          int pp2 = favoritePanel->getPos(p2);
 
//      printf("A: %s.ps %i  vs %s.ps %i \n", p1->getToolName().c_str(), pp1, p2->getToolName().c_str(), pp2);
          if (pp1 > pp2)
          {
             printf("Anomaly: %s.getPosFav() %i  > %s.getPosFav() %i \n", p1->getToolName().c_str(), p1->getPosFav(), p2->getToolName().c_str(), p2->getPosFav());

          }
        }
      }
    }
    */


    // second part normal panels are handled
    panels = env->getToolPanels();

    std::sort (panels.begin(), panels.end(), sortByOri);

    for (std::vector<ToolPanel*>::iterator it = panels.begin() ; it != panels.end(); ++it)
    {
     ToolPanel* p = static_cast<ToolPanel*>(*it);

      if ((p->getFavoriteButton()->get_active() == false)
         && (p->getTrashButton()->get_active() == false))
      {
//        printf("Parsing VBox switch todo NORM %s \n", p->getToolName().c_str());
        p->favorite_others_tabs_switch(dc);
      }
    }

    // third part trash panels
    for (std::vector<ToolPanel*>::iterator it = panels.begin() ; it != panels.end(); ++it)
    {
     ToolPanel* p = static_cast<ToolPanel*>(*it);

      if (p->getTrashButton()->get_active() == true)
      {
//        printf("Parsing VBox switch todo TRASH %s \n", p->getToolName().c_str());
        p->favorite_others_tabs_switch(dc);
      }
    }

/*
    for(auto toolPanel : toolPanels) 
        toolPanel->favorite_others_tabs_switch(dc);
*/

    //putting the ending panels and separator to the end
    for(int i=0; i< NB_PANEL; i++){
      int pos = toolPanels.size()-1;
      Gtk::Widget* w = static_cast<Gtk::Widget*>(vbPanelEnd[i]);
      vbPanel[i]->reorder_child(*w, pos);
      w =(Gtk::Widget*)hsPanelEnd[i];
      vbPanel[i]->reorder_child(*w, pos-1);
     }
   }
    // we update label info all the time
    // updating the label info(currently the position number)
    for(auto toolPanel : toolPanels) 
        toolPanel->updateLabelInfo();
  }
}


