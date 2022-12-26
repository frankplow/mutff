///
/// @file      mutff.c
/// @author    Frank Plowman <post@frankplowman.com>
/// @brief     MuTFF QuickTime file format library main source file
/// @copyright 2022 Frank Plowman
/// @license   This project is released under the GNU Public License Version 3.
///            For the terms of this license, see [LICENSE.md](LICENSE.md)
///

#include <stdio.h>

#include "mutff.h"

///
/// @brief Read data from a file, optionally converting from network order to
/// host order
///
/// @param [in]  fd   The file descriptor to read from
/// @param [out] dest Where to write the output data
/// @param [in]  n    The number of bytes to read
/// @param [in]  conv Whether to convert from network order to host order
/// @return           Whether the data was read successfully
///
static MuTFFError mutff_read(FILE *fd, void *dest, size_t n) {
  const size_t read_bytes = fread(dest, n, 1, fd);
  if (read_bytes != n) {
    if (ferror(fd)) {
      return MuTFFErrorIOError;
    }
    if (feof(fd)) {
      return MuTFFErrorEOF;
    }
    // @TODO: Should an unknown error be thrown here?
  }
  return MuTFFErrorNone;
}

// Convert a number from network (big) endian to host endian.
// These must be implemented here as newlib does not provide the
// standard library implementations ntohs & ntohl (arpa/inet.h).
//
// taken from:
// https://stackoverflow.com/questions/2100331/macro-definition-to-determine-big-endian-or-little-endian-machine
static uint16_t mutff_ntoh_16(uint16_t n) {
  unsigned char *np = (unsigned char *)&n;

  return ((uint32_t)np[0] << 8) | (uint32_t)np[1];
}

static uint32_t mutff_ntoh_24(uint32_t n) {
  unsigned char *np = (unsigned char *)&n;

  return ((uint32_t)np[0] << 16) | ((uint32_t)np[1] << 8) | (uint32_t)np[2];
}

static uint32_t mutff_ntoh_32(uint32_t n) {
  unsigned char *np = (unsigned char *)&n;

  return ((uint32_t)np[0] << 24) | ((uint32_t)np[1] << 16) |
         ((uint32_t)np[2] << 8) | (uint32_t)np[3];
}

static uint64_t mutff_ntoh_64(uint64_t n) {
  unsigned char *np = (unsigned char *)&n;

  return ((uint64_t)np[0] << 56) | ((uint64_t)np[1] << 48) |
         ((uint64_t)np[2] << 40) | ((uint64_t)np[3] << 32) |
         ((uint64_t)np[4] << 24) | ((uint64_t)np[5] << 16) |
         ((uint64_t)np[6] << 8) | (uint64_t)np[7];
}

MuTFFError mutff_read_atom_type(FILE *fd, MuTFFAtomType *out) {
  const size_t read_bytes = fread(out, 4, 1, fd);
  if (ferror(fd)) {
    return MuTFFErrorIOError;
  }
  if (feof(fd)) {
    return MuTFFErrorEOF;
  }
  return MuTFFErrorNone;
}

MuTFFError mutff_read_atom_version_flags(FILE *fd, MuTFFAtomVersionFlags *out) {
  const size_t read_bytes = fread(out, 4, 1, fd);
  if (ferror(fd)) {
    return MuTFFErrorIOError;
  }
  if (feof(fd)) {
    return MuTFFErrorEOF;
  }
  out->flags = mutff_ntoh_24(out->flags);
  return MuTFFErrorNone;
}

MuTFFError mutff_peek_atom_header(FILE *fd, MuTFFAtomHeader *out) {
  MuTFFError err;

  if ((err = mutff_read(fd, &out->size, 4))) {
    return err;
  }
  out->size = mutff_ntoh_32(out->size);
  if ((err = mutff_read_atom_type(fd, &out->type))) {
    return err;
  }
  fseek(fd, -8, SEEK_CUR);

  return MuTFFErrorNone;
}

