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

#define MuTFF_FOUR_C(in) ((in[0] << 24) + (in[1] << 16) + (in[2] << 8) + in[3])

///
/// @brief A 24-bit unsigned integer
///
typedef uint32_t mutff_uint24_t;

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
/// @brief A generic error in the MuTFF library
///
typedef enum {
  MuTFFErrorIOError = -1,
  MuTFFErrorEOF = -2,
  MuTFFErrorBadFormat = -3,
  MuTFFErrorOutOfMemory = -4,
} MuTFFError;

///
/// @brief The header + offset of a generic QuickTime atom
///
typedef struct {
  uint32_t size;
  uint32_t type;
} MuTFFAtomHeader;

///
/// @brief Read the header of an atom
///
/// The current file offset must be at the start of the atom.
///
/// @param [in] fd    The file descriptor
/// @param [out] out  Output
/// @return           If the read was successful, the number of bytes read.
///                   If there was an error, the MuTFFError code.
///
MuTFFError mutff_peek_atom_header(FILE *fd, MuTFFAtomHeader *out);

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
/// @param [in] fd    The file descriptor
/// @param [out] out  Output
/// @return           If a rectangle was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_quickdraw_rect(FILE *fd, MuTFFQuickDrawRect *out);

///
/// @brief Write a QuickDraw rectangle
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the rectangle was written successfully, then the number
///                   of bytes written, otherwise the (negative) MuTFFError
///                   code.
///
MuTFFError mutff_write_quickdraw_rect(FILE *fd, const MuTFFQuickDrawRect *in);

///
/// @brief Maximum size of the additional data in a QuickDraw region
///
#define MuTFF_MAX_QUICKDRAW_REGION_DATA_SIZE 8

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
/// @param [in] fd    The file descriptor
/// @param [out] out  Output
/// @return           If an atom was read successfully, then the number of bytes
///                   read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_quickdraw_region(FILE *fd, MuTFFQuickDrawRegion *out);

///
/// @brief Write a QuickDraw region
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the region was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_quickdraw_region(FILE *fd,
                                        const MuTFFQuickDrawRegion *in);

///
/// @brief The maximum number of compatible brands
///
#define MuTFF_MAX_COMPATIBLE_BRANDS 4

///
/// @brief File type compatibility atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/MuTFF/MuTFFChap1/mutff1.html#//apple_ref/doc/uid/TP40000939-CH203-CJBCBIFF
///
typedef struct {
  uint32_t size;
  uint32_t type;
  uint32_t major_brand;
  uint32_t minor_version;
  size_t compatible_brands_count;
  uint32_t compatible_brands[MuTFF_MAX_COMPATIBLE_BRANDS];
} MuTFFFileTypeCompatibilityAtom;

///
/// @brief Read a file type compatibility atom
///
/// @param [in] fd    The file descriptor
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_file_type_compatibility_atom(
    FILE *fd, MuTFFFileTypeCompatibilityAtom *out);

///
/// @brief Write a file type compatibility atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_file_type_compatibility_atom(
    FILE *fd, const MuTFFFileTypeCompatibilityAtom *in);

typedef struct {
  uint32_t size;
  uint32_t type;
} MuTFFMovieDataAtom;

///
/// @brief Read a movie data atom
///
/// @param [in] fd    The file descriptor
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_movie_data_atom(FILE *fd, MuTFFMovieDataAtom *out);

///
/// @brief Write a movie data atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_movie_data_atom(FILE *fd, const MuTFFMovieDataAtom *in);

///
/// @brief Free (unused) space atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/MuTFF/MuTFFChap1/mutff1.html#//apple_ref/doc/uid/TP40000939-CH203-55464
///
typedef struct {
  uint32_t size;
  uint32_t type;
} MuTFFFreeAtom;

///
/// @brief Read a free atom
///
/// @param [in] fd    The file descriptor
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_free_atom(FILE *fd, MuTFFFreeAtom *out);

///
/// @brief Write a free atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_free_atom(FILE *fd, const MuTFFFreeAtom *in);

///
/// @brief Skip (unused) space atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/MuTFF/MuTFFChap1/mutff1.html#//apple_ref/doc/uid/TP40000939-CH203-55464
///
typedef struct {
  uint32_t size;
  uint32_t type;
} MuTFFSkipAtom;

///
/// @brief Read a skip atom
///
/// @param [in] fd    The file descriptor
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_skip_atom(FILE *fd, MuTFFSkipAtom *out);

///
/// @brief Write a skip atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_skip_atom(FILE *fd, const MuTFFSkipAtom *in);

///
/// @brief Wide (reserved) space atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/MuTFF/MuTFFChap1/mutff1.html#//apple_ref/doc/uid/TP40000939-CH203-55464
///
typedef struct {
  uint32_t size;
  uint32_t type;
} MuTFFWideAtom;

///
/// @brief Read a wide atom
///
/// @param [in] fd    The file descriptor
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_wide_atom(FILE *fd, MuTFFWideAtom *out);

///
/// @brief Write a wide atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_wide_atom(FILE *fd, const MuTFFWideAtom *in);

///
/// @brief Preview atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/MuTFF/MuTFFChap1/mutff1.html#//apple_ref/doc/uid/TP40000939-CH203-38240
///
typedef struct {
  uint32_t size;
  uint32_t type;
  uint32_t modification_time;
  uint16_t version;
  uint32_t atom_type;
  uint16_t atom_index;
} MuTFFPreviewAtom;

