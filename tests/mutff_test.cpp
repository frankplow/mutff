#include <gtest/gtest.h>
#include <stdio.h>
#include <string.h>

extern "C" {
#include "mutff.h"
}

class MuTFFTest : public ::testing::Test {
 protected:
  FILE *fd;
  size_t size;

  void SetUp() override { fd = fopen("test.mov", "rb"); }

  void TearDown() override { fclose(fd); }
};

TEST_F(MuTFFTest, MovieFile) {
  MuTFFMovieFile movie_file;
  const MuTFFError err = mutff_read_movie_file(fd, &movie_file);
  ASSERT_EQ(err, MuTFFErrorNone) << "Error parsing movie file";

  EXPECT_EQ(movie_file.file_type_compatibility_count, 1);
  EXPECT_EQ(movie_file.file_type_compatibility[0].size, 0x14);
  EXPECT_EQ(movie_file.movie_count, 1);
  EXPECT_EQ(movie_file.movie[0].size, 706);
  EXPECT_EQ(movie_file.movie_data_count, 1);
  EXPECT_EQ(movie_file.movie_data[0].size, 28302);
  EXPECT_EQ(movie_file.free_count, 0);
  EXPECT_EQ(movie_file.skip_count, 0);
  EXPECT_EQ(movie_file.wide_count, 1);
  EXPECT_EQ(movie_file.wide[0].size, 8);
  EXPECT_EQ(movie_file.preview_count, 0);
}

