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
/// @brief A generic error in the MuTFF library
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

///
/// @brief The size of a QuickTime atom
///
typedef uint32_t MuTFFAtomSize;

///
/// @brief The type of a QuickTime atom
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

///
/// @brief Time given in the Macintosh/HFS+ format
///
///        Time is specified as seconds elapsed since 1904-01-01T00:00:00
///
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap4/qtff4.html#//apple_ref/doc/uid/TP40000939-CH206-CJBIBAAE
///
typedef uint32_t MacTime;

///
/// @brief Time given in the QuickTime format
///
///        Time is specified as time units elapsed since the start of the movie.
///        Time units are context dependent, defined as 1 second over the time
///        scale in the current context.
///
typedef uint32_t QTTime;

///
/// @brief A QuickTime 3x3 matrix
///
///        Each element is a 32-bit type and the elements are stored in
///        row-major order.
///
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap4/qtff4.html#//apple_ref/doc/uid/TP40000939-CH206-BBCCEGCB
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCGFGJG
///
typedef float QTMatrix[3][3];

///
/// @brief The ID of a track in a QuickTime file
///
typedef uint32_t QTTrackID;

///
/// @brief The header + offset of a generic QuickTime atom
///
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
/// @brief The maximum number of compatible brands
///
#define MuTFF_MAX_COMPATIBLE_BRANDS 4

///
/// @brief QuickTime identifier for a file format
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
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCGFGJG
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
  char version;
  char flags[3];
  MacTime creation_time;
  MacTime modification_time;
  uint32_t time_scale;
  QTTime duration;
  uint32_t preferred_rate;
  uint16_t preferred_volume;
  char _reserved[10];
  QTMatrix matrix_structure;
  QTTime preview_time;
  QTTime preview_duration;
  QTTime poster_time;
  QTTime selection_time;
  QTTime selection_duration;
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
/// @brief Maximum size of the data in a clipping region atom
/// @see MuTFFClippingRegionAtom
///
#define MuTFF_MAX_CLIPPING_REGION_DATA_SIZE 16

///
/// @brief Clipping region atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCHDAIB
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
  uint16_t region_size;
  uint64_t region_boundary_box;
  char clipping_region_data[MuTFF_MAX_CLIPPING_REGION_DATA_SIZE];
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
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCIHBFG
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
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCBDJEB
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
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCCFFGD
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

///
/// @brief Track header atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCEIDFA
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
  char version;
  char flags[3];
  MacTime creation_time;
  MacTime modification_time;
  QTTrackID track_id;
  char _reserved_1[4];
  QTTime duration;
  char _reserved_2[8];
  uint16_t layer;
  uint16_t alternate_group;
  uint16_t volume;
  char _reserved_3[2];
  QTMatrix matrix_structure;
  uint32_t track_width;
  uint32_t track_height;
} MuTFFTrackHeaderAtom;

///
/// @brief Read a track header atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_track_header_atom(FILE *fd, MuTFFTrackHeaderAtom *out);

///
/// @brief Track clean aperture dimensions atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-SW3
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
  char version;
  char flags[3];
  uint32_t width;
  uint32_t height;
} MuTFFTrackCleanApertureDimensionsAtom;

///
/// @brief Read a track clean aperture dimensions atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_track_clean_aperture_dimensions_atom(
    FILE *fd, MuTFFTrackCleanApertureDimensionsAtom *out);

///
/// @brief Track production aperture dimensions atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-SW13
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
  char version;
  char flags[3];
  uint32_t width;
  uint32_t height;
} MuTFFTrackProductionApertureDimensionsAtom;

///
/// @brief Read a track production aperture dimensions atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_track_production_aperture_dimensions_atom(
    FILE *fd, MuTFFTrackProductionApertureDimensionsAtom *out);

///
/// @brief Track encoded pixels dimensions atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-SW14
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
  char version;
  char flags[3];
  uint32_t width;
  uint32_t height;
} MuTFFTrackEncodedPixelsDimensionsAtom;

