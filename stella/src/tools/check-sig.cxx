#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cstring>

using namespace std;

typedef unsigned char uInt8;
typedef unsigned int uInt32;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int searchForBytes(const uInt8* image, uInt32 imagesize,
                   const uInt8* signature, uInt32 sigsize)
{
  uInt32 count = 0;
  for(uInt32 i = 0; i < imagesize - sigsize; ++i)
  {
    uInt32 matches = 0;
    for(uInt32 j = 0; j < sigsize; ++j)
    {
      if(image[i+j] == signature[j])
        ++matches;
      else
        break;
    }
    if(matches == sigsize)
      ++count;
  }

  return count;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int main(int ac, char* av[])
{
	if(ac != 3)
	{
		cout << "usage: " << av[0] << " <filename> <hex pattern>\n";
		exit(0);
	}
  
    ifstream in(av[1], ios_base::binary);
	in.seekg(0, ios::end);
	int i_size = (int) in.tellg();
	in.seekg(0, ios::beg);

	uInt8* image = new uInt8[i_size];
    in.read((char*)(image), i_size);
    in.close();

	int s_size = 0;
	uInt8* sig = new uInt8[strlen(av[2])/2];
	istringstream buf(av[2]);

	uInt32 c;
	while(buf >> hex >> c)
	{
		sig[s_size++] = (uInt8)c;
//		cerr << "character = " << hex << (int)sig[s_size-1] << endl;
	}
//	cerr << "sig size = " << hex << s_size << endl;

	int result = searchForBytes(image, i_size, sig, s_size);
	if(result > 0)
		cout << setw(3) << result << " hits:  \'" << av[2] << "\' - \"" << av[1] << "\"" << endl;

	delete[] image;
	delete[] sig;

	return 0;
}