///
/// @brief Read a preview atom
///
/// @param [in] fd    The file descriptor
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_preview_atom(FILE *fd, MuTFFPreviewAtom *out);

///
/// @brief Write a preview atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_preview_atom(FILE *fd, const MuTFFPreviewAtom *in);

///
/// @brief Movie header atom.
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCGFGJG
///
typedef struct {
  uint32_t size;
  uint32_t type;
  uint8_t version;
  mutff_uint24_t flags;
  uint32_t creation_time;
  uint32_t modification_time;
  uint32_t time_scale;
  uint32_t duration;
  mutff_q16_16_t preferred_rate;
  mutff_q8_8_t preferred_volume;
  char _reserved[10];
  uint32_t matrix_structure[3][3];
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
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_movie_header_atom(FILE *fd, MuTFFMovieHeaderAtom *out);

///
/// @brief Write a movie header atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_movie_header_atom(FILE *fd,
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
  uint32_t size;
  uint32_t type;
  MuTFFQuickDrawRegion region;
} MuTFFClippingRegionAtom;

///
/// @brief Read a clipping region atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_clipping_region_atom(FILE *fd,
                                           MuTFFClippingRegionAtom *out);

///
/// @brief Write a clipping region atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_clipping_region_atom(FILE *fd,
                                            const MuTFFClippingRegionAtom *in);

///
/// @brief Clipping atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCIHBFG
///
typedef struct {
  uint32_t size;
  uint32_t type;
  MuTFFClippingRegionAtom clipping_region;
} MuTFFClippingAtom;

///
/// @brief Read a clipping atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_clipping_atom(FILE *fd, MuTFFClippingAtom *out);

///
/// @brief Write a clipping atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_clipping_atom(FILE *fd, const MuTFFClippingAtom *in);

///
/// @brief The maximum number of entries in the color table
///
#define MuTFF_MAX_COLOR_TABLE_SIZE 16

///
/// @brief Color table atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCBDJEB
///
typedef struct {
  uint32_t size;
  uint32_t type;
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
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_color_table_atom(FILE *fd, MuTFFColorTableAtom *out);

///
/// @brief Write a color table atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_color_table_atom(FILE *fd,
                                        const MuTFFColorTableAtom *in);

///
/// @brief The maximum size of the data in an entry of a user data list
///
#define MuTFF_MAX_USER_DATA_ENTRY_SIZE 64

typedef struct {
  uint32_t size;
  uint32_t type;
  char data[MuTFF_MAX_USER_DATA_ENTRY_SIZE];
} MuTFFUserDataListEntry;

///
/// @brief Read an entry in a user data list
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed entry
/// @return           If the entry was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_user_data_list_entry(FILE *fd,
                                           MuTFFUserDataListEntry *out);

///
/// @brief Write an entry in a user data list
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the entry was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_user_data_list_entry(FILE *fd,
                                            const MuTFFUserDataListEntry *in);

///
/// @brief The maximum number of entries in the user data list
///
#define MuTFF_MAX_USER_DATA_ITEMS 16

///
/// @brief User data atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCCFFGD
///
typedef struct {
  uint32_t size;
  uint32_t type;
  MuTFFUserDataListEntry user_data_list[MuTFF_MAX_USER_DATA_ITEMS];
} MuTFFUserDataAtom;

///
/// @brief Read a user data atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_user_data_atom(FILE *fd, MuTFFUserDataAtom *out);

///
/// @brief Write a user data atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_user_data_atom(FILE *fd, const MuTFFUserDataAtom *in);

///
/// @brief Track header atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCEIDFA
///
typedef struct {
  uint32_t size;
  uint32_t type;
  uint8_t version;
  mutff_uint24_t flags;
  uint32_t creation_time;
  uint32_t modification_time;
  uint32_t track_id;
  char _reserved_1[4];
  uint32_t duration;
  char _reserved_2[8];
  uint16_t layer;
  uint16_t alternate_group;
  mutff_q8_8_t volume;
  char _reserved_3[2];
  uint32_t matrix_structure[3][3];
  mutff_q16_16_t track_width;
  mutff_q16_16_t track_height;
} MuTFFTrackHeaderAtom;

///
/// @brief Read a track header atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_track_header_atom(FILE *fd, MuTFFTrackHeaderAtom *out);

///
/// @brief Write a track header atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_track_header_atom(FILE *fd,
                                         const MuTFFTrackHeaderAtom *in);

///
/// @brief Track clean aperture dimensions atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-SW3
///
typedef struct {
  uint32_t size;
  uint32_t type;
  uint8_t version;
  mutff_uint24_t flags;
  mutff_q16_16_t width;
  mutff_q16_16_t height;
} MuTFFTrackCleanApertureDimensionsAtom;

///
/// @brief Read a track clean aperture dimensions atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_track_clean_aperture_dimensions_atom(
    FILE *fd, MuTFFTrackCleanApertureDimensionsAtom *out);

///
/// @brief Write a track clean aperture dimensions atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_track_clean_aperture_dimensions_atom(
    FILE *fd, const MuTFFTrackCleanApertureDimensionsAtom *in);

///
/// @brief Track production aperture dimensions atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-SW13
///
typedef struct {
  uint32_t size;
  uint32_t type;
  uint8_t version;
  mutff_uint24_t flags;
  mutff_q16_16_t width;
  mutff_q16_16_t height;
} MuTFFTrackProductionApertureDimensionsAtom;