///
/// @brief Read a track encoded pixels dimensions atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_track_encoded_pixels_dimensions_atom(
    FILE *fd, MuTFFTrackEncodedPixelsDimensionsAtom *out);

///
/// @brief Track aperture mode dimensions atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-SW15
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
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
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_track_aperture_mode_dimensions_atom(
    FILE *fd, MuTFFTrackApertureModeDimensionsAtom *out);

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
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_sample_description(FILE *fd, MuTFFSampleDescription *out);

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
  MuTFFAtomSize size;
  MuTFFAtomType type;
  char version;
  char flags[3];
  MuTFFSampleDescription matte_image_description_structure;
  size_t matte_data_len;
  char matte_data[MuTFF_MAX_MATTE_DATA_LEN];
} MuTFFCompressedMatteAtom;

///
/// @brief Read a compressed matte atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_compressed_matte_atom(FILE *fd,
                                            MuTFFCompressedMatteAtom *out);

///
/// @brief Track matte atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-25567
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
  MuTFFCompressedMatteAtom compressed_matte_atom;
} MuTFFTrackMatteAtom;

///
/// @brief Read a track matte atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_track_matte_atom(FILE *fd, MuTFFTrackMatteAtom *out);

///
/// @brief Entry an an edit list
/// @note Not an atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCGDIJF
///
typedef struct {
  QTTime track_duration;
  QTTime media_time;
  uint32_t media_rate;
} MuTFFEditListEntry;

///
/// @brief Read an edit list entry
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_edit_list_entry(FILE *fd, MuTFFEditListEntry *out);

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
  MuTFFAtomSize size;
  MuTFFAtomType type;
  char version;
  char flags[3];
  uint32_t number_of_entries;
  MuTFFEditListEntry edit_list_table[MuTFF_MAX_EDIT_LIST_ENTRIES];
} MuTFFEditListAtom;

///
/// @brief Read an edit list atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_edit_list_atom(FILE *fd, MuTFFEditListAtom *out);

///
/// @brief Edit atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCCFBEF
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
  MuTFFEditListAtom edit_list_atom;
} MuTFFEditAtom;

///
/// @brief Read an edit atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_edit_atom(FILE *fd, MuTFFEditAtom *out);

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
  MuTFFAtomSize size;
  MuTFFAtomType type;
  size_t track_id_count;
  QTTrackID track_ids[MuTFF_MAX_TRACK_REFERENCE_TYPE_TRACK_IDS];
} MuTFFTrackReferenceTypeAtom;

///
/// @brief Read a track reference type atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_track_reference_type_atom(
    FILE *fd, MuTFFTrackReferenceTypeAtom *out);

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
  MuTFFAtomSize size;
  MuTFFAtomType type;
  size_t track_reference_type_count;
  MuTFFTrackReferenceTypeAtom
      track_reference_type[MuTFF_MAX_TRACK_REFERENCE_TYPE_ATOMS];
} MuTFFTrackReferenceAtom;

/// @brief Read a track reference atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_track_reference_atom(FILE *fd,
                                           MuTFFTrackReferenceAtom *out);

///
/// @brief Track exclude from autoselection atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-SW47
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
} MuTFFTrackExcludeFromAutoselectionAtom;

/// @brief Read a track exclude from autoselection atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_track_exclude_from_autoselection_atom(
    FILE *fd, MuTFFTrackExcludeFromAutoselectionAtom *out);

///
/// @brief Track load settings atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCGIIFI
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
  QTTime preload_start_time;
  QTTime preload_duration;
  uint32_t preload_flags;
  uint32_t default_hints;
} MuTFFTrackLoadSettingsAtom;

/// @brief Read a track load settings atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_track_load_settings_atom(FILE *fd,
                                               MuTFFTrackLoadSettingsAtom *out);

///
/// @brief Input type atom
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
  uint32_t input_type;
} MuTFFInputTypeAtom;

