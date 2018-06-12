#ifndef SOWER_EDITOR_H_INCLUDED
#define SOWER_EDITOR_H_INCLUDED


#include "sower_skin.h"

  


namespace sower {
  using juce::Component;



  class Editor :  public Skin, public Component, public ComponentBoundsConstrainer, public Timer {
    public:
      static int init(void * hInstance, reaper_plugin_info_t * rec);
      static void destroy();
      static Editor* getInstance(){return instance;}
      static bool hookCommandProc(int command, int flag);
      static void hookPostCommandProc(int command, int flag);
      static void beginLoadProjectState(bool isUndo, struct project_config_extension_t *reg);
      static bool processExtensionLine(const char *line, ProjectStateContext *ctx, bool isUndo,
        struct project_config_extension_t *reg);
      static void saveExtensionConfig(ProjectStateContext* ctx, bool isUndo,
        struct project_config_extension_t *reg);  
      virtual void userTriedToCloseWindow() noexcept override;
      Editor();
      virtual ~Editor() override;
      static int translateAccel(MSG *msg, accelerator_register_t *ctx);
    private:

    //  ==================== COMPONENTS ==================== 

    /* ---------------------------------------------  
      |  Header            |  S  |   Toolbar        | 
      |--------------------|  p  |------------------|
      |  Gridheader        |  l  |  Gridholder      | 
      |                    |  i  |  with Grid       |                        
      |--------------------|  t  |------------------| -< Editor height if ControlPanel is not visible
      |           S  p  l  i  t  t  e  r            | 
      |--------------------|  e  |------------------   
      | ControlPanelHeader |  r  | ControlPanel     |   
      ----------------------------------------------  -< Editor height if ControlPanel is visible

  */


      class Splitter : public Component {
        public:
          typedef void (Editor::*Callback)(int pos);
          Splitter(bool vertical, Callback callback) noexcept;
          bool isVertical;
        private:
          int startPos;
          Callback dragCommand;
          void paint(Graphics&) override;
          void mouseDown(const MouseEvent&) override;
          void mouseDrag(const MouseEvent&) override;
      };


    typedef void (Editor::*CommandCallback)();


    class ToolButton :  public Component, public SettableTooltipClient {
    public:
      ToolButton(CommandCallback cmd, Image* img) noexcept;
      bool isMouseDown() noexcept { return isDown; }
      bool isMouseOver() noexcept { return isOver; }
      void setImage(Image* img) { image = img; }
    private:
      void paint(Graphics&) override;
      void mouseEnter(const MouseEvent&) override;
      void mouseExit(const MouseEvent&) override;
      void mouseDown(const MouseEvent&) override;
      void mouseUp(const MouseEvent&) override;
    protected:
      CommandCallback command;
      CommandCallback contextCommand;
      Image* image;
      bool isOver, isDown;
      void updateState(bool, bool);
    };


    class ScrollText : public Component, public SettableTooltipClient {
      public:
        typedef void (Editor::*Callback)(int pos);
        ScrollText(Callback cmdDrag, Callback cmdEndDrag);
        void setText(const String& s) { text = s; repaint(); }
      private:;
        Callback dragCommand;
        Callback endDragCommand;
        String text;
        int lastDelta;
        void paint(Graphics&) override;
        void mouseDrag(const MouseEvent&) override;
        void mouseUp(const MouseEvent&) override;
        void mouseWheelMove(const MouseEvent & event, const MouseWheelDetails & wheel);
    };

    class Header : public Component {
    public:
      Header(Editor& editor) noexcept;
      void mouseWheelMove(const MouseEvent & event, const MouseWheelDetails & wheel) {}
      void setText(const String& s) { text = s; repaint(); }
      ToolButton btnMenu;
    private:
      
      String text;
      void resized()  override; 
      void mouseUp(const MouseEvent&) override;
      void paint(Graphics&) override;
    };

    class Toolbar : public Component {
      public:
        Toolbar(Editor& editor) noexcept;
        void mouseWheelMove(const MouseEvent & event, const MouseWheelDetails & wheel) {}
        int getMinimumWidth() const noexcept;
        ToolButton btnBarPrev;
        ScrollText txtBar;
        ToolButton btnBarNext, btnSyncBar, btnLoop;
        ScrollText txtDenominator;
        ToolButton btnItemStretchMode, btnControlPanel, btnMidiProcessor, btnOptions;
      private:
        void addChild(Component& c, int& pos, int width);
        void paint(Graphics&) override;
        void mouseUp(const MouseEvent&) override;
    };