///
/// @brief Read a track production aperture dimensions atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_track_production_aperture_dimensions_atom(
    FILE *fd, MuTFFTrackProductionApertureDimensionsAtom *out);

///
/// @brief Write a track production aperture dimensions atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_track_production_aperture_dimensions_atom(
    FILE *fd, const MuTFFTrackProductionApertureDimensionsAtom *in);

///
/// @brief Track encoded pixels dimensions atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-SW14
///
typedef struct {
  uint32_t size;
  uint32_t type;
  uint8_t version;
  mutff_uint24_t flags;
  mutff_q16_16_t width;
  mutff_q16_16_t height;
} MuTFFTrackEncodedPixelsDimensionsAtom;

///
/// @brief Read a track encoded pixels dimensions atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_track_encoded_pixels_dimensions_atom(
    FILE *fd, MuTFFTrackEncodedPixelsDimensionsAtom *out);

///
/// @brief Write a track encoded pixels dimensions atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_track_encoded_pixels_dimensions_atom(
    FILE *fd, const MuTFFTrackEncodedPixelsDimensionsAtom *in);

///
/// @brief Track aperture mode dimensions atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-SW15
///
typedef struct {
  uint32_t size;
  uint32_t type;
  MuTFFTrackCleanApertureDimensionsAtom track_clean_aperture_dimension;
  MuTFFTrackProductionApertureDimensionsAtom
      track_production_aperture_dimension;
  MuTFFTrackEncodedPixelsDimensionsAtom track_encoded_pixels_dimension;
} MuTFFTrackApertureModeDimensionsAtom;

///
/// @brief Read a track aperture mode dimensions atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_track_aperture_mode_dimensions_atom(
    FILE *fd, MuTFFTrackApertureModeDimensionsAtom *out);

///
/// @brief Write a track aperture mode dimensions atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_track_aperture_mode_dimensions_atom(
    FILE *fd, const MuTFFTrackApertureModeDimensionsAtom *in);

///
/// @brief The maximum length of the format-specific data in a sample
///        description
/// @see MuTFFSampleDescription
///
#define MuTFF_MAX_SAMPLE_DESCRIPTION_DATA_LEN 16

///
/// @brief A sample description
/// @note This is not an atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-61112
///
typedef struct {
  uint32_t size;
  uint32_t data_format;
  char _reserved[6];
  uint16_t data_reference_index;
  char additional_data[MuTFF_MAX_SAMPLE_DESCRIPTION_DATA_LEN];
} MuTFFSampleDescription;

///
/// @brief Read a sample description
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed description
/// @return           If the description was read successfully, then the number
///                   of bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_sample_description(FILE *fd, MuTFFSampleDescription *out);

///
/// @brief Write a sample description
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the description was written successfully, then the
///                   number of bytes written, otherwise the (negative)
///                   MuTFFError code.
///
MuTFFError mutff_write_sample_description(FILE *fd,
                                          const MuTFFSampleDescription *in);

///
/// @brief The maximum length of the data in a compressed matte atom
/// @see MuTFFCompressedMatteAtom
///
#define MuTFF_MAX_MATTE_DATA_LEN 16

///
/// @brief Compressed matte atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-25573
///
typedef struct {
  uint32_t size;
  uint32_t type;
  uint8_t version;
  mutff_uint24_t flags;
  MuTFFSampleDescription matte_image_description_structure;
  size_t matte_data_len;
  char matte_data[MuTFF_MAX_MATTE_DATA_LEN];
} MuTFFCompressedMatteAtom;

///
/// @brief Read a compressed matte atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_compressed_matte_atom(FILE *fd,
                                            MuTFFCompressedMatteAtom *out);

///
/// @brief Write a compressed matte atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_compressed_matte_atom(
    FILE *fd, const MuTFFCompressedMatteAtom *in);

///
/// @brief Track matte atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-25567
///
typedef struct {
  uint32_t size;
  uint32_t type;
  MuTFFCompressedMatteAtom compressed_matte_atom;
} MuTFFTrackMatteAtom;

///
/// @brief Read a track matte atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_track_matte_atom(FILE *fd, MuTFFTrackMatteAtom *out);

///
/// @brief Write a track matte atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_track_matte_atom(FILE *fd,
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
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_edit_list_entry(FILE *fd, MuTFFEditListEntry *out);

///
/// @brief Write a movie data atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_edit_list_entry(FILE *fd, const MuTFFEditListEntry *in);

///
/// @brief The maximum number of entries in an edit list atom
/// @see MuTFFEditListAtom
///
#define MuTFF_MAX_EDIT_LIST_ENTRIES 8

///
/// @brief Edit list atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCGDIJF
///
typedef struct {
  uint32_t size;
  uint32_t type;
  uint8_t version;
  mutff_uint24_t flags;
  uint32_t number_of_entries;
  MuTFFEditListEntry edit_list_table[MuTFF_MAX_EDIT_LIST_ENTRIES];
} MuTFFEditListAtom;

///
/// @brief Read an edit list atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_edit_list_atom(FILE *fd, MuTFFEditListAtom *out);

///
/// @brief Write an edit list atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_edit_list_atom(FILE *fd, const MuTFFEditListAtom *in);

///
/// @brief Edit atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCCFBEF
///
typedef struct {
  uint32_t size;
  uint32_t type;
  MuTFFEditListAtom edit_list_atom;
} MuTFFEditAtom;

