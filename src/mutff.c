///
/// @file      mutff.c
/// @author    Frank Plowman <post@frankplowman.com>
/// @brief     MuTFF QuickTime file format library main source file
/// @copyright 2022 Frank Plowman
/// @license   This project is released under the GNU Public License Version 3.
///            For the terms of this license, see [LICENSE.md](LICENSE.md)
///

#include "mutff.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "mutff_stdlib.h"

#define MuTFF_FN(func, ...)              \
  do {                                   \
    err = func(fd, &bytes, __VA_ARGS__); \
    if (err != MuTFFErrorNone) {         \
      return err;                        \
    }                                    \
    *(n) += (bytes);                     \
  } while (0);

#define MuTFF_READ_CHILD(func, field, flag) \
  do {                                      \
    if (flag == true) {                     \
      return MuTFFErrorBadFormat;           \
    }                                       \
    MuTFF_FN(func, field);                  \
    (flag) = true;                          \
  } while (0);

#define MuTFF_SEEK_CUR(offset)    \
  do {                            \
    err = mutff_seek(fd, offset); \
    if (err != MuTFFErrorNone) {  \
      return err;                 \
    }                             \
    *(n) += (offset);             \
  } while (0);

static MuTFFError (*mutff_read)(mutff_file_t *, void *,
                                unsigned int) = mutff_read_stdlib;

void mutff_set_read_fn(MuTFFError (*fn)(mutff_file_t *, void *, unsigned int)) {
  mutff_read = fn;
}

static MuTFFError (*mutff_write)(mutff_file_t *, void *,
                                 unsigned int) = mutff_write_stdlib;

void mutff_set_write_fn(MuTFFError (*fn)(mutff_file_t *, void *,
                                         unsigned int)) {
  mutff_write = fn;
}

static MuTFFError (*mutff_tell)(mutff_file_t *,
                                unsigned int *) = mutff_tell_stdlib;

void mutff_set_tell_fn(MuTFFError (*fn)(mutff_file_t *, unsigned int *)) {
  mutff_tell = fn;
}

static MuTFFError (*mutff_seek)(mutff_file_t *, long) = mutff_seek_stdlib;

void mutff_set_seek_fn(MuTFFError (*fn)(mutff_file_t *, long)) {
  mutff_seek = fn;
}

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

static MuTFFError mutff_read_u8(mutff_file_t *fd, size_t *n, uint8_t *data) {
  const MuTFFError err = mutff_read(fd, data, 1);
  if (err != MuTFFErrorNone) {
    return err;
  }
  *n = 1U;
  return MuTFFErrorNone;
}

static MuTFFError mutff_read_i8(mutff_file_t *fd, size_t *n, int8_t *dest) {
  MuTFFError err;
  uint8_t twos;
  size_t bytes;
  *n = 0;
  *n = 0;

  MuTFF_FN(mutff_read_u8, &twos);
  // convert from twos complement to implementation-defined
  *dest = (twos & 0x7FU) - (twos & 0x80U);

  return MuTFFErrorNone;
}

static MuTFFError mutff_read_u16(mutff_file_t *fd, size_t *n, uint16_t *dest) {
  unsigned char data[2];
  const MuTFFError err = mutff_read(fd, data, 2);
  if (err != MuTFFErrorNone) {
    return err;
  }
  // Convert from network order (big-endian)
  // to host order (implementation-defined).
  *dest = mutff_ntoh_16(data);
  *n = 2U;
  return MuTFFErrorNone;
}

static MuTFFError mutff_read_i16(mutff_file_t *fd, size_t *n, int16_t *dest) {
  MuTFFError err;
  uint16_t twos;
  size_t bytes;
  *n = 0;

  MuTFF_FN(mutff_read_u16, &twos);
  *dest = (twos & 0x7FFFU) - (twos & 0x8000U);

  return MuTFFErrorNone;
}

static MuTFFError mutff_read_u24(mutff_file_t *fd, size_t *n,
                                 mutff_uint24_t *dest) {
  unsigned char data[3];
  const MuTFFError err = mutff_read(fd, data, 3);
  if (err != MuTFFErrorNone) {
    return err;
  }
  *dest = mutff_ntoh_24(data);
  *n = 3U;
  return MuTFFErrorNone;
}

static MuTFFError mutff_read_u32(mutff_file_t *fd, size_t *n, uint32_t *dest) {
  unsigned char data[4];
  const MuTFFError err = mutff_read(fd, data, 4);
  if (err != MuTFFErrorNone) {
    return err;
  }
  *dest = mutff_ntoh_32(data);
  *n = 4U;
  return MuTFFErrorNone;
}

static MuTFFError mutff_read_i32(mutff_file_t *fd, size_t *n, int32_t *dest) {
  MuTFFError err;
  uint32_t twos;
  size_t bytes;
  *n = 0;

  MuTFF_FN(mutff_read_u32, &twos);
  *dest = (twos & 0x7FFFFFFFU) - (twos & 0x80000000U);

  *n = bytes;
  return MuTFFErrorNone;
}

static MuTFFError mutff_read_u64(mutff_file_t *fd, size_t *n, uint64_t *dest) {
  unsigned char data[8];
  const MuTFFError err = mutff_read(fd, data, 8);
  if (err != MuTFFErrorNone) {
    return err;
  }
  *dest = mutff_ntoh_64(data);
  *n = 8U;
  return MuTFFErrorNone;
}

static MuTFFError mutff_write_u8(mutff_file_t *fd, size_t *n, uint8_t data) {
  const MuTFFError err = mutff_write(fd, &data, 1);
  if (err != MuTFFErrorNone) {
    return err;
  }
  *n = 1U;
  return MuTFFErrorNone;
}

static MuTFFError mutff_write_i8(mutff_file_t *fd, size_t *n, int8_t x) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  // ensure number is stored as two's complement
  x = x >= 0 ? x : ~abs(x) + 1;
  MuTFF_FN(mutff_write_u8, x);
  return MuTFFErrorNone;
}

static MuTFFError mutff_write_u16(mutff_file_t *fd, size_t *n, uint16_t x) {
  unsigned char data[2];
  // convert number to network order
  mutff_hton_16(data, x);
  const MuTFFError err = mutff_write(fd, &data, 2);
  if (err != MuTFFErrorNone) {
    return err;
  }
  *n = 2U;
  return MuTFFErrorNone;
}

static inline MuTFFError mutff_write_i16(mutff_file_t *fd, size_t *n,
                                         int16_t x) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  x = x >= 0 ? x : ~abs(x) + 1;
  MuTFF_FN(mutff_write_u16, x);
  return MuTFFErrorNone;
}

static MuTFFError mutff_write_u24(mutff_file_t *fd, size_t *n,
                                  mutff_uint24_t x) {
  unsigned char data[3];
  mutff_hton_24(data, x);
  const MuTFFError err = mutff_write(fd, &data, 3);
  if (err != MuTFFErrorNone) {
    return err;
  }
  *n = 3U;
  return MuTFFErrorNone;
}

static MuTFFError mutff_write_u32(mutff_file_t *fd, size_t *n, uint32_t x) {
  unsigned char data[4];
  mutff_hton_32(data, x);
  const MuTFFError err = mutff_write(fd, &data, 4);
  if (err != MuTFFErrorNone) {
    return err;
  }
  *n = 4U;
  return MuTFFErrorNone;
}

static inline MuTFFError mutff_write_i32(mutff_file_t *fd, size_t *n,
                                         int32_t x) {
  return mutff_write_u32(fd, n, x >= 0 ? x : ~abs(x) + 1);
}

static MuTFFError mutff_write_u64(mutff_file_t *fd, size_t *n, uint32_t x) {
  unsigned char data[8];
  mutff_hton_64(data, x);
  const MuTFFError err = mutff_write(fd, &data, 8);
  if (err != MuTFFErrorNone) {
    return err;
  }
  *n = 8U;
  return MuTFFErrorNone;
}

static MuTFFError mutff_read_q8_8(mutff_file_t *fd, size_t *n,
                                  mutff_q8_8_t *data) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  MuTFF_FN(mutff_read_i8, &data->integral);
  MuTFF_FN(mutff_read_u8, &data->fractional);
  return MuTFFErrorNone;
}

static MuTFFError mutff_write_q8_8(mutff_file_t *fd, size_t *n,
                                   mutff_q8_8_t data) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  MuTFF_FN(mutff_write_i8, data.integral);
  MuTFF_FN(mutff_write_u8, data.fractional);
  return MuTFFErrorNone;
}

static MuTFFError mutff_read_q16_16(mutff_file_t *fd, size_t *n,
                                    mutff_q16_16_t *data) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  MuTFF_FN(mutff_read_i16, &data->integral);
  MuTFF_FN(mutff_read_u16, &data->fractional);
  return MuTFFErrorNone;
}

static MuTFFError mutff_write_q16_16(mutff_file_t *fd, size_t *n,
                                     mutff_q16_16_t data) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  MuTFF_FN(mutff_write_i16, data.integral);
  MuTFF_FN(mutff_write_u16, data.fractional);
  return MuTFFErrorNone;
}

static MuTFFError mutff_read_q2_30(mutff_file_t *fd, size_t *n,
                                   mutff_q2_30_t *data) {
  MuTFFError err;
  size_t bytes;
  *n = 0;

  uint32_t x;
  MuTFF_FN(mutff_read_u32, &x);
  data->integral = ((x & 0x40000000U) >> 30U) - ((x & 0x80000000U) >> 30U);
  data->fractional = x & 0x3FFFFFFFU;

  return MuTFFErrorNone;
}

static MuTFFError mutff_write_q2_30(mutff_file_t *fd, size_t *n,
                                    mutff_q2_30_t data) {
  MuTFFError err;
  size_t bytes;
  *n = 0;

  uint32_t x = 0;
  x += (mutff_int_least30_t)(data.integral >= 0
                                 ? (data.integral & 0x1)
                                 : (~abs(data.integral) & 0x1 + 1))
       << 30U;
  x += data.fractional;
  MuTFF_FN(mutff_write_u32, x);

  return MuTFFErrorNone;
}

static MuTFFError mutff_read_matrix(mutff_file_t *fd, size_t *n,
                                    MuTFFMatrix *matrix) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  MuTFF_FN(mutff_read_q16_16, &matrix->a);
  MuTFF_FN(mutff_read_q16_16, &matrix->b);
  MuTFF_FN(mutff_read_q2_30, &matrix->u);
  MuTFF_FN(mutff_read_q16_16, &matrix->c);
  MuTFF_FN(mutff_read_q16_16, &matrix->d);
  MuTFF_FN(mutff_read_q2_30, &matrix->v);
  MuTFF_FN(mutff_read_q16_16, &matrix->tx);
  MuTFF_FN(mutff_read_q16_16, &matrix->ty);
  MuTFF_FN(mutff_read_q2_30, &matrix->w);
  return MuTFFErrorNone;
}

static MuTFFError mutff_write_matrix(mutff_file_t *fd, size_t *n,
                                     MuTFFMatrix matrix) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  MuTFF_FN(mutff_write_q16_16, matrix.a);
  MuTFF_FN(mutff_write_q16_16, matrix.b);
  MuTFF_FN(mutff_write_q2_30, matrix.u);
  MuTFF_FN(mutff_write_q16_16, matrix.c);
  MuTFF_FN(mutff_write_q16_16, matrix.d);
  MuTFF_FN(mutff_write_q2_30, matrix.v);
  MuTFF_FN(mutff_write_q16_16, matrix.tx);
  MuTFF_FN(mutff_write_q16_16, matrix.ty);
  MuTFF_FN(mutff_write_q2_30, matrix.w);
  return MuTFFErrorNone;
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

static MuTFFError mutff_read_header(mutff_file_t *fd, size_t *n, uint64_t *size,
                                    uint32_t *type) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint32_t short_size;

  MuTFF_FN(mutff_read_u32, &short_size);
  MuTFF_FN(mutff_read_u32, type);
  if (short_size == 1U) {
    MuTFF_FN(mutff_read_u64, size);
  } else {
    *size = short_size;
  }

  return MuTFFErrorNone;
}

static MuTFFError mutff_peek_atom_header(mutff_file_t *fd, size_t *n,
                                         uint64_t *size, uint32_t *type) {
  MuTFFError err;
  size_t bytes;
  *n = 0;

  MuTFF_FN(mutff_read_header, size, type);
  MuTFF_SEEK_CUR(-bytes);

  return MuTFFErrorNone;
}

static MuTFFError mutff_write_header(mutff_file_t *fd, size_t *n, uint64_t size,
                                     uint32_t type) {
  MuTFFError err;
  size_t bytes;
  *n = 0;

  if (size > UINT32_MAX) {
    MuTFF_FN(mutff_write_u32, 1);
    MuTFF_FN(mutff_write_u32, type);
    MuTFF_FN(mutff_write_u64, size);
  } else {
    MuTFF_FN(mutff_write_u32, size);
    MuTFF_FN(mutff_write_u32, type);
  }

  return MuTFFErrorNone;
}

MuTFFError mutff_read_quickdraw_rect(mutff_file_t *fd, size_t *n,
                                     MuTFFQuickDrawRect *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;

  MuTFF_FN(mutff_read_u16, &out->top);
  MuTFF_FN(mutff_read_u16, &out->left);
  MuTFF_FN(mutff_read_u16, &out->bottom);
  MuTFF_FN(mutff_read_u16, &out->right);

  return MuTFFErrorNone;
}

MuTFFError mutff_write_quickdraw_rect(mutff_file_t *fd, size_t *n,
                                      const MuTFFQuickDrawRect *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;

  MuTFF_FN(mutff_write_u16, in->top);
  MuTFF_FN(mutff_write_u16, in->left);
  MuTFF_FN(mutff_write_u16, in->bottom);
  MuTFF_FN(mutff_write_u16, in->right);

  return MuTFFErrorNone;
}

MuTFFError mutff_read_quickdraw_region(mutff_file_t *fd, size_t *n,
                                       MuTFFQuickDrawRegion *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;

  MuTFF_FN(mutff_read_u16, &out->size);
  MuTFF_FN(mutff_read_quickdraw_rect, &out->rect);
  const uint16_t data_size = out->size - *n;
  for (uint16_t i = 0; i < data_size; ++i) {
    MuTFF_FN(mutff_read_u8, (uint8_t *)&out->data[i]);
  }

  return MuTFFErrorNone;
}

MuTFFError mutff_write_quickdraw_region(mutff_file_t *fd, size_t *n,
                                        const MuTFFQuickDrawRegion *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;

  MuTFF_FN(mutff_write_u16, in->size);
  MuTFF_FN(mutff_write_quickdraw_rect, &in->rect);
  for (uint16_t i = 0; i < in->size - 10U; ++i) {
    MuTFF_FN(mutff_write_u8, in->data[i]);
  }

  return MuTFFErrorNone;
}

MuTFFError mutff_read_file_type_atom(mutff_file_t *fd, size_t *n,
                                     MuTFFFileTypeAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;

  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('f', 't', 'y', 'p')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FN(mutff_read_u32, &out->major_brand);
  MuTFF_FN(mutff_read_u32, &out->minor_version);

  // read variable-length data
  out->compatible_brands_count = (size - *n) / 4U;
  if (out->compatible_brands_count > MuTFF_MAX_COMPATIBLE_BRANDS) {
    return MuTFFErrorOutOfMemory;
  }
  for (size_t i = 0; i < out->compatible_brands_count; ++i) {
    MuTFF_FN(mutff_read_u32, &out->compatible_brands[i]);
  }
  MuTFF_SEEK_CUR(size - *n);

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_file_type_atom_size(
    uint64_t *out, const MuTFFFileTypeAtom *atom) {
  *out = mutff_atom_size(8U + 4U * atom->compatible_brands_count);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_file_type_atom(mutff_file_t *fd, size_t *n,
                                      const MuTFFFileTypeAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_file_type_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }

  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('f', 't', 'y', 'p'));
  MuTFF_FN(mutff_write_u32, in->major_brand);
  MuTFF_FN(mutff_write_u32, in->minor_version);
  for (size_t i = 0; i < in->compatible_brands_count; ++i) {
    MuTFF_FN(mutff_write_u32, in->compatible_brands[i]);
  }

  return MuTFFErrorNone;
}

