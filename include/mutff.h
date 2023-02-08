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

#define MuTFF_FOURCC(a, b, c, d)                                         \
  ((uint32_t)(a) << 24) + ((uint32_t)(b) << 16) + ((uint32_t)(c) << 8) + \
      (uint32_t)(d)

///
/// @brief A 24-bit unsigned integer
///
typedef uint32_t mutff_uint24_t;

///
/// @brief A signed integer with at least 2 bits
///
typedef int8_t mutff_int_least2_t;

///
/// @brief An unsigned integer with at least 30 bits
///
typedef uint32_t mutff_int_least30_t;

///
/// @brief A fixed-point number with 8 integral bits and 8 fractional bits
///
typedef struct {
  int8_t integral;
  uint8_t fractional;
} mutff_q8_8_t;

///
/// @brief A fixed-point number with 16 integral bits and 16 fractional bits
///
typedef struct {
  int16_t integral;
  uint16_t fractional;
} mutff_q16_16_t;

///
/// @brief A fixed-point number with 2 integral bits and 30 fractional bits
///
typedef struct {
  mutff_int_least2_t integral;
  mutff_int_least30_t fractional;
} mutff_q2_30_t;

///
/// @brief A QuickTime matrix structure
///
typedef struct {
  mutff_q16_16_t a;
  mutff_q16_16_t b;
  mutff_q2_30_t u;
  mutff_q16_16_t c;
  mutff_q16_16_t d;
  mutff_q2_30_t v;
  mutff_q16_16_t tx;
  mutff_q16_16_t ty;
  mutff_q2_30_t w;
} MuTFFMatrix;

///
/// @brief A generic error in the MuTFF library
///
typedef enum {
  MuTFFErrorNone,
  MuTFFErrorIOError,
  MuTFFErrorEOF,
  MuTFFErrorBadFormat,
  MuTFFErrorOutOfMemory,
} MuTFFError;

///
/// @brief A QuickDraw rectangle
/// @see
/// https://developer.apple.com/library/archive/documentation/mac/pdf/ImagingWithQuickDraw.pdf
/// Section 2-6
///
typedef struct {
  uint16_t top;
  uint16_t left;
  uint16_t bottom;
  uint16_t right;
} MuTFFQuickDrawRect;

///
/// @brief Read a QuickDraw rectangle
///
/// The current file offset must be at the start of the rectangle
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read
/// @param [out] out Output
/// @return          The MuTFFError code
///
MuTFFError mutff_read_quickdraw_rect(FILE *fd, size_t *n,
                                     MuTFFQuickDrawRect *out);

///
/// @brief Write a QuickDraw rectangle
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   Input
/// @return          The MuTFFError code
///
MuTFFError mutff_write_quickdraw_rect(FILE *fd, size_t *n,
                                      const MuTFFQuickDrawRect *in);

///
/// @brief Maximum size of the additional data in a QuickDraw region
///
#define MuTFF_MAX_QUICKDRAW_REGION_DATA_SIZE 8U

///
/// @brief A QuickDraw region
/// @see
/// https://developer.apple.com/library/archive/documentation/mac/pdf/ImagingWithQuickDraw.pdf
/// Section 2-7
///
typedef struct {
  uint16_t size;
  MuTFFQuickDrawRect rect;
  char data[MuTFF_MAX_QUICKDRAW_REGION_DATA_SIZE];
} MuTFFQuickDrawRegion;

///
/// @brief Read a QuickDraw region
///
/// The current file offset must be at the start of the region
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read
/// @param [out] out Output
/// @return          The MuTFFError code
///
MuTFFError mutff_read_quickdraw_region(FILE *fd, size_t *n,
                                       MuTFFQuickDrawRegion *out);

///
/// @brief Write a QuickDraw region
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_quickdraw_region(FILE *fd, size_t *n,
                                        const MuTFFQuickDrawRegion *in);

///
/// @brief The maximum number of compatible brands
///
#define MuTFF_MAX_COMPATIBLE_BRANDS 4U

///
/// @brief File type atom
///
/// The file type atom is a (semi-)optional atom contained in the file. While
/// older QuickTime files may not include the atom, new ones should.
///
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/MuTFF/MuTFFChap1/mutff1.html#//apple_ref/doc/uid/TP40000939-CH203-CJBCBIFF
///
typedef struct {
  uint32_t major_brand;
  uint32_t minor_version;
  size_t compatible_brands_count;
  uint32_t compatible_brands[MuTFF_MAX_COMPATIBLE_BRANDS];
} MuTFFFileTypeAtom;

///
/// @brief Read a file type compatibility atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read
/// @param [out]     The parsed atom
/// @return          The MUTFFError code
///
MuTFFError mutff_read_file_type_atom(FILE *fd, size_t *n,
                                     MuTFFFileTypeAtom *out);

///
/// @brief Write a file type compatibility atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MUTFFError code
///
MuTFFError mutff_write_file_type_atom(FILE *fd, size_t *n,
                                      const MuTFFFileTypeAtom *in);

///
/// @brief A movie data atom
///
/// Any number of movie data atoms may be contained in the file. They contain
/// media data. µTFF does not copy the data from movie data atoms into memory.
/// Instead it provides the file offset of the data.
///
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap1/qtff1.html#//apple_ref/doc/uid/TP40000939-CH203-55478
///
typedef struct {
  uint64_t data_size;
  long offset;
} MuTFFMovieDataAtom;

///
/// @brief Read a movie data atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read
/// @param [out]     The parsed atom
/// @return          The MUTFFError code
///
MuTFFError mutff_read_movie_data_atom(FILE *fd, size_t *n,
                                      MuTFFMovieDataAtom *out);

///
/// @brief Write a movie data atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MUTFFError code
///
MuTFFError mutff_write_movie_data_atom(FILE *fd, size_t *n,
                                       const MuTFFMovieDataAtom *in);

///
/// @brief Free (unused) space atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/MuTFF/MuTFFChap1/mutff1.html#//apple_ref/doc/uid/TP40000939-CH203-55464
///
typedef struct {
  uint64_t atom_size;
} MuTFFFreeAtom;

///
/// @brief Read a free atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read
/// @param [out]     The parsed atom
/// @return          The MUTFFError code
///
MuTFFError mutff_read_free_atom(FILE *fd, size_t *n, MuTFFFreeAtom *out);

///
/// @brief Write a free atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MUTFFError code
///
MuTFFError mutff_write_free_atom(FILE *fd, size_t *n, const MuTFFFreeAtom *in);

///
/// @brief Skip (unused) space atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/MuTFF/MuTFFChap1/mutff1.html#//apple_ref/doc/uid/TP40000939-CH203-55464
///
typedef struct {
  uint64_t atom_size;
} MuTFFSkipAtom;

///
/// @brief Read a skip atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read
/// @param [out]     The parsed atom
/// @return          The MUTFFError code
///
MuTFFError mutff_read_skip_atom(FILE *fd, size_t *n, MuTFFSkipAtom *out);

