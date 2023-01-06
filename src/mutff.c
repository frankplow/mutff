///
/// @file      mutff.c
/// @author    Frank Plowman <post@frankplowman.com>
/// @brief     MuTFF QuickTime file format library main source file
/// @copyright 2022 Frank Plowman
/// @license   This project is released under the GNU Public License Version 3.
///            For the terms of this license, see [LICENSE.md](LICENSE.md)
///

#include "mutff.h"

#include <stdlib.h>

// Convert a number from network (big) endian to host endian.
// These must be implemented here as newlib does not provide the
// standard library implementations ntohs & ntohl (arpa/inet.h).
//
// taken from:
// https://stackoverflow.com/questions/2100331/macro-definition-to-determine-big-endian-or-little-endian-machine
static uint16_t mutff_ntoh_16(uint16_t n) {
  unsigned char *np = (unsigned char *)&n;

  return (np[0] << 8) | np[1];
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

  return (np[0] << 16) | (np[1] << 8) | np[2];
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

  return (np[0] << 24) | (np[1] << 16) | (np[2] << 8) | np[3];
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

MuTFFError mutff_peek_atom_header(FILE *fd, MuTFFAtomHeader *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  fseek(fd, -8, SEEK_CUR);
  ret += -8;
  return 0;
}

MuTFFError mutff_read_quickdraw_rect(FILE *fd, MuTFFQuickDrawRect *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u16(fd, &out->top)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u16(fd, &out->left)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u16(fd, &out->bottom)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u16(fd, &out->right)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_write_quickdraw_rect(FILE *fd, const MuTFFQuickDrawRect *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u16(fd, in->top)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u16(fd, in->left)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u16(fd, in->bottom)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u16(fd, in->right)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_read_quickdraw_region(FILE *fd, MuTFFQuickDrawRegion *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u16(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_quickdraw_rect(fd, &out->rect)) < 0) {
    return err;
  }
  ret += err;
  const uint16_t data_size = out->size - ret;
  for (uint16_t i = 0; i < data_size; ++i) {
    if ((err = mutff_read_u8(fd, (uint8_t *)&out->data[i])) < 0) {
      return err;
    }
    ret += err;
  }

  return ret;
}

MuTFFError mutff_write_quickdraw_region(FILE *fd,
                                        const MuTFFQuickDrawRegion *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u16(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_quickdraw_rect(fd, &in->rect)) < 0) {
    return err;
  }
  ret += err;
  for (uint16_t i = 0; i < in->size - 10; ++i) {
    if ((err = mutff_write_u8(fd, in->data[i])) < 0) {
      return err;
    }
    ret += err;
  }
  return ret;
}

MuTFFError mutff_read_file_type_compatibility_atom(
    FILE *fd, MuTFFFileTypeCompatibilityAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;

  // read fixed-length data
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->major_brand)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->minor_version)) < 0) {
    return err;
  }
  ret += err;

  // read variable-length data
  const size_t compatible_brands_length = (out->size - 16);
  if (compatible_brands_length % 4 != 0) {
    return MuTFFErrorBadFormat;
  }
  out->compatible_brands_count = compatible_brands_length / 4;
  if (out->compatible_brands_count > MuTFF_MAX_COMPATIBLE_BRANDS) {
    return MuTFFErrorOutOfMemory;
  }
  for (size_t i = 0; i < out->compatible_brands_count; ++i) {
    if ((err = mutff_read_u32(fd, &out->compatible_brands[i])) < 0) {
      return err;
    }
    ret += err;
  }

  return ret;
}

MuTFFError mutff_write_file_type_compatibility_atom(
    FILE *fd, const MuTFFFileTypeCompatibilityAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->major_brand)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->minor_version)) < 0) {
    return err;
  }
  ret += err;
  for (size_t i = 0; i < in->compatible_brands_count; ++i) {
    if ((err = mutff_write_u32(fd, in->compatible_brands[i])) < 0) {
      return err;
    }
    ret += err;
  }
  return ret;
}

MuTFFError mutff_read_movie_data_atom(FILE *fd, MuTFFMovieDataAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  fseek(fd, out->size - 8, SEEK_CUR);
  ret += out->size - 8;
  return ret;
}

MuTFFError mutff_write_movie_data_atom(FILE *fd, const MuTFFMovieDataAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  for (uint32_t i = 0; i < in->size - 8; ++i) {
    fputc(0x00, fd);
  }
  ret += in->size - 8;
  return ret;
}

MuTFFError mutff_read_free_atom(FILE *fd, MuTFFFreeAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  fseek(fd, out->size - 8, SEEK_CUR);
  ret += out->size - 8;
  return ret;
}

MuTFFError mutff_write_free_atom(FILE *fd, const MuTFFFreeAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  for (uint32_t i = 0; i < in->size - 8; ++i) {
    fputc(0x00, fd);
  }
  ret += in->size - 8;
  return ret;
}

MuTFFError mutff_read_skip_atom(FILE *fd, MuTFFSkipAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  fseek(fd, out->size - 8, SEEK_CUR);
  ret += out->size - 8;
  return ret;
}

MuTFFError mutff_write_skip_atom(FILE *fd, const MuTFFSkipAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  for (uint32_t i = 0; i < in->size - 8; ++i) {
    fputc(0x00, fd);
  }
  ret += in->size - 8;
  return ret;
}

MuTFFError mutff_read_wide_atom(FILE *fd, MuTFFWideAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  fseek(fd, out->size - 8, SEEK_CUR);
  ret += out->size - 8;
  return ret;
}

MuTFFError mutff_write_wide_atom(FILE *fd, const MuTFFWideAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  for (uint32_t i = 0; i < in->size - 8; ++i) {
    fputc(0x00, fd);
  }
  ret += in->size - 8;
  return ret;
}

MuTFFError mutff_read_preview_atom(FILE *fd, MuTFFPreviewAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->modification_time)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u16(fd, &out->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->atom_type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u16(fd, &out->atom_index)) < 0) {
    return err;
  }
  ret += err;
  fseek(fd, out->size - 20, SEEK_CUR);
  ret += out->size - 20;
  return ret;
}

MuTFFError mutff_write_preview_atom(FILE *fd, const MuTFFPreviewAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->modification_time)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u16(fd, in->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->atom_type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u16(fd, in->atom_index)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_read_movie_header_atom(FILE *fd, MuTFFMovieHeaderAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u8(fd, &out->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u24(fd, &out->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->creation_time)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->modification_time)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->time_scale)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->duration)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_q16_16(fd, &out->preferred_rate)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_q8_8(fd, &out->preferred_volume)) < 0) {
    return err;
  }
  ret += err;
  fseek(fd, 10, SEEK_CUR);
  ret += 10;
  for (size_t j = 0; j < 3; ++j) {
    for (size_t i = 0; i < 3; ++i) {
      if ((err = mutff_read_u32(fd, &out->matrix_structure[j][i])) < 0) {
        return err;
      }
      ret += err;
    }
  }
  if ((err = mutff_read_u32(fd, &out->preview_time)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->preview_duration)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->poster_time)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->selection_time)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->selection_duration)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->current_time)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->next_track_id)) < 0) {
    return err;
  }
  ret += err;
  fseek(fd, out->size - 108, SEEK_CUR);
  ret += out->size - 108;
  return ret;
}

MuTFFError mutff_write_movie_header_atom(FILE *fd,
                                         const MuTFFMovieHeaderAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u8(fd, in->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u24(fd, in->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->creation_time)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->modification_time)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->time_scale)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->duration)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_q16_16(fd, in->preferred_rate)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_q8_8(fd, in->preferred_volume)) < 0) {
    return err;
  }
  ret += err;
  for (size_t i = 0; i < 10; ++i) {
    if ((err = mutff_write_u8(fd, in->_reserved[i])) < 0) {
      return err;
    }
    ret += err;
  }
  for (size_t j = 0; j < 3; ++j) {
    for (size_t i = 0; i < 3; ++i) {
      if ((err = mutff_write_u32(fd, in->matrix_structure[j][i])) < 0) {
        return err;
      }
      ret += err;
    }
  }
  if ((err = mutff_write_u32(fd, in->preview_time)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->preview_duration)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->poster_time)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->selection_time)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->selection_duration)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->current_time)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->next_track_id)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_read_clipping_region_atom(FILE *fd,
                                           MuTFFClippingRegionAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  MuTFFError region_size;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  if ((region_size = mutff_read_quickdraw_region(fd, &out->region)) < 0) {
    return err;
  }
  ret += region_size;
  fseek(fd, out->size - ret, SEEK_CUR);
  ret += out->size - ret;
  return ret;
}

MuTFFError mutff_write_clipping_region_atom(FILE *fd,
                                            const MuTFFClippingRegionAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_quickdraw_region(fd, &in->region)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_read_clipping_atom(FILE *fd, MuTFFClippingAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_clipping_region_atom(fd, &out->clipping_region)) < 0) {
    return err;
  }
  ret += err;
  fseek(fd, out->size - out->clipping_region.size - 8, SEEK_CUR);
  ret += out->size - out->clipping_region.size - 8;
  return ret;
}