MuTFFError mutff_read_movie_data_atom(mutff_file_t *fd, size_t *n,
                                      MuTFFMovieDataAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;

  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('m', 'd', 'a', 't')) {
    return MuTFFErrorBadFormat;
  }
  out->data_size = mutff_data_size(size);
  MuTFF_SEEK_CUR(size - *n);
  return MuTFFErrorNone;
}

static inline MuTFFError mutff_movie_data_atom_size(
    uint64_t *out, const MuTFFMovieDataAtom *atom) {
  *out = mutff_atom_size(atom->data_size);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_movie_data_atom(mutff_file_t *fd, size_t *n,
                                       const MuTFFMovieDataAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_movie_data_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('m', 'd', 'a', 't'));
  /* for (uint64_t i = 0; i < in->data_size; ++i) { */
  /*   MuTFF_FN(mutff_write_u8, 0); */
  /* } */
  return MuTFFErrorNone;
}

MuTFFError mutff_read_free_atom(mutff_file_t *fd, size_t *n,
                                MuTFFFreeAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('f', 'r', 'e', 'e')) {
    return MuTFFErrorBadFormat;
  }
  out->atom_size = size;
  MuTFF_SEEK_CUR(size - *n);
  return MuTFFErrorNone;
}

static inline MuTFFError mutff_free_atom_size(uint64_t *out,
                                              const MuTFFFreeAtom *atom) {
  *out = atom->atom_size;
  return MuTFFErrorNone;
}

MuTFFError mutff_write_free_atom(mutff_file_t *fd, size_t *n,
                                 const MuTFFFreeAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_free_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('f', 'r', 'e', 'e'));
  for (uint64_t i = 0; i < mutff_data_size(in->atom_size); ++i) {
    MuTFF_FN(mutff_write_u8, 0);
  }
  return MuTFFErrorNone;
}

MuTFFError mutff_read_skip_atom(mutff_file_t *fd, size_t *n,
                                MuTFFSkipAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('s', 'k', 'i', 'p')) {
    return MuTFFErrorBadFormat;
  }
  out->atom_size = size;
  MuTFF_SEEK_CUR(size - *n);
  return MuTFFErrorNone;
}

static inline MuTFFError mutff_skip_atom_size(uint64_t *out,
                                              const MuTFFSkipAtom *atom) {
  *out = atom->atom_size;
  return MuTFFErrorNone;
}

MuTFFError mutff_write_skip_atom(mutff_file_t *fd, size_t *n,
                                 const MuTFFSkipAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_skip_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('s', 'k', 'i', 'p'));
  for (uint64_t i = 0; i < mutff_data_size(in->atom_size); ++i) {
    MuTFF_FN(mutff_write_u8, 0);
  }
  return MuTFFErrorNone;
}

MuTFFError mutff_read_wide_atom(mutff_file_t *fd, size_t *n,
                                MuTFFWideAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('w', 'i', 'd', 'e')) {
    return MuTFFErrorBadFormat;
  }
  out->atom_size = size;
  MuTFF_SEEK_CUR(size - *n);
  return MuTFFErrorNone;
}

static inline MuTFFError mutff_wide_atom_size(uint64_t *out,
                                              const MuTFFWideAtom *atom) {
  *out = atom->atom_size;
  return MuTFFErrorNone;
}

MuTFFError mutff_write_wide_atom(mutff_file_t *fd, size_t *n,
                                 const MuTFFWideAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_wide_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('w', 'i', 'd', 'e'));
  for (uint64_t i = 0; i < mutff_data_size(in->atom_size); ++i) {
    MuTFF_FN(mutff_write_u8, 0);
  }
  return MuTFFErrorNone;
}

MuTFFError mutff_read_preview_atom(mutff_file_t *fd, size_t *n,
                                   MuTFFPreviewAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('p', 'n', 'o', 't')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FN(mutff_read_u32, &out->modification_time);
  MuTFF_FN(mutff_read_u16, &out->version);
  MuTFF_FN(mutff_read_u32, &out->atom_type);
  MuTFF_FN(mutff_read_u16, &out->atom_index);
  MuTFF_SEEK_CUR(size - *n);
  return MuTFFErrorNone;
}

static inline MuTFFError mutff_preview_atom_size(uint64_t *out,
                                                 const MuTFFPreviewAtom *atom) {
  *out = mutff_atom_size(12);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_preview_atom(mutff_file_t *fd, size_t *n,
                                    const MuTFFPreviewAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_preview_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('p', 'n', 'o', 't'));
  MuTFF_FN(mutff_write_u32, in->modification_time);
  MuTFF_FN(mutff_write_u16, in->version);
  MuTFF_FN(mutff_write_u32, in->atom_type);
  MuTFF_FN(mutff_write_u16, in->atom_index);
  return MuTFFErrorNone;
}

MuTFFError mutff_read_movie_header_atom(mutff_file_t *fd, size_t *n,
                                        MuTFFMovieHeaderAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('m', 'v', 'h', 'd')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FN(mutff_read_u8, &out->version);
  MuTFF_FN(mutff_read_u24, &out->flags);
  MuTFF_FN(mutff_read_u32, &out->creation_time);
  MuTFF_FN(mutff_read_u32, &out->modification_time);
  MuTFF_FN(mutff_read_u32, &out->time_scale);
  MuTFF_FN(mutff_read_u32, &out->duration);
  MuTFF_FN(mutff_read_q16_16, &out->preferred_rate);
  MuTFF_FN(mutff_read_q8_8, &out->preferred_volume);
  MuTFF_SEEK_CUR(10U);
  MuTFF_FN(mutff_read_matrix, &out->matrix_structure);
  MuTFF_FN(mutff_read_u32, &out->preview_time);
  MuTFF_FN(mutff_read_u32, &out->preview_duration);
  MuTFF_FN(mutff_read_u32, &out->poster_time);
  MuTFF_FN(mutff_read_u32, &out->selection_time);
  MuTFF_FN(mutff_read_u32, &out->selection_duration);
  MuTFF_FN(mutff_read_u32, &out->current_time);
  MuTFF_FN(mutff_read_u32, &out->next_track_id);
  MuTFF_SEEK_CUR(size - *n);
  return MuTFFErrorNone;
}

static inline MuTFFError mutff_movie_header_atom_size(
    uint64_t *out, const MuTFFMovieHeaderAtom *atom) {
  *out = mutff_atom_size(100);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_movie_header_atom(mutff_file_t *fd, size_t *n,
                                         const MuTFFMovieHeaderAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_movie_header_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('m', 'v', 'h', 'd'));
  MuTFF_FN(mutff_write_u8, in->version);
  MuTFF_FN(mutff_write_u24, in->flags);
  MuTFF_FN(mutff_write_u32, in->creation_time);
  MuTFF_FN(mutff_write_u32, in->modification_time);
  MuTFF_FN(mutff_write_u32, in->time_scale);
  MuTFF_FN(mutff_write_u32, in->duration);
  MuTFF_FN(mutff_write_q16_16, in->preferred_rate);
  MuTFF_FN(mutff_write_q8_8, in->preferred_volume);
  for (size_t i = 0; i < 10U; ++i) {
    MuTFF_FN(mutff_write_u8, 0);
  }
  MuTFF_FN(mutff_write_matrix, in->matrix_structure);
  MuTFF_FN(mutff_write_u32, in->preview_time);
  MuTFF_FN(mutff_write_u32, in->preview_duration);
  MuTFF_FN(mutff_write_u32, in->poster_time);
  MuTFF_FN(mutff_write_u32, in->selection_time);
  MuTFF_FN(mutff_write_u32, in->selection_duration);
  MuTFF_FN(mutff_write_u32, in->current_time);
  MuTFF_FN(mutff_write_u32, in->next_track_id);
  return MuTFFErrorNone;
}

