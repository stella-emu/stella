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
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef NAVIGATION_WIDGET_HXX
#define NAVIGATION_WIDGET_HXX

class EditTextWidget;
class FileListWidget;
class Font;

#include "Widget.hxx"

class NavigationWidget : public Widget
{
  public:
    NavigationWidget(GuiObject* boss, const GUI::Font& font,
      int x, int y, int w, int h);
    ~NavigationWidget() = default;

    void setWidth(int w) override;
    void setList(FileListWidget* list);
    void updateUI();

  private:
    ButtonWidget*     myHomeButton{nullptr};
    ButtonWidget*     myPrevButton{nullptr};
    ButtonWidget*     myNextButton{nullptr};
    ButtonWidget*     myUpButton{nullptr};
    EditTextWidget*   myDir{nullptr};

    FileListWidget*   myList{nullptr};
};

#endif