    class GridHeader : public Component, public SettableTooltipClient {
      public:
      private:
      void paint(Graphics& g) override;
      void mouseDown(const MouseEvent&) override; // selector on/off
      void mouseDrag(const MouseEvent&) override; // selector on/off
      void mouseUp(const MouseEvent&) override;   // change active channel / open channel menu
    };

    class Grid : public Component {
    public:
    private:
      int iLastColumn;
      bool bSet;
      virtual void paint(Graphics& g) override;
      void mouseDown(const MouseEvent&) override; // set/reset cell 
      void updateActiveCell();
      void mouseDrag(const MouseEvent&) override; // set/reset cell 
      void mouseUp(const MouseEvent&) override;   // open note dialog 

    };
    
    class GridHolder : public Viewport {
    private:
      // Updates project row/column info
      virtual void visibleAreaChanged(const juce::Rectangle<int>&) noexcept override;
    };

    
    class ControlPanelHeader: public Component {
      public:
        ControlPanelHeader() noexcept;
        void updateRangeOffset();
        void setValueString(const String& s);

      private:
        String sVal;
        String sTop[3], sMid[3], sBottom[3];
        virtual void paint(Graphics& g) override;
        virtual void mouseUp(const MouseEvent&) override; // menu
    };

    class ControlPanel : public Component {
    public:
      ControlPanel() noexcept;
    private:
      friend class ControlPanelHeader;
      int iLastColumn;
      int lastY;
      virtual void paint(Graphics& g) override;
      virtual void mouseDown(const MouseEvent&) override; 
      virtual void mouseDrag(const MouseEvent&) override; 
      virtual void mouseUp(const MouseEvent&) override;  
      virtual void resized() noexcept override;
      void updateLastColumn();
      
    };

    class Label : public Component {
      public:
        Label() noexcept;
        Label(const String& s);
        void setText(const String& s);
      private:
        String text;
        virtual void paint(Graphics& g) override;
    };

    class ChannelDialog : public Component,  public ComboBox::Listener, public Button::Listener {
    public:
      // chn = nullptr: add channel, else set 
      ChannelDialog(Project::Channel* chn = 0);
      

    private:
      Editor::Label lblTrack, lblMidiChn, lblPitch;
      ComboBox cbTrack, cbPitch, cbMidiChn;
      TextButton btnOk, btnCancel;
      CommandCallback callback;
      Project::Channel* channel;
      virtual void paint(Graphics& g) override;
      virtual void comboBoxChanged(ComboBox* ) override;
      void close(bool ok);
      virtual void buttonClicked(Button*)override;
      virtual void userTriedToCloseWindow() override;
      virtual bool keyPressed(const KeyPress & key) override;
    };

    class OptionsDialog : public Component, public Button::Listener {
    public:

      OptionsDialog();

    private:
      
      Editor::Label lblVersion;
      HyperlinkButton btnWWW;

      Editor::Label lblMaxResolution;
      ComboBox cbMaxResolution;
      Editor::Label lblMidiChn, lblPitch;
      ComboBox cbPitch, cbMidiChn;

      ToggleButton tbAutoDeleteItem;
      Editor::Label lblAllowUndo;
      ToggleButton tbAllowUndoCutPaste, tbAllowUndoInsertDelete, tbAllowUndoSetNote;
      Editor::Label lblPreview;
      ToggleButton tbPreviewHeader, tbPreviewCells;
      ToggleButton tbAnimatePlayback, tbShowTooltips;
   
      Editor::Label lblSkin;
      ComboBox cbSkin;
      TextButton btnApplySkin;
      TextButton btnOk, btnCancel;
      int iSkinApplied;
      virtual void paint(Graphics& g) override;
      void applySkin();
      void close(bool ok);
      virtual void buttonClicked(Button*)override;
      virtual void userTriedToCloseWindow() override;
      virtual bool keyPressed(const KeyPress & key) override;
    };

