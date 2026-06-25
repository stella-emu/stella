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

#ifndef STATE_MANAGER_HXX
#define STATE_MANAGER_HXX

class OSystem;
class RewindManager;
class Event;

#include "Serializer.hxx"

/**
  This class provides an interface to all things related to emulation state.
  States can be loaded or saved here, as well as recorded, rewound, and later
  played back.

  @author  Stephen Anthony
*/
class StateManager
{
  public:
    enum class Mode: uInt8 {
      Off,
      TimeMachine,
      MovieRecord,
      MoviePlayback
    };
    static constexpr string_view STATE_HEADER = "07000003state";
    static constexpr string_view MOVIE_HEADER = "001movie";

    /**
      Create a new statemananger class.
    */
    explicit StateManager(OSystem& osystem);
    ~StateManager();

  public:
    /**
      Answers whether the manager is in record or playback mode.
    */
    Mode mode() const { return myActiveMode; }

    /**
      Toggle movie recording mode.  Recording captures the initial machine
      state plus the per-frame input eventstream to a movie file.

      @return  True if recording is now active
    */
    bool toggleRecordMode();

    /**
      Toggle movie playback mode.  Playback restores the recorded initial
      machine state and then replays the input eventstream frame by frame.

      @return  True if playback is now active
    */
    bool togglePlaybackMode();

    /**
      Append the current (finalized) input window to the movie being recorded.
      Called once per frame while in MovieRecord mode.
    */
    void recordFrame(const Event& event);

    /**
      Load the next input window from the movie being played back into 'event'.
      Called once per frame while in MoviePlayback mode, before the frame is
      emulated.

      @param nowCycles  The current System::cycles() at this poll
      @return  True if a frame was loaded; false if the movie ended (playback
               is stopped) or an error occurred
    */
    bool playbackFrame(Event& event, uInt64 nowCycles);

    /**
      Toggle state rewind recording mode; this uses the RewindManager
      for its functionality.
    */
    void toggleTimeMachine();

    /**
      Sets state rewind recording mode; this uses the RewindManager
      for its functionality.
    */
    void setRewindMode(Mode mode) { myActiveMode = mode; }

    /**
      Optionally adds one extra state when entering the Time Machine dialog;
      this uses the RewindManager for its functionality.
    */
    bool addExtraState(string_view message);

    /**
      Rewinds states; this uses the RewindManager for its functionality.
    */
    bool rewindStates(uInt32 numStates = 1);

    /**
      Unwinds states; this uses the RewindManager for its functionality.
    */
    bool unwindStates(uInt32 numStates = 1);

    /**
      Rewinds/unwinds states; this uses the RewindManager for its functionality.
    */
    bool windStates(uInt32 numStates, bool unwind);

    /**
      Updates the state of the system based on the currently active mode.
    */
    void update();

    /**
      Load a state into the current system.

      @param slot  The state 'slot' to load state from
    */
    void loadState(int slot = -1);

    /**
      Save the current state from the system.

      @param slot  The state 'slot' to save into
    */
    void saveState(int slot = -1);

    /**
      Switches to the next higher or lower state slot (circular queue style).

      @param direction  +1 indicates increase, -1 indicates decrease.
    */
    void changeState(int direction = +1);

    /**
      Toggles auto slot mode.
    */
    void toggleAutoSlot();

    /**
      Load a state into the current system from the given Serializer.
      No messages are printed to the screen.

      @param in  The Serializer object to use

      @return  False on any load errors, else true
    */
    bool loadState(Serializer& in);

    /**
      Save the current state from the system into the given Serializer.
      No messages are printed to the screen.

      @param out  The Serializer object to use

      @return  False on any save errors, else true
    */
    bool saveState(Serializer& out);

    /**
      Resets manager to defaults.
    */
    void reset();

    /**
      Returns the current slot number
    */
    int currentSlot() const { return myCurrentSlot; }

    /**
      The rewind facility for the state manager
    */
    RewindManager& rewindManager() const { return *myRewindManager; }

  private:
    // Full path of the movie file for the currently loaded ROM
    string movieFile() const;

    // The mode to return to when a movie stops (TimeMachine or Off, per setting)
    Mode defaultMode() const;

    // Open the movie file and write its header (md5, controllers, initial state)
    bool startRecording();
    // Write the end-of-movie marker and close the movie file
    void stopRecording();

    // Open the movie file, validate its header and restore the initial state
    bool startPlayback();
    // Close the movie file
    void stopPlayback();

  private:
    // The parent OSystem object
    OSystem& myOSystem;

    // The current slot for load/save states
    int myCurrentSlot{0};

    // Whether the manager is in record or playback mode
    Mode myActiveMode{Mode::Off};

    // MD5 of the currently active ROM (either in movie or rewind mode)
    string myMD5;

    // Movie eventstream file (writer while recording, reader while playing back)
    unique_ptr<Serializer> myMovie;

    // Number of input-window frames recorded/played back so far
    uInt32 myMovieFrames{0};

    // Stored savestates to be later rewound
    unique_ptr<RewindManager> myRewindManager;

  private:
    // Following constructors and assignment operators not supported
    StateManager() = delete;
    StateManager(const StateManager&) = delete;
    StateManager(StateManager&&) = delete;
    StateManager& operator=(const StateManager&) = delete;
    StateManager& operator=(StateManager&&) = delete;
};

#endif  // STATE_MANAGER_HXX
