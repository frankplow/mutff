///
/// @file      mutff.c
/// @author    Frank Plowman <post@frankplowman.com>
/// @brief     MuTFF QuickTime file format library main source file
/// @copyright 2022 Frank Plowman
/// @license   This project is released under the GNU Public License Version 3.
///            For the terms of this license, see [LICENSE.md](LICENSE.md)
///

#include "mutff.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MuTFF_FIELD(func, ...)               \
  do {                                       \
    if ((err = func(fd, __VA_ARGS__)) < 0) { \
      return err;                            \
    }                                        \
    ret += err;                              \
  } while (0);

// Convert a number from network (big) endian to host endian.
// These must be implemented here as newlib does not provide the
// standard library implementations ntohs & ntohl (arpa/inet.h).
//
// taken from:
// https://stackoverflow.com/questions/2100331/macro-definition-to-determine-big-endian-or-little-endian-machine
static uint16_t mutff_ntoh_16(uint16_t n) {
  unsigned char *np = (unsigned char *)&n;

  return ((uint16_t)np[0] << 8) | (uint16_t)np[1];
}

static uint16_t mutff_hton_16(uint16_t n) {
  // note this is using implicit truncation
  unsigned char np[2];
  np[0] = n >> 8;
  np[1] = n;

  return *(uint16_t *)np;
}

static mutff_uint24_t mutff_ntoh_24(mutff_uint24_t n) {
  unsigned char *np = (unsigned char *)&n;

  return ((mutff_uint24_t)np[0] << 16) | ((mutff_uint24_t)np[1] << 8) |
         (mutff_uint24_t)np[2];
}

static mutff_uint24_t mutff_hton_24(mutff_uint24_t n) {
  // note this is using implicit truncation
  unsigned char np[4];
  np[0] = n >> 16;
  np[1] = n >> 8;
  np[2] = n;
  np[3] = 0;

  return *(mutff_uint24_t *)np;
}

static uint32_t mutff_ntoh_32(uint32_t n) {
  unsigned char *np = (unsigned char *)&n;

  return ((uint32_t)np[0] << 24) | ((uint32_t)np[1] << 16) |
         ((uint32_t)np[2] << 8) | (uint32_t)np[3];
}

static uint32_t mutff_hton_32(uint32_t n) {
  // note this is using implicit truncation
  unsigned char np[4];
  np[0] = n >> 24;
  np[1] = n >> 16;
  np[2] = n >> 8;
  np[3] = n;

  return *(uint32_t *)np;
}

static uint64_t mutff_ntoh_64(uint64_t n) {
  unsigned char *np = (unsigned char *)&n;

  return ((uint64_t)np[0] << 56) | ((uint64_t)np[1] << 48) |
         ((uint64_t)np[2] << 40) | ((uint64_t)np[3] << 32) |
         ((uint64_t)np[4] << 24) | ((uint64_t)np[5] << 16) |
         ((uint64_t)np[6] << 8) | (uint64_t)np[7];
}

static uint64_t mutff_hton_64(uint64_t n) {
  // note this is using implicit truncation
  unsigned char np[8];
  np[0] = n >> 56;
  np[1] = n >> 48;
  np[2] = n >> 40;
  np[3] = n >> 32;
  np[4] = n >> 24;
  np[5] = n >> 16;
  np[6] = n >> 8;
  np[7] = n;

  return *(uint64_t *)np;
}

static MuTFFError mutff_read_u8(FILE *fd, uint8_t *data) {
  const size_t read = fread(data, 1, 1, fd);
  if (read != 1) {
    if (feof(fd)) {
      return MuTFFErrorEOF;
    } else {
      return MuTFFErrorIOError;
    }
  }
  return 1;
}

static MuTFFError mutff_read_i8(FILE *fd, int8_t *data) {
  int8_t twos;
  const size_t read = fread(&twos, 1, 1, fd);
  if (read != 1) {
    if (feof(fd)) {
      return MuTFFErrorEOF;
    } else {
      return MuTFFErrorIOError;
    }
  }
  // Convert from two's complement to implementation-defined.
  *data = (twos & 0x7F) - (twos & 0x80);
  return 1;
}

static MuTFFError mutff_read_u16(FILE *fd, uint16_t *data) {
  uint16_t network_order;
  const size_t read = fread(&network_order, 2, 1, fd);
  if (read != 1) {
    if (feof(fd)) {
      return MuTFFErrorEOF;
    } else {
      return MuTFFErrorIOError;
    }
  }
  // Convert from network order (big-endian)
  // to host order (implementation-defined).
  *data = mutff_ntoh_16(network_order);
  return 2;
}

static MuTFFError mutff_read_i16(FILE *fd, int16_t *data) {
  int16_t network_order;
  const size_t read = fread(&network_order, 2, 1, fd);
  if (read != 1) {
    if (feof(fd)) {
      return MuTFFErrorEOF;
    }
    return MuTFFErrorIOError;
  }
  const int16_t twos = mutff_ntoh_16(network_order);
  *data = (twos & 0x7FFF) - (twos & 0x8000);
  return 2;
}

static MuTFFError mutff_read_u24(FILE *fd, mutff_uint24_t *data) {
  mutff_uint24_t network_order;
  const size_t read = fread(&network_order, 3, 1, fd);
  if (read != 1) {
    if (feof(fd)) {
      return MuTFFErrorEOF;
    } else {
      return MuTFFErrorIOError;
    }
  }
  *data = mutff_ntoh_24(network_order);
  return 3;
}

static MuTFFError mutff_read_u32(FILE *fd, uint32_t *data) {
  uint32_t network_order;
  const size_t read = fread(&network_order, 4, 1, fd);
  if (read != 1) {
    if (feof(fd)) {
      return MuTFFErrorEOF;
    } else {
      return MuTFFErrorIOError;
    }
  }
  *data = mutff_ntoh_32(network_order);
  return 4;
}

static MuTFFError mutff_read_i32(FILE *fd, int32_t *data) {
  int32_t network_order;
  const size_t read = fread(&network_order, 4, 1, fd);
  if (read != 1) {
    if (feof(fd)) {
      return MuTFFErrorEOF;
    } else {
      return MuTFFErrorIOError;
    }
  }
  const int32_t twos = mutff_ntoh_32(network_order);
  *data = (twos & 0x7FFFFFFF) - (twos & 0x80000000);
  return 4;
}

static MuTFFError mutff_read_u64(FILE *fd, uint64_t *data) {
  uint32_t network_order;
  const size_t read = fread(&network_order, 8, 1, fd);
  if (read != 1) {
    if (feof(fd)) {
      return MuTFFErrorEOF;
    } else {
      return MuTFFErrorIOError;
    }
  }
  *data = mutff_ntoh_64(network_order);
  return 8;
}

static MuTFFError mutff_write_u8(FILE *fd, uint8_t data) {
  const size_t written = fwrite(&data, 1, 1, fd);
  if (written != 1) {
    return MuTFFErrorIOError;
  }
  return 1;
}

static MuTFFError mutff_write_i8(FILE *fd, int8_t data) {
  // ensure number is stored as two's complement
  data = data >= 0 ? data : ~abs(data) + 1;
  const size_t written = fwrite(&data, 1, 1, fd);
  if (written != 1) {
    return MuTFFErrorIOError;
  }
  return 1;
}

static MuTFFError mutff_write_u16(FILE *fd, uint16_t data) {
  // convert number to network order
  data = mutff_hton_16(data);
  const size_t written = fwrite(&data, 2, 1, fd);
  if (written != 1) {
    return MuTFFErrorIOError;
  }
  return 2;
}

static MuTFFError mutff_write_i16(FILE *fd, int16_t data) {
  data = data >= 0 ? data : ~abs(data) + 1;
  data = mutff_hton_16(data);
  const size_t written = fwrite(&data, 2, 1, fd);
  if (written != 1) {
    return MuTFFErrorIOError;
  }
  return 2;
}

static MuTFFError mutff_write_u24(FILE *fd, mutff_uint24_t data) {
  data = mutff_hton_24(data);
  const size_t written = fwrite(&data, 3, 1, fd);
  if (written != 1) {
    return MuTFFErrorIOError;
  }
  return 3;
}

static MuTFFError mutff_write_u32(FILE *fd, uint32_t data) {
  data = mutff_hton_32(data);
  const size_t written = fwrite(&data, 4, 1, fd);
  if (written != 1) {
    return MuTFFErrorIOError;
  }
  return 4;
}

static MuTFFError mutff_write_i32(FILE *fd, int32_t data) {
  data = data >= 0 ? data : ~abs(data) + 1;
  data = mutff_hton_32(data);
  const size_t written = fwrite(&data, 4, 1, fd);
  if (written != 1) {
    return MuTFFErrorIOError;
  }
  return 4;
}

static MuTFFError mutff_write_u64(FILE *fd, uint32_t data) {
  data = mutff_hton_64(data);
  const size_t written = fwrite(&data, 8, 1, fd);
  if (written != 1) {
    return MuTFFErrorIOError;
  }
  return 8;
}

static MuTFFError mutff_read_q8_8(FILE *fd, mutff_q8_8_t *data) {
  MuTFFError ret = 0;
  if ((ret = mutff_read_i8(fd, &data->integral)) < 0) {
    return ret;
  }
  if ((ret = mutff_read_u8(fd, &data->fractional)) < 0) {
    return ret;
  }
  return 2;
}

static MuTFFError mutff_write_q8_8(FILE *fd, mutff_q8_8_t data) {
  MuTFFError ret = 0;
  if ((ret = mutff_write_i8(fd, data.integral)) < 0) {
    return ret;
  }
  if ((ret = mutff_write_u8(fd, data.fractional)) < 0) {
    return ret;
  }
  return 2;
}

static MuTFFError mutff_read_q16_16(FILE *fd, mutff_q16_16_t *data) {
  MuTFFError ret = 0;
  if ((ret = mutff_read_i16(fd, &data->integral)) < 0) {
    return ret;
  }
  if ((ret = mutff_read_u16(fd, &data->fractional)) < 0) {
    return ret;
  }
  return 4;
}

static MuTFFError mutff_write_q16_16(FILE *fd, mutff_q16_16_t data) {
  MuTFFError ret = 0;
  if ((ret = mutff_write_i16(fd, data.integral)) < 0) {
    return ret;
  }
  if ((ret = mutff_write_u16(fd, data.fractional) < 0)) {
    return ret;
  }
  return 4;
}

// return the size of an atom including its header, given the size of the data
// in it
static inline uint64_t mutff_atom_size(uint64_t data_size) {
  return data_size + 8 <= UINT32_MAX ? data_size + 8 : data_size + 16;
}

// return the size of the data in an atom, given the size of the entire atom
// including its header in it
static inline uint64_t mutff_data_size(uint64_t atom_size) {
  return atom_size <= UINT32_MAX ? atom_size - 8 : atom_size - 16;
}

static MuTFFError mutff_read_header(FILE *fd, uint64_t *size, uint32_t *type) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint32_t short_size;

  MuTFF_FIELD(mutff_read_u32, &short_size);
  MuTFF_FIELD(mutff_read_u32, type);
  if (short_size == 1) {
    MuTFF_FIELD(mutff_read_u64, size);
  } else {
    *size = short_size;
  }

  return ret;
}

static MuTFFError mutff_write_header(FILE *fd, uint64_t size, uint32_t type) {
  MuTFFError err;
  MuTFFError ret = 0;

  if (size > UINT32_MAX) {
    MuTFF_FIELD(mutff_write_u32, 1);
    MuTFF_FIELD(mutff_write_u32, type);
    MuTFF_FIELD(mutff_write_u64, size);
  } else {
    MuTFF_FIELD(mutff_write_u32, size);
    MuTFF_FIELD(mutff_write_u32, type);
  }

  return ret;
}

MuTFFError mutff_peek_atom_header(FILE *fd, MuTFFAtomHeader *out) {
  MuTFFError err;

  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  fseek(fd, -8, SEEK_CUR);

  return 0;
}

MuTFFError mutff_read_quickdraw_rect(FILE *fd, MuTFFQuickDrawRect *out) {
  MuTFFError err;
  MuTFFError ret = 0;

  MuTFF_FIELD(mutff_read_u16, &out->top);
  MuTFF_FIELD(mutff_read_u16, &out->left);
  MuTFF_FIELD(mutff_read_u16, &out->bottom);
  MuTFF_FIELD(mutff_read_u16, &out->right);

  return ret;
}