///
/// @brief Read an edit atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_edit_atom(FILE *fd, MuTFFEditAtom *out);

///
/// @brief Write an edit atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_edit_atom(FILE *fd, const MuTFFEditAtom *in);

///
/// @brief The maximum track IDs in a track reference type atom
/// @see MuTFFTrackReferenceTypeAtom
///
#define MuTFF_MAX_TRACK_REFERENCE_TYPE_TRACK_IDS 4

///
/// @brief Track reference type atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCGDBAF
///
typedef struct {
  uint32_t size;
  uint32_t type;
  size_t track_id_count;
  uint32_t track_ids[MuTFF_MAX_TRACK_REFERENCE_TYPE_TRACK_IDS];
} MuTFFTrackReferenceTypeAtom;

///
/// @brief Read a track reference type atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_track_reference_type_atom(
    FILE *fd, MuTFFTrackReferenceTypeAtom *out);

///
/// @brief Write a track reference type atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_track_reference_type_atom(
    FILE *fd, const MuTFFTrackReferenceTypeAtom *in);

///
/// @brief The maximum reference type atoms in a track reference atom
/// @see MuTFFTrackReferenceAtom
///
#define MuTFF_MAX_TRACK_REFERENCE_TYPE_ATOMS 4

///
/// @brief Track reference atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCGDBAF
///
typedef struct {
  uint32_t size;
  uint32_t type;
  size_t track_reference_type_count;
  MuTFFTrackReferenceTypeAtom
      track_reference_type[MuTFF_MAX_TRACK_REFERENCE_TYPE_ATOMS];
} MuTFFTrackReferenceAtom;

/// @brief Read a track reference atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_track_reference_atom(FILE *fd,
                                           MuTFFTrackReferenceAtom *out);

///
/// @brief Write a track reference atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_track_reference_atom(FILE *fd,
                                            const MuTFFTrackReferenceAtom *in);

///
/// @brief Track exclude from autoselection atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-SW47
///
typedef struct {
  uint32_t size;
  uint32_t type;
} MuTFFTrackExcludeFromAutoselectionAtom;

/// @brief Read a track exclude from autoselection atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_track_exclude_from_autoselection_atom(
    FILE *fd, MuTFFTrackExcludeFromAutoselectionAtom *out);

///
/// @brief Write a track exclude from autoselection atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_track_exclude_from_autoselection_atom(
    FILE *fd, const MuTFFTrackExcludeFromAutoselectionAtom *in);

///
/// @brief Track load settings atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCGIIFI
///
typedef struct {
  uint32_t size;
  uint32_t type;
  uint32_t preload_start_time;
  uint32_t preload_duration;
  uint32_t preload_flags;
  uint32_t default_hints;
} MuTFFTrackLoadSettingsAtom;

/// @brief Read a track load settings atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_track_load_settings_atom(FILE *fd,
                                               MuTFFTrackLoadSettingsAtom *out);

///
/// @brief Write a track load settings atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_track_load_settings_atom(
    FILE *fd, const MuTFFTrackLoadSettingsAtom *in);

///
/// @brief Input type atom
///
typedef struct {
  uint32_t size;
  uint32_t type;
  uint32_t input_type;
} MuTFFInputTypeAtom;

/// @brief Read an input type atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_input_type_atom(FILE *fd, MuTFFInputTypeAtom *out);

///
/// @brief Write an input type atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_input_type_atom(FILE *fd, const MuTFFInputTypeAtom *in);

///
/// @brief Object ID atom
///
typedef struct {
  uint32_t size;
  uint32_t type;
  uint32_t object_id;
} MuTFFObjectIDAtom;

/// @brief Read an object ID atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_object_id_atom(FILE *fd, MuTFFObjectIDAtom *out);

///
/// @brief Write an object ID atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_object_id_atom(FILE *fd, const MuTFFObjectIDAtom *in);

///
/// @brief Track input atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCDJBFG
///
typedef struct {
  uint32_t size;
  uint32_t type;
  uint32_t atom_id;
  char _reserved_1[2];
  uint16_t child_count;
  char _reserved_2[4];
  MuTFFInputTypeAtom input_type_atom;
  MuTFFObjectIDAtom object_id_atom;
} MuTFFTrackInputAtom;

/// @brief Read a track input atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_track_input_atom(FILE *fd, MuTFFTrackInputAtom *out);

///
/// @brief Write a track input atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_track_input_atom(FILE *fd,
                                        const MuTFFTrackInputAtom *in);

///
/// @brief Maximum entries in a track input map
/// @see MuTFFTrackInputMapAtom
///
#define MuTFF_MAX_TRACK_INPUT_ATOMS 2

///
/// @brief Track input map atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCDJBFG
///
typedef struct {
  uint32_t size;
  uint32_t type;
  size_t track_input_atom_count;
  MuTFFTrackInputAtom track_input_atoms[MuTFF_MAX_TRACK_INPUT_ATOMS];
} MuTFFTrackInputMapAtom;

///
/// @brief Read track input map atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_track_input_map_atom(FILE *fd,
                                           MuTFFTrackInputMapAtom *out);

///
/// @brief Write a track input map atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_track_input_map_atom(FILE *fd,
                                            const MuTFFTrackInputMapAtom *in);

