///
/// @file      mutff.c
/// @author    Frank Plowman <post@frankplowman.com>
/// @brief     MuTFF QuickTime file format library main source file
/// @copyright 2022 Frank Plowman
/// @license   This project is released under the GNU Public License Version 3.
///            For the terms of this license, see [LICENSE.md](LICENSE.md)
///

#include "mutff.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MuTFF_FIELD(func, ...)   \
  do {                           \
    err = func(fd, __VA_ARGS__); \
    if (mutff_is_error(err)) {   \
      return err;                \
    }                            \
    ret += (uint64_t)err;        \
  } while (0);

#define MuTFF_READ_CHILD(func, field, flag) \
  do {                                      \
    if (flag == true) {                     \
      return MuTFFErrorBadFormat;           \
    }                                       \
    MuTFF_FIELD(func, field);               \
    (flag) = true;                          \
  } while (0);

#define MuTFF_SEEK_CUR(offset)                \
  do {                                        \
    if (fseek(fd, (offset), SEEK_CUR) != 0) { \
      return MuTFFErrorIOError;               \
    }                                         \
    ret += (offset);                          \
  } while (0);

static inline bool mutff_is_error(MuTFFError err) { return (int)err < 0; }

// Convert a number from network (big) endian to host endian.
// These must be implemented here as newlib does not provide the
// standard library implementations ntohs & ntohl (arpa/inet.h).
//
// taken from:
// https://stackoverflow.com/questions/2100331/macro-definition-to-determine-big-endian-or-little-endian-machine
static uint16_t mutff_ntoh_16(unsigned char *no) {
  return ((uint16_t)no[0] << 8) | (uint16_t)no[1];
}

static void mutff_hton_16(unsigned char *dest, uint16_t n) {
  dest[0] = n >> 8;
  dest[1] = n;
}

static mutff_uint24_t mutff_ntoh_24(unsigned char *no) {
  return ((mutff_uint24_t)no[0] << 16) | ((mutff_uint24_t)no[1] << 8) |
         (mutff_uint24_t)no[2];
}

static void mutff_hton_24(unsigned char *dest, mutff_uint24_t n) {
  // note this is using implicit truncation
  dest[0] = n >> 16;
  dest[1] = n >> 8;
  dest[2] = n;
}

static uint32_t mutff_ntoh_32(unsigned char *no) {
  return ((uint32_t)no[0] << 24) | ((uint32_t)no[1] << 16) |
         ((uint32_t)no[2] << 8) | (uint32_t)no[3];
}

static void mutff_hton_32(unsigned char *dest, uint32_t n) {
  // note this is using implicit truncation
  dest[0] = n >> 24;
  dest[1] = n >> 16;
  dest[2] = n >> 8;
  dest[3] = n;
}

static uint64_t mutff_ntoh_64(unsigned char *no) {
  return ((uint64_t)no[0] << 56) | ((uint64_t)no[1] << 48) |
         ((uint64_t)no[2] << 40) | ((uint64_t)no[3] << 32) |
         ((uint64_t)no[4] << 24) | ((uint64_t)no[5] << 16) |
         ((uint64_t)no[6] << 8) | (uint64_t)no[7];
}

static void mutff_hton_64(unsigned char *dest, uint64_t n) {
  // note this is using implicit truncation
  dest[0] = n >> 56;
  dest[1] = n >> 48;
  dest[2] = n >> 40;
  dest[3] = n >> 32;
  dest[4] = n >> 24;
  dest[5] = n >> 16;
  dest[6] = n >> 8;
  dest[7] = n;
}

static MuTFFError mutff_read_u8(FILE *fd, uint8_t *data) {
  const size_t read = fread(data, 1, 1, fd);
  if (read != 1U) {
    if (feof(fd) != 0) {
      return MuTFFErrorEOF;
    } else {
      return MuTFFErrorIOError;
    }
  }
  return 1U;
}

static MuTFFError mutff_read_i8(FILE *fd, int8_t *dest) {
  MuTFFError ret;
  uint8_t twos;

  ret = mutff_read_u8(fd, &twos);
  if (mutff_is_error(ret)) {
    return ret;
  }
  // convert from twos complement to implementation-defined
  *dest = (twos & 0x7FU) - (twos & 0x80U);

  return ret;
}

static MuTFFError mutff_read_u16(FILE *fd, uint16_t *dest) {
  unsigned char data[2];
  const size_t read = fread(data, 2, 1, fd);
  if (read != 1U) {
    if (feof(fd) != 0) {
      return MuTFFErrorEOF;
    } else {
      return MuTFFErrorIOError;
    }
  }
  // Convert from network order (big-endian)
  // to host order (implementation-defined).
  *dest = mutff_ntoh_16(data);
  return 2U;
}

static MuTFFError mutff_read_i16(FILE *fd, int16_t *dest) {
  MuTFFError ret;
  uint16_t twos;

  ret = mutff_read_u16(fd, &twos);
  if (mutff_is_error(ret)) {
    return ret;
  }
  *dest = (twos & 0x7FFFU) - (twos & 0x8000U);

  return ret;
}

static MuTFFError mutff_read_u24(FILE *fd, mutff_uint24_t *dest) {
  unsigned char data[3];
  const size_t read = fread(data, 3, 1, fd);
  if (read != 1U) {
    if (feof(fd) != 0) {
      return MuTFFErrorEOF;
    } else {
      return MuTFFErrorIOError;
    }
  }
  *dest = mutff_ntoh_24(data);
  return 3U;
}

static MuTFFError mutff_read_u32(FILE *fd, uint32_t *dest) {
  unsigned char data[4];
  const size_t read = fread(data, 4, 1, fd);
  if (read != 1U) {
    if (feof(fd) != 0) {
      return MuTFFErrorEOF;
    } else {
      return MuTFFErrorIOError;
    }
  }
  *dest = mutff_ntoh_32(data);
  return 4U;
}

static MuTFFError mutff_read_i32(FILE *fd, int32_t *dest) {
  MuTFFError ret;
  uint32_t twos;

  ret = mutff_read_u32(fd, &twos);
  if (mutff_is_error(ret)) {
    return ret;
  }
  *dest = (twos & 0x7FFFFFFFU) - (twos & 0x80000000U);

  return ret;
}

static MuTFFError mutff_read_u64(FILE *fd, uint64_t *dest) {
  unsigned char data[8];
  const size_t read = fread(data, 8, 1, fd);
  if (read != 1U) {
    if (feof(fd) != 0) {
      return MuTFFErrorEOF;
    } else {
      return MuTFFErrorIOError;
    }
  }
  *dest = mutff_ntoh_64(data);
  return 8U;
}

static MuTFFError mutff_write_u8(FILE *fd, uint8_t data) {
  const size_t written = fwrite(&data, 1, 1, fd);
  if (written != 1U) {
    return MuTFFErrorIOError;
  }
  return 1U;
}

static MuTFFError mutff_write_i8(FILE *fd, int8_t n) {
  // ensure number is stored as two's complement
  return mutff_write_u8(fd, n >= 0 ? n : ~abs(n) + 1);
}

static MuTFFError mutff_write_u16(FILE *fd, uint16_t n) {
  unsigned char data[2];
  // convert number to network order
  mutff_hton_16(data, n);
  const size_t written = fwrite(&data, 2, 1, fd);
  if (written != 1U) {
    return MuTFFErrorIOError;
  }
  return 2U;
}

static inline MuTFFError mutff_write_i16(FILE *fd, int16_t n) {
  return mutff_write_u16(fd, n >= 0 ? n : ~abs(n) + 1);
}

static MuTFFError mutff_write_u24(FILE *fd, mutff_uint24_t n) {
  unsigned char data[3];
  mutff_hton_24(data, n);
  const size_t written = fwrite(&data, 3, 1, fd);
  if (written != 1U) {
    return MuTFFErrorIOError;
  }
  return 3U;
}

static MuTFFError mutff_write_u32(FILE *fd, uint32_t n) {
  unsigned char data[4];
  mutff_hton_32(data, n);
  const size_t written = fwrite(&data, 4, 1, fd);
  if (written != 1U) {
    return MuTFFErrorIOError;
  }
  return 4U;
}

static inline MuTFFError mutff_write_i32(FILE *fd, int32_t n) {
  return mutff_write_u32(fd, n = n >= 0 ? n : ~abs(n) + 1);
}

static MuTFFError mutff_write_u64(FILE *fd, uint32_t n) {
  unsigned char data[8];
  mutff_hton_64(data, n);
  const size_t written = fwrite(&data, 8, 1, fd);
  if (written != 1U) {
    return MuTFFErrorIOError;
  }
  return 8U;
}

static MuTFFError mutff_read_q8_8(FILE *fd, mutff_q8_8_t *data) {
  uint64_t ret = 0;
  ret = mutff_read_i8(fd, &data->integral);
  if (mutff_is_error(ret)) {
    return ret;
  }
  ret = mutff_read_u8(fd, &data->fractional);
  if (mutff_is_error(ret)) {
    return ret;
  }
  return 2U;
}

static MuTFFError mutff_write_q8_8(FILE *fd, mutff_q8_8_t data) {
  uint64_t ret = 0;
  ret = mutff_write_i8(fd, data.integral);
  if (mutff_is_error(ret)) {
    return ret;
  }
  ret = mutff_write_u8(fd, data.fractional);
  if (mutff_is_error(ret)) {
    return ret;
  }
  return 2U;
}

static MuTFFError mutff_read_q16_16(FILE *fd, mutff_q16_16_t *data) {
  uint64_t ret = 0;
  ret = mutff_read_i16(fd, &data->integral);
  if (mutff_is_error(ret)) {
    return ret;
  }
  ret = mutff_read_u16(fd, &data->fractional);
  if (mutff_is_error(ret)) {
    return ret;
  }
  return 4U;
}

static MuTFFError mutff_write_q16_16(FILE *fd, mutff_q16_16_t data) {
  uint64_t ret = 0;
  ret = mutff_write_i16(fd, data.integral);
  if (mutff_is_error(ret)) {
    return ret;
  }
  ret = mutff_write_u16(fd, data.fractional);
  if (mutff_is_error(ret)) {
    return ret;
  }
  return 4U;
}

// return the size of an atom including its header, given the size of the data
// in it
static inline uint64_t mutff_atom_size(uint64_t data_size) {
  return data_size + 8U <= UINT32_MAX ? data_size + 8U : data_size + 16U;
}

// return the size of the data in an atom, given the size of the entire atom
// including its header in it
static inline uint64_t mutff_data_size(uint64_t atom_size) {
  return atom_size <= UINT32_MAX ? atom_size - 8U : atom_size - 16U;
}

static MuTFFError mutff_read_header(FILE *fd, uint64_t *size, uint32_t *type) {
  MuTFFError err;
  uint64_t ret = 0;
  uint32_t short_size;

  MuTFF_FIELD(mutff_read_u32, &short_size);
  MuTFF_FIELD(mutff_read_u32, type);
  if (short_size == 1U) {
    MuTFF_FIELD(mutff_read_u64, size);
  } else {
    *size = short_size;
  }

  return ret;
}

static MuTFFError mutff_peek_atom_header(FILE *fd, uint64_t *size,
                                         uint32_t *type) {
  MuTFFError err;
  uint64_t ret = 0;

  MuTFF_FIELD(mutff_read_header, size, type);
  MuTFF_SEEK_CUR(-ret);

  return ret;
}

static MuTFFError mutff_write_header(FILE *fd, uint64_t size, uint32_t type) {
  MuTFFError err;
  uint64_t ret = 0;

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

MuTFFError mutff_read_quickdraw_rect(FILE *fd, MuTFFQuickDrawRect *out) {
  MuTFFError err;
  uint64_t ret = 0;

  MuTFF_FIELD(mutff_read_u16, &out->top);
  MuTFF_FIELD(mutff_read_u16, &out->left);
  MuTFF_FIELD(mutff_read_u16, &out->bottom);
  MuTFF_FIELD(mutff_read_u16, &out->right);

  return ret;
}

MuTFFError mutff_write_quickdraw_rect(FILE *fd, const MuTFFQuickDrawRect *in) {
  MuTFFError err;
  uint64_t ret = 0;

  MuTFF_FIELD(mutff_write_u16, in->top);
  MuTFF_FIELD(mutff_write_u16, in->left);
  MuTFF_FIELD(mutff_write_u16, in->bottom);
  MuTFF_FIELD(mutff_write_u16, in->right);

  return ret;
}

MuTFFError mutff_read_quickdraw_region(FILE *fd, MuTFFQuickDrawRegion *out) {
  MuTFFError err;
  uint64_t ret = 0;

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
  uint64_t ret = 0;

  MuTFF_FIELD(mutff_write_u16, in->size);
  MuTFF_FIELD(mutff_write_quickdraw_rect, &in->rect);
  for (uint16_t i = 0; i < in->size - 10U; ++i) {
    MuTFF_FIELD(mutff_write_u8, in->data[i]);
  }

  return ret;
}

MuTFFError mutff_read_file_type_atom(FILE *fd, MuTFFFileTypeAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;

  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('f', 't', 'y', 'p')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u32, &out->major_brand);
  MuTFF_FIELD(mutff_read_u32, &out->minor_version);

  // read variable-length data
  out->compatible_brands_count = (size - ret) / 4U;
  if (out->compatible_brands_count > MuTFF_MAX_COMPATIBLE_BRANDS) {
    return MuTFFErrorOutOfMemory;
  }
  for (size_t i = 0; i < out->compatible_brands_count; ++i) {
    MuTFF_FIELD(mutff_read_u32, &out->compatible_brands[i]);
  }
  MuTFF_SEEK_CUR(size - ret);

  return ret;
}

static inline MuTFFError mutff_file_type_atom_size(
    uint64_t *out, const MuTFFFileTypeAtom *atom) {
  *out = mutff_atom_size(8U + 4U * atom->compatible_brands_count);
  return 0;
}

MuTFFError mutff_write_file_type_atom(FILE *fd, const MuTFFFileTypeAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_file_type_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }

  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('f', 't', 'y', 'p'));
  MuTFF_FIELD(mutff_write_u32, in->major_brand);
  MuTFF_FIELD(mutff_write_u32, in->minor_version);
  for (size_t i = 0; i < in->compatible_brands_count; ++i) {
    MuTFF_FIELD(mutff_write_u32, in->compatible_brands[i]);
  }

  return ret;
}

MuTFFError mutff_read_movie_data_atom(FILE *fd, MuTFFMovieDataAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;

  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('m', 'd', 'a', 't')) {
    return MuTFFErrorBadFormat;
  }
  out->data_size = mutff_data_size(size);
  MuTFF_SEEK_CUR(size - ret);
  return ret;
}

static inline MuTFFError mutff_movie_data_atom_size(
    uint64_t *out, const MuTFFMovieDataAtom *atom) {
  *out = mutff_atom_size(atom->data_size);
  return 0;
}