///
/// @brief Write a skip atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MUTFFError code
///
MuTFFError mutff_write_skip_atom(FILE *fd, size_t *n, const MuTFFSkipAtom *in);

///
/// @brief Wide (reserved) space atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/MuTFF/MuTFFChap1/mutff1.html#//apple_ref/doc/uid/TP40000939-CH203-55464
///
typedef struct {
  uint64_t atom_size;
} MuTFFWideAtom;

///
/// @brief Read a wide atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read
/// @param [out]     The parsed atom
/// @return          The MUTFFError code
///
MuTFFError mutff_read_wide_atom(FILE *fd, size_t *n, MuTFFWideAtom *out);

///
/// @brief Write a wide atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MUTFFError code
///
MuTFFError mutff_write_wide_atom(FILE *fd, size_t *n, const MuTFFWideAtom *in);

///
/// @brief Preview atom
///
/// Preview atoms are an optional atom contained in the file. They describe file
/// metadata which may be used in producing a preview image or information about
/// the file.
///
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/MuTFF/MuTFFChap1/mutff1.html#//apple_ref/doc/uid/TP40000939-CH203-38240
///
typedef struct {
  uint32_t modification_time;
  uint16_t version;
  uint32_t atom_type;
  uint16_t atom_index;
} MuTFFPreviewAtom;

///
/// @brief Read a preview atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read
/// @param [out]     The parsed atom
/// @return          The MUTFFError code
///
MuTFFError mutff_read_preview_atom(FILE *fd, size_t *n, MuTFFPreviewAtom *out);

///
/// @brief Write a preview atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MUTFFError code
///
MuTFFError mutff_write_preview_atom(FILE *fd, size_t *n,
                                    const MuTFFPreviewAtom *in);

///
/// @brief Movie header atom.
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCGFGJG
///
typedef struct {
  uint8_t version;
  mutff_uint24_t flags;
  uint32_t creation_time;
  uint32_t modification_time;
  uint32_t time_scale;
  uint32_t duration;
  mutff_q16_16_t preferred_rate;
  mutff_q8_8_t preferred_volume;
  MuTFFMatrix matrix_structure;
  uint32_t preview_time;
  uint32_t preview_duration;
  uint32_t poster_time;
  uint32_t selection_time;
  uint32_t selection_duration;
  uint32_t current_time;
  uint32_t next_track_id;
} MuTFFMovieHeaderAtom;

///
/// @brief Read a movie header atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out]     The parsed atom
/// @return          The MUTFFError code
///
MuTFFError mutff_read_movie_header_atom(FILE *fd, size_t *n,
                                        MuTFFMovieHeaderAtom *out);

///
/// @brief Write a movie header atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MUTFFError code
///
MuTFFError mutff_write_movie_header_atom(FILE *fd, size_t *n,
                                         const MuTFFMovieHeaderAtom *in);

///
///
/// @brief Clipping region atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCHDAIB
/// @see
/// https://developer.apple.com/library/archive/documentation/mac/pdf/ImagingWithQuickDraw.pdf
/// Section 2-7
///
typedef struct {
  MuTFFQuickDrawRegion region;
} MuTFFClippingRegionAtom;

///
/// @brief Read a clipping region atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out]     The parsed atom
/// @return          The MUTFFError code
///
MuTFFError mutff_read_clipping_region_atom(FILE *fd, size_t *n,
                                           MuTFFClippingRegionAtom *out);

///
/// @brief Write a clipping region atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_clipping_region_atom(FILE *fd, size_t *n,
                                            const MuTFFClippingRegionAtom *in);

///
/// @brief Clipping atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCIHBFG
///
typedef struct {
  MuTFFClippingRegionAtom clipping_region;
} MuTFFClippingAtom;

///
/// @brief Read a clipping atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_clipping_atom(FILE *fd, size_t *n,
                                    MuTFFClippingAtom *out);

///
/// @brief Write a clipping atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_clipping_atom(FILE *fd, size_t *n,
                                     const MuTFFClippingAtom *in);

///
/// @brief The maximum number of entries in the color table
///
#define MuTFF_MAX_COLOR_TABLE_SIZE 16U

///
/// @brief Color table atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCBDJEB
///
typedef struct {
  uint32_t color_table_seed;
  uint16_t color_table_flags;
  uint16_t color_table_size;
  uint16_t color_array[MuTFF_MAX_COLOR_TABLE_SIZE][4];
} MuTFFColorTableAtom;

///
/// @brief Read a color table atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_color_table_atom(FILE *fd, size_t *n,
                                       MuTFFColorTableAtom *out);

///
/// @brief Write a color table atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_color_table_atom(FILE *fd, size_t *n,
                                        const MuTFFColorTableAtom *in);

///
/// @brief The maximum size of the data in an entry of a user data list
///
#define MuTFF_MAX_USER_DATA_ENTRY_SIZE 64U

typedef struct {
  uint32_t type;
  uint32_t data_size;
  char data[MuTFF_MAX_USER_DATA_ENTRY_SIZE];
} MuTFFUserDataListEntry;

///
/// @brief Read an entry in a user data list
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out  The parsed entry
/// @return           If the entry was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_user_data_list_entry(FILE *fd, size_t *n,
                                           MuTFFUserDataListEntry *out);

///
/// @brief Write an entry in a user data list
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return           If the entry was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_user_data_list_entry(FILE *fd, size_t *n,
                                            const MuTFFUserDataListEntry *in);

///
/// @brief The maximum number of entries in the user data list
///
#define MuTFF_MAX_USER_DATA_ITEMS 16U

///
/// @brief User data atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCCFFGD
///
typedef struct {
  size_t list_entries;
  MuTFFUserDataListEntry user_data_list[MuTFF_MAX_USER_DATA_ITEMS];
} MuTFFUserDataAtom;

///
/// @brief Read a user data atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_user_data_atom(FILE *fd, size_t *n,
                                     MuTFFUserDataAtom *out);

///
/// @brief Write a user data atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_user_data_atom(FILE *fd, size_t *n,
                                      const MuTFFUserDataAtom *in);

///
/// @brief Track header atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCEIDFA
///
typedef struct {
  uint8_t version;
  mutff_uint24_t flags;
  uint32_t creation_time;
  uint32_t modification_time;
  uint32_t track_id;
  uint32_t duration;
  uint16_t layer;
  uint16_t alternate_group;
  mutff_q8_8_t volume;
  MuTFFMatrix matrix_structure;
  mutff_q16_16_t track_width;
  mutff_q16_16_t track_height;
} MuTFFTrackHeaderAtom;

///
/// @brief Read a track header atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_track_header_atom(FILE *fd, size_t *n,
                                        MuTFFTrackHeaderAtom *out);

///
/// @brief Write a track header atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_track_header_atom(FILE *fd, size_t *n,
                                         const MuTFFTrackHeaderAtom *in);

