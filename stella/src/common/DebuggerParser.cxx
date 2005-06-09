
#include "bspf.hxx"
#include "DebuggerParser.hxx"
#include "DebuggerCommand.hxx"
#include "DCmdQuit.hxx"

DebuggerParser::DebuggerParser() {
	done = false;
	quitCmd = new DCmdQuit(this);
}

string DebuggerParser::currentAddress() {
	return "currentAddress()";
}

void DebuggerParser::setDone() {
	done = true;
}

string DebuggerParser::run(string command) {
	if(command == "quit") {
		// TODO: use lookup table to determine which DebuggerCommand to run
		return quitCmd->execute();
	} else {
		return "unimplemented command";
	}
}