MuTFFError mutff_write_quickdraw_rect(FILE *fd, const MuTFFQuickDrawRect *in) {
  MuTFFError err;
  MuTFFError ret = 0;

  MuTFF_FIELD(mutff_write_u16, in->top);
  MuTFF_FIELD(mutff_write_u16, in->left);
  MuTFF_FIELD(mutff_write_u16, in->bottom);
  MuTFF_FIELD(mutff_write_u16, in->right);

  return ret;
}

MuTFFError mutff_read_quickdraw_region(FILE *fd, MuTFFQuickDrawRegion *out) {
  MuTFFError err;
  MuTFFError ret = 0;

  MuTFF_FIELD(mutff_read_u16, &out->size);
  MuTFF_FIELD(mutff_read_quickdraw_rect, &out->rect);
  const uint16_t data_size = out->size - ret;
  for (uint16_t i = 0; i < data_size; ++i) {
    MuTFF_FIELD(mutff_read_u8, (uint8_t *)&out->data[i]);
  }

  return ret;
}

MuTFFError mutff_write_quickdraw_region(FILE *fd,
                                        const MuTFFQuickDrawRegion *in) {
  MuTFFError err;
  MuTFFError ret = 0;

  MuTFF_FIELD(mutff_write_u16, in->size);
  MuTFF_FIELD(mutff_write_quickdraw_rect, &in->rect);
  for (uint16_t i = 0; i < in->size - 10; ++i) {
    MuTFF_FIELD(mutff_write_u8, in->data[i]);
  }

  return ret;
}

MuTFFError mutff_read_file_type_compatibility_atom(
    FILE *fd, MuTFFFileTypeCompatibilityAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;

  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("ftyp")) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u32, &out->major_brand);
  MuTFF_FIELD(mutff_read_u32, &out->minor_version);

  // read variable-length data
  out->compatible_brands_count = (size - ret) / 4;
  if (out->compatible_brands_count > MuTFF_MAX_COMPATIBLE_BRANDS) {
    return MuTFFErrorOutOfMemory;
  }
  for (size_t i = 0; i < out->compatible_brands_count; ++i) {
    MuTFF_FIELD(mutff_read_u32, &out->compatible_brands[i]);
  }
  if (!fseek(fd, size - ret, SEEK_CUR)) {
    ret += size - ret;
  }

  return ret;
}

static inline uint64_t mutff_file_type_compatibility_atom_size(
    const MuTFFFileTypeCompatibilityAtom *atom) {
  return mutff_atom_size(8 + 4 * atom->compatible_brands_count);
}

MuTFFError mutff_write_file_type_compatibility_atom(
    FILE *fd, const MuTFFFileTypeCompatibilityAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_file_type_compatibility_atom_size(in);

  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("ftyp"));
  MuTFF_FIELD(mutff_write_u32, in->major_brand);
  MuTFF_FIELD(mutff_write_u32, in->minor_version);
  for (size_t i = 0; i < in->compatible_brands_count; ++i) {
    MuTFF_FIELD(mutff_write_u32, in->compatible_brands[i]);
  }

  return ret;
}

MuTFFError mutff_read_movie_data_atom(FILE *fd, MuTFFMovieDataAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;

  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("mdat")) {
    return MuTFFErrorBadFormat;
  }
  out->data_size = mutff_data_size(size);
  if (!fseek(fd, size - ret, SEEK_CUR)) {
    ret += size - ret;
  }
  return ret;
}

static inline uint64_t mutff_movie_data_atom_size(
    const MuTFFMovieDataAtom *atom) {
  return mutff_atom_size(atom->data_size);
}

MuTFFError mutff_write_movie_data_atom(FILE *fd, const MuTFFMovieDataAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_movie_data_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("mdat"));
  for (uint64_t i = 0; i < in->data_size; ++i) {
    MuTFF_FIELD(mutff_write_u8, 0);
  }
  return ret;
}

MuTFFError mutff_read_free_atom(FILE *fd, MuTFFFreeAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("free")) {
    return MuTFFErrorBadFormat;
  }
  out->data_size = mutff_data_size(size);
  if (!fseek(fd, size - ret, SEEK_CUR)) {
    ret += size - ret;
  }
  return ret;
}

static inline uint64_t mutff_free_atom_size(const MuTFFFreeAtom *atom) {
  return mutff_atom_size(atom->data_size);
}

MuTFFError mutff_write_free_atom(FILE *fd, const MuTFFFreeAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_free_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("free"));
  for (uint64_t i = 0; i < in->data_size; ++i) {
    MuTFF_FIELD(mutff_write_u8, 0);
  }
  return ret;
}

MuTFFError mutff_read_skip_atom(FILE *fd, MuTFFSkipAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("skip")) {
    return MuTFFErrorBadFormat;
  }
  out->data_size = mutff_data_size(size);
  if (!fseek(fd, size - ret, SEEK_CUR)) {
    ret += size - ret;
  }
  return ret;
}

static inline uint64_t mutff_skip_atom_size(const MuTFFSkipAtom *atom) {
  return mutff_atom_size(atom->data_size);
}

MuTFFError mutff_write_skip_atom(FILE *fd, const MuTFFSkipAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_skip_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("skip"));
  for (uint64_t i = 0; i < in->data_size; ++i) {
    MuTFF_FIELD(mutff_write_u8, 0);
  }
  return ret;
}

MuTFFError mutff_read_wide_atom(FILE *fd, MuTFFWideAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("wide")) {
    return MuTFFErrorBadFormat;
  }
  out->data_size = mutff_data_size(size);
  if (!fseek(fd, size - ret, SEEK_CUR)) {
    ret += size - ret;
  }
  return ret;
}

static inline uint64_t mutff_wide_atom_size(const MuTFFWideAtom *atom) {
  return mutff_atom_size(atom->data_size);
}

MuTFFError mutff_write_wide_atom(FILE *fd, const MuTFFWideAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_wide_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("wide"));
  for (uint64_t i = 0; i < in->data_size; ++i) {
    MuTFF_FIELD(mutff_write_u8, 0);
  }
  return ret;
}

MuTFFError mutff_read_preview_atom(FILE *fd, MuTFFPreviewAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("pnot")) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u32, &out->modification_time);
  MuTFF_FIELD(mutff_read_u16, &out->version);
  MuTFF_FIELD(mutff_read_u32, &out->atom_type);
  MuTFF_FIELD(mutff_read_u16, &out->atom_index);
  if (!fseek(fd, size - ret, SEEK_CUR)) {
    ret += size - ret;
  }
  return ret;
}

static inline uint64_t mutff_preview_atom_size(const MuTFFPreviewAtom *atom) {
  return mutff_atom_size(12);
}

MuTFFError mutff_write_preview_atom(FILE *fd, const MuTFFPreviewAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_preview_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("pnot"));
  MuTFF_FIELD(mutff_write_u32, in->modification_time);
  MuTFF_FIELD(mutff_write_u16, in->version);
  MuTFF_FIELD(mutff_write_u32, in->atom_type);
  MuTFF_FIELD(mutff_write_u16, in->atom_index);
  return ret;
}

MuTFFError mutff_read_movie_header_atom(FILE *fd, MuTFFMovieHeaderAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("mvhd")) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_u32, &out->creation_time);
  MuTFF_FIELD(mutff_read_u32, &out->modification_time);
  MuTFF_FIELD(mutff_read_u32, &out->time_scale);
  MuTFF_FIELD(mutff_read_u32, &out->duration);
  MuTFF_FIELD(mutff_read_q16_16, &out->preferred_rate);
  MuTFF_FIELD(mutff_read_q8_8, &out->preferred_volume);
  fseek(fd, 10, SEEK_CUR);
  ret += 10;
  for (size_t j = 0; j < 3; ++j) {
    for (size_t i = 0; i < 3; ++i) {
      MuTFF_FIELD(mutff_read_u32, &out->matrix_structure[j][i]);
    }
  }
  MuTFF_FIELD(mutff_read_u32, &out->preview_time);
  MuTFF_FIELD(mutff_read_u32, &out->preview_duration);
  MuTFF_FIELD(mutff_read_u32, &out->poster_time);
  MuTFF_FIELD(mutff_read_u32, &out->selection_time);
  MuTFF_FIELD(mutff_read_u32, &out->selection_duration);
  MuTFF_FIELD(mutff_read_u32, &out->current_time);
  MuTFF_FIELD(mutff_read_u32, &out->next_track_id);
  if (!fseek(fd, size - ret, SEEK_CUR)) {
    ret += size - ret;
  }
  return ret;
}

static inline uint64_t mutff_movie_header_atom_size(
    const MuTFFMovieHeaderAtom *atom) {
  return mutff_atom_size(100);
}

MuTFFError mutff_write_movie_header_atom(FILE *fd,
                                         const MuTFFMovieHeaderAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_movie_header_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("mvhd"));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_u32, in->creation_time);
  MuTFF_FIELD(mutff_write_u32, in->modification_time);
  MuTFF_FIELD(mutff_write_u32, in->time_scale);
  MuTFF_FIELD(mutff_write_u32, in->duration);
  MuTFF_FIELD(mutff_write_q16_16, in->preferred_rate);
  MuTFF_FIELD(mutff_write_q8_8, in->preferred_volume);
  for (size_t i = 0; i < 10; ++i) {
    MuTFF_FIELD(mutff_write_u8, 0);
  }
  for (size_t j = 0; j < 3; ++j) {
    for (size_t i = 0; i < 3; ++i) {
      MuTFF_FIELD(mutff_write_u32, in->matrix_structure[j][i]);
    }
  }
  MuTFF_FIELD(mutff_write_u32, in->preview_time);
  MuTFF_FIELD(mutff_write_u32, in->preview_duration);
  MuTFF_FIELD(mutff_write_u32, in->poster_time);
  MuTFF_FIELD(mutff_write_u32, in->selection_time);
  MuTFF_FIELD(mutff_write_u32, in->selection_duration);
  MuTFF_FIELD(mutff_write_u32, in->current_time);
  MuTFF_FIELD(mutff_write_u32, in->next_track_id);
  return ret;
}

MuTFFError mutff_read_clipping_region_atom(FILE *fd,
                                           MuTFFClippingRegionAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("crgn")) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_quickdraw_region, &out->region);
  if (!fseek(fd, size - ret, SEEK_CUR)) {
    ret += size - ret;
  }
  return ret;
}

static inline uint64_t mutff_clipping_region_atom_size(
    const MuTFFClippingRegionAtom *atom) {
  return mutff_atom_size(atom->region.size);
}

MuTFFError mutff_write_clipping_region_atom(FILE *fd,
                                            const MuTFFClippingRegionAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_clipping_region_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("crgn"));
  MuTFF_FIELD(mutff_write_quickdraw_region, &in->region);
  return ret;
}

MuTFFError mutff_read_clipping_atom(FILE *fd, MuTFFClippingAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("clip")) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_clipping_region_atom, &out->clipping_region);
  if (!fseek(fd, size - ret, SEEK_CUR)) {
    ret += size - ret;
  }
  return ret;
}

static inline uint64_t mutff_clipping_atom_size(const MuTFFClippingAtom *atom) {
  return mutff_atom_size(
      mutff_clipping_region_atom_size(&atom->clipping_region));
}

MuTFFError mutff_write_clipping_atom(FILE *fd, const MuTFFClippingAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_clipping_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("clip"));
  MuTFF_FIELD(mutff_write_clipping_region_atom, &in->clipping_region);
  return ret;
}

MuTFFError mutff_read_color_table_atom(FILE *fd, MuTFFColorTableAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("ctab")) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u32, &out->color_table_seed);
  MuTFF_FIELD(mutff_read_u16, &out->color_table_flags);
  MuTFF_FIELD(mutff_read_u16, &out->color_table_size);

  // read color array
  const size_t array_size = (out->color_table_size + 1) * 8;
  if (array_size != mutff_data_size(size) - 8) {
    return MuTFFErrorBadFormat;
  }
  for (size_t i = 0; i <= out->color_table_size; ++i) {
    for (size_t j = 0; j < 4; ++j) {
      MuTFF_FIELD(mutff_read_u16, &out->color_array[i][j]);
    }
  }
  if (!fseek(fd, size - ret, SEEK_CUR)) {
    ret += size - ret;
  }

  return ret;
}

static inline uint64_t mutff_color_table_atom_size(
    const MuTFFColorTableAtom *atom) {
  return mutff_atom_size(8 + (atom->color_table_size + 1) * 8);
}