///
/// @brief Media header atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-25615
///
typedef struct {
  uint32_t size;
  uint32_t type;
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
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_media_header_atom(FILE *fd, MuTFFMediaHeaderAtom *out);

///
/// @brief Write a media header atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_media_header_atom(FILE *fd,
                                         const MuTFFMediaHeaderAtom *in);

///
/// @brief Maximum language tag length
/// @see MuTFFExtendedLanguageTagAtom
///
#define MuTFF_MAX_LANGUAGE_TAG_LENGTH 8

///
/// @brief Extended language tag atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-SW16
///
typedef struct {
  uint32_t size;
  uint32_t type;
  uint8_t version;
  mutff_uint24_t flags;
  char language_tag_string[MuTFF_MAX_LANGUAGE_TAG_LENGTH];
} MuTFFExtendedLanguageTagAtom;

///
/// @brief Read extended language tag atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_extended_language_tag_atom(
    FILE *fd, MuTFFExtendedLanguageTagAtom *out);

///
/// @brief Write an extended language tag atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_extended_language_tag_atom(
    FILE *fd, const MuTFFExtendedLanguageTagAtom *in);

///
/// @brief Maximum component name length
/// @see MuTFFHandlerReferenceAtom
///
#define MuTFF_MAX_COMPONENT_NAME_LENGTH 24

///
/// @brief Handler reference atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCIBHFD
///
typedef struct {
  uint32_t size;
  uint32_t type;
  uint8_t version;
  mutff_uint24_t flags;
  uint32_t component_type;
  uint32_t component_subtype;
  uint32_t component_manufacturer;
  uint32_t component_flags;
  uint32_t component_flags_mask;
  char component_name[MuTFF_MAX_COMPONENT_NAME_LENGTH];
} MuTFFHandlerReferenceAtom;

/// @brief Read a handler reference atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_handler_reference_atom(FILE *fd,
                                             MuTFFHandlerReferenceAtom *out);

///
/// @brief Write a handler reference atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_handler_reference_atom(
    FILE *fd, const MuTFFHandlerReferenceAtom *in);

///
/// @brief Video media information header atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCFDGIG
///
typedef struct {
  uint32_t size;
  uint32_t type;
  uint8_t version;
  mutff_uint24_t flags;
  uint16_t graphics_mode;
  uint16_t opcolor[3];
} MuTFFVideoMediaInformationHeaderAtom;

/// @brief Read a video media information header atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_video_media_information_header_atom(
    FILE *fd, MuTFFVideoMediaInformationHeaderAtom *out);

///
/// @brief Write a video media information header atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_video_media_information_header_atom(
    FILE *fd, const MuTFFVideoMediaInformationHeaderAtom *in);

///
/// @brief The maximum size of the data in a data reference
/// @see MuTFFDataReference
///
#define MuTFF_MAX_DATA_REFERENCE_DATA_SIZE 16

///
/// @brief Data reference
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCGGDAE
///
typedef struct {
  uint32_t size;
  uint32_t type;
  uint8_t version;
  mutff_uint24_t flags;
  char data[MuTFF_MAX_DATA_REFERENCE_DATA_SIZE];
} MuTFFDataReference;

///
/// @brief Read a data reference
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed reference
/// @return           If the reference was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_data_reference(FILE *fd, MuTFFDataReference *out);

///
/// @brief Write a data reference
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_data_reference(FILE *fd, const MuTFFDataReference *in);

///
/// @brief The maximum number of data references in a data reference atom
/// @see MuTFFDataReferenceAtom
///
#define MuTFF_MAX_DATA_REFERENCES 4

///
/// @brief Data reference atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCGGDAE
///
typedef struct {
  uint32_t size;
  uint32_t type;
  uint8_t version;
  mutff_uint24_t flags;
  uint32_t number_of_entries;
  MuTFFDataReference data_references[MuTFF_MAX_DATA_REFERENCES];
} MuTFFDataReferenceAtom;

///
/// @brief Read a data reference atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_data_reference_atom(FILE *fd,
                                          MuTFFDataReferenceAtom *out);

///
/// @brief Write a data reference atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_data_reference_atom(FILE *fd,
                                           const MuTFFDataReferenceAtom *in);

///
/// @brief Data information atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCIFAIC
///
typedef struct {
  uint32_t size;
  uint32_t type;
  MuTFFDataReferenceAtom data_reference;
} MuTFFDataInformationAtom;

///
/// @brief Read a data information atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_data_information_atom(FILE *fd,
                                            MuTFFDataInformationAtom *out);

///
/// @brief Write a data information atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_data_information_atom(
    FILE *fd, const MuTFFDataInformationAtom *in);

#define MuTFF_MAX_SAMPLE_DESCRIPTION_TABLE_LEN 8

///
/// @brief Sample description atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-25691
///
typedef struct {
  uint32_t size;
  uint32_t type;
  uint8_t version;
  mutff_uint24_t flags;
  uint32_t number_of_entries;
  MuTFFSampleDescription
      sample_description_table[MuTFF_MAX_SAMPLE_DESCRIPTION_TABLE_LEN];
} MuTFFSampleDescriptionAtom;

///
/// @brief Read a sample description atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_sample_description_atom(FILE *fd,
                                              MuTFFSampleDescriptionAtom *out);

///
/// @brief Write a sample description atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_sample_description_atom(
    FILE *fd, const MuTFFSampleDescriptionAtom *in);

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
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_time_to_sample_table_entry(
    FILE *fd, MuTFFTimeToSampleTableEntry *out);