MuTFFError mutff_read_file_format(FILE *fd, QTFileFormat *out) {
  const size_t read_bytes = fread(out, 4, 1, fd);
  if (ferror(fd)) {
    return MuTFFErrorIOError;
  }
  if (feof(fd)) {
    return MuTFFErrorEOF;
  }
  return MuTFFErrorNone;
}

MuTFFError mutff_read_quickdraw_rect(FILE *fd, MuTFFQuickDrawRect *out) {
  const size_t read_bytes = fread(out, 8, 1, fd);
  if (read_bytes != 8) {
    if (ferror(fd)) {
      return MuTFFErrorIOError;
    }
    if (feof(fd)) {
      return MuTFFErrorEOF;
    }
  }
  out->top = mutff_ntoh_16(out->top);
  out->left = mutff_ntoh_16(out->left);
  out->bottom = mutff_ntoh_16(out->bottom);
  out->right = mutff_ntoh_16(out->right);
  return MuTFFErrorNone;
}

MuTFFError mutff_read_quickdraw_region(FILE *fd, MuTFFQuickDrawRegion *out) {
  const size_t read_bytes = fread(out, 2, 1, fd);
  if (read_bytes != 2) {
    if (ferror(fd)) {
      return MuTFFErrorIOError;
    }
    if (feof(fd)) {
      return MuTFFErrorEOF;
    }
  }
  out->size = mutff_ntoh_16(out->size);
  mutff_read_quickdraw_rect(fd, &out->rect);
  fseek(fd, out->size - 10, SEEK_CUR);
  return MuTFFErrorNone;
}

MuTFFError mutff_read_file_type_compatibility_atom(
    FILE *fd, MuTFFFileTypeCompatibilityAtom *out) {
  MuTFFError err;

  if ((err = mutff_read(fd, &out->size, 4))) {
    return err;
  }
  out->size = mutff_ntoh_32(out->size);
  if ((err = mutff_read_atom_type(fd, &out->type))) {
    return err;
  }
  if ((err = mutff_read(fd, &out->major_brand, 4))) {
    return err;
  }
  out->major_brand = mutff_ntoh_32(out->major_brand);
  if ((err = mutff_read(fd, &out->minor_version, 4))) {
    return err;
  }

  // read compatible brands
  const size_t compatible_brands_length = (out->size - 16);
  if (compatible_brands_length % 4 != 0) {
    return MuTFFErrorBadFormat;
  }
  out->compatible_brands_count = compatible_brands_length / 4;
  if (out->compatible_brands_count > MuTFF_MAX_COMPATIBLE_BRANDS) {
    return MuTFFErrorTooManyAtoms;
  }
  fread(out->compatible_brands, 4, out->compatible_brands_count, fd);
  if (ferror(fd)) {
    return MuTFFErrorIOError;
  }
  if (feof(fd)) {
    return MuTFFErrorEOF;
  }

  return MuTFFErrorNone;
}

MuTFFError mutff_read_movie_data_atom(FILE *fd, MuTFFMovieDataAtom *out) {
  MuTFFError err;

  if ((err = mutff_read(fd, &out->size, 4))) {
    return err;
  }
  out->size = mutff_ntoh_32(out->size);
  if ((err = mutff_read_atom_type(fd, &out->type))) {
    return err;
  }
  fseek(fd, out->size - 8, SEEK_CUR);

  return MuTFFErrorNone;
}

MuTFFError mutff_read_free_atom(FILE *fd, MuTFFFreeAtom *out) {
  MuTFFError err;

  if ((err = mutff_read(fd, &out->size, 4))) {
    return err;
  }
  out->size = mutff_ntoh_32(out->size);
  if ((err = mutff_read_atom_type(fd, &out->type))) {
    return err;
  }
  fseek(fd, out->size - 8, SEEK_CUR);

  return MuTFFErrorNone;
}