MuTFFError mutff_write_color_table_atom(FILE *fd,
                                        const MuTFFColorTableAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_color_table_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("ctab"));
  MuTFF_FIELD(mutff_write_u32, in->color_table_seed);
  MuTFF_FIELD(mutff_write_u16, in->color_table_flags);
  MuTFF_FIELD(mutff_write_u16, in->color_table_size);
  for (uint16_t i = 0; i <= in->color_table_size; ++i) {
    MuTFF_FIELD(mutff_write_u16, in->color_array[i][0]);
    MuTFF_FIELD(mutff_write_u16, in->color_array[i][1]);
    MuTFF_FIELD(mutff_write_u16, in->color_array[i][2]);
    MuTFF_FIELD(mutff_write_u16, in->color_array[i][3]);
  }
  return ret;
}

MuTFFError mutff_read_user_data_list_entry(FILE *fd,
                                           MuTFFUserDataListEntry *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  MuTFF_FIELD(mutff_read_header, &size, &out->type);

  // read variable-length data
  out->data_size = mutff_data_size(size);
  if (out->data_size > MuTFF_MAX_USER_DATA_ENTRY_SIZE) {
    return MuTFFErrorOutOfMemory;
  }
  for (uint32_t i = 0; i < out->data_size; ++i) {
    MuTFF_FIELD(mutff_read_u8, (uint8_t *)&out->data[i]);
  }

  return ret;
}

static inline uint64_t mutff_user_data_list_entry_size(
    const MuTFFUserDataListEntry *entry) {
  return mutff_atom_size(entry->data_size);
}

MuTFFError mutff_write_user_data_list_entry(FILE *fd,
                                            const MuTFFUserDataListEntry *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_user_data_list_entry_size(in);
  MuTFF_FIELD(mutff_write_header, size, in->type);
  for (uint32_t i = 0; i < in->data_size; ++i) {
    MuTFF_FIELD(mutff_write_u8, in->data[i]);
  }
  return ret;
}

MuTFFError mutff_read_user_data_atom(FILE *fd, MuTFFUserDataAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  MuTFFAtomHeader header;
  size_t i;
  size_t offset;

  // read data
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("udta")) {
    return MuTFFErrorBadFormat;
  }

  // read children
  i = 0;
  offset = 8;
  while (offset < size) {
    if (i >= MuTFF_MAX_USER_DATA_ITEMS) {
      return MuTFFErrorOutOfMemory;
    }
    MuTFF_FIELD(mutff_peek_atom_header, &header);
    offset += header.size;
    if (offset > size) {
      return MuTFFErrorBadFormat;
    }
    MuTFF_FIELD(mutff_read_user_data_list_entry, &out->user_data_list[i]);

    i++;
  }
  out->list_entries = i;

  return ret;
}

static inline uint64_t mutff_user_data_atom_size(
    const MuTFFUserDataAtom *atom) {
  uint64_t size = 0;
  for (size_t i = 0; i < atom->list_entries; ++i) {
    size += mutff_user_data_list_entry_size(&atom->user_data_list[i]);
  }
  return mutff_atom_size(size);
}

MuTFFError mutff_write_user_data_atom(FILE *fd, const MuTFFUserDataAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  MuTFFAtomHeader header;
  const uint64_t size = mutff_user_data_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("udta"));
  for (size_t i = 0; i < in->list_entries; ++i) {
    MuTFF_FIELD(mutff_write_user_data_list_entry, &in->user_data_list[i]);
  }
  return ret;
}

MuTFFError mutff_read_track_header_atom(FILE *fd, MuTFFTrackHeaderAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("tkhd")) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_u32, &out->creation_time);
  MuTFF_FIELD(mutff_read_u32, &out->modification_time);
  MuTFF_FIELD(mutff_read_u32, &out->track_id);
  fseek(fd, 4, SEEK_CUR);
  ret += 4;
  MuTFF_FIELD(mutff_read_u32, &out->duration);
  fseek(fd, 8, SEEK_CUR);
  ret += 8;
  MuTFF_FIELD(mutff_read_u16, &out->layer);
  MuTFF_FIELD(mutff_read_u16, &out->alternate_group);
  MuTFF_FIELD(mutff_read_q8_8, &out->volume);
  fseek(fd, 2, SEEK_CUR);
  ret += 2;
  for (size_t j = 0; j < 3; ++j) {
    for (size_t i = 0; i < 3; ++i) {
      MuTFF_FIELD(mutff_read_u32, &out->matrix_structure[j][i]);
    }
  }
  MuTFF_FIELD(mutff_read_q16_16, &out->track_width);
  MuTFF_FIELD(mutff_read_q16_16, &out->track_height);
  if (!fseek(fd, size - ret, SEEK_CUR)) {
    ret += size - ret;
  }
  return ret;
}

MuTFFError mutff_write_track_header_atom(FILE *fd,
                                         const MuTFFTrackHeaderAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  MuTFF_FIELD(mutff_write_header, 92, MuTFF_FOUR_C("tkhd"));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_u32, in->creation_time);
  MuTFF_FIELD(mutff_write_u32, in->modification_time);
  MuTFF_FIELD(mutff_write_u32, in->track_id);
  for (size_t i = 0; i < 4; ++i) {
    MuTFF_FIELD(mutff_write_u8, 0);
  }
  MuTFF_FIELD(mutff_write_u32, in->duration);
  for (size_t i = 0; i < 8; ++i) {
    MuTFF_FIELD(mutff_write_u8, 0);
  }
  MuTFF_FIELD(mutff_write_u16, in->layer);
  MuTFF_FIELD(mutff_write_u16, in->alternate_group);
  MuTFF_FIELD(mutff_write_q8_8, in->volume);
  for (size_t i = 0; i < 2; ++i) {
    MuTFF_FIELD(mutff_write_u8, 0);
  }
  for (size_t j = 0; j < 3; ++j) {
    for (size_t i = 0; i < 3; ++i) {
      MuTFF_FIELD(mutff_write_u32, in->matrix_structure[j][i]);
    }
  }
  MuTFF_FIELD(mutff_write_q16_16, in->track_width);
  MuTFF_FIELD(mutff_write_q16_16, in->track_height);
  return ret;
}

MuTFFError mutff_read_track_clean_aperture_dimensions_atom(
    FILE *fd, MuTFFTrackCleanApertureDimensionsAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("clef")) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_q16_16, &out->width);
  MuTFF_FIELD(mutff_read_q16_16, &out->height);
  return ret;
}

static inline uint64_t mutff_track_clean_aperture_dimensions_atom_size(
    const MuTFFTrackCleanApertureDimensionsAtom *atom) {
  return mutff_atom_size(12);
}

MuTFFError mutff_write_track_clean_aperture_dimensions_atom(
    FILE *fd, const MuTFFTrackCleanApertureDimensionsAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_track_clean_aperture_dimensions_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("clef"));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_q16_16, in->width);
  MuTFF_FIELD(mutff_write_q16_16, in->height);
  return ret;
}

MuTFFError mutff_read_track_production_aperture_dimensions_atom(
    FILE *fd, MuTFFTrackProductionApertureDimensionsAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("prof")) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_q16_16, &out->width);
  MuTFF_FIELD(mutff_read_q16_16, &out->height);
  return ret;
}

static inline uint64_t mutff_track_production_aperture_dimensions_atom_size(
    const MuTFFTrackProductionApertureDimensionsAtom *atom) {
  return mutff_atom_size(12);
}

MuTFFError mutff_write_track_production_aperture_dimensions_atom(
    FILE *fd, const MuTFFTrackProductionApertureDimensionsAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size =
      mutff_track_production_aperture_dimensions_atom_size(in);
  MuTFF_FIELD(mutff_write_header, 20, MuTFF_FOUR_C("prof"));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_q16_16, in->width);
  MuTFF_FIELD(mutff_write_q16_16, in->height);
  return ret;
}

MuTFFError mutff_read_track_encoded_pixels_dimensions_atom(
    FILE *fd, MuTFFTrackEncodedPixelsDimensionsAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("enof")) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_q16_16, &out->width);
  MuTFF_FIELD(mutff_read_q16_16, &out->height);
  return ret;
}

static inline uint64_t mutff_track_encoded_pixels_atom_size(
    const MuTFFTrackEncodedPixelsDimensionsAtom *atom) {
  return mutff_atom_size(12);
}

MuTFFError mutff_write_track_encoded_pixels_dimensions_atom(
    FILE *fd, const MuTFFTrackEncodedPixelsDimensionsAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_track_encoded_pixels_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("enof"));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_q16_16, in->width);
  MuTFF_FIELD(mutff_write_q16_16, in->height);
  return ret;
}

MuTFFError mutff_read_track_aperture_mode_dimensions_atom(
    FILE *fd, MuTFFTrackApertureModeDimensionsAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("tapt")) {
    return MuTFFErrorBadFormat;
  }

  // read children
  size_t offset = 8;
  MuTFFAtomHeader header;
  while (offset < size) {
    MuTFF_FIELD(mutff_peek_atom_header, &header);
    offset += header.size;
    if (offset > size) {
      return MuTFFErrorBadFormat;
    }

    switch (header.type) {
      /* case MuTFF_FOUR_C("clef"): */
      case 0x636c6566:
        MuTFF_FIELD(mutff_read_track_clean_aperture_dimensions_atom,
                    &out->track_clean_aperture_dimension);
        break;
      /* case MuTFF_FOUR_C("prof"): */
      case 0x70726f66:
        MuTFF_FIELD(mutff_read_track_production_aperture_dimensions_atom,
                    &out->track_production_aperture_dimension);
        break;
      /* case MuTFF_FOUR_C("enof"): */
      case 0x656e6f66:
        MuTFF_FIELD(mutff_read_track_encoded_pixels_dimensions_atom,
                    &out->track_encoded_pixels_dimension);
        break;
      default:
        // Unrecognised atom type, skip atom
        fseek(fd, header.size, SEEK_CUR);
        ret += header.size;
    }
  }

  return ret;
}

static inline uint64_t mutff_track_aperture_mode_dimensions_atom_size(
    const MuTFFTrackApertureModeDimensionsAtom *atom) {
  return mutff_atom_size(60);
}

MuTFFError mutff_write_track_aperture_mode_dimensions_atom(
    FILE *fd, const MuTFFTrackApertureModeDimensionsAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_track_aperture_mode_dimensions_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("tapt"));
  MuTFF_FIELD(mutff_write_track_clean_aperture_dimensions_atom,
              &in->track_clean_aperture_dimension);
  MuTFF_FIELD(mutff_write_track_production_aperture_dimensions_atom,
              &in->track_production_aperture_dimension);
  MuTFF_FIELD(mutff_write_track_encoded_pixels_dimensions_atom,
              &in->track_encoded_pixels_dimension);
  return ret;
}

MuTFFError mutff_read_sample_description(FILE *fd,
                                         MuTFFSampleDescription *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  MuTFF_FIELD(mutff_read_u32, &out->size);
  MuTFF_FIELD(mutff_read_u32, &out->data_format);
  fseek(fd, 6, SEEK_CUR);
  ret += 6;
  MuTFF_FIELD(mutff_read_u16, &out->data_reference_index);
  const uint32_t data_size = mutff_data_size(out->size) - 8;
  for (uint32_t i = 0; i < data_size; ++i) {
    MuTFF_FIELD(mutff_read_u8, (uint8_t *)&out->additional_data[i]);
  }
  return ret;
}

MuTFFError mutff_write_sample_description(FILE *fd,
                                          const MuTFFSampleDescription *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  MuTFF_FIELD(mutff_write_u32, in->size);
  MuTFF_FIELD(mutff_write_u32, in->data_format);
  for (size_t i = 0; i < 6; ++i) {
    MuTFF_FIELD(mutff_write_u8, 0);
  }
  MuTFF_FIELD(mutff_write_u16, in->data_reference_index);
  const size_t data_size = in->size - 16;
  for (size_t i = 0; i < data_size; ++i) {
    MuTFF_FIELD(mutff_write_u8, in->additional_data[i]);
  }
  return ret;
}

MuTFFError mutff_read_compressed_matte_atom(FILE *fd,
                                            MuTFFCompressedMatteAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("kmat")) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);

  // read sample description
  MuTFF_FIELD(mutff_read_sample_description,
              &out->matte_image_description_structure);

  // read matte data
  out->matte_data_len = size - 12 - out->matte_image_description_structure.size;
  for (uint32_t i = 0; i < out->matte_data_len; ++i) {
    MuTFF_FIELD(mutff_read_u8, (uint8_t *)&out->matte_data[i]);
  }

  return ret;
}