MuTFFError mutff_write_clipping_atom(FILE *fd, const MuTFFClippingAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_clipping_region_atom(fd, &in->clipping_region)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_read_color_table_atom(FILE *fd, MuTFFColorTableAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->color_table_seed)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u16(fd, &out->color_table_flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u16(fd, &out->color_table_size)) < 0) {
    return err;
  }
  ret += err;

  // read color array
  const size_t size = (out->color_table_size + 1) * 8;
  if (size != out->size - 16) {
    return MuTFFErrorBadFormat;
  }
  for (size_t i = 0; i <= out->color_table_size; ++i) {
    for (size_t j = 0; j < 4; ++j) {
      if ((err = mutff_read_u16(fd, &out->color_array[i][j])) < 0) {
        return err;
      }
      ret += err;
    }
  }

  return ret;
}

MuTFFError mutff_write_color_table_atom(FILE *fd,
                                        const MuTFFColorTableAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->color_table_seed)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u16(fd, in->color_table_flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u16(fd, in->color_table_size)) < 0) {
    return err;
  }
  ret += err;
  for (uint16_t i = 0; i <= in->color_table_size; ++i) {
    if ((err = mutff_write_u16(fd, in->color_array[i][0])) < 0) {
      return err;
    }
    ret += err;
    if ((err = mutff_write_u16(fd, in->color_array[i][1])) < 0) {
      return err;
    }
    ret += err;
    if ((err = mutff_write_u16(fd, in->color_array[i][2])) < 0) {
      return err;
    }
    ret += err;
    if ((err = mutff_write_u16(fd, in->color_array[i][3])) < 0) {
      return err;
    }
    ret += err;
  }
  return ret;
}

MuTFFError mutff_read_user_data_list_entry(FILE *fd,
                                           MuTFFUserDataListEntry *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;

  // read variable-length data
  const uint32_t data_size = out->size - 8;
  if (data_size > MuTFF_MAX_USER_DATA_ENTRY_SIZE) {
    return MuTFFErrorOutOfMemory;
  }
  for (uint32_t i = 0; i < data_size; ++i) {
    if ((err = mutff_read_u8(fd, (uint8_t *)&out->data[i])) < 0) {
      return err;
    }
    ret += err;
  }

  return ret;
}

