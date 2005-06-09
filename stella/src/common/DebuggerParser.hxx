
#ifndef DEBUGGER_PARSER_HXX
#define DEBUGGER_PARSER_HXX

#include "bspf.hxx"
#include "DebuggerCommand.hxx"

class DebuggerParser
{
	public:
		DebuggerParser();
		string currentAddress();
		void setDone();
		string run(string command);

	private:
		DebuggerCommand *quitCmd;
		bool done;
};

#endif