static inline uint64_t mutff_compressed_matte_atom_size(
    const MuTFFCompressedMatteAtom *atom) {
  return mutff_atom_size(4 + atom->matte_image_description_structure.size +
                         atom->matte_data_len);
}

MuTFFError mutff_write_compressed_matte_atom(
    FILE *fd, const MuTFFCompressedMatteAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_compressed_matte_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("kmat"));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_sample_description,
              &in->matte_image_description_structure);
  for (size_t i = 0; i < in->matte_data_len; ++i) {
    MuTFF_FIELD(mutff_write_u8, in->matte_data[i]);
  }
  return ret;
}

MuTFFError mutff_read_track_matte_atom(FILE *fd, MuTFFTrackMatteAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("matt")) {
    return MuTFFErrorBadFormat;
  }

  // read child atom
  MuTFF_FIELD(mutff_read_compressed_matte_atom, &out->compressed_matte_atom);

  // skip any remaining data
  if (!fseek(fd, size - ret, SEEK_CUR)) {
    ret += size - ret;
  }

  return ret;
}

static inline uint64_t mutff_track_matte_atom_size(
    const MuTFFTrackMatteAtom *atom) {
  return 8 + mutff_compressed_matte_atom_size(&atom->compressed_matte_atom);
}

MuTFFError mutff_write_track_matte_atom(FILE *fd,
                                        const MuTFFTrackMatteAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_track_matte_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("matt"));
  MuTFF_FIELD(mutff_write_compressed_matte_atom, &in->compressed_matte_atom);
  return ret;
}

MuTFFError mutff_read_edit_list_entry(FILE *fd, MuTFFEditListEntry *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  MuTFF_FIELD(mutff_read_u32, &out->track_duration);
  MuTFF_FIELD(mutff_read_u32, &out->media_time);
  MuTFF_FIELD(mutff_read_q16_16, &out->media_rate);
  return ret;
}

MuTFFError mutff_write_edit_list_entry(FILE *fd, const MuTFFEditListEntry *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  MuTFF_FIELD(mutff_write_u32, in->track_duration);
  MuTFF_FIELD(mutff_write_u32, in->media_time);
  MuTFF_FIELD(mutff_write_q16_16, in->media_rate);
  return ret;
}

MuTFFError mutff_read_edit_list_atom(FILE *fd, MuTFFEditListAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("elst")) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_u32, &out->number_of_entries);

  // read edit list table
  if (out->number_of_entries > MuTFF_MAX_EDIT_LIST_ENTRIES) {
    return MuTFFErrorOutOfMemory;
  }
  const size_t edit_list_table_size = size - 16;
  if (edit_list_table_size != out->number_of_entries * 12) {
    return MuTFFErrorBadFormat;
  }
  for (size_t i = 0; i < out->number_of_entries; ++i) {
    MuTFF_FIELD(mutff_read_edit_list_entry, &out->edit_list_table[i]);
  }

  return ret;
}

static inline uint64_t mutff_edit_list_atom_size(
    const MuTFFEditListAtom *atom) {
  return mutff_atom_size(8 + atom->number_of_entries * 12);
}

MuTFFError mutff_write_edit_list_atom(FILE *fd, const MuTFFEditListAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_edit_list_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("elst"));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_u32, in->number_of_entries);
  for (size_t i = 0; i < in->number_of_entries; ++i) {
    MuTFF_FIELD(mutff_write_edit_list_entry, &in->edit_list_table[i]);
  }
  return ret;
}

MuTFFError mutff_read_edit_atom(FILE *fd, MuTFFEditAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("edts")) {
    return MuTFFErrorBadFormat;
  }

  // read child atom
  MuTFF_FIELD(mutff_read_edit_list_atom, &out->edit_list_atom);

  if (!fseek(fd, size - ret, SEEK_CUR)) {
    ret += size - ret;
  }

  return ret;
}

static inline uint64_t mutff_edit_atom_size(const MuTFFEditAtom *atom) {
  return mutff_atom_size(mutff_edit_list_atom_size(&atom->edit_list_atom));
}

MuTFFError mutff_write_edit_atom(FILE *fd, const MuTFFEditAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_edit_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("edts"));
  MuTFF_FIELD(mutff_write_edit_list_atom, &in->edit_list_atom);
  return ret;
}

MuTFFError mutff_read_track_reference_type_atom(
    FILE *fd, MuTFFTrackReferenceTypeAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  MuTFF_FIELD(mutff_read_header, &size, &out->type);

  // read track references
  if (mutff_data_size(size) % 4 != 0) {
    return MuTFFErrorBadFormat;
  }
  out->track_id_count = mutff_data_size(size) / 4;
  if (out->track_id_count > MuTFF_MAX_TRACK_REFERENCE_TYPE_TRACK_IDS) {
    return MuTFFErrorOutOfMemory;
  }
  for (unsigned int i = 0; i < out->track_id_count; ++i) {
    MuTFF_FIELD(mutff_read_u32, &out->track_ids[i]);
  }

  return ret;
}

static inline uint64_t mutff_track_reference_type_atom_size(
    const MuTFFTrackReferenceTypeAtom *atom) {
  return mutff_atom_size(4 * atom->track_id_count);
}

MuTFFError mutff_write_track_reference_type_atom(
    FILE *fd, const MuTFFTrackReferenceTypeAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_track_reference_type_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, in->type);
  for (size_t i = 0; i < in->track_id_count; ++i) {
    MuTFF_FIELD(mutff_write_u32, in->track_ids[i]);
  }
  return ret;
}

MuTFFError mutff_read_track_reference_atom(FILE *fd,
                                           MuTFFTrackReferenceAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("tref")) {
    return MuTFFErrorBadFormat;
  }

  // read children
  size_t offset = 8;
  size_t i = 0;
  MuTFFAtomHeader header;
  while (offset < size) {
    if (i >= MuTFF_MAX_TRACK_REFERENCE_TYPE_ATOMS) {
      return MuTFFErrorOutOfMemory;
    }
    MuTFF_FIELD(mutff_peek_atom_header, &header);
    offset += header.size;
    if (offset > size) {
      return MuTFFErrorBadFormat;
    }
    MuTFF_FIELD(mutff_read_track_reference_type_atom,
                &out->track_reference_type[i]);
    i++;
  }
  out->track_reference_type_count = i;

  return ret;
}

static inline uint64_t mutff_track_reference_atom_size(
    const MuTFFTrackReferenceAtom *atom) {
  uint64_t size = 8;
  for (size_t i = 0; i < atom->track_reference_type_count; ++i) {
    size +=
        mutff_track_reference_type_atom_size(&atom->track_reference_type[i]);
  }
  return size;
}

MuTFFError mutff_write_track_reference_atom(FILE *fd,
                                            const MuTFFTrackReferenceAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_track_reference_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("tref"));
  for (size_t i = 0; i < in->track_reference_type_count; ++i) {
    MuTFF_FIELD(mutff_write_track_reference_type_atom,
                &in->track_reference_type[i]);
  }
  return ret;
}

MuTFFError mutff_read_track_exclude_from_autoselection_atom(
    FILE *fd, MuTFFTrackExcludeFromAutoselectionAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("txas")) {
    return MuTFFErrorBadFormat;
  }
  return ret;
}

static inline uint64_t mutff_track_exclude_from_autoselection_atom_size(
    const MuTFFTrackExcludeFromAutoselectionAtom *atom) {
  return mutff_atom_size(0);
}

MuTFFError mutff_write_track_exclude_from_autoselection_atom(
    FILE *fd, const MuTFFTrackExcludeFromAutoselectionAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_track_exclude_from_autoselection_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("txas"));
  return ret;
}

MuTFFError mutff_read_track_load_settings_atom(
    FILE *fd, MuTFFTrackLoadSettingsAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("load")) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u32, &out->preload_start_time);
  MuTFF_FIELD(mutff_read_u32, &out->preload_duration);
  MuTFF_FIELD(mutff_read_u32, &out->preload_flags);
  MuTFF_FIELD(mutff_read_u32, &out->default_hints);
  return ret;
}

static inline uint64_t mutff_track_load_settings_atom_size(
    const MuTFFTrackLoadSettingsAtom *atom) {
  return mutff_atom_size(16);
}

MuTFFError mutff_write_track_load_settings_atom(
    FILE *fd, const MuTFFTrackLoadSettingsAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_track_load_settings_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("load"));
  MuTFF_FIELD(mutff_write_u32, in->preload_start_time);
  MuTFF_FIELD(mutff_write_u32, in->preload_duration);
  MuTFF_FIELD(mutff_write_u32, in->preload_flags);
  MuTFF_FIELD(mutff_write_u32, in->default_hints);
  return ret;
}

MuTFFError mutff_read_input_type_atom(FILE *fd, MuTFFInputTypeAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("\0\0ty")) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u32, &out->input_type);
  return ret;
}

static inline uint64_t mutff_input_type_atom_size(
    const MuTFFInputTypeAtom *atom) {
  return mutff_atom_size(4);
}

MuTFFError mutff_write_input_type_atom(FILE *fd, const MuTFFInputTypeAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_input_type_atom_size(in);
  MuTFF_FIELD(mutff_write_header, 12, MuTFF_FOUR_C("\0\0ty"));
  MuTFF_FIELD(mutff_write_u32, in->input_type);
  return ret;
}

MuTFFError mutff_read_object_id_atom(FILE *fd, MuTFFObjectIDAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("obid")) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u32, &out->object_id);
  return ret;
}

static inline uint64_t mutff_object_id_atom_size(
    const MuTFFObjectIDAtom *atom) {
  return mutff_atom_size(4);
}

MuTFFError mutff_write_object_id_atom(FILE *fd, const MuTFFObjectIDAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_object_id_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("obid"));
  MuTFF_FIELD(mutff_write_u32, in->object_id);
  return ret;
}

MuTFFError mutff_read_track_input_atom(FILE *fd, MuTFFTrackInputAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("\0\0in")) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u32, &out->atom_id);
  fseek(fd, 2, SEEK_CUR);
  ret += 2;
  MuTFF_FIELD(mutff_read_u16, &out->child_count);
  fseek(fd, 4, SEEK_CUR);
  ret += 4;

  // read children
  size_t offset = 20;
  MuTFFAtomHeader header;
  while (offset < size) {
    MuTFF_FIELD(mutff_peek_atom_header, &header);
    offset += header.size;
    if (offset > size) {
      return MuTFFErrorBadFormat;
    }
    switch (header.type) {
      /* case MuTFF_FOUR_C("\0\0ty"): */
      case 0x00007479:
        MuTFF_FIELD(mutff_read_input_type_atom, &out->input_type_atom);
        break;
      /* case MuTFF_FOUR_C("obid"): */
      case 0x6f626964:
        MuTFF_FIELD(mutff_read_object_id_atom, &out->object_id_atom);
        break;
      default:
        fseek(fd, header.size, SEEK_CUR);
        ret += header.size;
    }
  }

  return ret;
}

static inline uint64_t mutff_track_input_atom_size(
    const MuTFFTrackInputAtom *atom) {
  return mutff_atom_size(12 +
                         mutff_input_type_atom_size(&atom->input_type_atom) +
                         mutff_object_id_atom_size(&atom->object_id_atom));
}

MuTFFError mutff_write_track_input_atom(FILE *fd,
                                        const MuTFFTrackInputAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_track_input_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("\0\0in"));
  MuTFF_FIELD(mutff_write_u32, in->atom_id);
  for (size_t i = 0; i < 2; ++i) {
    MuTFF_FIELD(mutff_write_u8, 0);
  }
  MuTFF_FIELD(mutff_write_u16, in->child_count);
  for (size_t i = 0; i < 4; ++i) {
    MuTFF_FIELD(mutff_write_u8, 0);
  }
  MuTFF_FIELD(mutff_write_input_type_atom, &in->input_type_atom);
  MuTFF_FIELD(mutff_write_object_id_atom, &in->object_id_atom);
  return ret;
}

