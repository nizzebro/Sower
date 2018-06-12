
#define REAPERAPI_IMPLEMENT
// reaper_plugin_functions.h defines to set func import macro

#define REAPERAPI_MINIMAL

#define REAPERAPI_WANT_GetAppVersion 
#define REAPERAPI_WANT_AddExtensionsMainMenu
#define REAPERAPI_WANT_SectionFromUniqueID
#define REAPERAPI_WANT_Main_OnCommand
//#define REAPERAPI_WANT_GetExtState
//#define REAPERAPI_WANT_SetExtState

#define REAPERAPI_WANT_DockWindowAddEx
#define REAPERAPI_WANT_DockWindowRemove
#define REAPERAPI_WANT_DockWindowActivate

#define REAPERAPI_WANT_get_config_var
#define REAPERAPI_WANT_TimeMap_GetMeasureInfo
#define REAPERAPI_WANT_TimeMap2_beatsToTime
#define REAPERAPI_WANT_TimeMap2_timeToBeats
#define REAPERAPI_WANT_TimeMap2_timeToQN
#define REAPERAPI_WANT_TimeMap2_QNToTime

#define REAPERAPI_WANT_CountTracks
#define REAPERAPI_WANT_CountSelectedTracks
#define REAPERAPI_WANT_GetTrack
#define REAPERAPI_WANT_GetTrackInfo
#define REAPERAPI_WANT_TrackFX_Show
#define REAPERAPI_WANT_TrackFX_GetInstrument
#define REAPERAPI_WANT_GetTrackMIDINoteNameEx
#define REAPERAPI_WANT_GetTrackMediaItem
#define REAPERAPI_WANT_GetSetMediaItemInfo
#define REAPERAPI_WANT_GetActiveTake
#define REAPERAPI_WANT_GetMediaItemTake_Item
#define REAPERAPI_WANT_CreateNewMIDIItemInProj
#define REAPERAPI_WANT_MIDI_GetPPQPosFromProjTime
#define REAPERAPI_WANT_MIDI_GetProjTimeFromPPQPos
#define REAPERAPI_WANT_MIDI_GetPPQPosFromProjQN
#define REAPERAPI_WANT_MIDI_InsertNote
#define REAPERAPI_WANT_MIDI_GetNote
#define REAPERAPI_WANT_MIDI_SetNote
#define REAPERAPI_WANT_MIDI_DeleteNote
#define REAPERAPI_WANT_MIDI_GetCC
#define REAPERAPI_WANT_MIDI_InsertCC
#define REAPERAPI_WANT_MIDI_DeleteCC

#define REAPERAPI_WANT_UpdateItemInProject
#define REAPERAPI_WANT_GetSetMediaItemTakeInfo

#define REAPERAPI_WANT_TrackFX_GetParamName
#define REAPERAPI_WANT_SelectAllMediaItems
#define REAPERAPI_WANT_SetMediaItemSelected
#define REAPERAPI_WANT_MIDI_CountEvts
#define REAPERAPI_WANT_CountTakes
#define REAPERAPI_WANT_DeleteTrackMediaItem
#define REAPERAPI_WANT_UpdateArrange
#define REAPERAPI_WANT_Undo_OnStateChange2
#define REAPERAPI_WANT_MarkProjectDirty

#define REAPERAPI_WANT_GetSetRepeatEx
#define REAPERAPI_WANT_GetSet_LoopTimeRange2
#define REAPERAPI_WANT_SetEditCurPos2
#define REAPERAPI_WANT_GetCursorPositionEx
#define REAPERAPI_WANT_GetPlayStateEx
#define REAPERAPI_WANT_GetPlayPositionEx

#define REAPERAPI_WANT_PlayTrackPreview2
#define REAPERAPI_WANT_StopTrackPreview2
#define REAPERAPI_WANT_GetMediaItemTake_Source
#define REAPERAPI_WANT_PCM_Source_CreateFromType
#define REAPERAPI_WANT_MIDI_eventlist_Create
#define REAPERAPI_WANT_MIDI_eventlist_Destroy
#define REAPERAPI_WANT_GetMediaItemTake_Source