MuTFFError mutff_write_user_data_list_entry(FILE *fd,
                                            const MuTFFUserDataListEntry *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  const uint32_t data_size = in->size - 8;
  for (uint32_t i = 0; i < data_size; ++i) {
    if ((err = mutff_write_u8(fd, in->data[i])) < 0) {
      return err;
    }
    ret += err;
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
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;

  // read children
  i = 0;
  offset = 8;
  while (offset < out->size) {
    if (i >= MuTFF_MAX_USER_DATA_ITEMS) {
      return MuTFFErrorOutOfMemory;
    }
    if ((err = mutff_peek_atom_header(fd, &header)) < 0) {
      return err;
    }
    ret += err;
    offset += header.size;
    if (offset > out->size) {
      return MuTFFErrorBadFormat;
    }
    if ((err = mutff_read_user_data_list_entry(fd, &out->user_data_list[i])) <
        0) {
      return err;
    }
    ret += err;

    i++;
  }

  return ret;
}

MuTFFError mutff_write_user_data_atom(FILE *fd, const MuTFFUserDataAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  MuTFFAtomHeader header;
  size_t i;
  size_t offset;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  i = 0;
  offset = 8;
  while (offset < in->size) {
    offset += in->user_data_list[i].size;
    if (offset > in->size) {
      return MuTFFErrorBadFormat;
    }
    if ((err = mutff_write_user_data_list_entry(fd, &in->user_data_list[i])) <
        0) {
      return err;
    }
    ret += err;
    i++;
  }
  return ret;
}

MuTFFError mutff_read_track_header_atom(FILE *fd, MuTFFTrackHeaderAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u8(fd, &out->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u24(fd, &out->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->creation_time)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->modification_time)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->track_id)) < 0) {
    return err;
  }
  ret += err;
  fseek(fd, 4, SEEK_CUR);
  ret += 4;
  if ((err = mutff_read_u32(fd, &out->duration)) < 0) {
    return err;
  }
  ret += err;
  fseek(fd, 8, SEEK_CUR);
  ret += 8;
  if ((err = mutff_read_u16(fd, &out->layer)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u16(fd, &out->alternate_group)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_q8_8(fd, &out->volume)) < 0) {
    return err;
  }
  ret += err;
  fseek(fd, 2, SEEK_CUR);
  ret += 2;
  for (size_t j = 0; j < 3; ++j) {
    for (size_t i = 0; i < 3; ++i) {
      if ((err = mutff_read_u32(fd, &out->matrix_structure[j][i])) < 0) {
        return err;
      }
      ret += err;
    }
  }
  if ((err = mutff_read_q16_16(fd, &out->track_width)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_q16_16(fd, &out->track_height)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_write_track_header_atom(FILE *fd,
                                         const MuTFFTrackHeaderAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u8(fd, in->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u24(fd, in->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->creation_time)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->modification_time)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->track_id)) < 0) {
    return err;
  }
  ret += err;
  for (size_t i = 0; i < 4; ++i) {
    if ((err = mutff_write_u8(fd, in->_reserved_1[i])) < 0) {
      return err;
    }
    ret += err;
  }
  if ((err = mutff_write_u32(fd, in->duration)) < 0) {
    return err;
  }
  ret += err;
  for (size_t i = 0; i < 8; ++i) {
    if ((err = mutff_write_u8(fd, in->_reserved_2[i])) < 0) {
      return err;
    }
    ret += err;
  }
  if ((err = mutff_write_u16(fd, in->layer)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u16(fd, in->alternate_group)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_q8_8(fd, in->volume)) < 0) {
    return err;
  }
  ret += err;
  for (size_t i = 0; i < 2; ++i) {
    if ((err = mutff_write_u8(fd, in->_reserved_3[i])) < 0) {
      return err;
    }
    ret += err;
  }
  for (size_t j = 0; j < 3; ++j) {
    for (size_t i = 0; i < 3; ++i) {
      if ((err = mutff_write_u32(fd, in->matrix_structure[j][i])) < 0) {
        return err;
      }
      ret += err;
    }
  }
  if ((err = mutff_write_q16_16(fd, in->track_width)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_q16_16(fd, in->track_height)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_read_track_clean_aperture_dimensions_atom(
    FILE *fd, MuTFFTrackCleanApertureDimensionsAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u8(fd, &out->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u24(fd, &out->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_q16_16(fd, &out->width)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_q16_16(fd, &out->height)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_write_track_clean_aperture_dimensions_atom(
    FILE *fd, const MuTFFTrackCleanApertureDimensionsAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u8(fd, in->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u24(fd, in->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_q16_16(fd, in->width)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_q16_16(fd, in->height)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_read_track_production_aperture_dimensions_atom(
    FILE *fd, MuTFFTrackProductionApertureDimensionsAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u8(fd, &out->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u24(fd, &out->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_q16_16(fd, &out->width)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_q16_16(fd, &out->height)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_write_track_production_aperture_dimensions_atom(
    FILE *fd, const MuTFFTrackProductionApertureDimensionsAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u8(fd, in->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u24(fd, in->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_q16_16(fd, in->width)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_q16_16(fd, in->height)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_read_track_encoded_pixels_dimensions_atom(
    FILE *fd, MuTFFTrackEncodedPixelsDimensionsAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u8(fd, &out->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u24(fd, &out->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_q16_16(fd, &out->width)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_q16_16(fd, &out->height)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_write_track_encoded_pixels_dimensions_atom(
    FILE *fd, const MuTFFTrackEncodedPixelsDimensionsAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u8(fd, in->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u24(fd, in->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_q16_16(fd, in->width)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_q16_16(fd, in->height)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_read_track_aperture_mode_dimensions_atom(
    FILE *fd, MuTFFTrackApertureModeDimensionsAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;

  // read children
  size_t offset = 8;
  MuTFFAtomHeader header;
  while (offset < out->size) {
    if ((err = mutff_peek_atom_header(fd, &header)) < 0) {
      return err;
    }
    ret += err;
    offset += header.size;
    if (offset > out->size) {
      return MuTFFErrorBadFormat;
    }

    switch (header.type) {
      /* case MuTFF_FOUR_C("clef"): */
      case 0x636c6566:
        if ((err = mutff_read_track_clean_aperture_dimensions_atom(
                 fd, &out->track_clean_aperture_dimension)) < 0) {
          return err;
        }
        ret += err;
        break;
      /* case MuTFF_FOUR_C("prof"): */
      case 0x70726f66:
        if ((err = mutff_read_track_production_aperture_dimensions_atom(
                 fd, &out->track_production_aperture_dimension)) < 0) {
          return err;
        }
        ret += err;
        break;
      /* case MuTFF_FOUR_C("enof"): */
      case 0x656e6f66:
        if ((err = mutff_read_track_encoded_pixels_dimensions_atom(
                 fd, &out->track_encoded_pixels_dimension)) < 0) {
          return err;
        }
        ret += err;
        break;
      default:
        // Unrecognised atom type, skip atom
        fseek(fd, header.size, SEEK_CUR);
        ret += header.size;
    }
  }

  return ret;
}

MuTFFError mutff_write_track_aperture_mode_dimensions_atom(
    FILE *fd, const MuTFFTrackApertureModeDimensionsAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_track_clean_aperture_dimensions_atom(
           fd, &in->track_clean_aperture_dimension)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_track_production_aperture_dimensions_atom(
           fd, &in->track_production_aperture_dimension)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_track_encoded_pixels_dimensions_atom(
           fd, &in->track_encoded_pixels_dimension)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_read_sample_description(FILE *fd,
                                         MuTFFSampleDescription *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->data_format)) < 0) {
    return err;
  }
  ret += err;
  fseek(fd, 6, SEEK_CUR);
  ret += 6;
  if ((err = mutff_read_u16(fd, &out->data_reference_index)) < 0) {
    return err;
  }
  ret += err;
  const uint32_t data_size = out->size - 16;
  for (uint32_t i = 0; i < data_size; ++i) {
    if ((err = mutff_read_u8(fd, (uint8_t *)&out->additional_data[i])) < 0) {
      return err;
    }
    ret += err;
  }
  return ret;
}

MuTFFError mutff_write_sample_description(FILE *fd,
                                          const MuTFFSampleDescription *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->data_format)) < 0) {
    return err;
  }
  ret += err;
  for (size_t i = 0; i < 6; ++i) {
    if ((err = mutff_write_u8(fd, in->_reserved[i])) < 0) {
      return err;
    }
    ret += err;
  }
  if ((err = mutff_write_u16(fd, in->data_reference_index)) < 0) {
    return err;
  }
  ret += err;
  const size_t data_size = in->size - 16;
  for (size_t i = 0; i < data_size; ++i) {
    if ((err = mutff_write_u8(fd, in->additional_data[i])) < 0) {
      return err;
    }
    ret += err;
  }
  return ret;
}

MuTFFError mutff_read_compressed_matte_atom(FILE *fd,
                                            MuTFFCompressedMatteAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u8(fd, &out->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u24(fd, &out->flags)) < 0) {
    return err;
  }
  ret += err;

  // read sample description
  if ((err = mutff_read_sample_description(
           fd, &out->matte_image_description_structure)) < 0) {
    return err;
  }
  ret += err;

  // read matte data
  out->matte_data_len =
      out->size - 12 - out->matte_image_description_structure.size;
  for (uint32_t i = 0; i < out->matte_data_len; ++i) {
    if ((err = mutff_read_u8(fd, (uint8_t *)&out->matte_data[i])) < 0) {
      return err;
    }
    ret += err;
  }

  return ret;
}

MuTFFError mutff_write_compressed_matte_atom(
    FILE *fd, const MuTFFCompressedMatteAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u8(fd, in->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u24(fd, in->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_sample_description(
           fd, &in->matte_image_description_structure)) < 0) {
    return err;
  }
  ret += err;
  for (size_t i = 0; i < in->matte_data_len; ++i) {
    if ((err = mutff_write_u8(fd, in->matte_data[i])) < 0) {
      return err;
    }
    ret += err;
  }
  return ret;
}

MuTFFError mutff_read_track_matte_atom(FILE *fd, MuTFFTrackMatteAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;

  // read child atom
  if ((err = mutff_read_compressed_matte_atom(
           fd, &out->compressed_matte_atom)) < 0) {
    return err;
  }
  ret += err;

  // skip any remaining data
  fseek(fd, out->size - out->compressed_matte_atom.size - 8, SEEK_CUR);
  ret += out->size - out->compressed_matte_atom.size - 8;

  return ret;
}

MuTFFError mutff_write_track_matte_atom(FILE *fd,
                                        const MuTFFTrackMatteAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_compressed_matte_atom(
           fd, &in->compressed_matte_atom)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_read_edit_list_entry(FILE *fd, MuTFFEditListEntry *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->track_duration)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->media_time)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_q16_16(fd, &out->media_rate)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_write_edit_list_entry(FILE *fd, const MuTFFEditListEntry *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->track_duration)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->media_time)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_q16_16(fd, in->media_rate)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_read_edit_list_atom(FILE *fd, MuTFFEditListAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u8(fd, &out->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u24(fd, &out->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->number_of_entries)) < 0) {
    return err;
  }
  ret += err;

  // read edit list table
  if (out->number_of_entries > MuTFF_MAX_EDIT_LIST_ENTRIES) {
    return MuTFFErrorOutOfMemory;
  }
  const size_t edit_list_table_size = out->size - 16;
  if (edit_list_table_size != out->number_of_entries * 12) {
    return MuTFFErrorBadFormat;
  }
  for (size_t i = 0; i < out->number_of_entries; ++i) {
    if ((err = mutff_read_edit_list_entry(fd, &out->edit_list_table[i])) < 0) {
      return err;
    }
    ret += err;
  }

  return ret;
}

MuTFFError mutff_write_edit_list_atom(FILE *fd, const MuTFFEditListAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u8(fd, in->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u24(fd, in->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->number_of_entries)) < 0) {
    return err;
  }
  ret += err;
  for (size_t i = 0; i < in->number_of_entries; ++i) {
    if ((err = mutff_write_edit_list_entry(fd, &in->edit_list_table[i])) < 0) {
      return err;
    }
    ret += err;
  }
  return ret;
}

MuTFFError mutff_read_edit_atom(FILE *fd, MuTFFEditAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;

  // read child atom
  if ((err = mutff_read_edit_list_atom(fd, &out->edit_list_atom)) < 0) {
    return err;
  }
  ret += err;

  // skip any remaining data
  fseek(fd, out->size - out->edit_list_atom.size - 8, SEEK_CUR);
  ret += out->size - out->edit_list_atom.size - 8;

  return ret;
}

MuTFFError mutff_write_edit_atom(FILE *fd, const MuTFFEditAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_edit_list_atom(fd, &in->edit_list_atom)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_read_track_reference_type_atom(
    FILE *fd, MuTFFTrackReferenceTypeAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;

  // read track references
  if ((out->size - 8) % 4 != 0) {
    return MuTFFErrorBadFormat;
  }
  out->track_id_count = (out->size - 8) / 4;
  if (out->track_id_count > MuTFF_MAX_TRACK_REFERENCE_TYPE_TRACK_IDS) {
    return MuTFFErrorOutOfMemory;
  }
  for (unsigned int i = 0; i < out->track_id_count; ++i) {
    if ((err = mutff_read_u32(fd, &out->track_ids[i])) < 0) {
      return err;
    }
    ret += err;
  }

  return ret;
}

MuTFFError mutff_write_track_reference_type_atom(
    FILE *fd, const MuTFFTrackReferenceTypeAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  for (size_t i = 0; i < in->track_id_count; ++i) {
    if ((err = mutff_write_u32(fd, in->track_ids[i])) < 0) {
      return err;
    }
    ret += err;
  }
  return ret;
}

MuTFFError mutff_write_track_reference_atom(FILE *fd,
                                            const MuTFFTrackReferenceAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  for (size_t i = 0; i < in->track_reference_type_count; ++i) {
    if ((err = mutff_write_track_reference_type_atom(
             fd, &in->track_reference_type[i])) < 0) {
      return err;
    }
    ret += err;
  }
  return ret;
}

MuTFFError mutff_read_track_reference_atom(FILE *fd,
                                           MuTFFTrackReferenceAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;

  // read children
  size_t offset = 8;
  size_t i = 0;
  MuTFFAtomHeader header;
  while (offset < out->size) {
    if (i >= MuTFF_MAX_TRACK_REFERENCE_TYPE_ATOMS) {
      return MuTFFErrorOutOfMemory;
    }
    if ((err = mutff_peek_atom_header(fd, &header)) < 0) {
      return err;
    }
    ret += err;
    offset += header.size;
    if (offset > out->size) {
      return MuTFFErrorBadFormat;
    }
    if ((err = mutff_read_track_reference_type_atom(
             fd, &out->track_reference_type[i])) < 0) {
      return err;
    };
    ret += err;
    i++;
  }
  out->track_reference_type_count = i;

  return ret;
}

MuTFFError mutff_read_track_exclude_from_autoselection_atom(
    FILE *fd, MuTFFTrackExcludeFromAutoselectionAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_write_track_exclude_from_autoselection_atom(
    FILE *fd, const MuTFFTrackExcludeFromAutoselectionAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_read_track_load_settings_atom(
    FILE *fd, MuTFFTrackLoadSettingsAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->preload_start_time)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->preload_duration)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->preload_flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->default_hints)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_write_track_load_settings_atom(
    FILE *fd, const MuTFFTrackLoadSettingsAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->preload_start_time)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->preload_duration)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->preload_flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->default_hints)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_read_input_type_atom(FILE *fd, MuTFFInputTypeAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->input_type)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_write_input_type_atom(FILE *fd, const MuTFFInputTypeAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->input_type)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_read_object_id_atom(FILE *fd, MuTFFObjectIDAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->object_id)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_write_object_id_atom(FILE *fd, const MuTFFObjectIDAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->object_id)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_read_track_input_atom(FILE *fd, MuTFFTrackInputAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->atom_id)) < 0) {
    return err;
  }
  ret += err;
  fseek(fd, 2, SEEK_CUR);
  ret += 2;
  if ((err = mutff_read_u16(fd, &out->child_count)) < 0) {
    return err;
  }
  ret += err;
  fseek(fd, 4, SEEK_CUR);
  ret += 4;

  // read children
  size_t offset = 20;
  MuTFFAtomHeader header;
  while (offset < out->size) {
    if ((err = mutff_peek_atom_header(fd, &header)) < 0) {
      return err;
    }
    ret += err;
    offset += header.size;
    if (offset > out->size) {
      return MuTFFErrorBadFormat;
    }
    switch (header.type) {
      /* case MuTFF_FOUR_C("\0\0ty"): */
      case 0x00007479:
        if ((err = mutff_read_input_type_atom(fd, &out->input_type_atom)) < 0) {
          return err;
        }
        ret += err;
        break;
      /* case MuTFF_FOUR_C("obid"): */
      case 0x6f626964:
        if ((err = mutff_read_object_id_atom(fd, &out->object_id_atom)) < 0) {
          return err;
        }
        ret += err;
        break;
      default:
        fseek(fd, header.size, SEEK_CUR);
        ret += header.size;
    }
  }

  return ret;
}

MuTFFError mutff_write_track_input_atom(FILE *fd,
                                        const MuTFFTrackInputAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->atom_id)) < 0) {
    return err;
  }
  ret += err;
  for (size_t i = 0; i < 2; ++i) {
    if ((err = mutff_write_u8(fd, 0)) < 0) {
      return err;
    }
    ret += err;
  }
  if ((err = mutff_write_u16(fd, in->child_count)) < 0) {
    return err;
  }
  ret += err;
  for (size_t i = 0; i < 4; ++i) {
    if ((err = mutff_write_u8(fd, 0)) < 0) {
      return err;
    }
    ret += err;
  }
  if ((err = mutff_write_input_type_atom(fd, &in->input_type_atom)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_object_id_atom(fd, &in->object_id_atom)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_read_track_input_map_atom(FILE *fd,
                                           MuTFFTrackInputMapAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;

  // read children
  size_t offset = 8;
  size_t i = 0;
  MuTFFAtomHeader header;
  while (offset < out->size) {
    if (i >= MuTFF_MAX_TRACK_REFERENCE_TYPE_ATOMS) {
      return MuTFFErrorOutOfMemory;
    }
    if ((err = mutff_peek_atom_header(fd, &header)) < 0) {
      return err;
    }
    ret += err;
    offset += header.size;
    if (offset > out->size) {
      return MuTFFErrorBadFormat;
    }
    if (header.type == MuTFF_FOUR_C("\0\0in")) {
      if ((err = mutff_read_track_input_atom(fd, &out->track_input_atoms[i])) <
          0) {
        return err;
      }
      ret += err;
      i++;
    } else {
      fseek(fd, header.size, SEEK_CUR);
      ret += header.size;
    }
  }
  out->track_input_atom_count = i;

  return ret;
}

MuTFFError mutff_write_track_input_map_atom(FILE *fd,
                                            const MuTFFTrackInputMapAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  for (size_t i = 0; i < in->track_input_atom_count; ++i) {
    if ((err = mutff_write_track_input_atom(fd, &in->track_input_atoms[i])) <
        0) {
      return err;
    }
    ret += err;
  }
  return ret;
}

MuTFFError mutff_read_media_header_atom(FILE *fd, MuTFFMediaHeaderAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u8(fd, &out->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u24(fd, &out->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->creation_time)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->modification_time)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->time_scale)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->duration)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u16(fd, &out->language)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u16(fd, &out->quality)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_write_media_header_atom(FILE *fd,
                                         const MuTFFMediaHeaderAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u8(fd, in->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u24(fd, in->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->creation_time)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->modification_time)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->time_scale)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->duration)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u16(fd, in->language)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u16(fd, in->quality)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_read_extended_language_tag_atom(
    FILE *fd, MuTFFExtendedLanguageTagAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u8(fd, &out->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u24(fd, &out->flags)) < 0) {
    return err;
  }
  ret += err;

  // read variable-length data
  const size_t tag_length = out->size - 12;
  if (tag_length > MuTFF_MAX_LANGUAGE_TAG_LENGTH) {
    return MuTFFErrorOutOfMemory;
  }
  for (size_t i = 0; i < tag_length; ++i) {
    if ((err = mutff_read_u8(fd, (uint8_t *)&out->language_tag_string[i])) <
        0) {
      return err;
    }
    ret += err;
  }

  return ret;
}

MuTFFError mutff_write_extended_language_tag_atom(
    FILE *fd, const MuTFFExtendedLanguageTagAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  size_t i;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u8(fd, in->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u24(fd, in->flags)) < 0) {
    return err;
  }
  ret += err;
  i = 0;
  while (in->language_tag_string[i]) {
    if ((err = mutff_write_u8(fd, in->language_tag_string[i])) < 0) {
      return err;
    }
    ret += err;
    ++i;
  }
  for (; i < in->size - 12; ++i) {
    if ((err = mutff_write_u8(fd, 0)) < 0) {
      return err;
    }
    ret += err;
  }
  return ret;
}

MuTFFError mutff_read_handler_reference_atom(FILE *fd,
                                             MuTFFHandlerReferenceAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u8(fd, &out->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u24(fd, &out->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->component_type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->component_subtype)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->component_manufacturer)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->component_flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->component_flags_mask)) < 0) {
    return err;
  }
  ret += err;

  // read variable-length data
  const size_t name_length = out->size - 32;
  if (name_length > MuTFF_MAX_COMPONENT_NAME_LENGTH) {
    return MuTFFErrorOutOfMemory;
  }
  for (size_t i = 0; i < name_length; ++i) {
    if ((err = mutff_read_u8(fd, (uint8_t *)&out->component_name[i])) < 0) {
      return err;
    }
    ret += err;
  }

  return ret;
}