MuTFFError mutff_read_track_input_map_atom(FILE *fd,
                                           MuTFFTrackInputMapAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("imap")) {
    return MuTFFErrorBadFormat;
  }

  // read children
  size_t offset = 8;
  size_t i = 0;
  MuTFFAtomHeader header;
  while (offset < size) {
    if (i >= MuTFF_MAX_TRACK_REFERENCE_TYPE_ATOMS) {
      return MuTFFErrorOutOfMemory;
    }
    MuTFF_FIELD(mutff_peek_atom_header, &header);
    offset += header.size;
    if (offset > size) {
      return MuTFFErrorBadFormat;
    }
    if (header.type == MuTFF_FOUR_C("\0\0in")) {
      MuTFF_FIELD(mutff_read_track_input_atom, &out->track_input_atoms[i]);
      i++;
    } else {
      fseek(fd, header.size, SEEK_CUR);
      ret += header.size;
    }
  }
  out->track_input_atom_count = i;

  return ret;
}

static inline uint64_t mutff_track_input_map_atom_size(
    const MuTFFTrackInputMapAtom *atom) {
  uint64_t size = 0;
  for (size_t i = 0; i < atom->track_input_atom_count; ++i) {
    size += mutff_track_input_atom_size(&atom->track_input_atoms[i]);
  }
  return mutff_atom_size(size);
}

MuTFFError mutff_write_track_input_map_atom(FILE *fd,
                                            const MuTFFTrackInputMapAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_track_input_map_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("imap"));
  for (size_t i = 0; i < in->track_input_atom_count; ++i) {
    MuTFF_FIELD(mutff_write_track_input_atom, &in->track_input_atoms[i]);
  }
  return ret;
}

MuTFFError mutff_read_media_header_atom(FILE *fd, MuTFFMediaHeaderAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("mdhd")) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_u32, &out->creation_time);
  MuTFF_FIELD(mutff_read_u32, &out->modification_time);
  MuTFF_FIELD(mutff_read_u32, &out->time_scale);
  MuTFF_FIELD(mutff_read_u32, &out->duration);
  MuTFF_FIELD(mutff_read_u16, &out->language);
  MuTFF_FIELD(mutff_read_u16, &out->quality);
  return ret;
}

static inline uint64_t mutff_media_header_atom_size(
    const MuTFFMediaHeaderAtom *atom) {
  return mutff_atom_size(24);
}

MuTFFError mutff_write_media_header_atom(FILE *fd,
                                         const MuTFFMediaHeaderAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_media_header_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("mdhd"));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_u32, in->creation_time);
  MuTFF_FIELD(mutff_write_u32, in->modification_time);
  MuTFF_FIELD(mutff_write_u32, in->time_scale);
  MuTFF_FIELD(mutff_write_u32, in->duration);
  MuTFF_FIELD(mutff_write_u16, in->language);
  MuTFF_FIELD(mutff_write_u16, in->quality);
  return ret;
}

MuTFFError mutff_read_extended_language_tag_atom(
    FILE *fd, MuTFFExtendedLanguageTagAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("elng")) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);

  // read variable-length data
  const size_t tag_length = size - 12;
  if (tag_length > MuTFF_MAX_LANGUAGE_TAG_LENGTH) {
    return MuTFFErrorOutOfMemory;
  }
  for (size_t i = 0; i < tag_length; ++i) {
    MuTFF_FIELD(mutff_read_u8, (uint8_t *)&out->language_tag_string[i]);
  }

  return ret;
}

// @TODO: should this round up to a multiple of four for performance reasons?
//        this particular string is zero-terminated so should be possible.
static inline uint64_t mutff_extended_language_tag_atom_size(
    const MuTFFExtendedLanguageTagAtom *atom) {
  return mutff_atom_size(4 + strlen(atom->language_tag_string) + 1);
}

MuTFFError mutff_write_extended_language_tag_atom(
    FILE *fd, const MuTFFExtendedLanguageTagAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  size_t i;
  const uint64_t size = mutff_extended_language_tag_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("elng"));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  i = 0;
  while (in->language_tag_string[i]) {
    MuTFF_FIELD(mutff_write_u8, in->language_tag_string[i]);
    ++i;
  }
  for (; i < mutff_data_size(size) - 4; ++i) {
    MuTFF_FIELD(mutff_write_u8, 0);
  }
  return ret;
}

MuTFFError mutff_read_handler_reference_atom(FILE *fd,
                                             MuTFFHandlerReferenceAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  size_t i;
  size_t name_length;

  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("hdlr")) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_u32, &out->component_type);
  MuTFF_FIELD(mutff_read_u32, &out->component_subtype);
  MuTFF_FIELD(mutff_read_u32, &out->component_manufacturer);
  MuTFF_FIELD(mutff_read_u32, &out->component_flags);
  MuTFF_FIELD(mutff_read_u32, &out->component_flags_mask);

  // read variable-length data
  name_length = size - ret;
  if (name_length > MuTFF_MAX_COMPONENT_NAME_LENGTH) {
    return MuTFFErrorOutOfMemory;
  }
  for (i = 0; i < name_length; ++i) {
    MuTFF_FIELD(mutff_read_u8, (uint8_t *)&out->component_name[i]);
  }
  out->component_name[i] = '\0';

  return ret;
}

static inline uint64_t mutff_handler_reference_atom_size(
    const MuTFFHandlerReferenceAtom *atom) {
  return mutff_atom_size(24 + strlen(atom->component_name));
}

MuTFFError mutff_write_handler_reference_atom(
    FILE *fd, const MuTFFHandlerReferenceAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_handler_reference_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("hdlr"));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_u32, in->component_type);
  MuTFF_FIELD(mutff_write_u32, in->component_subtype);
  MuTFF_FIELD(mutff_write_u32, in->component_manufacturer);
  MuTFF_FIELD(mutff_write_u32, in->component_flags);
  MuTFF_FIELD(mutff_write_u32, in->component_flags_mask);
  for (size_t i = 0; i < size - 32; ++i) {
    MuTFF_FIELD(mutff_write_u8, in->component_name[i]);
  }
  return ret;
}

MuTFFError mutff_read_video_media_information_header_atom(
    FILE *fd, MuTFFVideoMediaInformationHeaderAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("vmhd")) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_u16, &out->graphics_mode);
  for (size_t i = 0; i < 3; ++i) {
    MuTFF_FIELD(mutff_read_u16, &out->opcolor[i]);
  }
  return ret;
}

static inline uint64_t mutff_video_media_information_header_atom_size(
    const MuTFFVideoMediaInformationHeaderAtom *atom) {
  return mutff_atom_size(12);
}

MuTFFError mutff_write_video_media_information_header_atom(
    FILE *fd, const MuTFFVideoMediaInformationHeaderAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_video_media_information_header_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("vmhd"));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_u16, in->graphics_mode);
  for (size_t i = 0; i < 3; ++i) {
    MuTFF_FIELD(mutff_write_u16, in->opcolor[i]);
  }
  return ret;
}

MuTFFError mutff_read_data_reference(FILE *fd, MuTFFDataReference *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint32_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_u32, &size);
  MuTFF_FIELD(mutff_read_u32, &out->type);
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);

  // read variable-length data
  out->data_size = size - 12;
  if (out->data_size > MuTFF_MAX_DATA_REFERENCE_DATA_SIZE) {
    return MuTFFErrorOutOfMemory;
  }
  for (size_t i = 0; i < out->data_size; ++i) {
    MuTFF_FIELD(mutff_read_u8, (uint8_t *)&out->data[i]);
  }

  return ret;
}

static inline uint64_t mutff_data_reference_size(
    const MuTFFDataReference *ref) {
  return 12 + ref->data_size;
}

MuTFFError mutff_write_data_reference(FILE *fd, const MuTFFDataReference *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_data_reference_size(in);
  MuTFF_FIELD(mutff_write_header, size, in->type);
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  for (size_t i = 0; i < in->data_size; ++i) {
    MuTFF_FIELD(mutff_write_u8, in->data[i]);
  }
  return ret;
}

MuTFFError mutff_read_data_reference_atom(FILE *fd,
                                          MuTFFDataReferenceAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  MuTFFAtomHeader header;
  size_t offset;
  uint64_t size;
  uint32_t type;

  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("dref")) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_u32, &out->number_of_entries);

  // read child atoms
  if (out->number_of_entries > MuTFF_MAX_DATA_REFERENCES) {
    return MuTFFErrorOutOfMemory;
  }
  offset = 16;
  for (size_t i = 0; i < out->number_of_entries; ++i) {
    MuTFF_FIELD(mutff_peek_atom_header, &header);
    offset += header.size;
    if (offset > size) {
      return MuTFFErrorBadFormat;
    }
    MuTFF_FIELD(mutff_read_data_reference, &out->data_references[i]);
  }

  // skip any remaining space
  if (!fseek(fd, size - ret, SEEK_CUR)) {
    ret += size - ret;
  }

  return ret;
}

static inline uint64_t mutff_data_reference_atom_size(
    const MuTFFDataReferenceAtom *atom) {
  uint64_t size = 8;
  for (uint32_t i = 0; i < atom->number_of_entries; ++i) {
    size += mutff_data_reference_size(&atom->data_references[i]);
  }
  return mutff_atom_size(size);
}

MuTFFError mutff_write_data_reference_atom(FILE *fd,
                                           const MuTFFDataReferenceAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  size_t offset;
  const uint64_t size = mutff_data_reference_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("dref"));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_u32, in->number_of_entries);
  offset = 16;
  for (uint32_t i = 0; i < in->number_of_entries; ++i) {
    offset += mutff_data_reference_size(&in->data_references[i]);
    if (offset > size) {
      return MuTFFErrorBadFormat;
    }
    MuTFF_FIELD(mutff_write_data_reference, &in->data_references[i]);
  }
  return ret;
}

MuTFFError mutff_read_data_information_atom(FILE *fd,
                                            MuTFFDataInformationAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("dinf")) {
    return MuTFFErrorBadFormat;
  }

  // read child atom
  MuTFF_FIELD(mutff_read_data_reference_atom, &out->data_reference);

  // skip any remaining space
  if (!fseek(fd, size - ret, SEEK_CUR)) {
    ret += size - ret;
  }

  return ret;
}

static inline uint64_t mutff_data_information_atom_size(
    const MuTFFDataInformationAtom *atom) {
  return mutff_atom_size(mutff_data_reference_atom_size(&atom->data_reference));
}

MuTFFError mutff_write_data_information_atom(
    FILE *fd, const MuTFFDataInformationAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_data_information_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("dinf"));
  MuTFF_FIELD(mutff_write_data_reference_atom, &in->data_reference);
  return ret;
}

MuTFFError mutff_read_sample_description_atom(FILE *fd,
                                              MuTFFSampleDescriptionAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  MuTFFAtomHeader header;
  size_t offset;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("stsd")) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_u32, &out->number_of_entries);

  // read child atoms
  if (out->number_of_entries > MuTFF_MAX_SAMPLE_DESCRIPTION_TABLE_LEN) {
    return MuTFFErrorOutOfMemory;
  }
  offset = 16;
  for (size_t i = 0; i < out->number_of_entries; ++i) {
    MuTFF_FIELD(mutff_peek_atom_header, &header);
    offset += header.size;
    if (offset > size) {
      return MuTFFErrorBadFormat;
    }
    MuTFF_FIELD(mutff_read_sample_description,
                &out->sample_description_table[i]);
  }

  // skip any remaining space
  if (!fseek(fd, size - ret, SEEK_CUR)) {
    ret += size - ret;
  }

  return ret;
}

static inline uint64_t mutff_sample_description_atom_size(
    const MuTFFSampleDescriptionAtom *atom) {
  uint64_t size = 0;
  for (uint32_t i = 0; i < atom->number_of_entries; ++i) {
    size += atom->sample_description_table[i].size;
  }
  return mutff_atom_size(8 + size);
}

MuTFFError mutff_write_sample_description_atom(
    FILE *fd, const MuTFFSampleDescriptionAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  size_t offset;
  const uint64_t size = mutff_sample_description_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("stsd"));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_u32, in->number_of_entries);
  offset = 16;
  for (size_t i = 0; i < in->number_of_entries; ++i) {
    offset += in->sample_description_table[i].size;
    if (offset > size) {
      return MuTFFErrorBadFormat;
    }
    MuTFF_FIELD(mutff_write_sample_description,
                &in->sample_description_table[i]);
  }
  for (; offset < size; ++offset) {
    MuTFF_FIELD(mutff_write_u8, 0);
  }
  return ret;
}