MuTFFError mutff_write_movie_data_atom(FILE *fd, const MuTFFMovieDataAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_movie_data_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('m', 'd', 'a', 't'));
  for (uint64_t i = 0; i < in->data_size; ++i) {
    MuTFF_FIELD(mutff_write_u8, 0);
  }
  return ret;
}

MuTFFError mutff_read_free_atom(FILE *fd, MuTFFFreeAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('f', 'r', 'e', 'e')) {
    return MuTFFErrorBadFormat;
  }
  out->atom_size = size;
  MuTFF_SEEK_CUR(size - ret);
  return ret;
}

static inline MuTFFError mutff_free_atom_size(uint64_t *out,
                                              const MuTFFFreeAtom *atom) {
  *out = atom->atom_size;
  return 0;
}

MuTFFError mutff_write_free_atom(FILE *fd, const MuTFFFreeAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_free_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('f', 'r', 'e', 'e'));
  for (uint64_t i = 0; i < mutff_data_size(in->atom_size); ++i) {
    MuTFF_FIELD(mutff_write_u8, 0);
  }
  return ret;
}

MuTFFError mutff_read_skip_atom(FILE *fd, MuTFFSkipAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('s', 'k', 'i', 'p')) {
    return MuTFFErrorBadFormat;
  }
  out->atom_size = size;
  MuTFF_SEEK_CUR(size - ret);
  return ret;
}

static inline MuTFFError mutff_skip_atom_size(uint64_t *out,
                                              const MuTFFSkipAtom *atom) {
  *out = atom->atom_size;
  return 0;
}

MuTFFError mutff_write_skip_atom(FILE *fd, const MuTFFSkipAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_skip_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('s', 'k', 'i', 'p'));
  for (uint64_t i = 0; i < mutff_data_size(in->atom_size); ++i) {
    MuTFF_FIELD(mutff_write_u8, 0);
  }
  return ret;
}

MuTFFError mutff_read_wide_atom(FILE *fd, MuTFFWideAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('w', 'i', 'd', 'e')) {
    return MuTFFErrorBadFormat;
  }
  out->atom_size = size;
  MuTFF_SEEK_CUR(size - ret);
  return ret;
}

static inline MuTFFError mutff_wide_atom_size(uint64_t *out,
                                              const MuTFFWideAtom *atom) {
  *out = atom->atom_size;
  return 0;
}

MuTFFError mutff_write_wide_atom(FILE *fd, const MuTFFWideAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_wide_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('w', 'i', 'd', 'e'));
  for (uint64_t i = 0; i < mutff_data_size(in->atom_size); ++i) {
    MuTFF_FIELD(mutff_write_u8, 0);
  }
  return ret;
}

MuTFFError mutff_read_preview_atom(FILE *fd, MuTFFPreviewAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('p', 'n', 'o', 't')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u32, &out->modification_time);
  MuTFF_FIELD(mutff_read_u16, &out->version);
  MuTFF_FIELD(mutff_read_u32, &out->atom_type);
  MuTFF_FIELD(mutff_read_u16, &out->atom_index);
  MuTFF_SEEK_CUR(size - ret);
  return ret;
}

static inline MuTFFError mutff_preview_atom_size(uint64_t *out,
                                                 const MuTFFPreviewAtom *atom) {
  *out = mutff_atom_size(12);
  return 0;
}

MuTFFError mutff_write_preview_atom(FILE *fd, const MuTFFPreviewAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_preview_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('p', 'n', 'o', 't'));
  MuTFF_FIELD(mutff_write_u32, in->modification_time);
  MuTFF_FIELD(mutff_write_u16, in->version);
  MuTFF_FIELD(mutff_write_u32, in->atom_type);
  MuTFF_FIELD(mutff_write_u16, in->atom_index);
  return ret;
}

MuTFFError mutff_read_movie_header_atom(FILE *fd, MuTFFMovieHeaderAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('m', 'v', 'h', 'd')) {
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
  MuTFF_SEEK_CUR(10U);
  for (size_t j = 0; j < 3U; ++j) {
    for (size_t i = 0; i < 3U; ++i) {
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
  MuTFF_SEEK_CUR(size - ret);
  return ret;
}

static inline MuTFFError mutff_movie_header_atom_size(
    uint64_t *out, const MuTFFMovieHeaderAtom *atom) {
  *out = mutff_atom_size(100);
  return 0;
}

MuTFFError mutff_write_movie_header_atom(FILE *fd,
                                         const MuTFFMovieHeaderAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_movie_header_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('m', 'v', 'h', 'd'));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_u32, in->creation_time);
  MuTFF_FIELD(mutff_write_u32, in->modification_time);
  MuTFF_FIELD(mutff_write_u32, in->time_scale);
  MuTFF_FIELD(mutff_write_u32, in->duration);
  MuTFF_FIELD(mutff_write_q16_16, in->preferred_rate);
  MuTFF_FIELD(mutff_write_q8_8, in->preferred_volume);
  for (size_t i = 0; i < 10U; ++i) {
    MuTFF_FIELD(mutff_write_u8, 0);
  }
  for (size_t j = 0; j < 3U; ++j) {
    for (size_t i = 0; i < 3U; ++i) {
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
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;

  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('c', 'r', 'g', 'n')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_quickdraw_region, &out->region);
  MuTFF_SEEK_CUR(size - ret);

  return ret;
}

static inline MuTFFError mutff_clipping_region_atom_size(
    uint64_t *out, const MuTFFClippingRegionAtom *atom) {
  *out = mutff_atom_size(atom->region.size);
  return 0;
}

MuTFFError mutff_write_clipping_region_atom(FILE *fd,
                                            const MuTFFClippingRegionAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_clipping_region_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('c', 'r', 'g', 'n'));
  MuTFF_FIELD(mutff_write_quickdraw_region, &in->region);
  return ret;
}

MuTFFError mutff_read_clipping_atom(FILE *fd, MuTFFClippingAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  bool clipping_region_present = false;

  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('c', 'l', 'i', 'p')) {
    return MuTFFErrorBadFormat;
  }
  uint64_t child_size;
  uint32_t child_type;
  while (ret < size) {
    MuTFF_FIELD(mutff_peek_atom_header, &child_size, &child_type);
    if (size == 0U) {
      return MuTFFErrorBadFormat;
    }
    if (ret + child_size > size) {
      return MuTFFErrorBadFormat;
    }
    if (child_type == MuTFF_FOURCC('c', 'r', 'g', 'n')) {
      MuTFF_READ_CHILD(mutff_read_clipping_region_atom, &out->clipping_region,
                       clipping_region_present);
    } else {
      MuTFF_SEEK_CUR(child_size);
    }
  }

  if (!clipping_region_present) {
    return MuTFFErrorBadFormat;
  }

  return ret;
}

static inline MuTFFError mutff_clipping_atom_size(
    uint64_t *out, const MuTFFClippingAtom *atom) {
  uint64_t size;
  const MuTFFError err =
      mutff_clipping_region_atom_size(&size, &atom->clipping_region);
  if (mutff_is_error(err)) {
    return err;
  }
  *out = mutff_atom_size(size);
  return 0;
}

MuTFFError mutff_write_clipping_atom(FILE *fd, const MuTFFClippingAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_clipping_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('c', 'l', 'i', 'p'));
  MuTFF_FIELD(mutff_write_clipping_region_atom, &in->clipping_region);
  return ret;
}

MuTFFError mutff_read_color_table_atom(FILE *fd, MuTFFColorTableAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('c', 't', 'a', 'b')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u32, &out->color_table_seed);
  MuTFF_FIELD(mutff_read_u16, &out->color_table_flags);
  MuTFF_FIELD(mutff_read_u16, &out->color_table_size);

  // read color array
  const size_t array_size = (out->color_table_size + 1U) * 8U;
  if (array_size != mutff_data_size(size) - 8U) {
    return MuTFFErrorBadFormat;
  }
  for (size_t i = 0; i <= out->color_table_size; ++i) {
    for (size_t j = 0; j < 4U; ++j) {
      MuTFF_FIELD(mutff_read_u16, &out->color_array[i][j]);
    }
  }
  MuTFF_SEEK_CUR(size - ret);

  return ret;
}

static inline MuTFFError mutff_color_table_atom_size(
    uint64_t *out, const MuTFFColorTableAtom *atom) {
  *out = mutff_atom_size(8U + (atom->color_table_size + 1U) * 8U);
  return 0;
}

MuTFFError mutff_write_color_table_atom(FILE *fd,
                                        const MuTFFColorTableAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_color_table_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('c', 't', 'a', 'b'));
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
  uint64_t ret = 0;
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

static inline MuTFFError mutff_user_data_list_entry_size(
    uint64_t *out, const MuTFFUserDataListEntry *entry) {
  *out = mutff_atom_size(entry->data_size);
  return 0;
}

MuTFFError mutff_write_user_data_list_entry(FILE *fd,
                                            const MuTFFUserDataListEntry *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_user_data_list_entry_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, in->type);
  for (uint32_t i = 0; i < in->data_size; ++i) {
    MuTFF_FIELD(mutff_write_u8, in->data[i]);
  }
  return ret;
}

MuTFFError mutff_read_user_data_atom(FILE *fd, MuTFFUserDataAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;

  // read data
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('u', 'd', 't', 'a')) {
    return MuTFFErrorBadFormat;
  }

  // read children
  size_t i = 0;
  uint64_t child_size;
  uint32_t child_type;
  while (ret < size) {
    if (i >= MuTFF_MAX_USER_DATA_ITEMS) {
      return MuTFFErrorOutOfMemory;
    }
    MuTFF_FIELD(mutff_peek_atom_header, &child_size, &child_type);
    if (size == 0U) {
      return MuTFFErrorBadFormat;
    }
    if (ret + child_size > size) {
      return MuTFFErrorBadFormat;
    }
    MuTFF_FIELD(mutff_read_user_data_list_entry, &out->user_data_list[i]);

    i++;
  }
  out->list_entries = i;

  return ret;
}

static inline MuTFFError mutff_user_data_atom_size(
    uint64_t *out, const MuTFFUserDataAtom *atom) {
  MuTFFError err;
  uint64_t size = 0;
  for (size_t i = 0; i < atom->list_entries; ++i) {
    uint64_t user_data_size;
    err = mutff_user_data_list_entry_size(&user_data_size,
                                          &atom->user_data_list[i]);
    if (mutff_is_error(err)) {
      return err;
    }
    size += user_data_size;
  }
  *out = mutff_atom_size(size);
  return 0;
}

MuTFFError mutff_write_user_data_atom(FILE *fd, const MuTFFUserDataAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_user_data_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('u', 'd', 't', 'a'));
  for (size_t i = 0; i < in->list_entries; ++i) {
    MuTFF_FIELD(mutff_write_user_data_list_entry, &in->user_data_list[i]);
  }
  return ret;
}

MuTFFError mutff_read_track_header_atom(FILE *fd, MuTFFTrackHeaderAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('t', 'k', 'h', 'd')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_u32, &out->creation_time);
  MuTFF_FIELD(mutff_read_u32, &out->modification_time);
  MuTFF_FIELD(mutff_read_u32, &out->track_id);
  MuTFF_SEEK_CUR(4U);
  MuTFF_FIELD(mutff_read_u32, &out->duration);
  MuTFF_SEEK_CUR(8U);
  MuTFF_FIELD(mutff_read_u16, &out->layer);
  MuTFF_FIELD(mutff_read_u16, &out->alternate_group);
  MuTFF_FIELD(mutff_read_q8_8, &out->volume);
  MuTFF_SEEK_CUR(2U);
  for (size_t j = 0; j < 3U; ++j) {
    for (size_t i = 0; i < 3U; ++i) {
      MuTFF_FIELD(mutff_read_u32, &out->matrix_structure[j][i]);
    }
  }
  MuTFF_FIELD(mutff_read_q16_16, &out->track_width);
  MuTFF_FIELD(mutff_read_q16_16, &out->track_height);
  MuTFF_SEEK_CUR(size - ret);
  return ret;
}

static inline MuTFFError mutff_track_header_atom_size(
    uint64_t *out, const MuTFFTrackHeaderAtom *atom) {
  *out = mutff_atom_size(84);
  return 0;
}

MuTFFError mutff_write_track_header_atom(FILE *fd,
                                         const MuTFFTrackHeaderAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_track_header_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('t', 'k', 'h', 'd'));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_u32, in->creation_time);
  MuTFF_FIELD(mutff_write_u32, in->modification_time);
  MuTFF_FIELD(mutff_write_u32, in->track_id);
  for (size_t i = 0; i < 4U; ++i) {
    MuTFF_FIELD(mutff_write_u8, 0);
  }
  MuTFF_FIELD(mutff_write_u32, in->duration);
  for (size_t i = 0; i < 8U; ++i) {
    MuTFF_FIELD(mutff_write_u8, 0);
  }
  MuTFF_FIELD(mutff_write_u16, in->layer);
  MuTFF_FIELD(mutff_write_u16, in->alternate_group);
  MuTFF_FIELD(mutff_write_q8_8, in->volume);
  for (size_t i = 0; i < 2U; ++i) {
    MuTFF_FIELD(mutff_write_u8, 0);
  }
  for (size_t j = 0; j < 3U; ++j) {
    for (size_t i = 0; i < 3U; ++i) {
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
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('c', 'l', 'e', 'f')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_q16_16, &out->width);
  MuTFF_FIELD(mutff_read_q16_16, &out->height);
  return ret;
}

static inline MuTFFError mutff_track_clean_aperture_dimensions_atom_size(
    uint64_t *out, const MuTFFTrackCleanApertureDimensionsAtom *atom) {
  *out = mutff_atom_size(12);
  return 0;
}

MuTFFError mutff_write_track_clean_aperture_dimensions_atom(
    FILE *fd, const MuTFFTrackCleanApertureDimensionsAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_track_clean_aperture_dimensions_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('c', 'l', 'e', 'f'));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_q16_16, in->width);
  MuTFF_FIELD(mutff_write_q16_16, in->height);
  return ret;
}

MuTFFError mutff_read_track_production_aperture_dimensions_atom(
    FILE *fd, MuTFFTrackProductionApertureDimensionsAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('p', 'r', 'o', 'f')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_q16_16, &out->width);
  MuTFF_FIELD(mutff_read_q16_16, &out->height);
  return ret;
}

static inline MuTFFError mutff_track_production_aperture_dimensions_atom_size(
    uint64_t *out, const MuTFFTrackProductionApertureDimensionsAtom *atom) {
  *out = mutff_atom_size(12);
  return 0;
}

MuTFFError mutff_write_track_production_aperture_dimensions_atom(
    FILE *fd, const MuTFFTrackProductionApertureDimensionsAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_track_production_aperture_dimensions_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('p', 'r', 'o', 'f'));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_q16_16, in->width);
  MuTFF_FIELD(mutff_write_q16_16, in->height);
  return ret;
}