///
/// @brief Track clean aperture dimensions atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-SW3
///
typedef struct {
  uint8_t version;
  mutff_uint24_t flags;
  mutff_q16_16_t width;
  mutff_q16_16_t height;
} MuTFFTrackCleanApertureDimensionsAtom;

///
/// @brief Read a track clean aperture dimensions atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_track_clean_aperture_dimensions_atom(
    FILE *fd, size_t *n, MuTFFTrackCleanApertureDimensionsAtom *out);

///
/// @brief Write a track clean aperture dimensions atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_track_clean_aperture_dimensions_atom(
    FILE *fd, size_t *n, const MuTFFTrackCleanApertureDimensionsAtom *in);

///
/// @brief Track production aperture dimensions atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-SW13
///
typedef struct {
  uint8_t version;
  mutff_uint24_t flags;
  mutff_q16_16_t width;
  mutff_q16_16_t height;
} MuTFFTrackProductionApertureDimensionsAtom;

///
/// @brief Read a track production aperture dimensions atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_track_production_aperture_dimensions_atom(
    FILE *fd, size_t *n, MuTFFTrackProductionApertureDimensionsAtom *out);

///
/// @brief Write a track production aperture dimensions atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_track_production_aperture_dimensions_atom(
    FILE *fd, size_t *n, const MuTFFTrackProductionApertureDimensionsAtom *in);

///
/// @brief Track encoded pixels dimensions atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-SW14
///
typedef struct {
  uint8_t version;
  mutff_uint24_t flags;
  mutff_q16_16_t width;
  mutff_q16_16_t height;
} MuTFFTrackEncodedPixelsDimensionsAtom;

///
/// @brief Read a track encoded pixels dimensions atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_track_encoded_pixels_dimensions_atom(
    FILE *fd, size_t *n, MuTFFTrackEncodedPixelsDimensionsAtom *out);

///
/// @brief Write a track encoded pixels dimensions atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_track_encoded_pixels_dimensions_atom(
    FILE *fd, size_t *n, const MuTFFTrackEncodedPixelsDimensionsAtom *in);

///
/// @brief Track aperture mode dimensions atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-SW15
///
typedef struct {
  MuTFFTrackCleanApertureDimensionsAtom track_clean_aperture_dimensions;
  MuTFFTrackProductionApertureDimensionsAtom
      track_production_aperture_dimensions;
  MuTFFTrackEncodedPixelsDimensionsAtom track_encoded_pixels_dimensions;
} MuTFFTrackApertureModeDimensionsAtom;

///
/// @brief Read a track aperture mode dimensions atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_track_aperture_mode_dimensions_atom(
    FILE *fd, size_t *n, MuTFFTrackApertureModeDimensionsAtom *out);

///
/// @brief Write a track aperture mode dimensions atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_track_aperture_mode_dimensions_atom(
    FILE *fd, size_t *n, const MuTFFTrackApertureModeDimensionsAtom *in);

///
/// @brief The maximum length of the format-specific data in a sample
///        description
/// @see MuTFFSampleDescription
///
#define MuTFF_MAX_SAMPLE_DESCRIPTION_DATA_LEN 16U

///
/// @brief A sample description
/// @note This is not an atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-61112
///
typedef struct {
  uint32_t size;
  uint32_t data_format;
  uint16_t data_reference_index;
  char additional_data[MuTFF_MAX_SAMPLE_DESCRIPTION_DATA_LEN];
} MuTFFSampleDescription;

///
/// @brief Read a sample description
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed description
/// @return          The MuTFFError code
///
MuTFFError mutff_read_sample_description(FILE *fd, size_t *n,
                                         MuTFFSampleDescription *out);

///
/// @brief Write a sample description
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_sample_description(FILE *fd, size_t *n,
                                          const MuTFFSampleDescription *in);

///
/// @brief The maximum length of the data in a compressed matte atom
/// @see MuTFFCompressedMatteAtom
///
#define MuTFF_MAX_MATTE_DATA_LEN 16U

///
/// @brief Compressed matte atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-25573
///
typedef struct {
  uint8_t version;
  mutff_uint24_t flags;
  MuTFFSampleDescription matte_image_description_structure;
  size_t matte_data_len;
  char matte_data[MuTFF_MAX_MATTE_DATA_LEN];
} MuTFFCompressedMatteAtom;

///
/// @brief Read a compressed matte atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_compressed_matte_atom(FILE *fd, size_t *n,
                                            MuTFFCompressedMatteAtom *out);

///
/// @brief Write a compressed matte atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_compressed_matte_atom(
    FILE *fd, size_t *n, const MuTFFCompressedMatteAtom *in);

///
/// @brief Track matte atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-25567
///
typedef struct {
  MuTFFCompressedMatteAtom compressed_matte_atom;
} MuTFFTrackMatteAtom;

///
/// @brief Read a track matte atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_track_matte_atom(FILE *fd, size_t *n,
                                       MuTFFTrackMatteAtom *out);

///
/// @brief Write a track matte atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_track_matte_atom(FILE *fd, size_t *n,
                                        const MuTFFTrackMatteAtom *in);

///
/// @brief Entry an an edit list
/// @note Not an atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCGDIJF
///
typedef struct {
  uint32_t track_duration;
  uint32_t media_time;
  mutff_q16_16_t media_rate;
} MuTFFEditListEntry;

///
/// @brief Read an edit list entry
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_edit_list_entry(FILE *fd, size_t *n,
                                      MuTFFEditListEntry *out);

///
/// @brief Write a movie data atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_edit_list_entry(FILE *fd, size_t *n,
                                       const MuTFFEditListEntry *in);

///
/// @brief The maximum number of entries in an edit list atom
/// @see MuTFFEditListAtom
///
#define MuTFF_MAX_EDIT_LIST_ENTRIES 8U

///
/// @brief Edit list atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCGDIJF
///
typedef struct {
  uint8_t version;
  mutff_uint24_t flags;
  uint32_t number_of_entries;
  MuTFFEditListEntry edit_list_table[MuTFF_MAX_EDIT_LIST_ENTRIES];
} MuTFFEditListAtom;

///
/// @brief Read an edit list atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_edit_list_atom(FILE *fd, size_t *n,
                                     MuTFFEditListAtom *out);

///
/// @brief Write an edit list atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_edit_list_atom(FILE *fd, size_t *n,
                                      const MuTFFEditListAtom *in);

///
/// @brief Edit atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCCFBEF
///
typedef struct {
  MuTFFEditListAtom edit_list_atom;
} MuTFFEditAtom;

///
/// @brief Read an edit atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_edit_atom(FILE *fd, size_t *n, MuTFFEditAtom *out);

///
/// @brief Write an edit atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_edit_atom(FILE *fd, size_t *n, const MuTFFEditAtom *in);