MuTFFError mutff_write_handler_reference_atom(
    FILE *fd, const MuTFFHandlerReferenceAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u8(fd, in->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u24(fd, in->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->component_type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->component_subtype)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->component_manufacturer)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->component_flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->component_flags_mask)) < 0) {
    return err;
  }
  ret += err;
  for (size_t i = 0; i < in->size - 32; ++i) {
    if ((err = mutff_write_u8(fd, in->component_name[i])) < 0) {
      return err;
    }
    ret += err;
  }
  return ret;
}

MuTFFError mutff_read_video_media_information_header_atom(
    FILE *fd, MuTFFVideoMediaInformationHeaderAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u8(fd, &out->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u24(fd, &out->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u16(fd, &out->graphics_mode)) < 0) {
    return err;
  }
  ret += err;
  for (size_t i = 0; i < 3; ++i) {
    if ((err = mutff_read_u16(fd, &out->opcolor[i])) < 0) {
      return err;
    }
    ret += err;
  }
  return ret;
}

MuTFFError mutff_write_video_media_information_header_atom(
    FILE *fd, const MuTFFVideoMediaInformationHeaderAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u8(fd, in->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u24(fd, in->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u16(fd, in->graphics_mode)) < 0) {
    return err;
  }
  ret += err;
  for (size_t i = 0; i < 3; ++i) {
    if ((err = mutff_write_u16(fd, in->opcolor[i])) < 0) {
      return err;
    }
    ret += err;
  }
  return ret;
}

MuTFFError mutff_read_data_reference(FILE *fd, MuTFFDataReference *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u8(fd, &out->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u24(fd, &out->flags)) < 0) {
    return err;
  }
  ret += err;

  // read variable-length data
  const size_t data_size = out->size - 12;
  if (data_size > MuTFF_MAX_DATA_REFERENCE_DATA_SIZE) {
    return MuTFFErrorOutOfMemory;
  }
  for (size_t i = 0; i < data_size; ++i) {
    if ((err = mutff_read_u8(fd, (uint8_t *)&out->data[i])) < 0) {
      return err;
    }
    ret += err;
  }

  return ret;
}

MuTFFError mutff_write_data_reference(FILE *fd, const MuTFFDataReference *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u8(fd, in->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u24(fd, in->flags)) < 0) {
    return err;
  }
  ret += err;
  for (size_t i = 0; i < in->size - 12; ++i) {
    if ((err = mutff_write_u8(fd, in->data[i])) < 0) {
      return err;
    }
    ret += err;
  }
  return ret;
}

MuTFFError mutff_read_data_reference_atom(FILE *fd,
                                          MuTFFDataReferenceAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  MuTFFAtomHeader header;
  size_t offset;

  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u8(fd, &out->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u24(fd, &out->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->number_of_entries)) < 0) {
    return err;
  }
  ret += err;

  // read child atoms
  if (out->number_of_entries > MuTFF_MAX_DATA_REFERENCES) {
    return MuTFFErrorOutOfMemory;
  }
  offset = 16;
  for (size_t i = 0; i < out->number_of_entries; ++i) {
    if ((err = mutff_peek_atom_header(fd, &header)) < 0) {
      return err;
    }
    ret += err;
    offset += header.size;
    if (offset > out->size) {
      return MuTFFErrorBadFormat;
    }
    if ((err = mutff_read_data_reference(fd, &out->data_references[i])) < 0) {
      return err;
    }
    ret += err;
  }

  // skip any remaining space
  fseek(fd, out->size - offset, SEEK_CUR);
  ret += out->size - offset;

  return ret;
}

