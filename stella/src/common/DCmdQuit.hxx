
#ifndef DCMDQUIT_HXX
#define DCMDQUIT_HXX

#include "bspf.hxx"
#include "DebuggerParser.hxx"
#include "DebuggerCommand.hxx"

class DCmdQuit: public DebuggerCommand
{
	public:
		DCmdQuit(DebuggerParser* p);

		string getName();
		int getArgCount();
		string execute();
};

#endif