///
/// @brief The maximum track IDs in a track reference type atom
/// @see MuTFFTrackReferenceTypeAtom
///
#define MuTFF_MAX_TRACK_REFERENCE_TYPE_TRACK_IDS 4U

///
/// @brief Track reference type atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCGDBAF
///
typedef struct {
  uint32_t type;
  size_t track_id_count;
  uint32_t track_ids[MuTFF_MAX_TRACK_REFERENCE_TYPE_TRACK_IDS];
} MuTFFTrackReferenceTypeAtom;

///
/// @brief Read a track reference type atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_track_reference_type_atom(
    FILE *fd, size_t *n, MuTFFTrackReferenceTypeAtom *out);

///
/// @brief Write a track reference type atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_track_reference_type_atom(
    FILE *fd, size_t *n, const MuTFFTrackReferenceTypeAtom *in);

///
/// @brief The maximum reference type atoms in a track reference atom
/// @see MuTFFTrackReferenceAtom
///
#define MuTFF_MAX_TRACK_REFERENCE_TYPE_ATOMS 4U

///
/// @brief Track reference atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCGDBAF
///
typedef struct {
  size_t track_reference_type_count;
  MuTFFTrackReferenceTypeAtom
      track_reference_type[MuTFF_MAX_TRACK_REFERENCE_TYPE_ATOMS];
} MuTFFTrackReferenceAtom;

/// @brief Read a track reference atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_track_reference_atom(FILE *fd, size_t *n,
                                           MuTFFTrackReferenceAtom *out);

///
/// @brief Write a track reference atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_track_reference_atom(FILE *fd, size_t *n,
                                            const MuTFFTrackReferenceAtom *in);

///
/// @brief Track exclude from autoselection atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-SW47
///
typedef struct {
  // A dummy char is used here to force consistent behaviour between C and C++
  char _dummy;
} MuTFFTrackExcludeFromAutoselectionAtom;

/// @brief Read a track exclude from autoselection atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_track_exclude_from_autoselection_atom(
    FILE *fd, size_t *n, MuTFFTrackExcludeFromAutoselectionAtom *out);

///
/// @brief Write a track exclude from autoselection atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_track_exclude_from_autoselection_atom(
    FILE *fd, size_t *n, const MuTFFTrackExcludeFromAutoselectionAtom *in);

///
/// @brief Track load settings atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCGIIFI
///
typedef struct {
  uint32_t preload_start_time;
  uint32_t preload_duration;
  uint32_t preload_flags;
  uint32_t default_hints;
} MuTFFTrackLoadSettingsAtom;

/// @brief Read a track load settings atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_track_load_settings_atom(FILE *fd, size_t *n,
                                               MuTFFTrackLoadSettingsAtom *out);

///
/// @brief Write a track load settings atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_track_load_settings_atom(
    FILE *fd, size_t *n, const MuTFFTrackLoadSettingsAtom *in);

///
/// @brief Input type atom
///
typedef struct {
  uint32_t input_type;
} MuTFFInputTypeAtom;

/// @brief Read an input type atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_input_type_atom(FILE *fd, size_t *n,
                                      MuTFFInputTypeAtom *out);

///
/// @brief Write an input type atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_input_type_atom(FILE *fd, size_t *n,
                                       const MuTFFInputTypeAtom *in);

///
/// @brief Object ID atom
///
typedef struct {
  uint32_t object_id;
} MuTFFObjectIDAtom;

/// @brief Read an object ID atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_object_id_atom(FILE *fd, size_t *n,
                                     MuTFFObjectIDAtom *out);

///
/// @brief Write an object ID atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_object_id_atom(FILE *fd, size_t *n,
                                      const MuTFFObjectIDAtom *in);

///
/// @brief Track input atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCDJBFG
///
typedef struct {
  uint32_t atom_id;
  uint16_t child_count;

  MuTFFInputTypeAtom input_type_atom;

  bool object_id_atom_present;
  MuTFFObjectIDAtom object_id_atom;
} MuTFFTrackInputAtom;

/// @brief Read a track input atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_track_input_atom(FILE *fd, size_t *n,
                                       MuTFFTrackInputAtom *out);

///
/// @brief Write a track input atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_track_input_atom(FILE *fd, size_t *n,
                                        const MuTFFTrackInputAtom *in);

///
/// @brief Maximum entries in a track input map
/// @see MuTFFTrackInputMapAtom
///
#define MuTFF_MAX_TRACK_INPUT_ATOMS 2U

///
/// @brief Track input map atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCDJBFG
///
typedef struct {
  size_t track_input_atom_count;
  MuTFFTrackInputAtom track_input_atoms[MuTFF_MAX_TRACK_INPUT_ATOMS];
} MuTFFTrackInputMapAtom;

///
/// @brief Read track input map atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_track_input_map_atom(FILE *fd, size_t *n,
                                           MuTFFTrackInputMapAtom *out);

///
/// @brief Write a track input map atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_track_input_map_atom(FILE *fd, size_t *n,
                                            const MuTFFTrackInputMapAtom *in);

///
/// @brief Media header atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-25615
///
typedef struct {
  uint8_t version;
  mutff_uint24_t flags;
  uint32_t creation_time;
  uint32_t modification_time;
  uint32_t time_scale;
  uint32_t duration;
  uint16_t language;
  uint16_t quality;
} MuTFFMediaHeaderAtom;

///
/// @brief Read media header atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_media_header_atom(FILE *fd, size_t *n,
                                        MuTFFMediaHeaderAtom *out);

///
/// @brief Write a media header atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_media_header_atom(FILE *fd, size_t *n,
                                         const MuTFFMediaHeaderAtom *in);

///
/// @brief Maximum language tag length
/// @see MuTFFExtendedLanguageTagAtom
///
#define MuTFF_MAX_LANGUAGE_TAG_LENGTH 8U

///
/// @brief Extended language tag atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-SW16
///
typedef struct {
  uint8_t version;
  mutff_uint24_t flags;
  char language_tag_string[MuTFF_MAX_LANGUAGE_TAG_LENGTH];
} MuTFFExtendedLanguageTagAtom;

///
/// @brief Read extended language tag atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_extended_language_tag_atom(
    FILE *fd, size_t *n, MuTFFExtendedLanguageTagAtom *out);

///
/// @brief Write an extended language tag atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_extended_language_tag_atom(
    FILE *fd, size_t *n, const MuTFFExtendedLanguageTagAtom *in);

///
/// @brief Maximum component name length
/// @see MuTFFHandlerReferenceAtom
///
#define MuTFF_MAX_COMPONENT_NAME_LENGTH 24U

///
/// @brief Handler reference atom
/// @note  The component name should be a multiple of four characters long.
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCIBHFD
///
typedef struct {
  uint8_t version;
  mutff_uint24_t flags;
  uint32_t component_type;
  uint32_t component_subtype;
  uint32_t component_manufacturer;
  uint32_t component_flags;
  uint32_t component_flags_mask;
  char component_name[MuTFF_MAX_COMPONENT_NAME_LENGTH + 1U];
} MuTFFHandlerReferenceAtom;