MuTFFError mutff_read_skip_atom(FILE *fd, MuTFFSkipAtom *out) {
  MuTFFError err;

  if ((err = mutff_read(fd, &out->size, 4))) {
    return err;
  }
  out->size = mutff_ntoh_32(out->size);
  if ((err = mutff_read_atom_type(fd, &out->type))) {
    return err;
  }
  fseek(fd, out->size - 8, SEEK_CUR);

  return MuTFFErrorNone;
}

MuTFFError mutff_read_wide_atom(FILE *fd, MuTFFWideAtom *out) {
  MuTFFError err;

  if ((err = mutff_read(fd, &out->size, 4))) {
    return err;
  }
  out->size = mutff_ntoh_32(out->size);
  if ((err = mutff_read_atom_type(fd, &out->type))) {
    return err;
  }
  fseek(fd, out->size - 8, SEEK_CUR);

  return MuTFFErrorNone;
}

MuTFFError mutff_read_preview_atom(FILE *fd, MuTFFPreviewAtom *out) {
  MuTFFError err;

  if ((err = mutff_read(fd, &out->size, 4))) {
    return err;
  }
  out->size = mutff_ntoh_32(out->size);
  if ((err = mutff_read_atom_type(fd, &out->type))) {
    return err;
  }
  if ((err = mutff_read(fd, &out->modification_time, 4))) {
    return err;
  }
  out->modification_time = mutff_ntoh_32(out->modification_time);
  if ((err = mutff_read(fd, &out->version, 2))) {
    return err;
  }
  out->version = mutff_ntoh_16(out->version);
  if ((err = mutff_read(fd, &out->atom_type, 4))) {
    return err;
  }
  if ((err = mutff_read(fd, &out->atom_index, 2))) {
    return err;
  }
  out->atom_index = mutff_ntoh_16(out->atom_index);

  return MuTFFErrorNone;
}

MuTFFError mutff_read_movie_header_atom(FILE *fd, MuTFFMovieHeaderAtom *out) {
  MuTFFError err;

  if ((err = mutff_read(fd, &out->size, 4))) {
    return err;
  }
  out->size = mutff_ntoh_32(out->size);
  if ((err = mutff_read_atom_type(fd, &out->type))) {
    return err;
  }
  if ((err = mutff_read_atom_version_flags(fd, &out->version_flags))) {
    return err;
  }
  if ((err = mutff_read(fd, &out->creation_time, 4))) {
    return err;
  }
  out->creation_time = mutff_ntoh_32(out->creation_time);
  if ((err = mutff_read(fd, &out->modification_time, 4))) {
    return err;
  }
  out->modification_time = mutff_ntoh_32(out->modification_time);
  if ((err = mutff_read(fd, &out->time_scale, 4))) {
    return err;
  }
  out->time_scale = mutff_ntoh_32(out->time_scale);
  if ((err = mutff_read(fd, &out->duration, 4))) {
    return err;
  }
  out->duration = mutff_ntoh_32(out->duration);
  if ((err = mutff_read(fd, &out->preferred_rate, 4))) {
    return err;
  }
  out->preferred_rate = mutff_ntoh_32(out->preferred_rate);
  if ((err = mutff_read(fd, &out->preferred_volume, 2))) {
    return err;
  }
  out->preferred_volume = mutff_ntoh_16(out->preferred_volume);
  if ((err = mutff_read(fd, &out->_reserved, 10))) {
    return err;
  }
  for (size_t j = 0; j < 3; ++j) {
    for (size_t i = 0; i < 3; ++i) {
      if ((err = mutff_read(fd, &out->matrix_structure[j][i], 4))) {
        return err;
      }
      out->matrix_structure[j][i] = mutff_ntoh_32(out->matrix_structure[j][i]);
    }
  }
  if ((err = mutff_read(fd, &out->preview_time, 4))) {
    return err;
  }
  out->preview_time = mutff_ntoh_32(out->preview_time);
  if ((err = mutff_read(fd, &out->preview_duration, 4))) {
    return err;
  }
  out->preview_duration = mutff_ntoh_32(out->preview_duration);
  if ((err = mutff_read(fd, &out->poster_time, 4))) {
    return err;
  }
  out->poster_time = mutff_ntoh_32(out->poster_time);
  if ((err = mutff_read(fd, &out->selection_time, 4))) {
    return err;
  }
  out->selection_time = mutff_ntoh_32(out->selection_time);
  if ((err = mutff_read(fd, &out->selection_duration, 4))) {
    return err;
  }
  out->selection_duration = mutff_ntoh_32(out->selection_duration);
  if ((err = mutff_read(fd, &out->current_time, 4))) {
    return err;
  }
  out->current_time = mutff_ntoh_32(out->current_time);
  if ((err = mutff_read(fd, &out->next_track_id, 4))) {
    return err;
  }
  out->next_track_id = mutff_ntoh_32(out->next_track_id);

  return MuTFFErrorNone;
}

