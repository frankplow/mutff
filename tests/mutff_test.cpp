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

TEST_F(MuTFFTest, FileTypeCompatibilityAtom) {
  MuTFFFileTypeCompatibilityAtom file_type_compatibility_atom;
  fseek(fd, 0, SEEK_SET);
  const MuTFFError err = mutff_read_file_type_compatibility_atom(
      fd, &file_type_compatibility_atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(MuTFF_ATOM_ID(file_type_compatibility_atom.type),
            MuTFF_ATOM_ID("ftyp"));
  EXPECT_EQ(file_type_compatibility_atom.size, 0x14);
  EXPECT_EQ(file_type_compatibility_atom.major_brand, MuTFF_ATOM_ID("qt  "));
  EXPECT_EQ(file_type_compatibility_atom.minor_version, 0x00000200);
  EXPECT_EQ(file_type_compatibility_atom.compatible_brands_count, 1);
  EXPECT_EQ(MuTFF_ATOM_ID(file_type_compatibility_atom.compatible_brands[0]),
            MuTFF_ATOM_ID("qt  "));
}

TEST_F(MuTFFTest, MovieDataAtom) {
  MuTFFMovieDataAtom movie_data_atom;
  fseek(fd, 28, SEEK_SET);
  const MuTFFError err = mutff_read_movie_data_atom(fd, &movie_data_atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(MuTFF_ATOM_ID(movie_data_atom.type), MuTFF_ATOM_ID("mdat"));
  EXPECT_EQ(movie_data_atom.size, 28302);
}

TEST_F(MuTFFTest, WideAtom) {
  MuTFFWideAtom wide_atom;
  fseek(fd, 20, SEEK_SET);
  const MuTFFError err = mutff_read_wide_atom(fd, &wide_atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(MuTFF_ATOM_ID(wide_atom.type), MuTFF_ATOM_ID("wide"));
  EXPECT_EQ(wide_atom.size, 8);
}

TEST_F(MuTFFTest, MovieAtom) {
  MuTFFMovieAtom movie_atom;
  fseek(fd, 28330, SEEK_SET);
  const MuTFFError err = mutff_read_movie_atom(fd, &movie_atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(movie_atom.size, 0x02c2);
  EXPECT_EQ(MuTFF_ATOM_ID(movie_atom.type), MuTFF_ATOM_ID("moov"));

  EXPECT_EQ(movie_atom.movie_header.size, 108);

  EXPECT_EQ(movie_atom.track_count, 1);
  EXPECT_EQ(movie_atom.track[0].size, 557);

  EXPECT_EQ(movie_atom.user_data.size, 33);
}

TEST_F(MuTFFTest, MovieHeaderAtom) {
  MuTFFMovieHeaderAtom movie_header_atom;
  fseek(fd, 28338, SEEK_SET);
  const MuTFFError err = mutff_read_movie_header_atom(fd, &movie_header_atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(movie_header_atom.size, 0x6c);
  EXPECT_EQ(MuTFF_ATOM_ID(movie_header_atom.type), MuTFF_ATOM_ID("mvhd"));
  EXPECT_EQ(movie_header_atom.version, 0);
  EXPECT_EQ(movie_header_atom.flags[0], 0);
  EXPECT_EQ(movie_header_atom.flags[1], 0);
  EXPECT_EQ(movie_header_atom.flags[2], 0);
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
}