/// @brief Read a handler reference atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_handler_reference_atom(FILE *fd, size_t *n,
                                             MuTFFHandlerReferenceAtom *out);

///
/// @brief Write a handler reference atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_handler_reference_atom(
    FILE *fd, size_t *n, const MuTFFHandlerReferenceAtom *in);

///
/// @brief Video media information header atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCFDGIG
///
typedef struct {
  uint8_t version;
  mutff_uint24_t flags;
  uint16_t graphics_mode;
  uint16_t opcolor[3];
} MuTFFVideoMediaInformationHeaderAtom;

/// @brief Read a video media information header atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_video_media_information_header_atom(
    FILE *fd, size_t *n, MuTFFVideoMediaInformationHeaderAtom *out);

///
/// @brief Write a video media information header atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_video_media_information_header_atom(
    FILE *fd, size_t *n, const MuTFFVideoMediaInformationHeaderAtom *in);

///
/// @brief The maximum size of the data in a data reference
/// @see MuTFFDataReference
///
#define MuTFF_MAX_DATA_REFERENCE_DATA_SIZE 16U

///
/// @brief Data reference
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCGGDAE
///
typedef struct {
  uint32_t type;
  uint8_t version;
  mutff_uint24_t flags;
  uint32_t data_size;
  char data[MuTFF_MAX_DATA_REFERENCE_DATA_SIZE];
} MuTFFDataReference;

///
/// @brief Read a data reference
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read
/// @param [out] out The parsed reference
/// @return          The MuTFFError code
///
MuTFFError mutff_read_data_reference(FILE *fd, size_t *n,
                                     MuTFFDataReference *out);

///
/// @brief Write a data reference
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_data_reference(FILE *fd, size_t *n,
                                      const MuTFFDataReference *in);

///
/// @brief The maximum number of data references in a data reference atom
/// @see MuTFFDataReferenceAtom
///
#define MuTFF_MAX_DATA_REFERENCES 4U

///
/// @brief Data reference atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCGGDAE
///
typedef struct {
  uint8_t version;
  mutff_uint24_t flags;
  uint32_t number_of_entries;
  MuTFFDataReference data_references[MuTFF_MAX_DATA_REFERENCES];
} MuTFFDataReferenceAtom;

///
/// @brief Read a data reference atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_data_reference_atom(FILE *fd, size_t *n,
                                          MuTFFDataReferenceAtom *out);

///
/// @brief Write a data reference atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_data_reference_atom(FILE *fd, size_t *n,
                                           const MuTFFDataReferenceAtom *in);

///
/// @brief Data information atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCIFAIC
///
typedef struct {
  MuTFFDataReferenceAtom data_reference;
} MuTFFDataInformationAtom;

///
/// @brief Read a data information atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_data_information_atom(FILE *fd, size_t *n,
                                            MuTFFDataInformationAtom *out);

///
/// @brief Write a data information atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_data_information_atom(
    FILE *fd, size_t *n, const MuTFFDataInformationAtom *in);

#define MuTFF_MAX_SAMPLE_DESCRIPTION_TABLE_LEN 8U

///
/// @brief Sample description atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-25691
///
typedef struct {
  uint8_t version;
  mutff_uint24_t flags;
  uint32_t number_of_entries;
  MuTFFSampleDescription
      sample_description_table[MuTFF_MAX_SAMPLE_DESCRIPTION_TABLE_LEN];
} MuTFFSampleDescriptionAtom;

///
/// @brief Read a sample description atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_sample_description_atom(FILE *fd, size_t *n,
                                              MuTFFSampleDescriptionAtom *out);

///
/// @brief Write a sample description atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_sample_description_atom(
    FILE *fd, size_t *n, const MuTFFSampleDescriptionAtom *in);

///
/// @brief Entry in the time-to-sample table
/// @see MuTFFTimeToSampleAtom
///
typedef struct {
  uint32_t sample_count;
  uint32_t sample_duration;
} MuTFFTimeToSampleTableEntry;

///
/// @brief Read a time-to-sample table entry
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_time_to_sample_table_entry(
    FILE *fd, size_t *n, MuTFFTimeToSampleTableEntry *out);

///
/// @brief Write a time-to-sample table entry
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_time_to_sample_table_entry(
    FILE *fd, size_t *n, const MuTFFTimeToSampleTableEntry *in);

///
/// @brief Maximum number of entries in a time-to-sample atom
/// @see MuTFFTimeToSampleAtom
///
#define MuTFF_MAX_TIME_TO_SAMPLE_TABLE_LEN 4U

///
/// @brief Time-to-sample atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCGFJII
///
typedef struct {
  uint8_t version;
  mutff_uint24_t flags;
  uint32_t number_of_entries;
  MuTFFTimeToSampleTableEntry
      time_to_sample_table[MuTFF_MAX_TIME_TO_SAMPLE_TABLE_LEN];
} MuTFFTimeToSampleAtom;

///
/// @brief Read a time-to-sample atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_time_to_sample_atom(FILE *fd, size_t *n,
                                          MuTFFTimeToSampleAtom *out);

///
/// @brief Write a time-to-sample atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_time_to_sample_atom(FILE *fd, size_t *n,
                                           const MuTFFTimeToSampleAtom *in);

///
/// @brief Entry in the composition offset table
///
typedef struct {
  uint32_t sample_count;
  uint32_t composition_offset;
} MuTFFCompositionOffsetTableEntry;

///
/// @brief Read a composition offset table entry
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_composition_offset_table_entry(
    FILE *fd, size_t *n, MuTFFCompositionOffsetTableEntry *out);

///
/// @brief Write a composition offset table entry
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_composition_offset_table_entry(
    FILE *fd, size_t *n, const MuTFFCompositionOffsetTableEntry *in);

///
/// @brief Maximum length of the composition offset table
/// @see MuTFFCompositionOffsetAtom
///
#define MuTFF_MAX_COMPOSITION_OFFSET_TABLE_LEN 4U

///
/// @brief Composition offset atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-SW19
/// @note The MPEG-4 specification calls these composition time-to-sample boxes.
/// The format is identical.
///
typedef struct {
  uint8_t version;
  mutff_uint24_t flags;
  uint32_t entry_count;
  MuTFFCompositionOffsetTableEntry
      composition_offset_table[MuTFF_MAX_COMPOSITION_OFFSET_TABLE_LEN];
} MuTFFCompositionOffsetAtom;

///
/// @brief Read a composition offset atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_composition_offset_atom(FILE *fd, size_t *n,
                                              MuTFFCompositionOffsetAtom *out);

///
/// @brief Write a composition offset atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_composition_offset_atom(
    FILE *fd, size_t *n, const MuTFFCompositionOffsetAtom *in);