///
/// @brief Write a time-to-sample table entry
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_time_to_sample_table_entry(
    FILE *fd, const MuTFFTimeToSampleTableEntry *in);

///
/// @brief Maximum number of entries in a time-to-sample atom
/// @see MuTFFTimeToSampleAtom
///
#define MuTFF_MAX_TIME_TO_SAMPLE_TABLE_LEN 4

///
/// @brief Time-to-sample atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCGFJII
///
typedef struct {
  uint32_t size;
  uint32_t type;
  uint8_t version;
  mutff_uint24_t flags;
  uint32_t number_of_entries;
  MuTFFTimeToSampleTableEntry
      time_to_sample_table[MuTFF_MAX_TIME_TO_SAMPLE_TABLE_LEN];
} MuTFFTimeToSampleAtom;

///
/// @brief Read a time-to-sample atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_time_to_sample_atom(FILE *fd, MuTFFTimeToSampleAtom *out);

///
/// @brief Write a time-to-sample atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_time_to_sample_atom(FILE *fd,
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
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_composition_offset_table_entry(
    FILE *fd, MuTFFCompositionOffsetTableEntry *out);

///
/// @brief Write a composition offset table entry
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_composition_offset_table_entry(
    FILE *fd, const MuTFFCompositionOffsetTableEntry *in);

///
/// @brief Maximum length of the composition offset table
/// @see MuTFFCompositionOffsetAtom
///
#define MuTFF_MAX_COMPOSITION_OFFSET_TABLE_LEN 4

///
/// @brief Composition offset atom
///
typedef struct {
  uint32_t size;
  uint32_t type;
  uint8_t version;
  mutff_uint24_t flags;
  uint32_t entry_count;
  MuTFFCompositionOffsetTableEntry
      composition_offset_table[MuTFF_MAX_COMPOSITION_OFFSET_TABLE_LEN];
} MuTFFCompositionOffsetAtom;

///
/// @brief Read a composition offset atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_composition_offset_atom(FILE *fd,
                                              MuTFFCompositionOffsetAtom *out);

///
/// @brief Write a composition offset atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_composition_offset_atom(
    FILE *fd, const MuTFFCompositionOffsetAtom *in);

///
/// @brief Composition shift least greatest atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-SW20
///
typedef struct {
  uint32_t size;
  uint32_t type;
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
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_composition_shift_least_greatest_atom(
    FILE *fd, MuTFFCompositionShiftLeastGreatestAtom *out);

///
/// @brief Write a composition shift least greatest atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_composition_shift_least_greatest_atom(
    FILE *fd, const MuTFFCompositionShiftLeastGreatestAtom *in);

///
/// @brief Maximum length of the sync sample table
///
#define MuTFF_MAX_SYNC_SAMPLE_TABLE_LEN 8

///
/// @brief Sync sample atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-25701
///
typedef struct {
  uint32_t size;
  uint32_t type;
  uint8_t version;
  mutff_uint24_t flags;
  uint32_t number_of_entries;
  uint32_t sync_sample_table[MuTFF_MAX_SYNC_SAMPLE_TABLE_LEN];
} MuTFFSyncSampleAtom;

///
/// @brief Read a sync sample atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_sync_sample_atom(FILE *fd, MuTFFSyncSampleAtom *out);

///
/// @brief Write a sync sample atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_sync_sample_atom(FILE *fd,
                                        const MuTFFSyncSampleAtom *in);

///
/// @brief Maximum length of the partial sync sample table
///
#define MuTFF_MAX_PARTIAL_SYNC_SAMPLE_TABLE_LEN 4

///
/// @brief Partial sync sample atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-SW21
///
typedef struct {
  uint32_t size;
  uint32_t type;
  uint8_t version;
  mutff_uint24_t flags;
  uint32_t entry_count;
  uint32_t partial_sync_sample_table[MuTFF_MAX_PARTIAL_SYNC_SAMPLE_TABLE_LEN];
} MuTFFPartialSyncSampleAtom;

///
/// @brief Read a partial sync sample atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_partial_sync_sample_atom(FILE *fd,
                                               MuTFFPartialSyncSampleAtom *out);

///
/// @brief Write a partial sync sample atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_partial_sync_sample_atom(
    FILE *fd, const MuTFFPartialSyncSampleAtom *in);

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
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed entry
/// @return           If the entry was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_sample_to_chunk_table_entry(
    FILE *fd, MuTFFSampleToChunkTableEntry *out);

///
/// @brief Write a sample-to-chunk table entry
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_sample_to_chunk_table_entry(
    FILE *fd, const MuTFFSampleToChunkTableEntry *in);

///
/// @brief Maximum length of the sample-to-chunk table
/// @see MuTFFSampleToChunkAtom
///
#define MuTFF_MAX_SAMPLE_TO_CHUNK_TABLE_LEN 4

///
/// @brief Sample to chunk atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-25706
///
typedef struct {
  uint32_t size;
  uint32_t type;
  uint8_t version;
  mutff_uint24_t flags;
  uint32_t number_of_entries;
  MuTFFSampleToChunkTableEntry
      sample_to_chunk_table[MuTFF_MAX_SAMPLE_TO_CHUNK_TABLE_LEN];
} MuTFFSampleToChunkAtom;

///
/// @brief Read a sample to chunk atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_sample_to_chunk_atom(FILE *fd,
                                           MuTFFSampleToChunkAtom *out);

