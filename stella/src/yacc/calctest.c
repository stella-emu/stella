
#include <stdio.h>
#include <string.h>
#include "YaccParser.hxx"

//extern int YaccParser::yyparse();
//extern void YaccParser::set_input(const char *);
//extern int yyrestart(FILE *);

int main(int argc, char **argv) {

#ifndef BM
	YaccParser::parse(argv[1]);
	printf("\n= %d\n", YaccParser::getResult());
#else
	char buf[10];

	for(int i=0; i<1000000; i++) {
		sprintf(buf, "(1<2)&(3+4)");
		YaccParser::parse(buf);
	}
#endif


}