MuTFFError mutff_read_clipping_region_atom(FILE *fd,
                                           MuTFFClippingRegionAtom *out) {
  MuTFFError err;

  if ((err = mutff_read(fd, &out->size, 4))) {
    return err;
  }
  out->size = mutff_ntoh_32(out->size);
  if ((err = mutff_read_atom_type(fd, &out->type))) {
    return err;
  }
  if ((err = mutff_read_quickdraw_region(fd, &out->region))) {
    return err;
  }

  return MuTFFErrorNone;
}

MuTFFError mutff_read_clipping_atom(FILE *fd, MuTFFClippingAtom *out) {
  MuTFFError err;

  if ((err = mutff_read(fd, &out->size, 4))) {
    return err;
  }
  out->size = mutff_ntoh_32(out->size);
  if ((err = mutff_read_atom_type(fd, &out->type))) {
    return err;
  }
  if ((err = mutff_read_clipping_region_atom(fd, &out->clipping_region))) {
    return err;
  }

  return MuTFFErrorNone;
}

MuTFFError mutff_read_color_table_atom(FILE *fd, MuTFFColorTableAtom *out) {
  MuTFFError err;

  if ((err = mutff_read(fd, &out->size, 4))) {
    return err;
  }
  out->size = mutff_ntoh_32(out->size);
  if ((err = mutff_read_atom_type(fd, &out->type))) {
    return err;
  }
  if ((err = mutff_read(fd, &out->color_table_seed, 4))) {
    return err;
  }
  out->color_table_seed = mutff_ntoh_32(out->color_table_seed);
  if ((err = mutff_read(fd, &out->color_table_flags, 2))) {
    return err;
  }
  out->color_table_flags = mutff_ntoh_16(out->color_table_flags);
  if ((err = mutff_read(fd, &out->color_table_size, 2))) {
    return err;
  }
  out->color_table_size = mutff_ntoh_16(out->color_table_size);
  const size_t size = (out->color_table_size + 1) * 8;
  if (size != out->size - 16) {
    return MuTFFErrorBadFormat;
  }
  if ((err = mutff_read(fd, out->color_array, size))) {
    return err;
  }
  for (size_t i = 0; i <= out->color_table_size; ++i) {
    out->color_array[i][0] = mutff_ntoh_16(out->color_array[i][0]);
    out->color_array[i][1] = mutff_ntoh_16(out->color_array[i][1]);
    out->color_array[i][2] = mutff_ntoh_16(out->color_array[i][2]);
    out->color_array[i][3] = mutff_ntoh_16(out->color_array[i][3]);
  }

  return MuTFFErrorNone;
}

MuTFFError mutff_read_user_data_atom(FILE *fd, MuTFFUserDataAtom *out) {
  MuTFFError err;

  if ((err = mutff_read(fd, &out->size, 4))) {
    return err;
  }
  out->size = mutff_ntoh_32(out->size);
  if ((err = mutff_read_atom_type(fd, &out->type))) {
    return err;
  }
  size_t i = 0;
  size_t offset = 8;
  while (offset < out->size) {
    if (i > MuTFF_MAX_USER_DATA_ITEMS) {
      return MuTFFErrorTooManyAtoms;
    }
    if ((err = mutff_read(fd, &out->user_data_list[i].size, 4))) {
      return err;
    }
    out->user_data_list[i].size = mutff_ntoh_32(out->user_data_list[i].size);
    if ((err = mutff_read_atom_type(fd, &out->user_data_list[i].type))) {
      return err;
    }
    fseek(fd, out->user_data_list[i].size - 8, SEEK_CUR);
    offset += out->user_data_list[i].size;
    i++;
  }

  return MuTFFErrorNone;
}