///
/// @brief Composition shift least greatest atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-SW20
/// @note The MPEG-4 specification calls these composition to decode boxes.
///
typedef struct {
  uint8_t version;
  mutff_uint24_t flags;
  uint32_t composition_offset_to_display_offset_shift;
  int32_t least_display_offset;
  int32_t greatest_display_offset;
  int32_t display_start_time;
  int32_t display_end_time;
} MuTFFCompositionShiftLeastGreatestAtom;

///
/// @brief Read a composition shift least greatest atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_composition_shift_least_greatest_atom(
    FILE *fd, size_t *n, MuTFFCompositionShiftLeastGreatestAtom *out);

///
/// @brief Write a composition shift least greatest atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_composition_shift_least_greatest_atom(
    FILE *fd, size_t *n, const MuTFFCompositionShiftLeastGreatestAtom *in);

///
/// @brief Maximum length of the sync sample table
///
#define MuTFF_MAX_SYNC_SAMPLE_TABLE_LEN 8U

///
/// @brief Sync sample atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-25701
///
typedef struct {
  uint8_t version;
  mutff_uint24_t flags;
  uint32_t number_of_entries;
  uint32_t sync_sample_table[MuTFF_MAX_SYNC_SAMPLE_TABLE_LEN];
} MuTFFSyncSampleAtom;

///
/// @brief Read a sync sample atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_sync_sample_atom(FILE *fd, size_t *n,
                                       MuTFFSyncSampleAtom *out);

///
/// @brief Write a sync sample atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_sync_sample_atom(FILE *fd, size_t *n,
                                        const MuTFFSyncSampleAtom *in);

///
/// @brief Maximum length of the partial sync sample table
///
#define MuTFF_MAX_PARTIAL_SYNC_SAMPLE_TABLE_LEN 4U

///
/// @brief Partial sync sample atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-SW21
///
typedef struct {
  uint8_t version;
  mutff_uint24_t flags;
  uint32_t entry_count;
  uint32_t partial_sync_sample_table[MuTFF_MAX_PARTIAL_SYNC_SAMPLE_TABLE_LEN];
} MuTFFPartialSyncSampleAtom;

///
/// @brief Read a partial sync sample atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_partial_sync_sample_atom(FILE *fd, size_t *n,
                                               MuTFFPartialSyncSampleAtom *out);

///
/// @brief Write a partial sync sample atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_partial_sync_sample_atom(
    FILE *fd, size_t *n, const MuTFFPartialSyncSampleAtom *in);

///
/// @brief Entry in the sample-to-chunk table
/// @see MuTFFSampleToChunkAtom
///
typedef struct {
  uint32_t first_chunk;
  uint32_t samples_per_chunk;
  uint32_t sample_description_id;
} MuTFFSampleToChunkTableEntry;

///
/// @brief Read a sample to chunk table entry
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out  The parsed entry
/// @return           If the entry was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_sample_to_chunk_table_entry(
    FILE *fd, size_t *n, MuTFFSampleToChunkTableEntry *out);

///
/// @brief Write a sample-to-chunk table entry
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_sample_to_chunk_table_entry(
    FILE *fd, size_t *n, const MuTFFSampleToChunkTableEntry *in);

///
/// @brief Maximum length of the sample-to-chunk table
/// @see MuTFFSampleToChunkAtom
///
#define MuTFF_MAX_SAMPLE_TO_CHUNK_TABLE_LEN 4U

///
/// @brief Sample to chunk atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-25706
///
typedef struct {
  uint8_t version;
  mutff_uint24_t flags;
  uint32_t number_of_entries;
  MuTFFSampleToChunkTableEntry
      sample_to_chunk_table[MuTFF_MAX_SAMPLE_TO_CHUNK_TABLE_LEN];
} MuTFFSampleToChunkAtom;

///
/// @brief Read a sample to chunk atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_sample_to_chunk_atom(FILE *fd, size_t *n,
                                           MuTFFSampleToChunkAtom *out);

///
/// @brief Write a sample-to-chunk atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_sample_to_chunk_atom(FILE *fd, size_t *n,
                                            const MuTFFSampleToChunkAtom *in);

///
/// @brief Maximum number of entries in a sample size table
///
#define MuTFF_MAX_SAMPLE_SIZE_TABLE_LEN 4U

///
/// @brief Sample size atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-25710
///
typedef struct {
  uint8_t version;
  mutff_uint24_t flags;
  uint32_t sample_size;
  uint32_t number_of_entries;
  uint32_t sample_size_table[MuTFF_MAX_SAMPLE_SIZE_TABLE_LEN];
} MuTFFSampleSizeAtom;

///
/// @brief Read a sample size atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_sample_size_atom(FILE *fd, size_t *n,
                                       MuTFFSampleSizeAtom *out);

///
/// @brief Write a sample size atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_sample_size_atom(FILE *fd, size_t *n,
                                        const MuTFFSampleSizeAtom *in);

///
/// @brief Maximum length of the chunk offset table
///
#define MuTFF_MAX_CHUNK_OFFSET_TABLE_LEN 4U

///
/// @brief Chunk offset atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-25715
///
typedef struct {
  uint8_t version;
  mutff_uint24_t flags;
  uint32_t number_of_entries;
  uint32_t chunk_offset_table[MuTFF_MAX_CHUNK_OFFSET_TABLE_LEN];
} MuTFFChunkOffsetAtom;

///
/// @brief Read a chunk offset atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_chunk_offset_atom(FILE *fd, size_t *n,
                                        MuTFFChunkOffsetAtom *out);

///
/// @brief Write a chunk offset atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_chunk_offset_atom(FILE *fd, size_t *n,
                                         const MuTFFChunkOffsetAtom *in);

#define MuTFF_MAX_SAMPLE_DEPENDENCY_FLAGS_TABLE_LEN 4U

///
/// @brief Sample dependency flags atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-SW22
///
typedef struct {
  uint8_t version;
  mutff_uint24_t flags;
  uint32_t data_size;
  unsigned char sample_dependency_flags_table
      [MuTFF_MAX_SAMPLE_DEPENDENCY_FLAGS_TABLE_LEN];
} MuTFFSampleDependencyFlagsAtom;

///
/// @brief Read a sample dependency flags atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read to read from
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_sample_dependency_flags_atom(
    FILE *fd, size_t *n, MuTFFSampleDependencyFlagsAtom *out);

///
/// @brief Write a sample dependency flags atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_sample_dependency_flags_atom(
    FILE *fd, size_t *n, const MuTFFSampleDependencyFlagsAtom *in);

