/**
  Simple program that produces a hex list of a binary object file

  @author  Bradford W. Mott
  @version $Id: romtohex.cxx,v 1.1.1.1 2001-12-27 19:54:32 bwmott Exp $
*/

#include <iomanip.h>
#include <fstream.h>

main()
{
  ifstream in("rom.o");

  for(int t = 0; ; ++t)
  {
    unsigned char c;
    in.get(c);

    if(in.eof())
      break;

    cout << "0x" << hex << (int)c << ", ";

    if((t % 8) == 7)
      cout << endl;
  }
  cout << endl;
} 