MuTFFError mutff_read_track_atom(FILE *fd, MuTFFTrackAtom *out) {
  MuTFFError err;

  if ((err = mutff_read(fd, &out->size, 4))) {
    return err;
  }
  out->size = mutff_ntoh_32(out->size);
  if ((err = mutff_read_atom_type(fd, &out->type))) {
    return err;
  }
  fseek(fd, out->size - 8, SEEK_CUR);

  return MuTFFErrorNone;
}

MuTFFError mutff_read_movie_atom(FILE *fd, MuTFFMovieAtom *out) {
  MuTFFError err;
  MuTFFAtomHeader atom;
  *out = (MuTFFMovieAtom){0};

  // read header
  if ((err = mutff_read(fd, &out->size, 4))) {
    return err;
  }
  out->size = mutff_ntoh_32(out->size);
  if ((err = mutff_read_atom_type(fd, &out->type))) {
    return err;
  }

  // read child atoms
  while (!(err = mutff_peek_atom_header(fd, &atom))) {
    switch (MuTFF_FOUR_C(atom.type)) {
      /* case MuTFF_FOUR_C("mvhd"): */
      case 0x6d766864:
        if (out->movie_header.size) {
          return MuTFFErrorTooManyAtoms;
        }
        MuTFFMovieHeaderAtom movie_header;
        if ((err = mutff_read_movie_header_atom(fd, &movie_header))) {
          return err;
        }
        out->movie_header = movie_header;
        break;

      /* case MuTFF_FOUR_C("clip"): */
      case 0x636c6970:
        if (out->clipping.size) {
          return MuTFFErrorTooManyAtoms;
        }
        MuTFFClippingAtom clipping;
        if ((err = mutff_read_clipping_atom(fd, &clipping))) {
          return err;
        }
        out->clipping = clipping;
        break;

      /* case MuTFF_FOUR_C("trak"): */
      case 0x7472616b:
        if (out->track_count >= MuTFF_MAX_TRACK_ATOMS) {
          return MuTFFErrorTooManyAtoms;
        }
        MuTFFTrackAtom track;
        if ((err = mutff_read_track_atom(fd, &track))) {
          return err;
        }
        out->track[out->track_count] = track;
        out->track_count++;
        break;

      /* case MuTFF_FOUR_C("udta"): */
      case 0x75647461:
        if (out->user_data.size) {
          return MuTFFErrorTooManyAtoms;
        }
        MuTFFUserDataAtom user_data;
        if ((err = mutff_read_user_data_atom(fd, &user_data))) {
          return err;
        }
        out->user_data = user_data;
        break;

      /* case MuTFF_FOUR_C("ctab"): */
      case 0x63746162:
        if (out->color_table.size) {
          return MuTFFErrorTooManyAtoms;
        }
        MuTFFColorTableAtom color_table;
        if ((err = mutff_read_color_table_atom(fd, &color_table))) {
          return err;
        }
        out->color_table = color_table;
        break;

      default:
        // unrecognised atom type - skip as per spec
        fseek(fd, atom.size, SEEK_CUR);
        break;
    }
  }
  if (err != MuTFFErrorEOF) {
    return err;
  }

  return MuTFFErrorNone;
}