#include "reaper_plugin_functions.h"
#include "sower_editor.h"

using namespace sower;

Config sower::config;
Project sower::project;
void* sower::reaper;
int sower::reaperVersionMaj;



int sower::getTrackIndex(MediaTrack* track) noexcept {
  for (int i = 0; MediaTrack* t = GetTrack(0, i); ++i) {
    if (t == track) return i;
  }
  return -1;	// not found
}

String sower::getTrackName(int idx) {
  const char* name = (const char*)GetTrackInfo(idx, 0);
  if (!name) return "<empty>";
  if (!name[0]) return "track " + String(idx + 1);
  return name;
}

String sower::getTrackName(MediaTrack * track) { 
  if (!track) return "<empty>";
  const char* name = (const char*)GetTrackInfo((INT_PTR)track, 0);
  if (!name[0]) return "track_" + String(getTrackIndex(track) + 1);
  return name;
}

String sower::getNoteName(int pitch) {
  int oct = pitch / 12;
  int n = pitch % 12;
  --oct; n *= 2;
  static uint8_t noteChars[] = { 'C','\0', 'C','#', 'D','\0', 'D','#', 'E','\0', 'F','\0',
    'F','#', 'G','\0', 'G','#', 'A','\0', 'A','#', 'B','\0' };
  char c = noteChars[n];
  String s; 
  s += c;
  char alter = noteChars[n + 1];
  if (alter) s += alter;
  return s += oct;
}

String sower::getNoteName(MediaTrack * track, int pitch, int midiChn) {
  if (track) {
    char* name = (char*)GetTrackMIDINoteNameEx(0, track, pitch, midiChn);
    if (name && name[0]) return name;
  }
  return getNoteName(pitch);
}

String sower::getTrackAndNoteName(MediaTrack* track, int pitch, int midiChn) {
  if (!track) return "<empty>";
  return getTrackName(track) + ' ' + getNoteName(track, pitch, midiChn);
}

MediaItem_Take* sower::getTake(MediaTrack* track, int idx) {
  return GetActiveTake(GetTrackMediaItem(track, idx));
}

File sower::getPluginDataDir() {
  return 
#if JUCE_MAC
  File("~/Library/Application Support").
#else
  File::getSpecialLocation(File::userApplicationDataDirectory).
#endif
  getChildFile("reaper_sower");
}


// ================= Repaer callbacks =================


gaccel_register_t g_gaccel_reg = { { 0,0,0 },"Sower" };


bool Editor::hookCommandProc(int command, int flag) {

  if ((command == g_gaccel_reg.accel.cmd) && command) {
    if (!instance) new Editor(); else {
      if (isDocked())DockWindowActivate((HWND)instance->getWindowHandle());
      instance->toFront(true);
    }
    return true;
  }

  return false;
}

void Editor::hookPostCommandProc(int command, int flag) {
  if ((command == 40029) || (command == 40030)) {
    // undo/redo
    if (instance) {
      instance->updateGrid();
    }
  }
}

void Editor::beginLoadProjectState(bool isUndo, struct project_config_extension_t *reg) {
  if (isUndo) return;
  deleteAndZero(instance);
  project = {};     // load defaults
}


