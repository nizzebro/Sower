#ifndef SOWER_H_INCLUDED
#define SOWER_H_INCLUDED


#define SOWER_VERSION_MAJ 2
#define SOWER_VERSION_MIN 0
#define SOWER_CFG_VERSION 1

#ifdef SOWER_VERSION_FULL
#define SOWER_APP_STRING "Sower 2.0 Full version"
#else
#define SOWER_APP_STRING "Sower 2.0 Free version"
#endif


#include "reaper_plugin.h"
#include "../JuceLibraryCode/JuceHeader.h"


// ======  Common data =========

namespace sower {
  using namespace juce;
//struct Defaults {
//  enum {
//
//    kHeaderWidth = 128,
//    kHeaderWidthMin = 64,
//    kHeaderWidthMax = 300,
//
//    kCPanelHeight = 76,
//    kCPanelHeightMin = 44,
//    kCPanelHeightMax = 268,
//
//    kToolbarWidth = 256,
//    kNumToolButtons = 10,
//    kToolbarHeight = 32,
//    kToolButtonHeight = 24,
//
//    kSelectorWidth = 20,
//    kToolbarMenuWidth = 20,
//
//
//    kRowHeight = 32,
//    kChannelLeftWidth = 12,
//    kColumnWidth = 16,
//
//    kSplitterThickness = 8,
//
//    kParamTopHeight = 6,
//    kParamBottomHeight = 6,
//    kParamBodyHeight = 32,
//
//    kNumRowsSingle = 4,	// num rows in single mode at first time 
//    kTopRowSingleIndex = 60,		// ... and top row index
//
//    kTextColor = 0x00B0C8DE,
//    kFillColor = 0x00322C23,	// control panel fill
//    kResPowerMax = 5,	//1/32 beat: n>>5
//    kResPowerRange = 6	//beat...1/32 beat
//  };
//};


struct MidiProcessorConfig {

  enum {
    kQuantizePosition = 1 << 0,   // quantize note position using 'amtQuantize'
    kQuantizeLength = 1 << 1,   // quantize note length using 'lengthResolution' and 'amtQuantize'
    kQuantizeVelocity = 1 << 2,  // nudge note velocity to 'velocity' using 'amtQuantize'
    kRandomizePosition = 1 << 3,  // randomize note pos using 'amtRandomize'
    kRandomizeLength = 1 << 4,  // randomize note length using 'amtRandomize'
    kRandomizeVelocity = 1 << 5,  // randomize note velocity using  'amtRandomize'
    
    kSwing = 1 << 6,            // quantize with swing  using amtSwing 
    kDeleteExtraNotes = 1 << 7,   // delete non-quantized notes
    kQuantizeExtraNotes = 1 << 8,  // quantize from 'positionResolution' up to config.maxResolution
    kExcludeFirstBeat = 1 << 9,  // don't process first beat 
    kExcludeOnBeats = 1 << 10,    // don't process  on-beats at 'excludeResolution'
    kExcludeOffBeats = 1 << 11,  // don't process  off-beats at 'excludeResolution'
    kProcessSelectedBars = 1 << 12,

    kProcessMask = kQuantizePosition | kQuantizeLength | kQuantizeVelocity |
       kRandomizePosition | kRandomizeLength | kRandomizeVelocity,

    kExcludeMask = kExcludeFirstBeat | kExcludeOnBeats | kExcludeOffBeats

  };

  int flags = 0;
  int quantizePosResolution = 0;
  int quantizeLengthValue = 120;      //  quantize length abs value, ticks 1...qn
  int quantizeVelocityValue = 100;       //  quantize velocity abs value, 0...127
  double quantizePosWindow = 0.5f;  // 0.0...1.0
  double amtQuantize = 1.0f;   // 0.0...1.0
  double amtSwing = 0.25f;	// 0.0...1.0
  double amtRandomize = 0.25f;		// 0.0...1.0
  int excludeResolution = 0;
};


// Extension config; loaded from and saved to cfg file in appdata folder
struct Config {
  enum {
    kAutoDeleteItem = 1,	// delete empty item after delete last event
    kUndoCutPaste = 2,    // add cut/paste notes to undo history
    kUndoInsertDelete = 4,  // add insert/delete note to undo history
    kUndoSetNote = 8,       // add set note action to undo history
    //KAutoResize = 16,     // resize window on bar change
    kShowTooltips = 32,       // show tooltips
    kDisableResize = 64,
    kPreviewHeader = 128,
    kPreviewCell = 256,
    kAnimatePlayback = 512     // hilight beats played
  };
  struct Header {
    char mark[8];
    int version;
  } header = {{'s','o','w','e','r','c','f', 'g'}, SOWER_CFG_VERSION};
    