///
/// @brief Write a sample-to-chunk atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_sample_to_chunk_atom(FILE *fd,
                                            const MuTFFSampleToChunkAtom *in);

///
/// @brief Maximum number of entries in a sample size table
///
#define MuTFF_MAX_SAMPLE_SIZE_TABLE_LEN 4

///
/// @brief Sample size atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-25710
///
typedef struct {
  uint32_t size;
  uint32_t type;
  uint8_t version;
  mutff_uint24_t flags;
  uint32_t sample_size;
  uint32_t number_of_entries;
  uint32_t sample_size_table[MuTFF_MAX_SAMPLE_SIZE_TABLE_LEN];
} MuTFFSampleSizeAtom;

///
/// @brief Read a sample size atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_sample_size_atom(FILE *fd, MuTFFSampleSizeAtom *out);

///
/// @brief Write a sample size atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_sample_size_atom(FILE *fd,
                                        const MuTFFSampleSizeAtom *in);

///
/// @brief Maximum length of the chunk offset table
///
#define MuTFF_MAX_CHUNK_OFFSET_TABLE_LEN 4

///
/// @brief Chunk offset atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-25715
///
typedef struct {
  uint32_t size;
  uint32_t type;
  uint8_t version;
  mutff_uint24_t flags;
  uint32_t number_of_entries;
  uint32_t chunk_offset_table[MuTFF_MAX_CHUNK_OFFSET_TABLE_LEN];
} MuTFFChunkOffsetAtom;

///
/// @brief Read a chunk offset atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_chunk_offset_atom(FILE *fd, MuTFFChunkOffsetAtom *out);

///
/// @brief Write a chunk offset atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_chunk_offset_atom(FILE *fd,
                                         const MuTFFChunkOffsetAtom *in);

#define MuTFF_MAX_SAMPLE_DEPENDENCY_FLAGS_TABLE_LEN 4

///
/// @brief Sample dependency flags atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-SW22
///
typedef struct {
  uint32_t size;
  uint32_t type;
  uint8_t version;
  mutff_uint24_t flags;
  unsigned char sample_dependency_flags_table
      [MuTFF_MAX_SAMPLE_DEPENDENCY_FLAGS_TABLE_LEN];
} MuTFFSampleDependencyFlagsAtom;

///
/// @brief Read a sample dependency flags atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_sample_dependency_flags_atom(
    FILE *fd, MuTFFSampleDependencyFlagsAtom *out);

///
/// @brief Write a sample dependency flags atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_sample_dependency_flags_atom(
    FILE *fd, const MuTFFSampleDependencyFlagsAtom *in);

///
/// @brief Sample table atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCBFDFF
///
typedef struct {
  uint32_t size;
  uint32_t type;
  MuTFFSampleDescriptionAtom sample_description;
  MuTFFTimeToSampleAtom time_to_sample;
  MuTFFCompositionOffsetAtom composition_offset;
  MuTFFCompositionShiftLeastGreatestAtom composition_shift_least_greatest;
  MuTFFSyncSampleAtom sync_sample;
  MuTFFPartialSyncSampleAtom partial_sync_sample;
  MuTFFSampleToChunkAtom sample_to_chunk;
  MuTFFSampleSizeAtom sample_size;
  MuTFFChunkOffsetAtom chunk_offset;
  MuTFFSampleDependencyFlagsAtom sample_dependency_flags;
} MuTFFSampleTableAtom;

///
/// @brief Read a sample table atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_sample_table_atom(FILE *fd, MuTFFSampleTableAtom *out);

///
/// @brief Video media header atom
/// @note MuTFFVideoMediaInformationAtom, MuTFFSoundMediaInformationAtom and
/// MuTFFBaseMediaInformationAtom all have type `minf`
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-25638
///
typedef struct {
  uint32_t size;
  uint32_t type;
  MuTFFVideoMediaInformationHeaderAtom video_media_information_header;
  MuTFFHandlerReferenceAtom handler_reference;
  MuTFFDataInformationAtom data_information;
  MuTFFSampleTableAtom sample_table;
} MuTFFVideoMediaInformationAtom;

///
/// @brief Read a video media information atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_video_media_information_atom(
    FILE *fd, MuTFFVideoMediaInformationAtom *out);

///
/// @brief Sound media information header atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCFEAAI
///
typedef struct {
  uint32_t size;
  uint32_t type;
  uint8_t version;
  mutff_uint24_t flags;
  int16_t balance;
  char _reserved[2];
} MuTFFSoundMediaInformationHeaderAtom;

///
/// @brief Read a sound media information header atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_sound_media_information_header_atom(
    FILE *fd, MuTFFSoundMediaInformationHeaderAtom *out);

///
/// @brief Write a sound media information header atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_sound_media_information_header_atom(
    FILE *fd, const MuTFFSoundMediaInformationHeaderAtom *in);

///
/// @brief Sound media information atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-25647
///
typedef struct {
  uint32_t size;
  uint32_t type;
  MuTFFSoundMediaInformationHeaderAtom sound_media_information_header;
  MuTFFHandlerReferenceAtom handler_reference;
  MuTFFDataInformationAtom data_information;
  MuTFFSampleTableAtom sample_table;
} MuTFFSoundMediaInformationAtom;

///
/// @brief Read a sound media information atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_sound_media_information_atom(
    FILE *fd, MuTFFSoundMediaInformationAtom *out);

