/**
  Simple program that produces a hex list of a binary object file

  @author  Bradford W. Mott
  @version $Id: romtohex.cxx,v 1.2 2002-04-05 02:18:23 bwmott Exp $
*/

#include <iomanip.h>
#include <fstream.h>

main()
{
  ifstream in("scrom.bin");

  cout << "    ";

  for(int t = 0; ; ++t)
  {
    unsigned char c;
    in.get(c);

    if(in.eof())
      break;

    cout << "0x" << hex << (int)c << ", ";

    if((t % 8) == 7)
      cout << endl << "    ";
  }
  cout << endl;
} 