MuTFFError mutff_read_track_encoded_pixels_dimensions_atom(
    FILE *fd, MuTFFTrackEncodedPixelsDimensionsAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('e', 'n', 'o', 'f')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_q16_16, &out->width);
  MuTFF_FIELD(mutff_read_q16_16, &out->height);
  return ret;
}

static inline MuTFFError mutff_track_encoded_pixels_atom_size(
    uint64_t *out, const MuTFFTrackEncodedPixelsDimensionsAtom *atom) {
  *out = mutff_atom_size(12);
  return 0;
}

MuTFFError mutff_write_track_encoded_pixels_dimensions_atom(
    FILE *fd, const MuTFFTrackEncodedPixelsDimensionsAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_track_encoded_pixels_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('e', 'n', 'o', 'f'));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_q16_16, in->width);
  MuTFF_FIELD(mutff_write_q16_16, in->height);
  return ret;
}

MuTFFError mutff_read_track_aperture_mode_dimensions_atom(
    FILE *fd, MuTFFTrackApertureModeDimensionsAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  bool track_clean_aperture_dimensions_present = false;
  bool track_production_aperture_dimensions_present = false;
  bool track_encoded_pixels_dimensions_present = false;

  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('t', 'a', 'p', 't')) {
    return MuTFFErrorBadFormat;
  }

  // read children
  uint64_t child_size;
  uint32_t child_type;
  while (ret < size) {
    MuTFF_FIELD(mutff_peek_atom_header, &child_size, &child_type);
    if (size == 0U) {
      return MuTFFErrorBadFormat;
    }
    if (ret + child_size > size) {
      return MuTFFErrorBadFormat;
    }

    switch (child_type) {
      case MuTFF_FOURCC('c', 'l', 'e', 'f'):
        MuTFF_READ_CHILD(mutff_read_track_clean_aperture_dimensions_atom,
                         &out->track_clean_aperture_dimensions,
                         track_clean_aperture_dimensions_present);
        track_clean_aperture_dimensions_present = true;
        break;
      case MuTFF_FOURCC('p', 'r', 'o', 'f'):
        MuTFF_READ_CHILD(mutff_read_track_production_aperture_dimensions_atom,
                         &out->track_production_aperture_dimensions,
                         track_production_aperture_dimensions_present);
        track_production_aperture_dimensions_present = true;
        break;
      case MuTFF_FOURCC('e', 'n', 'o', 'f'):
        MuTFF_READ_CHILD(mutff_read_track_encoded_pixels_dimensions_atom,
                         &out->track_encoded_pixels_dimensions,
                         track_encoded_pixels_dimensions_present);
        break;
      default:
        // Unrecognised atom type, skip atom
        MuTFF_SEEK_CUR(child_size);
        break;
    }
  }

  if (!track_clean_aperture_dimensions_present ||
      !track_production_aperture_dimensions_present ||
      !track_encoded_pixels_dimensions_present) {
    return MuTFFErrorBadFormat;
  }

  return ret;
}

static inline MuTFFError mutff_track_aperture_mode_dimensions_atom_size(
    uint64_t *out, const MuTFFTrackApertureModeDimensionsAtom *atom) {
  *out = mutff_atom_size(60);
  return 0;
}

MuTFFError mutff_write_track_aperture_mode_dimensions_atom(
    FILE *fd, const MuTFFTrackApertureModeDimensionsAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_track_aperture_mode_dimensions_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('t', 'a', 'p', 't'));
  MuTFF_FIELD(mutff_write_track_clean_aperture_dimensions_atom,
              &in->track_clean_aperture_dimensions);
  MuTFF_FIELD(mutff_write_track_production_aperture_dimensions_atom,
              &in->track_production_aperture_dimensions);
  MuTFF_FIELD(mutff_write_track_encoded_pixels_dimensions_atom,
              &in->track_encoded_pixels_dimensions);
  return ret;
}

MuTFFError mutff_read_sample_description(FILE *fd,
                                         MuTFFSampleDescription *out) {
  MuTFFError err;
  uint64_t ret = 0;
  MuTFF_FIELD(mutff_read_u32, &out->size);
  MuTFF_FIELD(mutff_read_u32, &out->data_format);
  MuTFF_SEEK_CUR(6U);
  MuTFF_FIELD(mutff_read_u16, &out->data_reference_index);
  const uint32_t data_size = mutff_data_size(out->size) - 8U;
  for (uint32_t i = 0; i < data_size; ++i) {
    MuTFF_FIELD(mutff_read_u8, (uint8_t *)&out->additional_data[i]);
  }
  return ret;
}

MuTFFError mutff_write_sample_description(FILE *fd,
                                          const MuTFFSampleDescription *in) {
  MuTFFError err;
  uint64_t ret = 0;
  MuTFF_FIELD(mutff_write_u32, in->size);
  MuTFF_FIELD(mutff_write_u32, in->data_format);
  for (size_t i = 0; i < 6U; ++i) {
    MuTFF_FIELD(mutff_write_u8, 0);
  }
  MuTFF_FIELD(mutff_write_u16, in->data_reference_index);
  const size_t data_size = in->size - 16U;
  for (size_t i = 0; i < data_size; ++i) {
    MuTFF_FIELD(mutff_write_u8, in->additional_data[i]);
  }
  return ret;
}

MuTFFError mutff_read_compressed_matte_atom(FILE *fd,
                                            MuTFFCompressedMatteAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('k', 'm', 'a', 't')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);

  // read sample description
  MuTFF_FIELD(mutff_read_sample_description,
              &out->matte_image_description_structure);

  // read matte data
  out->matte_data_len =
      size - 12U - out->matte_image_description_structure.size;
  for (uint32_t i = 0; i < out->matte_data_len; ++i) {
    MuTFF_FIELD(mutff_read_u8, (uint8_t *)&out->matte_data[i]);
  }

  return ret;
}

static inline MuTFFError mutff_compressed_matte_atom_size(
    uint64_t *out, const MuTFFCompressedMatteAtom *atom) {
  *out = mutff_atom_size(4U + atom->matte_image_description_structure.size +
                         atom->matte_data_len);
  return 0;
}

MuTFFError mutff_write_compressed_matte_atom(
    FILE *fd, const MuTFFCompressedMatteAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_compressed_matte_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('k', 'm', 'a', 't'));
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
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  bool compressed_matte_atom_present = false;

  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('m', 'a', 't', 't')) {
    return MuTFFErrorBadFormat;
  }

  uint64_t child_size;
  uint32_t child_type;
  while (ret < size) {
    MuTFF_FIELD(mutff_peek_atom_header, &child_size, &child_type);
    if (size == 0U) {
      return MuTFFErrorBadFormat;
    }
    if (ret + child_size > size) {
      return MuTFFErrorBadFormat;
    }
    if (child_type == MuTFF_FOURCC('k', 'm', 'a', 't')) {
      MuTFF_READ_CHILD(mutff_read_compressed_matte_atom,
                       &out->compressed_matte_atom,
                       compressed_matte_atom_present);
    } else {
      MuTFF_SEEK_CUR(child_size);
    }
  }

  return ret;
}

static inline MuTFFError mutff_track_matte_atom_size(
    uint64_t *out, const MuTFFTrackMatteAtom *atom) {
  uint64_t size;
  const MuTFFError err =
      mutff_compressed_matte_atom_size(&size, &atom->compressed_matte_atom);
  if (mutff_is_error(err)) {
    return err;
  }
  *out = mutff_atom_size(size);
  return 0;
}

MuTFFError mutff_write_track_matte_atom(FILE *fd,
                                        const MuTFFTrackMatteAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_track_matte_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('m', 'a', 't', 't'));
  MuTFF_FIELD(mutff_write_compressed_matte_atom, &in->compressed_matte_atom);
  return ret;
}

MuTFFError mutff_read_edit_list_entry(FILE *fd, MuTFFEditListEntry *out) {
  MuTFFError err;
  uint64_t ret = 0;
  MuTFF_FIELD(mutff_read_u32, &out->track_duration);
  MuTFF_FIELD(mutff_read_u32, &out->media_time);
  MuTFF_FIELD(mutff_read_q16_16, &out->media_rate);
  return ret;
}

MuTFFError mutff_write_edit_list_entry(FILE *fd, const MuTFFEditListEntry *in) {
  MuTFFError err;
  uint64_t ret = 0;
  MuTFF_FIELD(mutff_write_u32, in->track_duration);
  MuTFF_FIELD(mutff_write_u32, in->media_time);
  MuTFF_FIELD(mutff_write_q16_16, in->media_rate);
  return ret;
}

MuTFFError mutff_read_edit_list_atom(FILE *fd, MuTFFEditListAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('e', 'l', 's', 't')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_u32, &out->number_of_entries);

  // read edit list table
  if (out->number_of_entries > MuTFF_MAX_EDIT_LIST_ENTRIES) {
    return MuTFFErrorOutOfMemory;
  }
  const size_t edit_list_table_size = size - 16U;
  if (edit_list_table_size != out->number_of_entries * 12U) {
    return MuTFFErrorBadFormat;
  }
  for (size_t i = 0; i < out->number_of_entries; ++i) {
    MuTFF_FIELD(mutff_read_edit_list_entry, &out->edit_list_table[i]);
  }

  return ret;
}

static inline MuTFFError mutff_edit_list_atom_size(
    uint64_t *out, const MuTFFEditListAtom *atom) {
  *out = mutff_atom_size(8U + atom->number_of_entries * 12U);
  return 0;
}

MuTFFError mutff_write_edit_list_atom(FILE *fd, const MuTFFEditListAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_edit_list_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('e', 'l', 's', 't'));
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
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  bool edit_list_present = false;

  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('e', 'd', 't', 's')) {
    return MuTFFErrorBadFormat;
  }

  uint64_t child_size;
  uint32_t child_type;
  while (ret < size) {
    MuTFF_FIELD(mutff_peek_atom_header, &child_size, &child_type);
    if (size == 0U) {
      return MuTFFErrorBadFormat;
    }
    if (ret + child_size > size) {
      return err;
    }
    if (child_type == MuTFF_FOURCC('e', 'l', 's', 't')) {
      MuTFF_READ_CHILD(mutff_read_edit_list_atom, &out->edit_list_atom,
                       edit_list_present);
    } else {
      MuTFF_SEEK_CUR(child_size);
    }
  }

  return ret;
}

static inline MuTFFError mutff_edit_atom_size(uint64_t *out,
                                              const MuTFFEditAtom *atom) {
  uint64_t size;
  const MuTFFError err =
      mutff_edit_list_atom_size(&size, &atom->edit_list_atom);
  if (mutff_is_error(err)) {
    return err;
  }
  *out = mutff_atom_size(size);
  return 0;
}

MuTFFError mutff_write_edit_atom(FILE *fd, const MuTFFEditAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_edit_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('e', 'd', 't', 's'));
  MuTFF_FIELD(mutff_write_edit_list_atom, &in->edit_list_atom);
  return ret;
}

MuTFFError mutff_read_track_reference_type_atom(
    FILE *fd, MuTFFTrackReferenceTypeAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  MuTFF_FIELD(mutff_read_header, &size, &out->type);

  // read track references
  if (mutff_data_size(size) % 4U != 0U) {
    return MuTFFErrorBadFormat;
  }
  out->track_id_count = mutff_data_size(size) / 4U;
  if (out->track_id_count > MuTFF_MAX_TRACK_REFERENCE_TYPE_TRACK_IDS) {
    return MuTFFErrorOutOfMemory;
  }
  for (unsigned int i = 0; i < out->track_id_count; ++i) {
    MuTFF_FIELD(mutff_read_u32, &out->track_ids[i]);
  }

  return ret;
}

static inline MuTFFError mutff_track_reference_type_atom_size(
    uint64_t *out, const MuTFFTrackReferenceTypeAtom *atom) {
  *out = mutff_atom_size(4U * atom->track_id_count);
  return 0;
}

MuTFFError mutff_write_track_reference_type_atom(
    FILE *fd, const MuTFFTrackReferenceTypeAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_track_reference_type_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, in->type);
  for (size_t i = 0; i < in->track_id_count; ++i) {
    MuTFF_FIELD(mutff_write_u32, in->track_ids[i]);
  }
  return ret;
}

MuTFFError mutff_read_track_reference_atom(FILE *fd,
                                           MuTFFTrackReferenceAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('t', 'r', 'e', 'f')) {
    return MuTFFErrorBadFormat;
  }

  // read children
  size_t i = 0;
  uint64_t child_size;
  uint32_t child_type;
  while (ret < size) {
    if (i >= MuTFF_MAX_TRACK_REFERENCE_TYPE_ATOMS) {
      return MuTFFErrorOutOfMemory;
    }
    MuTFF_FIELD(mutff_peek_atom_header, &child_size, &child_type);
    if (size == 0U) {
      return MuTFFErrorBadFormat;
    }
    if (ret + child_size > size) {
      return MuTFFErrorBadFormat;
    }
    MuTFF_FIELD(mutff_read_track_reference_type_atom,
                &out->track_reference_type[i]);
    i++;
  }
  out->track_reference_type_count = i;

  return ret;
}

static inline MuTFFError mutff_track_reference_atom_size(
    uint64_t *out, const MuTFFTrackReferenceAtom *atom) {
  MuTFFError err;
  uint64_t size = 0;
  for (size_t i = 0; i < atom->track_reference_type_count; ++i) {
    uint64_t reference_size;
    err = mutff_track_reference_type_atom_size(&reference_size,
                                               &atom->track_reference_type[i]);
    if (mutff_is_error(err)) {
      return err;
    }
    size += reference_size;
  }
  *out = mutff_atom_size(size);
  return 0;
}

MuTFFError mutff_write_track_reference_atom(FILE *fd,
                                            const MuTFFTrackReferenceAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_track_reference_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('t', 'r', 'e', 'f'));
  for (size_t i = 0; i < in->track_reference_type_count; ++i) {
    MuTFF_FIELD(mutff_write_track_reference_type_atom,
                &in->track_reference_type[i]);
  }
  return ret;
}

MuTFFError mutff_read_track_exclude_from_autoselection_atom(
    FILE *fd, MuTFFTrackExcludeFromAutoselectionAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('t', 'x', 'a', 's')) {
    return MuTFFErrorBadFormat;
  }
  return ret;
}

static inline MuTFFError mutff_track_exclude_from_autoselection_atom_size(
    uint64_t *out, const MuTFFTrackExcludeFromAutoselectionAtom *atom) {
  *out = mutff_atom_size(0);
  return 0;
}

MuTFFError mutff_write_track_exclude_from_autoselection_atom(
    FILE *fd, const MuTFFTrackExcludeFromAutoselectionAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_track_exclude_from_autoselection_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('t', 'x', 'a', 's'));
  return ret;
}

MuTFFError mutff_read_track_load_settings_atom(
    FILE *fd, MuTFFTrackLoadSettingsAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('l', 'o', 'a', 'd')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u32, &out->preload_start_time);
  MuTFF_FIELD(mutff_read_u32, &out->preload_duration);
  MuTFF_FIELD(mutff_read_u32, &out->preload_flags);
  MuTFF_FIELD(mutff_read_u32, &out->default_hints);
  return ret;
}

