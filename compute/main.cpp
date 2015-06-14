#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "eeg_spectrogram.hpp"
#include "eeg_change_point.hpp"
#define NUM_SAMPLES 10
using namespace arma;

void example_spectrogram(char* filename, float duration,
                         mat& spec_mat, spec_params_t* spec_params)
{
  printf("Using filename: %s, duration: %.2f hours\n", filename, duration);
  unsigned long long start = getticks();
  print_spec_params_t(spec_params);
  eeg_spectrogram_handler(spec_params, LL, spec_mat);
  unsigned long long end = getticks();
  log_time_diff(end - start);
  printf("Spectrogram shape: (%d, %d)\n",
         spec_params->nblocks, spec_params->nfreqs);
  printf("Sample data: [\n[ ");
  for (int i = 0; i < NUM_SAMPLES; i++)
  {
    for (int j = 0; j < NUM_SAMPLES; j++)
    {
      printf("%.5f, ", spec_mat(i, j));
    }
    printf("],\n[ ");
  }
  printf("]\n");
  close_edf(filename);
}

void example_change_points(mat& spec_mat, float duration)
{
  cp_data_t cp_data;
  get_change_points(spec_mat, duration, &cp_data);
  printf("Total change points found: %d\n", cp_data.total_count);
}

int main(int argc, char *argv[])
{
  if (argc <= 3)
  {
    float duration;
    char* filename;
    if (argc >= 2)
    {
      filename = argv[1];
    }
    else
    {
      // default filename
      // filename = "/home/ubuntu/MIT-EDFs/MIT-CSAIL-007.edf";
      filename = "/Users/joshblum/Dropbox (MIT)/MIT-EDFs/MIT-CSAIL-007.edf";

    }
    if (argc == 3)
    {
      duration = atof(argv[2]);
    }
    else
    {
      duration = 4.0; // default duration
    }
    spec_params_t spec_params;
    get_eeg_spectrogram_params(&spec_params, filename, duration);
    mat spec_mat = mat(spec_params.nfreqs, spec_params.nblocks);
    example_spectrogram(filename, duration, spec_mat, &spec_params);
    example_change_points(spec_mat,
                          get_nt(duration, spec_params.fs));
  }
  else
  {
    printf("\nusage: spectrogram <filename> <duration>\n\n");
  }
  return 1;
}
