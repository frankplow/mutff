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

TEST(MuTFF, ReadSampleDependencyFlagsAtom) {
  MuTFFError err;
  MuTFFSampleDependencyFlagsAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 14;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,  // size
    's', 'd', 't', 'p',           // type
    0x00,                         // version
    0x00, 0x01, 0x02,             // flags
    0x10, 0x11,                   // sample size table
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_sample_dependency_flags_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("sdtp"));
  EXPECT_EQ(atom.version_flags.version, 0x00);
  EXPECT_EQ(atom.version_flags.flags, 0x000102);
  EXPECT_EQ(atom.sample_dependency_flags_table[0], 0x10);
  EXPECT_EQ(atom.sample_dependency_flags_table[1], 0x11);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadChunkOffsetAtom) {
  MuTFFError err;
  MuTFFChunkOffsetAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 20;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,  // size
    's', 't', 'c', 'o',           // type
    0x00,                         // version
    0x00, 0x01, 0x02,             // flags
    0x00, 0x00, 0x00, 0x01,       // number of entries
    0x10, 0x11, 0x12, 0x13,       // sample size table
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_chunk_offset_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("stco"));
  EXPECT_EQ(atom.version_flags.version, 0x00);
  EXPECT_EQ(atom.version_flags.flags, 0x000102);
  EXPECT_EQ(atom.number_of_entries, 1);
  EXPECT_EQ(atom.chunk_offset_table[0], 0x10111213);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadSampleSizeAtom) {
  MuTFFError err;
  MuTFFSampleSizeAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 24;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,  // size
    's', 't', 's', 'z',           // type
    0x00,                         // version
    0x00, 0x01, 0x02,             // flags
    0x00, 0x00, 0x00, 0x00,       // sample size
    0x00, 0x00, 0x00, 0x01,       // number of entries
    0x10, 0x11, 0x12, 0x13,       // sample size table
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_sample_size_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("stsz"));
  EXPECT_EQ(atom.version_flags.version, 0x00);
  EXPECT_EQ(atom.version_flags.flags, 0x000102);
  EXPECT_EQ(atom.sample_size, 0);
  EXPECT_EQ(atom.number_of_entries, 1);
  EXPECT_EQ(atom.sample_size_table[0], 0x10111213);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadSampleToChunkAtom) {
  MuTFFError err;
  MuTFFSampleToChunkAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 40;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,  // size
    's', 't', 's', 'c',           // type
    0x00,                         // version
    0x00, 0x01, 0x02,             // flags
    0x00, 0x00, 0x00, 0x02,       // number of entries
    0x00, 0x01, 0x02, 0x03,       // table[0].first chunk
    0x10, 0x11, 0x12, 0x13,       // table[0].samples per chunk
    0x20, 0x21, 0x22, 0x23,       // table[0].sample description ID
    0x30, 0x31, 0x32, 0x33,       // table[1].first chunk
    0x40, 0x41, 0x42, 0x43,       // table[1].samples per chunk
    0x50, 0x51, 0x52, 0x53,       // table[1].sample description ID
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_sample_to_chunk_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("stsc"));
  EXPECT_EQ(atom.version_flags.version, 0x00);
  EXPECT_EQ(atom.version_flags.flags, 0x000102);
  EXPECT_EQ(atom.number_of_entries, 2);
  EXPECT_EQ(atom.sample_to_chunk_table[0].first_chunk, 0x00010203);
  EXPECT_EQ(atom.sample_to_chunk_table[0].samples_per_chunk, 0x10111213);
  EXPECT_EQ(atom.sample_to_chunk_table[0].sample_description_id, 0x20212223);
  EXPECT_EQ(atom.sample_to_chunk_table[1].first_chunk, 0x30313233);
  EXPECT_EQ(atom.sample_to_chunk_table[1].samples_per_chunk, 0x40414243);
  EXPECT_EQ(atom.sample_to_chunk_table[1].sample_description_id, 0x50515253);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadSampleToChunkTableEntry) {
  MuTFFError err;
  MuTFFSampleToChunkTableEntry entry;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 12;
  // clang-format off
  char data[data_size] = {
    0x00, 0x01, 0x02, 0x03,  // first chunk
    0x10, 0x11, 0x12, 0x13,  // samples per chunk
    0x20, 0x21, 0x22, 0x23,  // sample description ID
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_sample_to_chunk_table_entry(fd, &entry);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(entry.first_chunk, 0x00010203);
  EXPECT_EQ(entry.samples_per_chunk, 0x10111213);
  EXPECT_EQ(entry.sample_description_id, 0x20212223);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadPartialSyncSampleAtom) {
  MuTFFError err;
  MuTFFPartialSyncSampleAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 24;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,  // size
    's', 't', 'p', 's',           // type
    0x00,                         // version
    0x00, 0x01, 0x02,             // flags
    0x00, 0x00, 0x00, 0x02,       // number of entries
    0x00, 0x01, 0x02, 0x03,       // table[0].sample count
    0x10, 0x11, 0x12, 0x13,       // table[0].composition offset
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_partial_sync_sample_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("stps"));
  EXPECT_EQ(atom.version_flags.version, 0x00);
  EXPECT_EQ(atom.version_flags.flags, 0x000102);
  EXPECT_EQ(atom.entry_count, 2);
  EXPECT_EQ(atom.partial_sync_sample_table[0], 0x00010203);
  EXPECT_EQ(atom.partial_sync_sample_table[1], 0x10111213);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadSyncSampleAtom) {
  MuTFFError err;
  MuTFFSyncSampleAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 24;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,  // size
    's', 't', 't', 's',           // type
    0x00,                         // version
    0x00, 0x01, 0x02,             // flags
    0x00, 0x00, 0x00, 0x02,       // number of entries
    0x00, 0x01, 0x02, 0x03,       // table[0].sample count
    0x10, 0x11, 0x12, 0x13,       // table[0].composition offset
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_sync_sample_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("stts"));
  EXPECT_EQ(atom.version_flags.version, 0x00);
  EXPECT_EQ(atom.version_flags.flags, 0x000102);
  EXPECT_EQ(atom.number_of_entries, 2);
  EXPECT_EQ(atom.sync_sample_table[0], 0x00010203);
  EXPECT_EQ(atom.sync_sample_table[1], 0x10111213);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadCompositionShiftLeastGreatestAtom) {
  MuTFFError err;
  MuTFFCompositionShiftLeastGreatestAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 32;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,  // size
    'c', 's', 'l', 'g',           // type
    0x00,                         // version
    0x00, 0x01, 0x02,             // flags
    0x00, 0x01, 0x02, 0x03,       // composition offset to display offset shift
    0x10, 0x11, 0x12, 0x13,       // least display offset
    0x20, 0x21, 0x22, 0x23,       // greatest display offset
    0x30, 0x31, 0x32, 0x33,       // start display time
    0x40, 0x41, 0x42, 0x43,       // end display time
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_composition_shift_least_greatest_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("cslg"));
  EXPECT_EQ(atom.version_flags.version, 0x00);
  EXPECT_EQ(atom.version_flags.flags, 0x000102);
  EXPECT_EQ(atom.composition_offset_to_display_offset_shift, 0x00010203);
  EXPECT_EQ(atom.least_display_offset, 0x10111213);
  EXPECT_EQ(atom.greatest_display_offset, 0x20212223);
  EXPECT_EQ(atom.display_start_time, 0x30313233);
  EXPECT_EQ(atom.display_end_time, 0x40414243);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadCompositionOffsetAtom) {
  MuTFFError err;
  MuTFFCompositionOffsetAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 32;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,  // size
    'c', 't', 't', 's',           // type
    0x00,                         // version
    0x00, 0x01, 0x02,             // flags
    0x00, 0x00, 0x00, 0x02,       // number of entries
    0x00, 0x01, 0x02, 0x03,       // table[0].sample count
    0x10, 0x11, 0x12, 0x13,       // table[0].composition offset
    0x20, 0x21, 0x22, 0x23,       // table[1].sample count
    0x30, 0x31, 0x32, 0x33,       // table[1].composition offset
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_composition_offset_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("ctts"));
  EXPECT_EQ(atom.version_flags.version, 0x00);
  EXPECT_EQ(atom.version_flags.flags, 0x000102);
  EXPECT_EQ(atom.entry_count, 2);
  EXPECT_EQ(atom.composition_offset_table[0].sample_count, 0x00010203);
  EXPECT_EQ(atom.composition_offset_table[0].composition_offset, 0x10111213);
  EXPECT_EQ(atom.composition_offset_table[1].sample_count, 0x20212223);
  EXPECT_EQ(atom.composition_offset_table[1].composition_offset, 0x30313233);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadCompositionOffsetTableEntry) {
  MuTFFError err;
  MuTFFCompositionOffsetTableEntry entry;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 8;
  // clang-format off
  char data[data_size] = {
    0x00, 0x01, 0x02, 0x03,  // sample count
    0x10, 0x11, 0x12, 0x13,  // composition offset
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_composition_offset_table_entry(fd, &entry);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(entry.sample_count, 0x00010203);
  EXPECT_EQ(entry.composition_offset, 0x10111213);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadTimeToSampleAtom) {
  MuTFFError err;
  MuTFFTimeToSampleAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 32;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,  // size
    's', 't', 't', 's',           // type
    0x00,                         // version
    0x00, 0x01, 0x02,             // flags
    0x00, 0x00, 0x00, 0x02,       // number of entries
    0x00, 0x01, 0x02, 0x03,       // table[0].sample count
    0x10, 0x11, 0x12, 0x13,       // table[0].sample duration
    0x20, 0x21, 0x22, 0x23,       // table[1].sample count
    0x30, 0x31, 0x32, 0x33,       // table[1].sample duration
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_time_to_sample_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("stts"));
  EXPECT_EQ(atom.version_flags.version, 0x00);
  EXPECT_EQ(atom.version_flags.flags, 0x000102);
  EXPECT_EQ(atom.number_of_entries, 2);
  EXPECT_EQ(atom.time_to_sample_table[0].sample_count, 0x00010203);
  EXPECT_EQ(atom.time_to_sample_table[0].sample_duration, 0x10111213);
  EXPECT_EQ(atom.time_to_sample_table[1].sample_count, 0x20212223);
  EXPECT_EQ(atom.time_to_sample_table[1].sample_duration, 0x30313233);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadTimeToSampleTableEntry) {
  MuTFFError err;
  MuTFFTimeToSampleTableEntry entry;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 8;
  // clang-format off
  char data[data_size] = {
    0x00, 0x01, 0x02, 0x03,  // sample count
    0x10, 0x11, 0x12, 0x13,  // sample duration
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_time_to_sample_table_entry(fd, &entry);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(entry.sample_count, 0x00010203);
  EXPECT_EQ(entry.sample_duration, 0x10111213);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadSampleDescriptionAtom) {
  MuTFFError err;
  MuTFFSampleDescriptionAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 56;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,         // size
    's', 't', 's', 'd',                  // type
    0x00,                                // version
    0x00, 0x01, 0x02,                    // flags
    0x00, 0x00, 0x00, 0x02,              // number of entries
    0x00, 0x00, 0x00, 20,                // sample_description table[0].size
    'a', 'b', 'c', 'd',                  // sample_description table[0].data format
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // sample_description table[0].reserved
    0x00, 0x01,                          // sample_description table[0].data reference index
    0x00, 0x01, 0x02, 0x03,              // sample_description table[0].media-specific data
    0x00, 0x00, 0x00, 20,                // sample_description table[1].size
    'e', 'f', 'g', 'h',                  // sample_description table[1].data format
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // sample_description table[1].reserved
    0x10, 0x11,                          // sample_description table[1].data reference index
    0x10, 0x11, 0x12, 0x13,              // sample_description table[1].media-specific data
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_sample_description_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("stsd"));
  EXPECT_EQ(atom.version_flags.version, 0x00);
  EXPECT_EQ(atom.version_flags.flags, 0x000102);
  EXPECT_EQ(atom.number_of_entries, 2);
  EXPECT_EQ(atom.sample_description_table[0].size, 20);
  EXPECT_EQ(atom.sample_description_table[0].data_format, MuTFF_FOUR_C("abcd"));
  EXPECT_EQ(atom.sample_description_table[0].data_reference_index, 0x0001);
  EXPECT_EQ(MuTFF_FOUR_C(atom.sample_description_table[0].additional_data),
            0x00010203);
  EXPECT_EQ(atom.sample_description_table[1].size, 20);
  EXPECT_EQ(atom.sample_description_table[1].data_format, MuTFF_FOUR_C("efgh"));
  EXPECT_EQ(atom.sample_description_table[1].data_reference_index, 0x1011);
  EXPECT_EQ(MuTFF_FOUR_C(atom.sample_description_table[1].additional_data),
            0x10111213);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadDataInformationAtom) {
  MuTFFError err;
  MuTFFDataInformationAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 56;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,  // size
    'd', 'i', 'n', 'f',           // type
    0x00, 0x00, 0x00, 48,         // size
    'd', 'r', 'e', 'f',           // type
    0x00,                         // version
    0x00, 0x01, 0x02,             // flag
    0x00, 0x00, 0x00, 0x02,       // number of entries
    0x00, 0x00, 0x00, 16,         // data references[0].size
    'a', 'b', 'c', 'd',           // data references[0] type
    0x00,                         // data references[0] version
    0x00, 0x01, 0x02,             // data references[0] flags
    0x00, 0x01, 0x02, 0x03,       // data references[0] data
    0x00, 0x00, 0x00, 16,         // data references[1].size
    'e', 'f', 'g', 'h',           // data references[1] type
    0x10,                         // data references[1] version
    0x10, 0x11, 0x12,             // data references[1] flags
    0x10, 0x11, 0x12, 0x13,       // data references[1] data
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_data_information_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("dinf"));
  EXPECT_EQ(atom.data_reference.size, 48);
  EXPECT_EQ(MuTFF_FOUR_C(atom.data_reference.type), MuTFF_FOUR_C("dref"));
  EXPECT_EQ(atom.data_reference.number_of_entries, 2);
  EXPECT_EQ(atom.data_reference.data_references[0].size, 16);
  EXPECT_EQ(MuTFF_FOUR_C(atom.data_reference.data_references[0].type), MuTFF_FOUR_C("abcd"));
  EXPECT_EQ(atom.data_reference.data_references[0].version_flags.version, 0x00);
  EXPECT_EQ(atom.data_reference.data_references[0].version_flags.flags, 0x000102);
  EXPECT_EQ(MuTFF_FOUR_C(atom.data_reference.data_references[0].data), 0x00010203);
  EXPECT_EQ(atom.data_reference.data_references[1].size, 16);
  EXPECT_EQ(MuTFF_FOUR_C(atom.data_reference.data_references[1].type), MuTFF_FOUR_C("efgh"));
  EXPECT_EQ(atom.data_reference.data_references[1].version_flags.version, 0x10);
  EXPECT_EQ(atom.data_reference.data_references[1].version_flags.flags, 0x101112);
  EXPECT_EQ(MuTFF_FOUR_C(atom.data_reference.data_references[1].data), 0x10111213);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadDataReferenceAtom) {
  MuTFFError err;
  MuTFFDataReferenceAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 48;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,  // size
    'd', 'r', 'e', 'f',           // type
    0x00,                         // version
    0x00, 0x01, 0x02,             // flag
    0x00, 0x00, 0x00, 0x02,       // number of entries
    0x00, 0x00, 0x00, 16,         // data references[0].size
    'a', 'b', 'c', 'd',           // data references[0] type
    0x00,                         // data references[0] version
    0x00, 0x01, 0x02,             // data references[0] flags
    0x00, 0x01, 0x02, 0x03,       // data references[0] data
    0x00, 0x00, 0x00, 16,         // data references[1].size
    'e', 'f', 'g', 'h',           // data references[1] type
    0x10,                         // data references[1] version
    0x10, 0x11, 0x12,             // data references[1] flags
    0x10, 0x11, 0x12, 0x13,       // data references[1] data
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_data_reference_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("dref"));
  EXPECT_EQ(atom.number_of_entries, 2);
  EXPECT_EQ(atom.data_references[0].size, 16);
  EXPECT_EQ(MuTFF_FOUR_C(atom.data_references[0].type), MuTFF_FOUR_C("abcd"));
  EXPECT_EQ(atom.data_references[0].version_flags.version, 0x00);
  EXPECT_EQ(atom.data_references[0].version_flags.flags, 0x000102);
  EXPECT_EQ(MuTFF_FOUR_C(atom.data_references[0].data), 0x00010203);
  EXPECT_EQ(atom.data_references[1].size, 16);
  EXPECT_EQ(MuTFF_FOUR_C(atom.data_references[1].type), MuTFF_FOUR_C("efgh"));
  EXPECT_EQ(atom.data_references[1].version_flags.version, 0x10);
  EXPECT_EQ(atom.data_references[1].version_flags.flags, 0x101112);
  EXPECT_EQ(MuTFF_FOUR_C(atom.data_references[1].data), 0x10111213);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadDataReference) {
  MuTFFError err;
  MuTFFDataReference ref;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 16;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,  // size
    'a', 'b', 'c', 'd',           // type
    0x00,                         // version
    0x00, 0x01, 0x02,             // flags
    0x00, 0x01, 0x02, 0x03,       // data
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_data_reference(fd, &ref);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(ref.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(ref.type), MuTFF_FOUR_C("abcd"));
  EXPECT_EQ(ref.version_flags.version, 0x00);
  EXPECT_EQ(ref.version_flags.flags, 0x000102);
  EXPECT_EQ(MuTFF_FOUR_C(ref.data), 0x00010203);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadBaseMediaInformationAtom) {
  MuTFFError err;
  MuTFFBaseMediaInformationAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 84;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,  // size
    'm', 'i', 'n', 'f',           // type
    0x00, 0x00, 0x00, 76,         // size
    'g', 'm', 'h', 'd',           // type
    0x00, 0x00, 0x00, 24,         // size
    'g', 'm', 'i', 'n',           // type
    0x00,                         // version
    0x00, 0x01, 0x02,             // flags
    0x00, 0x01,                   // graphics mode
    0x10, 0x11,                   // opcolor[0]
    0x20, 0x21,                   // opcolor[1]
    0x30, 0x31,                   // opcolor[2]
    0x40, 0x41,                   // balance
    0x50, 0x50,                   // reserved
    0x00, 0x00, 0x00, 44,         // size
    't', 'e', 'x', 't',           // type
    0x01, 0x02, 0x03, 0x04,       // matrix[0][0]
    0x05, 0x06, 0x07, 0x08,       // matrix[0][1]
    0x09, 0x0a, 0x0b, 0x0c,       // matrix[0][2]
    0x0d, 0x0e, 0x0f, 0x10,       // matrix[1][0]
    0x11, 0x12, 0x13, 0x14,       // matrix[1][1]
    0x15, 0x16, 0x17, 0x18,       // matrix[1][2]
    0x19, 0x1a, 0x1b, 0x1c,       // matrix[2][0]
    0x1d, 0x1e, 0x1f, 0x20,       // matrix[2][1]
    0x21, 0x22, 0x23, 0x24,       // matrix[2][2]
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_base_media_information_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("minf"));
  EXPECT_EQ(atom.base_media_information_header.size, 76);
  EXPECT_EQ(MuTFF_FOUR_C(atom.base_media_information_header.type), MuTFF_FOUR_C("gmhd"));
  EXPECT_EQ(atom.base_media_information_header.base_media_info.size, 24);
  EXPECT_EQ(MuTFF_FOUR_C(atom.base_media_information_header.base_media_info.type), MuTFF_FOUR_C("gmin"));
  EXPECT_EQ(atom.base_media_information_header.base_media_info.version_flags.version, 0x00);
  EXPECT_EQ(atom.base_media_information_header.base_media_info.version_flags.flags, 0x000102);
  EXPECT_EQ(atom.base_media_information_header.base_media_info.graphics_mode, 0x0001);
  EXPECT_EQ(atom.base_media_information_header.base_media_info.opcolor[0], 0x1011);
  EXPECT_EQ(atom.base_media_information_header.base_media_info.opcolor[1], 0x2021);
  EXPECT_EQ(atom.base_media_information_header.base_media_info.opcolor[2], 0x3031);
  EXPECT_EQ(atom.base_media_information_header.base_media_info.balance, 0x4041);
  EXPECT_EQ(atom.base_media_information_header.text_media_information.size, 44);
  EXPECT_EQ(MuTFF_FOUR_C(atom.base_media_information_header.text_media_information.type), MuTFF_FOUR_C("text"));
  for (size_t j = 0; j < 3; ++j) {
    for (size_t i = 0; i < 3; ++i) {
      const uint32_t exp_base = (3 * j + i) * 4 + 1;
      const uint32_t exp = (exp_base << 24) + ((exp_base + 1) << 16) +
                           ((exp_base + 2) << 8) + (exp_base + 3);
      EXPECT_EQ(atom.base_media_information_header.text_media_information
                    .matrix_structure[j][i],
                exp);
    }
  }
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadBaseMediaInformationHeaderAtom) {
  MuTFFError err;
  MuTFFBaseMediaInformationHeaderAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 76;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,  // size
    'g', 'm', 'h', 'd',           // type
    0x00, 0x00, 0x00, 24,         // size
    'g', 'm', 'i', 'n',           // type
    0x00,                         // version
    0x00, 0x01, 0x02,             // flags
    0x00, 0x01,                   // graphics mode
    0x10, 0x11,                   // opcolor[0]
    0x20, 0x21,                   // opcolor[1]
    0x30, 0x31,                   // opcolor[2]
    0x40, 0x41,                   // balance
    0x50, 0x50,                   // reserved
    0x00, 0x00, 0x00, 44,         // size
    't', 'e', 'x', 't',           // type
    0x01, 0x02, 0x03, 0x04,       // matrix[0][0]
    0x05, 0x06, 0x07, 0x08,       // matrix[0][1]
    0x09, 0x0a, 0x0b, 0x0c,       // matrix[0][2]
    0x0d, 0x0e, 0x0f, 0x10,       // matrix[1][0]
    0x11, 0x12, 0x13, 0x14,       // matrix[1][1]
    0x15, 0x16, 0x17, 0x18,       // matrix[1][2]
    0x19, 0x1a, 0x1b, 0x1c,       // matrix[2][0]
    0x1d, 0x1e, 0x1f, 0x20,       // matrix[2][1]
    0x21, 0x22, 0x23, 0x24,       // matrix[2][2]
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_base_media_information_header_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("gmhd"));
  EXPECT_EQ(atom.base_media_info.size, 24);
  EXPECT_EQ(MuTFF_FOUR_C(atom.base_media_info.type), MuTFF_FOUR_C("gmin"));
  EXPECT_EQ(atom.base_media_info.version_flags.version, 0x00);
  EXPECT_EQ(atom.base_media_info.version_flags.flags, 0x000102);
  EXPECT_EQ(atom.base_media_info.graphics_mode, 0x0001);
  EXPECT_EQ(atom.base_media_info.opcolor[0], 0x1011);
  EXPECT_EQ(atom.base_media_info.opcolor[1], 0x2021);
  EXPECT_EQ(atom.base_media_info.opcolor[2], 0x3031);
  EXPECT_EQ(atom.base_media_info.balance, 0x4041);
  EXPECT_EQ(atom.text_media_information.size, 44);
  EXPECT_EQ(MuTFF_FOUR_C(atom.text_media_information.type), MuTFF_FOUR_C("text"));
  for (size_t j = 0; j < 3; ++j) {
    for (size_t i = 0; i < 3; ++i) {
      const uint32_t exp_base = (3 * j + i) * 4 + 1;
      const uint32_t exp = (exp_base << 24) + ((exp_base + 1) << 16) +
                           ((exp_base + 2) << 8) + (exp_base + 3);
      EXPECT_EQ(atom.text_media_information.matrix_structure[j][i], exp);
    }
  }
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadBaseMediaInfoAtom) {
  MuTFFError err;
  MuTFFBaseMediaInfoAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 24;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,  // size
    'g', 'm', 'i', 'n',           // type
    0x00,                         // version
    0x00, 0x01, 0x02,             // flags
    0x00, 0x01,                   // graphics mode
    0x10, 0x11,                   // opcolor[0]
    0x20, 0x21,                   // opcolor[1]
    0x30, 0x31,                   // opcolor[2]
    0x40, 0x41,                   // balance
    0x50, 0x50,                   // reserved
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_base_media_info_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("gmin"));
  EXPECT_EQ(atom.version_flags.version, 0x00);
  EXPECT_EQ(atom.version_flags.flags, 0x000102);
  EXPECT_EQ(atom.graphics_mode, 0x0001);
  EXPECT_EQ(atom.opcolor[0], 0x1011);
  EXPECT_EQ(atom.opcolor[1], 0x2021);
  EXPECT_EQ(atom.opcolor[2], 0x3031);
  EXPECT_EQ(atom.balance, 0x4041);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadTextMediaInformationAtom) {
  MuTFFError err;
  MuTFFTextMediaInformationAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 44;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,  // size
    't', 'e', 'x', 't',           // type
    0x01, 0x02, 0x03, 0x04,       // matrix[0][0]
    0x05, 0x06, 0x07, 0x08,       // matrix[0][1]
    0x09, 0x0a, 0x0b, 0x0c,       // matrix[0][2]
    0x0d, 0x0e, 0x0f, 0x10,       // matrix[1][0]
    0x11, 0x12, 0x13, 0x14,       // matrix[1][1]
    0x15, 0x16, 0x17, 0x18,       // matrix[1][2]
    0x19, 0x1a, 0x1b, 0x1c,       // matrix[2][0]
    0x1d, 0x1e, 0x1f, 0x20,       // matrix[2][1]
    0x21, 0x22, 0x23, 0x24,       // matrix[2][2]
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_text_media_information_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("text"));
  for (size_t j = 0; j < 3; ++j) {
    for (size_t i = 0; i < 3; ++i) {
      const uint32_t exp_base = (3 * j + i) * 4 + 1;
      const uint32_t exp = (exp_base << 24) + ((exp_base + 1) << 16) +
                           ((exp_base + 2) << 8) + (exp_base + 3);
      EXPECT_EQ(atom.matrix_structure[j][i], exp);
    }
  }
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadSoundMediaInformationHeaderAtom) {
  MuTFFError err;
  MuTFFSoundMediaInformationHeaderAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 16;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,  // size
    's', 'm', 'h', 'd',           // type
    0x00,                         // version
    0x00, 0x01, 0x02,             // flags
    0x10, 0x11,                   // balance
    0x00, 0x00,                   // reserved
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_sound_media_information_header_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("smhd"));
  EXPECT_EQ(atom.version_flags.version, 0x00);
  EXPECT_EQ(atom.version_flags.flags, 0x000102);
  EXPECT_EQ(atom.balance, 0x1011);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadVideoMediaInformationHeaderAtom) {
  MuTFFError err;
  MuTFFVideoMediaInformationHeaderAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 20;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,  // size
    'v', 'm', 'h', 'd',           // type
    0x00,                         // version
    0x00, 0x01, 0x02,             // flags
    0x00, 0x01,                   // graphics mode
    0x10, 0x11,                   // opcolor[0]
    0x20, 0x21,                   // opcolor[1]
    0x30, 0x31,                   // opcolor[2]
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_video_media_information_header_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("vmhd"));
  EXPECT_EQ(atom.version_flags.version, 0x00);
  EXPECT_EQ(atom.version_flags.flags, 0x000102);
  EXPECT_EQ(atom.graphics_mode, 0x0001);
  EXPECT_EQ(atom.opcolor[0], 0x1011);
  EXPECT_EQ(atom.opcolor[1], 0x2021);
  EXPECT_EQ(atom.opcolor[2], 0x3031);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadHandlerReferenceAtom) {
  MuTFFError err;
  MuTFFHandlerReferenceAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 36;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,  // size
    'h', 'd', 'l', 'r',           // type
    0x00,                         // version
    0x00, 0x01, 0x02,             // flags
    0x00, 0x01, 0x02, 0x03,       // component type
    0x10, 0x11, 0x12, 0x13,       // component subtype
    0x20, 0x21, 0x22, 0x23,       // component manufacturer
    0x30, 0x31, 0x32, 0x33,       // component flags
    0x40, 0x41, 0x42, 0x43,       // component flags mask
    'a', 'b', 'c', 'd',           // component name
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_handler_reference_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("hdlr"));
  EXPECT_EQ(atom.version_flags.version, 0x00);
  EXPECT_EQ(atom.version_flags.flags, 0x000102);
  EXPECT_EQ(atom.component_type, 0x00010203);
  EXPECT_EQ(atom.component_subtype, 0x10111213);
  EXPECT_EQ(atom.component_manufacturer, 0x20212223);
  EXPECT_EQ(atom.component_flags, 0x30313233);
  EXPECT_EQ(atom.component_flags_mask, 0x40414243);
  EXPECT_EQ(MuTFF_FOUR_C(atom.component_name), MuTFF_FOUR_C("abcd"));
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadExtendedLanguageTagAtom) {
  MuTFFError err;
  MuTFFExtendedLanguageTagAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 18;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,    // size
    'e', 'l', 'n', 'g',             // type
    0x00,                           // version
    0x00, 0x01, 0x02,               // flags
    'e', 'n', '-', 'U', 'S', '\0',  // language tag string
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_extended_language_tag_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("elng"));
  EXPECT_EQ(atom.version_flags.version, 0x00);
  EXPECT_EQ(atom.version_flags.flags, 0x000102);
  EXPECT_STREQ(atom.language_tag_string, "en-US");
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadMediaHeaderAtom) {
  MuTFFError err;
  MuTFFMediaHeaderAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 32;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,  // size
    'm', 'd', 'h', 'd',           // type
    0x00,                         // version
    0x00, 0x01, 0x02,             // flags
    0x00, 0x01, 0x02, 0x03,       // creation time
    0x10, 0x11, 0x12, 0x13,       // modification time
    0x20, 0x21, 0x22, 0x23,       // time scale
    0x30, 0x31, 0x32, 0x33,       // duration
    0x40, 0x41,                   // language
    0x50, 0x51,                   // quality
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_media_header_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("mdhd"));
  EXPECT_EQ(atom.version_flags.version, 0x00);
  EXPECT_EQ(atom.version_flags.flags, 0x000102);
  EXPECT_EQ(atom.creation_time, 0x00010203);
  EXPECT_EQ(atom.modification_time, 0x10111213);
  EXPECT_EQ(atom.time_scale, 0x20212223);
  EXPECT_EQ(atom.duration, 0x30313233);
  EXPECT_EQ(atom.language, 0x4041);
  EXPECT_EQ(atom.quality, 0x5051);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadTrackInputMapAtom) {
  MuTFFError err;
  MuTFFTrackInputMapAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 96;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,  // size
    'i', 'm', 'a', 'p',           // type
    0x00, 0x00, 0x00, 44,         // size
    '\0', '\0', 'i', 'n',         // type
    0x00, 0x01, 0x02, 0x03,       // atom id
    0x00, 0x00,                   // reserved
    0x00, 0x02,                   // child count
    0x00, 0x00, 0x00, 0x00,       // reserved
    0x00, 0x00, 0x00, 0x0c,       // input type atom.size
    '\0', '\0', 't', 'y',         // input type atom.type
    0x00, 0x01, 0x02, 0x03,       // input type atom.input type
    0x00, 0x00, 0x00, 0x0c,       // object id atom.size
    'o', 'b', 'i', 'd',           // object id atom.type
    0x00, 0x01, 0x02, 0x03,       // object id atom.object id
    0x00, 0x00, 0x00, 44,         // size
    '\0', '\0', 'i', 'n',         // type
    0x00, 0x01, 0x02, 0x03,       // atom id
    0x00, 0x00,                   // reserved
    0x00, 0x02,                   // child count
    0x00, 0x00, 0x00, 0x00,       // reserved
    0x00, 0x00, 0x00, 0x0c,       // input type atom.size
    '\0', '\0', 't', 'y',         // input type atom.type
    0x00, 0x01, 0x02, 0x03,       // input type atom.input type
    0x00, 0x00, 0x00, 0x0c,       // object id atom.size
    'o', 'b', 'i', 'd',           // object id atom.type
    0x00, 0x01, 0x02, 0x03,       // object id atom.object id
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_track_input_map_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("imap"));
  EXPECT_EQ(atom.track_input_atoms[0].size, 44);
  EXPECT_EQ(MuTFF_FOUR_C(atom.track_input_atoms[0].type), MuTFF_FOUR_C("\0\0in"));
  EXPECT_EQ(atom.track_input_atoms[0].atom_id, 0x00010203);
  EXPECT_EQ(atom.track_input_atoms[0].child_count, 0x0002);
  EXPECT_EQ(atom.track_input_atoms[0].input_type_atom.size, 0x0c);
  EXPECT_EQ(MuTFF_FOUR_C(atom.track_input_atoms[0].input_type_atom.type), MuTFF_FOUR_C("\0\0ty"));
  EXPECT_EQ(atom.track_input_atoms[0].input_type_atom.input_type, 0x00010203);
  EXPECT_EQ(atom.track_input_atoms[0].object_id_atom.size, 0x0c);
  EXPECT_EQ(MuTFF_FOUR_C(atom.track_input_atoms[0].object_id_atom.type), MuTFF_FOUR_C("obid"));
  EXPECT_EQ(atom.track_input_atoms[0].object_id_atom.object_id, 0x00010203);
  EXPECT_EQ(atom.track_input_atoms[1].size, 44);
  EXPECT_EQ(MuTFF_FOUR_C(atom.track_input_atoms[1].type), MuTFF_FOUR_C("\0\0in"));
  EXPECT_EQ(atom.track_input_atoms[1].atom_id, 0x00010203);
  EXPECT_EQ(atom.track_input_atoms[1].child_count, 0x0002);
  EXPECT_EQ(atom.track_input_atoms[1].input_type_atom.size, 0x0c);
  EXPECT_EQ(MuTFF_FOUR_C(atom.track_input_atoms[1].input_type_atom.type), MuTFF_FOUR_C("\0\0ty"));
  EXPECT_EQ(atom.track_input_atoms[1].input_type_atom.input_type, 0x00010203);
  EXPECT_EQ(atom.track_input_atoms[1].object_id_atom.size, 0x0c);
  EXPECT_EQ(MuTFF_FOUR_C(atom.track_input_atoms[1].object_id_atom.type), MuTFF_FOUR_C("obid"));
  EXPECT_EQ(atom.track_input_atoms[1].object_id_atom.object_id, 0x00010203);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadTrackInputAtom) {
  MuTFFError err;
  MuTFFTrackInputAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 44;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,  // size
    '\0', '\0', 'i', 'n',         // type
    0x00, 0x01, 0x02, 0x03,       // atom id
    0x00, 0x00,                   // reserved
    0x00, 0x02,                   // child count
    0x00, 0x00, 0x00, 0x00,       // reserved
    0x00, 0x00, 0x00, 0x0c,       // input type atom.size
    '\0', '\0', 't', 'y',         // input type atom.type
    0x00, 0x01, 0x02, 0x03,       // input type atom.input type
    0x00, 0x00, 0x00, 0x0c,       // object id atom.size
    'o', 'b', 'i', 'd',           // object id atom.type
    0x00, 0x01, 0x02, 0x03,       // object id atom.object id
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_track_input_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("\0\0in"));
  EXPECT_EQ(atom.atom_id, 0x00010203);
  EXPECT_EQ(atom.child_count, 0x0002);
  EXPECT_EQ(atom.input_type_atom.size, 0x0c);
  EXPECT_EQ(MuTFF_FOUR_C(atom.input_type_atom.type), MuTFF_FOUR_C("\0\0ty"));
  EXPECT_EQ(atom.input_type_atom.input_type, 0x00010203);
  EXPECT_EQ(atom.object_id_atom.size, 0x0c);
  EXPECT_EQ(MuTFF_FOUR_C(atom.object_id_atom.type), MuTFF_FOUR_C("obid"));
  EXPECT_EQ(atom.object_id_atom.object_id, 0x00010203);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadObjectIDAtom) {
  MuTFFError err;
  MuTFFObjectIDAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 12;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,  // size
    'o', 'b', 'i', 'd',           // type
    0x00, 0x01, 0x02, 0x03,       // object id
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_object_id_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("obid"));
  EXPECT_EQ(atom.object_id, 0x00010203);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadInputTypeAtom) {
  MuTFFError err;
  MuTFFInputTypeAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 12;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,  // size
    '\0', '\0', 't', 'y',         // type
    0x00, 0x01, 0x02, 0x03,       // input type
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_input_type_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("\0\0ty"));
  EXPECT_EQ(atom.input_type, 0x00010203);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadTrackLoadSettingsAtom) {
  MuTFFError err;
  MuTFFTrackLoadSettingsAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 24;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,  // size
    'l', 'o', 'a', 'd',           // type
    0x00, 0x01, 0x02, 0x03,       // preload start time
    0x10, 0x11, 0x12, 0x13,       // preload duration
    0x20, 0x21, 0x22, 0x23,       // preload flags
    0x30, 0x31, 0x32, 0x33,       // default hints
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_track_load_settings_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("load"));
  EXPECT_EQ(atom.preload_start_time, 0x00010203);
  EXPECT_EQ(atom.preload_duration, 0x10111213);
  EXPECT_EQ(atom.preload_flags, 0x20212223);
  EXPECT_EQ(atom.default_hints, 0x30313233);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadTrackExcludeFromAutoselectionAtom) {
  MuTFFError err;
  MuTFFTrackExcludeFromAutoselectionAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 8;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,  // size
    't', 'x', 'a', 's',           // type
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_track_exclude_from_autoselection_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("txas"));
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadTrackReferenceAtom) {
  MuTFFError err;
  MuTFFTrackReferenceAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 40;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,  // size
    't', 'r', 'e', 'f',           // type
    0x00, 0x00, 0x00, 0x10,       // track reference type[0].size
    'a', 'b', 'c', 'd',           // track reference type[0].type
    0x00, 0x01, 0x02, 0x03,       // track reference type[0].track_ids[0]
    0x10, 0x11, 0x12, 0x13,       // track reference type[0].track_ids[1]
    0x00, 0x00, 0x00, 0x10,       // track reference type[1].size
    'e', 'f', 'g', 'h',           // track reference type[1].type
    0x20, 0x21, 0x22, 0x23,       // track reference type[1].track_ids[0]
    0x30, 0x31, 0x32, 0x33,       // track reference type[1].track_ids[1]
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_track_reference_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("tref"));
  EXPECT_EQ(atom.track_reference_type[0].size, 0x10);
  EXPECT_EQ(MuTFF_FOUR_C(atom.track_reference_type[0].type), MuTFF_FOUR_C("abcd"));
  EXPECT_EQ(atom.track_reference_type[0].track_ids[0], 0x00010203);
  EXPECT_EQ(atom.track_reference_type[0].track_ids[1], 0x10111213);
  EXPECT_EQ(atom.track_reference_type[1].size, 0x10);
  EXPECT_EQ(MuTFF_FOUR_C(atom.track_reference_type[1].type), MuTFF_FOUR_C("efgh"));
  EXPECT_EQ(atom.track_reference_type[1].track_ids[0], 0x20212223);
  EXPECT_EQ(atom.track_reference_type[1].track_ids[1], 0x30313233);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadTrackReferenceTypeAtom) {
  MuTFFError err;
  MuTFFTrackReferenceTypeAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 16;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,  // size
    'a', 'b', 'c', 'd',           // type
    0x00, 0x01, 0x02, 0x03,       // track_ids[0]
    0x10, 0x11, 0x12, 0x13,       // track_ids[1]
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_track_reference_type_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("abcd"));
  EXPECT_EQ(atom.track_ids[0], 0x00010203);
  EXPECT_EQ(atom.track_ids[1], 0x10111213);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadEditAtom) {
  MuTFFError err;
  MuTFFEditAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 48;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,  // size
    'e', 'd', 't', 's',           // type
    0x00, 0x00, 0x00, 40,         // elst.size
    'e', 'l', 's', 't',           // elst.type
    0x00,                         // elst.version
    0x00, 0x01, 0x02,             // elst.flags
    0x00, 0x00, 0x00, 0x02,       // elst.number of entries
    0x00, 0x01, 0x02, 0x03,       // elst.entry[0].track duration
    0x10, 0x11, 0x12, 0x13,       // elst.entry[0].media time
    0x20, 0x21, 0x22, 0x23,       // elst.entry[0].media rate
    0x30, 0x31, 0x32, 0x33,       // elst.entry[1].track duration
    0x40, 0x41, 0x42, 0x43,       // elst.entry[1].media time
    0x50, 0x51, 0x52, 0x53,       // elst.entry[1].media rate
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_edit_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("edts"));
  EXPECT_EQ(atom.edit_list_atom.size, data_size - 8);
  EXPECT_EQ(MuTFF_FOUR_C(atom.edit_list_atom.type), MuTFF_FOUR_C("elst"));
  EXPECT_EQ(atom.edit_list_atom.version_flags.version, 0x00);
  EXPECT_EQ(atom.edit_list_atom.version_flags.flags, 0x000102);
  EXPECT_EQ(atom.edit_list_atom.number_of_entries, 0x00000002);
  EXPECT_EQ(atom.edit_list_atom.edit_list_table[0].track_duration, 0x00010203);
  EXPECT_EQ(atom.edit_list_atom.edit_list_table[0].media_time, 0x10111213);
  EXPECT_EQ(atom.edit_list_atom.edit_list_table[0].media_rate, 0x20212223);
  EXPECT_EQ(atom.edit_list_atom.edit_list_table[1].track_duration, 0x30313233);
  EXPECT_EQ(atom.edit_list_atom.edit_list_table[1].media_time, 0x40414243);
  EXPECT_EQ(atom.edit_list_atom.edit_list_table[1].media_rate, 0x50515253);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadEditListAtom) {
  MuTFFError err;
  MuTFFEditListAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 40;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,  // size
    'e', 'l', 's', 't',           // type
    0x00,                         // version
    0x00, 0x01, 0x02,             // flags
    0x00, 0x00, 0x00, 0x02,       // number of entries
    0x00, 0x01, 0x02, 0x03,       // entry[0].track duration
    0x10, 0x11, 0x12, 0x13,       // entry[0].media time
    0x20, 0x21, 0x22, 0x23,       // entry[0].media rate
    0x30, 0x31, 0x32, 0x33,       // entry[1].track duration
    0x40, 0x41, 0x42, 0x43,       // entry[1].media time
    0x50, 0x51, 0x52, 0x53,       // entry[1].media rate
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_edit_list_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("elst"));
  EXPECT_EQ(atom.version_flags.version, 0x00);
  EXPECT_EQ(atom.version_flags.flags, 0x000102);
  EXPECT_EQ(atom.number_of_entries, 0x00000002);
  EXPECT_EQ(atom.edit_list_table[0].track_duration, 0x00010203);
  EXPECT_EQ(atom.edit_list_table[0].media_time, 0x10111213);
  EXPECT_EQ(atom.edit_list_table[0].media_rate, 0x20212223);
  EXPECT_EQ(atom.edit_list_table[1].track_duration, 0x30313233);
  EXPECT_EQ(atom.edit_list_table[1].media_time, 0x40414243);
  EXPECT_EQ(atom.edit_list_table[1].media_rate, 0x50515253);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadEditListEntry) {
  MuTFFError err;
  MuTFFEditListEntry entry;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 12;
  // clang-format off
  char data[data_size] = {
    0x00, 0x01, 0x02, 0x03,  // track duration
    0x10, 0x11, 0x12, 0x13,  // media time
    0x20, 0x21, 0x22, 0x23,  // media rate
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_edit_list_entry(fd, &entry);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(entry.track_duration, 0x00010203);
  EXPECT_EQ(entry.media_time, 0x10111213);
  EXPECT_EQ(entry.media_rate, 0x20212223);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadTrackMatteAtom) {
  MuTFFError err;
  MuTFFTrackMatteAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 44;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,         // size
    'm', 'a', 't', 't',                  // type
    0x00, 0x00, 0x00, 36,                // kmat.size
    'k', 'm', 'a', 't',                  // kmat.type
    0x00,                                // kmat.version
    0x00, 0x01, 0x02,                    // kmat.flags
    0x00, 0x00, 0x00, 0x14,              // kmat.desc.size
    'a', 'b', 'c', 'd',                  // kmat.desc.data format
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // kmat.desc.reserved
    0x00, 0x01,                          // kmat.desc.data reference index
    0x00, 0x01, 0x02, 0x03,              // kmat.desc.media-specific data
    0x00, 0x01, 0x02, 0x03,              // kmat.matte data
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_track_matte_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("matt"));
  EXPECT_EQ(atom.compressed_matte_atom.size, data_size - 8);
  EXPECT_EQ(MuTFF_FOUR_C(atom.compressed_matte_atom.type), MuTFF_FOUR_C("kmat"));
  EXPECT_EQ(atom.compressed_matte_atom.version_flags.version, 0x00);
  EXPECT_EQ(atom.compressed_matte_atom.version_flags.flags, 0x000102);
  EXPECT_EQ(atom.compressed_matte_atom.matte_image_description_structure.size, 0x14);
  EXPECT_EQ(atom.compressed_matte_atom.matte_image_description_structure.data_format,
            MuTFF_FOUR_C("abcd"));
  EXPECT_EQ(atom.compressed_matte_atom.matte_image_description_structure.data_reference_index, 0x0001);
  EXPECT_EQ(
      MuTFF_FOUR_C(atom.compressed_matte_atom.matte_image_description_structure.additional_data),
      0x00010203);
  EXPECT_EQ(MuTFF_FOUR_C(atom.compressed_matte_atom.matte_data), 0x00010203);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadCompressedMatteAtom) {
  MuTFFError err;
  MuTFFCompressedMatteAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 36;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,         // size
    'k', 'm', 'a', 't',                  // type
    0x00,                                // version
    0x00, 0x01, 0x02,                    // flags
    0x00, 0x00, 0x00, 0x14,              // desc.size
    'a', 'b', 'c', 'd',                  // desc.data format
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // desc.reserved
    0x00, 0x01,                          // desc.data reference index
    0x00, 0x01, 0x02, 0x03,              // desc.media-specific data
    0x00, 0x01, 0x02, 0x03,              // matte data
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_compressed_matte_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("kmat"));
  EXPECT_EQ(atom.version_flags.version, 0x00);
  EXPECT_EQ(atom.version_flags.flags, 0x000102);
  EXPECT_EQ(atom.matte_image_description_structure.size, 0x14);
  EXPECT_EQ(atom.matte_image_description_structure.data_format,
            MuTFF_FOUR_C("abcd"));
  EXPECT_EQ(atom.matte_image_description_structure.data_reference_index, 0x0001);
  EXPECT_EQ(
      MuTFF_FOUR_C(atom.matte_image_description_structure.additional_data),
      0x00010203);
  EXPECT_EQ(MuTFF_FOUR_C(atom.matte_data), 0x00010203);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadSampleDescription) {
  MuTFFError err;
  MuTFFSampleDescription desc;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 20;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,         // size
    'a', 'b', 'c', 'd',                  // data format
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // reserved
    0x00, 0x01,                          // data reference index
    0x00, 0x01, 0x02, 0x03,              // media-specific data
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_sample_description(fd, &desc);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(desc.size, data_size);
  EXPECT_EQ(desc.data_format, MuTFF_FOUR_C("abcd"));
  EXPECT_EQ(desc.data_reference_index, 0x0001);
  EXPECT_EQ(MuTFF_FOUR_C(desc.additional_data), 0x00010203);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadTrackApertureModeDimensionsAtom) {
  MuTFFError err;
  MuTFFTrackApertureModeDimensionsAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 68;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,  // size
    't', 'a', 'p', 't',           // type
    0x00, 0x00, 0x00, 0x14,       // clef.size
    'c', 'l', 'e', 'f',           // clef.type
    0x00,                         // clef.version
    0x00, 0x01, 0x02,             // clef.flags
    0x00, 0x01, 0x02, 0x03,       // clef.width
    0x10, 0x11, 0x12, 0x13,       // clef.height
    0x00, 0x00, 0x00, 0x14,       // enof.size
    'e', 'n', 'o', 'f',           // enof.type
    0x00,                         // enof.version
    0x00, 0x01, 0x02,             // enof.flags
    0x00, 0x01, 0x02, 0x03,       // enof.width
    0x10, 0x11, 0x12, 0x13,       // enof.height
    0x00, 0x00, 0x00, 0x14,       // prof.size
    'p', 'r', 'o', 'f',           // prof.type
    0x00,                         // prof.version
    0x00, 0x01, 0x02,             // prof.flags
    0x00, 0x01, 0x02, 0x03,       // prof.width
    0x10, 0x11, 0x12, 0x13,       // prof.height
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_track_aperture_mode_dimensions_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("tapt"));
  EXPECT_EQ(MuTFF_FOUR_C(atom.track_clean_aperture_dimension.type),
            MuTFF_FOUR_C("clef"));
  EXPECT_EQ(MuTFF_FOUR_C(atom.track_production_aperture_dimension.type),
            MuTFF_FOUR_C("prof"));
  EXPECT_EQ(MuTFF_FOUR_C(atom.track_encoded_pixels_dimension.type),
            MuTFF_FOUR_C("enof"));
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadTrackEncodedPixelsDimensionsAtom) {
  MuTFFError err;
  MuTFFTrackEncodedPixelsDimensionsAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 20;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,  // size
    'e', 'n', 'o', 'f',           // type
    0x00,                         // version
    0x00, 0x01, 0x02,             // flags
    0x00, 0x01, 0x02, 0x03,       // width
    0x10, 0x11, 0x12, 0x13,       // height
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_track_encoded_pixels_dimensions_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("enof"));
  EXPECT_EQ(atom.version_flags.version, 0x00);
  EXPECT_EQ(atom.version_flags.flags, 0x000102);
  EXPECT_EQ(atom.width, 0x00010203);
  EXPECT_EQ(atom.height, 0x10111213);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadTrackProductionApertureDimensionsAtom) {
  MuTFFError err;
  MuTFFTrackProductionApertureDimensionsAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 20;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,  // size
    'p', 'r', 'o', 'f',           // type
    0x00,                         // version
    0x00, 0x01, 0x02,             // flags
    0x00, 0x01, 0x02, 0x03,       // width
    0x10, 0x11, 0x12, 0x13,       // height
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_track_production_aperture_dimensions_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("prof"));
  EXPECT_EQ(atom.version_flags.version, 0x00);
  EXPECT_EQ(atom.version_flags.flags, 0x000102);
  EXPECT_EQ(atom.width, 0x00010203);
  EXPECT_EQ(atom.height, 0x10111213);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadTrackCleanApertureDimensionsAtom) {
  MuTFFError err;
  MuTFFTrackCleanApertureDimensionsAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 20;
  // clang-format off
  char data[data_size] = {
    0x00, 0x00, 0x00, data_size,  // size
    'c', 'l', 'e', 'f',           // type
    0x00,                         // version
    0x00, 0x01, 0x02,             // flags
    0x00, 0x01, 0x02, 0x03,       // width
    0x10, 0x11, 0x12, 0x13,       // height
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_track_clean_aperture_dimensions_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("clef"));
  EXPECT_EQ(atom.version_flags.version, 0x00);
  EXPECT_EQ(atom.version_flags.flags, 0x000102);
  EXPECT_EQ(atom.width, 0x00010203);
  EXPECT_EQ(atom.height, 0x10111213);
  EXPECT_EQ(ftell(fd), data_size);
}

