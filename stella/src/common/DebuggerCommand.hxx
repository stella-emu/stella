
#ifndef DEBUGGER_COMMAND_HXX
#define DEBUGGER_COMMAND_HXX

#include "bspf.hxx"

class DebuggerParser;

class DebuggerCommand
{
	public:
		DebuggerCommand();
		DebuggerCommand(DebuggerParser* p);

		virtual string getName() = 0;
		virtual int getArgCount() = 0;
		virtual string execute() = 0;

	protected:
		DebuggerParser *parser;
};

#endif
