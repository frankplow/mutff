#include <gtest/gtest.h>
#include <stdio.h>
#include <string.h>

extern "C" {
#include "mutff.h"
}

class TestMov : public ::testing::Test {
 protected:
  FILE *fd;
  size_t size;

  // @TODO: check test.mov opened correctly
  void SetUp() override { fd = fopen("test.mov", "rb"); }

  void TearDown() override { fclose(fd); }
};

TEST_F(TestMov, MovieFile) {
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

TEST_F(TestMov, MovieAtom) {
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

TEST_F(TestMov, MovieHeaderAtom) {
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

TEST_F(TestMov, FileTypeCompatibilityAtom) {
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

// @TODO: Add tests for special sizes
TEST_F(TestMov, MovieDataAtom) {
  const size_t offset = 28;
  MuTFFMovieDataAtom movie_data_atom;
  fseek(fd, offset, SEEK_SET);
  const MuTFFError err = mutff_read_movie_data_atom(fd, &movie_data_atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(MuTFF_FOUR_C(movie_data_atom.type), MuTFF_FOUR_C("mdat"));
  EXPECT_EQ(movie_data_atom.size, 28302);

  EXPECT_EQ(ftell(fd), offset + movie_data_atom.size);
}

TEST_F(TestMov, WideAtom) {
  const size_t offset = 20;
  MuTFFWideAtom wide_atom;
  fseek(fd, offset, SEEK_SET);
  const MuTFFError err = mutff_read_wide_atom(fd, &wide_atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(MuTFF_FOUR_C(wide_atom.type), MuTFF_FOUR_C("wide"));
  EXPECT_EQ(wide_atom.size, 8);

  EXPECT_EQ(ftell(fd), offset + wide_atom.size);
}

TEST(MuTFF, ReadUserDataAtom) {
  MuTFFError err;
  MuTFFUserDataAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 28;
  // clang-format off
  char data[data_size] = {
      0x00, 0x00, 0x00, data_size,  // size
      'u',  'd',  't',  'a',        // type
      0x00, 0x00, 0x00, 0x0c,       // user_data_list[0].size
      'a',  'b',  'c',  'd',        // user_data_list[0].type
      'e',  'f',  'g',  'h',        // user_data_list[0].data
      0x00, 0x00, 0x00, 0x08,       // user_data_list[1].size
      'i',  'j',  'k',  'l',        // user_data_list[1].type
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_user_data_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("udta"));
  EXPECT_EQ(atom.user_data_list[0].size, 12);
  EXPECT_EQ(MuTFF_FOUR_C(atom.user_data_list[0].type), MuTFF_FOUR_C("abcd"));
  EXPECT_EQ(atom.user_data_list[1].size, 8);
  EXPECT_EQ(MuTFF_FOUR_C(atom.user_data_list[1].type), MuTFF_FOUR_C("ijkl"));
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadColorTableAtom) {
  MuTFFError err;
  MuTFFColorTableAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 32;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,                     // size
    'c',  't',  'a',  'b',                           // type
    0x00, 0x01, 0x02, 0x03,                          // seed
    0x00, 0x01,                                      // flags
    0x00, 0x01,                                      // color table size
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,  // color table[0]
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,  // color table[1]
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_color_table_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("ctab"));
  EXPECT_EQ(atom.color_table_seed, 0x00010203);
  EXPECT_EQ(atom.color_table_flags, 0x0001);
  EXPECT_EQ(atom.color_table_size, 0x0001);
  EXPECT_EQ(atom.color_array[0][0], 0x0001);
  EXPECT_EQ(atom.color_array[0][1], 0x0203);
  EXPECT_EQ(atom.color_array[0][2], 0x0405);
  EXPECT_EQ(atom.color_array[0][3], 0x0607);
  EXPECT_EQ(atom.color_array[1][0], 0x1011);
  EXPECT_EQ(atom.color_array[1][1], 0x1213);
  EXPECT_EQ(atom.color_array[1][2], 0x1415);
  EXPECT_EQ(atom.color_array[1][3], 0x1617);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadClippingAtom) {
  MuTFFError err;
  MuTFFClippingAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 26;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,                     // size
    'c', 'l', 'i', 'p',                              // type
    0x00, 0x00, 0x00, data_size - 8,                 // region size
    'c', 'r', 'g', 'n',                              // region type
    0x00, 0x0a,                                      // region region size
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,  // region boundary box
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_clipping_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("clip"));
  EXPECT_EQ(atom.clipping_region.size, data_size - 8);
  EXPECT_EQ(MuTFF_FOUR_C(atom.clipping_region.type), MuTFF_FOUR_C("crgn"));
  EXPECT_EQ(atom.clipping_region.region.size, 0x000a);
  EXPECT_EQ(atom.clipping_region.region.rect.top, 0x0001);
  EXPECT_EQ(atom.clipping_region.region.rect.left, 0x0203);
  EXPECT_EQ(atom.clipping_region.region.rect.bottom, 0x0405);
  EXPECT_EQ(atom.clipping_region.region.rect.right, 0x0607);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadClippingRegionAtom) {
  MuTFFError err;
  MuTFFClippingRegionAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 18;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,                     // size
    'c', 'r', 'g', 'n',                              // type
    0x00, 0x0a,                                      // region size
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,  // region boundary box
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_clipping_region_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("crgn"));
  EXPECT_EQ(atom.region.size, 0x000a);
  EXPECT_EQ(atom.region.rect.top, 0x0001);
  EXPECT_EQ(atom.region.rect.left, 0x0203);
  EXPECT_EQ(atom.region.rect.bottom, 0x0405);
  EXPECT_EQ(atom.region.rect.right, 0x0607);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadMovieHeaderAtom) {
  MuTFFError err;
  MuTFFMovieHeaderAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 108;
  // clang-format off
  char data[data_size] = {
      0x00, 0x00, 0x00, data_size,  // size
      'm',  'v',  'h',  'd',        // type
      0x01,                         // version
      0x01, 0x02, 0x03,             // flags
      0x01, 0x02, 0x03, 0x04,       // creation_time
      0x01, 0x02, 0x03, 0x04,       // modification_time
      0x01, 0x02, 0x03, 0x04,       // time_scale
      0x01, 0x02, 0x03, 0x04,       // duration
      0x01, 0x02, 0x03, 0x04,       // preferred_rate
      0x01, 0x02,                   // preferred_volume
      0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, // reserved
      0x01, 0x02, 0x03, 0x04,
      0x05, 0x06, 0x07, 0x08,
      0x09, 0x0a, 0x0b, 0x0c,
      0x0d, 0x0e, 0x0f, 0x10,
      0x11, 0x12, 0x13, 0x14,
      0x15, 0x16, 0x17, 0x18,
      0x19, 0x1a, 0x1b, 0x1c,
      0x1d, 0x1e, 0x1f, 0x20,
      0x21, 0x22, 0x23, 0x24,       // matrix_strucuture
      0x01, 0x02, 0x03, 0x04,       // preview_time
      0x01, 0x02, 0x03, 0x04,       // preview_duration
      0x01, 0x02, 0x03, 0x04,       // poster_time
      0x01, 0x02, 0x03, 0x04,       // selection_duration
      0x01, 0x02, 0x03, 0x04,       // selection_time
      0x01, 0x02, 0x03, 0x04,       // current_time
      0x01, 0x02, 0x03, 0x04,       // next_track_id
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_movie_header_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("mvhd"));
  EXPECT_EQ(atom.version_flags.version, 0x01);
  EXPECT_EQ(atom.version_flags.flags, 0x010203);
  EXPECT_EQ(atom.creation_time, 0x01020304);
  EXPECT_EQ(atom.modification_time, 0x01020304);
  EXPECT_EQ(atom.time_scale, 0x01020304);
  EXPECT_EQ(atom.duration, 0x01020304);
  EXPECT_EQ(atom.preferred_rate, 0x01020304);
  EXPECT_EQ(atom.preferred_volume, 0x0102);
  for (size_t j = 0; j < 3; ++j) {
    for (size_t i = 0; i < 3; ++i) {
      const uint32_t exp_base = (3 * j + i) * 4 + 1;
      const uint32_t exp = (exp_base << 24) + ((exp_base + 1) << 16) +
                           ((exp_base + 2) << 8) + (exp_base + 3);
      EXPECT_EQ(atom.matrix_structure[j][i], exp);
    }
  }
  EXPECT_EQ(atom.preview_time, 0x01020304);
  EXPECT_EQ(atom.preview_duration, 0x01020304);
  EXPECT_EQ(atom.poster_time, 0x01020304);
  EXPECT_EQ(atom.selection_time, 0x01020304);
  EXPECT_EQ(atom.selection_duration, 0x01020304);
  EXPECT_EQ(atom.current_time, 0x01020304);
  EXPECT_EQ(atom.next_track_id, 0x01020304);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadPreviewAtom) {
  MuTFFError err;
  MuTFFPreviewAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 20;
  char data[data_size] = {
      0x00, 0x00, 0x00, data_size,  // size
      'p',  'n',  'o',  't',        // type
      0x01, 0x02, 0x03, 0x04,       // modification_time
      0x01, 0x02,                   // version
      'a',  'b',  'c',  'd',        // atom_type
      0x01, 0x02,                   // atom_index
  };
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_preview_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("pnot"));
  EXPECT_EQ(atom.modification_time, 0x01020304);
  EXPECT_EQ(atom.version, 0x0102);
  EXPECT_EQ(MuTFF_FOUR_C(atom.atom_type), MuTFF_FOUR_C("abcd"));
  EXPECT_EQ(atom.atom_index, 0x0102);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadWideAtom) {
  MuTFFError err;
  MuTFFWideAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 16;
  char data[data_size] = {
      0x00, 0x00, 0x00, data_size,  // size
      'w',  'i',  'd',  'e',        // type
      0x00, 0x00, 0x00, 0x00,       // data
      0x00, 0x00, 0x00, 0x00,
  };
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_wide_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, 0x10);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("wide"));
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadSkipAtom) {
  MuTFFError err;
  MuTFFSkipAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 16;
  char data[data_size] = {
      0x00, 0x00, 0x00, data_size,  // size
      's',  'k',  'i',  'p',   // type
      0x00, 0x00, 0x00, 0x00,  // data
      0x00, 0x00, 0x00, 0x00,
  };
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_skip_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, 0x10);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("skip"));
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadFreeAtom) {
  MuTFFError err;
  MuTFFFreeAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 16;
  char data[data_size] = {
      0x00, 0x00, 0x00, 0x10,  // size
      'f',  'r',  'e',  'e',   // type
      0x00, 0x00, 0x00, 0x00,  // data
      0x00, 0x00, 0x00, 0x00,
  };
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_free_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, 0x10);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("free"));
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadMovieDataAtom) {
  MuTFFError err;
  MuTFFMovieDataAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 16;
  char data[data_size] = {
      0x00, 0x00, 0x00, data_size,  // size
      'm',  'd',  'a',  't',        // type
      0x00, 0x00, 0x00, 0x00,       // data
      0x00, 0x00, 0x00, 0x00,
  };
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_movie_data_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, 0x10);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("mdat"));
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadFileTypeCompatibilityAtom) {
  MuTFFError err;
  MuTFFFileTypeCompatibilityAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 20;
  char data[data_size] = {
      0x00, 0x00, 0x00, data_size,  // size
      'f',  't',  'y',  'p',        // type
      'q',  't',  ' ',  ' ',        // major brand
      20,   04,   06,   00,         // minor version
      'q',  't',  ' ',  ' ',        // compatible_brands[0]
  };
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_file_type_compatibility_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, 0x14);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("ftyp"));
  EXPECT_EQ(atom.major_brand, MuTFF_FOUR_C("qt  "));
  EXPECT_EQ(MuTFF_FOUR_C(atom.minor_version),
            MuTFF_FOUR_C(((char[]){20, 04, 06, 00})));
  EXPECT_EQ(atom.compatible_brands_count, 1);
  EXPECT_EQ(MuTFF_FOUR_C(atom.compatible_brands[0]), MuTFF_FOUR_C("qt  "));
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadFileFormat) {
  MuTFFError err;
  QTFileFormat file_format;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 4;
  char data[data_size] = {'a', 'b', 'c', 'd'};
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_file_format(fd, &file_format);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(MuTFF_FOUR_C(file_format), MuTFF_FOUR_C("abcd"));
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, PeekAtomHeader) {
  MuTFFAtomHeader header;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 8;
  char data[data_size] = {0x01, 0x02, 0x03, 0x04, 'a', 'b', 'c', 'd'};
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  mutff_peek_atom_header(fd, &header);

  EXPECT_EQ(header.size, 0x01020304);
  EXPECT_EQ(MuTFF_FOUR_C(header.type), MuTFF_FOUR_C("abcd"));
  EXPECT_EQ(ftell(fd), 0);
}

TEST(MuTFF, ReadAtomType) {
  MuTFFAtomType type;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 4;
  char data[data_size] = {'a', 'b', 'c', 'd'};
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  mutff_read_atom_type(fd, &type);

  EXPECT_EQ(MuTFF_FOUR_C(type), MuTFF_FOUR_C("abcd"));
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, FixedAtomSizes) {
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
