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
#ifndef __TOOLPANELCCORD__
#define __TOOLPANELCCORD__

#include "../rtengine/rtengine.h"
#include "toolpanel.h"
#include <vector>
#include "pparamschangelistener.h"
#include "profilechangelistener.h"
#include "imageareatoollistener.h"
#include <gtkmm.h>
#include "whitebalance.h"
#include "coarsepanel.h"
#include "tonecurve.h"
#include "vibrance.h"
#include "colorappearance.h"
#include "shadowshighlights.h"
#include "impulsedenoise.h"
#include "defringe.h"
#include "dirpyrdenoise.h"
#include "epd.h"
#include "sharpening.h"
#include "labcurve.h"
#include "metadatapanel.h"
#include "crop.h"
#include "icmpanel.h"
#include "resize.h"
#include "chmixer.h"
#include "blackwhite.h"
#include "cacorrection.h"
#include "lensprofile.h"
#include "distortion.h"
#include "perspective.h"
#include "rotate.h"
#include "vignetting.h"
#include "retinex.h"
#include "gradient.h"
#include "pcvignette.h"
#include "toolbar.h"
#include "lensgeom.h"
#include "lensgeomlistener.h"
#include "wavelet.h"
#include "dirpyrequalizer.h"
#include "hsvequalizer.h"
#include "preprocess.h"
#include "bayerpreprocess.h"
#include "bayerprocess.h"
#include "xtransprocess.h"
#include "darkframe.h"
#include "flatfield.h"
#include "sensorbayer.h"
#include "sensorxtrans.h"
#include "rawcacorrection.h"
#include "rawexposure.h"
#include "bayerrawexposure.h"
#include "xtransrawexposure.h"
#include "sharpenmicro.h"
#include "sharpenedge.h"
#include "rgbcurves.h"
#include "colortoning.h"
#include "filmsimulation.h"
#include "prsharpening.h"
#include "fattaltonemap.h"
#include "localcontrast.h"
#include "softlight.h"
#include "dehaze.h"
#include "guiutils.h"
#include "ttsaver.h"
#include "ttfavoritecolorer.h"
#include "ttpanelcolorer.h"
#include "ttudlrhider.h"
#include "tttabhider.h"
#include "tttweaker.h"
#include "ttvardisplayer.h"
#include "ttisoprofiler.h"

//defining the number of panels for once.
#define NB_PANEL 9
#define NB_PANEL_SWITCHABLE 7
#define PANEL_SWITCHABLE_START 1

class ImageEditorCoordinator;

