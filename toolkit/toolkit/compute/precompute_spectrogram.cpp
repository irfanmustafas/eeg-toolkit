#include <string>
#include <armadillo>
#include <math.h>

#include "eeg_spectrogram.hpp"

using namespace std;

int main(int argc, char* argv[])
{
  if (argc <= 4)
  {
    string mrn;
    if (argc == 2)
    {
      mrn = argv[1];
    }
    else
    {
      // default medial record number
      mrn = "007";
    }
    cout << "Using mrn: " << mrn << " and backend: " << TOSTRING(BACKEND) << endl;
    precompute_spectrogram(mrn);
    cout << "Completed." << endl;
  }
  else
  {
    cout << "\nusage: ./precompute_spectrogram <mrn>\n" << endl;
  }
  return 1;
}