bool Editor::processExtensionLine(const char *line, ProjectStateContext *ctx, bool isUndo,
  struct project_config_extension_t *reg) {
  if (isUndo) return true;
  if(strncmp(line, "<SOWER_v", 8)) return false;

  // beginLoadProjectState should delete window.. but check it for any case
  if (instance) {
    delete instance;
    project = {};     
  }

  line += 8;
  int majVersion = tok2i(&line);
  if ((majVersion <= 0) && (majVersion > 2)) return true; // old test version or newer

  if (!(*line)) return true;

  ++line; tok2i(&line); //skip min version

  if (!(*line)) return true;


  project.chnsMulti.resize(tok2i(&line));

  if (!(*line)) return true;

  // get ptr/pitch/midiChn for each multi-mode chn 
  for (auto& p: project.chnsMulti) {
    p.track = GetTrack(0, tok2i(&line)); if (!(*line)) return true;
    p.pitch = tok2i(&line); if (!(*line)) return true; 
    p.midiChn = tok2i(&line); if (!(*line)) return true;
  }
  project.chnSingle.track = GetTrack(0, tok2i(&line));	// get single chn track ptr
  if (!(*line)) return true;
  project.chnSingle.midiChn = tok2i(&line);
  if (!(*line)) return true;

  for (int* i = &project.iBar; i != &project.numBars; ++i) {
    *i = tok2i(&line);	// read iBar and the rest
    if (!(*line)) return true;
  }


  if (majVersion > 1) {
    project.numBars = jlimit(1, 8, tok2i(&line)); if (!(*line)) return true;
    project.controlPanelPage = jlimit(0, 2, tok2i(&line)); 
  }

  if ((project.flags & Project::kOpened) != 0) {
    project.flags &= ~Project::kOpened;
    new Editor();
  }

  return true;
}

void Editor::saveExtensionConfig(ProjectStateContext* ctx, bool isUndo,
  struct project_config_extension_t *reg) {
  #if defined SOWER_VERSION_FULL
  if (isUndo) return;

  char szChns[11 * 128 * 3];
  char* s = szChns;

  // write each multi-mode chn info to char buffer
  for (auto& p : project.chnsMulti) {
    s = u2tok(getTrackIndex(p.track), s);
    s = u2tok(p.pitch, s);
    s = u2tok(p.midiChn, s);
  }
  *s = 0;	// terminate string
  ctx->AddLine("<SOWER_v2.0 %u %s%i %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u>",
    project.chnsMulti.size(),
    szChns, getTrackIndex(project.chnSingle.track),
    (project.chnSingle.track ? project.chnSingle.midiChn : 0), project.iBar,
    project.resolution, project.flags, project.iLeftColumnVisible, project.nColumnsVisible,
    project.iTopRowVisibleSingle, project.iTopRowVisibleMulti, project.nRowsVisibleSingle,
    project.nRowsVisibleMulti, project.channelListWidth, project.controlPanelHeight, project.windowX,
    project.windowY, project.numBars, project.controlPanelPage);
  #endif
}

project_config_extension_t cfgExt =
{ Editor::processExtensionLine, Editor::saveExtensionConfig, Editor::beginLoadProjectState,0 };


void loadConfig() {
  File file = getPluginDataDir().getChildFile("Sower.cfg");
  FileInputStream fs(file);
  if (fs.openedOk()) {
    sower::Config::Header hdr;
    if(fs.read((void*)&hdr, sizeof(sower::Config::Header))
       < sizeof(sower::Config::Header)) return;
    if(strncmp(hdr.mark, sower::config.header.mark, 8))return;
    if(hdr.version != sower::config.header.version) return;
    fs.setPosition(0);
    fs.read((void*)&sower::config, sizeof(sower::Config));
  }
}

void saveConfig() {
  File file = getPluginDataDir().getChildFile("Sower.cfg");
  file.create();
  file.replaceWithData((void*)&sower::config, sizeof(sower::Config));
}

#if JUCE_MAC
  void insertMenuItem(void*, int);
#endif

void menuhook(const char* menuidstr, void* menu, int flag) {
  if (!strcmp(menuidstr, "Main extensions") && !flag) {
    #if JUCE_WINDOWS
      MENUITEMINFO mi;
      mi.cbSize = sizeof(mi);
      mi.fMask = MIIM_TYPE;
      if(GetMenuItemInfo((HMENU)menu, (UINT)-1, TRUE, &mi)) {
        if(mi.fType != MFT_SEPARATOR) {
          mi.fType = MFT_SEPARATOR;
          InsertMenuItem((HMENU)menu, (UINT)-1, TRUE, &mi);
        }
      }
      mi.fMask = MIIM_TYPE | MIIM_ID;
      mi.fType = MFT_STRING;
      mi.dwTypeData = "Sower";
      mi.wID = g_gaccel_reg.accel.cmd;
      InsertMenuItem((HMENU)menu, (UINT)-1, TRUE, &mi);
     
    #elif JUCE_MAC
    insertMenuItem(menu,g_gaccel_reg.accel.cmd);
    #endif
  }
}