/// @brief Read an input type atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_input_type_atom(FILE *fd, MuTFFInputTypeAtom *out);

///
/// @brief Object ID atom
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
  uint32_t object_id;
} MuTFFObjectIDAtom;

/// @brief Read an object ID atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_object_id_atom(FILE *fd, MuTFFObjectIDAtom *out);

///
/// @brief Track input atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCDJBFG
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
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
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_track_input_atom(FILE *fd, MuTFFTrackInputAtom *out);

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
  MuTFFAtomSize size;
  MuTFFAtomType type;
  size_t track_input_atom_count;
  MuTFFTrackInputAtom track_input_atoms[MuTFF_MAX_TRACK_INPUT_ATOMS];
} MuTFFTrackInputMapAtom;

///
/// @brief Read track input atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_track_input_map_atom(FILE *fd,
                                           MuTFFTrackInputMapAtom *out);

///
/// @brief Media header atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-25615
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
  char version;
  char flags[3];
  MacTime creation_time;
  MacTime modification_time;
  uint32_t time_scale;
  QTTime duration;
  uint16_t language;
  uint16_t quality;
} MuTFFMediaHeaderAtom;

///
/// @brief Read media header atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_media_header_atom(FILE *fd, MuTFFMediaHeaderAtom *out);

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
  MuTFFAtomSize size;
  MuTFFAtomType type;
  char version;
  char flags[3];
  char language_tag_string[MuTFF_MAX_LANGUAGE_TAG_LENGTH];
} MuTFFExtendedLanguageTagAtom;

///
/// @brief Read extended language tag atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_extended_language_tag_atom(
    FILE *fd, MuTFFExtendedLanguageTagAtom *out);

///
/// @brief Maximum component name length
/// @see MuTFFHandlerReferenceAtom
///
#define MuTFF_MAX_COMPONENT_NAME_LENGTH 16

///
/// @brief Handler reference atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCIBHFD
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
  char version;
  char flags[3];
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
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_handler_reference_atom(FILE *fd,
                                             MuTFFHandlerReferenceAtom *out);

///
/// @brief Video media information header atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCFDGIG
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
  char version;
  char flags[3];
  uint16_t graphics_mode;
  uint16_t opcolor[3];
} MuTFFVideoMediaInformationHeaderAtom;

/// @brief Read a video media information header atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_video_media_information_header_atom(
    FILE *fd, MuTFFVideoMediaInformationHeaderAtom *out);

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
  MuTFFAtomSize size;
  MuTFFAtomType type;
  char version;
  char flags[3];
  char data[MuTFF_MAX_DATA_REFERENCE_DATA_SIZE];
} MuTFFDataReference;

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
  MuTFFAtomSize size;
  MuTFFAtomType type;
  char version;
  char flags[3];
  uint32_t number_of_entries;
  MuTFFDataReference data_references[MuTFF_MAX_DATA_REFERENCES];
} MuTFFDataReferenceAtom;

///
/// @brief Read a data reference atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_data_reference_atom(FILE *fd,
                                          MuTFFDataReferenceAtom *out);

///
/// @brief Data information atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCIFAIC
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
  MuTFFDataReferenceAtom data_reference;
} MuTFFDataInformationAtom;

///
/// @brief Read a data information atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_data_information_atom(FILE *fd,
                                            MuTFFDataInformationAtom *out);

#define MuTFF_MAX_SAMPLE_DESCRIPTION_TABLE_LEN 8

///
/// @brief Sample description atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-25691
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
  char version;
  char flags[3];
  uint32_t number_of_entries;
  MuTFFSampleDescription
      sample_description_table[MuTFF_MAX_SAMPLE_DESCRIPTION_TABLE_LEN];
} MuTFFSampleDescriptionAtom;

