Cyberstella V1.2

Currently Open Todos:

Later:
- More Columns / Advanced sorting
- Clean OnPlay Routine
- Make whole screen resizable
- Check other versions command line params
- Check other versions keyboard mapping
- Add Screenshots - F12
- Add Pause - F3
- Add Fullscreen / Windowed mode -F4
- Check Direct Draw implementation / upgrade to DX8
- Integrate manuals for the built in games

ToDos from Brad Mott:
- Is the keyboard mapping correct (e.g., do both of the Joysticks
  work via the keyboard)
- Can you have two PC controllers connected?
- Finish the windowed version of StellaX (e.g., so that it'll run in
  a window as well as fullscreen)
- Improve the DirectSound support
- Provide custom keyboard / controller mapping (e.g., be able to
  map joystick buttons to the 2600 Select/Reset as well as the
  other 2600 functions...)

Own Ideas:
->Private Mail to Stephen Anthony

Suggestions from Brian Luttrull:
Are you planning or would you be interested in adding
more features to the front end of Stella?  For
example, the ability to display screenshots, cart
pics, docs, details, etc.  The Kstella frontend has
these features and the very friendly and helpful
author, Stephen Anthony, said you would be free to
take ideas and code from his frontend (under GPL).

Suggestions from Glenn Saunders:
A couple things I'd like to see in a 2600 emulator that Brad was working on 
in ActiveStella are scanline emulation and better controller mapping.

Scanline emulation is pretty easy.  You just use a 256 color palette where 
the scanline emulation lines are half the brightness as the regular palette 
items the 2600 uses.  I think with scanline emulation it looks a lot more 
authentic and less sterile.  That's what they did with Atari Arcade Hits.

Brad should have some of that code already.  Maybe he'll give the code to
you.

As for controller mapping, are you using Direct Input at all?  With Direct 
Input you'd be able to support anything the OS does rather than, let's say, 
directly polling the mouse port or the joystick port and expecting 
something to be there.  A lot of people (myself included) use USB 
peripherals.  Via a USB hub it might be possible to, for instance, connect 
4 trackballs at once to use as paddles for Warlords.  So Direct Input 
automatically polls the system for controllers so you just ask Direct Input 
what's available.  You don't have to hardcode anything in there.  You get 
an inventory of what the system has, and from that you can present various 
mapping choices.

P.S. The coolest part of ActiveStella, of course, was it being an ActiveX 
control.  So the GUI is not built into it.  You could put it on a webpage 
and drive it with Javascript or put it into a Director projector file or 
write a skinnable UI container.  It supported both a windowed and 
fullscreen mode.  Brad never got ActiveStella running stable.  This was the 
time when DirectX was in a lot of transition as well as operating systems 
(98->2000).  I think it's a little easier to write stable code that works 
on XP/2000/Me these days.

He was also talking about a way to hook a scripting language into the 
emulator that you could use almost like a software logic analyzer.  You 
could configure the script to trigger on certain events, like accesses to 
various memory locations.  This would have a lot of uses for debugging, of 
course, but also provide the foundation for high score saving.  If you knew 
where the score was stored in each game, and where the level byte was, and 
you knew the state of the machine when the game ends, then you could 
trigger script that plucks the score out to save it into a global high 
score table.  At the time we were discussing it we were concerned that this 
might slow the emulator down too much but today I think it wouldn't be an 
issue.  I'd really like to see this happen...