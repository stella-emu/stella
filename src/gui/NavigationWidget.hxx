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

#ifndef NAVIGATION_WIDGET_HXX
#define NAVIGATION_WIDGET_HXX

class FileListWidget;
namespace GUI {
  class Font;
}  // namespace GUI

#include "Widget.hxx"

/**
  The launcher/browser navigation bar: Home/Prev/Next/Up buttons plus
  a clickable path breadcrumb, both driving an attached FileListWidget.

  @author  Stephen Anthony and Thomas Jentzsch
*/
class NavigationWidget : public Widget
{
  public:
    // Sent (via CommandSender) when a breadcrumb segment is clicked
    struct Cmd {
      static constexpr GuiCmd::Code
        FolderClicked = GuiCmd::of("NavigationWidget.FolderClicked");
    };

  private:
    class PathWidget : public Widget
    {
      private:
        // One clickable breadcrumb segment (a single path component)
        class FolderLinkWidget : public ButtonWidget
        {
          public:
            FolderLinkWidget(GuiObject* boss, const GUI::Font& font,
              string_view text, string_view path);
            ~FolderLinkWidget() override = default;

            // The full path this segment navigates to when clicked
            void setPath(string_view path) { myPath = path; }
            const string& getPath() const  { return myPath; }

          protected:
            // Draws the label, framed when hovered
            void drawWidget(bool hilite) override;

          private:
            string myPath;

          private:
            // Following constructors and assignment operators not supported
            FolderLinkWidget() = delete;
            FolderLinkWidget(const FolderLinkWidget&) = delete;
            FolderLinkWidget(FolderLinkWidget&&) = delete;
            FolderLinkWidget& operator=(const FolderLinkWidget&) = delete;
            FolderLinkWidget& operator=(FolderLinkWidget&&) = delete;
        }; // FolderLinkWidget

      public:
        PathWidget(GuiObject* boss, CommandReceiver* target,
          const GUI::Font& font);
        ~PathWidget() override = default;

        // Rebuilds the breadcrumb for 'path', reusing/hiding FolderLinkWidgets
        // as needed; no-ops if 'path' is unchanged (see refresh())
        void setPath(string_view path);
        // The full path of the breadcrumb segment at 'idx'
        const string& getPath(int idx) const;
        // Force the folder-link widths to be recomputed (e.g. after a font
        // change) even though the path itself has not changed
        void refresh();

      private:
        // The path setPath() last built the breadcrumb for
        string myLastPath;
        // Pool of segment widgets, one per path component (grow-only; extras hidden)
        std::vector<FolderLinkWidget*> myFolderList;
        // Receives Cmd::FolderClicked when a segment is clicked
        CommandReceiver* myTarget{nullptr};

      private:
        // Following constructors and assignment operators not supported
        PathWidget() = delete;
        PathWidget(const PathWidget&) = delete;
        PathWidget(PathWidget&&) = delete;
        PathWidget& operator=(const PathWidget&) = delete;
        PathWidget& operator=(PathWidget&&) = delete;
    }; // PathWidget

  public:
    // Builds the Home/Prev/Next/Up buttons and the path breadcrumb
    NavigationWidget(GuiObject* boss, const GUI::Font& font);
    ~NavigationWidget() override = default;

    Common::Size naturalSize() const override;
    void setWidth(int w) override;
    // Repositions/resizes the widget, then re-flows its children (see layoutChildren())
    void setArea(int x, int y, int w, int h) override;
    // The file list the nav buttons/breadcrumb act on
    void setList(FileListWidget* list);
    // Shows/hides (and enables/disables) this widget and all its children
    // together; hiding also clears the breadcrumb
    void setVisible(bool isVisible) override;
    // Refreshes button enabled-state and the breadcrumb from the current list
    void updateUI();

  protected:
    // A breadcrumb segment was clicked; selects that directory in myList
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;

  private:
    // (Re)position and (re)size the child buttons and path field from the
    // current font metrics and this widget's geometry, re-picking the button
    // icon variants; keeps a runtime font change correct, not just resizes
    void layoutChildren();

  private:
    // The navigation buttons, in display order
    ButtonWidget*     myHomeButton{nullptr};
    ButtonWidget*     myPrevButton{nullptr};
    ButtonWidget*     myNextButton{nullptr};
    ButtonWidget*     myUpButton{nullptr};
    // The current-directory breadcrumb
    PathWidget*       myPath{nullptr};
    // The file list this navigation bar controls
    FileListWidget*   myList{nullptr};

  private:
    // Following constructors and assignment operators not supported
    NavigationWidget() = delete;
    NavigationWidget(const NavigationWidget&) = delete;
    NavigationWidget(NavigationWidget&&) = delete;
    NavigationWidget& operator=(const NavigationWidget&) = delete;
    NavigationWidget& operator=(NavigationWidget&&) = delete;
};

#endif  // NAVIGATION_WIDGET_HXX