MuTFFError mutff_read_movie_file(FILE *fd, MuTFFMovieFile *out) {
  MuTFFError err;
  MuTFFAtomHeader atom;
  *out = (MuTFFMovieFile){0};
  rewind(fd);

  while (!(err = mutff_peek_atom_header(fd, &atom))) {
    switch (MuTFF_FOUR_C(atom.type)) {
      /* case MuTFF_FOUR_C("ftyp"): */
      case 0x66747970:
        if (out->file_type_compatibility_count >=
            MuTFF_MAX_FILE_TYPE_COMPATIBILITY_ATOMS) {
          return MuTFFErrorTooManyAtoms;
        }
        MuTFFFileTypeCompatibilityAtom file_type_compatibility_atom;
        if ((err = mutff_read_file_type_compatibility_atom(
                 fd, &file_type_compatibility_atom))) {
          return err;
        };
        out->file_type_compatibility[out->file_type_compatibility_count] =
            file_type_compatibility_atom;
        out->file_type_compatibility_count++;

        break;

      /* case MuTFF_FOUR_C("moov"): */
      case 0x6d6f6f76:
        if (out->movie_count >= MuTFF_MAX_MOVIE_ATOMS) {
          return MuTFFErrorTooManyAtoms;
        }
        MuTFFMovieAtom movie_atom;
        if ((err = mutff_read_movie_atom(fd, &movie_atom))) {
          return err;
        }
        out->movie[out->movie_count] = movie_atom;
        out->movie_count++;
        break;

      /* case MuTFF_FOUR_C("mdat"): */
      case 0x6d646174:
        if (out->movie_data_count >= MuTFF_MAX_MOVIE_DATA_ATOMS) {
          return MuTFFErrorTooManyAtoms;
        }
        MuTFFMovieDataAtom movie_data_atom;
        if ((err = mutff_read_movie_data_atom(fd, &movie_data_atom))) {
          return err;
        }
        out->movie_data[out->movie_count] = movie_data_atom;
        out->movie_data_count++;
        break;

      /* case MuTFF_FOUR_C("free"): */
      case 0x66726565:
        if (out->free_count >= MuTFF_MAX_FREE_ATOMS) {
          return MuTFFErrorTooManyAtoms;
        }
        MuTFFFreeAtom free_atom;
        if ((err = mutff_read_free_atom(fd, &free_atom))) {
          return err;
        }
        out->free[out->movie_count] = free_atom;
        out->free_count++;
        break;

      /* case MuTFF_FOUR_C("skip"): */
      case 0x736b6970:
        if (out->skip_count >= MuTFF_MAX_SKIP_ATOMS) {
          return MuTFFErrorTooManyAtoms;
        }
        MuTFFSkipAtom skip_atom;
        if ((err = mutff_read_skip_atom(fd, &skip_atom))) {
          return err;
        }
        out->skip[out->movie_count] = skip_atom;
        out->skip_count++;
        break;

      /* case MuTFF_FOUR_C("wide"): */
      case 0x77696465:
        if (out->wide_count >= MuTFF_MAX_WIDE_ATOMS) {
          return MuTFFErrorTooManyAtoms;
        }
        MuTFFWideAtom wide_atom;
        if ((err = mutff_read_wide_atom(fd, &wide_atom))) {
          return err;
        }
        out->wide[out->movie_count] = wide_atom;
        out->wide_count++;
        break;

      /* case MuTFF_FOUR_C("pnot"): */
      case 0x706e6f74:
        if (out->preview_count >= MuTFF_MAX_WIDE_ATOMS) {
          return MuTFFErrorTooManyAtoms;
        }
        MuTFFPreviewAtom preview_atom;
        if ((err = mutff_read_preview_atom(fd, &preview_atom))) {
          return err;
        }
        out->preview[out->preview_count] = preview_atom;
        out->preview_count++;
        break;

      default:
        // unsupported basic type - skip as per spec
        fseek(fd, atom.size, SEEK_CUR);
        break;
    }
  }
  if (err != MuTFFErrorEOF) {
    return err;
  }

  return MuTFFErrorNone;
}

/* MuTFFError mutff_write_movie_file(FILE *fd, MuTFFMovieFile *in) { */
/*   mutff_write_file_type_compatibility(fd, &in->file_type_compatibility); */
/*   mutff_write_movie(fd, &in->movie); */
/* } */