  int flags = kUndoCutPaste | kUndoInsertDelete | kShowTooltips;
  MidiProcessorConfig midiProcessor;
  int pitch = 72;  
  int midiChn = 0;  
  int maxResolution = 4;
  char theme[1024] = "\0";

};


// helpers
// tok2i:	converts next ASCII token from *src to int;
// sets *src to next digit;  stops at \0  CharPtrTypePtr: pointer to pointer to char
template<typename CharPtrTypePtr>
int tok2i(const CharPtrTypePtr src) noexcept {
  int d = 0; int sign = 0;
  auto c = **src;
  if (c == '-') { --sign; ++(*src); }
  c = **src;
  while ((c >= '0') && (c <= '9')) {
    d = (d * 8) + d + d + c - 0x30;
    ++(*src); c = **src;
  }
  while ((c != 0) && (c != '-')) {
    ++(*src); c = **src;
    if ((c >= '0') && (c <= '9')) break;
  }
  return (d + sign) ^ sign;
}



// u2tok:	// converts unsigned int to ASCII token at <dest> and adds " "; sizeof(<dest>) must be >=11
//			   returns ptr to the next token  CharPtrType: pointer to char
template<typename CharPtrType>
char* u2tok(unsigned int u, CharPtrType dest, char term = ' ') noexcept {
  char buff[12]; 
  char* p = buff + 11;
  for (;; --p) {
    *p = '0' + (u % 10); u /= 10; if (u == 0) break;
  };
  for (;; ++p) { *dest = *p; ++dest;  if (p == (buff + 11)) break; }
  *dest = term;
  return ++dest;
}

int getTrackIndex(MediaTrack* track) noexcept;
String getTrackName(int idx);
String getTrackName(MediaTrack * track);
String getNoteName(int pitch);
String getNoteName(MediaTrack * track, int pitch, int midiChn);
String getTrackAndNoteName(MediaTrack* track, int pitch, int midiChn);
MediaItem_Take* getTake(MediaTrack* track, int idx);

File getPluginDataDir();
  
  

// ProjectSettings settings loaded from and saved to project file
struct Project {
  // flags
  enum Flags {
    kSingleMode = 1,		//  current mode is single-channel; else multi
    kStretchItem = 2,		// when there's no item to add notes to: stretch the prev.item; else insert a new item
    kSyncBar = 4,		// sync with Reaper (formerly kLoopOn but there's no sense to save loop state)
    kDocked = 8,		// window is set to be docked (though may be currently closed)
    kOpened = 16,		// window was opened on exit; used on project load only, in other cases Project::instance is used 
    kCPanelVisible = 32,		// control panel is opened	
    kStayOnTop = 64,		// window is set to stay on top
    kTriplets = 128		// grid mode: triplets
  };

  enum {
    kCPanelPageVelocity = 0, //  project.controlPanelPage values
    kCPanelPageOffset = 1,
    kCPanelPageLength = 2
  };


  struct Channel {
    MediaTrack* track;          // channel track
    int midiChn;                // channel MIDI chn filter
    int pitch;                  // channel pitch filter (multichannel mode only)
    friend  bool operator==(const Channel& lhs, const Channel& rhs) {
      return (lhs.track == rhs.track) && (lhs.midiChn == rhs.midiChn) && (lhs.pitch == rhs.pitch);
    }
    friend  bool operator!=(const Channel& lhs, const Channel& rhs) {
      return ! operator==(lhs, rhs);
    }
  };

  
  Channel  chnSingle;           // singletrack-mode channel
  Array<Channel> chnsMulti;     // multitrack-mode channels

  // ! The order of these following ints is important when loading project 
  
  int iBar = 0; 			          // bar index, zero-based
  int resolution = 1; 					// grid resolution, power of 2 (0: 1/4, 1: 1/8,...)
  int flags = 0;                // enum Flags
  int iLeftColumnVisible = 0;   // the leftmost visible column idx
  int nColumnsVisible = 0;        // number of visible columns 
  int iTopRowVisibleSingle = 63;   // the topmost row visible; single-channel mode
  int iTopRowVisibleMulti = 0;    // the topmost row visible;; multi-channel mode
  int nRowsVisibleSingle = 0;               // number of visible rows
  int nRowsVisibleMulti = 0;            
  int channelListWidth = 200;              // width of the left panel with channel names
  int controlPanelHeight = 200;           // width of the bottom panel with volume sliders
  int windowX = -1;                   // window x pos; -1 : centre; or dock idx if docked
  int windowY = -1;                    // window y pos; 
  // sower ver < 2.0 ends here
  int numBars = 1;
  int controlPanelPage = 0; // note parameter idx, 0: vel, 1: offs, 2: length

  int getTopmostRowVisible() {
    return (flags & kSingleMode) ? iTopRowVisibleSingle : iTopRowVisibleMulti;
  }
  void setTopmostRowVisible(int i) {
    int* row = (flags & kSingleMode) != 0 ? &iTopRowVisibleSingle : &iTopRowVisibleMulti;
    *row = i;
  }
  int getNumsRowsVisible() { return (flags & kSingleMode) ? nRowsVisibleSingle : nRowsVisibleMulti; }
  void setNumsRowVisible(int n) {
    int* nRows = (flags & kSingleMode) != 0 ? &nRowsVisibleSingle : &nRowsVisibleMulti;
    *nRows = n;
  }
};







extern Config config;
extern Project project;
extern void* reaper; // reaper hwnd
extern int reaperVersionMaj;
  

  
inline int countTrailingZeros(int number, int resultIfZero) {
  if (!number) return resultIfZero;
#if defined(__GNUC__) 
  return  __builtin_ctz(static_cast<unsigned int>(number));
#elif defined (_MSC_VER)
  unsigned long n;
  BitScanForward(&n, number);
  return (int)n;
#else
  int n = 0;
  while ((number & 1) == 0) { number >>= 1; ++n; }
  return n;
#endif
}




}; //end namespace sower




#endif //end #ifndef SOWER_H_INCLUDED
