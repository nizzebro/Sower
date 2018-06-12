#import "Cocoa/Cocoa.h"
#include <objc/runtime.h>
#include <objc/objc.h>
#include "reaper_plugin_functions.h"
#include "sower_editor.h"


void insertMenuItem(void* menu, int cmd) {
 
    if (!menu) return;
    NSMenu* m=(NSMenu *)menu;
    NSMenuItem* item;
    auto pos=[m numberOfItems];
    if(pos) {
      if(![[m itemAtIndex: pos-1] isSeparatorItem]){
        [m addItem:[NSMenuItem separatorItem]];
      }
    }
    NSString* s=(NSString *)CFStringCreateWithCString(0, "Sower", kCFStringEncodingASCII);
    item=[m addItemWithTitle:s action:NULL keyEquivalent:@""];
    
    if (s) [s release];
    if (!pos) [m setAutoenablesItems:NO];
    [item setEnabled:YES];
    [item setRepresentedObject:nil];
    [item setTag: cmd];
   
    NSMenuItem *fi=[m itemAtIndex:0];
    if (fi && fi != item) {
      if ([fi action]) [item setAction:[fi action]]; 
      if ([fi target]) [item setTarget:[fi target]]; 
    }
}



void sower::Editor::toggleAlwaysOnTop() {
  static NSInteger wndLevel;
  project.flags ^= project.kStayOnTop;
  NSWindow* w = [((NSView*)getWindowHandle()) window];
  if(project.flags & project.kStayOnTop) {
    wndLevel = [w level]; [w setLevel: NSFloatingWindowLevel];
  }
  else [w setLevel: wndLevel];
#if defined SOWER_VERSION_FULL
  MarkProjectDirty(0);
#endif
}



bool swellCanPostMessage(id, SEL) {
  return sower::Editor::getInstance() != nullptr;
}


LRESULT onSwellMessage(id, SEL, UINT msg, WPARAM wParam, LPARAM lParam) {
  if ((msg == WM_COMMAND) && (wParam == IDCANCEL)) {
    if(sower::Editor::getInstance() != nullptr) sower::Editor::getInstance()->userTriedToCloseWindow();
  }
  return 0;
};


void sower::Editor::createWindow() {
  if (isDocked()) {
    addToDesktop(0, reaper);
    DockWindowAddEx((HWND)getWindowHandle(), "Sower", "Sower", true);
    DockWindowActivate((HWND)(HWND)getWindowHandle());
    
    getPeer()->setConstrainer(nullptr);
    Class c = object_getClass((NSView*)getWindowHandle());
    if(class_addMethod(c,@selector(swellCanPostMessage),
                       (IMP)(void*)swellCanPostMessage, "c@:")) {
      class_addMethod(c,@selector(onSwellMessage:p1:p2:),
                         (IMP)(void*)onSwellMessage, "@@:@@");
      
    }
    //[((NSView*)reaper) swellAddOwnedWindow: getWindowHandle()];
      
  }
  
  else {
    addToDesktop(ComponentPeer::windowHasTitleBar | ComponentPeer::windowHasCloseButton
                 | ComponentPeer::windowIsResizable, 0);

    NSWindow* w = [((NSView*)getWindowHandle()) window];
    NSWindow* host = [((NSView*)sower::reaper) window];

    NSInteger lvl = (project.flags & project.kStayOnTop)? NSFloatingWindowLevel :[host level] + 1 ;
    [w setLevel: lvl];
    [w setHidesOnDeactivate: YES];

    NSButton *button = [w standardWindowButton:NSWindowZoomButton];
   // [button setHidden:YES];
   // button.alphaValue = 0.0;
    [button setEnabled:NO];
  //  button.image = nil;
  //  button.alternateImage = nil;
    button = [w standardWindowButton:NSWindowMiniaturizeButton];
   // [button setHidden:YES];
   // button.alphaValue = 0.0;
    [button setEnabled:NO];
   // button.image = nil;
   // button.alternateImage = nil;
    if (project.windowX == -1) centreWithSize(getWidth(), getHeight());
    else setTopLeftPosition(project.windowX, project.windowY);
    getPeer()->setConstrainer(this);
  }
}

bool isCommandKeyPressed() {
  return ( NSCommandKeyMask & [NSEvent modifierFlags] );
}


