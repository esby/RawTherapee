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
#pragma once

#include <vector>

#include <gtkmm.h>

#include "bayerpreprocess.h"
#include "bayerprocess.h"
#include "bayerrawexposure.h"
#include "blackwhite.h"
#include "cacorrection.h"
#include "chmixer.h"
#include "tonecurve.h"
#include "coarsepanel.h"
#include "colorappearance.h"
#include "vibrance.h"
#include "colortoning.h"
#include "crop.h"
#include "darkframe.h"
#include "defringe.h"
#include "dehaze.h"
#include "dirpyrdenoise.h"
#include "dirpyrequalizer.h"
#include "distortion.h"
#include "epd.h"
#include "fattaltonemap.h"
#include "filmnegative.h"
#include "filmsimulation.h"
#include "flatfield.h"
#include "gradient.h"
#include "guiutils.h"
#include "hsvequalizer.h"
#include "icmpanel.h"
#include "imageareatoollistener.h"
#include "impulsedenoise.h"
#include "labcurve.h"
#include "lensgeom.h"
#include "lensgeomlistener.h"
#include "lensprofile.h"
#include "localcontrast.h"
#include "locallab.h"
#include "pcvignette.h"
#include "pdsharpening.h"
#include "perspective.h"
#include "pparamschangelistener.h"
#include "preprocess.h"
#include "preprocesswb.h"
#include "profilechangelistener.h"
#include "prsharpening.h"
#include "rawcacorrection.h"
#include "rawexposure.h"
#include "resize.h"
#include "retinex.h"
#include "rgbcurves.h"
#include "rotate.h"
#include "sensorbayer.h"
#include "sensorxtrans.h"
#include "shadowshighlights.h"
#include "sharpenedge.h"
#include "sharpening.h"
#include "sharpenmicro.h"
#include "softlight.h"
#include "spot.h"
#include "tonecurve.h"
#include "toolbar.h"
#include "toolpanel.h"
#include "vibrance.h"
#include "vignetting.h"
#include "wavelet.h"
#include "whitebalance.h"
#include "xtransprocess.h"
#include "xtransrawexposure.h"

#include "../rtengine/noncopyable.h"
#include "../rtengine/rtengine.h"
#include "rtdef.h"
#include "guiutils.h"
#include "ttsaver.h"
#include "ttfavoritecolorer.h"
#include "ttpanelcolorer.h"
#include "ttudlrhider.h"
#include "tttabhider.h"
#include "tttweaker.h"
#include "ttvardisplayer.h"
#include "ttisoprofiler.h"
#include "toolbar.h"


//defining the number of panels for once.
#define NB_PANEL 10
#define NB_PANEL_SWITCHABLE 9
#define PANEL_SWITCHABLE_START 1