static inline MuTFFError mutff_track_load_settings_atom_size(
    uint64_t *out, const MuTFFTrackLoadSettingsAtom *atom) {
  *out = mutff_atom_size(16);
  return 0;
}

MuTFFError mutff_write_track_load_settings_atom(
    FILE *fd, const MuTFFTrackLoadSettingsAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_track_load_settings_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('l', 'o', 'a', 'd'));
  MuTFF_FIELD(mutff_write_u32, in->preload_start_time);
  MuTFF_FIELD(mutff_write_u32, in->preload_duration);
  MuTFF_FIELD(mutff_write_u32, in->preload_flags);
  MuTFF_FIELD(mutff_write_u32, in->default_hints);
  return ret;
}

MuTFFError mutff_read_input_type_atom(FILE *fd, MuTFFInputTypeAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('\0', '\0', 't', 'y')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u32, &out->input_type);
  return ret;
}

static inline MuTFFError mutff_input_type_atom_size(
    uint64_t *out, const MuTFFInputTypeAtom *atom) {
  *out = mutff_atom_size(4);
  return 0;
}

MuTFFError mutff_write_input_type_atom(FILE *fd, const MuTFFInputTypeAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_input_type_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('\0', '\0', 't', 'y'));
  MuTFF_FIELD(mutff_write_u32, in->input_type);
  return ret;
}

MuTFFError mutff_read_object_id_atom(FILE *fd, MuTFFObjectIDAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('o', 'b', 'i', 'd')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u32, &out->object_id);
  return ret;
}

static inline MuTFFError mutff_object_id_atom_size(
    uint64_t *out, const MuTFFObjectIDAtom *atom) {
  *out = mutff_atom_size(4);
  return 0;
}

MuTFFError mutff_write_object_id_atom(FILE *fd, const MuTFFObjectIDAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_object_id_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('o', 'b', 'i', 'd'));
  MuTFF_FIELD(mutff_write_u32, in->object_id);
  return ret;
}

MuTFFError mutff_read_track_input_atom(FILE *fd, MuTFFTrackInputAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  bool input_type_present = false;

  out->object_id_atom_present = false;

  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('\0', '\0', 'i', 'n')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u32, &out->atom_id);
  MuTFF_SEEK_CUR(2U);
  MuTFF_FIELD(mutff_read_u16, &out->child_count);
  MuTFF_SEEK_CUR(4U);

  // read children
  uint64_t child_size;
  uint32_t child_type;
  while (ret < size) {
    MuTFF_FIELD(mutff_peek_atom_header, &child_size, &child_type);
    if (size == 0U) {
      return MuTFFErrorBadFormat;
    }
    if (ret + child_size > size) {
      return MuTFFErrorBadFormat;
    }
    switch (child_type) {
      case MuTFF_FOURCC('\0', '\0', 't', 'y'):
        MuTFF_READ_CHILD(mutff_read_input_type_atom, &out->input_type_atom,
                         input_type_present);
        break;
      case MuTFF_FOURCC('o', 'b', 'i', 'd'):
        MuTFF_READ_CHILD(mutff_read_object_id_atom, &out->object_id_atom,
                         out->object_id_atom_present);
        break;
      default:
        MuTFF_SEEK_CUR(child_size);
        break;
    }
  }

  if (!input_type_present) {
    return MuTFFErrorBadFormat;
  }

  return ret;
}

static inline MuTFFError mutff_track_input_atom_size(
    uint64_t *out, const MuTFFTrackInputAtom *atom) {
  MuTFFError err;
  uint64_t child_size;

  uint64_t size = 12;
  err = mutff_input_type_atom_size(&child_size, &atom->input_type_atom);
  if (mutff_is_error(err)) {
    return err;
  }
  size += child_size;
  if (atom->object_id_atom_present) {
    err = mutff_object_id_atom_size(&child_size, &atom->object_id_atom);
    if (mutff_is_error(err)) {
      return err;
    }
    size += child_size;
  }

  *out = mutff_atom_size(size);
  return 0;
}

MuTFFError mutff_write_track_input_atom(FILE *fd,
                                        const MuTFFTrackInputAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_track_input_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('\0', '\0', 'i', 'n'));
  MuTFF_FIELD(mutff_write_u32, in->atom_id);
  for (size_t i = 0; i < 2U; ++i) {
    MuTFF_FIELD(mutff_write_u8, 0);
  }
  MuTFF_FIELD(mutff_write_u16, in->child_count);
  for (size_t i = 0; i < 4U; ++i) {
    MuTFF_FIELD(mutff_write_u8, 0);
  }
  MuTFF_FIELD(mutff_write_input_type_atom, &in->input_type_atom);
  MuTFF_FIELD(mutff_write_object_id_atom, &in->object_id_atom);
  return ret;
}

MuTFFError mutff_read_track_input_map_atom(FILE *fd,
                                           MuTFFTrackInputMapAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('i', 'm', 'a', 'p')) {
    return MuTFFErrorBadFormat;
  }

  // read children
  size_t i = 0;
  uint64_t child_size;
  uint32_t child_type;
  while (ret < size) {
    if (i >= MuTFF_MAX_TRACK_REFERENCE_TYPE_ATOMS) {
      return MuTFFErrorOutOfMemory;
    }
    MuTFF_FIELD(mutff_peek_atom_header, &child_size, &child_type);
    if (size == 0U) {
      return MuTFFErrorBadFormat;
    }
    if (ret + child_size > size) {
      return MuTFFErrorBadFormat;
    }
    if (child_type == MuTFF_FOURCC('\0', '\0', 'i', 'n')) {
      MuTFF_FIELD(mutff_read_track_input_atom, &out->track_input_atoms[i]);
      i++;
    } else {
      MuTFF_SEEK_CUR(child_size);
    }
  }
  out->track_input_atom_count = i;

  return ret;
}

static inline MuTFFError mutff_track_input_map_atom_size(
    uint64_t *out, const MuTFFTrackInputMapAtom *atom) {
  MuTFFError err;
  uint64_t size = 0;
  for (size_t i = 0; i < atom->track_input_atom_count; ++i) {
    uint64_t child_size;
    err = mutff_track_input_atom_size(&child_size, &atom->track_input_atoms[i]);
    if (mutff_is_error(err)) {
      return err;
    }
    size += child_size;
  }
  *out = mutff_atom_size(size);
  return 0;
}

MuTFFError mutff_write_track_input_map_atom(FILE *fd,
                                            const MuTFFTrackInputMapAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_track_input_map_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('i', 'm', 'a', 'p'));
  for (size_t i = 0; i < in->track_input_atom_count; ++i) {
    MuTFF_FIELD(mutff_write_track_input_atom, &in->track_input_atoms[i]);
  }
  return ret;
}

MuTFFError mutff_read_media_header_atom(FILE *fd, MuTFFMediaHeaderAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('m', 'd', 'h', 'd')) {
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

static inline MuTFFError mutff_media_header_atom_size(
    uint64_t *out, const MuTFFMediaHeaderAtom *atom) {
  *out = mutff_atom_size(24);
  return 0;
}

MuTFFError mutff_write_media_header_atom(FILE *fd,
                                         const MuTFFMediaHeaderAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_media_header_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('m', 'd', 'h', 'd'));
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
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('e', 'l', 'n', 'g')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);

  // read variable-length data
  const size_t tag_length = size - 12U;
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
static inline MuTFFError mutff_extended_language_tag_atom_size(
    uint64_t *out, const MuTFFExtendedLanguageTagAtom *atom) {
  *out = mutff_atom_size(4U + strlen(atom->language_tag_string) + 1U);
  return 0;
}

MuTFFError mutff_write_extended_language_tag_atom(
    FILE *fd, const MuTFFExtendedLanguageTagAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  size_t i;
  uint64_t size;
  err = mutff_extended_language_tag_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('e', 'l', 'n', 'g'));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  i = 0;
  while (in->language_tag_string[i] != (char)'\0') {
    MuTFF_FIELD(mutff_write_u8, in->language_tag_string[i]);
    ++i;
  }
  for (; i < mutff_data_size(size) - 4U; ++i) {
    MuTFF_FIELD(mutff_write_u8, 0);
  }
  return ret;
}

MuTFFError mutff_read_handler_reference_atom(FILE *fd,
                                             MuTFFHandlerReferenceAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  size_t i;
  size_t name_length;

  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('h', 'd', 'l', 'r')) {
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

static inline MuTFFError mutff_handler_reference_atom_size(
    uint64_t *out, const MuTFFHandlerReferenceAtom *atom) {
  *out = mutff_atom_size(24U + strlen(atom->component_name));
  return 0;
}

MuTFFError mutff_write_handler_reference_atom(
    FILE *fd, const MuTFFHandlerReferenceAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_handler_reference_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('h', 'd', 'l', 'r'));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_u32, in->component_type);
  MuTFF_FIELD(mutff_write_u32, in->component_subtype);
  MuTFF_FIELD(mutff_write_u32, in->component_manufacturer);
  MuTFF_FIELD(mutff_write_u32, in->component_flags);
  MuTFF_FIELD(mutff_write_u32, in->component_flags_mask);
  for (size_t i = 0; i < size - 32U; ++i) {
    MuTFF_FIELD(mutff_write_u8, in->component_name[i]);
  }
  return ret;
}

MuTFFError mutff_read_video_media_information_header_atom(
    FILE *fd, MuTFFVideoMediaInformationHeaderAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('v', 'm', 'h', 'd')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_u16, &out->graphics_mode);
  for (size_t i = 0; i < 3U; ++i) {
    MuTFF_FIELD(mutff_read_u16, &out->opcolor[i]);
  }
  return ret;
}

static inline MuTFFError mutff_video_media_information_header_atom_size(
    uint64_t *out, const MuTFFVideoMediaInformationHeaderAtom *atom) {
  *out = mutff_atom_size(12);
  return 0;
}

MuTFFError mutff_write_video_media_information_header_atom(
    FILE *fd, const MuTFFVideoMediaInformationHeaderAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_video_media_information_header_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('v', 'm', 'h', 'd'));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_u16, in->graphics_mode);
  for (size_t i = 0; i < 3U; ++i) {
    MuTFF_FIELD(mutff_write_u16, in->opcolor[i]);
  }
  return ret;
}

MuTFFError mutff_read_data_reference(FILE *fd, MuTFFDataReference *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint32_t size;
  MuTFF_FIELD(mutff_read_u32, &size);
  MuTFF_FIELD(mutff_read_u32, &out->type);
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);

  // read variable-length data
  out->data_size = size - 12U;
  if (out->data_size > MuTFF_MAX_DATA_REFERENCE_DATA_SIZE) {
    return MuTFFErrorOutOfMemory;
  }
  for (size_t i = 0; i < out->data_size; ++i) {
    MuTFF_FIELD(mutff_read_u8, (uint8_t *)&out->data[i]);
  }

  return ret;
}

static inline MuTFFError mutff_data_reference_size(
    uint64_t *out, const MuTFFDataReference *ref) {
  *out = 12U + ref->data_size;
  return 0;
}

MuTFFError mutff_write_data_reference(FILE *fd, const MuTFFDataReference *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_data_reference_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
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
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;

  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('d', 'r', 'e', 'f')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_u32, &out->number_of_entries);

  // read child atoms
  if (out->number_of_entries > MuTFF_MAX_DATA_REFERENCES) {
    return MuTFFErrorOutOfMemory;
  }

  uint64_t child_size;
  uint32_t child_type;
  for (size_t i = 0; i < out->number_of_entries; ++i) {
    MuTFF_FIELD(mutff_peek_atom_header, &child_size, &child_type);
    if (ret + child_size > size) {
      return MuTFFErrorBadFormat;
    }
    MuTFF_FIELD(mutff_read_data_reference, &out->data_references[i]);
  }

  // skip any remaining space
  MuTFF_SEEK_CUR(size - ret);

  return ret;
}

static inline MuTFFError mutff_data_reference_atom_size(
    uint64_t *out, const MuTFFDataReferenceAtom *atom) {
  MuTFFError err;
  uint64_t size = 8;
  for (uint32_t i = 0; i < atom->number_of_entries; ++i) {
    uint64_t child_size;
    err = mutff_data_reference_size(&child_size, &atom->data_references[i]);
    if (mutff_is_error(err)) {
      return err;
    }
    size += child_size;
  }
  *out = mutff_atom_size(size);
  return 0;
}

MuTFFError mutff_write_data_reference_atom(FILE *fd,
                                           const MuTFFDataReferenceAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_data_reference_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('d', 'r', 'e', 'f'));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_u32, in->number_of_entries);
  for (uint32_t i = 0; i < in->number_of_entries; ++i) {
    MuTFF_FIELD(mutff_write_data_reference, &in->data_references[i]);
  }
  return ret;
}

MuTFFError mutff_read_data_information_atom(FILE *fd,
                                            MuTFFDataInformationAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  bool data_reference_present = false;

  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('d', 'i', 'n', 'f')) {
    return MuTFFErrorBadFormat;
  }

  uint64_t child_size;
  uint32_t child_type;
  while (ret < size) {
    MuTFF_FIELD(mutff_peek_atom_header, &child_size, &child_type);
    if (size == 0U) {
      return MuTFFErrorBadFormat;
    }
    if (ret + child_size > size) {
      return MuTFFErrorBadFormat;
    }
    if (child_type == MuTFF_FOURCC('d', 'r', 'e', 'f')) {
      MuTFF_READ_CHILD(mutff_read_data_reference_atom, &out->data_reference,
                       data_reference_present);
    } else {
      MuTFF_SEEK_CUR(child_size);
    }
  }

  if (!data_reference_present) {
    return MuTFFErrorBadFormat;
  }

  return ret;
}

static inline MuTFFError mutff_data_information_atom_size(
    uint64_t *out, const MuTFFDataInformationAtom *atom) {
  uint64_t size;
  const MuTFFError err =
      mutff_data_reference_atom_size(&size, &atom->data_reference);
  if (mutff_is_error(err)) {
    return err;
  }
  *out = mutff_atom_size(size);
  return 0;
}

MuTFFError mutff_write_data_information_atom(
    FILE *fd, const MuTFFDataInformationAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_data_information_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('d', 'i', 'n', 'f'));
  MuTFF_FIELD(mutff_write_data_reference_atom, &in->data_reference);
  return ret;
}

MuTFFError mutff_read_sample_description_atom(FILE *fd,
                                              MuTFFSampleDescriptionAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('s', 't', 's', 'd')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_u32, &out->number_of_entries);

  // read child atoms
  if (out->number_of_entries > MuTFF_MAX_SAMPLE_DESCRIPTION_TABLE_LEN) {
    return MuTFFErrorOutOfMemory;
  }

  uint64_t child_size;
  uint32_t child_type;
  for (size_t i = 0; i < out->number_of_entries; ++i) {
    MuTFF_FIELD(mutff_peek_atom_header, &child_size, &child_type);
    if (ret + child_size > size) {
      return MuTFFErrorBadFormat;
    }
    MuTFF_FIELD(mutff_read_sample_description,
                &out->sample_description_table[i]);
  }

  // skip any remaining space
  MuTFF_SEEK_CUR(size - ret);

  return ret;
}