MuTFFError mutff_read_time_to_sample_table_entry(
    FILE *fd, MuTFFTimeToSampleTableEntry *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  MuTFF_FIELD(mutff_read_u32, &out->sample_count);
  MuTFF_FIELD(mutff_read_u32, &out->sample_duration);
  return ret;
}

MuTFFError mutff_write_time_to_sample_table_entry(
    FILE *fd, const MuTFFTimeToSampleTableEntry *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  MuTFF_FIELD(mutff_write_u32, in->sample_count);
  MuTFF_FIELD(mutff_write_u32, in->sample_duration);
  return ret;
}

MuTFFError mutff_read_time_to_sample_atom(FILE *fd,
                                          MuTFFTimeToSampleAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("stts")) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_u32, &out->number_of_entries);

  // read time to sample table
  if (out->number_of_entries > MuTFF_MAX_TIME_TO_SAMPLE_TABLE_LEN) {
    return MuTFFErrorOutOfMemory;
  }
  const size_t table_size = mutff_data_size(size) - 8;
  if (table_size != out->number_of_entries * 8) {
    return MuTFFErrorBadFormat;
  }
  for (size_t i = 0; i < out->number_of_entries; ++i) {
    MuTFF_FIELD(mutff_read_time_to_sample_table_entry,
                &out->time_to_sample_table[i]);
    ;
  }

  return ret;
}

static inline uint64_t mutff_time_to_sample_atom_size(
    const MuTFFTimeToSampleAtom *atom) {
  return mutff_atom_size(8 + atom->number_of_entries * 8);
}

MuTFFError mutff_write_time_to_sample_atom(FILE *fd,
                                           const MuTFFTimeToSampleAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_time_to_sample_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("stts"));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_u32, in->number_of_entries);
  if (in->number_of_entries * 8 != mutff_data_size(size) - 8) {
    return MuTFFErrorBadFormat;
  }
  for (uint32_t i = 0; i < in->number_of_entries; ++i) {
    MuTFF_FIELD(mutff_write_time_to_sample_table_entry,
                &in->time_to_sample_table[i]);
  }
  return ret;
}

MuTFFError mutff_read_composition_offset_table_entry(
    FILE *fd, MuTFFCompositionOffsetTableEntry *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  MuTFF_FIELD(mutff_read_u32, &out->sample_count);
  MuTFF_FIELD(mutff_read_u32, &out->composition_offset);
  return ret;
}

MuTFFError mutff_write_composition_offset_table_entry(
    FILE *fd, const MuTFFCompositionOffsetTableEntry *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  MuTFF_FIELD(mutff_write_u32, in->sample_count);
  MuTFF_FIELD(mutff_write_u32, in->composition_offset);
  return ret;
}

MuTFFError mutff_read_composition_offset_atom(FILE *fd,
                                              MuTFFCompositionOffsetAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("ctts")) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_u32, &out->entry_count);

  // read composition offset table
  if (out->entry_count > MuTFF_MAX_COMPOSITION_OFFSET_TABLE_LEN) {
    return MuTFFErrorOutOfMemory;
  }
  const size_t table_size = mutff_data_size(size) - 8;
  if (table_size != out->entry_count * 8) {
    return MuTFFErrorBadFormat;
  }
  for (size_t i = 0; i < out->entry_count; ++i) {
    MuTFF_FIELD(mutff_read_composition_offset_table_entry,
                &out->composition_offset_table[i]);
    ;
  }

  return ret;
}

static inline uint64_t mutff_composition_offset_atom_size(
    const MuTFFCompositionOffsetAtom *atom) {
  return mutff_atom_size(8 + 8 * atom->entry_count);
}

MuTFFError mutff_write_composition_offset_atom(
    FILE *fd, const MuTFFCompositionOffsetAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_composition_offset_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("ctts"));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_u32, in->entry_count);
  if (in->entry_count * 8 != mutff_data_size(size) - 8) {
    return MuTFFErrorBadFormat;
  }
  for (uint32_t i = 0; i < in->entry_count; ++i) {
    MuTFF_FIELD(mutff_write_composition_offset_table_entry,
                &in->composition_offset_table[i]);
  }
  return ret;
}

MuTFFError mutff_read_composition_shift_least_greatest_atom(
    FILE *fd, MuTFFCompositionShiftLeastGreatestAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("cslg")) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_u32, &out->composition_offset_to_display_offset_shift);
  MuTFF_FIELD(mutff_read_i32, &out->least_display_offset);
  MuTFF_FIELD(mutff_read_i32, &out->greatest_display_offset);
  MuTFF_FIELD(mutff_read_i32, &out->display_start_time);
  MuTFF_FIELD(mutff_read_i32, &out->display_end_time);
  return ret;
}

static inline uint64_t mutff_composition_shift_least_greatest_atom_size(
    const MuTFFCompositionShiftLeastGreatestAtom *atom) {
  return mutff_atom_size(24);
}

MuTFFError mutff_write_composition_shift_least_greatest_atom(
    FILE *fd, const MuTFFCompositionShiftLeastGreatestAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_composition_shift_least_greatest_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("cslg"));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_u32, in->composition_offset_to_display_offset_shift);
  MuTFF_FIELD(mutff_write_i32, in->least_display_offset);
  MuTFF_FIELD(mutff_write_i32, in->greatest_display_offset);
  MuTFF_FIELD(mutff_write_i32, in->display_start_time);
  MuTFF_FIELD(mutff_write_i32, in->display_end_time);
  return ret;
}

MuTFFError mutff_read_sync_sample_atom(FILE *fd, MuTFFSyncSampleAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("stss")) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_u32, &out->number_of_entries);

  // read sync sample table
  if (out->number_of_entries > MuTFF_MAX_SYNC_SAMPLE_TABLE_LEN) {
    return MuTFFErrorOutOfMemory;
  }
  const size_t table_size = mutff_data_size(size) - 8;
  if (table_size != out->number_of_entries * 4) {
    return MuTFFErrorBadFormat;
  }
  for (size_t i = 0; i < out->number_of_entries; ++i) {
    MuTFF_FIELD(mutff_read_u32, &out->sync_sample_table[i]);
  }

  return ret;
}

static inline uint64_t mutff_sync_sample_atom_size(
    const MuTFFSyncSampleAtom *atom) {
  return mutff_atom_size(8 + atom->number_of_entries * 4);
}

MuTFFError mutff_write_sync_sample_atom(FILE *fd,
                                        const MuTFFSyncSampleAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_sync_sample_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("stss"));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_u32, in->number_of_entries);
  if (in->number_of_entries * 4 != mutff_data_size(size) - 8) {
    return MuTFFErrorBadFormat;
  }
  for (uint32_t i = 0; i < in->number_of_entries; ++i) {
    MuTFF_FIELD(mutff_write_u32, in->sync_sample_table[i]);
  }
  return ret;
}

MuTFFError mutff_read_partial_sync_sample_atom(
    FILE *fd, MuTFFPartialSyncSampleAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("stps")) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_u32, &out->entry_count);

  // read partial sync sample table
  if (out->entry_count > MuTFF_MAX_PARTIAL_SYNC_SAMPLE_TABLE_LEN) {
    return MuTFFErrorOutOfMemory;
  }
  const size_t table_size = mutff_data_size(size) - 8;
  if (table_size != out->entry_count * 4) {
    return MuTFFErrorBadFormat;
  }
  for (size_t i = 0; i < out->entry_count; ++i) {
    MuTFF_FIELD(mutff_read_u32, &out->partial_sync_sample_table[i]);
  }

  return ret;
}

static inline uint64_t mutff_partial_sync_sample_atom_size(
    const MuTFFPartialSyncSampleAtom *atom) {
  return mutff_atom_size(8 + atom->entry_count * 4);
}

MuTFFError mutff_write_partial_sync_sample_atom(
    FILE *fd, const MuTFFPartialSyncSampleAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_partial_sync_sample_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("stps"));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_u32, in->entry_count);
  if (in->entry_count * 4 != mutff_data_size(size) - 8) {
    return MuTFFErrorBadFormat;
  }
  for (uint32_t i = 0; i < in->entry_count; ++i) {
    MuTFF_FIELD(mutff_write_u32, in->partial_sync_sample_table[i]);
  }
  return ret;
}

MuTFFError mutff_read_sample_to_chunk_table_entry(
    FILE *fd, MuTFFSampleToChunkTableEntry *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  MuTFF_FIELD(mutff_read_u32, &out->first_chunk);
  MuTFF_FIELD(mutff_read_u32, &out->samples_per_chunk);
  MuTFF_FIELD(mutff_read_u32, &out->sample_description_id);
  return ret;
}

MuTFFError mutff_write_sample_to_chunk_table_entry(
    FILE *fd, const MuTFFSampleToChunkTableEntry *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  MuTFF_FIELD(mutff_write_u32, in->first_chunk);
  MuTFF_FIELD(mutff_write_u32, in->samples_per_chunk);
  MuTFF_FIELD(mutff_write_u32, in->sample_description_id);
  return ret;
}

MuTFFError mutff_read_sample_to_chunk_atom(FILE *fd,
                                           MuTFFSampleToChunkAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("stsc")) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_u32, &out->number_of_entries);

  // read table
  if (out->number_of_entries > MuTFF_MAX_SAMPLE_TO_CHUNK_TABLE_LEN) {
    return MuTFFErrorOutOfMemory;
  }
  const size_t table_size = mutff_data_size(size) - 8;
  if (table_size != out->number_of_entries * 12) {
    return MuTFFErrorBadFormat;
  }
  for (size_t i = 0; i < out->number_of_entries; ++i) {
    MuTFF_FIELD(mutff_read_sample_to_chunk_table_entry,
                &out->sample_to_chunk_table[i]);
  }

  return ret;
}

static inline uint64_t mutff_sample_to_chunk_atom_size(
    const MuTFFSampleToChunkAtom *atom) {
  return mutff_atom_size(8 + atom->number_of_entries * 12);
}

MuTFFError mutff_write_sample_to_chunk_atom(FILE *fd,
                                            const MuTFFSampleToChunkAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_sample_to_chunk_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("stsc"));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_u32, in->number_of_entries);
  if (in->number_of_entries * 12 != mutff_data_size(size) - 8) {
    return MuTFFErrorBadFormat;
  }
  for (uint32_t i = 0; i < in->number_of_entries; ++i) {
    MuTFF_FIELD(mutff_write_sample_to_chunk_table_entry,
                &in->sample_to_chunk_table[i]);
  }
  return ret;
}

MuTFFError mutff_read_sample_size_atom(FILE *fd, MuTFFSampleSizeAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("stsz")) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_u32, &out->sample_size);
  MuTFF_FIELD(mutff_read_u32, &out->number_of_entries);

  if (out->sample_size == 0) {
    // read table
    if (out->number_of_entries > MuTFF_MAX_SAMPLE_SIZE_TABLE_LEN) {
      return MuTFFErrorOutOfMemory;
    }
    const size_t table_size = mutff_data_size(size) - 12;
    if (table_size != out->number_of_entries * 4) {
      return MuTFFErrorBadFormat;
    }
    for (size_t i = 0; i < out->number_of_entries; ++i) {
      MuTFF_FIELD(mutff_read_u32, &out->sample_size_table[i]);
    }
  } else {
    // skip table
    if (!fseek(fd, size - ret, SEEK_CUR)) {
      ret += size - ret;
    }
  }

  return ret;
}

static inline uint64_t mutff_sample_size_atom_size(
    const MuTFFSampleSizeAtom *atom) {
  return mutff_atom_size(12 + atom->number_of_entries * 4);
}

MuTFFError mutff_write_sample_size_atom(FILE *fd,
                                        const MuTFFSampleSizeAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_sample_size_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("stsz"));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_u32, in->sample_size);
  MuTFF_FIELD(mutff_write_u32, in->number_of_entries);
  // @TODO: does this need a branch for in->sample_size != 0?
  //        i.e. what to do if sample_size != 0 but number_of_entries != 0
  if (in->number_of_entries * 4 != mutff_data_size(size) - 12) {
    return MuTFFErrorBadFormat;
  }
  for (uint32_t i = 0; i < in->number_of_entries; ++i) {
    MuTFF_FIELD(mutff_write_u32, in->sample_size_table[i]);
  }
  return ret;
}

