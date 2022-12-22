///
/// @file      mutff.c
/// @author    Frank Plowman <post@frankplowman.com>
/// @brief     MuTFF QuickTime file format library main header file
/// @copyright 2022 Frank Plowman
/// @license   This project is released under the GNU Public License Version 3.
///            For the terms of this license, see [LICENSE.md](LICENSE.md)
///

#ifndef MUTFF_H_
#define MUTFF_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/// @addtogroup MuTFF
/// @{

#define MuTFF_ATOM_ID(str) \
  ((str[0] << 24) + (str[1] << 16) + (str[2] << 8) + str[3])

///
/// @brief A generic error in the MuTFF library.
///
typedef enum {
  MuTFFErrorNone = 0,
  MuTFFErrorEOF,
  MuTFFErrorIOError,
  MuTFFErrorAtomTooLong,
  MuTFFErrorNotBasicAtomType,
  MuTFFErrorTooManyAtoms,
  MuTFFErrorBadFormat,
} MuTFFError;

typedef uint32_t MuTFFAtomSize;

///
/// @brief The type of an atom
///
typedef char MuTFFAtomType[4];

///
/// @brief Read the type of an atom
///
/// The current file offset must be at the start of the type ID.
///
/// @param [in] fd    The file descriptor
/// @param [out] out  Output
/// @return           Whether or not the type was read successfully
///
MuTFFError mutff_read_atom_type(FILE *fd, MuTFFAtomType *out);

// Macintosh date format time
// Time passed in seconds since 1904-01-01T00:00:00
typedef uint32_t MacTime;

// Quicktime movie time
// Time since start of movie in time units (1/time_scale)s
typedef uint32_t QTTime;
//
// Quicktime movie duration
// Measured in time units (1/time_scale)s
typedef uint32_t QTDuration;

// @TODO: assert sizeof(QTMatrix) = 36
typedef float QTMatrix[9];

typedef uint32_t QTTrackID;

typedef struct {
  size_t offset;
  MuTFFAtomSize size;
  MuTFFAtomType type;
} MuTFFAtomHeader;

///
/// @brief Read the header of an atom
///
/// The current file offset must be at the start of the atom.
///
/// @param [in] fd    The file descriptor
/// @param [out] out  Output
/// @return           Whether or not an atom was read successfully
///
MuTFFError mutff_peek_atom_header(FILE *fd, MuTFFAtomHeader *out);

///
/// @brief The maximum number of compatible brands .
///
#define MuTFF_MAX_COMPATIBLE_BRANDS 4

///
/// @brief QuickTime identifier for a file format.
///
typedef char QTFileFormat[4];

///
/// @brief Read the header of an atom
///
/// The current file offset must be at the start of the atom.
///
/// @param [in] fd    The file descriptor
/// @param [out] out  Output
/// @return           Whether or not an atom was read successfully
///
MuTFFError read_file_format(FILE *fd, QTFileFormat *out);

///
/// @brief File type compatibility atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/MuTFF/MuTFFChap1/mutff1.html#//apple_ref/doc/uid/TP40000939-CH203-CJBCBIFF
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
  uint32_t major_brand;
  uint32_t minor_version;
  size_t compatible_brands_count;
  QTFileFormat compatible_brands[MuTFF_MAX_COMPATIBLE_BRANDS];
} MuTFFFileTypeCompatibilityAtom;

///
/// @brief Read a file type compatibility atom
///
/// @param [in] fd    The file descriptor
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_file_type_compatibility_atom(
    FILE *fd, MuTFFFileTypeCompatibilityAtom *out);

typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
} MuTFFMovieDataAtom;

///
/// @brief Read a movie data atom
///
/// @param [in] fd    The file descriptor
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_movie_data_atom(FILE *fd, MuTFFMovieDataAtom *out);

///
/// @brief Free (unused) space atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/MuTFF/MuTFFChap1/mutff1.html#//apple_ref/doc/uid/TP40000939-CH203-55464
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
} MuTFFFreeAtom;

///
/// @brief Read a free atom
///
/// @param [in] fd    The file descriptor
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_free_atom(FILE *fd, MuTFFFreeAtom *out);

///
/// @brief Skip (unused) space atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/MuTFF/MuTFFChap1/mutff1.html#//apple_ref/doc/uid/TP40000939-CH203-55464
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
} MuTFFSkipAtom;

///
/// @brief Read a skip atom
///
/// @param [in] fd    The file descriptor
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_skip_atom(FILE *fd, MuTFFSkipAtom *out);

///
/// @brief Wide (reserved) space atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/MuTFF/MuTFFChap1/mutff1.html#//apple_ref/doc/uid/TP40000939-CH203-55464
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
} MuTFFWideAtom;

///
/// @brief Read a wide atom
///
/// @param [in] fd    The file descriptor
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_wide_atom(FILE *fd, MuTFFWideAtom *out);

///
/// @brief Preview atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/MuTFF/MuTFFChap1/mutff1.html#//apple_ref/doc/uid/TP40000939-CH203-38240
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
  MacTime modification_time;
  uint16_t version;
  MuTFFAtomType atom_type;
  uint16_t atom_index;
} MuTFFPreviewAtom;

///
/// @brief Read a preview atom
///
/// @param [in] fd    The file descriptor
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_preview_atom(FILE *fd, MuTFFPreviewAtom *out);