class ImageEditorCoordinator;
class MetaDataPanel;

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
    public PerspCorrectionPanelListener,
    public ICMPanelListener,
    public ImageAreaToolListener,
    public rtengine::ImageTypeListener,
    public FilmNegProvider,
    public rtengine::NonCopyable
{
protected:
    Environment* env;
    bool isReaction;
    bool useRtFav;
    int favoriteCount;

    WhiteBalance* whitebalance;
    Vignetting* vignetting;
    Gradient* gradient;
    Locallab* locallab;
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
    Spot* spot;
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
    PreprocessWB* preprocessWB;
    BayerRAWExposure* bayerrawexposure;
    XTransRAWExposure* xtransrawexposure;
    FattalToneMapping *fattal;
    MetaDataPanel* metadata;
    FilmNegative* filmNegative;
    PdSharpening* pdSharpening;
    std::vector<PParamsChangeListener*> paramcListeners;

    rtengine::StagedImageProcessor* ipc;

    std::vector<ToolPanel*> toolPanels;
    std::vector<FoldableToolPanel*> favorites;
    ToolVBox* favoritePanel;
    ToolVBox* exposurePanel;
    ToolVBox* detailsPanel;
    ToolVBox* colorPanel;
    ToolVBox* transformPanel;
    ToolVBox* rawPanel;
    ToolVBox* advancedPanel;
    ToolVBox* locallabPanel;
    ToolVBox* trashPanel;
    ToolVBox* usefulPanel;

    Gtk::Notebook* metadataPanel;

    ToolBar* toolBar;

    TextOrIcon* toiF;
    TextOrIcon* toiE;
    TextOrIcon* toiD;
    TextOrIcon* toiC;
    TextOrIcon* toiT;
    TextOrIcon* toiR;
    TextOrIcon* toiM;
    TextOrIcon* toiW;
    TextOrIcon* toiL;
    TextOrIcon* toiP;
    TextOrIcon* toiU;

    Gtk::Label* labelF;
    Gtk::Label* labelE;
    Gtk::Label* labelD;
    Gtk::Label* labelC;
    Gtk::Label* labelT;
    Gtk::Label* labelR;
    Gtk::Label* labelM;
    Gtk::Label* labelL;
    Gtk::Label* labelP;
    Gtk::Label* labelU;

    Gtk::Image* imgIconF;
    Gtk::Image* imgIconE;
    Gtk::Image* imgIconD;
    Gtk::Image* imgIconC;
    Gtk::Image* imgIconT;
    Gtk::Image* imgIconR;
    Gtk::Image* imgIconM;
    Gtk::Image* imgIconL;
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
    Gtk::ScrolledWindow* locallabPanelSW;
    Gtk::ScrolledWindow* trashPanelSW;
    Gtk::ScrolledWindow* usefulPanelSW;

    std::vector<MyExpander*> expList;

    bool hasChanged;

    void handlePanel(Gtk::VBox* vbox, Gtk::ScrolledWindow* panelSW, int panelIterator, int spacing);
    void addPanel(Gtk::Box* where, FoldableToolPanel* panel, int level = 1);
    void foldThemAll(GdkEventButton* event);
    void updateVScrollbars(bool hide);
    void addfavoritePanel (Gtk::Box* where, FoldableToolPanel* panel, int level = 1);
    void notebookPageChanged(Gtk::Widget* page, guint page_num);

private:
    EditDataProvider *editDataProvider;
    sigc::connection notebookconn;
    bool photoLoadedOnce; // Used to indicated that a photo has been loaded yet
    Gtk::Widget* prevPage;

public:
    CoarsePanel* coarse;
    Gtk::Notebook* toolPanelNotebook;

    ToolPanelCoordinator (bool batch = false, bool benchmark=false);
    virtual ~ToolPanelCoordinator () override;

    bool getChangedState()
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
    void foldAllButOne(Gtk::Box* parent, FoldableToolPanel* openedSection);

    // multiple listeners can be added that are notified on changes (typical: profile panel and the history)
    void addPParamsChangeListener(PParamsChangeListener* pp)
    {
        paramcListeners.push_back(pp);
    }
    void setProfilePanel(ProfilePanel* pp) 
    {
      env->setProfilePanel(pp);
    }

    void doDeploy();
    void doDeployLate();
    void doReact(FakeProcEvent ev);

    // toolpanellistener interface
    void refreshPreview(const rtengine::ProcEvent& event) override;
    void panelChanged(const rtengine::ProcEvent& event, const Glib::ustring& descr) override;
    void setTweakOperator (rtengine::TweakOperator *tOperator) override;
    void unsetTweakOperator (rtengine::TweakOperator *tOperator) override;

    // FilmNegProvider interface
    void imageTypeChanged (bool isRaw, bool isBayer, bool isXtrans, bool isMono = false) override;

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
    void dirSelected(const Glib::ustring& dirname, const Glib::ustring& openfile);

    // to support the GUI:
    CropGUIListener* getCropGUIListener();  // through the CropGUIListener the editor area can notify the "crop" ToolPanel when the crop selection changes

    // init the toolpanelcoordinator with an image & close it
    void initImage(rtengine::StagedImageProcessor* ipc_, bool israw);
    void closeImage();

    // update the "expanded" state of the Tools
    void updateToolState();
    void openAllTools();
    void closeAllTools();
    // read/write the "expanded" state of the expanders & read/write the crop panel settings (ratio, guide type, etc.)
    void readOptions();
    void writeOptions();
    void writeToolExpandedStatus(std::vector<int> &tpOpen);
    void updateShowtooltipVisibility (bool showtooltip);

    // wbprovider interface
    void getAutoWB (double& temp, double& green, double equal, double tempBias) override
    {
        if (ipc) {
            ipc->getAutoWB(temp, green, equal, tempBias);
        }
    }
    void getCamWB (double& temp, double& green) override
    {
        if (ipc) {
            ipc->getCamWB(temp, green);
        }
    }

    //DFProvider interface
    rtengine::RawImage* getDF() override;

    //FFProvider interface
    rtengine::RawImage* getFF() override;
    Glib::ustring GetCurrentImageFilePath() override;

    // FilmNegProvider interface
    bool getFilmNegativeSpot(rtengine::Coord spot, int spotSize, RGB &refInput, RGB &refOutput) override;

    // rotatelistener interface
    void straightenRequested () override;
    void autoCropRequested () override;
    void autoPerspRequested (bool corr_pitch, bool corr_yaw, double& rot, double& pitch, double& yaw, const std::vector<rtengine::ControlLine> *lines = nullptr) override;
    double autoDistorRequested () override;

    // spotwblistener interface
    void spotWBRequested (int size) override;

    // croppanellistener interface
    void cropSelectRequested () override;

    // PerspCorrectionPanelListener interface
    void controlLineEditModeChanged(bool active) override;

    // icmpanellistener interface
    void saveInputICCReference(const Glib::ustring& fname, bool apply_wb) override;

    // imageareatoollistener interface
    void spotWBselected(int x, int y, Thumbnail* thm = nullptr) override;
    void sharpMaskSelected(bool sharpMask) override final;
    int getSpotWBRectSize() const override;
    void cropSelectionReady() override;
    void rotateSelectionReady(double rotate_deg, Thumbnail* thm = nullptr) override;
    ToolBar* getToolBar() const final;
    CropGUIListener* startCropEditing(Thumbnail* thm = nullptr) override;

    void updateTPVScrollbar(bool hide);
    bool handleShortcutKey(GdkEventKey* event);

    // ToolBarListener interface
    void toolDeselected(ToolMode tool) override;
    void toolSelected (ToolMode tool) override;
    void editModeSwitchedOff () final;

    void setEditProvider(EditDataProvider *provider);
    void on_notebook_switch_page(Gtk::Widget* page, guint page_num);

private:
    IdleRegister idle_register;
};