MuTFFError mutff_read_chunk_offset_atom(FILE *fd, MuTFFChunkOffsetAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("stco")) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_u32, &out->number_of_entries);

  // read table
  if (out->number_of_entries > MuTFF_MAX_CHUNK_OFFSET_TABLE_LEN) {
    return MuTFFErrorOutOfMemory;
  }
  const size_t table_size = mutff_data_size(size) - 8;
  if (table_size != out->number_of_entries * 4) {
    return MuTFFErrorBadFormat;
  }
  for (size_t i = 0; i < out->number_of_entries; ++i) {
    MuTFF_FIELD(mutff_read_u32, &out->chunk_offset_table[i]);
  }

  return ret;
}

static inline uint64_t mutff_chunk_offset_atom_size(
    const MuTFFChunkOffsetAtom *atom) {
  return mutff_atom_size(8 + atom->number_of_entries * 4);
}

MuTFFError mutff_write_chunk_offset_atom(FILE *fd,
                                         const MuTFFChunkOffsetAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_chunk_offset_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("stco"));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_u32, in->number_of_entries);
  if (in->number_of_entries * 4 != mutff_data_size(size) - 8) {
    return MuTFFErrorBadFormat;
  }
  for (uint32_t i = 0; i < in->number_of_entries; ++i) {
    MuTFF_FIELD(mutff_write_u32, in->chunk_offset_table[i]);
  }
  return ret;
}

MuTFFError mutff_read_sample_dependency_flags_atom(
    FILE *fd, MuTFFSampleDependencyFlagsAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("sdtp")) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);

  // read table
  out->data_size = mutff_data_size(size) - 4;
  if (out->data_size > MuTFF_MAX_SAMPLE_DEPENDENCY_FLAGS_TABLE_LEN) {
    return MuTFFErrorOutOfMemory;
  }
  for (size_t i = 0; i < out->data_size; ++i) {
    MuTFF_FIELD(mutff_read_u8, &out->sample_dependency_flags_table[i]);
  }

  return ret;
}

static inline uint64_t mutff_sample_dependency_flags_atom_size(
    const MuTFFSampleDependencyFlagsAtom *atom) {
  return mutff_atom_size(4 + atom->data_size);
}

MuTFFError mutff_write_sample_dependency_flags_atom(
    FILE *fd, const MuTFFSampleDependencyFlagsAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_sample_dependency_flags_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("sdtp"));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  const size_t flags_table_size = mutff_data_size(size) - 4;
  for (uint32_t i = 0; i < flags_table_size; ++i) {
    MuTFF_FIELD(mutff_write_u8, in->sample_dependency_flags_table[i]);
  }
  return ret;
}

MuTFFError mutff_read_sample_table_atom(FILE *fd, MuTFFSampleTableAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  MuTFFAtomHeader header;
  size_t offset;
  uint64_t size;
  uint32_t type;

  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("stbl")) {
    return MuTFFErrorBadFormat;
  }

  // read child atoms
  offset = 8;
  while (offset < size) {
    MuTFF_FIELD(mutff_peek_atom_header, &header);
    offset += header.size;
    if (offset > size) {
      return MuTFFErrorBadFormat;
    }

    switch (header.type) {
      /* case MuTFF_FOUR_C("stsd"): */
      case 0x73747364:
        MuTFF_FIELD(mutff_read_sample_description_atom,
                    &out->sample_description);
        break;
      /* case MuTFF_FOUR_C("stts"): */
      case 0x73747473:
        MuTFF_FIELD(mutff_read_time_to_sample_atom, &out->time_to_sample);
        break;
      /* case MuTFF_FOUR_C("ctts"): */
      case 0x63747473:
        MuTFF_FIELD(mutff_read_composition_offset_atom,
                    &out->composition_offset);
        break;
      /* case MuTFF_FOUR_C("cslg"): */
      case 0x63736c67:
        MuTFF_FIELD(mutff_read_composition_shift_least_greatest_atom,
                    &out->composition_shift_least_greatest);
        break;
      /* case MuTFF_FOUR_C("stss"): */
      case 0x73747373:
        MuTFF_FIELD(mutff_read_sync_sample_atom, &out->sync_sample);
        break;
      /* case MuTFF_FOUR_C("stps"): */
      case 0x73747073:
        MuTFF_FIELD(mutff_read_partial_sync_sample_atom,
                    &out->partial_sync_sample);
        break;
      /* case MuTFF_FOUR_C("stsc"): */
      case 0x73747363:
        MuTFF_FIELD(mutff_read_sample_to_chunk_atom, &out->sample_to_chunk);
        break;
      /* case MuTFF_FOUR_C("stsz"): */
      case 0x7374737a:
        MuTFF_FIELD(mutff_read_sample_size_atom, &out->sample_size);
        break;
      /* case MuTFF_FOUR_C("stco"): */
      case 0x7374636f:
        MuTFF_FIELD(mutff_read_chunk_offset_atom, &out->chunk_offset);
        break;
      /* case MuTFF_FOUR_C("sdtp"): */
      case 0x73647470:
        MuTFF_FIELD(mutff_read_sample_dependency_flags_atom,
                    &out->sample_dependency_flags);
        break;
      // reserved for future use
      /* /1* case MuTFF_FOUR_C("stsh"): *1/ */
      /* case 0x73747368: */
      /*   break; */
      /* /1* case MuTFF_FOUR_C("sgpd"): *1/ */
      /* case 0x73677064: */
      /*   break; */
      /* /1* case MuTFF_FOUR_C("sbgp"): *1/ */
      /* case 0x73626770: */
      /*   break; */
      default:
        fseek(fd, header.size, SEEK_CUR);
        ret += header.size;
    }
  }

  return ret;
}

MuTFFError mutff_read_video_media_information_atom(
    FILE *fd, MuTFFVideoMediaInformationAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  MuTFFAtomHeader header;
  size_t offset;
  uint64_t size;
  uint32_t type;

  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("minf")) {
    return MuTFFErrorBadFormat;
  }

  // read child atoms
  offset = 8;
  while (offset < size) {
    MuTFF_FIELD(mutff_peek_atom_header, &header);
    offset += header.size;
    if (offset > size) {
      return MuTFFErrorBadFormat;
    }

    switch (header.type) {
      /* case MuTFF_FOUR_C("vmhd"): */
      case 0x766d6864:
        MuTFF_FIELD(mutff_read_video_media_information_header_atom,
                    &out->video_media_information_header);
        break;
      /* case MuTFF_FOUR_C("hdlr"): */
      case 0x68646c72:
        MuTFF_FIELD(mutff_read_handler_reference_atom, &out->handler_reference);
        break;
      /* case MuTFF_FOUR_C("dinf"): */
      case 0x64696e66:
        MuTFF_FIELD(mutff_read_data_information_atom, &out->data_information);
        break;
      /* case MUTFF_FOUR_C("stbl"): */
      case 0x7374626c:
        MuTFF_FIELD(mutff_read_sample_table_atom, &out->sample_table);
        break;
      default:
        fseek(fd, header.size, SEEK_CUR);
        ret += header.size;
    }
  }

  return ret;
}

MuTFFError mutff_read_sound_media_information_header_atom(
    FILE *fd, MuTFFSoundMediaInformationHeaderAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("smhd")) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_i16, &out->balance);
  fseek(fd, 2, SEEK_CUR);
  ret += 2;
  return ret;
}

static inline uint64_t mutff_sound_media_information_header_atom(
    const MuTFFSoundMediaInformationHeaderAtom *atom) {
  return mutff_atom_size(8);
}

MuTFFError mutff_write_sound_media_information_header_atom(
    FILE *fd, const MuTFFSoundMediaInformationHeaderAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_sound_media_information_header_atom(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("smhd"));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_i16, in->balance);
  for (size_t i = 0; i < 2; ++i) {
    MuTFF_FIELD(mutff_write_u8, 0);
  }
  return ret;
}

MuTFFError mutff_read_sound_media_information_atom(
    FILE *fd, MuTFFSoundMediaInformationAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  MuTFFAtomHeader header;
  size_t offset;
  uint64_t size;
  uint32_t type;

  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("minf")) {
    return MuTFFErrorBadFormat;
  }

  // read child atoms
  offset = 8;
  while (offset < size) {
    MuTFF_FIELD(mutff_peek_atom_header, &header);
    offset += header.size;
    if (offset > size) {
      return MuTFFErrorBadFormat;
    }

    switch (header.type) {
      /* case MuTFF_FOUR_C("smhd"): */
      case 0x736d6864:
        MuTFF_FIELD(mutff_read_sound_media_information_header_atom,
                    &out->sound_media_information_header);
        break;
      /* case MuTFF_FOUR_C("hdlr"): */
      case 0x68646c72:
        MuTFF_FIELD(mutff_read_handler_reference_atom, &out->handler_reference);
        break;
      /* case MuTFF_FOUR_C("dinf"): */
      case 0x64696e66:
        MuTFF_FIELD(mutff_read_data_information_atom, &out->data_information);
        break;
      /* case MUTFF_FOUR_C("stbl"): */
      case 0x7374626c:
        MuTFF_FIELD(mutff_read_sample_table_atom, &out->sample_table);
        break;
      default:
        fseek(fd, header.size, SEEK_CUR);
        ret += header.size;
    }
  }

  return ret;
}

MuTFFError mutff_read_base_media_info_atom(FILE *fd,
                                           MuTFFBaseMediaInfoAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("gmin")) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_u16, &out->graphics_mode);
  for (size_t i = 0; i < 3; ++i) {
    MuTFF_FIELD(mutff_read_u16, &out->opcolor[i]);
  }
  MuTFF_FIELD(mutff_read_i16, &out->balance);
  fseek(fd, 2, SEEK_CUR);
  ret += 2;
  return ret;
}

static inline uint64_t mutff_base_media_info_atom_size(
    const MuTFFBaseMediaInfoAtom *atom) {
  return mutff_atom_size(16);
}

MuTFFError mutff_write_base_media_info_atom(FILE *fd,
                                            const MuTFFBaseMediaInfoAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_base_media_info_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("gmin"));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_u16, in->graphics_mode);
  for (size_t i = 0; i < 3; ++i) {
    MuTFF_FIELD(mutff_write_u16, in->opcolor[i]);
  }
  MuTFF_FIELD(mutff_write_i16, in->balance);
  for (size_t i = 0; i < 2; ++i) {
    MuTFF_FIELD(mutff_write_u8, 0);
  }
  return ret;
}

MuTFFError mutff_read_text_media_information_atom(
    FILE *fd, MuTFFTextMediaInformationAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("text")) {
    return MuTFFErrorBadFormat;
  }
  for (size_t j = 0; j < 3; ++j) {
    for (size_t i = 0; i < 3; ++i) {
      MuTFF_FIELD(mutff_read_u32, &out->matrix_structure[j][i]);
    }
  }
  return ret;
}

static inline uint64_t mutff_text_media_information_atom_size(
    const MuTFFTextMediaInformationAtom *atom) {
  return mutff_atom_size(36);
}

MuTFFError mutff_write_text_media_information_atom(
    FILE *fd, const MuTFFTextMediaInformationAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_text_media_information_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("text"));
  for (size_t j = 0; j < 3; ++j) {
    for (size_t i = 0; i < 3; ++i) {
      MuTFF_FIELD(mutff_write_u32, in->matrix_structure[j][i]);
    }
  }
  return ret;
}

MuTFFError mutff_read_base_media_information_header_atom(
    FILE *fd, MuTFFBaseMediaInformationHeaderAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  MuTFFAtomHeader header;
  size_t offset;
  uint64_t size;
  uint32_t type;

  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("gmhd")) {
    return MuTFFErrorBadFormat;
  }

  // read child atoms
  offset = 8;
  while (offset < size) {
    MuTFF_FIELD(mutff_peek_atom_header, &header);
    offset += header.size;
    if (offset > size) {
      return MuTFFErrorBadFormat;
    }

    switch (header.type) {
      /* case MuTFF_FOUR_C("gmin"): */
      case 0x676d696e:
        MuTFF_FIELD(mutff_read_base_media_info_atom, &out->base_media_info);
        break;
      /* case MuTFF_FOUR_C("text"): */
      case 0x74657874:
        MuTFF_FIELD(mutff_read_text_media_information_atom,
                    &out->text_media_information);
        break;
      default:
        fseek(fd, header.size, SEEK_CUR);
        ret += header.size;
    }
  }

  return ret;
}

static inline uint64_t mutff_base_media_information_header_atom_size(
    const MuTFFBaseMediaInformationHeaderAtom *atom) {
  return mutff_atom_size(
      mutff_base_media_info_atom_size(&atom->base_media_info) +
      mutff_text_media_information_atom_size(&atom->text_media_information));
}