static inline MuTFFError mutff_sample_description_atom_size(
    uint64_t *out, const MuTFFSampleDescriptionAtom *atom) {
  uint64_t size = 0;
  for (uint32_t i = 0; i < atom->number_of_entries; ++i) {
    size += atom->sample_description_table[i].size;
  }
  *out = mutff_atom_size(8U + size);
  return 0;
}

MuTFFError mutff_write_sample_description_atom(
    FILE *fd, const MuTFFSampleDescriptionAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  size_t offset;
  uint64_t size;
  err = mutff_sample_description_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('s', 't', 's', 'd'));
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
  uint64_t ret = 0;
  MuTFF_FIELD(mutff_read_u32, &out->sample_count);
  MuTFF_FIELD(mutff_read_u32, &out->sample_duration);
  return ret;
}

MuTFFError mutff_write_time_to_sample_table_entry(
    FILE *fd, const MuTFFTimeToSampleTableEntry *in) {
  MuTFFError err;
  uint64_t ret = 0;
  MuTFF_FIELD(mutff_write_u32, in->sample_count);
  MuTFF_FIELD(mutff_write_u32, in->sample_duration);
  return ret;
}

MuTFFError mutff_read_time_to_sample_atom(FILE *fd,
                                          MuTFFTimeToSampleAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('s', 't', 't', 's')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_u32, &out->number_of_entries);

  // read time to sample table
  if (out->number_of_entries > MuTFF_MAX_TIME_TO_SAMPLE_TABLE_LEN) {
    return MuTFFErrorOutOfMemory;
  }
  const size_t table_size = mutff_data_size(size) - 8U;
  if (table_size != out->number_of_entries * 8U) {
    return MuTFFErrorBadFormat;
  }
  for (size_t i = 0; i < out->number_of_entries; ++i) {
    MuTFF_FIELD(mutff_read_time_to_sample_table_entry,
                &out->time_to_sample_table[i]);
  }

  return ret;
}

static inline MuTFFError mutff_time_to_sample_atom_size(
    uint64_t *out, const MuTFFTimeToSampleAtom *atom) {
  *out = mutff_atom_size(8U + atom->number_of_entries * 8U);
  return 0;
}

MuTFFError mutff_write_time_to_sample_atom(FILE *fd,
                                           const MuTFFTimeToSampleAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_time_to_sample_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('s', 't', 't', 's'));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_u32, in->number_of_entries);
  if (in->number_of_entries * 8U != mutff_data_size(size) - 8U) {
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
  uint64_t ret = 0;
  MuTFF_FIELD(mutff_read_u32, &out->sample_count);
  MuTFF_FIELD(mutff_read_u32, &out->composition_offset);
  return ret;
}

MuTFFError mutff_write_composition_offset_table_entry(
    FILE *fd, const MuTFFCompositionOffsetTableEntry *in) {
  MuTFFError err;
  uint64_t ret = 0;
  MuTFF_FIELD(mutff_write_u32, in->sample_count);
  MuTFF_FIELD(mutff_write_u32, in->composition_offset);
  return ret;
}

MuTFFError mutff_read_composition_offset_atom(FILE *fd,
                                              MuTFFCompositionOffsetAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('c', 't', 't', 's')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_u32, &out->entry_count);

  // read composition offset table
  if (out->entry_count > MuTFF_MAX_COMPOSITION_OFFSET_TABLE_LEN) {
    return MuTFFErrorOutOfMemory;
  }
  const size_t table_size = mutff_data_size(size) - 8U;
  if (table_size != out->entry_count * 8U) {
    return MuTFFErrorBadFormat;
  }
  for (size_t i = 0; i < out->entry_count; ++i) {
    MuTFF_FIELD(mutff_read_composition_offset_table_entry,
                &out->composition_offset_table[i]);
    ;
  }

  return ret;
}

static inline MuTFFError mutff_composition_offset_atom_size(
    uint64_t *out, const MuTFFCompositionOffsetAtom *atom) {
  *out = mutff_atom_size(8U + 8U * atom->entry_count);
  return 0;
}

MuTFFError mutff_write_composition_offset_atom(
    FILE *fd, const MuTFFCompositionOffsetAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_composition_offset_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('c', 't', 't', 's'));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_u32, in->entry_count);
  if (in->entry_count * 8U != mutff_data_size(size) - 8U) {
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
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('c', 's', 'l', 'g')) {
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

static inline MuTFFError mutff_composition_shift_least_greatest_atom_size(
    uint64_t *out, const MuTFFCompositionShiftLeastGreatestAtom *atom) {
  *out = mutff_atom_size(24);
  return 0;
}

MuTFFError mutff_write_composition_shift_least_greatest_atom(
    FILE *fd, const MuTFFCompositionShiftLeastGreatestAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_composition_shift_least_greatest_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('c', 's', 'l', 'g'));
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
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('s', 't', 's', 's')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_u32, &out->number_of_entries);

  // read sync sample table
  if (out->number_of_entries > MuTFF_MAX_SYNC_SAMPLE_TABLE_LEN) {
    return MuTFFErrorOutOfMemory;
  }
  const size_t table_size = mutff_data_size(size) - 8U;
  if (table_size != out->number_of_entries * 4U) {
    return MuTFFErrorBadFormat;
  }
  for (size_t i = 0; i < out->number_of_entries; ++i) {
    MuTFF_FIELD(mutff_read_u32, &out->sync_sample_table[i]);
  }

  return ret;
}

static inline MuTFFError mutff_sync_sample_atom_size(
    uint64_t *out, const MuTFFSyncSampleAtom *atom) {
  *out = mutff_atom_size(8U + atom->number_of_entries * 4U);
  return 0;
}

MuTFFError mutff_write_sync_sample_atom(FILE *fd,
                                        const MuTFFSyncSampleAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_sync_sample_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('s', 't', 's', 's'));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_u32, in->number_of_entries);
  if (in->number_of_entries * 4U != mutff_data_size(size) - 8U) {
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
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('s', 't', 'p', 's')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_u32, &out->entry_count);

  // read partial sync sample table
  if (out->entry_count > MuTFF_MAX_PARTIAL_SYNC_SAMPLE_TABLE_LEN) {
    return MuTFFErrorOutOfMemory;
  }
  const size_t table_size = mutff_data_size(size) - 8U;
  if (table_size != out->entry_count * 4U) {
    return MuTFFErrorBadFormat;
  }
  for (size_t i = 0; i < out->entry_count; ++i) {
    MuTFF_FIELD(mutff_read_u32, &out->partial_sync_sample_table[i]);
  }

  return ret;
}

static inline MuTFFError mutff_partial_sync_sample_atom_size(
    uint64_t *out, const MuTFFPartialSyncSampleAtom *atom) {
  *out = mutff_atom_size(8U + atom->entry_count * 4U);
  return 0;
}

MuTFFError mutff_write_partial_sync_sample_atom(
    FILE *fd, const MuTFFPartialSyncSampleAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_partial_sync_sample_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('s', 't', 'p', 's'));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_u32, in->entry_count);
  if (in->entry_count * 4U != mutff_data_size(size) - 8U) {
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
  uint64_t ret = 0;
  MuTFF_FIELD(mutff_read_u32, &out->first_chunk);
  MuTFF_FIELD(mutff_read_u32, &out->samples_per_chunk);
  MuTFF_FIELD(mutff_read_u32, &out->sample_description_id);
  return ret;
}

MuTFFError mutff_write_sample_to_chunk_table_entry(
    FILE *fd, const MuTFFSampleToChunkTableEntry *in) {
  MuTFFError err;
  uint64_t ret = 0;
  MuTFF_FIELD(mutff_write_u32, in->first_chunk);
  MuTFF_FIELD(mutff_write_u32, in->samples_per_chunk);
  MuTFF_FIELD(mutff_write_u32, in->sample_description_id);
  return ret;
}

MuTFFError mutff_read_sample_to_chunk_atom(FILE *fd,
                                           MuTFFSampleToChunkAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('s', 't', 's', 'c')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_u32, &out->number_of_entries);

  // read table
  if (out->number_of_entries > MuTFF_MAX_SAMPLE_TO_CHUNK_TABLE_LEN) {
    return MuTFFErrorOutOfMemory;
  }
  const size_t table_size = mutff_data_size(size) - 8U;
  if (table_size != out->number_of_entries * 12U) {
    return MuTFFErrorBadFormat;
  }
  for (size_t i = 0; i < out->number_of_entries; ++i) {
    MuTFF_FIELD(mutff_read_sample_to_chunk_table_entry,
                &out->sample_to_chunk_table[i]);
  }

  return ret;
}

static inline MuTFFError mutff_sample_to_chunk_atom_size(
    uint64_t *out, const MuTFFSampleToChunkAtom *atom) {
  *out = mutff_atom_size(8U + atom->number_of_entries * 12U);
  return 0;
}

MuTFFError mutff_write_sample_to_chunk_atom(FILE *fd,
                                            const MuTFFSampleToChunkAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_sample_to_chunk_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('s', 't', 's', 'c'));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_u32, in->number_of_entries);
  if (in->number_of_entries * 12U != mutff_data_size(size) - 8U) {
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
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('s', 't', 's', 'z')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_u32, &out->sample_size);
  MuTFF_FIELD(mutff_read_u32, &out->number_of_entries);

  if (out->sample_size == 0U) {
    // read table
    if (out->number_of_entries > MuTFF_MAX_SAMPLE_SIZE_TABLE_LEN) {
      return MuTFFErrorOutOfMemory;
    }
    const size_t table_size = mutff_data_size(size) - 12U;
    if (table_size != out->number_of_entries * 4U) {
      return MuTFFErrorBadFormat;
    }
    for (size_t i = 0; i < out->number_of_entries; ++i) {
      MuTFF_FIELD(mutff_read_u32, &out->sample_size_table[i]);
    }
  } else {
    // skip table
    MuTFF_SEEK_CUR(size - ret);
  }

  return ret;
}

static inline MuTFFError mutff_sample_size_atom_size(
    uint64_t *out, const MuTFFSampleSizeAtom *atom) {
  *out = mutff_atom_size(12U + atom->number_of_entries * 4U);
  return 0;
}

MuTFFError mutff_write_sample_size_atom(FILE *fd,
                                        const MuTFFSampleSizeAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_sample_size_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('s', 't', 's', 'z'));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_u32, in->sample_size);
  MuTFF_FIELD(mutff_write_u32, in->number_of_entries);
  // @TODO: does this need a branch for in->sample_size != 0?
  //        i.e. what to do if sample_size != 0 but number_of_entries != 0
  if (in->number_of_entries * 4U != mutff_data_size(size) - 12U) {
    return MuTFFErrorBadFormat;
  }
  for (uint32_t i = 0; i < in->number_of_entries; ++i) {
    MuTFF_FIELD(mutff_write_u32, in->sample_size_table[i]);
  }
  return ret;
}

MuTFFError mutff_read_chunk_offset_atom(FILE *fd, MuTFFChunkOffsetAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('s', 't', 'c', 'o')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_u32, &out->number_of_entries);

  // read table
  if (out->number_of_entries > MuTFF_MAX_CHUNK_OFFSET_TABLE_LEN) {
    return MuTFFErrorOutOfMemory;
  }
  const size_t table_size = mutff_data_size(size) - 8U;
  if (table_size != out->number_of_entries * 4U) {
    return MuTFFErrorBadFormat;
  }
  for (size_t i = 0; i < out->number_of_entries; ++i) {
    MuTFF_FIELD(mutff_read_u32, &out->chunk_offset_table[i]);
  }

  return ret;
}

static inline MuTFFError mutff_chunk_offset_atom_size(
    uint64_t *out, const MuTFFChunkOffsetAtom *atom) {
  *out = mutff_atom_size(8U + atom->number_of_entries * 4U);
  return 0;
}

MuTFFError mutff_write_chunk_offset_atom(FILE *fd,
                                         const MuTFFChunkOffsetAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_chunk_offset_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('s', 't', 'c', 'o'));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_u32, in->number_of_entries);
  if (in->number_of_entries * 4U != mutff_data_size(size) - 8U) {
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
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('s', 'd', 't', 'p')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);

  // read table
  out->data_size = mutff_data_size(size) - 4U;
  if (out->data_size > MuTFF_MAX_SAMPLE_DEPENDENCY_FLAGS_TABLE_LEN) {
    return MuTFFErrorOutOfMemory;
  }
  for (size_t i = 0; i < out->data_size; ++i) {
    MuTFF_FIELD(mutff_read_u8, &out->sample_dependency_flags_table[i]);
  }

  return ret;
}

static inline MuTFFError mutff_sample_dependency_flags_atom_size(
    uint64_t *out, const MuTFFSampleDependencyFlagsAtom *atom) {
  *out = mutff_atom_size(4U + atom->data_size);
  return 0;
}

MuTFFError mutff_write_sample_dependency_flags_atom(
    FILE *fd, const MuTFFSampleDependencyFlagsAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_sample_dependency_flags_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('s', 'd', 't', 'p'));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  const size_t flags_table_size = mutff_data_size(size) - 4U;
  for (uint32_t i = 0; i < flags_table_size; ++i) {
    MuTFF_FIELD(mutff_write_u8, in->sample_dependency_flags_table[i]);
  }
  return ret;
}