///
/// @brief Read a sample description atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_sample_description_atom(FILE *fd,
                                              MuTFFSampleDescriptionAtom *out);

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
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_time_to_sample_table_entry(
    FILE *fd, MuTFFTimeToSampleTableEntry *out);

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
  MuTFFAtomSize size;
  MuTFFAtomType type;
  char version;
  char flags[3];
  uint32_t number_of_entries;
  MuTFFTimeToSampleTableEntry
      time_to_sample_table[MuTFF_MAX_TIME_TO_SAMPLE_TABLE_LEN];
} MuTFFTimeToSampleAtom;

///
/// @brief Read a time-to-sample atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_time_to_sample_atom(FILE *fd, MuTFFTimeToSampleAtom *out);

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
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_composition_offset_table_entry(
    FILE *fd, MuTFFCompositionOffsetTableEntry *out);

///
/// @brief Maximum length of the composition offset table
/// @see MuTFFCompositionOffsetAtom
///
#define MuTFF_MAX_COMPOSITION_OFFSET_TABLE_LEN 4

///
/// @brief Composition offset atom
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
  char version;
  char flags[3];
  uint32_t entry_count;
  MuTFFCompositionOffsetTableEntry
      composition_offset_table[MuTFF_MAX_COMPOSITION_OFFSET_TABLE_LEN];
} MuTFFCompositionOffsetAtom;

///
/// @brief Read a composition offset atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_composition_offset_atom(FILE *fd,
                                              MuTFFCompositionOffsetAtom *out);

///
/// @brief Composition shift least greatest atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-SW20
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
  char version;
  char flags[3];
  uint32_t composition_offset_to_display_offset_shift;
  uint32_t least_display_offset;
  QTTime display_start_time;
  QTTime display_end_time;
} MuTFFCompositionShiftLeastGreatestAtom;

///
/// @brief Read a composition shift least greatest atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_composition_shift_least_greatest_atom(
    FILE *fd, MuTFFCompositionShiftLeastGreatestAtom *out);

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
  MuTFFAtomSize size;
  MuTFFAtomType type;
  char version;
  char flags[3];
  uint32_t number_of_entries;
  uint32_t sync_sample_table[MuTFF_MAX_SYNC_SAMPLE_TABLE_LEN];
} MuTFFSyncSampleAtom;

///
/// @brief Read a sync sample atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_sync_sample_atom(FILE *fd, MuTFFSyncSampleAtom *out);

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
  MuTFFAtomSize size;
  MuTFFAtomType type;
  char version;
  char flags[3];
  uint32_t entry_count;
  uint32_t partial_sync_sample_table[MuTFF_MAX_PARTIAL_SYNC_SAMPLE_TABLE_LEN];
} MuTFFPartialSyncSampleAtom;

///
/// @brief Read a partial sync sample atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_partial_sync_sample_atom(FILE *fd,
                                               MuTFFPartialSyncSampleAtom *out);

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
  MuTFFAtomSize size;
  MuTFFAtomType type;
  char version;
  char flags[3];
  uint32_t number_of_entries;
  MuTFFSampleToChunkTableEntry
      sample_to_chunk_table[MuTFF_MAX_SAMPLE_TO_CHUNK_TABLE_LEN];
} MuTFFSampleToChunkAtom;

///
/// @brief Read a sample to chunk atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_sample_to_chunk_atom(FILE *fd,
                                           MuTFFSampleToChunkAtom *out);

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
  MuTFFAtomSize size;
  MuTFFAtomType type;
  char version;
  char flags[3];
  uint32_t sample_size;
  uint32_t number_of_entries;
  uint32_t sample_size_table[MuTFF_MAX_SAMPLE_SIZE_TABLE_LEN];
} MuTFFSampleSizeAtom;

///
/// @brief Read a sample size atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_sample_size_atom(FILE *fd, MuTFFSampleSizeAtom *out);

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
  MuTFFAtomSize size;
  MuTFFAtomType type;
  char version;
  char flags[3];
  uint32_t number_of_entries;
  uint32_t chunk_offset_table[MuTFF_MAX_CHUNK_OFFSET_TABLE_LEN];
} MuTFFChunkOffsetAtom;