MuTFFError mutff_write_data_reference_atom(FILE *fd,
                                           const MuTFFDataReferenceAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  size_t offset;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u8(fd, in->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u24(fd, in->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->number_of_entries)) < 0) {
    return err;
  }
  ret += err;
  offset = 16;
  for (uint32_t i = 0; i < in->number_of_entries; ++i) {
    offset += in->data_references[i].size;
    if (offset > in->size) {
      return MuTFFErrorBadFormat;
    }
    if ((err = mutff_write_data_reference(fd, &in->data_references[i])) < 0) {
      return err;
    }
    ret += err;
  }
  for (; offset < in->size; ++offset) {
    if ((err = mutff_write_u8(fd, 0)) < 0) {
      return err;
    }
    ret += err;
  }
  return ret;
}

MuTFFError mutff_read_data_information_atom(FILE *fd,
                                            MuTFFDataInformationAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;

  // read child atom
  if ((err = mutff_read_data_reference_atom(fd, &out->data_reference)) < 0) {
    return err;
  }
  ret += err;

  // skip any remaining space
  fseek(fd, out->size - out->data_reference.size - 8, SEEK_CUR);
  ret += out->size - out->data_reference.size - 8;

  return ret;
}

MuTFFError mutff_write_data_information_atom(
    FILE *fd, const MuTFFDataInformationAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_data_reference_atom(fd, &in->data_reference)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_read_sample_description_atom(FILE *fd,
                                              MuTFFSampleDescriptionAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  MuTFFAtomHeader header;
  size_t offset;

  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u8(fd, &out->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u24(fd, &out->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->number_of_entries)) < 0) {
    return err;
  }
  ret += err;

  // read child atoms
  if (out->number_of_entries > MuTFF_MAX_SAMPLE_DESCRIPTION_TABLE_LEN) {
    return MuTFFErrorOutOfMemory;
  }
  offset = 16;
  for (size_t i = 0; i < out->number_of_entries; ++i) {
    if ((err = mutff_peek_atom_header(fd, &header)) < 0) {
      return err;
    }
    ret += err;
    offset += header.size;
    if (offset > out->size) {
      return MuTFFErrorBadFormat;
    }
    if ((err = mutff_read_sample_description(
             fd, &out->sample_description_table[i])) < 0) {
      return err;
    }
    ret += err;
  }

  // skip any remaining space
  fseek(fd, out->size - offset, SEEK_CUR);
  ret += out->size - offset;

  return ret;
}

MuTFFError mutff_write_sample_description_atom(
    FILE *fd, const MuTFFSampleDescriptionAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  size_t offset;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u8(fd, in->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u24(fd, in->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->number_of_entries)) < 0) {
    return err;
  }
  ret += err;
  offset = 16;
  for (size_t i = 0; i < in->number_of_entries; ++i) {
    offset += in->sample_description_table[i].size;
    if (offset > in->size) {
      return MuTFFErrorBadFormat;
    }
    if ((err = mutff_write_sample_description(
             fd, &in->sample_description_table[i])) < 0) {
      return err;
    }
    ret += err;
  }
  for (; offset < in->size; ++offset) {
    if ((err = mutff_write_u8(fd, 0)) < 0) {
      return err;
    }
    ret += err;
  }
  return ret;
}

MuTFFError mutff_read_time_to_sample_table_entry(
    FILE *fd, MuTFFTimeToSampleTableEntry *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->sample_count)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->sample_duration)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_write_time_to_sample_table_entry(
    FILE *fd, const MuTFFTimeToSampleTableEntry *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->sample_count)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->sample_duration)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_read_time_to_sample_atom(FILE *fd,
                                          MuTFFTimeToSampleAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u8(fd, &out->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u24(fd, &out->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->number_of_entries)) < 0) {
    return err;
  }
  ret += err;

  // read time to sample table
  if (out->number_of_entries > MuTFF_MAX_TIME_TO_SAMPLE_TABLE_LEN) {
    return MuTFFErrorOutOfMemory;
  }
  const size_t table_size = out->size - 16;
  if (table_size != out->number_of_entries * 8) {
    return MuTFFErrorBadFormat;
  }
  for (size_t i = 0; i < out->number_of_entries; ++i) {
    if ((err = mutff_read_time_to_sample_table_entry(
             fd, &out->time_to_sample_table[i])) < 0) {
      return err;
    }
    ret += err;
    ;
  }

  return ret;
}

MuTFFError mutff_write_time_to_sample_atom(FILE *fd,
                                           const MuTFFTimeToSampleAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u8(fd, in->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u24(fd, in->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->number_of_entries)) < 0) {
    return err;
  }
  ret += err;
  if (in->number_of_entries * 8 != in->size - 16) {
    return MuTFFErrorBadFormat;
  }
  for (uint32_t i = 0; i < in->number_of_entries; ++i) {
    if ((err = mutff_write_time_to_sample_table_entry(
             fd, &in->time_to_sample_table[i])) < 0) {
      return err;
    }
    ret += err;
  }
  return ret;
}

MuTFFError mutff_read_composition_offset_table_entry(
    FILE *fd, MuTFFCompositionOffsetTableEntry *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->sample_count)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->composition_offset)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_write_composition_offset_table_entry(
    FILE *fd, const MuTFFCompositionOffsetTableEntry *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->sample_count)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->composition_offset)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_read_composition_offset_atom(FILE *fd,
                                              MuTFFCompositionOffsetAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u8(fd, &out->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u24(fd, &out->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->entry_count)) < 0) {
    return err;
  }
  ret += err;

  // read composition offset table
  if (out->entry_count > MuTFF_MAX_COMPOSITION_OFFSET_TABLE_LEN) {
    return MuTFFErrorOutOfMemory;
  }
  const size_t table_size = out->size - 16;
  if (table_size != out->entry_count * 8) {
    return MuTFFErrorBadFormat;
  }
  for (size_t i = 0; i < out->entry_count; ++i) {
    if ((err = mutff_read_composition_offset_table_entry(
             fd, &out->composition_offset_table[i])) < 0) {
      return err;
    }
    ret += err;
    ;
  }

  return ret;
}

MuTFFError mutff_write_composition_offset_atom(
    FILE *fd, const MuTFFCompositionOffsetAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u8(fd, in->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u24(fd, in->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->entry_count)) < 0) {
    return err;
  }
  ret += err;
  if (in->entry_count * 8 != in->size - 16) {
    return MuTFFErrorBadFormat;
  }
  for (uint32_t i = 0; i < in->entry_count; ++i) {
    if ((err = mutff_write_composition_offset_table_entry(
             fd, &in->composition_offset_table[i])) < 0) {
      return err;
    }
    ret += err;
  }
  return ret;
}

MuTFFError mutff_read_composition_shift_least_greatest_atom(
    FILE *fd, MuTFFCompositionShiftLeastGreatestAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u8(fd, &out->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u24(fd, &out->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(
           fd, &out->composition_offset_to_display_offset_shift)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_i32(fd, &out->least_display_offset)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_i32(fd, &out->greatest_display_offset)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_i32(fd, &out->display_start_time)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_i32(fd, &out->display_end_time)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_write_composition_shift_least_greatest_atom(
    FILE *fd, const MuTFFCompositionShiftLeastGreatestAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u8(fd, in->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u24(fd, in->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(
           fd, in->composition_offset_to_display_offset_shift)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_i32(fd, in->least_display_offset)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_i32(fd, in->greatest_display_offset)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_i32(fd, in->display_start_time)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_i32(fd, in->display_end_time)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_read_sync_sample_atom(FILE *fd, MuTFFSyncSampleAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u8(fd, &out->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u24(fd, &out->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->number_of_entries)) < 0) {
    return err;
  }
  ret += err;

  // read sync sample table
  if (out->number_of_entries > MuTFF_MAX_SYNC_SAMPLE_TABLE_LEN) {
    return MuTFFErrorOutOfMemory;
  }
  const size_t table_size = out->size - 16;
  if (table_size != out->number_of_entries * 4) {
    return MuTFFErrorBadFormat;
  }
  for (size_t i = 0; i < out->number_of_entries; ++i) {
    if ((err = mutff_read_u32(fd, &out->sync_sample_table[i])) < 0) {
      return err;
    }
    ret += err;
  }

  return ret;
}

MuTFFError mutff_write_sync_sample_atom(FILE *fd,
                                        const MuTFFSyncSampleAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u8(fd, in->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u24(fd, in->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->number_of_entries)) < 0) {
    return err;
  }
  ret += err;
  if (in->number_of_entries * 4 != in->size - 16) {
    return MuTFFErrorBadFormat;
  }
  for (uint32_t i = 0; i < in->number_of_entries; ++i) {
    if ((err = mutff_write_u32(fd, in->sync_sample_table[i])) < 0) {
      return err;
    }
    ret += err;
  }
  return ret;
}

MuTFFError mutff_read_partial_sync_sample_atom(
    FILE *fd, MuTFFPartialSyncSampleAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u8(fd, &out->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u24(fd, &out->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->entry_count)) < 0) {
    return err;
  }
  ret += err;

  // read partial sync sample table
  if (out->entry_count > MuTFF_MAX_PARTIAL_SYNC_SAMPLE_TABLE_LEN) {
    return MuTFFErrorOutOfMemory;
  }
  const size_t table_size = out->size - 16;
  if (table_size != out->entry_count * 4) {
    return MuTFFErrorBadFormat;
  }
  for (size_t i = 0; i < out->entry_count; ++i) {
    if ((err = mutff_read_u32(fd, &out->partial_sync_sample_table[i])) < 0) {
      return err;
    }
    ret += err;
  }

  return ret;
}

MuTFFError mutff_write_partial_sync_sample_atom(
    FILE *fd, const MuTFFPartialSyncSampleAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u8(fd, in->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u24(fd, in->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->entry_count)) < 0) {
    return err;
  }
  ret += err;
  if (in->entry_count * 4 != in->size - 16) {
    return MuTFFErrorBadFormat;
  }
  for (uint32_t i = 0; i < in->entry_count; ++i) {
    if ((err = mutff_write_u32(fd, in->partial_sync_sample_table[i])) < 0) {
      return err;
    }
    ret += err;
  }
  return ret;
}

MuTFFError mutff_read_sample_to_chunk_table_entry(
    FILE *fd, MuTFFSampleToChunkTableEntry *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->first_chunk)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->samples_per_chunk)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->sample_description_id)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_write_sample_to_chunk_table_entry(
    FILE *fd, const MuTFFSampleToChunkTableEntry *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->first_chunk)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->samples_per_chunk)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->sample_description_id)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_read_sample_to_chunk_atom(FILE *fd,
                                           MuTFFSampleToChunkAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u8(fd, &out->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u24(fd, &out->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->number_of_entries)) < 0) {
    return err;
  }
  ret += err;

  // read table
  if (out->number_of_entries > MuTFF_MAX_SAMPLE_TO_CHUNK_TABLE_LEN) {
    return MuTFFErrorOutOfMemory;
  }
  const size_t table_size = out->size - 16;
  if (table_size != out->number_of_entries * 12) {
    return MuTFFErrorBadFormat;
  }
  for (size_t i = 0; i < out->number_of_entries; ++i) {
    if ((err = mutff_read_sample_to_chunk_table_entry(
             fd, &out->sample_to_chunk_table[i])) < 0) {
      return err;
    }
    ret += err;
  }

  return ret;
}