    class MIDIProcDialog : public Component, public ComboBox::Listener,
      public Button::Listener, public ScrollBar::Listener {
    public:
 
      MIDIProcDialog();
      ~MIDIProcDialog();
    private:  
      ToggleButton tbQuantizePos; ComboBox cbQuantizePosResolution;
      Label lblQuantizePosWindow; ScrollBar slQuantizePosWindow; ToggleButton tbQuantizeDelete;
      ToggleButton tbSwing; ScrollBar slSwingAmount;
      
      ToggleButton tbQuantizeLength; ScrollBar slQuantizeLengthValue; Label lblQuantizeLengthValue;

      ToggleButton tbQuantizeVelocity; ScrollBar slQuantizeVelocityValue; Label lblQuantizeVelocityValue;

      Label lblQuantizeAmount; ScrollBar slQuantizeAmount;

      Label lblRandomize;   
      ToggleButton tbRandomizePos, tbRandomizeLength, tbRandomizeVelocity;
      Label lblRandomizeAmount; ScrollBar slRandomizeAmount;

      Label lblExclude;
      ToggleButton tbExcludeFirstBeat, tbExcludeOnBeats, tbExcludeOffBeats; ComboBox cbExcludeResolution;

      ToggleButton tbSelectBars;

      TextButton btnApply, btnClose;

      virtual void paint(Graphics& g) override;
      virtual void buttonClicked(Button*)override;
      virtual void scrollBarMoved(ScrollBar* bar,
      double newRangeStart) override;
      virtual void comboBoxChanged(ComboBox*) override;
      virtual void userTriedToCloseWindow() override;
      virtual bool keyPressed(const KeyPress & key) override;
     
      void save() noexcept;
      
    };
    
    class NoteDialog : public Component,
      public Button::Listener, public ScrollBar::Listener {
    public:

      NoteDialog(int x, int y, int row, int quant);

    private:
      Label lblVelocity;
      ScrollBar slVelocity;
      Label lblOffset;
      ScrollBar slOffset;
      Label lblLength;
      ScrollBar slLength;
 

      TextButton btnOk, btnCancel;

      int iRow, iQuant;

      virtual void paint(Graphics& g) override;
      virtual void buttonClicked(Button*)override;
      virtual void scrollBarMoved(ScrollBar* bar,
        double newRangeStart) override;
      virtual void userTriedToCloseWindow() override;
      virtual bool keyPressed(const KeyPress & key) override;
      void close(bool);
    };







    // Quant is musical note quantized at max possible resolution.
    // If config.maxResolution == 4  then it's 1/64 note (or 1/96 if triplets are used).
    // Data source used by grid is an array of quants [numRows * numQuants]; where:
    // numRows = number of multi-mode channels or 128 in single-channel mode;
    // numQuants = number of quants in the bar.
    // The array is reloaded whenever channel mode is switched (from multi- to single), triplet mode 
    // is switched, channels are added/removed or config.maxResolution is set in preferences.
    // But when the grid resolution is changed, grid is repainted only but the array stays the same 
    // since is based on config.maxResolution so column-to-quant conversion is only binary shift.
    // Quant is empty if it's take member is nullptr. Quant note has tick offset (ppqQuantOffset member) 
    // in quant boundaries, so the first note may be up-beat up to -1/2 of quant length.

    struct Quant {
      Quant() :take(nullptr){}
      MediaItem_Take* take; 
      uint32_t ppqLength;
      int16_t ppqQuantOffset;   // (int)-ppqQuant/2... (int)ppqQuant/2 - 1.0
      uint8_t velocity;
      bool isSet() const noexcept { return take != nullptr; }
    };

    
    struct RowCopyNote {  
      double ppqStartInBar;
      int ppqLength;
      int velocity;
    };

    //  ==================== STATIC MEMBERS ====================

    static Editor* instance;
    static int noteVelocityUsed;		// last edited note velocity; to use for new notes
    static int ppqNoteLengthUsed;			// last edited note length; to use for new notes
                                    
    static Array<Array<RowCopyNote> > copiedRows;	// copy/paste buffer

    //  ==================== MEMBERS ====================
    

    // state
    static bool isSingleMode() noexcept { return (project.flags & project.kSingleMode) != 0; }
    static bool useStretchItem() noexcept { return (project.flags & project.kStretchItem) != 0; }
    static bool useSyncBar() noexcept{ return (project.flags & project.kSyncBar) != 0; }
    static bool isDocked() noexcept { return (project.flags & project.kDocked) != 0; }
    static bool isControlPanelVisible() noexcept { return (project.flags & project.kCPanelVisible) != 0; }
    static  bool useTriplets() noexcept { return (project.flags & project.kTriplets) != 0; }
    static int getNumBars() noexcept { return project.numBars; }
    static int getControlPanelPage() noexcept { return project.controlPanelPage; }
    static bool isPlaying() noexcept;

  