#if JUCE_MAC
  extern bool isCommandKeyPressed();
#endif

int Editor::translateAccel(MSG *msg, accelerator_register_t *ctx) {
  if (!Editor::instance) return 0;
  if(msg->hwnd != Editor::instance->getWindowHandle()) return 0;
  bool isKeyDown = (msg->message == WM_KEYDOWN);
  bool bCtrl =
  #if JUCE_WINDOWS
    (GetAsyncKeyState(VK_CONTROL) & 0x8000);
  #elif JUCE_MAC
   isCommandKeyPressed();
  #endif
  int idx;
  switch (msg->wParam) {
    case VK_LEFT:
    case VK_RIGHT:
      if(isKeyDown) {
        if (bCtrl)  Editor::instance->gridHolder.getHorizontalScrollBar().
          moveScrollbarInPages(msg->wParam == VK_LEFT ? -1 : 1);
        else Editor::instance->advanceBar(msg->wParam == VK_LEFT? -1 : 1);
      }
      return 1;
    case VK_UP:
    case VK_DOWN:
      if(isKeyDown) {
        idx = Editor::instance->iRowActive + (msg->wParam == VK_UP ? -1 : 1);
        if ((idx >= 0) && (idx < getNumRowsMax())) {
          Editor::instance->setRowActive(idx);
          Editor::instance->setRowVisible(idx);
          if (config.flags & config.kPreviewHeader) Editor::instance->playPreview(idx);
        }
      }
      return 1;
    case 0x43: if (bCtrl && isKeyDown) Editor::instance->copyRows(); return 1;
    case 0x56: if (bCtrl && isKeyDown) Editor::instance->pasteRows(); return 1;
    case 0x58: if (bCtrl && isKeyDown) Editor::instance->copyRows(true); return 1;
    case VK_ESCAPE: if(isKeyDown && (!isDocked()))
      Editor::instance->userTriedToCloseWindow();
      return 1;
    default:
      return -666;
  }

}

accelerator_register_t g_translateAccel_reg = { &Editor::translateAccel, true };

int Editor::init(void* hInstance, reaper_plugin_info_t *rec) {
  if (rec->caller_version != REAPER_PLUGIN_VERSION || !rec->GetFunc) return false;
  // register Sower command
  int cmd = rec->Register("command_id", (void*)"Sower"); if (!cmd) return false;
  g_gaccel_reg.accel.cmd = (WORD)cmd;

  if (REAPERAPI_LoadAPI(rec->GetFunc) > 0) return false;
  //AttachWindowTopmostButton
  if (!rec->Register("gaccel", &g_gaccel_reg)) return false;
  if (!rec->Register("hookcommand", (void*)&Editor::hookCommandProc)) return false;
  rec->Register("hookpostcommand", (void*)&Editor::hookPostCommandProc);
  rec->Register("projectconfig", &cfgExt);
  rec->Register("accelerator", &g_translateAccel_reg);

  AddExtensionsMainMenu();
  rec->Register("hookcustommenu", (void*)&menuhook);

  initialiseJuce_GUI();

  reaper = rec->hwnd_main;
  const char* pVersion = GetAppVersion();
  reaperVersionMaj = tok2i(&pVersion);
  loadConfig();

  return true;
}

void Editor::destroy() {
  delete Editor::instance;
  saveConfig();
  shutdownJuce_GUI();
}

extern "C" {
  REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(REAPER_PLUGIN_HINSTANCE hInstance, reaper_plugin_info_t *rec) {
    if (rec) return Editor::init(hInstance, rec);
    else Editor::destroy(); return 0;
  };
}


