#!/bin/bash
set -x #echo on

# Runner script for ingesting files of different files and sizes for different
# READ_CHUNK_SIZE.

if [ "$#" -ne 1 ]; then
  echo "usage: ingestion_runner [BinaryBackend|HDF5Backend|TileDBBackend]"
  exit
fi

WORK_DIR=".."
cd $WORK_DIR

MRN="005"
RESULTS_FILE="experiments/ingestion_results.txt"
mv $RESULTS_FILE $RESULTS_FILE-bak-$(date +%s)

NUM_RUNS=3
BACKEND=$1
READ_CHUNK_SIZES="1 2 4 8 16 32 64 128 256 512" #MB
FILE_SIZES="1 2 4 8 16 32 64 128" # GB

# setup symlinks for file names
for file_size in $FILE_SIZES; do
  ln -s ~/eeg-data/eeg-data/${MRN}.edf ~/eeg-data/eeg-data/${MRN}-${file_size}gb.edf
done;

for i in $(seq 1 ${NUM_RUNS}); do
  for file_size in $FILE_SIZES; do
    for read_chunk in $READ_CHUNK_SIZES; do
      make clean
      make edf_converter READ_CHUNK=$read_chunk BACKEND=$BACKEND
      # edf_converter <mrn> <desired_size>
      ./edf_converter ${MRN}-${file_size}gb $file_size |& tee -a $RESULTS_FILE
    done;
  done;
done;