    Quant* getQuants(int iRow = 0, int iQuant = 0) {
      return quants.getRawDataPointer() + iRow * numQuants + iQuant;
    }

    static int getMinResolution(int iQuant, int maxResolution = config.maxResolution) {
      return maxResolution - countTrailingZeros(iQuant & ((1 << maxResolution) - 1), maxResolution);
    }

    static int columnToQuant(int i, int res = project.resolution) {
      return i << (config.maxResolution - res); 
    }

    static int quantToColumnFloor(int i, int res = project.resolution) {
      return i >> (config.maxResolution - res);
    }
    static int quantToColumnRound(int i, int res = project.resolution) {
      return quantToColumnFloor(i + columnToQuant(1, res) / 2);
    }
    int getNumerator(int res = project.resolution) { return quantToColumnFloor(numQuants, res); }
    int getDenominator(int res = project.resolution) { return quantToColumnFloor(quantDenominator, res); }
    double getColumnTicks(int res = project.resolution) { 
      return ppqQuant * columnToQuant(1, res); 
    }


    int getNumColumnsMax() noexcept { return getNumerator(); }

    static int getNumRowsMax() noexcept { return isSingleMode()? 
      (project.chnSingle.track? 128 :0) : project.chnsMulti.size(); 
    }

    void createWindow();
    void destroyWindow();
    void openUserGuide();

    virtual void paint(Graphics & g) override;

    // ***** focus *****
    void grabFocus(); // calls component::grabFocus or DockActivate
    void focusGained(FocusChangeType) override; // validates tracks and calls updateBar() 
    bool keyPressed(const KeyPress & key) noexcept override;
    void update();

    
    
    // ***** pos/size *****

    enum {
      kColumnWidth = 16,
      kRowHeight = 32,
      kToolbarHeight = 32,
      kToolbuttonWidth = 32,
      kChannelLeftIndent = 12,
      kChannelRightIndent = 20,
      kParamIndent = 6,
      kSplitterThickness = 8,
      kMinimumHeaderWidth = 100,
      kMinimumControlPanelHeight = 100,
      kMinimumGridAreaWidth = 256,
      kScrollbarThickness = 13
    };
    void checkIncrements(juce::Rectangle<int>& bounds);
    virtual void checkBounds(juce::Rectangle<int>& bounds, const juce::Rectangle<int>& oldBounds,
      const juce::Rectangle<int>& limits, bool isStretchingTop, bool isStretchingLeft,
      bool isStretchingBottom, bool isStretchingRight) noexcept override;

    void scrolled(int iCol, int nCols, int iRow, int nRows);  // updates project;  viewport calls it
    void verticalSplitterDrag(int pos);
    void horizontalSplitterDrag(int pos);
    virtual void moved() override;
    void resizeWindow(int nColumns, int nRows);



    virtual void resized() override;                          // sets subcomp sizes

    virtual void mouseWheelMove(const MouseEvent& event,
      const MouseWheelDetails& wheel) override;

    // ***** commands *****

    void openHeaderMenu() { openMainMenu(&header.btnMenu);}
    void openMainMenu(Component* c);
    void openChannelMultiMenu();
    void addChannelMenuItems(PopupMenu& menu) noexcept;
    void processMenuResult(int i);



    void openNoteDialog(int x, int y, int iQuant, int iRow);
    void openAddChannelDialog();
    void openSetChannelDialog();
    void setBar(int i);
    void setBarToPrevious();        // decrements project.iBar and calls barChanged()
    void barScrolled(int delta);
    void advanceBar(int delta);
    void setBarToNext();            // increments project.iBar and calls barChanged()
    
    
    

    void toggleDocked(); // sets flag and reopens window
    void toggleAlwaysOnTop(); // sets flag and calls setAlwaysOnTop if not docked
    void toggleLoopState(); // sets flag and resizes window
    void toggleChannelMode();   // sets flag and resizes window
    void toggleUseTriplets(); // if the numerator is even, sets flag, reloads data and resizes window
    void toggleControlPanelVisible(); // sets flag and resizes window
    void toggleItemStretchMode() ;
    void toggleSyncBar();