MuTFFError mutff_read_clipping_region_atom(mutff_file_t *fd, size_t *n,
                                           MuTFFClippingRegionAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;

  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('c', 'r', 'g', 'n')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FN(mutff_read_quickdraw_region, &out->region);
  MuTFF_SEEK_CUR(size - *n);

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_clipping_region_atom_size(
    uint64_t *out, const MuTFFClippingRegionAtom *atom) {
  *out = mutff_atom_size(atom->region.size);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_clipping_region_atom(mutff_file_t *fd, size_t *n,
                                            const MuTFFClippingRegionAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_clipping_region_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('c', 'r', 'g', 'n'));
  MuTFF_FN(mutff_write_quickdraw_region, &in->region);
  return MuTFFErrorNone;
}

MuTFFError mutff_read_clipping_atom(mutff_file_t *fd, size_t *n,
                                    MuTFFClippingAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  bool clipping_region_present = false;

  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('c', 'l', 'i', 'p')) {
    return MuTFFErrorBadFormat;
  }
  uint64_t child_size;
  uint32_t child_type;
  while (*n < size) {
    MuTFF_FN(mutff_peek_atom_header, &child_size, &child_type);
    if (size == 0U) {
      return MuTFFErrorBadFormat;
    }
    if (*n + child_size > size) {
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

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_clipping_atom_size(
    uint64_t *out, const MuTFFClippingAtom *atom) {
  uint64_t size;
  const MuTFFError err =
      mutff_clipping_region_atom_size(&size, &atom->clipping_region);
  if (err != MuTFFErrorNone) {
    return err;
  }
  *out = mutff_atom_size(size);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_clipping_atom(mutff_file_t *fd, size_t *n,
                                     const MuTFFClippingAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_clipping_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('c', 'l', 'i', 'p'));
  MuTFF_FN(mutff_write_clipping_region_atom, &in->clipping_region);
  return MuTFFErrorNone;
}

MuTFFError mutff_read_color_table_atom(mutff_file_t *fd, size_t *n,
                                       MuTFFColorTableAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('c', 't', 'a', 'b')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FN(mutff_read_u32, &out->color_table_seed);
  MuTFF_FN(mutff_read_u16, &out->color_table_flags);
  MuTFF_FN(mutff_read_u16, &out->color_table_size);

  // read color array
  const size_t array_size = (out->color_table_size + 1U) * 8U;
  if (array_size != mutff_data_size(size) - 8U) {
    return MuTFFErrorBadFormat;
  }
  for (size_t i = 0; i <= out->color_table_size; ++i) {
    for (size_t j = 0; j < 4U; ++j) {
      MuTFF_FN(mutff_read_u16, &out->color_array[i][j]);
    }
  }
  MuTFF_SEEK_CUR(size - *n);

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_color_table_atom_size(
    uint64_t *out, const MuTFFColorTableAtom *atom) {
  *out = mutff_atom_size(8U + (atom->color_table_size + 1U) * 8U);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_color_table_atom(mutff_file_t *fd, size_t *n,
                                        const MuTFFColorTableAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_color_table_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('c', 't', 'a', 'b'));
  MuTFF_FN(mutff_write_u32, in->color_table_seed);
  MuTFF_FN(mutff_write_u16, in->color_table_flags);
  MuTFF_FN(mutff_write_u16, in->color_table_size);
  for (uint16_t i = 0; i <= in->color_table_size; ++i) {
    MuTFF_FN(mutff_write_u16, in->color_array[i][0]);
    MuTFF_FN(mutff_write_u16, in->color_array[i][1]);
    MuTFF_FN(mutff_write_u16, in->color_array[i][2]);
    MuTFF_FN(mutff_write_u16, in->color_array[i][3]);
  }
  return MuTFFErrorNone;
}

MuTFFError mutff_read_user_data_list_entry(mutff_file_t *fd, size_t *n,
                                           MuTFFUserDataListEntry *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  MuTFF_FN(mutff_read_header, &size, &out->type);

  // read variable-length data
  out->data_size = mutff_data_size(size);
  if (out->data_size > MuTFF_MAX_USER_DATA_ENTRY_SIZE) {
    return MuTFFErrorOutOfMemory;
  }
  for (uint32_t i = 0; i < out->data_size; ++i) {
    MuTFF_FN(mutff_read_u8, (uint8_t *)&out->data[i]);
  }

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_user_data_list_entry_size(
    uint64_t *out, const MuTFFUserDataListEntry *entry) {
  *out = mutff_atom_size(entry->data_size);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_user_data_list_entry(mutff_file_t *fd, size_t *n,
                                            const MuTFFUserDataListEntry *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_user_data_list_entry_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, in->type);
  for (uint32_t i = 0; i < in->data_size; ++i) {
    MuTFF_FN(mutff_write_u8, in->data[i]);
  }
  return MuTFFErrorNone;
}

MuTFFError mutff_read_user_data_atom(mutff_file_t *fd, size_t *n,
                                     MuTFFUserDataAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;

  // read data
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('u', 'd', 't', 'a')) {
    return MuTFFErrorBadFormat;
  }

  // read children
  size_t i = 0;
  uint64_t child_size;
  uint32_t child_type;
  while (*n < size) {
    if (i >= MuTFF_MAX_USER_DATA_ITEMS) {
      return MuTFFErrorOutOfMemory;
    }
    MuTFF_FN(mutff_peek_atom_header, &child_size, &child_type);
    if (size == 0U) {
      return MuTFFErrorBadFormat;
    }
    if (*n + child_size > size) {
      return MuTFFErrorBadFormat;
    }
    MuTFF_FN(mutff_read_user_data_list_entry, &out->user_data_list[i]);

    i++;
  }
  out->list_entries = i;

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_user_data_atom_size(
    uint64_t *out, const MuTFFUserDataAtom *atom) {
  MuTFFError err;
  uint64_t size = 0;
  for (size_t i = 0; i < atom->list_entries; ++i) {
    uint64_t user_data_size;
    err = mutff_user_data_list_entry_size(&user_data_size,
                                          &atom->user_data_list[i]);
    if (err != MuTFFErrorNone) {
      return err;
    }
    size += user_data_size;
  }
  *out = mutff_atom_size(size);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_user_data_atom(mutff_file_t *fd, size_t *n,
                                      const MuTFFUserDataAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_user_data_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('u', 'd', 't', 'a'));
  for (size_t i = 0; i < in->list_entries; ++i) {
    MuTFF_FN(mutff_write_user_data_list_entry, &in->user_data_list[i]);
  }
  return MuTFFErrorNone;
}

MuTFFError mutff_read_track_header_atom(mutff_file_t *fd, size_t *n,
                                        MuTFFTrackHeaderAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('t', 'k', 'h', 'd')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FN(mutff_read_u8, &out->version);
  MuTFF_FN(mutff_read_u24, &out->flags);
  MuTFF_FN(mutff_read_u32, &out->creation_time);
  MuTFF_FN(mutff_read_u32, &out->modification_time);
  MuTFF_FN(mutff_read_u32, &out->track_id);
  MuTFF_SEEK_CUR(4U);
  MuTFF_FN(mutff_read_u32, &out->duration);
  MuTFF_SEEK_CUR(8U);
  MuTFF_FN(mutff_read_u16, &out->layer);
  MuTFF_FN(mutff_read_u16, &out->alternate_group);
  MuTFF_FN(mutff_read_q8_8, &out->volume);
  MuTFF_SEEK_CUR(2U);
  MuTFF_FN(mutff_read_matrix, &out->matrix_structure);
  MuTFF_FN(mutff_read_q16_16, &out->track_width);
  MuTFF_FN(mutff_read_q16_16, &out->track_height);
  MuTFF_SEEK_CUR(size - *n);
  return MuTFFErrorNone;
}

static inline MuTFFError mutff_track_header_atom_size(
    uint64_t *out, const MuTFFTrackHeaderAtom *atom) {
  *out = mutff_atom_size(84);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_track_header_atom(mutff_file_t *fd, size_t *n,
                                         const MuTFFTrackHeaderAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_track_header_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('t', 'k', 'h', 'd'));
  MuTFF_FN(mutff_write_u8, in->version);
  MuTFF_FN(mutff_write_u24, in->flags);
  MuTFF_FN(mutff_write_u32, in->creation_time);
  MuTFF_FN(mutff_write_u32, in->modification_time);
  MuTFF_FN(mutff_write_u32, in->track_id);
  for (size_t i = 0; i < 4U; ++i) {
    MuTFF_FN(mutff_write_u8, 0);
  }
  MuTFF_FN(mutff_write_u32, in->duration);
  for (size_t i = 0; i < 8U; ++i) {
    MuTFF_FN(mutff_write_u8, 0);
  }
  MuTFF_FN(mutff_write_u16, in->layer);
  MuTFF_FN(mutff_write_u16, in->alternate_group);
  MuTFF_FN(mutff_write_q8_8, in->volume);
  for (size_t i = 0; i < 2U; ++i) {
    MuTFF_FN(mutff_write_u8, 0);
  }
  MuTFF_FN(mutff_write_matrix, in->matrix_structure);
  MuTFF_FN(mutff_write_q16_16, in->track_width);
  MuTFF_FN(mutff_write_q16_16, in->track_height);
  return MuTFFErrorNone;
}

MuTFFError mutff_read_track_clean_aperture_dimensions_atom(
    mutff_file_t *fd, size_t *n, MuTFFTrackCleanApertureDimensionsAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('c', 'l', 'e', 'f')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FN(mutff_read_u8, &out->version);
  MuTFF_FN(mutff_read_u24, &out->flags);
  MuTFF_FN(mutff_read_q16_16, &out->width);
  MuTFF_FN(mutff_read_q16_16, &out->height);
  return MuTFFErrorNone;
}

static inline MuTFFError mutff_track_clean_aperture_dimensions_atom_size(
    uint64_t *out, const MuTFFTrackCleanApertureDimensionsAtom *atom) {
  *out = mutff_atom_size(12);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_track_clean_aperture_dimensions_atom(
    mutff_file_t *fd, size_t *n,
    const MuTFFTrackCleanApertureDimensionsAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_track_clean_aperture_dimensions_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('c', 'l', 'e', 'f'));
  MuTFF_FN(mutff_write_u8, in->version);
  MuTFF_FN(mutff_write_u24, in->flags);
  MuTFF_FN(mutff_write_q16_16, in->width);
  MuTFF_FN(mutff_write_q16_16, in->height);
  return MuTFFErrorNone;
}

MuTFFError mutff_read_track_production_aperture_dimensions_atom(
    mutff_file_t *fd, size_t *n,
    MuTFFTrackProductionApertureDimensionsAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('p', 'r', 'o', 'f')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FN(mutff_read_u8, &out->version);
  MuTFF_FN(mutff_read_u24, &out->flags);
  MuTFF_FN(mutff_read_q16_16, &out->width);
  MuTFF_FN(mutff_read_q16_16, &out->height);
  return MuTFFErrorNone;
}

static inline MuTFFError mutff_track_production_aperture_dimensions_atom_size(
    uint64_t *out, const MuTFFTrackProductionApertureDimensionsAtom *atom) {
  *out = mutff_atom_size(12);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_track_production_aperture_dimensions_atom(
    mutff_file_t *fd, size_t *n,
    const MuTFFTrackProductionApertureDimensionsAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_track_production_aperture_dimensions_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('p', 'r', 'o', 'f'));
  MuTFF_FN(mutff_write_u8, in->version);
  MuTFF_FN(mutff_write_u24, in->flags);
  MuTFF_FN(mutff_write_q16_16, in->width);
  MuTFF_FN(mutff_write_q16_16, in->height);
  return MuTFFErrorNone;
}

MuTFFError mutff_read_track_encoded_pixels_dimensions_atom(
    mutff_file_t *fd, size_t *n, MuTFFTrackEncodedPixelsDimensionsAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('e', 'n', 'o', 'f')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FN(mutff_read_u8, &out->version);
  MuTFF_FN(mutff_read_u24, &out->flags);
  MuTFF_FN(mutff_read_q16_16, &out->width);
  MuTFF_FN(mutff_read_q16_16, &out->height);
  return MuTFFErrorNone;
}

static inline MuTFFError mutff_track_encoded_pixels_atom_size(
    uint64_t *out, const MuTFFTrackEncodedPixelsDimensionsAtom *atom) {
  *out = mutff_atom_size(12);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_track_encoded_pixels_dimensions_atom(
    mutff_file_t *fd, size_t *n,
    const MuTFFTrackEncodedPixelsDimensionsAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_track_encoded_pixels_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('e', 'n', 'o', 'f'));
  MuTFF_FN(mutff_write_u8, in->version);
  MuTFF_FN(mutff_write_u24, in->flags);
  MuTFF_FN(mutff_write_q16_16, in->width);
  MuTFF_FN(mutff_write_q16_16, in->height);
  return MuTFFErrorNone;
}

MuTFFError mutff_read_track_aperture_mode_dimensions_atom(
    mutff_file_t *fd, size_t *n, MuTFFTrackApertureModeDimensionsAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  bool track_clean_aperture_dimensions_present = false;
  bool track_production_aperture_dimensions_present = false;
  bool track_encoded_pixels_dimensions_present = false;

  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('t', 'a', 'p', 't')) {
    return MuTFFErrorBadFormat;
  }

  // read children
  uint64_t child_size;
  uint32_t child_type;
  while (*n < size) {
    MuTFF_FN(mutff_peek_atom_header, &child_size, &child_type);
    if (size == 0U) {
      return MuTFFErrorBadFormat;
    }
    if (*n + child_size > size) {
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

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_track_aperture_mode_dimensions_atom_size(
    uint64_t *out, const MuTFFTrackApertureModeDimensionsAtom *atom) {
  *out = mutff_atom_size(60);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_track_aperture_mode_dimensions_atom(
    mutff_file_t *fd, size_t *n,
    const MuTFFTrackApertureModeDimensionsAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_track_aperture_mode_dimensions_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('t', 'a', 'p', 't'));
  MuTFF_FN(mutff_write_track_clean_aperture_dimensions_atom,
           &in->track_clean_aperture_dimensions);
  MuTFF_FN(mutff_write_track_production_aperture_dimensions_atom,
           &in->track_production_aperture_dimensions);
  MuTFF_FN(mutff_write_track_encoded_pixels_dimensions_atom,
           &in->track_encoded_pixels_dimensions);
  return MuTFFErrorNone;
}

MuTFFError mutff_read_sample_description(mutff_file_t *fd, size_t *n,
                                         MuTFFSampleDescription *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint32_t size;
  MuTFF_FN(mutff_read_u32, &size);
  MuTFF_FN(mutff_read_u32, &out->data_format);
  MuTFF_SEEK_CUR(6U);
  MuTFF_FN(mutff_read_u16, &out->data_reference_index);
  MuTFF_FN(mutff_media_type_read_fn(mutff_media_type(out->data_format)),
           &out->data);
  return MuTFFErrorNone;
}

static inline MuTFFError mutff_sample_description_size(
    uint32_t *out, const MuTFFSampleDescription *desc) {
  uint64_t data_size;
  const MuTFFError err = mutff_media_type_size_fn(
      mutff_media_type(desc->data_format))(&data_size, &desc->data);
  if (err != MuTFFErrorNone) {
    return err;
  }
  *out = 16U + data_size;
  return MuTFFErrorNone;
}

MuTFFError mutff_write_sample_description(mutff_file_t *fd, size_t *n,
                                          const MuTFFSampleDescription *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint32_t size;
  err = mutff_sample_description_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_u32, size);
  MuTFF_FN(mutff_write_u32, in->data_format);
  for (size_t i = 0; i < 6U; ++i) {
    MuTFF_FN(mutff_write_u8, 0);
  }
  MuTFF_FN(mutff_write_u16, in->data_reference_index);
  MuTFF_FN(mutff_media_type_write_fn(mutff_media_type(in->data_format)),
           &in->data);
  return MuTFFErrorNone;
}

MuTFFError mutff_read_video_sample_description(
    mutff_file_t *fd, size_t *n, MuTFFVideoSampleDescription *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  MuTFF_FN(mutff_read_u16, &out->version);
  MuTFF_SEEK_CUR(2U);
  MuTFF_FN(mutff_read_u32, &out->vendor);
  MuTFF_FN(mutff_read_u32, &out->temporal_quality);
  MuTFF_FN(mutff_read_u32, &out->spatial_quality);
  MuTFF_FN(mutff_read_u16, &out->width);
  MuTFF_FN(mutff_read_u16, &out->height);
  MuTFF_FN(mutff_read_q16_16, &out->horizontal_resolution);
  MuTFF_FN(mutff_read_q16_16, &out->vertical_resolution);
  MuTFF_SEEK_CUR(4U);
  MuTFF_FN(mutff_read_u16, &out->frame_count);
  for (size_t i = 0; i < 32U; ++i) {
    // @TODO: does this need a non-specific mutff_read_8 function?
    MuTFF_FN(mutff_read_u8, &out->compressor_name[i]);
  }
  MuTFF_FN(mutff_read_u16, &out->depth);
  MuTFF_FN(mutff_read_i16, &out->color_table_id);
  return MuTFFErrorNone;
}

inline MuTFFError mutff_video_sample_description_size(
    uint64_t *out, const MuTFFVideoSampleDescription *desc) {
  *out = 70;
  return MuTFFErrorNone;
}

MuTFFError mutff_write_video_sample_description(
    mutff_file_t *fd, size_t *n, const MuTFFVideoSampleDescription *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  MuTFF_FN(mutff_write_u16, in->version);
  MuTFF_FN(mutff_write_u16, 0);
  MuTFF_FN(mutff_write_u32, in->vendor);
  MuTFF_FN(mutff_write_u32, in->temporal_quality);
  MuTFF_FN(mutff_write_u32, in->spatial_quality);
  MuTFF_FN(mutff_write_u16, in->width);
  MuTFF_FN(mutff_write_u16, in->height);
  MuTFF_FN(mutff_write_q16_16, in->horizontal_resolution);
  MuTFF_FN(mutff_write_q16_16, in->vertical_resolution);
  MuTFF_FN(mutff_write_u32, 0);
  MuTFF_FN(mutff_write_u16, in->frame_count);
  for (size_t i = 0; i < 32U; ++i) {
    MuTFF_FN(mutff_write_u8, in->compressor_name[i]);
  }
  MuTFF_FN(mutff_write_u16, in->depth);
  MuTFF_FN(mutff_write_i16, in->color_table_id);
  return MuTFFErrorNone;
}

MuTFFError mutff_read_compressed_matte_atom(mutff_file_t *fd, size_t *n,
                                            MuTFFCompressedMatteAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('k', 'm', 'a', 't')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FN(mutff_read_u8, &out->version);
  MuTFF_FN(mutff_read_u24, &out->flags);

  // read sample description
  MuTFF_FN(mutff_read_sample_description,
           &out->matte_image_description_structure);

  // read matte data
  uint32_t sample_desc_size;
  err = mutff_sample_description_size(&sample_desc_size,
                                      &out->matte_image_description_structure);
  if (err != MuTFFErrorNone) {
    return err;
  }
  out->matte_data_len = size - *n;
  for (uint32_t i = 0; i < out->matte_data_len; ++i) {
    MuTFF_FN(mutff_read_u8, (uint8_t *)&out->matte_data[i]);
  }

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_compressed_matte_atom_size(
    uint64_t *out, const MuTFFCompressedMatteAtom *atom) {
  uint32_t sample_desc_size;
  const MuTFFError err = mutff_sample_description_size(
      &sample_desc_size, &atom->matte_image_description_structure);
  if (err != MuTFFErrorNone) {
    return err;
  }
  *out = mutff_atom_size(4U + sample_desc_size + atom->matte_data_len);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_compressed_matte_atom(
    mutff_file_t *fd, size_t *n, const MuTFFCompressedMatteAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_compressed_matte_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('k', 'm', 'a', 't'));
  MuTFF_FN(mutff_write_u8, in->version);
  MuTFF_FN(mutff_write_u24, in->flags);
  MuTFF_FN(mutff_write_sample_description,
           &in->matte_image_description_structure);
  for (size_t i = 0; i < in->matte_data_len; ++i) {
    MuTFF_FN(mutff_write_u8, in->matte_data[i]);
  }
  return MuTFFErrorNone;
}

MuTFFError mutff_read_track_matte_atom(mutff_file_t *fd, size_t *n,
                                       MuTFFTrackMatteAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  bool compressed_matte_atom_present = false;

  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('m', 'a', 't', 't')) {
    return MuTFFErrorBadFormat;
  }

  uint64_t child_size;
  uint32_t child_type;
  while (*n < size) {
    MuTFF_FN(mutff_peek_atom_header, &child_size, &child_type);
    if (size == 0U) {
      return MuTFFErrorBadFormat;
    }
    if (*n + child_size > size) {
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

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_track_matte_atom_size(
    uint64_t *out, const MuTFFTrackMatteAtom *atom) {
  uint64_t size;
  const MuTFFError err =
      mutff_compressed_matte_atom_size(&size, &atom->compressed_matte_atom);
  if (err != MuTFFErrorNone) {
    return err;
  }
  *out = mutff_atom_size(size);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_track_matte_atom(mutff_file_t *fd, size_t *n,
                                        const MuTFFTrackMatteAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_track_matte_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('m', 'a', 't', 't'));
  MuTFF_FN(mutff_write_compressed_matte_atom, &in->compressed_matte_atom);
  return MuTFFErrorNone;
}

MuTFFError mutff_read_edit_list_entry(mutff_file_t *fd, size_t *n,
                                      MuTFFEditListEntry *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  MuTFF_FN(mutff_read_u32, &out->track_duration);
  MuTFF_FN(mutff_read_u32, &out->media_time);
  MuTFF_FN(mutff_read_q16_16, &out->media_rate);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_edit_list_entry(mutff_file_t *fd, size_t *n,
                                       const MuTFFEditListEntry *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  MuTFF_FN(mutff_write_u32, in->track_duration);
  MuTFF_FN(mutff_write_u32, in->media_time);
  MuTFF_FN(mutff_write_q16_16, in->media_rate);
  return MuTFFErrorNone;
}

MuTFFError mutff_read_edit_list_atom(mutff_file_t *fd, size_t *n,
                                     MuTFFEditListAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('e', 'l', 's', 't')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FN(mutff_read_u8, &out->version);
  MuTFF_FN(mutff_read_u24, &out->flags);
  MuTFF_FN(mutff_read_u32, &out->number_of_entries);

  // read edit list table
  if (out->number_of_entries > MuTFF_MAX_EDIT_LIST_ENTRIES) {
    return MuTFFErrorOutOfMemory;
  }
  const size_t edit_list_table_size = size - 16U;
  if (edit_list_table_size != out->number_of_entries * 12U) {
    return MuTFFErrorBadFormat;
  }
  for (size_t i = 0; i < out->number_of_entries; ++i) {
    MuTFF_FN(mutff_read_edit_list_entry, &out->edit_list_table[i]);
  }

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_edit_list_atom_size(
    uint64_t *out, const MuTFFEditListAtom *atom) {
  *out = mutff_atom_size(8U + atom->number_of_entries * 12U);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_edit_list_atom(mutff_file_t *fd, size_t *n,
                                      const MuTFFEditListAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_edit_list_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('e', 'l', 's', 't'));
  MuTFF_FN(mutff_write_u8, in->version);
  MuTFF_FN(mutff_write_u24, in->flags);
  MuTFF_FN(mutff_write_u32, in->number_of_entries);
  for (size_t i = 0; i < in->number_of_entries; ++i) {
    MuTFF_FN(mutff_write_edit_list_entry, &in->edit_list_table[i]);
  }
  return MuTFFErrorNone;
}

MuTFFError mutff_read_edit_atom(mutff_file_t *fd, size_t *n,
                                MuTFFEditAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  bool edit_list_present = false;

  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('e', 'd', 't', 's')) {
    return MuTFFErrorBadFormat;
  }

  uint64_t child_size;
  uint32_t child_type;
  while (*n < size) {
    MuTFF_FN(mutff_peek_atom_header, &child_size, &child_type);
    if (size == 0U) {
      return MuTFFErrorBadFormat;
    }
    if (*n + child_size > size) {
      return err;
    }
    if (child_type == MuTFF_FOURCC('e', 'l', 's', 't')) {
      MuTFF_READ_CHILD(mutff_read_edit_list_atom, &out->edit_list_atom,
                       edit_list_present);
    } else {
      MuTFF_SEEK_CUR(child_size);
    }
  }

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_edit_atom_size(uint64_t *out,
                                              const MuTFFEditAtom *atom) {
  uint64_t size;
  const MuTFFError err =
      mutff_edit_list_atom_size(&size, &atom->edit_list_atom);
  if (err != MuTFFErrorNone) {
    return err;
  }
  *out = mutff_atom_size(size);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_edit_atom(mutff_file_t *fd, size_t *n,
                                 const MuTFFEditAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_edit_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('e', 'd', 't', 's'));
  MuTFF_FN(mutff_write_edit_list_atom, &in->edit_list_atom);
  return MuTFFErrorNone;
}

MuTFFError mutff_read_track_reference_type_atom(
    mutff_file_t *fd, size_t *n, MuTFFTrackReferenceTypeAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  MuTFF_FN(mutff_read_header, &size, &out->type);

  // read track references
  if (mutff_data_size(size) % 4U != 0U) {
    return MuTFFErrorBadFormat;
  }
  out->track_id_count = mutff_data_size(size) / 4U;
  if (out->track_id_count > MuTFF_MAX_TRACK_REFERENCE_TYPE_TRACK_IDS) {
    return MuTFFErrorOutOfMemory;
  }
  for (unsigned int i = 0; i < out->track_id_count; ++i) {
    MuTFF_FN(mutff_read_u32, &out->track_ids[i]);
  }

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_track_reference_type_atom_size(
    uint64_t *out, const MuTFFTrackReferenceTypeAtom *atom) {
  *out = mutff_atom_size(4U * atom->track_id_count);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_track_reference_type_atom(
    mutff_file_t *fd, size_t *n, const MuTFFTrackReferenceTypeAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_track_reference_type_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, in->type);
  for (size_t i = 0; i < in->track_id_count; ++i) {
    MuTFF_FN(mutff_write_u32, in->track_ids[i]);
  }
  return MuTFFErrorNone;
}

MuTFFError mutff_read_track_reference_atom(mutff_file_t *fd, size_t *n,
                                           MuTFFTrackReferenceAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('t', 'r', 'e', 'f')) {
    return MuTFFErrorBadFormat;
  }

  // read children
  size_t i = 0;
  uint64_t child_size;
  uint32_t child_type;
  while (*n < size) {
    if (i >= MuTFF_MAX_TRACK_REFERENCE_TYPE_ATOMS) {
      return MuTFFErrorOutOfMemory;
    }
    MuTFF_FN(mutff_peek_atom_header, &child_size, &child_type);
    if (size == 0U) {
      return MuTFFErrorBadFormat;
    }
    if (*n + child_size > size) {
      return MuTFFErrorBadFormat;
    }
    MuTFF_FN(mutff_read_track_reference_type_atom,
             &out->track_reference_type[i]);
    i++;
  }
  out->track_reference_type_count = i;

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_track_reference_atom_size(
    uint64_t *out, const MuTFFTrackReferenceAtom *atom) {
  MuTFFError err;
  uint64_t size = 0;
  for (size_t i = 0; i < atom->track_reference_type_count; ++i) {
    uint64_t reference_size;
    err = mutff_track_reference_type_atom_size(&reference_size,
                                               &atom->track_reference_type[i]);
    if (err != MuTFFErrorNone) {
      return err;
    }
    size += reference_size;
  }
  *out = mutff_atom_size(size);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_track_reference_atom(mutff_file_t *fd, size_t *n,
                                            const MuTFFTrackReferenceAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_track_reference_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('t', 'r', 'e', 'f'));
  for (size_t i = 0; i < in->track_reference_type_count; ++i) {
    MuTFF_FN(mutff_write_track_reference_type_atom,
             &in->track_reference_type[i]);
  }
  return MuTFFErrorNone;
}

MuTFFError mutff_read_track_exclude_from_autoselection_atom(
    mutff_file_t *fd, size_t *n, MuTFFTrackExcludeFromAutoselectionAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('t', 'x', 'a', 's')) {
    return MuTFFErrorBadFormat;
  }
  return MuTFFErrorNone;
}

static inline MuTFFError mutff_track_exclude_from_autoselection_atom_size(
    uint64_t *out, const MuTFFTrackExcludeFromAutoselectionAtom *atom) {
  *out = mutff_atom_size(0);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_track_exclude_from_autoselection_atom(
    mutff_file_t *fd, size_t *n,
    const MuTFFTrackExcludeFromAutoselectionAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_track_exclude_from_autoselection_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('t', 'x', 'a', 's'));
  return MuTFFErrorNone;
}

MuTFFError mutff_read_track_load_settings_atom(
    mutff_file_t *fd, size_t *n, MuTFFTrackLoadSettingsAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('l', 'o', 'a', 'd')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FN(mutff_read_u32, &out->preload_start_time);
  MuTFF_FN(mutff_read_u32, &out->preload_duration);
  MuTFF_FN(mutff_read_u32, &out->preload_flags);
  MuTFF_FN(mutff_read_u32, &out->default_hints);
  return MuTFFErrorNone;
}

static inline MuTFFError mutff_track_load_settings_atom_size(
    uint64_t *out, const MuTFFTrackLoadSettingsAtom *atom) {
  *out = mutff_atom_size(16);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_track_load_settings_atom(
    mutff_file_t *fd, size_t *n, const MuTFFTrackLoadSettingsAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_track_load_settings_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('l', 'o', 'a', 'd'));
  MuTFF_FN(mutff_write_u32, in->preload_start_time);
  MuTFF_FN(mutff_write_u32, in->preload_duration);
  MuTFF_FN(mutff_write_u32, in->preload_flags);
  MuTFF_FN(mutff_write_u32, in->default_hints);
  return MuTFFErrorNone;
}

MuTFFError mutff_read_input_type_atom(mutff_file_t *fd, size_t *n,
                                      MuTFFInputTypeAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('\0', '\0', 't', 'y')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FN(mutff_read_u32, &out->input_type);
  return MuTFFErrorNone;
}

static inline MuTFFError mutff_input_type_atom_size(
    uint64_t *out, const MuTFFInputTypeAtom *atom) {
  *out = mutff_atom_size(4);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_input_type_atom(mutff_file_t *fd, size_t *n,
                                       const MuTFFInputTypeAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_input_type_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('\0', '\0', 't', 'y'));
  MuTFF_FN(mutff_write_u32, in->input_type);
  return MuTFFErrorNone;
}

MuTFFError mutff_read_object_id_atom(mutff_file_t *fd, size_t *n,
                                     MuTFFObjectIDAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('o', 'b', 'i', 'd')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FN(mutff_read_u32, &out->object_id);
  return MuTFFErrorNone;
}

static inline MuTFFError mutff_object_id_atom_size(
    uint64_t *out, const MuTFFObjectIDAtom *atom) {
  *out = mutff_atom_size(4);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_object_id_atom(mutff_file_t *fd, size_t *n,
                                      const MuTFFObjectIDAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_object_id_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('o', 'b', 'i', 'd'));
  MuTFF_FN(mutff_write_u32, in->object_id);
  return MuTFFErrorNone;
}

MuTFFError mutff_read_track_input_atom(mutff_file_t *fd, size_t *n,
                                       MuTFFTrackInputAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  bool input_type_present = false;

  out->object_id_atom_present = false;

  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('\0', '\0', 'i', 'n')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FN(mutff_read_u32, &out->atom_id);
  MuTFF_SEEK_CUR(2U);
  MuTFF_FN(mutff_read_u16, &out->child_count);
  MuTFF_SEEK_CUR(4U);

  // read children
  uint64_t child_size;
  uint32_t child_type;
  while (*n < size) {
    MuTFF_FN(mutff_peek_atom_header, &child_size, &child_type);
    if (size == 0U) {
      return MuTFFErrorBadFormat;
    }
    if (*n + child_size > size) {
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

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_track_input_atom_size(
    uint64_t *out, const MuTFFTrackInputAtom *atom) {
  MuTFFError err;
  uint64_t child_size;

  uint64_t size = 12;
  err = mutff_input_type_atom_size(&child_size, &atom->input_type_atom);
  if (err != MuTFFErrorNone) {
    return err;
  }
  size += child_size;
  if (atom->object_id_atom_present) {
    err = mutff_object_id_atom_size(&child_size, &atom->object_id_atom);
    if (err != MuTFFErrorNone) {
      return err;
    }
    size += child_size;
  }

  *out = mutff_atom_size(size);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_track_input_atom(mutff_file_t *fd, size_t *n,
                                        const MuTFFTrackInputAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_track_input_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('\0', '\0', 'i', 'n'));
  MuTFF_FN(mutff_write_u32, in->atom_id);
  for (size_t i = 0; i < 2U; ++i) {
    MuTFF_FN(mutff_write_u8, 0);
  }
  MuTFF_FN(mutff_write_u16, in->child_count);
  for (size_t i = 0; i < 4U; ++i) {
    MuTFF_FN(mutff_write_u8, 0);
  }
  MuTFF_FN(mutff_write_input_type_atom, &in->input_type_atom);
  MuTFF_FN(mutff_write_object_id_atom, &in->object_id_atom);
  return MuTFFErrorNone;
}

MuTFFError mutff_read_track_input_map_atom(mutff_file_t *fd, size_t *n,
                                           MuTFFTrackInputMapAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('i', 'm', 'a', 'p')) {
    return MuTFFErrorBadFormat;
  }

  // read children
  size_t i = 0;
  uint64_t child_size;
  uint32_t child_type;
  while (*n < size) {
    if (i >= MuTFF_MAX_TRACK_REFERENCE_TYPE_ATOMS) {
      return MuTFFErrorOutOfMemory;
    }
    MuTFF_FN(mutff_peek_atom_header, &child_size, &child_type);
    if (size == 0U) {
      return MuTFFErrorBadFormat;
    }
    if (*n + child_size > size) {
      return MuTFFErrorBadFormat;
    }
    if (child_type == MuTFF_FOURCC('\0', '\0', 'i', 'n')) {
      MuTFF_FN(mutff_read_track_input_atom, &out->track_input_atoms[i]);
      i++;
    } else {
      MuTFF_SEEK_CUR(child_size);
    }
  }
  out->track_input_atom_count = i;

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_track_input_map_atom_size(
    uint64_t *out, const MuTFFTrackInputMapAtom *atom) {
  MuTFFError err;
  uint64_t size = 0;
  for (size_t i = 0; i < atom->track_input_atom_count; ++i) {
    uint64_t child_size;
    err = mutff_track_input_atom_size(&child_size, &atom->track_input_atoms[i]);
    if (err != MuTFFErrorNone) {
      return err;
    }
    size += child_size;
  }
  *out = mutff_atom_size(size);
  return 0;
}

MuTFFError mutff_write_track_input_map_atom(mutff_file_t *fd, size_t *n,
                                            const MuTFFTrackInputMapAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_track_input_map_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('i', 'm', 'a', 'p'));
  for (size_t i = 0; i < in->track_input_atom_count; ++i) {
    MuTFF_FN(mutff_write_track_input_atom, &in->track_input_atoms[i]);
  }
  return MuTFFErrorNone;
}

MuTFFError mutff_read_media_header_atom(mutff_file_t *fd, size_t *n,
                                        MuTFFMediaHeaderAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('m', 'd', 'h', 'd')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FN(mutff_read_u8, &out->version);
  MuTFF_FN(mutff_read_u24, &out->flags);
  MuTFF_FN(mutff_read_u32, &out->creation_time);
  MuTFF_FN(mutff_read_u32, &out->modification_time);
  MuTFF_FN(mutff_read_u32, &out->time_scale);
  MuTFF_FN(mutff_read_u32, &out->duration);
  MuTFF_FN(mutff_read_u16, &out->language);
  MuTFF_FN(mutff_read_u16, &out->quality);
  return MuTFFErrorNone;
}

static inline MuTFFError mutff_media_header_atom_size(
    uint64_t *out, const MuTFFMediaHeaderAtom *atom) {
  *out = mutff_atom_size(24);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_media_header_atom(mutff_file_t *fd, size_t *n,
                                         const MuTFFMediaHeaderAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_media_header_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('m', 'd', 'h', 'd'));
  MuTFF_FN(mutff_write_u8, in->version);
  MuTFF_FN(mutff_write_u24, in->flags);
  MuTFF_FN(mutff_write_u32, in->creation_time);
  MuTFF_FN(mutff_write_u32, in->modification_time);
  MuTFF_FN(mutff_write_u32, in->time_scale);
  MuTFF_FN(mutff_write_u32, in->duration);
  MuTFF_FN(mutff_write_u16, in->language);
  MuTFF_FN(mutff_write_u16, in->quality);
  return MuTFFErrorNone;
}

MuTFFError mutff_read_extended_language_tag_atom(
    mutff_file_t *fd, size_t *n, MuTFFExtendedLanguageTagAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('e', 'l', 'n', 'g')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FN(mutff_read_u8, &out->version);
  MuTFF_FN(mutff_read_u24, &out->flags);

  // read variable-length data
  const size_t tag_length = size - 12U;
  if (tag_length > MuTFF_MAX_LANGUAGE_TAG_LENGTH) {
    return MuTFFErrorOutOfMemory;
  }
  for (size_t i = 0; i < tag_length; ++i) {
    MuTFF_FN(mutff_read_u8, (uint8_t *)&out->language_tag_string[i]);
  }

  return MuTFFErrorNone;
}

// @TODO: should this round up to a multiple of four for performance reasons?
//        this particular string is zero-terminated so should be possible.
static inline MuTFFError mutff_extended_language_tag_atom_size(
    uint64_t *out, const MuTFFExtendedLanguageTagAtom *atom) {
  *out = mutff_atom_size(4U + strlen(atom->language_tag_string) + 1U);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_extended_language_tag_atom(
    mutff_file_t *fd, size_t *n, const MuTFFExtendedLanguageTagAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  size_t i;
  uint64_t size;
  err = mutff_extended_language_tag_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('e', 'l', 'n', 'g'));
  MuTFF_FN(mutff_write_u8, in->version);
  MuTFF_FN(mutff_write_u24, in->flags);
  i = 0;
  while (in->language_tag_string[i] != (char)'\0') {
    MuTFF_FN(mutff_write_u8, in->language_tag_string[i]);
    ++i;
  }
  for (; i < mutff_data_size(size) - 4U; ++i) {
    MuTFF_FN(mutff_write_u8, 0);
  }
  return MuTFFErrorNone;
}

MuTFFError mutff_read_handler_reference_atom(mutff_file_t *fd, size_t *n,
                                             MuTFFHandlerReferenceAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  size_t i;
  size_t name_length;

  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('h', 'd', 'l', 'r')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FN(mutff_read_u8, &out->version);
  MuTFF_FN(mutff_read_u24, &out->flags);
  MuTFF_FN(mutff_read_u32, &out->component_type);
  MuTFF_FN(mutff_read_u32, &out->component_subtype);
  MuTFF_FN(mutff_read_u32, &out->component_manufacturer);
  MuTFF_FN(mutff_read_u32, &out->component_flags);
  MuTFF_FN(mutff_read_u32, &out->component_flags_mask);

  // read variable-length data
  name_length = size - *n;
  if (name_length > MuTFF_MAX_COMPONENT_NAME_LENGTH) {
    return MuTFFErrorOutOfMemory;
  }
  for (i = 0; i < name_length; ++i) {
    MuTFF_FN(mutff_read_u8, (uint8_t *)&out->component_name[i]);
  }
  out->component_name[i] = '\0';

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_handler_reference_atom_size(
    uint64_t *out, const MuTFFHandlerReferenceAtom *atom) {
  *out = mutff_atom_size(24U + strlen(atom->component_name));
  return MuTFFErrorNone;
}

MuTFFError mutff_write_handler_reference_atom(
    mutff_file_t *fd, size_t *n, const MuTFFHandlerReferenceAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_handler_reference_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('h', 'd', 'l', 'r'));
  MuTFF_FN(mutff_write_u8, in->version);
  MuTFF_FN(mutff_write_u24, in->flags);
  MuTFF_FN(mutff_write_u32, in->component_type);
  MuTFF_FN(mutff_write_u32, in->component_subtype);
  MuTFF_FN(mutff_write_u32, in->component_manufacturer);
  MuTFF_FN(mutff_write_u32, in->component_flags);
  MuTFF_FN(mutff_write_u32, in->component_flags_mask);
  for (size_t i = 0; i < size - 32U; ++i) {
    MuTFF_FN(mutff_write_u8, in->component_name[i]);
  }
  return MuTFFErrorNone;
}

MuTFFError mutff_read_video_media_information_header_atom(
    mutff_file_t *fd, size_t *n, MuTFFVideoMediaInformationHeaderAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('v', 'm', 'h', 'd')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FN(mutff_read_u8, &out->version);
  MuTFF_FN(mutff_read_u24, &out->flags);
  MuTFF_FN(mutff_read_u16, &out->graphics_mode);
  for (size_t i = 0; i < 3U; ++i) {
    MuTFF_FN(mutff_read_u16, &out->opcolor[i]);
  }
  return MuTFFErrorNone;
}

static inline MuTFFError mutff_video_media_information_header_atom_size(
    uint64_t *out, const MuTFFVideoMediaInformationHeaderAtom *atom) {
  *out = mutff_atom_size(12);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_video_media_information_header_atom(
    mutff_file_t *fd, size_t *n,
    const MuTFFVideoMediaInformationHeaderAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_video_media_information_header_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('v', 'm', 'h', 'd'));
  MuTFF_FN(mutff_write_u8, in->version);
  MuTFF_FN(mutff_write_u24, in->flags);
  MuTFF_FN(mutff_write_u16, in->graphics_mode);
  for (size_t i = 0; i < 3U; ++i) {
    MuTFF_FN(mutff_write_u16, in->opcolor[i]);
  }
  return MuTFFErrorNone;
}

MuTFFError mutff_read_data_reference(mutff_file_t *fd, size_t *n,
                                     MuTFFDataReference *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint32_t size;
  MuTFF_FN(mutff_read_u32, &size);
  MuTFF_FN(mutff_read_u32, &out->type);
  MuTFF_FN(mutff_read_u8, &out->version);
  MuTFF_FN(mutff_read_u24, &out->flags);

  // read variable-length data
  out->data_size = size - 12U;
  if (out->data_size > MuTFF_MAX_DATA_REFERENCE_DATA_SIZE) {
    return MuTFFErrorOutOfMemory;
  }
  for (size_t i = 0; i < out->data_size; ++i) {
    MuTFF_FN(mutff_read_u8, (uint8_t *)&out->data[i]);
  }

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_data_reference_size(
    uint64_t *out, const MuTFFDataReference *ref) {
  *out = 12U + ref->data_size;
  return MuTFFErrorNone;
}

MuTFFError mutff_write_data_reference(mutff_file_t *fd, size_t *n,
                                      const MuTFFDataReference *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_data_reference_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, in->type);
  MuTFF_FN(mutff_write_u8, in->version);
  MuTFF_FN(mutff_write_u24, in->flags);
  for (size_t i = 0; i < in->data_size; ++i) {
    MuTFF_FN(mutff_write_u8, in->data[i]);
  }
  return MuTFFErrorNone;
}

MuTFFError mutff_read_data_reference_atom(mutff_file_t *fd, size_t *n,
                                          MuTFFDataReferenceAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;

  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('d', 'r', 'e', 'f')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FN(mutff_read_u8, &out->version);
  MuTFF_FN(mutff_read_u24, &out->flags);
  MuTFF_FN(mutff_read_u32, &out->number_of_entries);

  // read child atoms
  if (out->number_of_entries > MuTFF_MAX_DATA_REFERENCES) {
    return MuTFFErrorOutOfMemory;
  }

  uint64_t child_size;
  uint32_t child_type;
  for (size_t i = 0; i < out->number_of_entries; ++i) {
    MuTFF_FN(mutff_peek_atom_header, &child_size, &child_type);
    if (*n + child_size > size) {
      return MuTFFErrorBadFormat;
    }
    MuTFF_FN(mutff_read_data_reference, &out->data_references[i]);
  }

  // skip any remaining space
  MuTFF_SEEK_CUR(size - *n);

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_data_reference_atom_size(
    uint64_t *out, const MuTFFDataReferenceAtom *atom) {
  MuTFFError err;
  uint64_t size = 8;
  for (uint32_t i = 0; i < atom->number_of_entries; ++i) {
    uint64_t child_size;
    err = mutff_data_reference_size(&child_size, &atom->data_references[i]);
    if (err != MuTFFErrorNone) {
      return err;
    }
    size += child_size;
  }
  *out = mutff_atom_size(size);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_data_reference_atom(mutff_file_t *fd, size_t *n,
                                           const MuTFFDataReferenceAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_data_reference_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('d', 'r', 'e', 'f'));
  MuTFF_FN(mutff_write_u8, in->version);
  MuTFF_FN(mutff_write_u24, in->flags);
  MuTFF_FN(mutff_write_u32, in->number_of_entries);
  for (uint32_t i = 0; i < in->number_of_entries; ++i) {
    MuTFF_FN(mutff_write_data_reference, &in->data_references[i]);
  }
  return MuTFFErrorNone;
}

MuTFFError mutff_read_data_information_atom(mutff_file_t *fd, size_t *n,
                                            MuTFFDataInformationAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  bool data_reference_present = false;

  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('d', 'i', 'n', 'f')) {
    return MuTFFErrorBadFormat;
  }

  uint64_t child_size;
  uint32_t child_type;
  while (*n < size) {
    MuTFF_FN(mutff_peek_atom_header, &child_size, &child_type);
    if (size == 0U) {
      return MuTFFErrorBadFormat;
    }
    if (*n + child_size > size) {
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

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_data_information_atom_size(
    uint64_t *out, const MuTFFDataInformationAtom *atom) {
  uint64_t size;
  const MuTFFError err =
      mutff_data_reference_atom_size(&size, &atom->data_reference);
  if (err != MuTFFErrorNone) {
    return err;
  }
  *out = mutff_atom_size(size);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_data_information_atom(
    mutff_file_t *fd, size_t *n, const MuTFFDataInformationAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_data_information_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('d', 'i', 'n', 'f'));
  MuTFF_FN(mutff_write_data_reference_atom, &in->data_reference);
  return MuTFFErrorNone;
}

MuTFFError mutff_read_sample_description_atom(mutff_file_t *fd, size_t *n,
                                              MuTFFSampleDescriptionAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('s', 't', 's', 'd')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FN(mutff_read_u8, &out->version);
  MuTFF_FN(mutff_read_u24, &out->flags);
  MuTFF_FN(mutff_read_u32, &out->number_of_entries);

  // read child atoms
  if (out->number_of_entries > MuTFF_MAX_SAMPLE_DESCRIPTION_TABLE_LEN) {
    return MuTFFErrorOutOfMemory;
  }

  uint64_t child_size;
  uint32_t child_type;
  for (size_t i = 0; i < out->number_of_entries; ++i) {
    MuTFF_FN(mutff_peek_atom_header, &child_size, &child_type);
    if (*n + child_size > size) {
      return MuTFFErrorBadFormat;
    }
    MuTFF_FN(mutff_read_sample_description, &out->sample_description_table[i]);
  }

  // skip any remaining space
  MuTFF_SEEK_CUR(size - *n);

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_sample_description_atom_size(
    uint64_t *out, const MuTFFSampleDescriptionAtom *atom) {
  MuTFFError err;
  uint64_t size = 0;
  for (uint32_t i = 0; i < atom->number_of_entries; ++i) {
    uint32_t desc_size;
    err = mutff_sample_description_size(&desc_size,
                                        &atom->sample_description_table[i]);
    if (err != MuTFFErrorNone) {
      return err;
    }
    size += desc_size;
  }
  *out = mutff_atom_size(8U + size);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_sample_description_atom(
    mutff_file_t *fd, size_t *n, const MuTFFSampleDescriptionAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_sample_description_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('s', 't', 's', 'd'));
  MuTFF_FN(mutff_write_u8, in->version);
  MuTFF_FN(mutff_write_u24, in->flags);
  MuTFF_FN(mutff_write_u32, in->number_of_entries);
  for (size_t i = 0; i < in->number_of_entries; ++i) {
    uint32_t desc_size;
    err = mutff_sample_description_size(&desc_size,
                                        &in->sample_description_table[i]);
    if (err != MuTFFErrorNone) {
      return err;
    }
    MuTFF_FN(mutff_write_sample_description, &in->sample_description_table[i]);
  }
  return MuTFFErrorNone;
}

MuTFFError mutff_read_time_to_sample_table_entry(
    mutff_file_t *fd, size_t *n, MuTFFTimeToSampleTableEntry *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  MuTFF_FN(mutff_read_u32, &out->sample_count);
  MuTFF_FN(mutff_read_u32, &out->sample_duration);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_time_to_sample_table_entry(
    mutff_file_t *fd, size_t *n, const MuTFFTimeToSampleTableEntry *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  MuTFF_FN(mutff_write_u32, in->sample_count);
  MuTFF_FN(mutff_write_u32, in->sample_duration);
  return MuTFFErrorNone;
}

MuTFFError mutff_read_time_to_sample_atom(mutff_file_t *fd, size_t *n,
                                          MuTFFTimeToSampleAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('s', 't', 't', 's')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FN(mutff_read_u8, &out->version);
  MuTFF_FN(mutff_read_u24, &out->flags);
  MuTFF_FN(mutff_read_u32, &out->number_of_entries);

  // read time to sample table
  if (out->number_of_entries > MuTFF_MAX_TIME_TO_SAMPLE_TABLE_LEN) {
    return MuTFFErrorOutOfMemory;
  }
  const size_t table_size = mutff_data_size(size) - 8U;
  if (table_size != out->number_of_entries * 8U) {
    return MuTFFErrorBadFormat;
  }
  for (size_t i = 0; i < out->number_of_entries; ++i) {
    MuTFF_FN(mutff_read_time_to_sample_table_entry,
             &out->time_to_sample_table[i]);
  }

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_time_to_sample_atom_size(
    uint64_t *out, const MuTFFTimeToSampleAtom *atom) {
  *out = mutff_atom_size(8U + atom->number_of_entries * 8U);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_time_to_sample_atom(mutff_file_t *fd, size_t *n,
                                           const MuTFFTimeToSampleAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_time_to_sample_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('s', 't', 't', 's'));
  MuTFF_FN(mutff_write_u8, in->version);
  MuTFF_FN(mutff_write_u24, in->flags);
  MuTFF_FN(mutff_write_u32, in->number_of_entries);
  if (in->number_of_entries * 8U != mutff_data_size(size) - 8U) {
    return MuTFFErrorBadFormat;
  }
  for (uint32_t i = 0; i < in->number_of_entries; ++i) {
    MuTFF_FN(mutff_write_time_to_sample_table_entry,
             &in->time_to_sample_table[i]);
  }
  return MuTFFErrorNone;
}

MuTFFError mutff_read_composition_offset_table_entry(
    mutff_file_t *fd, size_t *n, MuTFFCompositionOffsetTableEntry *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  MuTFF_FN(mutff_read_u32, &out->sample_count);
  MuTFF_FN(mutff_read_u32, &out->composition_offset);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_composition_offset_table_entry(
    mutff_file_t *fd, size_t *n, const MuTFFCompositionOffsetTableEntry *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  MuTFF_FN(mutff_write_u32, in->sample_count);
  MuTFF_FN(mutff_write_u32, in->composition_offset);
  return MuTFFErrorNone;
}

MuTFFError mutff_read_composition_offset_atom(mutff_file_t *fd, size_t *n,
                                              MuTFFCompositionOffsetAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('c', 't', 't', 's')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FN(mutff_read_u8, &out->version);
  MuTFF_FN(mutff_read_u24, &out->flags);
  MuTFF_FN(mutff_read_u32, &out->entry_count);

  // read composition offset table
  if (out->entry_count > MuTFF_MAX_COMPOSITION_OFFSET_TABLE_LEN) {
    return MuTFFErrorOutOfMemory;
  }
  const size_t table_size = mutff_data_size(size) - 8U;
  if (table_size != out->entry_count * 8U) {
    return MuTFFErrorBadFormat;
  }
  for (size_t i = 0; i < out->entry_count; ++i) {
    MuTFF_FN(mutff_read_composition_offset_table_entry,
             &out->composition_offset_table[i]);
    ;
  }

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_composition_offset_atom_size(
    uint64_t *out, const MuTFFCompositionOffsetAtom *atom) {
  *out = mutff_atom_size(8U + 8U * atom->entry_count);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_composition_offset_atom(
    mutff_file_t *fd, size_t *n, const MuTFFCompositionOffsetAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_composition_offset_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('c', 't', 't', 's'));
  MuTFF_FN(mutff_write_u8, in->version);
  MuTFF_FN(mutff_write_u24, in->flags);
  MuTFF_FN(mutff_write_u32, in->entry_count);
  if (in->entry_count * 8U != mutff_data_size(size) - 8U) {
    return MuTFFErrorBadFormat;
  }
  for (uint32_t i = 0; i < in->entry_count; ++i) {
    MuTFF_FN(mutff_write_composition_offset_table_entry,
             &in->composition_offset_table[i]);
  }
  return MuTFFErrorNone;
}

MuTFFError mutff_read_composition_shift_least_greatest_atom(
    mutff_file_t *fd, size_t *n, MuTFFCompositionShiftLeastGreatestAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('c', 's', 'l', 'g')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FN(mutff_read_u8, &out->version);
  MuTFF_FN(mutff_read_u24, &out->flags);
  MuTFF_FN(mutff_read_u32, &out->composition_offset_to_display_offset_shift);
  MuTFF_FN(mutff_read_i32, &out->least_display_offset);
  MuTFF_FN(mutff_read_i32, &out->greatest_display_offset);
  MuTFF_FN(mutff_read_i32, &out->display_start_time);
  MuTFF_FN(mutff_read_i32, &out->display_end_time);
  return MuTFFErrorNone;
}

static inline MuTFFError mutff_composition_shift_least_greatest_atom_size(
    uint64_t *out, const MuTFFCompositionShiftLeastGreatestAtom *atom) {
  *out = mutff_atom_size(24);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_composition_shift_least_greatest_atom(
    mutff_file_t *fd, size_t *n,
    const MuTFFCompositionShiftLeastGreatestAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_composition_shift_least_greatest_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('c', 's', 'l', 'g'));
  MuTFF_FN(mutff_write_u8, in->version);
  MuTFF_FN(mutff_write_u24, in->flags);
  MuTFF_FN(mutff_write_u32, in->composition_offset_to_display_offset_shift);
  MuTFF_FN(mutff_write_i32, in->least_display_offset);
  MuTFF_FN(mutff_write_i32, in->greatest_display_offset);
  MuTFF_FN(mutff_write_i32, in->display_start_time);
  MuTFF_FN(mutff_write_i32, in->display_end_time);
  return MuTFFErrorNone;
}

MuTFFError mutff_read_sync_sample_atom(mutff_file_t *fd, size_t *n,
                                       MuTFFSyncSampleAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('s', 't', 's', 's')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FN(mutff_read_u8, &out->version);
  MuTFF_FN(mutff_read_u24, &out->flags);
  MuTFF_FN(mutff_read_u32, &out->number_of_entries);

  // read sync sample table
  if (out->number_of_entries > MuTFF_MAX_SYNC_SAMPLE_TABLE_LEN) {
    return MuTFFErrorOutOfMemory;
  }
  const size_t table_size = mutff_data_size(size) - 8U;
  if (table_size != out->number_of_entries * 4U) {
    return MuTFFErrorBadFormat;
  }
  for (size_t i = 0; i < out->number_of_entries; ++i) {
    MuTFF_FN(mutff_read_u32, &out->sync_sample_table[i]);
  }

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_sync_sample_atom_size(
    uint64_t *out, const MuTFFSyncSampleAtom *atom) {
  *out = mutff_atom_size(8U + atom->number_of_entries * 4U);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_sync_sample_atom(mutff_file_t *fd, size_t *n,
                                        const MuTFFSyncSampleAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_sync_sample_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('s', 't', 's', 's'));
  MuTFF_FN(mutff_write_u8, in->version);
  MuTFF_FN(mutff_write_u24, in->flags);
  MuTFF_FN(mutff_write_u32, in->number_of_entries);
  if (in->number_of_entries * 4U != mutff_data_size(size) - 8U) {
    return MuTFFErrorBadFormat;
  }
  for (uint32_t i = 0; i < in->number_of_entries; ++i) {
    MuTFF_FN(mutff_write_u32, in->sync_sample_table[i]);
  }
  return MuTFFErrorNone;
}

MuTFFError mutff_read_partial_sync_sample_atom(
    mutff_file_t *fd, size_t *n, MuTFFPartialSyncSampleAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('s', 't', 'p', 's')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FN(mutff_read_u8, &out->version);
  MuTFF_FN(mutff_read_u24, &out->flags);
  MuTFF_FN(mutff_read_u32, &out->entry_count);

  // read partial sync sample table
  if (out->entry_count > MuTFF_MAX_PARTIAL_SYNC_SAMPLE_TABLE_LEN) {
    return MuTFFErrorOutOfMemory;
  }
  const size_t table_size = mutff_data_size(size) - 8U;
  if (table_size != out->entry_count * 4U) {
    return MuTFFErrorBadFormat;
  }
  for (size_t i = 0; i < out->entry_count; ++i) {
    MuTFF_FN(mutff_read_u32, &out->partial_sync_sample_table[i]);
  }

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_partial_sync_sample_atom_size(
    uint64_t *out, const MuTFFPartialSyncSampleAtom *atom) {
  *out = mutff_atom_size(8U + atom->entry_count * 4U);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_partial_sync_sample_atom(
    mutff_file_t *fd, size_t *n, const MuTFFPartialSyncSampleAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_partial_sync_sample_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('s', 't', 'p', 's'));
  MuTFF_FN(mutff_write_u8, in->version);
  MuTFF_FN(mutff_write_u24, in->flags);
  MuTFF_FN(mutff_write_u32, in->entry_count);
  if (in->entry_count * 4U != mutff_data_size(size) - 8U) {
    return MuTFFErrorBadFormat;
  }
  for (uint32_t i = 0; i < in->entry_count; ++i) {
    MuTFF_FN(mutff_write_u32, in->partial_sync_sample_table[i]);
  }
  return MuTFFErrorNone;
}

MuTFFError mutff_read_sample_to_chunk_table_entry(
    mutff_file_t *fd, size_t *n, MuTFFSampleToChunkTableEntry *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  MuTFF_FN(mutff_read_u32, &out->first_chunk);
  MuTFF_FN(mutff_read_u32, &out->samples_per_chunk);
  MuTFF_FN(mutff_read_u32, &out->sample_description_id);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_sample_to_chunk_table_entry(
    mutff_file_t *fd, size_t *n, const MuTFFSampleToChunkTableEntry *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  MuTFF_FN(mutff_write_u32, in->first_chunk);
  MuTFF_FN(mutff_write_u32, in->samples_per_chunk);
  MuTFF_FN(mutff_write_u32, in->sample_description_id);
  return MuTFFErrorNone;
}

MuTFFError mutff_read_sample_to_chunk_atom(mutff_file_t *fd, size_t *n,
                                           MuTFFSampleToChunkAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('s', 't', 's', 'c')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FN(mutff_read_u8, &out->version);
  MuTFF_FN(mutff_read_u24, &out->flags);
  MuTFF_FN(mutff_read_u32, &out->number_of_entries);

  // read table
  if (out->number_of_entries > MuTFF_MAX_SAMPLE_TO_CHUNK_TABLE_LEN) {
    return MuTFFErrorOutOfMemory;
  }
  const size_t table_size = mutff_data_size(size) - 8U;
  if (table_size != out->number_of_entries * 12U) {
    return MuTFFErrorBadFormat;
  }
  for (size_t i = 0; i < out->number_of_entries; ++i) {
    MuTFF_FN(mutff_read_sample_to_chunk_table_entry,
             &out->sample_to_chunk_table[i]);
  }

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_sample_to_chunk_atom_size(
    uint64_t *out, const MuTFFSampleToChunkAtom *atom) {
  *out = mutff_atom_size(8U + atom->number_of_entries * 12U);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_sample_to_chunk_atom(mutff_file_t *fd, size_t *n,
                                            const MuTFFSampleToChunkAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_sample_to_chunk_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('s', 't', 's', 'c'));
  MuTFF_FN(mutff_write_u8, in->version);
  MuTFF_FN(mutff_write_u24, in->flags);
  MuTFF_FN(mutff_write_u32, in->number_of_entries);
  if (in->number_of_entries * 12U != mutff_data_size(size) - 8U) {
    return MuTFFErrorBadFormat;
  }
  for (uint32_t i = 0; i < in->number_of_entries; ++i) {
    MuTFF_FN(mutff_write_sample_to_chunk_table_entry,
             &in->sample_to_chunk_table[i]);
  }
  return MuTFFErrorNone;
}

MuTFFError mutff_read_sample_size_atom(mutff_file_t *fd, size_t *n,
                                       MuTFFSampleSizeAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('s', 't', 's', 'z')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FN(mutff_read_u8, &out->version);
  MuTFF_FN(mutff_read_u24, &out->flags);
  MuTFF_FN(mutff_read_u32, &out->sample_size);
  MuTFF_FN(mutff_read_u32, &out->number_of_entries);

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
      MuTFF_FN(mutff_read_u32, &out->sample_size_table[i]);
    }
  } else {
    // skip table
    MuTFF_SEEK_CUR(size - *n);
  }

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_sample_size_atom_size(
    uint64_t *out, const MuTFFSampleSizeAtom *atom) {
  *out = mutff_atom_size(
      12U + (atom->sample_size == 0U ? atom->number_of_entries * 4U : 0U));
  return MuTFFErrorNone;
}

MuTFFError mutff_write_sample_size_atom(mutff_file_t *fd, size_t *n,
                                        const MuTFFSampleSizeAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_sample_size_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('s', 't', 's', 'z'));
  MuTFF_FN(mutff_write_u8, in->version);
  MuTFF_FN(mutff_write_u24, in->flags);
  MuTFF_FN(mutff_write_u32, in->sample_size);
  MuTFF_FN(mutff_write_u32, in->number_of_entries);
  if (in->sample_size == 0U) {
    for (uint32_t i = 0; i < in->number_of_entries; ++i) {
      MuTFF_FN(mutff_write_u32, in->sample_size_table[i]);
    }
  }
  return MuTFFErrorNone;
}

MuTFFError mutff_read_chunk_offset_atom(mutff_file_t *fd, size_t *n,
                                        MuTFFChunkOffsetAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('s', 't', 'c', 'o')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FN(mutff_read_u8, &out->version);
  MuTFF_FN(mutff_read_u24, &out->flags);
  MuTFF_FN(mutff_read_u32, &out->number_of_entries);

  // read table
  if (out->number_of_entries > MuTFF_MAX_CHUNK_OFFSET_TABLE_LEN) {
    return MuTFFErrorOutOfMemory;
  }
  const size_t table_size = mutff_data_size(size) - 8U;
  if (table_size != out->number_of_entries * 4U) {
    return MuTFFErrorBadFormat;
  }
  for (size_t i = 0; i < out->number_of_entries; ++i) {
    MuTFF_FN(mutff_read_u32, &out->chunk_offset_table[i]);
  }

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_chunk_offset_atom_size(
    uint64_t *out, const MuTFFChunkOffsetAtom *atom) {
  *out = mutff_atom_size(8U + atom->number_of_entries * 4U);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_chunk_offset_atom(mutff_file_t *fd, size_t *n,
                                         const MuTFFChunkOffsetAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_chunk_offset_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('s', 't', 'c', 'o'));
  MuTFF_FN(mutff_write_u8, in->version);
  MuTFF_FN(mutff_write_u24, in->flags);
  MuTFF_FN(mutff_write_u32, in->number_of_entries);
  if (in->number_of_entries * 4U != mutff_data_size(size) - 8U) {
    return MuTFFErrorBadFormat;
  }
  for (uint32_t i = 0; i < in->number_of_entries; ++i) {
    MuTFF_FN(mutff_write_u32, in->chunk_offset_table[i]);
  }
  return MuTFFErrorNone;
}

MuTFFError mutff_read_sample_dependency_flags_atom(
    mutff_file_t *fd, size_t *n, MuTFFSampleDependencyFlagsAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('s', 'd', 't', 'p')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FN(mutff_read_u8, &out->version);
  MuTFF_FN(mutff_read_u24, &out->flags);

  // read table
  out->data_size = mutff_data_size(size) - 4U;
  if (out->data_size > MuTFF_MAX_SAMPLE_DEPENDENCY_FLAGS_TABLE_LEN) {
    return MuTFFErrorOutOfMemory;
  }
  for (size_t i = 0; i < out->data_size; ++i) {
    MuTFF_FN(mutff_read_u8, &out->sample_dependency_flags_table[i]);
  }

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_sample_dependency_flags_atom_size(
    uint64_t *out, const MuTFFSampleDependencyFlagsAtom *atom) {
  *out = mutff_atom_size(4U + atom->data_size);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_sample_dependency_flags_atom(
    mutff_file_t *fd, size_t *n, const MuTFFSampleDependencyFlagsAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_sample_dependency_flags_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('s', 'd', 't', 'p'));
  MuTFF_FN(mutff_write_u8, in->version);
  MuTFF_FN(mutff_write_u24, in->flags);
  const size_t flags_table_size = mutff_data_size(size) - 4U;
  for (uint32_t i = 0; i < flags_table_size; ++i) {
    MuTFF_FN(mutff_write_u8, in->sample_dependency_flags_table[i]);
  }
  return MuTFFErrorNone;
}

MuTFFError mutff_read_sample_table_atom(mutff_file_t *fd, size_t *n,
                                        MuTFFSampleTableAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
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

  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('s', 't', 'b', 'l')) {
    return MuTFFErrorBadFormat;
  }

  // read child atoms
  uint64_t child_size;
  uint32_t child_type;
  while (*n < size) {
    MuTFF_FN(mutff_peek_atom_header, &child_size, &child_type);
    if (size == 0U) {
      return MuTFFErrorBadFormat;
    }
    if (*n + child_size > size) {
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

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_sample_table_atom_size(
    uint64_t *out, const MuTFFSampleTableAtom *atom) {
  MuTFFError err;
  uint64_t size;
  uint64_t child_size;
  err = mutff_sample_description_atom_size(&size, &atom->sample_description);
  if (err != MuTFFErrorNone) {
    return err;
  }
  err = mutff_time_to_sample_atom_size(&child_size, &atom->time_to_sample);
  if (err != MuTFFErrorNone) {
    return err;
  }
  size += child_size;
  if (atom->composition_offset_present) {
    err = mutff_composition_offset_atom_size(&child_size,
                                             &atom->composition_offset);
    if (err != MuTFFErrorNone) {
      return err;
    }
    size += child_size;
  }
  if (atom->composition_shift_least_greatest_present) {
    err = mutff_composition_shift_least_greatest_atom_size(
        &child_size, &atom->composition_shift_least_greatest);
    if (err != MuTFFErrorNone) {
      return err;
    }
    size += child_size;
  }
  if (atom->sync_sample_present) {
    err = mutff_sync_sample_atom_size(&child_size, &atom->sync_sample);
    if (err != MuTFFErrorNone) {
      return err;
    }
    size += child_size;
  }
  if (atom->partial_sync_sample_present) {
    err = mutff_partial_sync_sample_atom_size(&child_size,
                                              &atom->partial_sync_sample);
    if (err != MuTFFErrorNone) {
      return err;
    }
    size += child_size;
  }
  if (atom->sample_to_chunk_present) {
    err = mutff_sample_to_chunk_atom_size(&child_size, &atom->sample_to_chunk);
    if (err != MuTFFErrorNone) {
      return err;
    }
    size += child_size;
  }
  if (atom->sample_size_present) {
    err = mutff_sample_size_atom_size(&child_size, &atom->sample_size);
    if (err != MuTFFErrorNone) {
      return err;
    }
    size += child_size;
  }
  if (atom->chunk_offset_present) {
    err = mutff_chunk_offset_atom_size(&child_size, &atom->chunk_offset);
    if (err != MuTFFErrorNone) {
      return err;
    }
    size += child_size;
  }
  if (atom->sample_dependency_flags_present) {
    err = mutff_sample_dependency_flags_atom_size(
        &child_size, &atom->sample_dependency_flags);
    if (err != MuTFFErrorNone) {
      return err;
    }
    size += child_size;
  }
  *out = mutff_atom_size(size);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_sample_table_atom(mutff_file_t *fd, size_t *n,
                                         const MuTFFSampleTableAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;

  uint64_t size;
  err = mutff_sample_table_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('s', 't', 'b', 'l'));
  MuTFF_FN(mutff_write_sample_description_atom, &in->sample_description);
  MuTFF_FN(mutff_write_time_to_sample_atom, &in->time_to_sample);
  if (in->composition_offset_present) {
    MuTFF_FN(mutff_write_composition_offset_atom, &in->composition_offset);
  }
  if (in->composition_shift_least_greatest_present) {
    MuTFF_FN(mutff_write_composition_shift_least_greatest_atom,
             &in->composition_shift_least_greatest);
  }
  if (in->sync_sample_present) {
    MuTFF_FN(mutff_write_sync_sample_atom, &in->sync_sample);
  }
  if (in->partial_sync_sample_present) {
    MuTFF_FN(mutff_write_partial_sync_sample_atom, &in->partial_sync_sample);
  }
  if (in->sample_to_chunk_present) {
    MuTFF_FN(mutff_write_sample_to_chunk_atom, &in->sample_to_chunk);
  }
  if (in->sample_size_present) {
    MuTFF_FN(mutff_write_sample_size_atom, &in->sample_size);
  }
  if (in->chunk_offset_present) {
    MuTFF_FN(mutff_write_chunk_offset_atom, &in->chunk_offset);
  }
  if (in->sample_dependency_flags_present) {
    MuTFF_FN(mutff_write_sample_dependency_flags_atom,
             &in->sample_dependency_flags);
  }

  return MuTFFErrorNone;
}

MuTFFError mutff_read_video_media_information_atom(
    mutff_file_t *fd, size_t *n, MuTFFVideoMediaInformationAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  bool video_media_information_header_present = false;
  bool handler_reference_present = false;

  out->data_information_present = false;
  out->sample_table_present = false;

  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('m', 'i', 'n', 'f')) {
    return MuTFFErrorBadFormat;
  }

  // read child atoms
  uint64_t child_size;
  uint32_t child_type;
  while (*n < size) {
    MuTFF_FN(mutff_peek_atom_header, &child_size, &child_type);
    if (size == 0U) {
      return MuTFFErrorBadFormat;
    }
    if (*n + child_size > size) {
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

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_video_media_information_atom_size(
    uint64_t *out, const MuTFFVideoMediaInformationAtom *atom) {
  MuTFFError err;
  uint64_t size;
  uint64_t child_size;
  err = mutff_video_media_information_header_atom_size(
      &size, &atom->video_media_information_header);
  if (err != MuTFFErrorNone) {
    return err;
  }
  err =
      mutff_handler_reference_atom_size(&child_size, &atom->handler_reference);
  if (err != MuTFFErrorNone) {
    return err;
  }
  size += child_size;
  if (atom->data_information_present) {
    err =
        mutff_data_information_atom_size(&child_size, &atom->data_information);
    if (err != MuTFFErrorNone) {
      return err;
    }
    size += child_size;
  }
  if (atom->sample_table_present) {
    err = mutff_sample_table_atom_size(&child_size, &atom->sample_table);
    if (err != MuTFFErrorNone) {
      return err;
    }
    size += child_size;
  }
  *out = mutff_atom_size(size);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_video_media_information_atom(
    mutff_file_t *fd, size_t *n, const MuTFFVideoMediaInformationAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;

  uint64_t size;
  err = mutff_video_media_information_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('m', 'i', 'n', 'f'));
  MuTFF_FN(mutff_write_video_media_information_header_atom,
           &in->video_media_information_header);
  MuTFF_FN(mutff_write_handler_reference_atom, &in->handler_reference);
  if (in->data_information_present) {
    MuTFF_FN(mutff_write_data_information_atom, &in->data_information);
  }
  if (in->sample_table_present) {
    MuTFF_FN(mutff_write_sample_table_atom, &in->sample_table);
  }

  return MuTFFErrorNone;
}

MuTFFError mutff_read_sound_media_information_header_atom(
    mutff_file_t *fd, size_t *n, MuTFFSoundMediaInformationHeaderAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('s', 'm', 'h', 'd')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FN(mutff_read_u8, &out->version);
  MuTFF_FN(mutff_read_u24, &out->flags);
  MuTFF_FN(mutff_read_i16, &out->balance);
  MuTFF_SEEK_CUR(2U);
  return MuTFFErrorNone;
}

static inline MuTFFError mutff_sound_media_information_header_atom_size(
    uint64_t *out, const MuTFFSoundMediaInformationHeaderAtom *atom) {
  *out = mutff_atom_size(8);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_sound_media_information_header_atom(
    mutff_file_t *fd, size_t *n,
    const MuTFFSoundMediaInformationHeaderAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_sound_media_information_header_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('s', 'm', 'h', 'd'));
  MuTFF_FN(mutff_write_u8, in->version);
  MuTFF_FN(mutff_write_u24, in->flags);
  MuTFF_FN(mutff_write_i16, in->balance);
  for (size_t i = 0; i < 2U; ++i) {
    MuTFF_FN(mutff_write_u8, 0);
  }
  return MuTFFErrorNone;
}

MuTFFError mutff_read_sound_media_information_atom(
    mutff_file_t *fd, size_t *n, MuTFFSoundMediaInformationAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  bool sound_media_information_header_present = false;
  bool handler_reference_present = false;

  out->data_information_present = false;
  out->sample_table_present = false;

  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('m', 'i', 'n', 'f')) {
    return MuTFFErrorBadFormat;
  }

  // read child atoms
  uint64_t child_size;
  uint32_t child_type;
  while (*n < size) {
    MuTFF_FN(mutff_peek_atom_header, &child_size, &child_type);
    if (size == 0U) {
      return MuTFFErrorBadFormat;
    }
    if (*n + child_size > size) {
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

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_sound_media_information_atom_size(
    uint64_t *out, const MuTFFSoundMediaInformationAtom *atom) {
  MuTFFError err;
  uint64_t size;
  uint64_t child_size;
  err = mutff_sound_media_information_header_atom_size(
      &size, &atom->sound_media_information_header);
  if (err != MuTFFErrorNone) {
    return err;
  }
  err =
      mutff_handler_reference_atom_size(&child_size, &atom->handler_reference);
  if (err != MuTFFErrorNone) {
    return err;
  }
  size += child_size;
  if (atom->data_information_present) {
    err =
        mutff_data_information_atom_size(&child_size, &atom->data_information);
    if (err != MuTFFErrorNone) {
      return err;
    }
    size += child_size;
  }
  if (atom->sample_table_present) {
    err = mutff_sample_table_atom_size(&child_size, &atom->sample_table);
    if (err != MuTFFErrorNone) {
      return err;
    }
    size += child_size;
  }
  *out = mutff_atom_size(size);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_sound_media_information_atom(
    mutff_file_t *fd, size_t *n, const MuTFFSoundMediaInformationAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;

  uint64_t size;
  err = mutff_sound_media_information_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('m', 'i', 'n', 'f'));
  MuTFF_FN(mutff_write_sound_media_information_header_atom,
           &in->sound_media_information_header);
  MuTFF_FN(mutff_write_handler_reference_atom, &in->handler_reference);
  if (in->data_information_present) {
    MuTFF_FN(mutff_write_data_information_atom, &in->data_information);
  }
  if (in->sample_table_present) {
    MuTFF_FN(mutff_write_sample_table_atom, &in->sample_table);
  }

  return MuTFFErrorNone;
}

MuTFFError mutff_read_base_media_info_atom(mutff_file_t *fd, size_t *n,
                                           MuTFFBaseMediaInfoAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('g', 'm', 'i', 'n')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FN(mutff_read_u8, &out->version);
  MuTFF_FN(mutff_read_u24, &out->flags);
  MuTFF_FN(mutff_read_u16, &out->graphics_mode);
  for (size_t i = 0; i < 3U; ++i) {
    MuTFF_FN(mutff_read_u16, &out->opcolor[i]);
  }
  MuTFF_FN(mutff_read_i16, &out->balance);
  MuTFF_SEEK_CUR(2U);
  return MuTFFErrorNone;
}

static inline MuTFFError mutff_base_media_info_atom_size(
    uint64_t *out, const MuTFFBaseMediaInfoAtom *atom) {
  *out = mutff_atom_size(16);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_base_media_info_atom(mutff_file_t *fd, size_t *n,
                                            const MuTFFBaseMediaInfoAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_base_media_info_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('g', 'm', 'i', 'n'));
  MuTFF_FN(mutff_write_u8, in->version);
  MuTFF_FN(mutff_write_u24, in->flags);
  MuTFF_FN(mutff_write_u16, in->graphics_mode);
  for (size_t i = 0; i < 3U; ++i) {
    MuTFF_FN(mutff_write_u16, in->opcolor[i]);
  }
  MuTFF_FN(mutff_write_i16, in->balance);
  for (size_t i = 0; i < 2U; ++i) {
    MuTFF_FN(mutff_write_u8, 0);
  }
  return MuTFFErrorNone;
}

MuTFFError mutff_read_text_media_information_atom(
    mutff_file_t *fd, size_t *n, MuTFFTextMediaInformationAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('t', 'e', 'x', 't')) {
    return MuTFFErrorBadFormat;
  }
  MuTFF_FN(mutff_read_matrix, &out->matrix_structure);
  return MuTFFErrorNone;
}

static inline MuTFFError mutff_text_media_information_atom_size(
    uint64_t *out, const MuTFFTextMediaInformationAtom *atom) {
  *out = mutff_atom_size(36);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_text_media_information_atom(
    mutff_file_t *fd, size_t *n, const MuTFFTextMediaInformationAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_text_media_information_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('t', 'e', 'x', 't'));
  MuTFF_FN(mutff_write_matrix, in->matrix_structure);
  return MuTFFErrorNone;
}

MuTFFError mutff_read_base_media_information_header_atom(
    mutff_file_t *fd, size_t *n, MuTFFBaseMediaInformationHeaderAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  bool base_media_info_present = false;

  out->text_media_information_present = false;

  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('g', 'm', 'h', 'd')) {
    return MuTFFErrorBadFormat;
  }

  // read child atoms
  uint64_t child_size;
  uint32_t child_type;
  while (*n < size) {
    MuTFF_FN(mutff_peek_atom_header, &child_size, &child_type);
    if (size == 0U) {
      return MuTFFErrorBadFormat;
    }
    if (*n + child_size > size) {
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

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_base_media_information_header_atom_size(
    uint64_t *out, const MuTFFBaseMediaInformationHeaderAtom *atom) {
  MuTFFError err;
  uint64_t size;
  uint64_t child_size;

  err = mutff_base_media_info_atom_size(&size, &atom->base_media_info);
  if (err != MuTFFErrorNone) {
    return err;
  }
  err = mutff_text_media_information_atom_size(&child_size,
                                               &atom->text_media_information);
  if (err != MuTFFErrorNone) {
    return err;
  }
  size += child_size;

  *out = mutff_atom_size(size);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_base_media_information_header_atom(
    mutff_file_t *fd, size_t *n,
    const MuTFFBaseMediaInformationHeaderAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_base_media_information_header_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('g', 'm', 'h', 'd'));
  MuTFF_FN(mutff_write_base_media_info_atom, &in->base_media_info);
  MuTFF_FN(mutff_write_text_media_information_atom,
           &in->text_media_information);
  return MuTFFErrorNone;
}

MuTFFError mutff_read_base_media_information_atom(
    mutff_file_t *fd, size_t *n, MuTFFBaseMediaInformationAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('m', 'i', 'n', 'f')) {
    return MuTFFErrorBadFormat;
  }

  // read child atom
  MuTFF_FN(mutff_read_base_media_information_header_atom,
           &out->base_media_information_header);

  // skip remaining space
  MuTFF_SEEK_CUR(size - *n);

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_base_media_information_atom_size(
    uint64_t *out, const MuTFFBaseMediaInformationAtom *atom) {
  uint64_t size;
  const MuTFFError err = mutff_base_media_information_header_atom_size(
      &size, &atom->base_media_information_header);
  if (err != MuTFFErrorNone) {
    return err;
  }
  *out = mutff_atom_size(size);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_base_media_information_atom(
    mutff_file_t *fd, size_t *n, const MuTFFBaseMediaInformationAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  err = mutff_base_media_information_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('m', 'i', 'n', 'f'));
  MuTFF_FN(mutff_write_base_media_information_header_atom,
           &in->base_media_information_header);
  return MuTFFErrorNone;
}

MuTFFMediaType mutff_media_type(uint32_t type) {
  switch (type) {
    case MuTFF_FOURCC('v', 'i', 'd', 'e'):
      return MuTFFMediaTypeVideo;
    case MuTFF_FOURCC('c', 'v', 'i', 'd'):
      return MuTFFMediaTypeVideo;
    case MuTFF_FOURCC('j', 'p', 'e', 'g'):
      return MuTFFMediaTypeVideo;
    case MuTFF_FOURCC('s', 'm', 'c', ' '):
      return MuTFFMediaTypeVideo;
    case MuTFF_FOURCC('r', 'l', 'e', ' '):
      return MuTFFMediaTypeVideo;
    case MuTFF_FOURCC('r', 'p', 'z', 'a'):
      return MuTFFMediaTypeVideo;
    case MuTFF_FOURCC('k', 'p', 'c', 'd'):
      return MuTFFMediaTypeVideo;
    case MuTFF_FOURCC('p', 'n', 'g', ' '):
      return MuTFFMediaTypeVideo;
    case MuTFF_FOURCC('m', 'j', 'p', 'a'):
      return MuTFFMediaTypeVideo;
    case MuTFF_FOURCC('m', 'j', 'p', 'b'):
      return MuTFFMediaTypeVideo;
    case MuTFF_FOURCC('S', 'V', 'Q', '1'):
      return MuTFFMediaTypeVideo;
    case MuTFF_FOURCC('S', 'V', 'Q', '3'):
      return MuTFFMediaTypeVideo;
    case MuTFF_FOURCC('m', 'p', '4', 'v'):
      return MuTFFMediaTypeVideo;
    case MuTFF_FOURCC('a', 'v', 'c', '1'):
      return MuTFFMediaTypeVideo;
    case MuTFF_FOURCC('d', 'v', 'c', ' '):
      return MuTFFMediaTypeVideo;
    case MuTFF_FOURCC('d', 'v', 'c', 'p'):
      return MuTFFMediaTypeVideo;
    case MuTFF_FOURCC('g', 'i', 'f', ' '):
      return MuTFFMediaTypeVideo;
    case MuTFF_FOURCC('h', '2', '6', '3'):
      return MuTFFMediaTypeVideo;
    case MuTFF_FOURCC('t', 'i', 'f', 'f'):
      return MuTFFMediaTypeVideo;
    case MuTFF_FOURCC('r', 'a', 'w', ' '):
      return MuTFFMediaTypeVideo;
    case MuTFF_FOURCC('2', 'v', 'u', 'Y'):
      return MuTFFMediaTypeVideo;
    case MuTFF_FOURCC('y', 'u', 'v', '2'):
      return MuTFFMediaTypeVideo;
    case MuTFF_FOURCC('v', '3', '0', '8'):
      return MuTFFMediaTypeVideo;
    case MuTFF_FOURCC('v', '4', '0', '8'):
      return MuTFFMediaTypeVideo;
    case MuTFF_FOURCC('v', '2', '1', '6'):
      return MuTFFMediaTypeVideo;
    case MuTFF_FOURCC('v', '4', '1', '0'):
      return MuTFFMediaTypeVideo;
    case MuTFF_FOURCC('v', '2', '1', '0'):
      return MuTFFMediaTypeVideo;
    default:
      return MuTFFMediaTypeUnknown;
  }
}

inline MuTFFWriteFn mutff_media_type_write_fn(MuTFFMediaType type) {
  switch (type) {
    case MuTFFMediaTypeVideo:
      return (MuTFFWriteFn)mutff_write_video_sample_description;
    default:
      return NULL;
  }
}

inline MuTFFReadFn mutff_media_type_read_fn(MuTFFMediaType type) {
  switch (type) {
    case MuTFFMediaTypeVideo:
      return (MuTFFReadFn)mutff_read_video_sample_description;
    default:
      return NULL;
  }
}

inline MuTFFSizeFn mutff_media_type_size_fn(MuTFFMediaType type) {
  switch (type) {
    case MuTFFMediaTypeVideo:
      return (MuTFFSizeFn)mutff_video_sample_description_size;
    default:
      return NULL;
  }
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

MuTFFError mutff_media_atom_type(MuTFFMediaType *out,
                                 const MuTFFMediaAtom *atom) {
  if (!atom->handler_reference_present) {
    return MuTFFErrorBadFormat;
  }
  *out = mutff_media_type(atom->handler_reference.component_subtype);
  return MuTFFErrorNone;
}

MuTFFError mutff_read_media_atom(mutff_file_t *fd, size_t *n,
                                 MuTFFMediaAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  bool media_header_present = false;
  unsigned int media_information_offset;

  out->extended_language_tag_present = false;
  out->handler_reference_present = false;
  out->media_information_present = false;
  out->user_data_present = false;

  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('m', 'd', 'i', 'a')) {
    return MuTFFErrorBadFormat;
  }

  // read child atoms
  uint64_t child_size;
  uint32_t child_type;
  while (*n < size) {
    MuTFF_FN(mutff_peek_atom_header, &child_size, &child_type);
    if (size == 0U) {
      return MuTFFErrorBadFormat;
    }
    if (*n + child_size > size) {
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
        err = mutff_tell(fd, &media_information_offset);
        if (err != MuTFFErrorNone) {
          return err;
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

  unsigned int atom_end_offset;
  err = mutff_tell(fd, &atom_end_offset);
  if (err != MuTFFErrorNone) {
    return err;
  }

  if (out->media_information_present) {
    MuTFFMediaType media_type;
    err = mutff_media_atom_type(&media_type, out);
    if (err != MuTFFErrorNone) {
      return err;
    }
    err =
        mutff_seek(fd, (long)media_information_offset - (long)atom_end_offset);
    if (err != MuTFFErrorNone) {
      return err;
    }
    switch (mutff_media_information_type(media_type)) {
      case MuTFFVideoMediaInformation:
        err = mutff_read_video_media_information_atom(
            fd, &bytes, &out->video_media_information);
        break;
      case MuTFFSoundMediaInformation:
        err = mutff_read_sound_media_information_atom(
            fd, &bytes, &out->sound_media_information);
        break;
      case MuTFFBaseMediaInformation:
        err = mutff_read_base_media_information_atom(
            fd, &bytes, &out->base_media_information);
        break;
      default:
        return MuTFFErrorBadFormat;
    }
    if (err != MuTFFErrorNone) {
      return err;
    }
    err = mutff_seek(fd, (long)atom_end_offset -
                             (long)media_information_offset - (long)bytes);
    if (err != MuTFFErrorNone) {
      return err;
    }
  }

  return MuTFFErrorNone;
}

static MuTFFError mutff_media_atom_size(uint64_t *out,
                                        const MuTFFMediaAtom *atom) {
  MuTFFError err;
  uint64_t size;
  uint64_t child_size;
  err = mutff_media_header_atom_size(&size, &atom->media_header);
  if (err != MuTFFErrorNone) {
    return err;
  }
  if (atom->extended_language_tag_present) {
    err = mutff_extended_language_tag_atom_size(&child_size,
                                                &atom->extended_language_tag);
    if (err != MuTFFErrorNone) {
      return err;
    }
    size += child_size;
  }
  if (atom->handler_reference_present) {
    err = mutff_handler_reference_atom_size(&child_size,
                                            &atom->handler_reference);
    if (err != MuTFFErrorNone) {
      return err;
    }
    size += child_size;
  }
  if (atom->media_information_present) {
    MuTFFMediaType type;
    err = mutff_media_atom_type(&type, atom);
    if (err != MuTFFErrorNone) {
      return err;
    }

    switch (mutff_media_information_type(type)) {
      case MuTFFVideoMediaInformation:
        err = mutff_video_media_information_atom_size(
            &child_size, &atom->video_media_information);
        if (err != MuTFFErrorNone) {
          return err;
        }
        size += child_size;
        break;
      case MuTFFSoundMediaInformation:
        err = mutff_sound_media_information_atom_size(
            &child_size, &atom->sound_media_information);
        if (err != MuTFFErrorNone) {
          return err;
        }
        size += child_size;
        break;
      case MuTFFBaseMediaInformation:
        err = mutff_base_media_information_atom_size(
            &child_size, &atom->base_media_information);
        if (err != MuTFFErrorNone) {
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
    if (err != MuTFFErrorNone) {
      return err;
    }
    size += child_size;
  }
  *out = mutff_atom_size(size);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_media_atom(mutff_file_t *fd, size_t *n,
                                  const MuTFFMediaAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;

  uint64_t size;
  err = mutff_media_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('m', 'd', 'i', 'a'));
  MuTFF_FN(mutff_write_media_header_atom, &in->media_header);
  if (in->extended_language_tag_present) {
    MuTFF_FN(mutff_write_extended_language_tag_atom,
             &in->extended_language_tag);
  }
  if (in->handler_reference_present) {
    MuTFF_FN(mutff_write_handler_reference_atom, &in->handler_reference);
  }
  if (in->media_information_present) {
    MuTFFMediaType type;
    MuTFFError err;
    err = mutff_media_atom_type(&type, in);
    if (err != MuTFFErrorNone) {
      return err;
    }

    switch (mutff_media_information_type(type)) {
      case MuTFFVideoMediaInformation:
        MuTFF_FN(mutff_write_video_media_information_atom,
                 &in->video_media_information);
        break;
      case MuTFFSoundMediaInformation:
        MuTFF_FN(mutff_write_sound_media_information_atom,
                 &in->sound_media_information);
        break;
      case MuTFFBaseMediaInformation:
        MuTFF_FN(mutff_write_base_media_information_atom,
                 &in->base_media_information);
        break;
      default:
        return MuTFFErrorBadFormat;
    }
  }
  if (in->user_data_present) {
    MuTFF_FN(mutff_write_user_data_atom, &in->user_data);
  }

  return MuTFFErrorNone;
}

MuTFFError mutff_read_track_atom(mutff_file_t *fd, size_t *n,
                                 MuTFFTrackAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
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

  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('t', 'r', 'a', 'k')) {
    return MuTFFErrorBadFormat;
  }

  // read child atoms
  uint64_t child_size;
  uint32_t child_type;
  while (*n < size) {
    MuTFF_FN(mutff_peek_atom_header, &child_size, &child_type);
    if (size == 0U) {
      return MuTFFErrorBadFormat;
    }
    if (*n + child_size > size) {
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

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_track_atom_size(uint64_t *out,
                                               const MuTFFTrackAtom *atom) {
  MuTFFError err;
  uint64_t size;
  uint64_t child_size;
  err = mutff_track_header_atom_size(&size, &atom->track_header);
  if (err != MuTFFErrorNone) {
    return err;
  }
  err = mutff_media_atom_size(&child_size, &atom->media);
  if (err != MuTFFErrorNone) {
    return err;
  }
  size += child_size;
  if (atom->track_aperture_mode_dimensions_present) {
    err = mutff_track_aperture_mode_dimensions_atom_size(
        &child_size, &atom->track_aperture_mode_dimensions);
    if (err != MuTFFErrorNone) {
      return err;
    }
    size += child_size;
  }
  if (atom->clipping_present) {
    err = mutff_clipping_atom_size(&child_size, &atom->clipping);
    if (err != MuTFFErrorNone) {
      return err;
    }
    size += child_size;
  }
  if (atom->track_matte_present) {
    err = mutff_track_matte_atom_size(&child_size, &atom->track_matte);
    if (err != MuTFFErrorNone) {
      return err;
    }
    size += child_size;
  }
  if (atom->edit_present) {
    err = mutff_edit_atom_size(&child_size, &atom->edit);
    if (err != MuTFFErrorNone) {
      return err;
    }
    size += child_size;
  }
  if (atom->track_reference_present) {
    err = mutff_track_reference_atom_size(&child_size, &atom->track_reference);
    if (err != MuTFFErrorNone) {
      return err;
    }
    size += child_size;
  }
  if (atom->track_exclude_from_autoselection_present) {
    err = mutff_track_exclude_from_autoselection_atom_size(
        &child_size, &atom->track_exclude_from_autoselection);
    if (err != MuTFFErrorNone) {
      return err;
    }
    size += child_size;
  }
  if (atom->track_load_settings_present) {
    err = mutff_track_load_settings_atom_size(&child_size,
                                              &atom->track_load_settings);
    if (err != MuTFFErrorNone) {
      return err;
    }
    size += child_size;
  }
  if (atom->track_input_map_present) {
    err = mutff_track_input_map_atom_size(&child_size, &atom->track_input_map);
    if (err != MuTFFErrorNone) {
      return err;
    }
    size += child_size;
  }
  if (atom->user_data_present) {
    err = mutff_user_data_atom_size(&child_size, &atom->user_data);
    if (err != MuTFFErrorNone) {
      return err;
    }
    size += child_size;
  }
  *out = mutff_atom_size(size);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_track_atom(mutff_file_t *fd, size_t *n,
                                  const MuTFFTrackAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;

  uint64_t size;
  err = mutff_track_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('t', 'r', 'a', 'k'));
  MuTFF_FN(mutff_write_track_header_atom, &in->track_header);
  MuTFF_FN(mutff_write_media_atom, &in->media);
  if (in->track_aperture_mode_dimensions_present) {
    MuTFF_FN(mutff_write_track_aperture_mode_dimensions_atom,
             &in->track_aperture_mode_dimensions);
  }
  if (in->clipping_present) {
    MuTFF_FN(mutff_write_clipping_atom, &in->clipping);
  }
  if (in->track_matte_present) {
    MuTFF_FN(mutff_write_track_matte_atom, &in->track_matte);
  }
  if (in->edit_present) {
    MuTFF_FN(mutff_write_edit_atom, &in->edit);
  }
  if (in->track_reference_present) {
    MuTFF_FN(mutff_write_track_reference_atom, &in->track_reference);
  }
  if (in->track_exclude_from_autoselection_present) {
    MuTFF_FN(mutff_write_track_exclude_from_autoselection_atom,
             &in->track_exclude_from_autoselection);
  }
  if (in->track_load_settings_present) {
    MuTFF_FN(mutff_write_track_load_settings_atom, &in->track_load_settings);
  }
  if (in->track_input_map_present) {
    MuTFF_FN(mutff_write_track_input_map_atom, &in->track_input_map);
  }
  if (in->user_data_present) {
    MuTFF_FN(mutff_write_user_data_atom, &in->user_data);
  }

  return MuTFFErrorNone;
}

MuTFFError mutff_read_movie_atom(mutff_file_t *fd, size_t *n,
                                 MuTFFMovieAtom *out) {
  MuTFFError err;
  size_t bytes;
  *n = 0;
  uint64_t size;
  uint32_t type;
  bool movie_header_present = false;

  out->track_count = 0;
  out->clipping_present = false;
  out->color_table_present = false;
  out->user_data_present = false;

  MuTFF_FN(mutff_read_header, &size, &type);
  if (type != MuTFF_FOURCC('m', 'o', 'o', 'v')) {
    return MuTFFErrorBadFormat;
  }

  // read child atoms
  uint64_t child_size;
  uint32_t child_type;
  while (*n < size) {
    MuTFF_FN(mutff_peek_atom_header, &child_size, &child_type);
    if (size == 0U) {
      return MuTFFErrorBadFormat;
    }
    if (*n + child_size > size) {
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
        MuTFF_FN(mutff_read_track_atom, &out->track[out->track_count]);
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

  return MuTFFErrorNone;
}

static inline MuTFFError mutff_movie_atom_size(uint64_t *out,
                                               const MuTFFMovieAtom *atom) {
  MuTFFError err;
  uint64_t size;
  uint64_t child_size;
  err = mutff_movie_header_atom_size(&size, &atom->movie_header);
  if (err != MuTFFErrorNone) {
    return err;
  }
  for (size_t i = 0; i < atom->track_count; ++i) {
    err = mutff_track_atom_size(&child_size, &atom->track[i]);
    if (err != MuTFFErrorNone) {
      return err;
    }
    size += child_size;
  }
  if (atom->clipping_present) {
    err = mutff_clipping_atom_size(&child_size, &atom->clipping);
    if (err != MuTFFErrorNone) {
      return err;
    }
    size += child_size;
  }
  if (atom->color_table_present) {
    err = mutff_color_table_atom_size(&child_size, &atom->color_table);
    if (err != MuTFFErrorNone) {
      return err;
    }
    size += child_size;
  }
  if (atom->user_data_present) {
    err = mutff_user_data_atom_size(&child_size, &atom->user_data);
    if (err != MuTFFErrorNone) {
      return err;
    }
    size += child_size;
  }
  *out = mutff_atom_size(size);
  return MuTFFErrorNone;
}

MuTFFError mutff_write_movie_atom(mutff_file_t *fd, size_t *n,
                                  const MuTFFMovieAtom *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;

  uint64_t size;
  err = mutff_movie_atom_size(&size, in);
  if (err != MuTFFErrorNone) {
    return err;
  }
  MuTFF_FN(mutff_write_header, size, MuTFF_FOURCC('m', 'o', 'o', 'v'));
  MuTFF_FN(mutff_write_movie_header_atom, &in->movie_header);
  for (size_t i = 0; i < in->track_count; ++i) {
    MuTFF_FN(mutff_write_track_atom, &in->track[i]);
  }
  if (in->clipping_present) {
    MuTFF_FN(mutff_write_clipping_atom, &in->clipping);
  }
  if (in->color_table_present) {
    MuTFF_FN(mutff_write_color_table_atom, &in->color_table);
  }
  if (in->user_data_present) {
    MuTFF_FN(mutff_write_user_data_atom, &in->user_data);
  }

  return MuTFFErrorNone;
}

MuTFFError mutff_read_movie_file(mutff_file_t *fd, size_t *n,
                                 MuTFFMovieFile *out) {
  MuTFFError err;
  uint64_t size;
  uint32_t type;
  size_t bytes;
  *n = 0;
  bool movie_present = false;

  out->preview_present = false;
  out->movie_data_count = 0;
  out->free_count = 0;
  out->skip_count = 0;
  out->wide_count = 0;

  MuTFF_FN(mutff_peek_atom_header, &size, &type);
  if (type == MuTFF_FOURCC('f', 't', 'y', 'p')) {
    MuTFF_FN(mutff_read_file_type_atom, &out->file_type);
    out->file_type_present = true;
  }

  while (mutff_peek_atom_header(fd, &bytes, &size, &type) == MuTFFErrorNone) {
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
        MuTFF_FN(mutff_read_movie_data_atom,
                 &out->movie_data[out->movie_data_count]);
        out->movie_data_count++;
        break;

      case MuTFF_FOURCC('f', 'r', 'e', 'e'):
        if (out->free_count >= MuTFF_MAX_FREE_ATOMS) {
          return MuTFFErrorOutOfMemory;
        }
        MuTFF_FN(mutff_read_free_atom, &out->free[out->free_count]);
        out->free_count++;
        break;

      case MuTFF_FOURCC('s', 'k', 'i', 'p'):
        if (out->skip_count >= MuTFF_MAX_SKIP_ATOMS) {
          return MuTFFErrorOutOfMemory;
        }
        MuTFF_FN(mutff_read_skip_atom, &out->skip[out->skip_count]);
        out->skip_count++;
        break;

      case MuTFF_FOURCC('w', 'i', 'd', 'e'):
        if (out->wide_count >= MuTFF_MAX_WIDE_ATOMS) {
          return MuTFFErrorOutOfMemory;
        }
        MuTFF_FN(mutff_read_wide_atom, &out->wide[out->wide_count]);
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

  return MuTFFErrorNone;
}

MuTFFError mutff_write_movie_file(mutff_file_t *fd, size_t *n,
                                  const MuTFFMovieFile *in) {
  MuTFFError err;
  size_t bytes;
  *n = 0;

  if (in->file_type_present) {
    MuTFF_FN(mutff_write_file_type_atom, &in->file_type);
  }
  MuTFF_FN(mutff_write_movie_atom, &in->movie);
  for (size_t i = 0; i < in->movie_data_count; ++i) {
    MuTFF_FN(mutff_write_movie_data_atom, &in->movie_data[i]);
  }
  for (size_t i = 0; i < in->free_count; ++i) {
    MuTFF_FN(mutff_write_free_atom, &in->free[i]);
  }
  for (size_t i = 0; i < in->skip_count; ++i) {
    MuTFF_FN(mutff_write_skip_atom, &in->skip[i]);
  }
  for (size_t i = 0; i < in->wide_count; ++i) {
    MuTFF_FN(mutff_write_wide_atom, &in->wide[i]);
  }
  if (in->preview_present) {
    MuTFF_FN(mutff_write_preview_atom, &in->preview);
  }

  return MuTFFErrorNone;
}