MuTFFError mutff_read_sample_table_atom(FILE *fd, MuTFFSampleTableAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  bool sample_description_present = false;
  bool time_to_sample_present = false;

  out->composition_offset_present = false;
  out->composition_shift_least_greatest_present = false;
  out->sync_sample_present = false;
  out->partial_sync_sample_present = false;
  out->sample_to_chunk_present = false;
  out->sample_size_present = false;
  out->chunk_offset_present = false;
  out->sample_dependency_flags_present = false;

  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('s', 't', 'b', 'l')) {
    return MuTFFErrorBadFormat;
  }

  // read child atoms
  uint64_t child_size;
  uint32_t child_type;
  while (ret < size) {
    MuTFF_FIELD(mutff_peek_atom_header, &child_size, &child_type);
    if (size == 0U) {
      return MuTFFErrorBadFormat;
    }
    if (ret + child_size > size) {
      return MuTFFErrorBadFormat;
    }

    switch (child_type) {
      case MuTFF_FOURCC('s', 't', 's', 'd'):
        MuTFF_READ_CHILD(mutff_read_sample_description_atom,
                         &out->sample_description, sample_description_present);
        break;
      case MuTFF_FOURCC('s', 't', 't', 's'):
        MuTFF_READ_CHILD(mutff_read_time_to_sample_atom, &out->time_to_sample,
                         time_to_sample_present);
        break;
      case MuTFF_FOURCC('c', 't', 't', 's'):
        MuTFF_READ_CHILD(mutff_read_composition_offset_atom,
                         &out->composition_offset,
                         out->composition_offset_present);
        break;
      case MuTFF_FOURCC('c', 's', 'l', 'g'):
        MuTFF_READ_CHILD(mutff_read_composition_shift_least_greatest_atom,
                         &out->composition_shift_least_greatest,
                         out->composition_shift_least_greatest_present);
        break;
      case MuTFF_FOURCC('s', 't', 's', 's'):
        MuTFF_READ_CHILD(mutff_read_sync_sample_atom, &out->sync_sample,
                         out->sync_sample_present);
        break;
      case MuTFF_FOURCC('s', 't', 'p', 's'):
        MuTFF_READ_CHILD(mutff_read_partial_sync_sample_atom,
                         &out->partial_sync_sample,
                         out->partial_sync_sample_present);
        break;
      case MuTFF_FOURCC('s', 't', 's', 'c'):
        MuTFF_READ_CHILD(mutff_read_sample_to_chunk_atom, &out->sample_to_chunk,
                         out->sample_to_chunk_present);
        break;
      case MuTFF_FOURCC('s', 't', 's', 'z'):
        MuTFF_READ_CHILD(mutff_read_sample_size_atom, &out->sample_size,
                         out->sample_size_present);
        break;
      case MuTFF_FOURCC('s', 't', 'c', 'o'):
        MuTFF_READ_CHILD(mutff_read_chunk_offset_atom, &out->chunk_offset,
                         out->chunk_offset_present);
        break;
      case MuTFF_FOURCC('s', 'd', 't', 'p'):
        MuTFF_READ_CHILD(mutff_read_sample_dependency_flags_atom,
                         &out->sample_dependency_flags,
                         out->sample_dependency_flags_present);
        break;
      // reserved for future use
      /* case MuTFF_FOURCC('s', 't', 's', 'h'): */
      /*   break; */
      /* case MuTFF_FOURCC('s', 'g', 'p', 'd'): */
      /*   break; */
      /* case MuTFF_FOURCC('s', 'b', 'g', 'p'): */
      /*   break; */
      default:
        MuTFF_SEEK_CUR(child_size);
        break;
    }
  }

  if (!sample_description_present || !time_to_sample_present) {
    return MuTFFErrorBadFormat;
  }

  return ret;
}

static inline MuTFFError mutff_sample_table_atom_size(
    uint64_t *out, const MuTFFSampleTableAtom *atom) {
  MuTFFError err;
  uint64_t size;
  uint64_t child_size;
  err = mutff_sample_description_atom_size(&size, &atom->sample_description);
  if (mutff_is_error(err)) {
    return err;
  }
  err = mutff_time_to_sample_atom_size(&child_size, &atom->time_to_sample);
  if (mutff_is_error(err)) {
    return err;
  }
  size += child_size;
  if (atom->composition_offset_present) {
    err = mutff_composition_offset_atom_size(&child_size,
                                             &atom->composition_offset);
    if (mutff_is_error(err)) {
      return err;
    }
    size += child_size;
  }
  if (atom->composition_shift_least_greatest_present) {
    err = mutff_composition_shift_least_greatest_atom_size(
        &child_size, &atom->composition_shift_least_greatest);
    if (mutff_is_error(err)) {
      return err;
    }
    size += child_size;
  }
  if (atom->sync_sample_present) {
    err = mutff_sync_sample_atom_size(&child_size, &atom->sync_sample);
    if (mutff_is_error(err)) {
      return err;
    }
    size += child_size;
  }
  if (atom->partial_sync_sample_present) {
    err = mutff_partial_sync_sample_atom_size(&child_size,
                                              &atom->partial_sync_sample);
    if (mutff_is_error(err)) {
      return err;
    }
    size += child_size;
  }
  if (atom->sample_to_chunk_present) {
    err = mutff_sample_to_chunk_atom_size(&child_size, &atom->sample_to_chunk);
    if (mutff_is_error(err)) {
      return err;
    }
    size += child_size;
  }
  if (atom->sample_size_present) {
    err = mutff_sample_size_atom_size(&child_size, &atom->sample_size);
    if (mutff_is_error(err)) {
      return err;
    }
    size += child_size;
  }
  if (atom->chunk_offset_present) {
    err = mutff_chunk_offset_atom_size(&child_size, &atom->chunk_offset);
    if (mutff_is_error(err)) {
      return err;
    }
    size += child_size;
  }
  if (atom->sample_dependency_flags_present) {
    err = mutff_sample_dependency_flags_atom_size(
        &child_size, &atom->sample_dependency_flags);
    if (mutff_is_error(err)) {
      return err;
    }
    size += child_size;
  }
  *out = mutff_atom_size(size);
  return 0;
}

MuTFFError mutff_write_sample_table_atom(FILE *fd,
                                         const MuTFFSampleTableAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;

  uint64_t size;
  err = mutff_sample_table_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('s', 't', 'b', 'l'));
  MuTFF_FIELD(mutff_write_sample_description_atom, &in->sample_description);
  MuTFF_FIELD(mutff_write_time_to_sample_atom, &in->time_to_sample);
  if (in->composition_offset_present) {
    MuTFF_FIELD(mutff_write_composition_offset_atom, &in->composition_offset);
  }
  if (in->composition_shift_least_greatest_present) {
    MuTFF_FIELD(mutff_write_composition_shift_least_greatest_atom,
                &in->composition_shift_least_greatest);
  }
  if (in->sync_sample_present) {
    MuTFF_FIELD(mutff_write_sync_sample_atom, &in->sync_sample);
  }
  if (in->partial_sync_sample_present) {
    MuTFF_FIELD(mutff_write_partial_sync_sample_atom, &in->partial_sync_sample);
  }
  if (in->sample_to_chunk_present) {
    MuTFF_FIELD(mutff_write_sample_to_chunk_atom, &in->sample_to_chunk);
  }
  if (in->sample_size_present) {
    MuTFF_FIELD(mutff_write_sample_size_atom, &in->sample_size);
  }
  if (in->chunk_offset_present) {
    MuTFF_FIELD(mutff_write_chunk_offset_atom, &in->chunk_offset);
  }
  if (in->sample_dependency_flags_present) {
    MuTFF_FIELD(mutff_write_sample_dependency_flags_atom,
                &in->sample_dependency_flags);
  }

  return ret;
}

MuTFFError mutff_read_video_media_information_atom(
    FILE *fd, MuTFFVideoMediaInformationAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  bool video_media_information_header_present = false;
  bool handler_reference_present = false;

  out->data_information_present = false;
  out->sample_table_present = false;

  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('m', 'i', 'n', 'f')) {
    return MuTFFErrorBadFormat;
  }

  // read child atoms
  uint64_t child_size;
  uint32_t child_type;
  while (ret < size) {
    MuTFF_FIELD(mutff_peek_atom_header, &child_size, &child_type);
    if (size == 0U) {
      return MuTFFErrorBadFormat;
    }
    if (ret + child_size > size) {
      return MuTFFErrorBadFormat;
    }

    switch (child_type) {
      case MuTFF_FOURCC('v', 'm', 'h', 'd'):
        MuTFF_READ_CHILD(mutff_read_video_media_information_header_atom,
                         &out->video_media_information_header,
                         video_media_information_header_present);
        break;
      case MuTFF_FOURCC('h', 'd', 'l', 'r'):
        MuTFF_READ_CHILD(mutff_read_handler_reference_atom,
                         &out->handler_reference, handler_reference_present);
        break;
      case MuTFF_FOURCC('d', 'i', 'n', 'f'):
        MuTFF_READ_CHILD(mutff_read_data_information_atom,
                         &out->data_information, out->data_information_present);
        break;
      case MuTFF_FOURCC('s', 't', 'b', 'l'):
        MuTFF_READ_CHILD(mutff_read_sample_table_atom, &out->sample_table,
                         out->sample_table_present);
        break;
      default:
        MuTFF_SEEK_CUR(child_size);
        break;
    }
  }

  if (!video_media_information_header_present || !handler_reference_present) {
    return MuTFFErrorBadFormat;
  }

  return ret;
}

static inline MuTFFError mutff_video_media_information_atom_size(
    uint64_t *out, const MuTFFVideoMediaInformationAtom *atom) {
  MuTFFError err;
  uint64_t size;
  uint64_t child_size;
  err = mutff_video_media_information_header_atom_size(
      &size, &atom->video_media_information_header);
  if (mutff_is_error(err)) {
    return err;
  }
  err =
      mutff_handler_reference_atom_size(&child_size, &atom->handler_reference);
  if (mutff_is_error(err)) {
    return err;
  }
  size += child_size;
  if (atom->data_information_present) {
    err =
        mutff_data_information_atom_size(&child_size, &atom->data_information);
    if (mutff_is_error(err)) {
      return err;
    }
    size += child_size;
  }
  if (atom->sample_table_present) {
    err = mutff_sample_table_atom_size(&child_size, &atom->sample_table);
    if (mutff_is_error(err)) {
      return err;
    }
    size += child_size;
  }
  *out = mutff_atom_size(size);
  return 0;
}

MuTFFError mutff_write_video_media_information_atom(
    FILE *fd, const MuTFFVideoMediaInformationAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;

  uint64_t size;
  err = mutff_video_media_information_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('m', 'i', 'n', 'f'));
  MuTFF_FIELD(mutff_write_video_media_information_header_atom,
              &in->video_media_information_header);
  MuTFF_FIELD(mutff_write_handler_reference_atom, &in->handler_reference);
  if (in->data_information_present) {
    MuTFF_FIELD(mutff_write_data_information_atom, &in->data_information);
  }
  if (in->sample_table_present) {
    MuTFF_FIELD(mutff_write_sample_table_atom, &in->sample_table);
  }

  return ret;
}

MuTFFError mutff_read_sound_media_information_header_atom(
    FILE *fd, MuTFFSoundMediaInformationHeaderAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('s', 'm', 'h', 'd')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_i16, &out->balance);
  MuTFF_SEEK_CUR(2U);
  return ret;
}

static inline MuTFFError mutff_sound_media_information_header_atom_size(
    uint64_t *out, const MuTFFSoundMediaInformationHeaderAtom *atom) {
  *out = mutff_atom_size(8);
  return 0;
}

MuTFFError mutff_write_sound_media_information_header_atom(
    FILE *fd, const MuTFFSoundMediaInformationHeaderAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_sound_media_information_header_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('s', 'm', 'h', 'd'));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_i16, in->balance);
  for (size_t i = 0; i < 2U; ++i) {
    MuTFF_FIELD(mutff_write_u8, 0);
  }
  return ret;
}

MuTFFError mutff_read_sound_media_information_atom(
    FILE *fd, MuTFFSoundMediaInformationAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  bool sound_media_information_header_present = false;
  bool handler_reference_present = false;

  out->data_information_present = false;
  out->sample_table_present = false;

  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('m', 'i', 'n', 'f')) {
    return MuTFFErrorBadFormat;
  }

  // read child atoms
  uint64_t child_size;
  uint32_t child_type;
  while (ret < size) {
    MuTFF_FIELD(mutff_peek_atom_header, &child_size, &child_type);
    if (size == 0U) {
      return MuTFFErrorBadFormat;
    }
    if (ret + child_size > size) {
      return MuTFFErrorBadFormat;
    }

    switch (child_type) {
      case MuTFF_FOURCC('s', 'm', 'h', 'd'):
        MuTFF_READ_CHILD(mutff_read_sound_media_information_header_atom,
                         &out->sound_media_information_header,
                         sound_media_information_header_present);
        break;
      case MuTFF_FOURCC('h', 'd', 'l', 'r'):
        MuTFF_READ_CHILD(mutff_read_handler_reference_atom,
                         &out->handler_reference, handler_reference_present);
        break;
      case MuTFF_FOURCC('d', 'i', 'n', 'f'):
        MuTFF_READ_CHILD(mutff_read_data_information_atom,
                         &out->data_information, out->data_information_present);
        break;
      case MuTFF_FOURCC('s', 't', 'b', 'l'):
        MuTFF_READ_CHILD(mutff_read_sample_table_atom, &out->sample_table,
                         out->sample_table_present);
        break;
      default:
        MuTFF_SEEK_CUR(child_size);
        break;
    }
  }

  if (!sound_media_information_header_present || !handler_reference_present) {
    return MuTFFErrorBadFormat;
  }

  return ret;
}

static inline MuTFFError mutff_sound_media_information_atom_size(
    uint64_t *out, const MuTFFSoundMediaInformationAtom *atom) {
  MuTFFError err;
  uint64_t size;
  uint64_t child_size;
  err = mutff_sound_media_information_header_atom_size(
      &size, &atom->sound_media_information_header);
  if (mutff_is_error(err)) {
    return err;
  }
  err =
      mutff_handler_reference_atom_size(&child_size, &atom->handler_reference);
  if (mutff_is_error(err)) {
    return err;
  }
  size += child_size;
  if (atom->data_information_present) {
    err =
        mutff_data_information_atom_size(&child_size, &atom->data_information);
    if (mutff_is_error(err)) {
      return err;
    }
    size += child_size;
  }
  if (atom->sample_table_present) {
    err = mutff_sample_table_atom_size(&child_size, &atom->sample_table);
    if (mutff_is_error(err)) {
      return err;
    }
    size += child_size;
  }
  *out = mutff_atom_size(size);
  return 0;
}

MuTFFError mutff_write_sound_media_information_atom(
    FILE *fd, const MuTFFSoundMediaInformationAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;

  uint64_t size;
  err = mutff_sound_media_information_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('m', 'i', 'n', 'f'));
  MuTFF_FIELD(mutff_write_sound_media_information_header_atom,
              &in->sound_media_information_header);
  MuTFF_FIELD(mutff_write_handler_reference_atom, &in->handler_reference);
  if (in->data_information_present) {
    MuTFF_FIELD(mutff_write_data_information_atom, &in->data_information);
  }
  if (in->sample_table_present) {
    MuTFF_FIELD(mutff_write_sample_table_atom, &in->sample_table);
  }

  return ret;
}

MuTFFError mutff_read_base_media_info_atom(FILE *fd,
                                           MuTFFBaseMediaInfoAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('g', 'm', 'i', 'n')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FIELD(mutff_read_u8, &out->version);
  MuTFF_FIELD(mutff_read_u24, &out->flags);
  MuTFF_FIELD(mutff_read_u16, &out->graphics_mode);
  for (size_t i = 0; i < 3U; ++i) {
    MuTFF_FIELD(mutff_read_u16, &out->opcolor[i]);
  }
  MuTFF_FIELD(mutff_read_i16, &out->balance);
  MuTFF_SEEK_CUR(2U);
  return ret;
}

static inline MuTFFError mutff_base_media_info_atom_size(
    uint64_t *out, const MuTFFBaseMediaInfoAtom *atom) {
  *out = mutff_atom_size(16);
  return 0;
}