TEST(MuTFF, ReadTrackHeaderAtom) {
  MuTFFError err;
  MuTFFTrackHeaderAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 92;
  // clang-format off
  char data[data_size] = {
      0x00, 0x00, 0x00, data_size,  // size
      't', 'k', 'h', 'd',           // type
      0x00,                         // version
      0x00, 0x01, 0x02,             // flags
      0x00, 0x01, 0x02, 0x03,       // creation time
      0x00, 0x01, 0x02, 0x03,       // modification time
      0x00, 0x01, 0x02, 0x03,       // track ID
      0x00, 0x00, 0x00, 0x00,       // reserved
      0x00, 0x01, 0x02, 0x03,       // duration
      0x00, 0x00, 0x00, 0x00,       // reserved
      0x00, 0x00, 0x00, 0x00,       // reserved
      0x00, 0x01,                   // layer
      0x00, 0x01,                   // alternate group
      0x00, 0x01,                   // volume
      0x00, 0x00,                   // reserved
      0x01, 0x02, 0x03, 0x04,       // matrix[0][0]
      0x05, 0x06, 0x07, 0x08,       // matrix[0][1]
      0x09, 0x0a, 0x0b, 0x0c,       // matrix[0][2]
      0x0d, 0x0e, 0x0f, 0x10,       // matrix[1][0]
      0x11, 0x12, 0x13, 0x14,       // matrix[1][1]
      0x15, 0x16, 0x17, 0x18,       // matrix[1][2]
      0x19, 0x1a, 0x1b, 0x1c,       // matrix[2][0]
      0x1d, 0x1e, 0x1f, 0x20,       // matrix[2][1]
      0x21, 0x22, 0x23, 0x24,       // matrix[2][2]
      0x00, 0x01, 0x02, 0x03,       // track width
      0x00, 0x01, 0x02, 0x03,       // track height
  };
  // clang-format on
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  err = mutff_read_track_header_atom(fd, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);

  EXPECT_EQ(atom.size, data_size);
  EXPECT_EQ(MuTFF_FOUR_C(atom.type), MuTFF_FOUR_C("tkhd"));
  EXPECT_EQ(atom.version_flags.version, 0x00);
  EXPECT_EQ(atom.version_flags.flags, 0x000102);
  EXPECT_EQ(atom.creation_time, 0x00010203);
  EXPECT_EQ(atom.modification_time, 0x00010203);
  EXPECT_EQ(atom.track_id, 0x00010203);
  EXPECT_EQ(atom.duration, 0x00010203);
  EXPECT_EQ(atom.layer, 0x0001);
  EXPECT_EQ(atom.alternate_group, 0x0001);
  EXPECT_EQ(atom.volume, 0x0001);
  for (size_t j = 0; j < 3; ++j) {
    for (size_t i = 0; i < 3; ++i) {
      const uint32_t exp_base = (3 * j + i) * 4 + 1;
      const uint32_t exp = (exp_base << 24) + ((exp_base + 1) << 16) +
                           ((exp_base + 2) << 8) + (exp_base + 3);
      EXPECT_EQ(atom.matrix_structure[j][i], exp);
    }
  }
  EXPECT_EQ(ftell(fd), data_size);
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
  EXPECT_EQ(sizeof(MuTFFCompositionShiftLeastGreatestAtom), 32);
  EXPECT_EQ(sizeof(MuTFFSampleToChunkTableEntry), 12);
  EXPECT_EQ(sizeof(MuTFFSoundMediaInformationHeaderAtom), 16);
  EXPECT_EQ(sizeof(MuTFFBaseMediaInfoAtom), 24);
  EXPECT_EQ(sizeof(MuTFFTextMediaInformationAtom), 44);
  EXPECT_EQ(sizeof(MuTFFBaseMediaInformationHeaderAtom), 76);
  EXPECT_EQ(sizeof(MuTFFBaseMediaInformationAtom), 84);
}