    void setTooltips();

    void playPreview(int iRow) noexcept;


    void copyRows(bool bCut = false);
    void cutRows() { copyRows(true); }
    void pasteRows();
    void processRows();
    void processBar();

    void openMidiProcessor();
    void openOptions();
    
    void openActiveChannelFX();
    int selectActiveChannelItems() noexcept; // returns number of items selected
    void openActiveChannelTakeInMIDIEditor() noexcept;
    void openActiveChannelItemProperties() noexcept;

    // notifications

    
    void updateBar();               // sets time-sig related data and calls updateGrid()

    void updateGrid();

    


    void setNumberOfBars(int n);

    // loads beats to grid and calls setResolution()
    void setResolution(int n);      // sets grid scroll position and calls updateSize()
    void resolutionScrolled(int delta);
    void advanceResolution(int delta);



    // channels
    
    void addChannelMulti(MediaTrack* trk, int pitch, int midiChn) noexcept;
    void addChannelsMultiWithSelectedTracks() noexcept;
    void setChannelMulti(Project::Channel& chn, MediaTrack* trk, int pitch, int midiChn) noexcept;
    void setChannelSingle(MediaTrack* trk, int midiChn) noexcept;
    void removeChannelMulti(int idx) noexcept;
    void removeSelectedChannelsMulti() noexcept;
    void removeAllChannelsMulti() noexcept;
    void setRowVisible(int idx);
    void openChannelSingle(int idx);
    void swapChannelsMulti(int idx1, int idx2);
    void setRowActive(int idx) noexcept;
 
    Array<int>& getSelectedRows() {return isSingleMode() ? selectedRowsSingle : selectedRowsMulti; }
    bool isRowSelected(int idx) { return getSelectedRows().contains(idx); }

    virtual void timerCallback() override;

    void updateNote(int iRow, int iQuant, int velocity,
      int ppqOffset, int ppqLength, bool bUndo = true) noexcept;

    void insertNote(int iRow, int iQuant) noexcept;

    void insertNote(int iRow, int iQuant, int velocity, int ppqOffset,
      int ppqLength, bool bUndo = true) noexcept;

    MediaItem_Take * insertNote(double ppqNoteStartInBar, MediaTrack * track,
      int midiChn, int pitch, int velocity,
      int ppqLength, bool bUndo) noexcept;

    void removeNote(int iRow, int iQuant, bool bUndo = true) noexcept;

    bool removeNote(MediaTrack* track, MediaItem_Take * take, double ppqNoteStartInBar,
      int midiChn, int pitch, bool bUndo) noexcept;

   



    //  ======================  MEMBER VARIABLES ========================
    

    int ppqQN;                // ticks per QN, reaper config value 
 
    //  ------------------ bar data ---------------------

    double tmeBarStart;       // current bar start, sec
    double tmeBarEnd;         // current bar end, sec
    double qnBarStart;       // current bar start, qn
    double qnBarEnd;         // current bar end, qn
    int quantDenominator;   // quant note denominator (useTriplets()? 6 :4) << config.maxResolution) 
    double qnQuant;
    double ppqQuant;
    int numQuants;          // bar length, quants
    

    
 

    //  ------------------ grid data ---------------------

    Array<Quant> quants;      // numQuants * getNumRows()
    int minResolution;         // mimimal grid resolution to show all bar notes
    Array<int> numMinResolutionNotes;
    
    int iRowActive = 0;            // highlighted row - it's parameters are shown in control panel; 
    int iColumnAnimated = -1;           // set to -1 in updateBar() and setResolution()  
    bool loopState = false;

    Array<int> selectedRowsSingle;
    Array<int> selectedRowsMulti;
    
   

    preview_register_t preview;
    midi_realtime_write_struct_t midiwrite;


    //  ------------------ sub-components ----------------------

    Header header;
    Toolbar toolbar;
    GridHeader gridHeader;
    GridHolder gridHolder;
    Grid grid;
    ControlPanelHeader controlPanelHeader;
    ControlPanel controlPanel;
    Splitter splitterVert;
    Splitter splitterHorz;
    ScopedPointer<TooltipWindow> tooltip;
    WeakReference<Component> modalComp;

    WeakReference<Component> midiProcessor;
  };


} //end namespace sower
#endif // SOWER_EDITOR_H_INCLUDED