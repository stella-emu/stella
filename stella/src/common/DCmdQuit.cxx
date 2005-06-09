
#include "bspf.hxx"
#include "DCmdQuit.hxx"
#include "DebuggerParser.hxx"



DCmdQuit::DCmdQuit(DebuggerParser* p) {
	parser = p;
}

string DCmdQuit::getName() {
	return "Quit";
}

int DCmdQuit::getArgCount() {
	return 0;
}

string DCmdQuit::execute() {
	parser->setDone();
	return "If you quit the debugger, I'll summon Satan all over your hard drive!";
}