class ToolPanelCoordinator :
    public ToolPanelListener,
    public ToolBarListener,
    public ProfileChangeListener,
    public WBProvider,
    public DFProvider,
    public FFProvider,
    public LensGeomListener,
    public SpotWBListener,
    public CropPanelListener,
    public ICMPanelListener,
    public ImageAreaToolListener,
    public rtengine::ImageTypeListener
{
protected:
    Environment* env;
    bool isReaction;

    WhiteBalance* whitebalance;
    Vignetting* vignetting;
    Gradient* gradient;
    Retinex*  retinex;
    PCVignette* pcvignette;
    LensGeometry* lensgeom;
    LensProfilePanel* lensProf;
    Rotate* rotate;
    Distortion* distortion;
    PerspCorrection* perspective;
    CACorrection* cacorrection;
    ColorAppearance* colorappearance;
    Vibrance* vibrance;
    ChMixer* chmixer;
    BlackWhite* blackwhite;
    Resize* resize;
    PrSharpening* prsharpening;
    ICMPanel* icm;
    Crop* crop;
    ToneCurve* toneCurve;
    ShadowsHighlights* shadowshighlights;
    LocalContrast *localContrast;
    Defringe* defringe;
    ImpulseDenoise* impulsedenoise;
    DirPyrDenoise* dirpyrdenoise;
    EdgePreservingDecompositionUI *epd;
    Sharpening* sharpening;
    SharpenEdge* sharpenEdge;
    SharpenMicro* sharpenMicro;
    LCurve* lcurve;
    RGBCurves* rgbcurves;
    ColorToning* colortoning;
    Wavelet * wavelet;
    DirPyrEqualizer* dirpyrequalizer;
    HSVEqualizer* hsvequalizer;
    SoftLight *softlight;
    Dehaze *dehaze;
    FilmSimulation *filmSimulation;
    SensorBayer * sensorbayer;
    SensorXTrans * sensorxtrans;
    BayerProcess* bayerprocess;
    XTransProcess* xtransprocess;
    BayerPreProcess* bayerpreprocess;
    PreProcess* preprocess;
    DarkFrame* darkframe;
    FlatField* flatfield;
    RAWCACorr* rawcacorrection;
    RAWExposure* rawexposure;
    BayerRAWExposure* bayerrawexposure;
    XTransRAWExposure* xtransrawexposure;
    FattalToneMapping *fattal;
    MetaDataPanel* metadata;

    std::vector<PParamsChangeListener*> paramcListeners;

    rtengine::StagedImageProcessor* ipc;

    std::vector<ToolPanel*> toolPanels;
    ToolVBox* exposurePanel;
    ToolVBox* detailsPanel;
    ToolVBox* colorPanel;
    ToolVBox* transformPanel;
    ToolVBox* rawPanel;
    ToolVBox* advancedPanel;
    ToolVBox* favoritePanel;
    ToolVBox* trashPanel;
    ToolVBox* usefulPanel;
    Gtk::Notebook* metadataPanel;

    ToolBar* toolBar;

    TextOrIcon* toiE;
    TextOrIcon* toiD;
    TextOrIcon* toiC;
    TextOrIcon* toiT;
    TextOrIcon* toiR;
    TextOrIcon* toiM;
    TextOrIcon* toiW;
    TextOrIcon* toiF;
    TextOrIcon* toiP;
    TextOrIcon* toiU;

    Gtk::Label* labelF;
    Gtk::Label* labelE;
    Gtk::Label* labelD;
    Gtk::Label* labelC;
    Gtk::Label* labelT;
    Gtk::Label* labelR;
    Gtk::Label* labelM;
    Gtk::Label* labelP;
    Gtk::Label* labelU;

    Gtk::Image* imgIconF;
    Gtk::Image* imgIconE;
    Gtk::Image* imgIconD;
    Gtk::Image* imgIconC;
    Gtk::Image* imgIconT;
    Gtk::Image* imgIconR;
    Gtk::Image* imgIconM;
    Gtk::Image* imgIconP;
    Gtk::Image* imgIconU;

    Gtk::Image* imgPanelEnd[NB_PANEL]; 
    Gtk::VBox* vbPanelEnd[NB_PANEL];
    Gtk::HSeparator* hsPanelEnd[NB_PANEL]; 
    Gtk::VBox* vbPanel[NB_PANEL]; // 

    Gtk::ScrolledWindow* favoritePanelSW;
    Gtk::ScrolledWindow* exposurePanelSW;
    Gtk::ScrolledWindow* detailsPanelSW;
    Gtk::ScrolledWindow* colorPanelSW;
    Gtk::ScrolledWindow* transformPanelSW;
    Gtk::ScrolledWindow* rawPanelSW;
    Gtk::ScrolledWindow* advancedPanelSW;
    Gtk::ScrolledWindow* trashPanelSW;
    Gtk::ScrolledWindow* usefulPanelSW;

    std::vector<MyExpander*> expList;

    bool hasChanged;

    void addPanel (Gtk::Box* where, FoldableToolPanel* panel, int level = 1);
    void foldThemAll (GdkEventButton* event);
    void updateVScrollbars (bool hide);
    void handlePanel(Gtk::VBox* vbox, Gtk::ScrolledWindow* panelSW, int panelIterator, int spacing);

private:
    EditDataProvider *editDataProvider;

public:
    CoarsePanel* coarse;
    Gtk::Notebook* toolPanelNotebook;

    ToolPanelCoordinator (bool batch = false, bool benchmark=false);
    virtual ~ToolPanelCoordinator () override;


    bool getChangedState                ()
    {
        return hasChanged;
    }
    Environment* getEnv(){ return env;}
    void updateCurveBackgroundHistogram(
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
    );
    void foldAllButOne (Gtk::Box* parent, FoldableToolPanel* openedSection);

    // multiple listeners can be added that are notified on changes (typical: profile panel and the history)
    void addPParamsChangeListener   (PParamsChangeListener* pp)
    {
        paramcListeners.push_back (pp);
    }
    void setProfilePanel(ProfilePanel* pp) 
    {
      env->setProfilePanel(pp);
    }

    void doDeploy();
    void doDeployLate();
    void doReact(FakeProcEvent ev);

    // toolpanellistener interface
    void panelChanged(const rtengine::ProcEvent& event, const Glib::ustring& descr) override;

    void imageTypeChanged (bool isRaw, bool isBayer, bool isXtrans, bool isMono = false) override;

//    void autoContrastChanged (double autoContrast);
    // profilechangelistener interface
    void profileChange(
        const rtengine::procparams::PartialProfile* nparams,
        const rtengine::ProcEvent& event,
        const Glib::ustring& descr,
        const ParamsEdited* paramsEdited = nullptr,
        bool fromLastSave = false
    ) override;
    void setDefaults(const rtengine::procparams::ProcParams* defparams) override;

    // DirSelectionListener interface
    void dirSelected (const Glib::ustring& dirname, const Glib::ustring& openfile);

    // to support the GUI:
    CropGUIListener* getCropGUIListener (); // through the CropGUIListener the editor area can notify the "crop" ToolPanel when the crop selection changes

    // init the toolpanelcoordinator with an image & close it
    void initImage          (rtengine::StagedImageProcessor* ipc_, bool israw);
    void closeImage         ();

    // update the "expanded" state of the Tools
    void updateToolState    ();
    void openAllTools       ();
    void closeAllTools      ();
    // read/write the "expanded" state of the expanders & read/write the crop panel settings (ratio, guide type, etc.)
    void readOptions        ();
    void writeOptions       ();
    void writeToolExpandedStatus (std::vector<int> &tpOpen);


    // wbprovider interface
    void getAutoWB (double& temp, double& green, double equal, double tempBias) override
    {
        if (ipc) {
            ipc->getAutoWB (temp, green, equal, tempBias);
        }
    }
    void getCamWB (double& temp, double& green) override
    {
        if (ipc) {
            ipc->getCamWB (temp, green);
        }
    }

    //DFProvider interface
    rtengine::RawImage* getDF() override;

    //FFProvider interface
    rtengine::RawImage* getFF() override;
    Glib::ustring GetCurrentImageFilePath() override;

    // rotatelistener interface
    void straightenRequested () override;
    void autoCropRequested () override;
    double autoDistorRequested () override;

    // spotwblistener interface
    void spotWBRequested (int size) override;

    // croppanellistener interface
    void cropSelectRequested () override;

    // icmpanellistener interface
    void saveInputICCReference(const Glib::ustring& fname, bool apply_wb) override;

    // imageareatoollistener interface
    void spotWBselected(int x, int y, Thumbnail* thm = nullptr) override;
    void sharpMaskSelected(bool sharpMask) override;
    int getSpotWBRectSize() const override;
    void cropSelectionReady() override;
    void rotateSelectionReady(double rotate_deg, Thumbnail* thm = nullptr) override;
    ToolBar* getToolBar() const override;
    CropGUIListener* startCropEditing(Thumbnail* thm = nullptr) override;

    void updateTPVScrollbar (bool hide);
    bool handleShortcutKey (GdkEventKey* event);

    // ToolBarListener interface
    void toolSelected (ToolMode tool) override;
    void editModeSwitchedOff () override;

    void setEditProvider (EditDataProvider *provider);
    void on_notebook_switch_page(Gtk::Widget* page, guint page_num);
private:
    IdleRegister idle_register;
};

#endif
