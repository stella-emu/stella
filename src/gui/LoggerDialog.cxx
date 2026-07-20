//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "bspf.hxx"
#include "Dialog.hxx"
#include "FSNode.hxx"
#include "GuiObject.hxx"
#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "Settings.hxx"
#include "PopUpWidget.hxx"
#include "StringListWidget.hxx"
#include "BrowserDialog.hxx"
#include "StringParser.hxx"
#include "Widget.hxx"
#include "Layout.hxx"
#include "Font.hxx"
#include "Logger.hxx"
#include "LoggerDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
LoggerDialog::LoggerDialog(OSystem& osystem, DialogContainer& parent,
                           const GUI::Font& font, int max_w, int max_h,
                           bool uselargefont)
  : Dialog(osystem, parent, font, "System logs")
{
  WidgetArray wid;

  // This is one dialog that can take as much space as is available
  setSize(4000, 4000, max_w, max_h);

  // Widgets are only created here (at placeholder geometry); layout() assigns
  // all geometry from the current font and dialog size.

  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  // Scrollable listing of the log output
  myLogInfo = new StringListWidget(this, uselargefont ? font :
                  instance().frameBuffer().infoFont(), false);
  myLogInfo->setEditable(false);
  wid.push_back(myLogInfo);

  // Level of logging (how much info to print)
  VariantList items;
  VarList::push_back(items, "None", static_cast<int>(Logger::Level::ERR));
  VarList::push_back(items, "Basic", static_cast<int>(Logger::Level::INFO));
  VarList::push_back(items, "Verbose", static_cast<int>(Logger::Level::DEBUG));
  myLogLevel = new PopUpWidget(this, font, items, "Log level");
  wid.push_back(myLogLevel);

  // Should log output also be shown on the console?
  myLogToConsole = new CheckboxWidget(this, font, "Print to console");
  wid.push_back(myLogToConsole);

  // 'Save log to disk' occupies the left (Defaults) slot of the button group,
  // with OK/Cancel on the right
  addDefaultWidget(new ButtonWidget(this, font,
                   "Save log to disk" + ELLIPSIS, GuiObject::kDefaultsCmd));
  wid.push_back(_defaultWidget);
  addOKCancelBGroup(wid, font);

  addToFocusList(wid);

  setHelpAnchor("Logs");
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LoggerDialog::layout()
{
  using GUI::BoxLayout;
  using GUI::widgetItem;
  using GUI::anchoredItem;
  using Dir = BoxLayout::Dir;

  const int fontWidth    = Dialog::fontWidth(),
            buttonHeight = Dialog::buttonHeight(),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder(),
            VGAP         = Dialog::vGap();

  // The popup draws its own label, so give it a label column of its own
  GUI::alignLabels({{myLogLevel}});

  // Bottom controls: the log-level popup and a checkbox to its right
  auto controlRow = std::make_unique<BoxLayout>(Dir::Horizontal, 0, 0, 0);
  controlRow->addAuto(anchoredItem(myLogLevel));
  controlRow->addSpace(fontWidth * 4);
  controlRow->addAuto(anchoredItem(myLogToConsole));

  // Vertical stack: the log listing fills the available space, then the control
  // row, then a reserved slot for the button group (positioned separately by
  // layoutButtonGroup()).  This dialog takes all the space it is given, so its
  // size is not derived from the content.
  auto root = std::make_unique<BoxLayout>(Dir::Vertical, 0, HBORDER, VBORDER);
  root->addStretch(widgetItem(myLogInfo));
  root->addSpace(VGAP * 2);
  root->addAuto(std::move(controlRow));
  root->addSpace(VGAP * 2);
  root->addSpace(buttonHeight);

  root->doLayout(0, _th, _w, _h - _th);

  // 'Save log to disk' (left) + OK/Cancel (right), along the bottom edge
  layoutButtonGroup();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LoggerDialog::loadConfig()
{
  const StringParser parser(Logger::instance().logMessages());
  myLogInfo->setList(parser.stringList());
  myLogInfo->setSelected(0);
  myLogInfo->scrollToEnd();

  myLogLevel->setSelected(instance().settings().getString("loglevel"),
    static_cast<int>(Logger::Level::INFO));
  myLogToConsole->setState(instance().settings().getBool("logtoconsole"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LoggerDialog::saveConfig()
{
  const int loglevel = myLogLevel->getSelectedTag().toInt();
  const bool logtoconsole = myLogToConsole->getState();

  instance().settings().setValue("loglevel", loglevel);
  instance().settings().setValue("logtoconsole", logtoconsole);

  Logger::instance().setLogParameters(loglevel, logtoconsole);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LoggerDialog::saveLogFile(const FSNode& node)
{
  try
  {
    node.write(Logger::instance().logMessages());
    instance().frameBuffer().showTextMessage("System log saved");
  }
  catch(...)
  {
    instance().frameBuffer().showTextMessage("Error saving system log");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LoggerDialog::handleCommand(CommandSender* sender, int cmd,
                                 int data, int id)
{
  switch(cmd)
  {
    case GuiObject::kOKCmd:
      saveConfig();
      close();
      break;

    case GuiObject::kDefaultsCmd:
      BrowserDialog::show(this, _font, "Save Log as",
                          instance().userDir().getPath() + "stella.log",
                          BrowserDialog::Mode::FileSave,
                          [this](bool OK, const FSNode& node) {
                            if(OK) saveLogFile(node);
                          });
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