MuTFFError mutff_write_sample_to_chunk_atom(FILE *fd,
                                            const MuTFFSampleToChunkAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u8(fd, in->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u24(fd, in->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->number_of_entries)) < 0) {
    return err;
  }
  ret += err;
  if (in->number_of_entries * 12 != in->size - 16) {
    return MuTFFErrorBadFormat;
  }
  for (uint32_t i = 0; i < in->number_of_entries; ++i) {
    if ((err = mutff_write_sample_to_chunk_table_entry(
             fd, &in->sample_to_chunk_table[i])) < 0) {
      return err;
    }
    ret += err;
  }
  return ret;
}

MuTFFError mutff_read_sample_size_atom(FILE *fd, MuTFFSampleSizeAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u8(fd, &out->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u24(fd, &out->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->sample_size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->number_of_entries)) < 0) {
    return err;
  }
  ret += err;

  if (out->sample_size == 0) {
    // read table
    if (out->number_of_entries > MuTFF_MAX_SAMPLE_SIZE_TABLE_LEN) {
      return MuTFFErrorOutOfMemory;
    }
    const size_t table_size = out->size - 20;
    if (table_size != out->number_of_entries * 4) {
      return MuTFFErrorBadFormat;
    }
    for (size_t i = 0; i < out->number_of_entries; ++i) {
      if ((err = mutff_read_u32(fd, &out->sample_size_table[i])) < 0) {
        return err;
      }
      ret += err;
    }
  } else {
    // skip table
    fseek(fd, out->size - 20, SEEK_CUR);
    ret += out->size - 20;
  }

  return ret;
}

MuTFFError mutff_write_sample_size_atom(FILE *fd,
                                        const MuTFFSampleSizeAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u8(fd, in->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u24(fd, in->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->sample_size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->number_of_entries)) < 0) {
    return err;
  }
  ret += err;
  // @TODO: does this need a branch for in->sample_size != 0?
  //        i.e. what to do if sample_size != 0 but number_of_entries != 0
  if (in->number_of_entries * 4 != in->size - 20) {
    return MuTFFErrorBadFormat;
  }
  for (uint32_t i = 0; i < in->number_of_entries; ++i) {
    if ((err = mutff_write_u32(fd, in->sample_size_table[i])) < 0) {
      return err;
    }
    ret += err;
  }
  return ret;
}

MuTFFError mutff_read_chunk_offset_atom(FILE *fd, MuTFFChunkOffsetAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u8(fd, &out->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u24(fd, &out->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->number_of_entries)) < 0) {
    return err;
  }
  ret += err;

  // read table
  if (out->number_of_entries > MuTFF_MAX_CHUNK_OFFSET_TABLE_LEN) {
    return MuTFFErrorOutOfMemory;
  }
  const size_t table_size = out->size - 16;
  if (table_size != out->number_of_entries * 4) {
    return MuTFFErrorBadFormat;
  }
  for (size_t i = 0; i < out->number_of_entries; ++i) {
    if ((err = mutff_read_u32(fd, &out->chunk_offset_table[i])) < 0) {
      return err;
    }
    ret += err;
  }

  return ret;
}

MuTFFError mutff_write_chunk_offset_atom(FILE *fd,
                                         const MuTFFChunkOffsetAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u8(fd, in->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u24(fd, in->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->number_of_entries)) < 0) {
    return err;
  }
  ret += err;
  if (in->number_of_entries * 4 != in->size - 16) {
    return MuTFFErrorBadFormat;
  }
  for (uint32_t i = 0; i < in->number_of_entries; ++i) {
    if ((err = mutff_write_u32(fd, in->chunk_offset_table[i])) < 0) {
      return err;
    }
    ret += err;
  }
  return ret;
}

MuTFFError mutff_read_sample_dependency_flags_atom(
    FILE *fd, MuTFFSampleDependencyFlagsAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u8(fd, &out->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u24(fd, &out->flags)) < 0) {
    return err;
  }
  ret += err;

  // read table
  const size_t table_size = out->size - 12;
  if (table_size > MuTFF_MAX_SAMPLE_DEPENDENCY_FLAGS_TABLE_LEN) {
    return MuTFFErrorOutOfMemory;
  }
  for (size_t i = 0; i < table_size; ++i) {
    if ((err = mutff_read_u8(fd, &out->sample_dependency_flags_table[i])) < 0) {
      return err;
    }
    ret += err;
  }

  return ret;
}

MuTFFError mutff_write_sample_dependency_flags_atom(
    FILE *fd, const MuTFFSampleDependencyFlagsAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u8(fd, in->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u24(fd, in->flags)) < 0) {
    return err;
  }
  ret += err;
  const size_t flags_table_size = in->size - 12;
  for (uint32_t i = 0; i < flags_table_size; ++i) {
    if ((err = mutff_write_u8(fd, in->sample_dependency_flags_table[i])) < 0) {
      return err;
    }
    ret += err;
  }
  return ret;
}

