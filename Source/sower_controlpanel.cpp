#include "sower_editor.h"
#include "reaper_plugin_functions.h"
using namespace sower;


sower::Editor::ControlPanelHeader::ControlPanelHeader() noexcept {
  sTop[0] = "velocity 127";
  sMid[0] = "63";
  sBottom[0] = "127";
  sMid[1] = "0";
  sTop[2] = "length 4/4";
  sMid[2] = "1/16";
  sBottom[2] = "0:01";

}



void sower::Editor::ControlPanelHeader::setValueString(const String & s) {
  sVal = s;
  repaint();
}



void sower::Editor::ControlPanelHeader::updateRangeOffset() {
  int ppqHalf = ((int)(Editor::instance->ppqQuant)) / 2;
  sTop[1] = String("offset " + String(ppqHalf - 1));
  sBottom[1] = String(-ppqHalf);
  if (project.controlPanelPage == project.kCPanelPageOffset) {
    repaint();
    Editor::instance->controlPanel.repaint();
  }
}


void Editor::ControlPanelHeader::paint(Graphics& g) {
  g.setFont(Editor::instance->fontSmall);
  g.setColour(Editor::instance->textColor);

  if (sVal.isEmpty()) {
    juce::Rectangle<int> rc(kSmallTextHeight / 2, kSmallTextHeight / 2,
      getWidth() - kSmallTextHeight, getHeight() - kSmallTextHeight);
    int i = project.controlPanelPage;
    g.drawText(sTop[i], rc, Justification::topRight);
    g.drawText(sBottom[i], rc, Justification::bottomRight);
    g.drawText(sMid[i], rc, Justification::centredRight);
  }
  else {
    juce::Rectangle<int> rc(kSmallTextHeight / 2, Editor::instance->controlPanel.lastY - kSmallTextHeight / 2,
      getWidth() - kSmallTextHeight, kSmallTextHeight);
    g.drawText(sVal, rc, Justification::topRight);
  }
}

void sower::Editor::ControlPanelHeader::mouseUp(const MouseEvent& e) {
  if (e.mods.isPopupMenu()) {
    PopupMenu m;
    
    m.addItem(project.kCPanelPageVelocity + 1, "velocity", true,
      project.controlPanelPage == project.kCPanelPageVelocity);
    m.addItem(project.kCPanelPageOffset  + 1, "offset", true,
      project.controlPanelPage == project.kCPanelPageOffset);
    m.addItem(project.kCPanelPageLength + 1, "length", true,
      project.controlPanelPage == project.kCPanelPageLength);
    int res = m.show();
    if (res) {
      if ((--res) != project.controlPanelPage) {
        project.controlPanelPage = res;
        repaint();
        Editor::instance->controlPanel.repaint();
      }
    }
  }
}



Editor::ControlPanel::ControlPanel() noexcept {
}









void Editor::ControlPanel::paint(Graphics& g) {
  Editor& editor = *Editor::instance;
  auto rc = g.getClipBounds();
  int x = rc.getX(); 
  // get start index and rounded-to-cell coords
  int iCol = x / kColumnWidth + project.iLeftColumnVisible;
  x -= x % kColumnWidth;

  int iQuant = editor.columnToQuant(iCol);
  int dQuants = editor.columnToQuant(1);

  int right = jmin(rc.getRight(), editor.gridHolder.getViewPositionX() + editor.gridHolder.getViewWidth());
  
  Quant* pRow = editor.getNumRowsMax() != 0 ? editor.getQuants(editor.iRowActive) : nullptr;

  for (; x < right; x += kColumnWidth, iQuant += dQuants) {

    int isUpBeat;
    if (!useTriplets()) isUpBeat = getMinResolution(iQuant) ? 1 : 0;
    else isUpBeat = (iQuant % (columnToQuant(1, 0) + columnToQuant(1, 0) / 2)) != 0;
    // draw top and bottom images
    g.drawImageAt(editor.images[ImageIndex::paramTop1 + isUpBeat], x, 0);
    g.drawImageAt(editor.images[ImageIndex::paramBottom1 + isUpBeat],
      x, getHeight() - kParamIndent);
    // get body size and base (no-data) image idx; if there's a note we adjust both
    int htOff = getHeight() - kParamIndent * 2;
    if (htOff <= 0) continue;
    int iImgOff = ImageIndex::paramCellOff1;

    if (pRow) {
      Quant& quant = pRow[iQuant];
      if (quant.isSet()) {
        int htOn;
        if (project.controlPanelPage == project.kCPanelPageVelocity) {
          htOn = (int)(htOff * (quant.velocity / 128.0));         
        }
        else if (project.controlPanelPage == project.kCPanelPageOffset) {
          htOn =  (int)(htOff * ((quant.ppqQuantOffset + editor.ppqQuant / 2) / editor.ppqQuant));
        }
        else {
          htOn = (int)(htOff * pow((double)(quant.ppqLength - 1) / (double)(Editor::instance->ppqQN * 4), 1.0/3.0));
        }
        htOn = jlimit(1, htOff, htOn);
        htOff -= htOn;
          g.drawImageWithin(Editor::instance->images[ImageIndex::paramCellVal1 + isUpBeat],
            x, Editor::kParamIndent + htOff, kColumnWidth, htOn,
            RectanglePlacement::stretchToFit);

        iImgOff = ImageIndex::paramCellRem1;

      } // if quant.isSet()
    } // if quants

    g.drawImageWithin(Editor::instance->images[iImgOff + isUpBeat],
      x, Editor::kParamIndent, kColumnWidth, htOff,
      RectanglePlacement::stretchToFit);
  }

}

