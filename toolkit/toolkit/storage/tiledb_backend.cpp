#include "backends.hpp"

#include<string>
#include<armadillo>

using namespace std;
using namespace arma;

#define DIM_NAME "__hide"
#define ATTR_NAME "sample"
#define ROW_NAME "samples"
#define COL_NAME "channels"
#define RANGE_SIZE 4
#define TILEDB_READ_MODE "r"
#define TILEDB_WRITE_MODE "w"
#define TILEDB_APPEND_MODE "a"

string TileDBBackend::mrn_to_array_name(string mrn)
{
  return _mrn_to_array_name(mrn, "-tiledb");
}

string TileDBBackend::get_array_name(string mrn)
{
  return mrn + "-tiledb";
}


string TileDBBackend::get_workspace()
{
  return DATADIR;
}

string TileDBBackend::get_group()
{
  return "";
}

ArrayMetadata TileDBBackend::get_array_metadata(string mrn)
{
  // TODO(joshblum) store this in TileDB metadata when it's implemented
  int fs = 256;
  int nsamples = 0;
  if (mrn == "007") { // tmp fix for testing
    nsamples = 84992;
  } else if (mrn == "005")
  {
    nsamples = 3686400;
  }
  int nrows = nsamples;
  int ncols = NCHANNELS;
  return ArrayMetadata(fs, nsamples, nrows, ncols);
}

void TileDBBackend::create_array(string mrn, ArrayMetadata* metadata)
{
  string group = get_group();
  string workspace = get_workspace();
  string array_name = get_array_name(mrn);
  // csv of array schema
  string array_schema_str = array_name + ",1," + ATTR_NAME + ",2," + COL_NAME + "," + ROW_NAME + ",0," + to_string(metadata->ncols) + ",0," + to_string(metadata->nrows) + ",float32,int32,*,column-major,*,*,*,*";

  // Initialize TileDB
  TileDB_CTX* tiledb_ctx;
  tiledb_ctx_init(tiledb_ctx);

  // Store the array schema
  tiledb_array_define(tiledb_ctx, workspace.c_str(), group.c_str(), array_schema_str.c_str());
  tiledb_ctx_finalize(tiledb_ctx);
}

void TileDBBackend::open_array(string mrn)
{
  _open_array(mrn, TILEDB_READ_MODE);
}

void TileDBBackend::_open_array(string mrn, const char* mode)
{
  if (in_cache(mrn))
  {
    return;
  }
  TileDB_CTX* tiledb_ctx;
  tiledb_ctx_init(tiledb_ctx);
  string group = get_group();
  string workspace = get_workspace();
  string array_name = get_array_name(mrn);
  int array_id = tiledb_array_open(tiledb_ctx, workspace.c_str(), group.c_str(), array_name.c_str(), mode);
  put_cache(mrn, tiledb_cache_pair(tiledb_ctx, array_id));
}

void TileDBBackend::read_array(string mrn, int ch, int start_offset, int end_offset, frowvec& buf)
{
  double* range = new double[RANGE_SIZE];
  range[0] = CH_REVERSE_IDX[ch];
  range[1] = CH_REVERSE_IDX[ch];
  range[2] = start_offset;
  range[3] = end_offset;
  _read_array(mrn, range, buf);
}

void TileDBBackend::read_array(string mrn, int start_offset, int end_offset, fmat& buf)
{
  double* range = new double[RANGE_SIZE];
  range[0] = 0;
  range[1] = 512; //(TODO)joshblum: remove hardcording when metadata is working //get_ncols(mrn);
  range[2] = start_offset;
  range[3] = end_offset;
  _read_array(mrn, range, buf);
}

void TileDBBackend::_read_array(string mrn, double* range, fmat& buf)
{
  tiledb_cache_pair cache_pair = get_cache(mrn);
  TileDB_CTX* tiledb_ctx = cache_pair.first;
  int array_id = cache_pair.second;
  const char* dim_names = DIM_NAME;
  int dim_names_num = 1;
  const char* attribute_names = ATTR_NAME;
  int attribute_names_num = 1;
  size_t cell_buf_size = sizeof(float) * buf.n_elem;
  tiledb_subarray_buf(tiledb_ctx, array_id, range,
      RANGE_SIZE, &dim_names, dim_names_num,
      &attribute_names, attribute_names_num,
      buf.memptr(), &cell_buf_size);
}

void TileDBBackend::write_array(string mrn, int ch, int start_offset, int end_offset, fmat& buf)
{
  if (in_cache(mrn))
  {
    close_array(mrn); // we need to reopen in append mode
  }
  _open_array(mrn, TILEDB_APPEND_MODE);
  tiledb_cache_pair pair = get_cache(mrn);
  TileDB_CTX* tiledb_ctx = pair.first;
  int array_id = pair.second;
  cell_t cell;
  if (ch == ALL) {
    buf = buf.t();
  }
  for (uword i = 0; i < buf.n_rows; i++)
  {
    for (uword j = 0; j < buf.n_cols; j++)
    {
      // x (col) coord, y (row) coord, attribute
      cell.x = start_offset + j;
      cell.y = ch == ALL ? i : CH_REVERSE_IDX[ch];
      cell.sample = buf(i, j);
      tiledb_cell_write_sorted(tiledb_ctx, array_id, &cell);
    }
  }

  // Finalize TileDB
  close_array(mrn);
}

void TileDBBackend::close_array(string mrn)
{
  if (in_cache(mrn))
  {
    tiledb_cache_pair pair = get_cache(mrn);
    tiledb_array_close(pair.first, pair.second);
    tiledb_ctx_finalize(pair.first);
    pop_cache(mrn);
  }
}

void TileDBBackend::edf_to_array(string mrn)
{
  EDFBackend edf_backend;
  edf_backend.open_array(mrn);

  int fs = edf_backend.get_fs(mrn); // TODO(joshblum) store this in TileDB metadata when it's implemented
  int nsamples = edf_backend.get_nsamples(mrn);
  int nrows = nsamples;
  int ncols = NCHANNELS;
  cout << "Writing " << nsamples << " samples with fs=" << fs << "." << endl;

  ArrayMetadata metadata = ArrayMetadata(fs, nsamples, nrows, ncols);
  create_array(mrn, &metadata);

  cout << "Writing cells to TileDB." << endl;
  int ch, start_offset, end_offset;
  for (int i = 0; i < ncols; i++)
  {
    ch = CHANNEL_ARRAY[i];
    start_offset = 0;
    end_offset = min(nsamples, CHUNK_SIZE);
    frowvec chunk_buf = frowvec(end_offset); // store samples from each channel here

    // read chunks from each signal and write them
    for (; end_offset <= nsamples; end_offset = min(end_offset + CHUNK_SIZE, nsamples))
    {
      if (end_offset - start_offset != CHUNK_SIZE) {
        chunk_buf.resize(end_offset - start_offset);
      }
      edf_backend.read_array(mrn, ch, start_offset, end_offset, chunk_buf);
      write_array(mrn, ch, start_offset, end_offset, chunk_buf);

      start_offset = end_offset;
      // ensure we write the last part of the samples
      if (end_offset == nsamples)
      {
        break;
      }

      if (!(ch % 2 || (end_offset / CHUNK_SIZE) % 10)) // print for even channels every 10 chunks (40MB)
      {
        cout << "Wrote " << end_offset / CHUNK_SIZE << " chunks for ch: " << ch << endl;
      }
    }
  }

  cout << "Conversion complete." << endl;
}
