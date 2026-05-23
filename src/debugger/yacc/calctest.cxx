
#include <cstdio>
#include <cstring>
#include "YaccParser.hxx"

int main(int argc, char** argv)
{
#ifndef BM
  auto expr = YaccParser::parse(argv[1]);
  if(!expr) {
    (void)fprintf(stderr, "parse error: %s\n", YaccParser::errorMessage().c_str());
    return 1;
  }
  printf("\n= %d\n", expr->evaluate());
#else
  auto expr = YaccParser::parse("1+2+3+4+5+6+7");
  for(int i = 0; i < 100000000; i++)
    expr->evaluate();
#endif
}