MuTFFError mutff_read_sample_table_atom(FILE *fd, MuTFFSampleTableAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  MuTFFAtomHeader header;
  size_t offset;

  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;

  // read child atoms
  offset = 8;
  while (offset < out->size) {
    if ((err = mutff_peek_atom_header(fd, &header)) < 0) {
      return err;
    }
    ret += err;
    offset += header.size;
    if (offset > out->size) {
      return MuTFFErrorBadFormat;
    }

    switch (header.type) {
      /* case MuTFF_FOUR_C("stsd"): */
      case 0x73747364:
        if ((err = mutff_read_sample_description_atom(
                 fd, &out->sample_description)) < 0) {
          return err;
        }
        ret += err;
        break;
      /* case MuTFF_FOUR_C("stts"): */
      case 0x73747473:
        if ((err = mutff_read_time_to_sample_atom(fd, &out->time_to_sample)) <
            0) {
          return err;
        }
        ret += err;
        break;
      /* case MuTFF_FOUR_C("ctts"): */
      case 0x63747473:
        if ((err = mutff_read_composition_offset_atom(
                 fd, &out->composition_offset)) < 0) {
          return err;
        }
        ret += err;
        break;
      /* case MuTFF_FOUR_C("cslg"): */
      case 0x63736c67:
        if ((err = mutff_read_composition_shift_least_greatest_atom(
                 fd, &out->composition_shift_least_greatest)) < 0) {
          return err;
        }
        ret += err;
        break;
      /* case MuTFF_FOUR_C("stss"): */
      case 0x73747373:
        if ((err = mutff_read_sync_sample_atom(fd, &out->sync_sample)) < 0) {
          return err;
        }
        ret += err;
        break;
      /* case MuTFF_FOUR_C("stps"): */
      case 0x73747073:
        if ((err = mutff_read_partial_sync_sample_atom(
                 fd, &out->partial_sync_sample)) < 0) {
          return err;
        }
        ret += err;
        break;
      /* case MuTFF_FOUR_C("stsc"): */
      case 0x73747363:
        if ((err = mutff_read_sample_to_chunk_atom(fd, &out->sample_to_chunk)) <
            0) {
          return err;
        }
        ret += err;
        break;
      /* case MuTFF_FOUR_C("stsz"): */
      case 0x7374737a:
        if ((err = mutff_read_sample_size_atom(fd, &out->sample_size)) < 0) {
          return err;
        }
        ret += err;
        break;
      /* case MuTFF_FOUR_C("stco"): */
      case 0x7374636f:
        if ((err = mutff_read_chunk_offset_atom(fd, &out->chunk_offset)) < 0) {
          return err;
        }
        ret += err;
        break;
      /* case MuTFF_FOUR_C("sdtp"): */
      case 0x73647470:
        if ((err = mutff_read_sample_dependency_flags_atom(
                 fd, &out->sample_dependency_flags)) < 0) {
          return err;
        }
        ret += err;
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

  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;

  // read child atoms
  offset = 8;
  while (offset < out->size) {
    if ((err = mutff_peek_atom_header(fd, &header)) < 0) {
      return err;
    }
    ret += err;
    offset += header.size;
    if (offset > out->size) {
      return MuTFFErrorBadFormat;
    }

    switch (header.type) {
      /* case MuTFF_FOUR_C("vmhd"): */
      case 0x766d6864:
        if ((err = mutff_read_video_media_information_header_atom(
                 fd, &out->video_media_information_header)) < 0) {
          return err;
        }
        ret += err;
        break;
      /* case MuTFF_FOUR_C("hdlr"): */
      case 0x68646c72:
        if ((err = mutff_read_handler_reference_atom(
                 fd, &out->handler_reference)) < 0) {
          return err;
        }
        ret += err;
        break;
      /* case MuTFF_FOUR_C("dinf"): */
      case 0x64696e66:
        if ((err = mutff_read_data_information_atom(
                 fd, &out->data_information)) < 0) {
          return err;
        }
        ret += err;
        break;
      /* case MUTFF_FOUR_C("stbl"): */
      case 0x7374626c:
        if ((err = mutff_read_sample_table_atom(fd, &out->sample_table)) < 0) {
          return err;
        }
        ret += err;
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
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u8(fd, &out->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u24(fd, &out->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_i16(fd, &out->balance)) < 0) {
    return err;
  }
  ret += err;
  fseek(fd, 2, SEEK_CUR);
  ret += 2;
  return ret;
}

MuTFFError mutff_write_sound_media_information_header_atom(
    FILE *fd, const MuTFFSoundMediaInformationHeaderAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u8(fd, in->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u24(fd, in->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_i16(fd, in->balance)) < 0) {
    return err;
  }
  ret += err;
  for (size_t i = 0; i < 2; ++i) {
    if ((err = mutff_write_u8(fd, 0)) < 0) {
      return err;
    }
    ret += err;
  }
  return ret;
}

MuTFFError mutff_read_sound_media_information_atom(
    FILE *fd, MuTFFSoundMediaInformationAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  MuTFFAtomHeader header;
  size_t offset;

  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;

  // read child atoms
  offset = 8;
  while (offset < out->size) {
    if ((err = mutff_peek_atom_header(fd, &header)) < 0) {
      return err;
    }
    ret += err;
    offset += header.size;
    if (offset > out->size) {
      return MuTFFErrorBadFormat;
    }

    switch (header.type) {
      /* case MuTFF_FOUR_C("smhd"): */
      case 0x736d6864:
        if ((err = mutff_read_sound_media_information_header_atom(
                 fd, &out->sound_media_information_header)) < 0) {
          return err;
        }
        ret += err;
        break;
      /* case MuTFF_FOUR_C("hdlr"): */
      case 0x68646c72:
        if ((err = mutff_read_handler_reference_atom(
                 fd, &out->handler_reference)) < 0) {
          return err;
        }
        ret += err;
        break;
      /* case MuTFF_FOUR_C("dinf"): */
      case 0x64696e66:
        if ((err = mutff_read_data_information_atom(fd,
                                                    &out->data_information))) {
          return err;
        }
        break;
      /* case MUTFF_FOUR_C("stbl"): */
      case 0x7374626c:
        if ((err = mutff_read_sample_table_atom(fd, &out->sample_table)) < 0) {
          return err;
        }
        ret += err;
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
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u8(fd, &out->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u24(fd, &out->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u16(fd, &out->graphics_mode)) < 0) {
    return err;
  }
  ret += err;
  for (size_t i = 0; i < 3; ++i) {
    if ((err = mutff_read_u16(fd, &out->opcolor[i])) < 0) {
      return err;
    }
    ret += err;
  }
  if ((err = mutff_read_i16(fd, &out->balance)) < 0) {
    return err;
  }
  ret += err;
  fseek(fd, 2, SEEK_CUR);
  ret += 2;
  return ret;
}

MuTFFError mutff_write_base_media_info_atom(FILE *fd,
                                            const MuTFFBaseMediaInfoAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u8(fd, in->version)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u24(fd, in->flags)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u16(fd, in->graphics_mode)) < 0) {
    return err;
  }
  ret += err;
  for (size_t i = 0; i < 3; ++i) {
    if ((err = mutff_write_u16(fd, in->opcolor[i])) < 0) {
      return err;
    }
    ret += err;
  }
  if ((err = mutff_write_i16(fd, in->balance)) < 0) {
    return err;
  }
  ret += err;
  for (size_t i = 0; i < 2; ++i) {
    if ((err = mutff_write_u8(fd, 0)) < 0) {
      return err;
    }
    ret += err;
  }
  return ret;
}

MuTFFError mutff_read_text_media_information_atom(
    FILE *fd, MuTFFTextMediaInformationAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;
  for (size_t j = 0; j < 3; ++j) {
    for (size_t i = 0; i < 3; ++i) {
      if ((err = mutff_read_u32(fd, &out->matrix_structure[j][i])) < 0) {
        return err;
      }
      ret += err;
    }
  }
  return ret;
}

MuTFFError mutff_write_text_media_information_atom(
    FILE *fd, const MuTFFTextMediaInformationAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  for (size_t j = 0; j < 3; ++j) {
    for (size_t i = 0; i < 3; ++i) {
      if ((err = mutff_write_u32(fd, in->matrix_structure[j][i])) < 0) {
        return err;
      }
      ret += err;
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

  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;

  // read child atoms
  offset = 8;
  while (offset < out->size) {
    if ((err = mutff_peek_atom_header(fd, &header)) < 0) {
      return err;
    }
    ret += err;
    offset += header.size;
    if (offset > out->size) {
      return MuTFFErrorBadFormat;
    }

    switch (header.type) {
      /* case MuTFF_FOUR_C("gmin"): */
      case 0x676d696e:
        if ((err = mutff_read_base_media_info_atom(fd, &out->base_media_info)) <
            0) {
          return err;
        }
        ret += err;
        break;
      /* case MuTFF_FOUR_C("text"): */
      case 0x74657874:
        if ((err = mutff_read_text_media_information_atom(
                 fd, &out->text_media_information)) < 0) {
          return err;
        }
        ret += err;
        break;
      default:
        fseek(fd, header.size, SEEK_CUR);
        ret += header.size;
    }
  }

  return ret;
}

MuTFFError mutff_write_base_media_information_header_atom(
    FILE *fd, const MuTFFBaseMediaInformationHeaderAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_base_media_info_atom(fd, &in->base_media_info)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_text_media_information_atom(
           fd, &in->text_media_information)) < 0) {
    return err;
  }
  ret += err;
  return ret;
}

MuTFFError mutff_read_base_media_information_atom(
    FILE *fd, MuTFFBaseMediaInformationAtom *out) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;

  // read child atom
  if ((err = mutff_read_base_media_information_header_atom(
           fd, &out->base_media_information_header)) < 0) {
    return err;
  }
  ret += err;

  // skip remaining space
  fseek(fd, out->size - ret, SEEK_CUR);
  ret += out->size - ret;

  return ret;
}

MuTFFError mutff_write_base_media_information_atom(
    FILE *fd, const MuTFFBaseMediaInformationAtom *in) {
  MuTFFError err;
  MuTFFError ret = 0;
  if ((err = mutff_write_u32(fd, in->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_u32(fd, in->type)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_write_base_media_information_header_atom(
           fd, &in->base_media_information_header)) < 0) {
    return err;
  }
  ret += err;
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

  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;

  // read child atoms
  offset = 8;
  while (offset < out->size) {
    if ((err = mutff_peek_atom_header(fd, &header)) < 0) {
      return err;
    }
    ret += err;
    offset += header.size;
    if (offset > out->size) {
      return MuTFFErrorBadFormat;
    }

    switch (header.type) {
      /* case MuTFF_FOUR_C("mdhd"): */
      case 0x6d646864:
        if ((err = mutff_read_media_header_atom(fd, &out->media_header)) < 0) {
          return err;
        }
        ret += err;
        break;
      /* case MuTFF_FOUR_C("elng"): */
      case 0x656c6e67:
        if ((err = mutff_read_extended_language_tag_atom(
                 fd, &out->extended_language_tag)) < 0) {
          return err;
        }
        ret += err;
        break;
      /* case MuTFF_FOUR_C("hdlr"): */
      case 0x68646c72:
        if ((err = mutff_read_handler_reference_atom(
                 fd, &out->handler_reference)) < 0) {
          return err;
        }
        ret += err;
        break;
      /* case MuTFF_FOUR_C("minf"): */
      case 0x6d696e66:
        if ((err = mutff_read_media_information_atom(
                 fd, &out->media_information)) < 0) {
          return err;
        }
        ret += err;
        break;
      /* case MuTFF_FOUR_C("udta"): */
      case 0x75647461:
        if ((err = mutff_read_user_data_atom(fd, &out->user_data)) < 0) {
          return err;
        }
        ret += err;
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

  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;

  // read child atoms
  offset = 8;
  while (offset < out->size) {
    if ((err = mutff_peek_atom_header(fd, &header)) < 0) {
      return err;
    }
    ret += err;
    offset += header.size;
    if (offset > out->size) {
      return MuTFFErrorBadFormat;
    }

    switch (header.type) {
      /* case MuTFF_FOUR_C("tkhd"): */
      case 0x746b6864:
        if ((err = mutff_read_track_header_atom(fd, &out->track_header)) < 0) {
          return err;
        }
        ret += err;
        break;
      /* case MuTFF_FOUR_C("tapt"): */
      case 0x74617074:
        if ((err = mutff_read_track_aperture_mode_dimensions_atom(
                 fd, &out->track_aperture_mode_dimensions)) < 0) {
          return err;
        }
        ret += err;
        break;
      /* case MuTFF_FOUR_C("clip"): */
      case 0x636c6970:
        if ((err = mutff_read_clipping_atom(fd, &out->clipping)) < 0) {
          return err;
        }
        ret += err;
        break;
      /* case MuTFF_FOUR_C("matt"): */
      case 0x6d617474:
        if ((err = mutff_read_track_matte_atom(fd, &out->track_matte)) < 0) {
          return err;
        }
        ret += err;
        break;
      /* case MuTFF_FOUR_C("edts"): */
      case 0x65647473:
        if ((err = mutff_read_edit_atom(fd, &out->edit)) < 0) {
          return err;
        }
        ret += err;
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
        if ((err = mutff_read_track_exclude_from_autoselection_atom(
                 fd, &out->track_exclude_from_autoselection)) < 0) {
          return err;
        }
        ret += err;
        break;
      /* case MuTFF_FOUR_C("load"): */
      case 0x6c6f6164:
        if ((err = mutff_read_track_load_settings_atom(
                 fd, &out->track_load_settings)) < 0) {
          return err;
        }
        ret += err;
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
        if ((err = mutff_read_media_atom(fd, &out->media)) < 0) {
          return err;
        }
        ret += err;
        break;
      /* case MuTFF_FOUR_C("udta"): */
      case 0x75647461:
        if ((err = mutff_read_user_data_atom(fd, &out->user_data)) < 0) {
          return err;
        }
        ret += err;
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

  if ((err = mutff_read_u32(fd, &out->size)) < 0) {
    return err;
  }
  ret += err;
  if ((err = mutff_read_u32(fd, &out->type)) < 0) {
    return err;
  }
  ret += err;

  // read child atoms
  offset = 8;
  while (offset < out->size) {
    if ((err = mutff_peek_atom_header(fd, &header)) < 0) {
      return err;
    }
    offset += header.size;
    if (offset > out->size) {
      return MuTFFErrorBadFormat;
    }

    switch (header.type) {
      /* case MuTFF_FOUR_C("mvhd"): */
      case 0x6d766864:
        if (out->movie_header.size) {
          return MuTFFErrorBadFormat;
        }
        MuTFFMovieHeaderAtom movie_header;
        if ((err = mutff_read_movie_header_atom(fd, &movie_header)) < 0) {
          return err;
        }
        ret += err;
        out->movie_header = movie_header;
        break;

      /* case MuTFF_FOUR_C("clip"): */
      case 0x636c6970:
        if (out->clipping.size) {
          return MuTFFErrorBadFormat;
        }
        MuTFFClippingAtom clipping;
        if ((err = mutff_read_clipping_atom(fd, &clipping)) < 0) {
          return err;
        }
        ret += err;
        out->clipping = clipping;
        break;

      /* case MuTFF_FOUR_C("trak"): */
      case 0x7472616b:
        if (out->track_count >= MuTFF_MAX_TRACK_ATOMS) {
          return MuTFFErrorBadFormat;
        }
        MuTFFTrackAtom track;
        if ((err = mutff_read_track_atom(fd, &track)) < 0) {
          return err;
        }
        ret += err;
        out->track[out->track_count] = track;
        out->track_count++;
        break;

      /* case MuTFF_FOUR_C("udta"): */
      case 0x75647461:
        if (out->user_data.size) {
          return MuTFFErrorBadFormat;
        }
        MuTFFUserDataAtom user_data;
        if ((err = mutff_read_user_data_atom(fd, &user_data)) < 0) {
          return err;
        }
        ret += err;
        out->user_data = user_data;
        break;

      /* case MuTFF_FOUR_C("ctab"): */
      case 0x63746162:
        if (out->color_table.size) {
          return MuTFFErrorBadFormat;
        }
        MuTFFColorTableAtom color_table;
        if ((err = mutff_read_color_table_atom(fd, &color_table)) < 0) {
          return err;
        }
        ret += err;
        out->color_table = color_table;
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
  *out = (MuTFFMovieFile){0};
  rewind(fd);

  while (!(err = mutff_peek_atom_header(fd, &atom))) {
    switch (atom.type) {
      /* case MuTFF_FOUR_C("ftyp"): */
      case 0x66747970:
        if (out->file_type_compatibility_count >=
            MuTFF_MAX_FILE_TYPE_COMPATIBILITY_ATOMS) {
          return MuTFFErrorOutOfMemory;
        }
        MuTFFFileTypeCompatibilityAtom file_type_compatibility_atom;
        if ((err = mutff_read_file_type_compatibility_atom(
                 fd, &file_type_compatibility_atom)) < 0) {
          return err;
        }
        ret += err;
        ;
        out->file_type_compatibility[out->file_type_compatibility_count] =
            file_type_compatibility_atom;
        out->file_type_compatibility_count++;

        break;

      /* case MuTFF_FOUR_C("moov"): */
      case 0x6d6f6f76:
        if (out->movie_count >= MuTFF_MAX_MOVIE_ATOMS) {
          return MuTFFErrorOutOfMemory;
        }
        MuTFFMovieAtom movie_atom;
        if ((err = mutff_read_movie_atom(fd, &movie_atom)) < 0) {
          return err;
        }
        ret += err;
        out->movie[out->movie_count] = movie_atom;
        out->movie_count++;
        break;

      /* case MuTFF_FOUR_C("mdat"): */
      case 0x6d646174:
        if (out->movie_data_count >= MuTFF_MAX_MOVIE_DATA_ATOMS) {
          return MuTFFErrorOutOfMemory;
        }
        MuTFFMovieDataAtom movie_data_atom;
        if ((err = mutff_read_movie_data_atom(fd, &movie_data_atom)) < 0) {
          return err;
        }
        ret += err;
        out->movie_data[out->movie_count] = movie_data_atom;
        out->movie_data_count++;
        break;

      /* case MuTFF_FOUR_C("free"): */
      case 0x66726565:
        if (out->free_count >= MuTFF_MAX_FREE_ATOMS) {
          return MuTFFErrorOutOfMemory;
        }
        MuTFFFreeAtom free_atom;
        if ((err = mutff_read_free_atom(fd, &free_atom)) < 0) {
          return err;
        }
        ret += err;
        out->free[out->movie_count] = free_atom;
        out->free_count++;
        break;

      /* case MuTFF_FOUR_C("skip"): */
      case 0x736b6970:
        if (out->skip_count >= MuTFF_MAX_SKIP_ATOMS) {
          return MuTFFErrorOutOfMemory;
        }
        MuTFFSkipAtom skip_atom;
        if ((err = mutff_read_skip_atom(fd, &skip_atom)) < 0) {
          return err;
        }
        ret += err;
        out->skip[out->movie_count] = skip_atom;
        out->skip_count++;
        break;

      /* case MuTFF_FOUR_C("wide"): */
      case 0x77696465:
        if (out->wide_count >= MuTFF_MAX_WIDE_ATOMS) {
          return MuTFFErrorOutOfMemory;
        }
        MuTFFWideAtom wide_atom;
        if ((err = mutff_read_wide_atom(fd, &wide_atom)) < 0) {
          return err;
        }
        ret += err;
        out->wide[out->movie_count] = wide_atom;
        out->wide_count++;
        break;

      /* case MuTFF_FOUR_C("pnot"): */
      case 0x706e6f74:
        if (out->preview_count >= MuTFF_MAX_WIDE_ATOMS) {
          return MuTFFErrorOutOfMemory;
        }
        MuTFFPreviewAtom preview_atom;
        if ((err = mutff_read_preview_atom(fd, &preview_atom)) < 0) {
          return err;
        }
        ret += err;
        out->preview[out->preview_count] = preview_atom;
        out->preview_count++;
        break;

      default:
        // unsupported basic type - skip as per spec
        fseek(fd, atom.size, SEEK_CUR);
        ret += atom.size;
    }
  }
  if (err != MuTFFErrorEOF) {
    return err;
  }

  return ret;
}

/* MuTFFError mutff_write_movie_file(FILE *fd, MuTFFMovieFile *in) { */
/*   mutff_write_file_type_compatibility(fd, &in->file_type_compatibility); */
/*   mutff_write_movie(fd, &in->movie); */
/* } */