///
/// @brief Read a chunk offset atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_chunk_offset_atom(FILE *fd, MuTFFChunkOffsetAtom *out);

#define MuTFF_MAX_SAMPLE_DEPENDENCY_FLAGS_TABLE_LEN 4

///
/// @brief Sample dependency flags atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-SW22
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
  char version;
  char flags;
  char sample_dependency_flags_table
      [MuTFF_MAX_SAMPLE_DEPENDENCY_FLAGS_TABLE_LEN];
} MuTFFSampleDependencyFlagsAtom;

///
/// @brief Read a sample dependency flags atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_sample_dependency_flags_atom(
    FILE *fd, MuTFFSampleDependencyFlagsAtom *out);

///
/// @brief Sample table atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCBFDFF
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
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
/// @return           Whether or not the atom was read successfully
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
  MuTFFAtomSize size;
  MuTFFAtomType type;
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
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_video_media_information_atom(
    FILE *fd, MuTFFVideoMediaInformationAtom *out);

///
/// @brief Sound media information header atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCFEAAI
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
  char version;
  char flags[3];
  int16_t balance;
  char _reserved[2];
} MuTFFSoundMediaInformationHeaderAtom;

///
/// @brief Read a sound media information header atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_sound_media_information_header_atom(
    FILE *fd, MuTFFSoundMediaInformationHeaderAtom *out);

///
/// @brief Sound media information atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-25647
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
  MuTFFSoundMediaInformationHeaderAtom sound_media_information_header;
  MuTFFDataInformationAtom data_information;
  MuTFFSampleTableAtom sample_table;
} MuTFFSoundMediaInformationAtom;

///
/// @brief Read a sound media information atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
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
  MuTFFAtomSize size;
  MuTFFAtomType type;
  char version;
  char flags[3];
  int16_t graphics_mode;
  uint16_t opcolor[3];
  int16_t balance;
  char _reserved[2];
} MuTFFBaseMediaInfoAtom;

///
/// @brief Read a base media info atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_base_media_info_atom(FILE *fd,
                                           MuTFFBaseMediaInfoAtom *out);

///
/// @brief Text media information atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap3/qtff3.html#//apple_ref/doc/uid/TP40000939-CH205-SW90
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
  QTMatrix matrix_structure;
} MuTFFTextMediaInformationAtom;

///
/// @brief Read a text media information atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_text_media_information_atom(
    FILE *fd, MuTFFTextMediaInformationAtom *out);

///
/// @brief Base media information header atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCIIDIA
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
  MuTFFBaseMediaInfoAtom base_media_info;
  MuTFFTextMediaInformationAtom text_media_information;
} MuTFFBaseMediaInformationHeaderAtom;

///
/// @brief Read a base media information header atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_base_media_information_header_atom(
    FILE *fd, MuTFFBaseMediaInformationHeaderAtom *out);

///
/// @brief Base media information atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCBJEBH
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
  MuTFFBaseMediaInformationHeaderAtom base_media_information_header;
} MuTFFBaseMediaInformationAtom;

///
/// @brief Read a base media information atom
///
/// @param [in] fd    The file descriptor to read from
/// @param [out] out  The parsed atom
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_base_media_information_atom(
    FILE *fd, MuTFFBaseMediaInformationAtom *out);

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

///
/// @brief Media atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCHFJFA
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
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
/// @return           Whether or not the atom was read successfully
///
MuTFFError mutff_read_media_atom(FILE *fd, MuTFFMediaAtom *out);

///
/// @brief Track atom
/// @see
/// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html#//apple_ref/doc/uid/TP40000939-CH204-BBCBEAIF
///
typedef struct {
  MuTFFAtomSize size;
  MuTFFAtomType type;
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
/// @return           Whether or not the atom was read successfully
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