///
/// @brief Sample table atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCBFDFF
///
typedef struct {
  MuTFFSampleDescriptionAtom sample_description;

  MuTFFTimeToSampleAtom time_to_sample;

  bool composition_offset_present;
  MuTFFCompositionOffsetAtom composition_offset;

  bool composition_shift_least_greatest_present;
  MuTFFCompositionShiftLeastGreatestAtom composition_shift_least_greatest;

  bool sync_sample_present;
  MuTFFSyncSampleAtom sync_sample;

  bool partial_sync_sample_present;
  MuTFFPartialSyncSampleAtom partial_sync_sample;

  bool sample_to_chunk_present;
  MuTFFSampleToChunkAtom sample_to_chunk;

  bool sample_size_present;
  MuTFFSampleSizeAtom sample_size;

  bool chunk_offset_present;
  MuTFFChunkOffsetAtom chunk_offset;

  bool sample_dependency_flags_present;
  MuTFFSampleDependencyFlagsAtom sample_dependency_flags;
} MuTFFSampleTableAtom;

///
/// @brief Read a sample table atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_sample_table_atom(FILE *fd, size_t *n,
                                        MuTFFSampleTableAtom *out);

///
/// @brief Write a sample table atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_sample_table_atom(FILE *fd, size_t *n,
                                         const MuTFFSampleTableAtom *in);

///
/// @brief Video media header atom
/// @note MuTFFVideoMediaInformationAtom, MuTFFSoundMediaInformationAtom and
/// MuTFFBaseMediaInformationAtom all have type `minf`
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-25638
///
typedef struct {
  MuTFFVideoMediaInformationHeaderAtom video_media_information_header;

  MuTFFHandlerReferenceAtom handler_reference;

  bool data_information_present;
  MuTFFDataInformationAtom data_information;

  bool sample_table_present;
  MuTFFSampleTableAtom sample_table;
} MuTFFVideoMediaInformationAtom;

///
/// @brief Read a video media information atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_video_media_information_atom(
    FILE *fd, size_t *n, MuTFFVideoMediaInformationAtom *out);

///
/// @brief Write a video media information atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_video_media_information_atom(
    FILE *fd, size_t *n, const MuTFFVideoMediaInformationAtom *in);

///
/// @brief Sound media information header atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCFEAAI
///
typedef struct {
  uint8_t version;
  mutff_uint24_t flags;
  int16_t balance;
} MuTFFSoundMediaInformationHeaderAtom;

///
/// @brief Read a sound media information header atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_sound_media_information_header_atom(
    FILE *fd, size_t *n, MuTFFSoundMediaInformationHeaderAtom *out);

///
/// @brief Write a sound media information header atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_sound_media_information_header_atom(
    FILE *fd, size_t *n, const MuTFFSoundMediaInformationHeaderAtom *in);

///
/// @brief Sound media information atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-25647
///
typedef struct {
  MuTFFSoundMediaInformationHeaderAtom sound_media_information_header;

  MuTFFHandlerReferenceAtom handler_reference;

  bool data_information_present;
  MuTFFDataInformationAtom data_information;

  bool sample_table_present;
  MuTFFSampleTableAtom sample_table;
} MuTFFSoundMediaInformationAtom;

///
/// @brief Read a sound media information atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_sound_media_information_atom(
    FILE *fd, size_t *n, MuTFFSoundMediaInformationAtom *out);

///
/// @brief Write a sound media information atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_sound_media_information_atom(
    FILE *fd, size_t *n, const MuTFFSoundMediaInformationAtom *in);

///
/// @brief Base media info atom
/// @note Not to be confused with a [base media information
/// atom](MuTFFBaseMediaInformationAtom)
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCCHBFJ
///
typedef struct {
  uint8_t version;
  mutff_uint24_t flags;
  uint16_t graphics_mode;
  uint16_t opcolor[3];
  int16_t balance;
} MuTFFBaseMediaInfoAtom;

///
/// @brief Read a base media info atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_base_media_info_atom(FILE *fd, size_t *n,
                                           MuTFFBaseMediaInfoAtom *out);

///
/// @brief Write a base media info atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_base_media_info_atom(FILE *fd, size_t *n,
                                            const MuTFFBaseMediaInfoAtom *in);

///
/// @brief Text media information atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap3/qtff3.html#//apple_ref/doc/uid/TP40000939-CH205-SW90
///
typedef struct {
  MuTFFMatrix matrix_structure;
} MuTFFTextMediaInformationAtom;

///
/// @brief Read a text media information atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_text_media_information_atom(
    FILE *fd, size_t *n, MuTFFTextMediaInformationAtom *out);

///
/// @brief Write a text media information atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_text_media_information_atom(
    FILE *fd, size_t *n, const MuTFFTextMediaInformationAtom *in);

///
/// @brief Base media information header atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCIIDIA
///
typedef struct {
  MuTFFBaseMediaInfoAtom base_media_info;

  bool text_media_information_present;
  MuTFFTextMediaInformationAtom text_media_information;
} MuTFFBaseMediaInformationHeaderAtom;

///
/// @brief Read a base media information header atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_base_media_information_header_atom(
    FILE *fd, size_t *n, MuTFFBaseMediaInformationHeaderAtom *out);

///
/// @brief Write a base media information header atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_base_media_information_header_atom(
    FILE *fd, size_t *n, const MuTFFBaseMediaInformationHeaderAtom *in);

///
/// @brief Base media information atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCBJEBH
///
typedef struct {
  MuTFFBaseMediaInformationHeaderAtom base_media_information_header;
} MuTFFBaseMediaInformationAtom;

///
/// @brief Read a base media information atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_base_media_information_atom(
    FILE *fd, size_t *n, MuTFFBaseMediaInformationAtom *out);

///
/// @brief Write a base media information atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_base_media_information_atom(
    FILE *fd, size_t *n, const MuTFFBaseMediaInformationAtom *in);

///
/// @brief Media types
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap3/qtff3.html#//apple_ref/doc/uid/TP40000939-CH205-SW1
///
typedef enum {
  MuTFFMediaTypeVideo = MuTFF_FOURCC('v', 'i', 'd', 'e'),
  MuTFFMediaTypeSound = MuTFF_FOURCC('s', 'o', 'u', 'n'),
  MuTFFMediaTypeTimedMetadata = MuTFF_FOURCC('m', 'e', 't', 'a'),
  MuTFFMediaTypeTextMedia = MuTFF_FOURCC('t', 'e', 'x', 't'),
  MuTFFMediaTypeClosedCaptioningMedia = MuTFF_FOURCC('c', 'l', 'c', 'p'),
  MuTFFMediaTypeSubtitleMedia = MuTFF_FOURCC('s', 'b', 't', 'l'),
  MuTFFMediaTypeMusicMedia = MuTFF_FOURCC('m', 'u', 's', 'i'),
  MuTFFMediaTypeMPEG1Media = MuTFF_FOURCC('M', 'P', 'E', 'G'),
  MuTFFMediaTypeSpriteMedia = MuTFF_FOURCC('s', 'p', 'r', 't'),
  MuTFFMediaTypeTweenMedia = MuTFF_FOURCC('t', 'w', 'e', 'n'),
  MuTFFMediaType3DMedia = MuTFF_FOURCC('q', 'd', '3', 'd'),
  MuTFFMediaTypeStreamingMedia = MuTFF_FOURCC('s', 't', 'r', 'm'),
  MuTFFMediaTypeHintMedia = MuTFF_FOURCC('h', 'i', 'n', 't'),
  MuTFFMediaTypeVRMedia = MuTFF_FOURCC('q', 't', 'v', 'r'),
  MuTFFMediaTypePanoramaMedia = MuTFF_FOURCC('p', 'a', 'n', 'o'),
  MuTFFMediaTypeObjectMedia = MuTFF_FOURCC('o', 'b', 'j', 'e'),
} MuTFFMediaType;