TEST_F(MuTFFTest, MovieAtom) {
  const size_t offset = 28330;
  MuTFFMovieAtom movie_atom;
  fseek(fd, offset, SEEK_SET);
  const MuTFFError err = mutff_read_movie_atom(fd, &movie_atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(movie_atom.size, 0x02c2);
  EXPECT_EQ(MuTFF_FOUR_C(movie_atom.type), MuTFF_FOUR_C("moov"));
  EXPECT_EQ(movie_atom.movie_header.size, 108);
  EXPECT_EQ(movie_atom.track_count, 1);
  EXPECT_EQ(movie_atom.track[0].size, 557);
  EXPECT_EQ(movie_atom.user_data.size, 33);

  EXPECT_EQ(ftell(fd), offset + movie_atom.size);
}

TEST_F(MuTFFTest, MovieHeaderAtom) {
  const size_t offset = 28338;
  MuTFFMovieHeaderAtom movie_header_atom;
  fseek(fd, offset, SEEK_SET);
  const MuTFFError err = mutff_read_movie_header_atom(fd, &movie_header_atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(movie_header_atom.size, 0x6c);
  EXPECT_EQ(MuTFF_FOUR_C(movie_header_atom.type), MuTFF_FOUR_C("mvhd"));
  EXPECT_EQ(movie_header_atom.version_flags.version, 0);
  EXPECT_EQ(movie_header_atom.version_flags.flags, 0);
  EXPECT_EQ(movie_header_atom.creation_time, 0);
  EXPECT_EQ(movie_header_atom.modification_time, 0);
  EXPECT_EQ(movie_header_atom.time_scale, 1000);
  EXPECT_EQ(movie_header_atom.duration, 1167);
  EXPECT_EQ(movie_header_atom.preferred_rate, 0x00010000);
  EXPECT_EQ(movie_header_atom.preferred_volume, 0x0100);
  EXPECT_EQ(movie_header_atom.preferred_volume, 0x0100);
  // @TODO: test matrix_structure
  // 00  00  00  00 a
  // 00  00  00  00 b
  // 00  00  00  01 u
  // 00  00  00  00 c
  // 00  00  00  00 d
  // 00  00  00  00 v
  // 00  00  00  01 x
  // 00  00  00  00 y
  // 00  00  00  00 w
  EXPECT_EQ(movie_header_atom.preview_time, 0);
  EXPECT_EQ(movie_header_atom.preview_duration, 0);
  EXPECT_EQ(movie_header_atom.poster_time, 0);
  EXPECT_EQ(movie_header_atom.selection_time, 0);
  EXPECT_EQ(movie_header_atom.current_time, 0);
  EXPECT_EQ(movie_header_atom.next_track_id, 2);

  EXPECT_EQ(ftell(fd), offset + movie_header_atom.size);
}

TEST_F(MuTFFTest, FileTypeCompatibilityAtom) {
  const size_t offset = 0;
  MuTFFFileTypeCompatibilityAtom file_type_compatibility_atom;
  fseek(fd, offset, SEEK_SET);
  const MuTFFError err = mutff_read_file_type_compatibility_atom(
      fd, &file_type_compatibility_atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(MuTFF_FOUR_C(file_type_compatibility_atom.type),
            MuTFF_FOUR_C("ftyp"));
  EXPECT_EQ(file_type_compatibility_atom.size, 0x14);
  EXPECT_EQ(file_type_compatibility_atom.major_brand, MuTFF_FOUR_C("qt  "));
  EXPECT_EQ(MuTFF_FOUR_C(file_type_compatibility_atom.minor_version),
            MuTFF_FOUR_C(((char[]){00, 00, 02, 00})));
  EXPECT_EQ(file_type_compatibility_atom.compatible_brands_count, 1);
  EXPECT_EQ(MuTFF_FOUR_C(file_type_compatibility_atom.compatible_brands[0]),
            MuTFF_FOUR_C("qt  "));

  EXPECT_EQ(ftell(fd), offset + file_type_compatibility_atom.size);
}

TEST_F(MuTFFTest, MovieDataAtom) {
  const size_t offset = 28;
  MuTFFMovieDataAtom movie_data_atom;
  fseek(fd, offset, SEEK_SET);
  const MuTFFError err = mutff_read_movie_data_atom(fd, &movie_data_atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(MuTFF_FOUR_C(movie_data_atom.type), MuTFF_FOUR_C("mdat"));
  EXPECT_EQ(movie_data_atom.size, 28302);

  EXPECT_EQ(ftell(fd), offset + movie_data_atom.size);
}

TEST_F(MuTFFTest, WideAtom) {
  const size_t offset = 20;
  MuTFFWideAtom wide_atom;
  fseek(fd, offset, SEEK_SET);
  const MuTFFError err = mutff_read_wide_atom(fd, &wide_atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(MuTFF_FOUR_C(wide_atom.type), MuTFF_FOUR_C("wide"));
  EXPECT_EQ(wide_atom.size, 8);

  EXPECT_EQ(ftell(fd), offset + wide_atom.size);
}

TEST_F(MuTFFTest, FixedAtomSizes) {
  EXPECT_EQ(sizeof(MuTFFPreviewAtom), 20);
  EXPECT_EQ(sizeof(MuTFFMovieHeaderAtom), 108);
  EXPECT_EQ(sizeof(MuTFFTrackHeaderAtom), 92);
  EXPECT_EQ(sizeof(MuTFFTrackCleanApertureDimensionsAtom), 20);
  EXPECT_EQ(sizeof(MuTFFTrackProductionApertureDimensionsAtom), 20);
  EXPECT_EQ(sizeof(MuTFFTrackEncodedPixelsDimensionsAtom), 20);
  EXPECT_EQ(sizeof(MuTFFTrackApertureModeDimensionsAtom), 68);
  EXPECT_EQ(sizeof(MuTFFEditListEntry), 12);
  EXPECT_EQ(sizeof(MuTFFTrackExcludeFromAutoselectionAtom), 8);
  EXPECT_EQ(sizeof(MuTFFTrackLoadSettingsAtom), 24);
  EXPECT_EQ(sizeof(MuTFFInputTypeAtom), 12);
  EXPECT_EQ(sizeof(MuTFFMediaHeaderAtom), 32);
  EXPECT_EQ(sizeof(MuTFFVideoMediaInformationHeaderAtom), 20);
  EXPECT_EQ(sizeof(MuTFFTimeToSampleTableEntry), 8);
  EXPECT_EQ(sizeof(MuTFFCompositionOffsetTableEntry), 8);
  EXPECT_EQ(sizeof(MuTFFCompositionShiftLeastGreatestAtom), 28);
  EXPECT_EQ(sizeof(MuTFFSampleToChunkTableEntry), 12);
  EXPECT_EQ(sizeof(MuTFFSoundMediaInformationHeaderAtom), 16);
  EXPECT_EQ(sizeof(MuTFFBaseMediaInfoAtom), 24);
  EXPECT_EQ(sizeof(MuTFFTextMediaInformationAtom), 44);
  EXPECT_EQ(sizeof(MuTFFBaseMediaInformationHeaderAtom), 76);
  EXPECT_EQ(sizeof(MuTFFBaseMediaInformationAtom), 84);
}
