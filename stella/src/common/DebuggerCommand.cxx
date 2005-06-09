
#include "bspf.hxx"
#include "DebuggerCommand.hxx"
#include "DebuggerParser.hxx"

DebuggerCommand::DebuggerCommand() {
}

DebuggerCommand::DebuggerCommand(DebuggerParser* p) {
	parser = p;
}