///
/// @brief Media information types
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCHEIJG
///
typedef enum {
  MuTFFVideoMediaInformation,
  MuTFFSoundMediaInformation,
  MuTFFBaseMediaInformation,
} MuTFFMediaInformationType;

///
/// @brief Relates a MuTFFMediaType to its corresponding
/// MuTFFMediaInformationHeaderType
///
MuTFFMediaInformationType mutff_media_information_type(
    MuTFFMediaType media_type);

///
/// @brief Media atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCHFJFA
///
typedef struct {
  MuTFFMediaHeaderAtom media_header;

  bool extended_language_tag_present;
  MuTFFExtendedLanguageTagAtom extended_language_tag;

  bool handler_reference_present;
  MuTFFHandlerReferenceAtom handler_reference;

  bool media_information_present;
  MuTFFVideoMediaInformationAtom video_media_information;
  MuTFFSoundMediaInformationAtom sound_media_information;
  MuTFFBaseMediaInformationAtom base_media_information;

  bool user_data_present;
  MuTFFUserDataAtom user_data;
} MuTFFMediaAtom;

///
/// @brief Determine the type of a media atom
///
/// @param [out] out  The media type of the media atom
/// @param [in] atom  The atom to determine the type of
/// @return           The MuTFFError code
///
MuTFFError mutff_media_type(MuTFFMediaType *out, const MuTFFMediaAtom *atom);

/// @brief Read a media atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_media_atom(FILE *fd, size_t *n, MuTFFMediaAtom *out);

///
/// @brief Write a media atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_media_atom(FILE *fd, size_t *n,
                                  const MuTFFMediaAtom *in);

///
/// @brief Track atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCBEAIF
///
typedef struct {
  MuTFFTrackHeaderAtom track_header;

  MuTFFMediaAtom media;

  bool track_aperture_mode_dimensions_present;
  MuTFFTrackApertureModeDimensionsAtom track_aperture_mode_dimensions;

  bool clipping_present;
  MuTFFClippingAtom clipping;

  bool track_matte_present;
  MuTFFTrackMatteAtom track_matte;

  bool edit_present;
  MuTFFEditAtom edit;

  bool track_reference_present;
  MuTFFTrackReferenceAtom track_reference;

  bool track_exclude_from_autoselection_present;
  MuTFFTrackExcludeFromAutoselectionAtom track_exclude_from_autoselection;

  bool track_load_settings_present;
  MuTFFTrackLoadSettingsAtom track_load_settings;

  bool track_input_map_present;
  MuTFFTrackInputMapAtom track_input_map;

  bool user_data_present;
  MuTFFUserDataAtom user_data;
} MuTFFTrackAtom;

///
/// @brief Read a track atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_track_atom(FILE *fd, size_t *n, MuTFFTrackAtom *out);

///
/// @brief Write a track atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_track_atom(FILE *fd, size_t *n,
                                  const MuTFFTrackAtom *in);

///
/// @brief The maximum number of track atoms in a movie atom
///
#define MuTFF_MAX_TRACK_ATOMS 4U

///
/// @brief Movie atom
///
/// A single movie atom is required in the file. It describes how the file
/// should be played.
///
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-SW1
///
typedef struct {
  MuTFFMovieHeaderAtom movie_header;

  size_t track_count;
  MuTFFTrackAtom track[MuTFF_MAX_TRACK_ATOMS];

  bool clipping_present;
  MuTFFClippingAtom clipping;

  bool color_table_present;
  MuTFFColorTableAtom color_table;

  bool user_data_present;
  MuTFFUserDataAtom user_data;
} MuTFFMovieAtom;

///
/// @brief Read a movie atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read
/// @param [out] out The parsed atom
/// @return          The MuTFFError code
///
MuTFFError mutff_read_movie_atom(FILE *fd, size_t *n, MuTFFMovieAtom *out);

///
/// @brief Write a movie atom
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The atom
/// @return          The MuTFFError code
///
MuTFFError mutff_write_movie_atom(FILE *fd, size_t *n,
                                  const MuTFFMovieAtom *in);

#define MuTFF_MAX_MOVIE_DATA_ATOMS 4U
#define MuTFF_MAX_FREE_ATOMS 4U
#define MuTFF_MAX_SKIP_ATOMS 4U
#define MuTFF_MAX_WIDE_ATOMS 4U

///
/// @brief A QuickTime movie file
///
/// At the top level, the movie file should contain:
/// * Optionally a file type atom, always as the first atom. This is recommended
/// for new files.
/// * A single required movie atom
/// * Zero or more movie data atoms
/// * Optionally a preview atom
/// Optionally seperated by free, skip and wide atoms.
///
/// The order of the atoms is technically arbitrary excepting the file type
/// atom, however the above order is typical and recommended.
///
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap1/qtff1.html#//apple_ref/doc/uid/TP40000939-CH203-39025
///
typedef struct {
  bool file_type_present;
  MuTFFFileTypeAtom file_type;

  MuTFFMovieAtom movie;

  size_t movie_data_count;
  MuTFFMovieDataAtom movie_data[MuTFF_MAX_MOVIE_DATA_ATOMS];

  size_t free_count;
  MuTFFFreeAtom free[MuTFF_MAX_FREE_ATOMS];

  size_t skip_count;
  MuTFFSkipAtom skip[MuTFF_MAX_SKIP_ATOMS];

  size_t wide_count;
  MuTFFWideAtom wide[MuTFF_MAX_WIDE_ATOMS];

  bool preview_present;
  MuTFFPreviewAtom preview;
} MuTFFMovieFile;

///
/// @brief Read a QuickTime movie file
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes read
/// @param [out] out The parsed file
/// @return          The MuTFFError code
///
MuTFFError mutff_read_movie_file(FILE *fd, size_t *n, MuTFFMovieFile *out);

///
/// @brief Write a QuickTime movie file
///
/// @param [in] fd   The file descriptor
/// @param [out] n   The number of bytes written
/// @param [in] in   The file
/// @return          The MuTFFError code
///
MuTFFError mutff_write_movie_file(FILE *fd, size_t *n,
                                  const MuTFFMovieFile *in);

/// @} MuTFF

#endif  // MUTFF_H_