///
/// @brief Base media info atom
/// @note Not to be confused with a [base media information
/// atom](MuTFFBaseMediaInformationAtom)
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCCHBFJ
///
typedef struct {
  uint32_t size;
  uint32_t type;
  uint8_t version;
  mutff_uint24_t flags;
  uint16_t graphics_mode;
  uint16_t opcolor[3];
  int16_t balance;
  char _reserved[2];
} MuTFFBaseMediaInfoAtom;

///
/// @brief Read a base media info atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_base_media_info_atom(FILE *fd,
                                           MuTFFBaseMediaInfoAtom *out);

///
/// @brief Write a base media info atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_base_media_info_atom(FILE *fd,
                                            const MuTFFBaseMediaInfoAtom *in);

///
/// @brief Text media information atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap3/qtff3.html#//apple_ref/doc/uid/TP40000939-CH205-SW90
///
typedef struct {
  uint32_t size;
  uint32_t type;
  uint32_t matrix_structure[3][3];
} MuTFFTextMediaInformationAtom;

///
/// @brief Read a text media information atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_text_media_information_atom(
    FILE *fd, MuTFFTextMediaInformationAtom *out);

///
/// @brief Write a text media information atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_text_media_information_atom(
    FILE *fd, const MuTFFTextMediaInformationAtom *in);

///
/// @brief Base media information header atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCIIDIA
///
typedef struct {
  uint32_t size;
  uint32_t type;
  MuTFFBaseMediaInfoAtom base_media_info;
  MuTFFTextMediaInformationAtom text_media_information;
} MuTFFBaseMediaInformationHeaderAtom;

///
/// @brief Read a base media information header atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_base_media_information_header_atom(
    FILE *fd, MuTFFBaseMediaInformationHeaderAtom *out);

///
/// @brief Write a base media information header atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_base_media_information_header_atom(
    FILE *fd, const MuTFFBaseMediaInformationHeaderAtom *in);

///
/// @brief Base media information atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCBJEBH
///
typedef struct {
  uint32_t size;
  uint32_t type;
  MuTFFBaseMediaInformationHeaderAtom base_media_information_header;
} MuTFFBaseMediaInformationAtom;

///
/// @brief Read a base media information atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_base_media_information_atom(
    FILE *fd, MuTFFBaseMediaInformationAtom *out);

///
/// @brief Write a base media information atom
///
/// @param [in] fd    The file descriptor
/// @param [in] in    Input
/// @return           If the atom was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_base_media_information_atom(
    FILE *fd, const MuTFFBaseMediaInformationAtom *in);

///
/// @brief A media information atom
///
/// Can be one of: MuTFFVideoMediaInformationAtom,
/// MuTFFSoundMediaInformationAtom, MuTFFBaseMediaInformationAtom.
/// All three have the same QTFF type `minf`
///
typedef union {
  MuTFFVideoMediaInformationAtom video;
  MuTFFSoundMediaInformationAtom sound;
  MuTFFBaseMediaInformationAtom base;
} MuTFFMediaInformationAtom;

/// @brief Read a media information atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_media_information_atom(FILE *fd,
                                             MuTFFMediaInformationAtom *out);

///
/// @brief Media atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCHFJFA
///
typedef struct {
  uint32_t size;
  uint32_t type;
  MuTFFMediaHeaderAtom media_header;
  MuTFFExtendedLanguageTagAtom extended_language_tag;
  MuTFFHandlerReferenceAtom handler_reference;
  MuTFFMediaInformationAtom media_information;
  MuTFFUserDataAtom user_data;
} MuTFFMediaAtom;

/// @brief Read a media atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_media_atom(FILE *fd, MuTFFMediaAtom *out);

///
/// @brief Track atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCBEAIF
///
typedef struct {
  uint32_t size;
  uint32_t type;
  MuTFFTrackHeaderAtom track_header;
  MuTFFTrackApertureModeDimensionsAtom track_aperture_mode_dimensions;
  MuTFFClippingAtom clipping;
  MuTFFTrackMatteAtom track_matte;
  MuTFFEditAtom edit;
  MuTFFTrackReferenceAtom track_reference;
  MuTFFTrackExcludeFromAutoselectionAtom track_exclude_from_autoselection;
  MuTFFTrackLoadSettingsAtom track_load_settings;
  MuTFFTrackInputMapAtom track_input_map;
  MuTFFMediaAtom media;
  MuTFFUserDataAtom user_data;
} MuTFFTrackAtom;

///
/// @brief Read a track atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_track_atom(FILE *fd, MuTFFTrackAtom *out);

///
/// @brief The maximum number of track atoms in a movie atom
///
#define MuTFF_MAX_TRACK_ATOMS 4

///
/// @brief Movie atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-SW1
///
typedef struct {
  uint32_t size;
  uint32_t type;
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
/// @return           If the atom was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
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
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap1/qtff1.html#//apple_ref/doc/uid/TP40000939-CH203-39025
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
/// @return           If the file was read successfully, then the number of
///                   bytes read, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_read_movie_file(FILE *fd, MuTFFMovieFile *out);

///
/// @brief Write a QuickTime movie file
///
/// @param [in] fd    The file descriptor to write to
/// @param [in] in    The file
/// @return           If the file was written successfully, then the number of
///                   bytes written, otherwise the (negative) MuTFFError code.
///
MuTFFError mutff_write_movie_file(FILE *fd, MuTFFMovieFile *in);

/// @} MuTFF

#endif  // MUTFF_H_