MuTFFError mutff_write_base_media_info_atom(FILE *fd,
                                            const MuTFFBaseMediaInfoAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_base_media_info_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('g', 'm', 'i', 'n'));
  MuTFF_FIELD(mutff_write_u8, in->version);
  MuTFF_FIELD(mutff_write_u24, in->flags);
  MuTFF_FIELD(mutff_write_u16, in->graphics_mode);
  for (size_t i = 0; i < 3U; ++i) {
    MuTFF_FIELD(mutff_write_u16, in->opcolor[i]);
  }
  MuTFF_FIELD(mutff_write_i16, in->balance);
  for (size_t i = 0; i < 2U; ++i) {
    MuTFF_FIELD(mutff_write_u8, 0);
  }
  return ret;
}

MuTFFError mutff_read_text_media_information_atom(
    FILE *fd, MuTFFTextMediaInformationAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('t', 'e', 'x', 't')) {
    return MuTFFErrorBadFormat;
  }
  for (size_t j = 0; j < 3U; ++j) {
    for (size_t i = 0; i < 3U; ++i) {
      MuTFF_FIELD(mutff_read_u32, &out->matrix_structure[j][i]);
    }
  }
  return ret;
}

static inline MuTFFError mutff_text_media_information_atom_size(
    uint64_t *out, const MuTFFTextMediaInformationAtom *atom) {
  *out = mutff_atom_size(36);
  return 0;
}

MuTFFError mutff_write_text_media_information_atom(
    FILE *fd, const MuTFFTextMediaInformationAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_text_media_information_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('t', 'e', 'x', 't'));
  for (size_t j = 0; j < 3U; ++j) {
    for (size_t i = 0; i < 3U; ++i) {
      MuTFF_FIELD(mutff_write_u32, in->matrix_structure[j][i]);
    }
  }
  return ret;
}

MuTFFError mutff_read_base_media_information_header_atom(
    FILE *fd, MuTFFBaseMediaInformationHeaderAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  bool base_media_info_present = false;

  out->text_media_information_present = false;

  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('g', 'm', 'h', 'd')) {
    return MuTFFErrorBadFormat;
  }

  // read child atoms
  uint64_t child_size;
  uint32_t child_type;
  while (ret < size) {
    MuTFF_FIELD(mutff_peek_atom_header, &child_size, &child_type);
    if (size == 0U) {
      return MuTFFErrorBadFormat;
    }
    if (ret + child_size > size) {
      return MuTFFErrorBadFormat;
    }

    switch (child_type) {
      case MuTFF_FOURCC('g', 'm', 'i', 'n'):
        MuTFF_READ_CHILD(mutff_read_base_media_info_atom, &out->base_media_info,
                         base_media_info_present);
        break;
      case MuTFF_FOURCC('t', 'e', 'x', 't'):
        MuTFF_READ_CHILD(mutff_read_text_media_information_atom,
                         &out->text_media_information,
                         out->text_media_information_present);
        break;
      default:
        MuTFF_SEEK_CUR(child_size);
        break;
    }
  }

  return ret;
}

static inline MuTFFError mutff_base_media_information_header_atom_size(
    uint64_t *out, const MuTFFBaseMediaInformationHeaderAtom *atom) {
  MuTFFError err;
  uint64_t size;
  uint64_t child_size;

  err = mutff_base_media_info_atom_size(&size, &atom->base_media_info);
  if (mutff_is_error(err)) {
    return err;
  }
  err = mutff_text_media_information_atom_size(&child_size,
                                               &atom->text_media_information);
  if (mutff_is_error(err)) {
    return err;
  }
  size += child_size;

  *out = mutff_atom_size(size);
  return 0;
}

MuTFFError mutff_write_base_media_information_header_atom(
    FILE *fd, const MuTFFBaseMediaInformationHeaderAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_base_media_information_header_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('g', 'm', 'h', 'd'));
  MuTFF_FIELD(mutff_write_base_media_info_atom, &in->base_media_info);
  MuTFF_FIELD(mutff_write_text_media_information_atom,
              &in->text_media_information);
  return ret;
}

MuTFFError mutff_read_base_media_information_atom(
    FILE *fd, MuTFFBaseMediaInformationAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('m', 'i', 'n', 'f')) {
    return MuTFFErrorBadFormat;
  }

  // read child atom
  MuTFF_FIELD(mutff_read_base_media_information_header_atom,
              &out->base_media_information_header);

  // skip remaining space
  MuTFF_SEEK_CUR(size - ret);

  return ret;
}

static inline MuTFFError mutff_base_media_information_atom_size(
    uint64_t *out, const MuTFFBaseMediaInformationAtom *atom) {
  uint64_t size;
  const MuTFFError err = mutff_base_media_information_header_atom_size(
      &size, &atom->base_media_information_header);
  if (mutff_is_error(err)) {
    return err;
  }
  *out = mutff_atom_size(size);
  return 0;
}

MuTFFError mutff_write_base_media_information_atom(
    FILE *fd, const MuTFFBaseMediaInformationAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  err = mutff_base_media_information_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('m', 'i', 'n', 'f'));
  MuTFF_FIELD(mutff_write_base_media_information_header_atom,
              &in->base_media_information_header);
  return ret;
}

inline MuTFFMediaInformationType mutff_media_information_type(
    MuTFFMediaType media_type) {
  switch (media_type) {
    case MuTFFMediaTypeVideo:
      return MuTFFVideoMediaInformation;
    case MuTFFMediaTypeSound:
      return MuTFFSoundMediaInformation;
    default:
      return MuTFFBaseMediaInformation;
  }
}

static inline bool mutff_is_known_media_type(uint32_t type) {
  switch (type) {
    case MuTFFMediaTypeVideo:
      return true;
    case MuTFFMediaTypeSound:
      return true;
    case MuTFFMediaTypeTimedMetadata:
      return true;
    case MuTFFMediaTypeTextMedia:
      return true;
    case MuTFFMediaTypeClosedCaptioningMedia:
      return true;
    case MuTFFMediaTypeSubtitleMedia:
      return true;
    case MuTFFMediaTypeMusicMedia:
      return true;
    case MuTFFMediaTypeMPEG1Media:
      return true;
    case MuTFFMediaTypeSpriteMedia:
      return true;
    case MuTFFMediaTypeTweenMedia:
      return true;
    case MuTFFMediaType3DMedia:
      return true;
    case MuTFFMediaTypeStreamingMedia:
      return true;
    case MuTFFMediaTypeHintMedia:
      return true;
    case MuTFFMediaTypeVRMedia:
      return true;
    case MuTFFMediaTypePanoramaMedia:
      return true;
    case MuTFFMediaTypeObjectMedia:
      return true;
    default:
      return false;
  }
}

MuTFFError mutff_media_type(MuTFFMediaType *out, const MuTFFMediaAtom *atom) {
  if (!atom->handler_reference_present) {
    return MuTFFErrorBadFormat;
  }
  if (!mutff_is_known_media_type(atom->handler_reference.component_subtype)) {
    return MuTFFErrorBadFormat;
  }
  *out = (MuTFFMediaType)atom->handler_reference.component_subtype;
  return 0;
}

MuTFFError mutff_read_media_atom(FILE *fd, MuTFFMediaAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  bool media_header_present = false;
  long media_information_offset;

  out->extended_language_tag_present = false;
  out->handler_reference_present = false;
  out->media_information_present = false;
  out->user_data_present = false;

  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('m', 'd', 'i', 'a')) {
    return MuTFFErrorBadFormat;
  }

  // read child atoms
  uint64_t child_size;
  uint32_t child_type;
  while (ret < size) {
    MuTFF_FIELD(mutff_peek_atom_header, &child_size, &child_type);
    if (size == 0U) {
      return MuTFFErrorBadFormat;
    }
    if (ret + child_size > size) {
      return MuTFFErrorBadFormat;
    }

    switch (child_type) {
      case MuTFF_FOURCC('m', 'd', 'h', 'd'):
        MuTFF_READ_CHILD(mutff_read_media_header_atom, &out->media_header,
                         media_header_present);
        break;
      case MuTFF_FOURCC('e', 'l', 'n', 'g'):
        MuTFF_READ_CHILD(mutff_read_extended_language_tag_atom,
                         &out->extended_language_tag,
                         out->extended_language_tag_present);
        break;
      case MuTFF_FOURCC('h', 'd', 'l', 'r'):
        MuTFF_READ_CHILD(mutff_read_handler_reference_atom,
                         &out->handler_reference,
                         out->handler_reference_present);
        break;
      case MuTFF_FOURCC('m', 'i', 'n', 'f'):
        if (out->media_information_present) {
          return MuTFFErrorBadFormat;
        }
        errno = 0;
        media_information_offset = ftell(fd);
        if (errno != 0) {
          return MuTFFErrorIOError;
        }
        MuTFF_SEEK_CUR(child_size);
        out->media_information_present = true;
        break;
      case MuTFF_FOURCC('u', 'd', 't', 'a'):
        MuTFF_READ_CHILD(mutff_read_user_data_atom, &out->user_data,
                         out->user_data_present);
        break;
      default:
        MuTFF_SEEK_CUR(child_size);
        break;
    }
  }

  if (!media_header_present) {
    return MuTFFErrorBadFormat;
  }

  errno = 0;
  const long atom_end_offset = ftell(fd);
  if (errno != 0) {
    return MuTFFErrorIOError;
  }
  if (out->media_information_present) {
    MuTFFMediaType media_type;
    err = mutff_media_type(&media_type, out);
    if (mutff_is_error(err)) {
      return err;
    }
    if (fseek(fd, media_information_offset, SEEK_SET) != 0) {
      return MuTFFErrorIOError;
    }
    switch (mutff_media_information_type(media_type)) {
      case MuTFFVideoMediaInformation:
        err = mutff_read_video_media_information_atom(
            fd, &out->video_media_information);
        break;
      case MuTFFSoundMediaInformation:
        err = mutff_read_sound_media_information_atom(
            fd, &out->sound_media_information);
        break;
      case MuTFFBaseMediaInformation:
        err = mutff_read_base_media_information_atom(
            fd, &out->base_media_information);
        break;
      default:
        return MuTFFErrorBadFormat;
    }
    if (mutff_is_error(err)) {
      return err;
    }
    if (fseek(fd, atom_end_offset, SEEK_SET) != 0) {
      return MuTFFErrorIOError;
    }
  }

  return ret;
}

static MuTFFError mutff_media_atom_size(uint64_t *out,
                                        const MuTFFMediaAtom *atom) {
  MuTFFError err;
  uint64_t size;
  uint64_t child_size;
  err = mutff_media_header_atom_size(&size, &atom->media_header);
  if (mutff_is_error(err)) {
    return err;
  }
  if (atom->extended_language_tag_present) {
    err = mutff_extended_language_tag_atom_size(&child_size,
                                                &atom->extended_language_tag);
    if (mutff_is_error(err)) {
      return err;
    }
    size += child_size;
  }
  if (atom->handler_reference_present) {
    err = mutff_handler_reference_atom_size(&child_size,
                                            &atom->handler_reference);
    if (mutff_is_error(err)) {
      return err;
    }
    size += child_size;
  }
  if (atom->media_information_present) {
    MuTFFMediaType type;
    err = mutff_media_type(&type, atom);
    if (mutff_is_error(err)) {
      return err;
    }

    switch (mutff_media_information_type(type)) {
      case MuTFFVideoMediaInformation:
        err = mutff_video_media_information_atom_size(
            &child_size, &atom->video_media_information);
        if (mutff_is_error(err)) {
          return err;
        }
        size += child_size;
        break;
      case MuTFFSoundMediaInformation:
        err = mutff_sound_media_information_atom_size(
            &child_size, &atom->sound_media_information);
        if (mutff_is_error(err)) {
          return err;
        }
        size += child_size;
        break;
      case MuTFFBaseMediaInformation:
        err = mutff_base_media_information_atom_size(
            &child_size, &atom->base_media_information);
        if (mutff_is_error(err)) {
          return err;
        }
        size += child_size;
        break;
      default:
        return err;
        break;
    }
  }
  if (atom->user_data_present) {
    err = mutff_user_data_atom_size(&child_size, &atom->user_data);
    if (mutff_is_error(err)) {
      return err;
    }
    size += child_size;
  }
  *out = mutff_atom_size(size);
  return 0;
}

MuTFFError mutff_write_media_atom(FILE *fd, const MuTFFMediaAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;

  uint64_t size;
  err = mutff_media_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('m', 'd', 'i', 'a'));
  MuTFF_FIELD(mutff_write_media_header_atom, &in->media_header);
  if (in->extended_language_tag_present) {
    MuTFF_FIELD(mutff_write_extended_language_tag_atom,
                &in->extended_language_tag);
  }
  if (in->handler_reference_present) {
    MuTFF_FIELD(mutff_write_handler_reference_atom, &in->handler_reference);
  }
  if (in->media_information_present) {
    MuTFFMediaType type;
    MuTFFError err;
    err = mutff_media_type(&type, in);
    if (mutff_is_error(err)) {
      return err;
    }

    switch (mutff_media_information_type(type)) {
      case MuTFFVideoMediaInformation:
        MuTFF_FIELD(mutff_write_video_media_information_atom,
                    &in->video_media_information);
        break;
      case MuTFFSoundMediaInformation:
        MuTFF_FIELD(mutff_write_sound_media_information_atom,
                    &in->sound_media_information);
        break;
      case MuTFFBaseMediaInformation:
        MuTFF_FIELD(mutff_write_base_media_information_atom,
                    &in->base_media_information);
        break;
      default:
        return MuTFFErrorBadFormat;
    }
  }
  if (in->user_data_present) {
    MuTFF_FIELD(mutff_write_user_data_atom, &in->user_data);
  }

  return ret;
}