void sower::Editor::ControlPanel::mouseDown(const MouseEvent & e) {
  if ((!e.mods.isLeftButtonDown()) || (Editor::instance->getNumRowsMax() == 0)) return;
  iLastColumn = -1;
  auto pos = e.getPosition();
  if((pos.getY() <= kParamIndent) || (pos.getY() >= (getHeight() - kParamIndent))) return;
  int iColumn = pos.getX() / kColumnWidth + project.iLeftColumnVisible;
  if(iColumn >= Editor::instance->getNumColumnsMax()) return;
  iLastColumn = iColumn;
  lastY = pos.getY();
  updateLastColumn();
}


void sower::Editor::ControlPanel::mouseDrag(const MouseEvent & e) {
  if ((!e.mods.isLeftButtonDown()) || (Editor::instance->getNumRowsMax() == 0)) return;
  auto pos = e.getPosition();
  int iColumn = pos.getX() / kColumnWidth + project.iLeftColumnVisible;
  if (iColumn >= Editor::instance->getNumColumnsMax()) return;
  int y = jlimit((int)kParamIndent, getHeight() - kParamIndent, pos.getY());
  if ((iLastColumn == iColumn) && (y == lastY)) return;
  iLastColumn = iColumn;
  lastY = y;
  updateLastColumn();
}

void Editor::ControlPanel::mouseUp(const MouseEvent& e) {
  if ((!e.mods.isLeftButtonDown()) || (Editor::instance->getNumRowsMax() == 0)) return;
  Editor::instance->controlPanelHeader.setValueString(String());
}

void sower::Editor::ControlPanel::updateLastColumn() {
  Editor& editor = *Editor::instance;
  int iQuant = editor.columnToQuant(iLastColumn);
  Quant& quant = *editor.getQuants(editor.iRowActive, iQuant);
  if (!quant.isSet()) return;

  int vel = quant.velocity;
  int offs = quant.ppqQuantOffset;
  int len = quant.ppqLength;

  int ht = getHeight() - kParamIndent * 2;
  int y = getHeight() - kParamIndent - lastY;
  double f = (double) y / ht; 
  String s;
  if(project.controlPanelPage == project.kCPanelPageVelocity) {
    vel = (int)jlimit(1, 127, (int)(128.0 * f));
    s = String(vel);
  }
  else if (project.controlPanelPage == project.kCPanelPageOffset) {
    offs = (int)(jlimit(0.0, editor.ppqQuant - 1.0, editor.ppqQuant * f) - editor.ppqQuant / 2);
    s = String(offs);
  }
  else {
    len = 1 + (int)((double)(Editor::instance->ppqQN * 4) * pow(f, 3.0));
    s = String(len);
  }
  Editor::instance->updateNote(editor.iRowActive, iQuant, vel, offs, len);
  Editor::instance->controlPanelHeader.setValueString(s);
  repaint((iLastColumn - project.iLeftColumnVisible) * Editor::kColumnWidth,
    kParamIndent, Editor::kColumnWidth, getHeight() - kParamIndent);
  Editor::instance->controlPanelHeader.repaint();
}

void Editor::ControlPanel::resized() noexcept {
  if(project.controlPanelHeight != getHeight()) {
  project.controlPanelHeight = getHeight();

  #if defined SOWER_VERSION_FULL
    MarkProjectDirty(0);
  #endif
  }
}