MuTFFError mutff_write_base_media_information_header_atom(
    FILE *fd, const MuTFFBaseMediaInformationHeaderAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_base_media_information_header_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("gmhd"));
  MuTFF_FIELD(mutff_write_base_media_info_atom, &in->base_media_info);
  MuTFF_FIELD(mutff_write_text_media_information_atom,
              &in->text_media_information);
  return ret;
}

MuTFFError mutff_read_base_media_information_atom(
    FILE *fd, MuTFFBaseMediaInformationAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("minf")) {
    return MuTFFErrorBadFormat;
  }

  // read child atom
  MuTFF_FIELD(mutff_read_base_media_information_header_atom,
              &out->base_media_information_header);

  // skip remaining space
  if (!fseek(fd, size - ret, SEEK_CUR)) {
    ret += size - ret;
  }

  return ret;
}

static inline uint64_t mutff_base_media_information_atom_size(
    const MuTFFBaseMediaInformationAtom *atom) {
  return mutff_atom_size(mutff_base_media_information_header_atom_size(
      &atom->base_media_information_header));
}

MuTFFError mutff_write_base_media_information_atom(
    FILE *fd, const MuTFFBaseMediaInformationAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  const uint64_t size = mutff_base_media_information_atom_size(in);
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOUR_C("minf"));
  MuTFF_FIELD(mutff_write_base_media_information_header_atom,
              &in->base_media_information_header);
  return ret;
}

MuTFFError mutff_read_media_information_atom(FILE *fd,
                                             MuTFFMediaInformationAtom *out) {
  MuTFFError err;
  MuTFFAtomHeader header;
  size_t offset;
  uint32_t size;

  const size_t start_offset = ftell(fd);

  if ((err = mutff_read_u32(fd, &size)) < 0) {
    return err;
  }

  // skip type
  fseek(fd, 4, SEEK_CUR);

  // iterate over children
  offset = 8;
  while (offset < size) {
    if ((err = mutff_peek_atom_header(fd, &header)) < 0) {
      return err;
    }
    offset += header.size;
    if (offset > size) {
      return MuTFFErrorBadFormat;
    }

    switch (header.type) {
      /* case MuTFF_FOUR_C("vmhd"): */
      case 0x766d6864:
        fseek(fd, start_offset, SEEK_SET);
        return mutff_read_video_media_information_atom(fd, &out->video);
      /* case MuTFF_FOUR_C("smhd"): */
      case 0x736d6864:
        return mutff_read_sound_media_information_atom(fd, &out->sound);
      /* case MuTFF_FOUR_C("gmhd"): */
      case 0x676d6864:
        return mutff_read_base_media_information_atom(fd, &out->base);
      default:
        fseek(fd, header.size, SEEK_CUR);
    }
  }

  return MuTFFErrorBadFormat;
}

MuTFFError mutff_read_media_atom(FILE *fd, MuTFFMediaAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  MuTFFAtomHeader header;
  size_t offset;
  uint64_t size;
  uint32_t type;

  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("mdia")) {
    return MuTFFErrorBadFormat;
  }

  // read child atoms
  offset = 8;
  while (offset < size) {
    MuTFF_FIELD(mutff_peek_atom_header, &header);
    offset += header.size;
    if (offset > size) {
      return MuTFFErrorBadFormat;
    }

    switch (header.type) {
      /* case MuTFF_FOUR_C("mdhd"): */
      case 0x6d646864:
        MuTFF_FIELD(mutff_read_media_header_atom, &out->media_header);
        break;
      /* case MuTFF_FOUR_C("elng"): */
      case 0x656c6e67:
        MuTFF_FIELD(mutff_read_extended_language_tag_atom,
                    &out->extended_language_tag);
        break;
      /* case MuTFF_FOUR_C("hdlr"): */
      case 0x68646c72:
        MuTFF_FIELD(mutff_read_handler_reference_atom, &out->handler_reference);
        break;
      /* case MuTFF_FOUR_C("minf"): */
      case 0x6d696e66:
        MuTFF_FIELD(mutff_read_media_information_atom, &out->media_information);
        break;
      /* case MuTFF_FOUR_C("udta"): */
      case 0x75647461:
        MuTFF_FIELD(mutff_read_user_data_atom, &out->user_data);
        break;
      default:
        fseek(fd, header.size, SEEK_CUR);
        ret += header.size;
    }
  }

  return ret;
}

MuTFFError mutff_read_track_atom(FILE *fd, MuTFFTrackAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  MuTFFAtomHeader header;
  size_t offset;
  uint64_t size;
  uint32_t type;

  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("trak")) {
    return MuTFFErrorBadFormat;
  }

  // read child atoms
  offset = 8;
  while (offset < size) {
    MuTFF_FIELD(mutff_peek_atom_header, &header);
    offset += header.size;
    if (offset > size) {
      return MuTFFErrorBadFormat;
    }

    switch (header.type) {
      /* case MuTFF_FOUR_C("tkhd"): */
      case 0x746b6864:
        MuTFF_FIELD(mutff_read_track_header_atom, &out->track_header);
        break;
      /* case MuTFF_FOUR_C("tapt"): */
      case 0x74617074:
        MuTFF_FIELD(mutff_read_track_aperture_mode_dimensions_atom,
                    &out->track_aperture_mode_dimensions);
        break;
      /* case MuTFF_FOUR_C("clip"): */
      case 0x636c6970:
        MuTFF_FIELD(mutff_read_clipping_atom, &out->clipping);
        break;
      /* case MuTFF_FOUR_C("matt"): */
      case 0x6d617474:
        MuTFF_FIELD(mutff_read_track_matte_atom, &out->track_matte);
        break;
      /* case MuTFF_FOUR_C("edts"): */
      case 0x65647473:
        MuTFF_FIELD(mutff_read_edit_atom, &out->edit);
        break;
      /* case MuTFF_FOUR_C("tref"): */
      case 0x74726566:
        if ((err =
                 mutff_read_track_reference_atom(fd, &out->track_reference))) {
          return err;
        }
        break;
      /* case MuTFF_FOUR_C("txas"): */
      case 0x74786173:
        MuTFF_FIELD(mutff_read_track_exclude_from_autoselection_atom,
                    &out->track_exclude_from_autoselection);
        break;
      /* case MuTFF_FOUR_C("load"): */
      case 0x6c6f6164:
        MuTFF_FIELD(mutff_read_track_load_settings_atom,
                    &out->track_load_settings);
        break;
      /* case MuTFF_FOUR_C("imap"): */
      case 0x696d6170:
        if ((err =
                 mutff_read_track_input_map_atom(fd, &out->track_input_map))) {
          return err;
        }
        break;
      /* case MuTFF_FOUR_C("mdia"): */
      case 0x6d646961:
        MuTFF_FIELD(mutff_read_media_atom, &out->media);
        break;
      /* case MuTFF_FOUR_C("udta"): */
      case 0x75647461:
        MuTFF_FIELD(mutff_read_user_data_atom, &out->user_data);
        break;
      default:
        fseek(fd, header.size, SEEK_CUR);
        ret += header.size;
    }
  }

  return ret;
}

MuTFFError mutff_read_movie_atom(FILE *fd, MuTFFMovieAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  MuTFFAtomHeader header;
  size_t offset;
  *out = (MuTFFMovieAtom){0};
  uint64_t size;
  uint32_t type;

  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOUR_C("moov")) {
    return MuTFFErrorBadFormat;
  }

  // read child atoms
  offset = 8;
  while (offset < size) {
    if ((err = mutff_peek_atom_header(fd, &header)) < 0) {
      return err;
    }
    offset += header.size;
    if (offset > size) {
      return MuTFFErrorBadFormat;
    }

    switch (header.type) {
      /* case MuTFF_FOUR_C("mvhd"): */
      case 0x6d766864:
        MuTFF_FIELD(mutff_read_movie_header_atom, &out->movie_header);
        break;

      /* case MuTFF_FOUR_C("clip"): */
      case 0x636c6970:
        MuTFF_FIELD(mutff_read_clipping_atom, &out->clipping);
        break;

      /* case MuTFF_FOUR_C("trak"): */
      case 0x7472616b:
        if (out->track_count >= MuTFF_MAX_TRACK_ATOMS) {
          return MuTFFErrorBadFormat;
        }
        MuTFF_FIELD(mutff_read_track_atom, &out->track[out->track_count]);
        out->track_count++;
        break;

      /* case MuTFF_FOUR_C("udta"): */
      case 0x75647461:
        MuTFF_FIELD(mutff_read_user_data_atom, &out->user_data);
        break;

      /* case MuTFF_FOUR_C("ctab"): */
      case 0x63746162:
        MuTFF_FIELD(mutff_read_color_table_atom, &out->color_table);
        break;

      default:
        // unrecognised atom type - skip as per spec
        fseek(fd, header.size, SEEK_CUR);
        ret += header.size;
        break;
    }
  }

  return ret;
}

MuTFFError mutff_read_movie_file(FILE *fd, MuTFFMovieFile *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  MuTFFAtomHeader atom;
  out->file_type_compatibility_count = 0;
  out->movie_count = 0;
  out->movie_data_count = 0;
  out->free_count = 0;
  out->skip_count = 0;
  out->wide_count = 0;
  out->preview_count = 0;
  rewind(fd);

  while ((err = mutff_peek_atom_header(fd, &atom)) >= 0) {
    switch (atom.type) {
      /* case MuTFF_FOUR_C("ftyp"): */
      case 0x66747970:
        if (out->file_type_compatibility_count >=
            MuTFF_MAX_FILE_TYPE_COMPATIBILITY_ATOMS) {
          return MuTFFErrorOutOfMemory;
        }
        MuTFF_FIELD(
            mutff_read_file_type_compatibility_atom,
            &out->file_type_compatibility[out->file_type_compatibility_count]);
        out->file_type_compatibility_count++;

        break;

      /* case MuTFF_FOUR_C("moov"): */
      case 0x6d6f6f76:
        if (out->movie_count >= MuTFF_MAX_MOVIE_ATOMS) {
          return MuTFFErrorOutOfMemory;
        }
        MuTFF_FIELD(mutff_read_movie_atom, &out->movie[out->movie_count]);
        out->movie_count++;
        break;

      /* case MuTFF_FOUR_C("mdat"): */
      case 0x6d646174:
        if (out->movie_data_count >= MuTFF_MAX_MOVIE_DATA_ATOMS) {
          return MuTFFErrorOutOfMemory;
        }
        MuTFF_FIELD(mutff_read_movie_data_atom,
                    &out->movie_data[out->movie_data_count]);
        out->movie_data_count++;
        break;

      /* case MuTFF_FOUR_C("free"): */
      case 0x66726565:
        if (out->free_count >= MuTFF_MAX_FREE_ATOMS) {
          return MuTFFErrorOutOfMemory;
        }
        MuTFF_FIELD(mutff_read_free_atom, &out->free[out->free_count]);
        out->free_count++;
        break;

      /* case MuTFF_FOUR_C("skip"): */
      case 0x736b6970:
        if (out->skip_count >= MuTFF_MAX_SKIP_ATOMS) {
          return MuTFFErrorOutOfMemory;
        }
        MuTFF_FIELD(mutff_read_skip_atom, &out->skip[out->skip_count]);
        out->skip_count++;
        break;

      /* case MuTFF_FOUR_C("wide"): */
      case 0x77696465:
        if (out->wide_count >= MuTFF_MAX_WIDE_ATOMS) {
          return MuTFFErrorOutOfMemory;
        }
        MuTFF_FIELD(mutff_read_wide_atom, &out->wide[out->wide_count]);
        out->wide_count++;
        break;

      /* case MuTFF_FOUR_C("pnot"): */
      case 0x706e6f74:
        if (out->preview_count >= MuTFF_MAX_WIDE_ATOMS) {
          return MuTFFErrorOutOfMemory;
        }
        MuTFF_FIELD(mutff_read_preview_atom, &out->preview[out->preview_count]);
        out->preview_count++;
        break;

      default:
        // unsupported basic type - skip as per spec
        fseek(fd, atom.size, SEEK_CUR);
        ret += atom.size;
    }
  }

  return ret;
}

/* MuTFFError mutff_write_movie_file(FILE *fd, MuTFFMovieFile *in) { */
/*   mutff_write_file_type_compatibility(fd, &in->file_type_compatibility);
 */
/*   mutff_write_movie(fd, &in->movie); */
/* } */