MuTFFError mutff_read_track_atom(FILE *fd, MuTFFTrackAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  bool track_header_present = false;
  bool media_present = false;

  out->track_aperture_mode_dimensions_present = false;
  out->clipping_present = false;
  out->track_matte_present = false;
  out->edit_present = false;
  out->track_reference_present = false;
  out->track_exclude_from_autoselection_present = false;
  out->track_load_settings_present = false;
  out->track_input_map_present = false;
  out->user_data_present = false;

  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('t', 'r', 'a', 'k')) {
    return MuTFFErrorBadFormat;
  }

  // read child atoms
  uint64_t child_size;
  uint32_t child_type;
  while (ret < size) {
    MuTFF_FIELD(mutff_peek_atom_header, &child_size, &child_type);
    if (size == 0U) {
      return MuTFFErrorBadFormat;
    }
    if (ret + child_size > size) {
      return MuTFFErrorBadFormat;
    }

    switch (child_type) {
      case MuTFF_FOURCC('t', 'k', 'h', 'd'):
        MuTFF_READ_CHILD(mutff_read_track_header_atom, &out->track_header,
                         track_header_present);
        break;
      case MuTFF_FOURCC('t', 'a', 'p', 't'):
        MuTFF_READ_CHILD(mutff_read_track_aperture_mode_dimensions_atom,
                         &out->track_aperture_mode_dimensions,
                         out->track_aperture_mode_dimensions_present);
        break;
      case MuTFF_FOURCC('c', 'l', 'i', 'p'):
        MuTFF_READ_CHILD(mutff_read_clipping_atom, &out->clipping,
                         out->clipping_present);
        break;
      case MuTFF_FOURCC('m', 'a', 't', 't'):
        MuTFF_READ_CHILD(mutff_read_track_matte_atom, &out->track_matte,
                         out->track_matte_present);
        break;
      case MuTFF_FOURCC('e', 'd', 't', 's'):
        MuTFF_READ_CHILD(mutff_read_edit_atom, &out->edit, out->edit_present);
        break;
      case MuTFF_FOURCC('t', 'r', 'e', 'f'):
        MuTFF_READ_CHILD(mutff_read_track_reference_atom, &out->track_reference,
                         out->track_reference_present);
        break;
      case MuTFF_FOURCC('t', 'x', 'a', 's'):
        MuTFF_READ_CHILD(mutff_read_track_exclude_from_autoselection_atom,
                         &out->track_exclude_from_autoselection,
                         out->track_exclude_from_autoselection_present);
        break;
      case MuTFF_FOURCC('l', 'o', 'a', 'd'):
        MuTFF_READ_CHILD(mutff_read_track_load_settings_atom,
                         &out->track_load_settings,
                         out->track_load_settings_present);
        break;
      case MuTFF_FOURCC('i', 'm', 'a', 'p'):
        MuTFF_READ_CHILD(mutff_read_track_input_map_atom, &out->track_input_map,
                         out->track_input_map_present);
        break;
      case MuTFF_FOURCC('m', 'd', 'i', 'a'):
        MuTFF_READ_CHILD(mutff_read_media_atom, &out->media, media_present);
        break;
      case MuTFF_FOURCC('u', 'd', 't', 'a'):
        MuTFF_READ_CHILD(mutff_read_user_data_atom, &out->user_data,
                         out->user_data_present);
        break;
      default:
        MuTFF_SEEK_CUR(child_size);
        break;
    }
  }

  if (!track_header_present || !media_present) {
    return MuTFFErrorBadFormat;
  }

  return ret;
}

static inline MuTFFError mutff_track_atom_size(uint64_t *out,
                                               const MuTFFTrackAtom *atom) {
  MuTFFError err;
  uint64_t size;
  uint64_t child_size;
  err = mutff_track_header_atom_size(&size, &atom->track_header);
  if (mutff_is_error(err)) {
    return err;
  }
  err = mutff_media_atom_size(&child_size, &atom->media);
  if (mutff_is_error(err)) {
    return err;
  }
  size += child_size;
  if (atom->track_aperture_mode_dimensions_present) {
    err = mutff_track_aperture_mode_dimensions_atom_size(
        &child_size, &atom->track_aperture_mode_dimensions);
    if (mutff_is_error(err)) {
      return err;
    }
    size += child_size;
  }
  if (atom->clipping_present) {
    err = mutff_clipping_atom_size(&child_size, &atom->clipping);
    if (mutff_is_error(err)) {
      return err;
    }
    size += child_size;
  }
  if (atom->track_matte_present) {
    err = mutff_track_matte_atom_size(&child_size, &atom->track_matte);
    if (mutff_is_error(err)) {
      return err;
    }
    size += child_size;
  }
  if (atom->edit_present) {
    err = mutff_edit_atom_size(&child_size, &atom->edit);
    if (mutff_is_error(err)) {
      return err;
    }
    size += child_size;
  }
  if (atom->track_reference_present) {
    err = mutff_track_reference_atom_size(&child_size, &atom->track_reference);
    if (mutff_is_error(err)) {
      return err;
    }
    size += child_size;
  }
  if (atom->track_exclude_from_autoselection_present) {
    err = mutff_track_exclude_from_autoselection_atom_size(
        &child_size, &atom->track_exclude_from_autoselection);
    if (mutff_is_error(err)) {
      return err;
    }
    size += child_size;
  }
  if (atom->track_load_settings_present) {
    err = mutff_track_load_settings_atom_size(&child_size,
                                              &atom->track_load_settings);
    if (mutff_is_error(err)) {
      return err;
    }
    size += child_size;
  }
  if (atom->track_input_map_present) {
    err = mutff_track_input_map_atom_size(&child_size, &atom->track_input_map);
    if (mutff_is_error(err)) {
      return err;
    }
    size += child_size;
  }
  if (atom->user_data_present) {
    err = mutff_user_data_atom_size(&child_size, &atom->user_data);
    if (mutff_is_error(err)) {
      return err;
    }
    size += child_size;
  }
  *out = mutff_atom_size(size);
  return 0;
}

MuTFFError mutff_write_track_atom(FILE *fd, const MuTFFTrackAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;

  uint64_t size;
  err = mutff_track_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('t', 'r', 'a', 'k'));
  MuTFF_FIELD(mutff_write_track_header_atom, &in->track_header);
  MuTFF_FIELD(mutff_write_media_atom, &in->media);
  if (in->track_aperture_mode_dimensions_present) {
    MuTFF_FIELD(mutff_write_track_aperture_mode_dimensions_atom,
                &in->track_aperture_mode_dimensions);
  }
  if (in->clipping_present) {
    MuTFF_FIELD(mutff_write_clipping_atom, &in->clipping);
  }
  if (in->track_matte_present) {
    MuTFF_FIELD(mutff_write_track_matte_atom, &in->track_matte);
  }
  if (in->edit_present) {
    MuTFF_FIELD(mutff_write_edit_atom, &in->edit);
  }
  if (in->track_reference_present) {
    MuTFF_FIELD(mutff_write_track_reference_atom, &in->track_reference);
  }
  if (in->track_exclude_from_autoselection_present) {
    MuTFF_FIELD(mutff_write_track_exclude_from_autoselection_atom,
                &in->track_exclude_from_autoselection);
  }
  if (in->track_load_settings_present) {
    MuTFF_FIELD(mutff_write_track_load_settings_atom, &in->track_load_settings);
  }
  if (in->track_input_map_present) {
    MuTFF_FIELD(mutff_write_track_input_map_atom, &in->track_input_map);
  }
  if (in->user_data_present) {
    MuTFF_FIELD(mutff_write_user_data_atom, &in->user_data);
  }

  return ret;
}

MuTFFError mutff_read_movie_atom(FILE *fd, MuTFFMovieAtom *out) {
  MuTFFError err;
  uint64_t ret = 0;
  uint64_t size;
  uint32_t type;
  bool movie_header_present = false;

  out->track_count = 0;
  out->clipping_present = false;
  out->color_table_present = false;
  out->user_data_present = false;

  MuTFF_FIELD(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('m', 'o', 'o', 'v')) {
    return MuTFFErrorBadFormat;
  }

  // read child atoms
  uint64_t child_size;
  uint32_t child_type;
  while (ret < size) {
    MuTFF_FIELD(mutff_peek_atom_header, &child_size, &child_type);
    if (size == 0U) {
      return MuTFFErrorBadFormat;
    }
    if (ret + child_size > size) {
      return MuTFFErrorBadFormat;
    }

    switch (child_type) {
      case MuTFF_FOURCC('m', 'v', 'h', 'd'):
        MuTFF_READ_CHILD(mutff_read_movie_header_atom, &out->movie_header,
                         movie_header_present);
        break;

      case MuTFF_FOURCC('c', 'l', 'i', 'p'):
        MuTFF_READ_CHILD(mutff_read_clipping_atom, &out->clipping,
                         out->clipping_present);
        break;

      case MuTFF_FOURCC('t', 'r', 'a', 'k'):
        if (out->track_count >= MuTFF_MAX_TRACK_ATOMS) {
          return MuTFFErrorBadFormat;
        }
        MuTFF_FIELD(mutff_read_track_atom, &out->track[out->track_count]);
        out->track_count++;
        break;

      case MuTFF_FOURCC('u', 'd', 't', 'a'):
        MuTFF_READ_CHILD(mutff_read_user_data_atom, &out->user_data,
                         out->user_data_present);
        break;

      case MuTFF_FOURCC('c', 't', 'a', 'b'):
        MuTFF_READ_CHILD(mutff_read_color_table_atom, &out->color_table,
                         out->color_table_present);
        break;

      default:
        // unrecognised atom type - skip as per spec
        MuTFF_SEEK_CUR(child_size);
        break;
    }
  }

  if (!movie_header_present) {
    return MuTFFErrorBadFormat;
  }

  return ret;
}

static inline MuTFFError mutff_movie_atom_size(uint64_t *out,
                                               const MuTFFMovieAtom *atom) {
  MuTFFError err;
  uint64_t size;
  uint64_t child_size;
  err = mutff_movie_header_atom_size(&size, &atom->movie_header);
  if (mutff_is_error(err)) {
    return err;
  }
  for (size_t i = 0; i < atom->track_count; ++i) {
    err = mutff_track_atom_size(&child_size, &atom->track[i]);
    if (mutff_is_error(err)) {
      return err;
    }
    size += child_size;
  }
  if (atom->clipping_present) {
    err = mutff_clipping_atom_size(&child_size, &atom->clipping);
    if (mutff_is_error(err)) {
      return err;
    }
    size += child_size;
  }
  if (atom->color_table_present) {
    err = mutff_color_table_atom_size(&child_size, &atom->color_table);
    if (mutff_is_error(err)) {
      return err;
    }
    size += child_size;
  }
  if (atom->user_data_present) {
    err = mutff_user_data_atom_size(&child_size, &atom->user_data);
    if (mutff_is_error(err)) {
      return err;
    }
    size += child_size;
  }
  *out = mutff_atom_size(size);
  return 0;
}

MuTFFError mutff_write_movie_atom(FILE *fd, const MuTFFMovieAtom *in) {
  MuTFFError err;
  uint64_t ret = 0;

  uint64_t size;
  err = mutff_movie_atom_size(&size, in);
  if (mutff_is_error(err)) {
    return err;
  }
  MuTFF_FIELD(mutff_write_header, size, MuTFF_FOURCC('m', 'o', 'o', 'v'));
  MuTFF_FIELD(mutff_write_movie_header_atom, &in->movie_header);
  for (size_t i = 0; i < in->track_count; ++i) {
    MuTFF_FIELD(mutff_write_track_atom, &in->track[i]);
  }
  if (in->clipping_present) {
    MuTFF_FIELD(mutff_write_clipping_atom, &in->clipping);
  }
  if (in->color_table_present) {
    MuTFF_FIELD(mutff_write_color_table_atom, &in->color_table);
  }
  if (in->user_data_present) {
    MuTFF_FIELD(mutff_write_user_data_atom, &in->user_data);
  }

  return ret;
}

MuTFFError mutff_read_movie_file(FILE *fd, MuTFFMovieFile *out) {
  MuTFFError err;
  uint64_t size;
  uint32_t type;
  uint64_t ret = 0;
  bool movie_present = false;

  out->preview_present = false;
  out->movie_data_count = 0;
  out->free_count = 0;
  out->skip_count = 0;
  out->wide_count = 0;

  rewind(fd);
  MuTFF_FIELD(mutff_peek_atom_header, &size, &type);
  if (type == MuTFF_FOURCC('f', 't', 'y', 'p')) {
    MuTFF_FIELD(mutff_read_file_type_atom, &out->file_type);
    out->file_type_present = true;
  }

  while (!mutff_is_error(mutff_peek_atom_header(fd, &size, &type))) {
    if (size == 0U) {
      return MuTFFErrorBadFormat;
    }

    switch (type) {
      case MuTFF_FOURCC('f', 't', 'y', 'p'):
        return MuTFFErrorBadFormat;

      case MuTFF_FOURCC('m', 'o', 'o', 'v'):
        MuTFF_READ_CHILD(mutff_read_movie_atom, &out->movie, movie_present);
        break;

      case MuTFF_FOURCC('m', 'd', 'a', 't'):
        if (out->movie_data_count >= MuTFF_MAX_MOVIE_DATA_ATOMS) {
          return MuTFFErrorOutOfMemory;
        }
        MuTFF_FIELD(mutff_read_movie_data_atom,
                    &out->movie_data[out->movie_data_count]);
        out->movie_data_count++;
        break;

      case MuTFF_FOURCC('f', 'r', 'e', 'e'):
        if (out->free_count >= MuTFF_MAX_FREE_ATOMS) {
          return MuTFFErrorOutOfMemory;
        }
        MuTFF_FIELD(mutff_read_free_atom, &out->free[out->free_count]);
        out->free_count++;
        break;

      case MuTFF_FOURCC('s', 'k', 'i', 'p'):
        if (out->skip_count >= MuTFF_MAX_SKIP_ATOMS) {
          return MuTFFErrorOutOfMemory;
        }
        MuTFF_FIELD(mutff_read_skip_atom, &out->skip[out->skip_count]);
        out->skip_count++;
        break;

      case MuTFF_FOURCC('w', 'i', 'd', 'e'):
        if (out->wide_count >= MuTFF_MAX_WIDE_ATOMS) {
          return MuTFFErrorOutOfMemory;
        }
        MuTFF_FIELD(mutff_read_wide_atom, &out->wide[out->wide_count]);
        out->wide_count++;
        break;

      case MuTFF_FOURCC('p', 'n', 'o', 't'):
        MuTFF_READ_CHILD(mutff_read_preview_atom, &out->preview,
                         out->preview_present);
        break;

      default:
        // unsupported basic type - skip as per spec
        MuTFF_SEEK_CUR(size);
        break;
    }
  }

  if (!movie_present) {
    return MuTFFErrorBadFormat;
  }

  return ret;
}

MuTFFError mutff_write_movie_file(FILE *fd, const MuTFFMovieFile *in) {
  MuTFFError err;
  uint64_t ret = 0;

  if (in->file_type_present) {
    MuTFF_FIELD(mutff_write_file_type_atom, &in->file_type);
  }
  MuTFF_FIELD(mutff_write_movie_atom, &in->movie);
  for (size_t i = 0; i < in->movie_data_count; ++i) {
    MuTFF_FIELD(mutff_write_movie_data_atom, &in->movie_data[i]);
  }
  for (size_t i = 0; i < in->free_count; ++i) {
    MuTFF_FIELD(mutff_write_free_atom, &in->free[i]);
  }
  for (size_t i = 0; i < in->skip_count; ++i) {
    MuTFF_FIELD(mutff_write_skip_atom, &in->skip[i]);
  }
  for (size_t i = 0; i < in->wide_count; ++i) {
    MuTFF_FIELD(mutff_write_wide_atom, &in->wide[i]);
  }
  if (in->preview_present) {
    MuTFF_FIELD(mutff_write_preview_atom, &in->preview);
  }

  return ret;
}