///
/// @brief Movie header atom.
///
// @TODO: assert sizeof(MovieHeaderAtom) == 108
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
  char version;
  char flags[3];
  MacTime creation_time;
  MacTime modification_time;
  uint32_t time_scale;
  QTDuration duration;
  uint32_t preferred_rate;
  uint16_t preferred_volume;
  char _reserved[10];
  QTMatrix matrix_structure;
  QTTime preview_time;
  QTDuration preview_duration;
  QTTime poster_time;
  QTTime selection_time;
  QTDuration selection_duration;
  QTTime current_time;
  QTTrackID next_track_id;
} MuTFFMovieHeaderAtom;

///
/// @brief Read a movie header atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_movie_header_atom(FILE *fd, MuTFFMovieHeaderAtom *out);

///
/// @brief Clipping region atom
///
// @TODO: clipping data field (variable size)
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
  uint16_t region_size;
  uint64_t region_boundary_box;
} MuTFFClippingRegionAtom;

///
/// @brief Read a clipping region atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_clipping_region_atom(FILE *fd,
                                           MuTFFClippingRegionAtom *out);

///
/// @brief Clipping atom
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
  MuTFFClippingRegionAtom clipping_region;
} MuTFFClippingAtom;

///
/// @brief Read a clipping atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_clipping_atom(FILE *fd, MuTFFClippingAtom *out);

///
/// @brief The maximum number of entries in the color table
///
#define MuTFF_MAX_COLOR_TABLE_SIZE 16

///
/// @brief Color table atom
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
  uint32_t color_table_seed;
  uint16_t color_table_flags;
  uint16_t color_table_size;
  uint16_t color_array[MuTFF_MAX_COLOR_TABLE_SIZE][4];
} MuTFFColorTableAtom;

///
/// @brief Read a color table atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_color_table_atom(FILE *fd, MuTFFColorTableAtom *out);

///
/// @brief The maximum number of entries in the user data list
///
#define MuTFF_MAX_USER_DATA_ITEMS 16

///
/// @brief User data atom
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
  MuTFFAtomHeader user_data_list[MuTFF_MAX_USER_DATA_ITEMS];
} MuTFFUserDataAtom;

///
/// @brief Read a user data atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_user_data_atom(FILE *fd, MuTFFUserDataAtom *out);

typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
  /* MuTFFTrackHeaderAtom track_header; */
  /* MuTFFTrackApertureModeDimensionsAtom track_aperture_mode_dimensions; */
  /* MuTFFClippingAtom clipping; */
  /* MuTFFTrackMatteAtom track_matte; */
  /* MuTFFEditAtom edit; */
  /* MuTFFTrackReferenceAtom track_reference; */
  /* MuTFFTrackExcludeFromAutoselectionAtom track_exclude_from_autoselection; */
  /* MuTFFTrackLoadSettingsAtom track_load_settings; */
  /* MuTFFMediaAtom media; */
  /* MuTFFUserDataAtom user_data; */
} MuTFFTrackAtom;

///
/// @brief Read a track atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_track_atom(FILE *fd, MuTFFTrackAtom *out);

///
/// @brief The maximum number of track atoms in a movie atom
///
#define MuTFF_MAX_TRACK_ATOMS 4

///
/// @brief Movie atom.
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
  MuTFFMovieHeaderAtom movie_header;
  MuTFFClippingAtom clipping;
  MuTFFColorTableAtom color_table;
  MuTFFUserDataAtom user_data;
  size_t track_count;
  MuTFFTrackAtom track[MuTFF_MAX_TRACK_ATOMS];
} MuTFFMovieAtom;

///
/// @brief Read a movie atom
///
/// @param [in] fd    The file descriptor
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_movie_atom(FILE *fd, MuTFFMovieAtom *out);

#define MuTFF_MAX_FILE_TYPE_COMPATIBILITY_ATOMS 1
#define MuTFF_MAX_MOVIE_ATOMS 1
#define MuTFF_MAX_MOVIE_DATA_ATOMS 4
#define MuTFF_MAX_FREE_ATOMS 4
#define MuTFF_MAX_SKIP_ATOMS 4
#define MuTFF_MAX_WIDE_ATOMS 4
#define MuTFF_MAX_PREVIEW_ATOMS 1

///
/// @brief A QuickTime movie file
///
typedef struct {
  size_t file_type_compatibility_count;
  MuTFFFileTypeCompatibilityAtom
      file_type_compatibility[MuTFF_MAX_FILE_TYPE_COMPATIBILITY_ATOMS];

  size_t movie_count;
  MuTFFMovieAtom movie[MuTFF_MAX_MOVIE_ATOMS];

  size_t movie_data_count;
  MuTFFMovieDataAtom movie_data[MuTFF_MAX_MOVIE_DATA_ATOMS];

  size_t free_count;
  MuTFFFreeAtom free[MuTFF_MAX_FREE_ATOMS];

  size_t skip_count;
  MuTFFSkipAtom skip[MuTFF_MAX_SKIP_ATOMS];

  size_t wide_count;
  MuTFFWideAtom wide[MuTFF_MAX_WIDE_ATOMS];

  size_t preview_count;
  MuTFFPreviewAtom preview[MuTFF_MAX_PREVIEW_ATOMS];
} MuTFFMovieFile;

///
/// @brief Read a QuickTime movie file
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed file
/// @return           Whether or not the file was read successfully
///
MuTFFError mutff_read_movie_file(FILE *fd, MuTFFMovieFile *out);

///
/// @brief Write a QuickTime movie file
///
/// @param [in] fd    The file descriptor to write to
/// @param [in] in    The file
/// @return           Whether or not the file was written successfully
///
MuTFFError mutff_write_movie_file(FILE *fd, MuTFFMovieFile *in);

/// @} MuTFF

#endif  // MUTFF_H_
