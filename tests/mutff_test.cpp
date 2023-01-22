#include <gtest/gtest.h>
#include <stdio.h>
#include <string.h>

#include <cstdio>

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
  const MuTFFError ret = mutff_read_movie_file(fd, &movie_file);
  ASSERT_EQ(ret, 29036);

  EXPECT_EQ(movie_file.file_type_present, true);
  EXPECT_EQ(movie_file.movie_data_count, 1);
  EXPECT_EQ(movie_file.free_count, 0);
  EXPECT_EQ(movie_file.skip_count, 0);
  EXPECT_EQ(movie_file.wide_count, 1);
  EXPECT_EQ(movie_file.preview_present, false);
  EXPECT_EQ(ftell(fd), 29036);
}

TEST_F(TestMov, MovieAtom) {
  const size_t offset = 28330;
  MuTFFMovieAtom movie_atom;
  fseek(fd, offset, SEEK_SET);
  const MuTFFError ret = mutff_read_movie_atom(fd, &movie_atom);
  ASSERT_EQ(ret, 706);

  EXPECT_EQ(movie_atom.track_count, 1);

  EXPECT_EQ(ftell(fd), offset + 706);
}

TEST_F(TestMov, MovieHeaderAtom) {
  const size_t offset = 28338;
  MuTFFMovieHeaderAtom movie_header_atom;
  fseek(fd, offset, SEEK_SET);
  const MuTFFError ret = mutff_read_movie_header_atom(fd, &movie_header_atom);
  ASSERT_EQ(ret, 108);

  EXPECT_EQ(movie_header_atom.version, 0);
  EXPECT_EQ(movie_header_atom.flags, 0);
  EXPECT_EQ(movie_header_atom.creation_time, 0);
  EXPECT_EQ(movie_header_atom.modification_time, 0);
  EXPECT_EQ(movie_header_atom.time_scale, 1000);
  EXPECT_EQ(movie_header_atom.duration, 1167);
  EXPECT_EQ(movie_header_atom.preferred_rate.integral, 1);
  EXPECT_EQ(movie_header_atom.preferred_rate.fractional, 0);
  EXPECT_EQ(movie_header_atom.preferred_volume.integral, 1);
  EXPECT_EQ(movie_header_atom.preferred_volume.fractional, 0);
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

  EXPECT_EQ(ftell(fd), offset + 108);
}

TEST_F(TestMov, FileTypeAtom) {
  const size_t offset = 0;
  MuTFFFileTypeAtom file_type_atom;
  fseek(fd, offset, SEEK_SET);
  const MuTFFError ret = mutff_read_file_type_atom(fd, &file_type_atom);
  ASSERT_EQ(ret, 20);

  EXPECT_EQ(file_type_atom.major_brand, MuTFF_FOURCC('q', 't', ' ', ' '));
  EXPECT_EQ(file_type_atom.minor_version, 0x00000200);
  EXPECT_EQ(file_type_atom.compatible_brands_count, 1);
  EXPECT_EQ(file_type_atom.compatible_brands[0],
            MuTFF_FOURCC('q', 't', ' ', ' '));

  EXPECT_EQ(ftell(fd), offset + 20);
}

// @TODO: Add tests for special sizes
TEST_F(TestMov, MovieDataAtom) {
  const size_t offset = 28;
  MuTFFMovieDataAtom movie_data_atom;
  fseek(fd, offset, SEEK_SET);
  const MuTFFError ret = mutff_read_movie_data_atom(fd, &movie_data_atom);
  ASSERT_EQ(ret, 28302);

  EXPECT_EQ(ftell(fd), offset + 28302);
}

TEST_F(TestMov, WideAtom) {
  const size_t offset = 20;
  MuTFFWideAtom wide_atom;
  fseek(fd, offset, SEEK_SET);
  const MuTFFError ret = mutff_read_wide_atom(fd, &wide_atom);
  ASSERT_EQ(ret, 8);

  EXPECT_EQ(ftell(fd), offset + 8);
}

TEST_F(TestMov, TrackAtom) {
  const size_t offset = 28446;
  MuTFFTrackAtom track_atom;
  fseek(fd, offset, SEEK_SET);
  const MuTFFError ret = mutff_read_track_atom(fd, &track_atom);
  ASSERT_EQ(ret, 557);

  EXPECT_EQ(track_atom.track_aperture_mode_dimensions_present, false);
  EXPECT_EQ(track_atom.clipping_present, false);
  EXPECT_EQ(track_atom.track_matte_present, false);
  EXPECT_EQ(track_atom.edit_present, true);
  EXPECT_EQ(track_atom.track_reference_present, false);
  EXPECT_EQ(track_atom.track_exclude_from_autoselection_present, false);
  EXPECT_EQ(track_atom.track_load_settings_present, false);
  EXPECT_EQ(track_atom.track_input_map_present, false);
  EXPECT_EQ(track_atom.user_data_present, false);

  EXPECT_EQ(ftell(fd), offset + 557);
}

TEST_F(TestMov, TrackHeaderAtom) {
  const size_t offset = 28454;
  MuTFFTrackHeaderAtom atom;
  fseek(fd, offset, SEEK_SET);
  const MuTFFError ret = mutff_read_track_header_atom(fd, &atom);
  ASSERT_EQ(ret, 92);

  EXPECT_EQ(atom.version, 0x00);
  EXPECT_EQ(atom.flags, 0x000003);
  EXPECT_EQ(atom.creation_time, 0);
  EXPECT_EQ(atom.modification_time, 0);
  EXPECT_EQ(atom.track_id, 1);
  EXPECT_EQ(atom.duration, 0x048f);
  EXPECT_EQ(atom.layer, 0);
  EXPECT_EQ(atom.alternate_group, 0);
  EXPECT_EQ(atom.volume.integral, 0);
  EXPECT_EQ(atom.volume.fractional, 0);
  EXPECT_EQ(atom.matrix_structure[0][0], 0x00010000);
  EXPECT_EQ(atom.matrix_structure[0][1], 0);
  EXPECT_EQ(atom.matrix_structure[0][2], 0);
  EXPECT_EQ(atom.matrix_structure[1][0], 0);
  EXPECT_EQ(atom.matrix_structure[1][1], 0x00010000);
  EXPECT_EQ(atom.matrix_structure[1][2], 0);
  EXPECT_EQ(atom.matrix_structure[2][0], 0);
  EXPECT_EQ(atom.matrix_structure[2][1], 0);
  EXPECT_EQ(atom.matrix_structure[2][2], 0x40000000);
  EXPECT_EQ(atom.track_width.integral, 640);
  EXPECT_EQ(atom.track_width.fractional, 0);
  EXPECT_EQ(atom.track_height.integral, 480);
  EXPECT_EQ(atom.track_height.fractional, 0);

  EXPECT_EQ(ftell(fd), offset + 92);
}

TEST_F(TestMov, EditAtom) {
  const size_t offset = 28546;
  MuTFFEditAtom atom;
  fseek(fd, offset, SEEK_SET);
  const MuTFFError ret = mutff_read_edit_atom(fd, &atom);
  ASSERT_EQ(ret, 36);

  EXPECT_EQ(atom.edit_list_atom.version, 0x00);
  EXPECT_EQ(atom.edit_list_atom.flags, 0x000000);
  EXPECT_EQ(atom.edit_list_atom.number_of_entries, 1);
  EXPECT_EQ(atom.edit_list_atom.edit_list_table[0].track_duration, 0x048f);
  EXPECT_EQ(atom.edit_list_atom.edit_list_table[0].media_time, 0);
  EXPECT_EQ(atom.edit_list_atom.edit_list_table[0].media_rate.integral, 1);
  EXPECT_EQ(atom.edit_list_atom.edit_list_table[0].media_rate.fractional, 0);

  EXPECT_EQ(ftell(fd), offset + 36);
}

TEST_F(TestMov, MediaAtom) {
  const size_t offset = 28582;
  MuTFFMediaAtom atom;
  fseek(fd, offset, SEEK_SET);
  const MuTFFError ret = mutff_read_media_atom(fd, &atom);
  ASSERT_EQ(ret, 421);

  EXPECT_EQ(atom.extended_language_tag_present, false);
  EXPECT_EQ(atom.handler_reference_present, true);
  EXPECT_EQ(atom.media_information_present, true);
  EXPECT_EQ(atom.user_data_present, false);

  EXPECT_EQ(ftell(fd), offset + 421);
}

TEST_F(TestMov, MediaHeaderAtom) {
  const size_t offset = 28590;
  MuTFFMediaHeaderAtom atom;
  fseek(fd, offset, SEEK_SET);
  const MuTFFError ret = mutff_read_media_header_atom(fd, &atom);
  ASSERT_EQ(ret, 32);

  EXPECT_EQ(ftell(fd), offset + 32);
}

TEST_F(TestMov, MediaHandlerReferenceAtom) {
  const size_t offset = 28622;
  MuTFFHandlerReferenceAtom atom;
  fseek(fd, offset, SEEK_SET);
  const MuTFFError ret = mutff_read_handler_reference_atom(fd, &atom);
  ASSERT_EQ(ret, 45);

  EXPECT_EQ(atom.version, 0x00);
  EXPECT_EQ(atom.flags, 0x000000);
  EXPECT_EQ(atom.component_type, MuTFF_FOURCC('m', 'h', 'l', 'r'));
  EXPECT_EQ(atom.component_subtype, MuTFF_FOURCC('v', 'i', 'd', 'e'));
  EXPECT_EQ(atom.component_manufacturer, 0);
  EXPECT_EQ(atom.component_flags, 0);
  EXPECT_EQ(atom.component_flags_mask, 0);
  const char *component_name = "\fVideoHandler";
  for (size_t i = 0; i < 13; ++i) {
    EXPECT_EQ(atom.component_name[i], component_name[i]);
  }

  EXPECT_EQ(ftell(fd), offset + 45);
}

TEST_F(TestMov, VideoMediaInformationHeader) {
  const size_t offset = 28675;
  MuTFFVideoMediaInformationHeaderAtom atom;
  fseek(fd, offset, SEEK_SET);
  const MuTFFError ret =
      mutff_read_video_media_information_header_atom(fd, &atom);
  ASSERT_EQ(ret, 20);

  EXPECT_EQ(ftell(fd), offset + 20);
}

TEST_F(TestMov, VideoMediaInformationHandlerReference) {
  const size_t offset = 28695;
  MuTFFHandlerReferenceAtom atom;
  fseek(fd, offset, SEEK_SET);
  const MuTFFError ret = mutff_read_handler_reference_atom(fd, &atom);
  ASSERT_EQ(ret, 44);

  EXPECT_EQ(atom.version, 0x00);
  EXPECT_EQ(atom.flags, 0x000000);
  EXPECT_EQ(atom.component_type, MuTFF_FOURCC('d', 'h', 'l', 'r'));
  EXPECT_EQ(atom.component_subtype, MuTFF_FOURCC('u', 'r', 'l', ' '));
  EXPECT_EQ(atom.component_manufacturer, 0);
  EXPECT_EQ(atom.component_flags, 0);
  EXPECT_EQ(atom.component_flags_mask, 0);
  const char *component_name = "\vDataHandler";
  for (size_t i = 0; i < 12; ++i) {
    EXPECT_EQ(atom.component_name[i], component_name[i]);
  }

  EXPECT_EQ(ftell(fd), offset + 44);
}

TEST_F(TestMov, VideoMediaInformationDataInformation) {
  const size_t offset = 28739;
  MuTFFDataInformationAtom atom;
  fseek(fd, offset, SEEK_SET);
  const MuTFFError ret = mutff_read_data_information_atom(fd, &atom);
  ASSERT_EQ(ret, 36);

  EXPECT_EQ(ftell(fd), offset + 36);
}

TEST_F(TestMov, VideoMediaInformationSampleTable) {
  const size_t offset = 28775;
  MuTFFSampleTableAtom atom;
  fseek(fd, offset, SEEK_SET);
  const MuTFFError ret = mutff_read_sample_table_atom(fd, &atom);
  ASSERT_EQ(ret, 228);

  EXPECT_EQ(ftell(fd), offset + 228);
}

TEST_F(TestMov, VideoMediaInformationSampleTableDescription) {
  const size_t offset = 28783;
  MuTFFSampleDescriptionAtom atom;
  fseek(fd, offset, SEEK_SET);
  const MuTFFError ret = mutff_read_sample_description_atom(fd, &atom);
  ASSERT_EQ(ret, 128);

  EXPECT_EQ(ftell(fd), offset + 128);
}

TEST_F(TestMov, TimeToSample) {
  const size_t offset = 28911;
  MuTFFTimeToSampleAtom atom;
  fseek(fd, offset, SEEK_SET);
  const MuTFFError ret = mutff_read_time_to_sample_atom(fd, &atom);
  ASSERT_EQ(ret, 24);

  EXPECT_EQ(ftell(fd), offset + 24);
}

TEST_F(TestMov, SampleToChunk) {
  const size_t offset = 28935;
  MuTFFSampleToChunkAtom atom;
  fseek(fd, offset, SEEK_SET);
  const MuTFFError ret = mutff_read_sample_to_chunk_atom(fd, &atom);
  ASSERT_EQ(ret, 28);

  EXPECT_EQ(ftell(fd), offset + 28);
}

TEST_F(TestMov, SampleSize) {
  const size_t offset = 28963;
  MuTFFSampleSizeAtom atom;
  fseek(fd, offset, SEEK_SET);
  const MuTFFError ret = mutff_read_sample_size_atom(fd, &atom);
  ASSERT_EQ(ret, 20);

  EXPECT_EQ(atom.version, 0x00);
  EXPECT_EQ(atom.flags, 0x000000);
  EXPECT_EQ(atom.sample_size, 0x07e5);
  EXPECT_EQ(atom.number_of_entries, 0x0e);

  EXPECT_EQ(ftell(fd), offset + 20);
}

// {{{1 sample dependency flags atom unit tests
static const uint32_t sdtp_test_data_size = 14;
// clang-format off
static const unsigned char sdtp_test_data[sdtp_test_data_size] = {
    sdtp_test_data_size >> 24,  // size
    sdtp_test_data_size >> 16,
    sdtp_test_data_size >> 8,
    sdtp_test_data_size,
    's', 'd', 't', 'p',         // type
    0x00,                       // version
    0x00, 0x01, 0x02,           // flags
    0x10, 0x11,                 // sample size table
};
// clang-format on
// clang-format off
static const MuTFFSampleDependencyFlagsAtom sdtp_test_struct = {
    0x00,                    // version
    0x000102,                // flags
    2,
    {                        // sample dependency flags table
      0x10, 0x11,
    }
};
// clang-format on

TEST(MuTFF, WriteSampleDependencyFlagsAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret =
      mutff_write_sample_dependency_flags_atom(fd, &sdtp_test_struct);
  ASSERT_EQ(ret, sdtp_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, sdtp_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], sdtp_test_data[i]);
  }
}

TEST(MuTFF, ReadSampleDependencyFlagsAtom) {
  MuTFFError ret;
  MuTFFSampleDependencyFlagsAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(sdtp_test_data, sdtp_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_sample_dependency_flags_atom(fd, &atom);
  ASSERT_EQ(ret, sdtp_test_data_size);

  EXPECT_EQ(atom.version, sdtp_test_struct.version);
  EXPECT_EQ(atom.flags, sdtp_test_struct.flags);
  EXPECT_EQ(atom.sample_dependency_flags_table[0],
            sdtp_test_struct.sample_dependency_flags_table[0]);
  EXPECT_EQ(atom.sample_dependency_flags_table[1],
            sdtp_test_struct.sample_dependency_flags_table[1]);
  EXPECT_EQ(ftell(fd), sdtp_test_data_size);
}
// }}}1

// {{{1 chunk offset atom unit tests
static const uint32_t stco_test_data_size = 20;
// clang-format off
static const unsigned char stco_test_data[stco_test_data_size] = {
    stco_test_data_size >> 24,  // size
    stco_test_data_size >> 16,
    stco_test_data_size >> 8,
    stco_test_data_size,
    's', 't', 'c', 'o',         // type
    0x00,                       // version
    0x00, 0x01, 0x02,           // flags
    0x00, 0x00, 0x00, 0x01,     // number of entries
    0x10, 0x11, 0x12, 0x13,     // sample size table
};
// clang-format on
// clang-format off
static const MuTFFChunkOffsetAtom stco_test_struct = {
    0x00,                  // version
    0x000102,              // flags
    1,                     // number of entries
    {
      0x10111213,          // chunk offset table[0]
    }
};
// clang-format on

TEST(MuTFF, WriteChunkOffsetAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret = mutff_write_chunk_offset_atom(fd, &stco_test_struct);
  ASSERT_EQ(ret, stco_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, stco_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], stco_test_data[i]);
  }
}

TEST(MuTFF, ReadChunkOffsetAtom) {
  MuTFFError ret;
  MuTFFChunkOffsetAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(stco_test_data, stco_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_chunk_offset_atom(fd, &atom);
  ASSERT_EQ(ret, stco_test_data_size);

  EXPECT_EQ(atom.version, stco_test_struct.version);
  EXPECT_EQ(atom.flags, stco_test_struct.flags);
  EXPECT_EQ(atom.number_of_entries, stco_test_struct.number_of_entries);
  EXPECT_EQ(atom.chunk_offset_table[0], stco_test_struct.chunk_offset_table[0]);
  EXPECT_EQ(ftell(fd), stco_test_data_size);
}
// }}}1

// {{{1 sample size atom unit tests
static const uint32_t stsz_test_data_size = 24;
// clang-format off
static const unsigned char stsz_test_data[stsz_test_data_size] = {
    stsz_test_data_size >> 24,  // size
    stsz_test_data_size >> 16,
    stsz_test_data_size >> 8,
    stsz_test_data_size,
    's', 't', 's', 'z',         // type
    0x00,                       // version
    0x00, 0x01, 0x02,           // flags
    0x00, 0x00, 0x00, 0x00,     // sample size
    0x00, 0x00, 0x00, 0x01,     // number of entries
    0x10, 0x11, 0x12, 0x13,     // sample size table
};
// clang-format on
// clang-format off
static const MuTFFSampleSizeAtom stsz_test_struct = {
    0x00,
    0x000102,
    0,
    1,
    {
      0x10111213,
    }
};
// clang-format on

TEST(MuTFF, WriteSampleSizeAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret = mutff_write_sample_size_atom(fd, &stsz_test_struct);
  ASSERT_EQ(ret, stsz_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, stsz_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], stsz_test_data[i]);
  }
}

TEST(MuTFF, ReadSampleSizeAtom) {
  MuTFFError ret;
  MuTFFSampleSizeAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(stsz_test_data, stsz_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_sample_size_atom(fd, &atom);
  ASSERT_EQ(ret, stsz_test_data_size);

  EXPECT_EQ(atom.version, stsz_test_struct.version);
  EXPECT_EQ(atom.flags, stsz_test_struct.flags);
  EXPECT_EQ(atom.sample_size, stsz_test_struct.sample_size);
  EXPECT_EQ(atom.number_of_entries, stsz_test_struct.number_of_entries);
  EXPECT_EQ(atom.sample_size_table[0], stsz_test_struct.sample_size_table[0]);
  EXPECT_EQ(ftell(fd), stsz_test_data_size);
}
// }}}1

// {{{1 sample-to-chunk atom unit tests
static const uint32_t stsc_test_data_size = 40;
// clang-format off
static const unsigned char stsc_test_data[stsc_test_data_size] = {
    stsc_test_data_size >> 24,  // size
    stsc_test_data_size >> 16,
    stsc_test_data_size >> 8,
    stsc_test_data_size,
    's', 't', 's', 'c',         // type
    0x00,                       // version
    0x00, 0x01, 0x02,           // flags
    0x00, 0x00, 0x00, 0x02,     // number of entries
    0x00, 0x01, 0x02, 0x03,     // table[0].first chunk
    0x10, 0x11, 0x12, 0x13,     // table[0].samples per chunk
    0x20, 0x21, 0x22, 0x23,     // table[0].sample description ID
    0x30, 0x31, 0x32, 0x33,     // table[1].first chunk
    0x40, 0x41, 0x42, 0x43,     // table[1].samples per chunk
    0x50, 0x51, 0x52, 0x53,     // table[1].sample description ID
};
// clang-format on
// clang-format off
static const MuTFFSampleToChunkAtom stsc_test_struct = {
    0x00,
    0x000102,
    2,
    {
      {
        0x00010203,
        0x10111213,
        0x20212223,
      },
      {
        0x30313233,
        0x40414243,
        0x50515253,
      }
    }
};
// clang-format on

TEST(MuTFF, WriteSampleToChunkAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret =
      mutff_write_sample_to_chunk_atom(fd, &stsc_test_struct);
  ASSERT_EQ(ret, stsc_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, stsc_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], stsc_test_data[i]);
  }
}

TEST(MuTFF, ReadSampleToChunkAtom) {
  MuTFFError ret;
  MuTFFSampleToChunkAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(stsc_test_data, stsc_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_sample_to_chunk_atom(fd, &atom);
  ASSERT_EQ(ret, stsc_test_data_size);

  EXPECT_EQ(atom.version, stsc_test_struct.version);
  EXPECT_EQ(atom.flags, stsc_test_struct.flags);
  EXPECT_EQ(atom.number_of_entries, stsc_test_struct.number_of_entries);
  EXPECT_EQ(atom.sample_to_chunk_table[0].first_chunk,
            stsc_test_struct.sample_to_chunk_table[0].first_chunk);
  EXPECT_EQ(atom.sample_to_chunk_table[0].samples_per_chunk,
            stsc_test_struct.sample_to_chunk_table[0].samples_per_chunk);
  EXPECT_EQ(atom.sample_to_chunk_table[0].sample_description_id,
            stsc_test_struct.sample_to_chunk_table[0].sample_description_id);
  EXPECT_EQ(atom.sample_to_chunk_table[1].first_chunk,
            stsc_test_struct.sample_to_chunk_table[1].first_chunk);
  EXPECT_EQ(atom.sample_to_chunk_table[1].samples_per_chunk,
            stsc_test_struct.sample_to_chunk_table[1].samples_per_chunk);
  EXPECT_EQ(atom.sample_to_chunk_table[1].sample_description_id,
            stsc_test_struct.sample_to_chunk_table[1].sample_description_id);
  EXPECT_EQ(ftell(fd), stsc_test_data_size);
}
// }}}1

// {{{1 sample-to-chunk table entry unit tests
static const uint32_t stsc_entry_test_data_size = 12;
// clang-format off
static const unsigned char stsc_entry_test_data[stsc_entry_test_data_size] = {
    0x00, 0x01, 0x02, 0x03,  // first chunk
    0x10, 0x11, 0x12, 0x13,  // samples per chunk
    0x20, 0x21, 0x22, 0x23,  // sample description ID
};
// clang-format on
// clang-format off
static const MuTFFSampleToChunkTableEntry stsc_entry_test_struct = {
    0x00010203,  // first chunk
    0x10111213,  // samples per chunk
    0x20212223,  // sample description ID
};
// clang-format on

TEST(MuTFF, WriteSampleToChunkTableEntry) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret =
      mutff_write_sample_to_chunk_table_entry(fd, &stsc_entry_test_struct);
  ASSERT_EQ(ret, stsc_entry_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, stsc_entry_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], stsc_entry_test_data[i]);
  }
}

TEST(MuTFF, ReadSampleToChunkTableEntry) {
  MuTFFError ret;
  MuTFFSampleToChunkTableEntry entry;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(stsc_entry_test_data, stsc_entry_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_sample_to_chunk_table_entry(fd, &entry);
  ASSERT_EQ(ret, stsc_entry_test_data_size);

  EXPECT_EQ(entry.first_chunk, stsc_entry_test_struct.first_chunk);
  EXPECT_EQ(entry.samples_per_chunk, stsc_entry_test_struct.samples_per_chunk);
  EXPECT_EQ(entry.sample_description_id,
            stsc_entry_test_struct.sample_description_id);
  EXPECT_EQ(ftell(fd), stsc_entry_test_data_size);
}
// }}}1

// {{{1 partial sync sample atom unit tests
static const uint32_t stps_test_data_size = 24;
// clang-format off
static const unsigned char stps_test_data[stps_test_data_size] = {
    stps_test_data_size >> 24,  // size
    stps_test_data_size >> 16,
    stps_test_data_size >> 8,
    stps_test_data_size,
    's', 't', 'p', 's',         // type
    0x00,                       // version
    0x00, 0x01, 0x02,           // flags
    0x00, 0x00, 0x00, 0x02,     // number of entries
    0x00, 0x01, 0x02, 0x03,     // table[0].sample count
    0x10, 0x11, 0x12, 0x13,     // table[1].composition offset
};
// clang-format on
// clang-format off
static const MuTFFPartialSyncSampleAtom stps_test_struct = {
    0x00,                    // version
    0x000102,                // flags
    2,                       // number of entries
    {
      0x00010203,            // table[0]
      0x10111213,            // table[1]
    }
};
// clang-format on

TEST(MuTFF, WritePartialSyncSampleAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret =
      mutff_write_partial_sync_sample_atom(fd, &stps_test_struct);
  ASSERT_EQ(ret, stps_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, stps_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], stps_test_data[i]);
  }
}

TEST(MuTFF, ReadPartialSyncSampleAtom) {
  MuTFFError ret;
  MuTFFPartialSyncSampleAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(stps_test_data, stps_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_partial_sync_sample_atom(fd, &atom);
  ASSERT_EQ(ret, stps_test_data_size);

  EXPECT_EQ(atom.version, stps_test_struct.version);
  EXPECT_EQ(atom.flags, stps_test_struct.flags);
  EXPECT_EQ(atom.entry_count, stps_test_struct.entry_count);
  EXPECT_EQ(atom.partial_sync_sample_table[0],
            stps_test_struct.partial_sync_sample_table[0]);
  EXPECT_EQ(atom.partial_sync_sample_table[1],
            stps_test_struct.partial_sync_sample_table[1]);
  EXPECT_EQ(ftell(fd), stps_test_data_size);
}
// }}}1

// {{{1 sync sample atom unit tests
static const uint32_t stss_test_data_size = 24;
// clang-format off
static const unsigned char stss_test_data[stss_test_data_size] = {
    stss_test_data_size >> 24,  // size
    stss_test_data_size >> 16,
    stss_test_data_size >> 8,
    stss_test_data_size,
    's', 't', 's', 's',         // type
    0x00,                       // version
    0x00, 0x01, 0x02,           // flags
    0x00, 0x00, 0x00, 0x02,     // number of entries
    0x00, 0x01, 0x02, 0x03,     // table[0].sample count
    0x10, 0x11, 0x12, 0x13,     // table[1].composition offset
};
// clang-format on
// clang-format off
static const MuTFFSyncSampleAtom stss_test_struct = {
    0x00,                    // version
    0x000102,                // flags
    2,                       // number of entries
    {
      0x00010203,            // table[0]
      0x10111213,            // table[1]
    }
};
// clang-format on

TEST(MuTFF, WriteSyncSampleAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret = mutff_write_sync_sample_atom(fd, &stss_test_struct);
  ASSERT_EQ(ret, stss_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, stss_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], stss_test_data[i]);
  }
}

TEST(MuTFF, ReadSyncSampleAtom) {
  MuTFFError ret;
  MuTFFSyncSampleAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(stss_test_data, stss_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_sync_sample_atom(fd, &atom);
  ASSERT_EQ(ret, stss_test_data_size);

  EXPECT_EQ(atom.version, stss_test_struct.version);
  EXPECT_EQ(atom.flags, stss_test_struct.flags);
  EXPECT_EQ(atom.number_of_entries, stss_test_struct.number_of_entries);
  EXPECT_EQ(atom.sync_sample_table[0], stss_test_struct.sync_sample_table[0]);
  EXPECT_EQ(atom.sync_sample_table[1], stss_test_struct.sync_sample_table[1]);
  EXPECT_EQ(ftell(fd), stss_test_data_size);
}
// }}}1

// {{{1 composition shift least greatest atom unit tests
static const uint32_t cslg_test_data_size = 32;
// clang-format off
static const unsigned char cslg_test_data[cslg_test_data_size] = {
    cslg_test_data_size >> 24,  // size
    cslg_test_data_size >> 16,
    cslg_test_data_size >> 8,
    cslg_test_data_size,
    'c', 's', 'l', 'g',         // type
    0x00,                       // version
    0x00, 0x01, 0x02,           // flags
    0x00, 0x01, 0x02, 0x03,     // composition offset to display offset shift
    0x10, 0x11, 0x12, 0x13,     // least display offset
    0x20, 0x21, 0x22, 0x23,     // greatest display offset
    0x30, 0x31, 0x32, 0x33,     // start display time
    0x40, 0x41, 0x42, 0x43,     // end display time
};
// clang-format on
// clang-format off
static const MuTFFCompositionShiftLeastGreatestAtom cslg_test_struct = {
    0x00,                  // version
    0x000102,              // flags
    0x00010203,            // composition offset to display offset shift
    0x10111213,            // least display offset
    0x20212223,            // greatest display offset
    0x30313233,            // start display time
    0x40414243,            // end display time
};
// clang-format on

TEST(MuTFF, WriteCompositionShiftLeastGreatestAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret =
      mutff_write_composition_shift_least_greatest_atom(fd, &cslg_test_struct);
  ASSERT_EQ(ret, cslg_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, cslg_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], cslg_test_data[i]);
  }
}

TEST(MuTFF, ReadCompositionShiftLeastGreatestAtom) {
  MuTFFError ret;
  MuTFFCompositionShiftLeastGreatestAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(cslg_test_data, cslg_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_composition_shift_least_greatest_atom(fd, &atom);
  ASSERT_EQ(ret, cslg_test_data_size);

  EXPECT_EQ(atom.version, cslg_test_struct.version);
  EXPECT_EQ(atom.flags, cslg_test_struct.flags);
  EXPECT_EQ(atom.composition_offset_to_display_offset_shift,
            cslg_test_struct.composition_offset_to_display_offset_shift);
  EXPECT_EQ(atom.least_display_offset, cslg_test_struct.least_display_offset);
  EXPECT_EQ(atom.greatest_display_offset,
            cslg_test_struct.greatest_display_offset);
  EXPECT_EQ(atom.display_start_time, cslg_test_struct.display_start_time);
  EXPECT_EQ(atom.display_end_time, cslg_test_struct.display_end_time);
  EXPECT_EQ(ftell(fd), cslg_test_data_size);
}
// }}}1

// {{{1 composition offset atom unit tests
static const uint32_t ctts_test_data_size = 32;
// clang-format off
static const unsigned char ctts_test_data[ctts_test_data_size] = {
    ctts_test_data_size >> 24,  // size
    ctts_test_data_size >> 16,
    ctts_test_data_size >> 8,
    ctts_test_data_size,
    'c', 't', 't', 's',         // type
    0x00,                       // version
    0x00, 0x01, 0x02,           // flags
    0x00, 0x00, 0x00, 0x02,     // number of entries
    0x00, 0x01, 0x02, 0x03,     // table[0].sample count
    0x10, 0x11, 0x12, 0x13,     // table[0].sample duration
    0x20, 0x21, 0x22, 0x23,     // table[1].sample count
    0x30, 0x31, 0x32, 0x33,     // table[1].sample duration
};
// clang-format on
// clang-format off
static const MuTFFCompositionOffsetAtom ctts_test_struct = {
    0x00,                    // version
    0x000102,                // flags
    2,                       // number of entries
    {
      {
        0x00010203,
        0x10111213,
      },
      {
        0x20212223,
        0x30313233,
      }
    }
};
// clang-format on

TEST(MuTFF, WriteCompositionOffsetAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret =
      mutff_write_composition_offset_atom(fd, &ctts_test_struct);
  ASSERT_EQ(ret, ctts_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, ctts_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], ctts_test_data[i]);
  }
}

TEST(MuTFF, ReadCompositionOffsetAtom) {
  MuTFFError ret;
  MuTFFCompositionOffsetAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(ctts_test_data, ctts_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_composition_offset_atom(fd, &atom);
  ASSERT_EQ(ret, ctts_test_data_size);

  EXPECT_EQ(atom.version, ctts_test_struct.version);
  EXPECT_EQ(atom.flags, ctts_test_struct.flags);
  EXPECT_EQ(atom.entry_count, ctts_test_struct.entry_count);
  EXPECT_EQ(atom.composition_offset_table[0].sample_count,
            ctts_test_struct.composition_offset_table[0].sample_count);
  EXPECT_EQ(atom.composition_offset_table[0].composition_offset,
            ctts_test_struct.composition_offset_table[0].composition_offset);
  EXPECT_EQ(atom.composition_offset_table[1].sample_count,
            ctts_test_struct.composition_offset_table[1].sample_count);
  EXPECT_EQ(atom.composition_offset_table[1].composition_offset,
            ctts_test_struct.composition_offset_table[1].composition_offset);
  EXPECT_EQ(ftell(fd), ctts_test_data_size);
}
// }}}1

// {{{1 composition offset table entry unit tests
static const uint32_t ctts_entry_test_data_size = 8;
// clang-format off
static const unsigned char ctts_entry_test_data[ctts_entry_test_data_size] = {
    0x00, 0x01, 0x02, 0x03,  // sample count
    0x10, 0x11, 0x12, 0x13,  // composition offset
};
// clang-format on
// clang-format off
static const MuTFFCompositionOffsetTableEntry ctts_entry_test_struct = {
    0x00010203,
    0x10111213,
};
// clang-format on

TEST(MuTFF, WriteCompositionOffsetTableEntry) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret =
      mutff_write_composition_offset_table_entry(fd, &ctts_entry_test_struct);
  ASSERT_EQ(ret, ctts_entry_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, ctts_entry_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], ctts_entry_test_data[i]);
  }
}

TEST(MuTFF, ReadCompositionOffsetTableEntry) {
  MuTFFError ret;
  MuTFFCompositionOffsetTableEntry entry;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(ctts_entry_test_data, ctts_entry_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_composition_offset_table_entry(fd, &entry);
  ASSERT_EQ(ret, ctts_entry_test_data_size);

  EXPECT_EQ(entry.sample_count, ctts_entry_test_struct.sample_count);
  EXPECT_EQ(entry.composition_offset,
            ctts_entry_test_struct.composition_offset);
  EXPECT_EQ(ftell(fd), ctts_entry_test_data_size);
}
// }}}1

// {{{1 time-to-sample atom unit tests
static const uint32_t stts_test_data_size = 32;
// clang-format off
static const unsigned char stts_test_data[stts_test_data_size] = {
    stts_test_data_size >> 24,  // size
    stts_test_data_size >> 16,
    stts_test_data_size >> 8,
    stts_test_data_size,
    's', 't', 't', 's',         // type
    0x00,                       // version
    0x00, 0x01, 0x02,           // flags
    0x00, 0x00, 0x00, 0x02,     // number of entries
    0x00, 0x01, 0x02, 0x03,     // table[0].sample count
    0x10, 0x11, 0x12, 0x13,     // table[0].sample duration
    0x20, 0x21, 0x22, 0x23,     // table[1].sample count
    0x30, 0x31, 0x32, 0x33,     // table[1].sample duration
};
// clang-format on
// clang-format off
static const MuTFFTimeToSampleAtom stts_test_struct = {
    0x00,                    // version
    0x000102,                // flags
    2,                       // number of entries
    {
      {
        0x00010203,
        0x10111213,
      },
      {
        0x20212223,
        0x30313233,
      }
    }
};
// clang-format on

TEST(MuTFF, WriteTimeToSampleAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret = mutff_write_time_to_sample_atom(fd, &stts_test_struct);
  ASSERT_EQ(ret, stts_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, stts_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], stts_test_data[i]);
  }
}

TEST(MuTFF, ReadTimeToSampleAtom) {
  MuTFFError ret;
  MuTFFTimeToSampleAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(stts_test_data, stts_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_time_to_sample_atom(fd, &atom);
  ASSERT_EQ(ret, stts_test_data_size);

  EXPECT_EQ(atom.version, stts_test_struct.version);
  EXPECT_EQ(atom.flags, stts_test_struct.flags);
  EXPECT_EQ(atom.number_of_entries, stts_test_struct.number_of_entries);
  EXPECT_EQ(atom.time_to_sample_table[0].sample_count,
            stts_test_struct.time_to_sample_table[0].sample_count);
  EXPECT_EQ(atom.time_to_sample_table[0].sample_duration,
            stts_test_struct.time_to_sample_table[0].sample_duration);
  EXPECT_EQ(atom.time_to_sample_table[1].sample_count,
            stts_test_struct.time_to_sample_table[1].sample_count);
  EXPECT_EQ(atom.time_to_sample_table[1].sample_duration,
            stts_test_struct.time_to_sample_table[1].sample_duration);
  EXPECT_EQ(ftell(fd), stts_test_data_size);
}
// }}}1

// {{{1 time to sample table entry unit tests
static const uint32_t stts_entry_test_data_size = 8;
// clang-format off
static const unsigned char stts_entry_test_data[stts_entry_test_data_size] = {
    0x00, 0x01, 0x02, 0x03,  // sample count
    0x10, 0x11, 0x12, 0x13,  // sample duration
};
// clang-format on
// clang-format off
static const MuTFFTimeToSampleTableEntry stts_entry_test_struct = {
    0x00010203,
    0x10111213,
};
// clang-format on

TEST(MuTFF, WriteTimeToSampleTableEntry) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret =
      mutff_write_time_to_sample_table_entry(fd, &stts_entry_test_struct);
  ASSERT_EQ(ret, stts_entry_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, stts_entry_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], stts_entry_test_data[i]);
  }
}

TEST(MuTFF, ReadTimeToSampleTableEntry) {
  MuTFFError ret;
  MuTFFTimeToSampleTableEntry entry;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(stts_entry_test_data, stts_entry_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_time_to_sample_table_entry(fd, &entry);
  ASSERT_EQ(ret, stts_entry_test_data_size);

  EXPECT_EQ(entry.sample_count, stts_entry_test_struct.sample_count);
  EXPECT_EQ(entry.sample_duration, stts_entry_test_struct.sample_duration);
  EXPECT_EQ(ftell(fd), stts_entry_test_data_size);
}
// }}}1

// {{{1 sample description atom unit tests
static const uint32_t stsd_test_data_size = 56;
// clang-format off
static const unsigned char stsd_test_data[stsd_test_data_size] = {
    stsd_test_data_size >> 24,           // size
    stsd_test_data_size >> 16,
    stsd_test_data_size >> 8,
    stsd_test_data_size,
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
// clang-format off
static const MuTFFSampleDescriptionAtom stsd_test_struct = {
    0x00,
    0x000102,
    2,
    {
      {
        20,
        MuTFF_FOURCC('a', 'b', 'c', 'd'),
        0x0001,
        {
          0x00, 0x01, 0x02, 0x03,
        }
      },
      {
        20,
        MuTFF_FOURCC('e', 'f', 'g', 'h'),
        0x1011,
        {
          0x10, 0x11, 0x12, 0x13,
        }
      },
    }
};
// clang-format on

TEST(MuTFF, WriteSampleDescriptionAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret =
      mutff_write_sample_description_atom(fd, &stsd_test_struct);
  ASSERT_EQ(ret, stsd_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, stsd_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], stsd_test_data[i]);
  }
}

TEST(MuTFF, ReadSampleDescriptionAtom) {
  MuTFFError ret;
  MuTFFSampleDescriptionAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(stsd_test_data, stsd_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_sample_description_atom(fd, &atom);
  ASSERT_EQ(ret, stsd_test_data_size);

  EXPECT_EQ(atom.version, stsd_test_struct.version);
  EXPECT_EQ(atom.flags, stsd_test_struct.flags);
  EXPECT_EQ(atom.number_of_entries, stsd_test_struct.number_of_entries);
  for (uint32_t i = 0; i < stsd_test_struct.number_of_entries; ++i) {
    EXPECT_EQ(atom.sample_description_table[i].size,
              stsd_test_struct.sample_description_table[i].size);
    EXPECT_EQ(atom.sample_description_table[i].data_format,
              stsd_test_struct.sample_description_table[i].data_format);
    EXPECT_EQ(
        atom.sample_description_table[i].data_reference_index,
        stsd_test_struct.sample_description_table[i].data_reference_index);
    const uint32_t data_size =
        stsd_test_struct.sample_description_table[i].size - 16;
    for (uint32_t j = 0; j < data_size; ++j) {
      EXPECT_EQ(
          atom.sample_description_table[i].additional_data[j],
          stsd_test_struct.sample_description_table[i].additional_data[j]);
    }
  }
  EXPECT_EQ(ftell(fd), stsd_test_data_size);
}
// }}}1

// {{{1 data reference atom unit tests
static const uint32_t dref_test_data_size = 48;
// clang-format off
static const unsigned char dref_test_data[dref_test_data_size] = {
    dref_test_data_size >> 24,  // size
    dref_test_data_size >> 16,
    dref_test_data_size >> 8,
    dref_test_data_size,
    'd', 'r', 'e', 'f',         // type
    0x00,                       // version
    0x00, 0x01, 0x02,           // flag
    0x00, 0x00, 0x00, 0x02,     // number of entries
    0x00, 0x00, 0x00, 16,       // data references[0].size
    'a', 'b', 'c', 'd',         // data references[0] type
    0x00,                       // data references[0] version
    0x00, 0x01, 0x02,           // data references[0] flags
    0x00, 0x01, 0x02, 0x03,     // data references[0] data
    0x00, 0x00, 0x00, 16,       // data references[1].size
    'e', 'f', 'g', 'h',         // data references[1] type
    0x10,                       // data references[1] version
    0x10, 0x11, 0x12,           // data references[1] flags
    0x10, 0x11, 0x12, 0x13,     // data references[1] data
};
// clang-format on
// clang-format off
static const MuTFFDataReferenceAtom dref_test_struct = {
    0x00,
    0x000102,
    2,
    {
      {
        MuTFF_FOURCC('a', 'b', 'c', 'd'),
        0x00,
        0x000102,
        4,
        {
          0x00, 0x01, 0x02, 0x03,
        },
      },
      {
        MuTFF_FOURCC('e', 'f', 'g', 'h'),
        0x10,
        0x101112,
        4,
        {
          0x10, 0x11, 0x12, 0x13,
        },
      },
    }
};
// clang-format on

TEST(MuTFF, WriteDataReferenceAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret = mutff_write_data_reference_atom(fd, &dref_test_struct);
  ASSERT_EQ(ret, dref_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, dref_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], dref_test_data[i]);
  }
}

TEST(MuTFF, ReadDataReferenceAtom) {
  MuTFFError ret;
  MuTFFDataReferenceAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(dref_test_data, dref_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_data_reference_atom(fd, &atom);
  ASSERT_EQ(ret, dref_test_data_size);

  EXPECT_EQ(atom.number_of_entries, dref_test_struct.number_of_entries);
  EXPECT_EQ(atom.data_references[0].type,
            dref_test_struct.data_references[0].type);
  EXPECT_EQ(atom.data_references[0].version,
            dref_test_struct.data_references[0].version);
  EXPECT_EQ(atom.data_references[0].flags,
            dref_test_struct.data_references[0].flags);
  EXPECT_EQ(atom.data_references[0].data_size,
            dref_test_struct.data_references[0].data_size);
  for (size_t i = 0; i < dref_test_struct.data_references[0].data_size; ++i) {
    EXPECT_EQ(dref_test_struct.data_references[0].data[i],
              dref_test_struct.data_references[0].data[i]);
  }
  EXPECT_EQ(atom.data_references[1].type,
            dref_test_struct.data_references[1].type);
  EXPECT_EQ(atom.data_references[1].version,
            dref_test_struct.data_references[1].version);
  EXPECT_EQ(atom.data_references[1].flags,
            dref_test_struct.data_references[1].flags);
  EXPECT_EQ(atom.data_references[1].data_size,
            dref_test_struct.data_references[1].data_size);
  for (size_t i = 0; i < dref_test_struct.data_references[1].data_size; ++i) {
    EXPECT_EQ(dref_test_struct.data_references[1].data[i],
              dref_test_struct.data_references[1].data[i]);
  }
  EXPECT_EQ(ftell(fd), dref_test_data_size);
}
// }}}1

// {{{1 data information atom unit tests
static const uint32_t dinf_test_data_size = 56;
// clang-format off
static const unsigned char dinf_test_data[dinf_test_data_size] = {
    dinf_test_data_size >> 24,  // size
    dinf_test_data_size >> 16,
    dinf_test_data_size >> 8,
    dinf_test_data_size,
    'd', 'i', 'n', 'f',         // type
    0x00, 0x00, 0x00, 48,       // size
    'd', 'r', 'e', 'f',         // type
    0x00,                       // version
    0x00, 0x01, 0x02,           // flag
    0x00, 0x00, 0x00, 0x02,     // number of entries
    0x00, 0x00, 0x00, 16,       // data references[0].size
    'a', 'b', 'c', 'd',         // data references[0] type
    0x00,                       // data references[0] version
    0x00, 0x01, 0x02,           // data references[0] flags
    0x00, 0x01, 0x02, 0x03,     // data references[0] data
    0x00, 0x00, 0x00, 16,       // data references[1].size
    'e', 'f', 'g', 'h',         // data references[1] type
    0x10,                       // data references[1] version
    0x10, 0x11, 0x12,           // data references[1] flags
    0x10, 0x11, 0x12, 0x13,     // data references[1] data
};
// clang-format on
// clang-format off
static const MuTFFDataInformationAtom dinf_test_struct = {
    dref_test_struct,        // data reference
};
// clang-format on

TEST(MuTFF, WriteDataInformationAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret =
      mutff_write_data_information_atom(fd, &dinf_test_struct);
  ASSERT_EQ(ret, dinf_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, dinf_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], dinf_test_data[i]);
  }
}

TEST(MuTFF, ReadDataInformationAtom) {
  MuTFFError ret;
  MuTFFDataInformationAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(dinf_test_data, dinf_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_data_information_atom(fd, &atom);
  ASSERT_EQ(ret, dinf_test_data_size);

  EXPECT_EQ(atom.data_reference.number_of_entries,
            dinf_test_struct.data_reference.number_of_entries);
  for (uint32_t i = 0; i < dinf_test_struct.data_reference.number_of_entries;
       ++i) {
    EXPECT_EQ(atom.data_reference.data_references[i].type,
              dinf_test_struct.data_reference.data_references[i].type);
    EXPECT_EQ(atom.data_reference.data_references[i].version,
              dinf_test_struct.data_reference.data_references[i].version);
    EXPECT_EQ(atom.data_reference.data_references[i].flags,
              dinf_test_struct.data_reference.data_references[i].flags);
    EXPECT_EQ(atom.data_reference.data_references[i].data_size,
              dinf_test_struct.data_reference.data_references[i].data_size);
    const size_t data_size =
        dinf_test_struct.data_reference.data_references[i].data_size;
    for (uint32_t j = 0; j < data_size; ++j) {
      EXPECT_EQ(atom.data_reference.data_references[i].data[j],
                dinf_test_struct.data_reference.data_references[i].data[j]);
    }
  }
  EXPECT_EQ(ftell(fd), dinf_test_data_size);
}
// }}}1

// {{{1 data reference unit tests
static const uint32_t data_ref_test_data_size = 16;
// clang-format off
static const unsigned char data_ref_test_data[data_ref_test_data_size] = {
    data_ref_test_data_size >> 24,  // size
    data_ref_test_data_size >> 16,
    data_ref_test_data_size >> 8,
    data_ref_test_data_size,
    'a', 'b', 'c', 'd',           // type
    0x00,                         // version
    0x00, 0x01, 0x02,             // flags
    0x00, 0x01, 0x02, 0x03,       // data
};
// clang-format on
// clang-format off
static const MuTFFDataReference data_ref_test_struct = {
    MuTFF_FOURCC('a', 'b', 'c', 'd'),        // type
    0x00,                        // version
    0x000102,                    // flags
    4,                           // data size
    {                            // data
      0x00, 0x01, 0x02, 0x03,
    },
};
// clang-format on

TEST(MuTFF, WriteDataReference) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret = mutff_write_data_reference(fd, &data_ref_test_struct);
  ASSERT_EQ(ret, data_ref_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, data_ref_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], data_ref_test_data[i]);
  }
}

TEST(MuTFF, ReadDataReference) {
  MuTFFError ret;
  MuTFFDataReference ref;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(data_ref_test_data, data_ref_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_data_reference(fd, &ref);
  ASSERT_EQ(ret, data_ref_test_data_size);

  EXPECT_EQ(ref.type, data_ref_test_struct.type);
  EXPECT_EQ(ref.version, data_ref_test_struct.version);
  EXPECT_EQ(ref.flags, data_ref_test_struct.flags);
  EXPECT_EQ(ref.data_size, data_ref_test_struct.data_size);
  for (size_t i = 0; i < data_ref_test_struct.data_size; ++i) {
    EXPECT_EQ(ref.data[i], data_ref_test_struct.data[i]);
  }
  EXPECT_EQ(ftell(fd), data_ref_test_data_size);
}
// }}}1

// {{{1 base media info atom unit tests
static const uint32_t gmin_test_data_size = 24;
// clang-format off
static const unsigned char gmin_test_data[gmin_test_data_size] = {
    gmin_test_data_size >> 24,  // size
    gmin_test_data_size >> 16,
    gmin_test_data_size >> 8,
    gmin_test_data_size,
    'g', 'm', 'i', 'n',         // type
    0x00,                       // version
    0x00, 0x01, 0x02,           // flags
    0x00, 0x01,                 // graphics mode
    0x10, 0x11,                 // opcolor[0]
    0x20, 0x21,                 // opcolor[1]
    0x30, 0x31,                 // opcolor[2]
    0x40, 0x41,                 // balance
    0x00, 0x00,                 // reserved
};
// clang-format on
// clang-format off
static const MuTFFBaseMediaInfoAtom gmin_test_struct = {
    0x00,                    // version
    0x000102,                // flags
    0x0001,                  // graphics mode
    0x1011,                  // opcolor[0]
    0x2021,                  // opcolor[1]
    0x3031,                  // opcolor[2]
    0x4041,                  // balance
};
// clang-format on

TEST(MuTFF, WriteBaseMediaInfoAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret =
      mutff_write_base_media_info_atom(fd, &gmin_test_struct);
  ASSERT_EQ(ret, gmin_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, gmin_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], gmin_test_data[i]);
  }
}

TEST(MuTFF, ReadBaseMediaInfoAtom) {
  MuTFFError ret;
  MuTFFBaseMediaInfoAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(gmin_test_data, gmin_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_base_media_info_atom(fd, &atom);
  ASSERT_EQ(ret, gmin_test_data_size);

  EXPECT_EQ(atom.version, gmin_test_struct.version);
  EXPECT_EQ(atom.flags, gmin_test_struct.flags);
  EXPECT_EQ(atom.graphics_mode, gmin_test_struct.graphics_mode);
  EXPECT_EQ(atom.opcolor[0], gmin_test_struct.opcolor[0]);
  EXPECT_EQ(atom.opcolor[1], gmin_test_struct.opcolor[1]);
  EXPECT_EQ(atom.opcolor[2], gmin_test_struct.opcolor[2]);
  EXPECT_EQ(atom.balance, gmin_test_struct.balance);
  EXPECT_EQ(ftell(fd), gmin_test_data_size);
}
// }}}1

// {{{1 text media information atom unit tests
static const uint32_t text_test_data_size = 44;
// clang-format off
static const unsigned char text_test_data[text_test_data_size] = {
    text_test_data_size >> 24,  // size
    text_test_data_size >> 16,
    text_test_data_size >> 8,
    text_test_data_size,
    't', 'e', 'x', 't',         // type
    0x01, 0x02, 0x03, 0x04,     // matrix[0][0]
    0x05, 0x06, 0x07, 0x08,     // matrix[0][1]
    0x09, 0x0a, 0x0b, 0x0c,     // matrix[0][2]
    0x0d, 0x0e, 0x0f, 0x10,     // matrix[1][0]
    0x11, 0x12, 0x13, 0x14,     // matrix[1][1]
    0x15, 0x16, 0x17, 0x18,     // matrix[1][2]
    0x19, 0x1a, 0x1b, 0x1c,     // matrix[2][0]
    0x1d, 0x1e, 0x1f, 0x20,     // matrix[2][1]
    0x21, 0x22, 0x23, 0x24,     // matrix[2][2]
};
// clang-format on
// clang-format off
static const MuTFFTextMediaInformationAtom text_test_struct = {
    {
      {
        0x01020304,
        0x05060708,
        0x090a0b0c,
      },
      {
        0x0d0e0f10,
        0x11121314,
        0x15161718,
      },
      {
        0x191a1b1c,
        0x1d1e1f20,
        0x21222324,
      },
    }
};
// clang-format on

TEST(MuTFF, WriteTextMediaInformationAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret =
      mutff_write_text_media_information_atom(fd, &text_test_struct);
  ASSERT_EQ(ret, text_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, text_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], text_test_data[i]);
  }
}

TEST(MuTFF, ReadTextMediaInformationAtom) {
  MuTFFError ret;
  MuTFFTextMediaInformationAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(text_test_data, text_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_text_media_information_atom(fd, &atom);
  ASSERT_EQ(ret, text_test_data_size);

  for (size_t j = 0; j < 3; ++j) {
    for (size_t i = 0; i < 3; ++i) {
      EXPECT_EQ(atom.matrix_structure[j][i],
                text_test_struct.matrix_structure[j][i]);
    }
  }
  EXPECT_EQ(ftell(fd), text_test_data_size);
}
// }}}1

// {{{1 base media information header atom unit tests
static const uint32_t gmhd_test_data_size = 76;
// clang-format off
static const unsigned char gmhd_test_data[gmhd_test_data_size] = {
    gmhd_test_data_size >> 24,  // size
    gmhd_test_data_size >> 16,
    gmhd_test_data_size >> 8,
    gmhd_test_data_size,
    'g', 'm', 'h', 'd',         // type
    0x00, 0x00, 0x00, 24,       // size
    'g', 'm', 'i', 'n',         // type
    0x00,                       // version
    0x00, 0x01, 0x02,           // flags
    0x00, 0x01,                 // graphics mode
    0x10, 0x11,                 // opcolor[0]
    0x20, 0x21,                 // opcolor[1]
    0x30, 0x31,                 // opcolor[2]
    0x40, 0x41,                 // balance
    0x00, 0x00,                 // reserved
    0x00, 0x00, 0x00, 44,       // size
    't', 'e', 'x', 't',         // type
    0x01, 0x02, 0x03, 0x04,     // matrix[0][0]
    0x05, 0x06, 0x07, 0x08,     // matrix[0][1]
    0x09, 0x0a, 0x0b, 0x0c,     // matrix[0][2]
    0x0d, 0x0e, 0x0f, 0x10,     // matrix[1][0]
    0x11, 0x12, 0x13, 0x14,     // matrix[1][1]
    0x15, 0x16, 0x17, 0x18,     // matrix[1][2]
    0x19, 0x1a, 0x1b, 0x1c,     // matrix[2][0]
    0x1d, 0x1e, 0x1f, 0x20,     // matrix[2][1]
    0x21, 0x22, 0x23, 0x24,     // matrix[2][2]
};
// clang-format on
// clang-format off
static const MuTFFBaseMediaInformationHeaderAtom gmhd_test_struct = {
    gmin_test_struct,
    true,
    text_test_struct,
};
// clang-format on

TEST(MuTFF, WriteBaseMediaInformationHeaderAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret =
      mutff_write_base_media_information_header_atom(fd, &gmhd_test_struct);
  ASSERT_EQ(ret, gmhd_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, gmhd_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], gmhd_test_data[i]);
  }
}

TEST(MuTFF, ReadBaseMediaInformationHeaderAtom) {
  MuTFFError ret;
  MuTFFBaseMediaInformationHeaderAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(gmhd_test_data, gmhd_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_base_media_information_header_atom(fd, &atom);
  ASSERT_EQ(ret, gmhd_test_data_size);

  EXPECT_EQ(atom.base_media_info.version,
            gmhd_test_struct.base_media_info.version);
  EXPECT_EQ(atom.base_media_info.flags, gmhd_test_struct.base_media_info.flags);
  EXPECT_EQ(atom.base_media_info.graphics_mode,
            gmhd_test_struct.base_media_info.graphics_mode);
  EXPECT_EQ(atom.base_media_info.opcolor[0],
            gmhd_test_struct.base_media_info.opcolor[0]);
  EXPECT_EQ(atom.base_media_info.opcolor[1],
            gmhd_test_struct.base_media_info.opcolor[1]);
  EXPECT_EQ(atom.base_media_info.opcolor[2],
            gmhd_test_struct.base_media_info.opcolor[2]);
  EXPECT_EQ(atom.base_media_info.balance,
            gmhd_test_struct.base_media_info.balance);

  ASSERT_EQ(atom.text_media_information_present, true);
  for (size_t j = 0; j < 3; ++j) {
    for (size_t i = 0; i < 3; ++i) {
      EXPECT_EQ(atom.text_media_information.matrix_structure[j][i],
                gmhd_test_struct.text_media_information.matrix_structure[j][i]);
    }
  }
  EXPECT_EQ(ftell(fd), gmhd_test_data_size);
}
// }}}1

// {{{1 base media information atom unit tests
static const uint32_t minf_test_data_size = 84;
// clang-format off
static const unsigned char minf_test_data[minf_test_data_size] = {
    minf_test_data_size >> 24,  // size
    minf_test_data_size >> 16,
    minf_test_data_size >> 8,
    minf_test_data_size,
    'm', 'i', 'n', 'f',         // type
    0x00, 0x00, 0x00, 76,       // size
    'g', 'm', 'h', 'd',         // type
    0x00, 0x00, 0x00, 24,       // size
    'g', 'm', 'i', 'n',         // type
    0x00,                       // version
    0x00, 0x01, 0x02,           // flags
    0x00, 0x01,                 // graphics mode
    0x10, 0x11,                 // opcolor[0]
    0x20, 0x21,                 // opcolor[1]
    0x30, 0x31,                 // opcolor[2]
    0x40, 0x41,                 // balance
    0x00, 0x00,                 // reserved
    0x00, 0x00, 0x00, 44,       // size
    't', 'e', 'x', 't',         // type
    0x01, 0x02, 0x03, 0x04,     // matrix[0][0]
    0x05, 0x06, 0x07, 0x08,     // matrix[0][1]
    0x09, 0x0a, 0x0b, 0x0c,     // matrix[0][2]
    0x0d, 0x0e, 0x0f, 0x10,     // matrix[1][0]
    0x11, 0x12, 0x13, 0x14,     // matrix[1][1]
    0x15, 0x16, 0x17, 0x18,     // matrix[1][2]
    0x19, 0x1a, 0x1b, 0x1c,     // matrix[2][0]
    0x1d, 0x1e, 0x1f, 0x20,     // matrix[2][1]
    0x21, 0x22, 0x23, 0x24,     // matrix[2][2]
};
// clang-format on
// clang-format off
static const MuTFFBaseMediaInformationAtom minf_test_struct = {
    gmhd_test_struct,        // base media information header
};
// clang-format on

TEST(MuTFF, WriteBaseMediaInformationAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret =
      mutff_write_base_media_information_atom(fd, &minf_test_struct);
  ASSERT_EQ(ret, minf_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, minf_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], minf_test_data[i]);
  }
}

TEST(MuTFF, ReadBaseMediaInformationAtom) {
  MuTFFError ret;
  MuTFFBaseMediaInformationAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(minf_test_data, minf_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_base_media_information_atom(fd, &atom);
  ASSERT_EQ(ret, minf_test_data_size);

  EXPECT_EQ(
      atom.base_media_information_header.base_media_info.version,
      minf_test_struct.base_media_information_header.base_media_info.version);
  EXPECT_EQ(
      atom.base_media_information_header.base_media_info.flags,
      minf_test_struct.base_media_information_header.base_media_info.flags);
  EXPECT_EQ(atom.base_media_information_header.base_media_info.graphics_mode,
            minf_test_struct.base_media_information_header.base_media_info
                .graphics_mode);
  EXPECT_EQ(atom.base_media_information_header.base_media_info.opcolor[0],
            minf_test_struct.base_media_information_header.base_media_info
                .opcolor[0]);
  EXPECT_EQ(atom.base_media_information_header.base_media_info.opcolor[1],
            minf_test_struct.base_media_information_header.base_media_info
                .opcolor[1]);
  EXPECT_EQ(atom.base_media_information_header.base_media_info.opcolor[2],
            minf_test_struct.base_media_information_header.base_media_info
                .opcolor[2]);
  EXPECT_EQ(
      atom.base_media_information_header.base_media_info.balance,
      minf_test_struct.base_media_information_header.base_media_info.balance);
  for (size_t j = 0; j < 3; ++j) {
    for (size_t i = 0; i < 3; ++i) {
      EXPECT_EQ(atom.base_media_information_header.text_media_information
                    .matrix_structure[j][i],
                minf_test_struct.base_media_information_header
                    .text_media_information.matrix_structure[j][i]);
    }
  }
  EXPECT_EQ(ftell(fd), minf_test_data_size);
}
// }}}1

// {{{1 sound media information header atom unit tests
static const uint32_t smhd_test_data_size = 16;
// clang-format off
static const unsigned char smhd_test_data[smhd_test_data_size] = {
    smhd_test_data_size >> 24,  // size
    smhd_test_data_size >> 16,
    smhd_test_data_size >> 8,
    smhd_test_data_size,
    's', 'm', 'h', 'd',         // type
    0x00,                       // version
    0x00, 0x01, 0x02,           // flags
    0xff, 0xfe,                 // balance
    0x00, 0x00,                 // reserved
};
// clang-format on
// clang-format off
static const MuTFFSoundMediaInformationHeaderAtom smhd_test_struct = {
    0x00,                    // version
    0x000102,                // flags
    -2,                      // balance

};
// clang-format on

TEST(MuTFF, WriteSoundMediaInformationHeaderAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret =
      mutff_write_sound_media_information_header_atom(fd, &smhd_test_struct);
  ASSERT_EQ(ret, smhd_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, smhd_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], smhd_test_data[i]);
  }
}

TEST(MuTFF, ReadSoundMediaInformationHeaderAtom) {
  MuTFFError ret;
  MuTFFSoundMediaInformationHeaderAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(smhd_test_data, smhd_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_sound_media_information_header_atom(fd, &atom);
  ASSERT_EQ(ret, smhd_test_data_size);

  EXPECT_EQ(atom.version, smhd_test_struct.version);
  EXPECT_EQ(atom.flags, smhd_test_struct.flags);
  EXPECT_EQ(atom.balance, smhd_test_struct.balance);
  EXPECT_EQ(ftell(fd), smhd_test_data_size);
}
// }}}1

// {{{1 video media information header atom unit tests
static const uint32_t vmhd_test_data_size = 20;
// clang-format off
static const unsigned char vmhd_test_data[vmhd_test_data_size] = {
    vmhd_test_data_size >> 24,  // size
    vmhd_test_data_size >> 16,
    vmhd_test_data_size >> 8,
    vmhd_test_data_size,
    'v', 'm', 'h', 'd',         // type
    0x00,                       // version
    0x00, 0x01, 0x02,           // flags
    0x00, 0x01,                 // graphics mode
    0x10, 0x11,                 // opcolor[0]
    0x20, 0x21,                 // opcolor[1]
    0x30, 0x31,                 // opcolor[2]
};
// clang-format on
// clang-format off
static const MuTFFVideoMediaInformationHeaderAtom vmhd_test_struct = {
    0x00,                    // version
    0x000102,                // flags
    0x0001,                  // graphics mode
    0x1011,                  // opcolor[0]
    0x2021,                  // opcolor[1]
    0x3031,                  // opcolor[2]
};
// clang-format on

TEST(MuTFF, WriteVideoMediaInformationHeaderAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret =
      mutff_write_video_media_information_header_atom(fd, &vmhd_test_struct);
  ASSERT_EQ(ret, vmhd_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, vmhd_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], vmhd_test_data[i]);
  }
}

TEST(MuTFF, ReadVideoMediaInformationHeaderAtom) {
  MuTFFError ret;
  MuTFFVideoMediaInformationHeaderAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(vmhd_test_data, vmhd_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_video_media_information_header_atom(fd, &atom);
  ASSERT_EQ(ret, vmhd_test_data_size);

  EXPECT_EQ(atom.version, vmhd_test_struct.version);
  EXPECT_EQ(atom.flags, vmhd_test_struct.flags);
  EXPECT_EQ(atom.graphics_mode, vmhd_test_struct.graphics_mode);
  EXPECT_EQ(atom.opcolor[0], vmhd_test_struct.opcolor[0]);
  EXPECT_EQ(atom.opcolor[1], vmhd_test_struct.opcolor[1]);
  EXPECT_EQ(atom.opcolor[2], vmhd_test_struct.opcolor[2]);
  EXPECT_EQ(ftell(fd), vmhd_test_data_size);
}
// }}}1

// {{{1 handler reference atom unit tests
static const uint32_t hdlr_test_data_size = 36;
// clang-format off
static const unsigned char hdlr_test_data[hdlr_test_data_size] = {
    hdlr_test_data_size >> 24,  // size
    hdlr_test_data_size >> 16,
    hdlr_test_data_size >> 8,
    hdlr_test_data_size,
    'h', 'd', 'l', 'r',         // type
    0x00,                       // version
    0x00, 0x01, 0x02,           // flags
    0x00, 0x01, 0x02, 0x03,     // component type
    0x10, 0x11, 0x12, 0x13,     // component subtype
    0x20, 0x21, 0x22, 0x23,     // component manufacturer
    0x30, 0x31, 0x32, 0x33,     // component flags
    0x40, 0x41, 0x42, 0x43,     // component flags mask
    'a', 'b', 'c', 'd',         // component name
};
// clang-format on
// clang-format off
static const MuTFFHandlerReferenceAtom hdlr_test_struct = {
    0x00,        // version
    0x000102,    // flags
    0x00010203,  // component type
    0x10111213,  // component subtype
    0x20212223,  // component manufacturer
    0x30313233,  // component flags
    0x40414243,  // component flags mask
    "abcd",      // component name
};
// clang-format on

TEST(MuTFF, WriteHandlerReferenceAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret =
      mutff_write_handler_reference_atom(fd, &hdlr_test_struct);
  ASSERT_EQ(ret, hdlr_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, hdlr_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], hdlr_test_data[i]);
  }
}

TEST(MuTFF, ReadHandlerReferenceAtom) {
  MuTFFError ret;
  MuTFFHandlerReferenceAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(hdlr_test_data, hdlr_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_handler_reference_atom(fd, &atom);
  ASSERT_EQ(ret, hdlr_test_data_size);

  EXPECT_EQ(atom.version, hdlr_test_struct.version);
  EXPECT_EQ(atom.flags, hdlr_test_struct.flags);
  EXPECT_EQ(atom.component_type, hdlr_test_struct.component_type);
  EXPECT_EQ(atom.component_subtype, hdlr_test_struct.component_subtype);
  EXPECT_EQ(atom.component_manufacturer,
            hdlr_test_struct.component_manufacturer);
  EXPECT_EQ(atom.component_flags, hdlr_test_struct.component_flags);
  EXPECT_EQ(atom.component_flags_mask, hdlr_test_struct.component_flags_mask);
  EXPECT_STREQ(atom.component_name, hdlr_test_struct.component_name);
  EXPECT_EQ(ftell(fd), hdlr_test_data_size);
}
// }}}1

// {{{1 extended language tag atom unit tests
static const uint32_t elng_test_data_size = 18;
// clang-format off
static const unsigned char elng_test_data[elng_test_data_size] = {
    elng_test_data_size >> 24,      // size
    elng_test_data_size >> 16,
    elng_test_data_size >> 8,
    elng_test_data_size,
    'e', 'l', 'n', 'g',             // type
    0x00,                           // version
    0x00, 0x01, 0x02,               // flags
    'e', 'n', '-', 'U', 'S', '\0',  // language tag string
};
// clang-format on
// clang-format off
static const MuTFFExtendedLanguageTagAtom elng_test_struct = {
    0x00,                    // version
    0x000102,                // flags
    "en-US",                 // language tag string
};
// clang-format on

TEST(MuTFF, WriteExtendedLanguageTagAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret =
      mutff_write_extended_language_tag_atom(fd, &elng_test_struct);
  ASSERT_EQ(ret, elng_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, elng_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], elng_test_data[i]);
  }
}

TEST(MuTFF, ReadExtendedLanguageTagAtom) {
  MuTFFError ret;
  MuTFFExtendedLanguageTagAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(elng_test_data, elng_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_extended_language_tag_atom(fd, &atom);
  ASSERT_EQ(ret, elng_test_data_size);

  EXPECT_EQ(atom.version, elng_test_struct.version);
  EXPECT_EQ(atom.flags, elng_test_struct.flags);
  EXPECT_STREQ(atom.language_tag_string, elng_test_struct.language_tag_string);
  EXPECT_EQ(ftell(fd), elng_test_data_size);
}
// }}}1

// {{{1 media header atom unit tests
static const uint32_t mdhd_test_data_size = 32;
// clang-format off
static const unsigned char mdhd_test_data[mdhd_test_data_size] = {
    mdhd_test_data_size >> 24,  // size
    mdhd_test_data_size >> 16,
    mdhd_test_data_size >> 8,
    mdhd_test_data_size,
    'm', 'd', 'h', 'd',         // type
    0x00,                       // version
    0x00, 0x01, 0x02,           // flags
    0x00, 0x01, 0x02, 0x03,     // creation time
    0x10, 0x11, 0x12, 0x13,     // modification time
    0x20, 0x21, 0x22, 0x23,     // time scale
    0x30, 0x31, 0x32, 0x33,     // duration
    0x40, 0x41,                 // language
    0x50, 0x51,                 // quality
};
// clang-format on
// clang-format off
static const MuTFFMediaHeaderAtom mdhd_test_struct = {
    0x00,                    // version
    0x000102,                // flags
    0x00010203,              // creation time
    0x10111213,              // modification time
    0x20212223,              // time scale
    0x30313233,              // duration
    0x4041,                  // language
    0x5051,                  // quality
};
// clang-format on

TEST(MuTFF, WriteMediaHeaderAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret = mutff_write_media_header_atom(fd, &mdhd_test_struct);
  ASSERT_EQ(ret, mdhd_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, mdhd_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], mdhd_test_data[i]);
  }
}

TEST(MuTFF, ReadMediaHeaderAtom) {
  MuTFFError ret;
  MuTFFMediaHeaderAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(mdhd_test_data, mdhd_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_media_header_atom(fd, &atom);
  ASSERT_EQ(ret, mdhd_test_data_size);

  EXPECT_EQ(atom.version, mdhd_test_struct.version);
  EXPECT_EQ(atom.flags, mdhd_test_struct.flags);
  EXPECT_EQ(atom.creation_time, mdhd_test_struct.creation_time);
  EXPECT_EQ(atom.modification_time, mdhd_test_struct.modification_time);
  EXPECT_EQ(atom.time_scale, mdhd_test_struct.time_scale);
  EXPECT_EQ(atom.duration, mdhd_test_struct.duration);
  EXPECT_EQ(atom.language, mdhd_test_struct.language);
  EXPECT_EQ(atom.quality, mdhd_test_struct.quality);
  EXPECT_EQ(ftell(fd), mdhd_test_data_size);
}
// }}}1

// {{{1 object id atom unit tests
static const uint32_t obid_test_data_size = 12;
// clang-format off
static const unsigned char obid_test_data[obid_test_data_size] = {
    obid_test_data_size >> 24,  // size
    obid_test_data_size >> 16,
    obid_test_data_size >> 8,
    obid_test_data_size,
    'o', 'b', 'i', 'd',         // type
    0x00, 0x01, 0x02, 0x03,     // object id
};
// clang-format on
// clang-format off
static const MuTFFObjectIDAtom obid_test_struct = {
    0x00010203,              // object id
};
// clang-format on

TEST(MuTFF, WriteObjectIDAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret = mutff_write_object_id_atom(fd, &obid_test_struct);
  ASSERT_EQ(ret, obid_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, obid_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], obid_test_data[i]);
  }
}

TEST(MuTFF, ReadObjectIDAtom) {
  MuTFFError ret;
  MuTFFObjectIDAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(obid_test_data, obid_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_object_id_atom(fd, &atom);
  ASSERT_EQ(ret, obid_test_data_size);

  EXPECT_EQ(atom.object_id, obid_test_struct.object_id);
  EXPECT_EQ(ftell(fd), obid_test_data_size);
}
// }}}1

// {{{1 input type atom unit tests
static const uint32_t ty_test_data_size = 12;
// clang-format off
static const unsigned char ty_test_data[ty_test_data_size] = {
    ty_test_data_size >> 24,  // size
    ty_test_data_size >> 16,
    ty_test_data_size >> 8,
    ty_test_data_size,
    '\0', '\0', 't', 'y',     // type
    0x00, 0x01, 0x02, 0x03,   // input type
};
// clang-format on
// clang-format off
static const MuTFFInputTypeAtom ty_test_struct = {
    0x00010203,              // input type
};
// clang-format on

TEST(MuTFF, WriteInputTypeAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret = mutff_write_input_type_atom(fd, &ty_test_struct);
  ASSERT_EQ(ret, ty_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, ty_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], ty_test_data[i]);
  }
}

TEST(MuTFF, ReadInputTypeAtom) {
  MuTFFError ret;
  MuTFFInputTypeAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(ty_test_data, ty_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_input_type_atom(fd, &atom);
  ASSERT_EQ(ret, ty_test_data_size);

  EXPECT_EQ(atom.input_type, ty_test_struct.input_type);
  EXPECT_EQ(ftell(fd), ty_test_data_size);
}
// }}}1

// {{{1 track input atom unit tests
static const uint32_t in_test_data_size = 44;
// clang-format off
static const unsigned char in_test_data[in_test_data_size] = {
    in_test_data_size >> 24,  // size
    in_test_data_size >> 16,
    in_test_data_size >> 8,
    in_test_data_size,
    '\0', '\0', 'i', 'n',     // type
    0x00, 0x01, 0x02, 0x03,   // atom id
    0x00, 0x00,               // reserved
    0x00, 0x02,               // child count
    0x00, 0x00, 0x00, 0x00,   // reserved
    0x00, 0x00, 0x00, 0x0c,   // input type atom.size
    '\0', '\0', 't', 'y',     // input type atom.type
    0x00, 0x01, 0x02, 0x03,   // input type atom.input type
    0x00, 0x00, 0x00, 0x0c,   // object id atom.size
    'o', 'b', 'i', 'd',       // object id atom.type
    0x00, 0x01, 0x02, 0x03,   // object id atom.object id
};
// clang-format on
// clang-format off
static const MuTFFTrackInputAtom in_test_struct = {
    0x00010203,                // atom id
    2,                         // child count
    ty_test_struct,            // input type atom
    true,
    obid_test_struct,          // object id atom
};
// clang-format on

TEST(MuTFF, WriteTrackInputAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret = mutff_write_track_input_atom(fd, &in_test_struct);
  ASSERT_EQ(ret, in_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, in_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], in_test_data[i]);
  }
}

TEST(MuTFF, ReadTrackInputAtom) {
  MuTFFError ret;
  MuTFFTrackInputAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(in_test_data, in_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_track_input_atom(fd, &atom);
  ASSERT_EQ(ret, in_test_data_size);

  EXPECT_EQ(atom.atom_id, in_test_struct.atom_id);
  EXPECT_EQ(atom.child_count, in_test_struct.child_count);
  EXPECT_EQ(atom.input_type_atom.input_type,
            in_test_struct.input_type_atom.input_type);
  ASSERT_EQ(atom.object_id_atom_present, true);
  EXPECT_EQ(atom.object_id_atom.object_id,
            in_test_struct.object_id_atom.object_id);
  EXPECT_EQ(ftell(fd), in_test_data_size);
}
// }}}1

// {{{1 track input map atom unit tests
static const uint32_t imap_test_data_size = 96;
// clang-format off
static const unsigned char imap_test_data[imap_test_data_size] = {
    imap_test_data_size >> 24,  // size
    imap_test_data_size >> 16,
    imap_test_data_size >> 8,
    imap_test_data_size,
    'i', 'm', 'a', 'p',         // type
    0x00, 0x00, 0x00, 44,       // size
    '\0', '\0', 'i', 'n',       // type
    0x00, 0x01, 0x02, 0x03,     // atom id
    0x00, 0x00,                 // reserved
    0x00, 0x02,                 // child count
    0x00, 0x00, 0x00, 0x00,     // reserved
    0x00, 0x00, 0x00, 0x0c,     // input type atom.size
    '\0', '\0', 't', 'y',       // input type atom.type
    0x00, 0x01, 0x02, 0x03,     // input type atom.input type
    0x00, 0x00, 0x00, 0x0c,     // object id atom.size
    'o', 'b', 'i', 'd',         // object id atom.type
    0x00, 0x01, 0x02, 0x03,     // object id atom.object id
    0x00, 0x00, 0x00, 44,       // size
    '\0', '\0', 'i', 'n',       // type
    0x00, 0x01, 0x02, 0x03,     // atom id
    0x00, 0x00,                 // reserved
    0x00, 0x02,                 // child count
    0x00, 0x00, 0x00, 0x00,     // reserved
    0x00, 0x00, 0x00, 0x0c,     // input type atom.size
    '\0', '\0', 't', 'y',       // input type atom.type
    0x00, 0x01, 0x02, 0x03,     // input type atom.input type
    0x00, 0x00, 0x00, 0x0c,     // object id atom.size
    'o', 'b', 'i', 'd',         // object id atom.type
    0x00, 0x01, 0x02, 0x03,     // object id atom.object id
};
// clang-format on
// clang-format off
static const MuTFFTrackInputMapAtom imap_test_struct = {
    2,                       // track input atom count
    {                        // track input atoms
      in_test_struct,
      in_test_struct,
    }
};
// clang-format on

TEST(MuTFF, WriteTrackInputMapAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret =
      mutff_write_track_input_map_atom(fd, &imap_test_struct);
  ASSERT_EQ(ret, imap_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, imap_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], imap_test_data[i]);
  }
}

TEST(MuTFF, ReadTrackInputMapAtom) {
  MuTFFError ret;
  MuTFFTrackInputMapAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(imap_test_data, imap_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_track_input_map_atom(fd, &atom);
  ASSERT_EQ(ret, imap_test_data_size);

  EXPECT_EQ(atom.track_input_atoms[0].atom_id,
            imap_test_struct.track_input_atoms[0].atom_id);
  EXPECT_EQ(atom.track_input_atoms[0].child_count,
            imap_test_struct.track_input_atoms[0].child_count);
  EXPECT_EQ(atom.track_input_atoms[0].input_type_atom.input_type,
            imap_test_struct.track_input_atoms[0].input_type_atom.input_type);
  EXPECT_EQ(atom.track_input_atoms[0].object_id_atom.object_id,
            imap_test_struct.track_input_atoms[0].object_id_atom.object_id);
  EXPECT_EQ(atom.track_input_atoms[1].atom_id,
            imap_test_struct.track_input_atoms[1].atom_id);
  EXPECT_EQ(atom.track_input_atoms[1].child_count,
            imap_test_struct.track_input_atoms[1].child_count);
  EXPECT_EQ(atom.track_input_atoms[1].input_type_atom.input_type,
            imap_test_struct.track_input_atoms[1].input_type_atom.input_type);
  EXPECT_EQ(atom.track_input_atoms[1].object_id_atom.object_id,
            imap_test_struct.track_input_atoms[1].object_id_atom.object_id);
  EXPECT_EQ(ftell(fd), imap_test_data_size);
}
// }}}1

// {{{1 track load settings atom unit tests
static const uint32_t load_test_data_size = 24;
// clang-format off
static const unsigned char load_test_data[load_test_data_size] = {
    load_test_data_size >> 24,  // size
    load_test_data_size >> 16,
    load_test_data_size >> 8,
    load_test_data_size,
    'l', 'o', 'a', 'd',         // type
    0x00, 0x01, 0x02, 0x03,     // preload start time
    0x10, 0x11, 0x12, 0x13,     // preload duration
    0x20, 0x21, 0x22, 0x23,     // preload flags
    0x30, 0x31, 0x32, 0x33,     // default hints
};
// clang-format on
// clang-format off
static const MuTFFTrackLoadSettingsAtom load_test_struct = {
    0x00010203,            // preload start time
    0x10111213,            // preload duration
    0x20212223,            // preload flags
    0x30313233,            // default hints
};
// clang-format on

TEST(MuTFF, WriteTrackLoadSettingsAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret =
      mutff_write_track_load_settings_atom(fd, &load_test_struct);
  ASSERT_EQ(ret, load_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, load_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], load_test_data[i]);
  }
}

TEST(MuTFF, ReadTrackLoadSettingsAtom) {
  MuTFFError ret;
  MuTFFTrackLoadSettingsAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(load_test_data, load_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_track_load_settings_atom(fd, &atom);
  ASSERT_EQ(ret, load_test_data_size);

  EXPECT_EQ(atom.preload_start_time, load_test_struct.preload_start_time);
  EXPECT_EQ(atom.preload_duration, load_test_struct.preload_duration);
  EXPECT_EQ(atom.preload_flags, load_test_struct.preload_flags);
  EXPECT_EQ(atom.default_hints, load_test_struct.default_hints);
  EXPECT_EQ(ftell(fd), load_test_data_size);
}
// }}}1

// {{{1 track exclude from autoselection atom unit tests
static const uint32_t txas_test_data_size = 8;
// clang-format off
static const unsigned char txas_test_data[txas_test_data_size] = {
    txas_test_data_size >> 24,  // size
    txas_test_data_size >> 16,
    txas_test_data_size >> 8,
    txas_test_data_size,
    't', 'x', 'a', 's',         // type
};
// clang-format on
// clang-format off
static const MuTFFTrackExcludeFromAutoselectionAtom txas_test_struct = {
};
// clang-format on

TEST(MuTFF, WriteTrackExcludeFromAutoselectionAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret =
      mutff_write_track_exclude_from_autoselection_atom(fd, &txas_test_struct);
  ASSERT_EQ(ret, txas_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, txas_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], txas_test_data[i]);
  }
}

TEST(MuTFF, ReadTrackExcludeFromAutoselectionAtom) {
  MuTFFError ret;
  MuTFFTrackExcludeFromAutoselectionAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(txas_test_data, txas_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_track_exclude_from_autoselection_atom(fd, &atom);
  ASSERT_EQ(ret, txas_test_data_size);

  EXPECT_EQ(ftell(fd), txas_test_data_size);
}
// }}}1

// {{{1 track reference atom unit tests
static const uint32_t tref_test_data_size = 40;
// clang-format off
static const unsigned char tref_test_data[tref_test_data_size] = {
    tref_test_data_size >> 24,  // size
    tref_test_data_size >> 16,
    tref_test_data_size >> 8,
    tref_test_data_size,
    't', 'r', 'e', 'f',         // type
    0x00, 0x00, 0x00, 0x10,     // track reference type[0].size
    'a', 'b', 'c', 'd',         // track reference type[0].type
    0x00, 0x01, 0x02, 0x03,     // track reference type[0].track_ids[0]
    0x10, 0x11, 0x12, 0x13,     // track reference type[0].track_ids[1]
    0x00, 0x00, 0x00, 0x10,     // track reference type[1].size
    'e', 'f', 'g', 'h',         // track reference type[1].type
    0x20, 0x21, 0x22, 0x23,     // track reference type[1].track_ids[0]
    0x30, 0x31, 0x32, 0x33,     // track reference type[1].track_ids[1]
};
// clang-format on
// clang-format off
static const MuTFFTrackReferenceAtom tref_test_struct = {
    2,                         // track reference type count
    {
      {
        MuTFF_FOURCC('a', 'b', 'c', 'd'),  // track reference type[0].type
        2,
        {
          0x00010203,          // track reference type[0].track_ids[0]
          0x10111213,          // track reference type[0].track_ids[1]
        },
      },
      {
        MuTFF_FOURCC('e', 'f', 'g', 'h'),  // track reference type[1].type
        2,
        {
          0x20212223,          // track reference type[1].track_ids[0]
          0x30313233,          // track reference type[1].track_ids[1]
        },
      },
    },
};
// clang-format on

TEST(MuTFF, WriteTrackReferenceAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret =
      mutff_write_track_reference_atom(fd, &tref_test_struct);
  ASSERT_EQ(ret, tref_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, tref_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], tref_test_data[i]);
  }
}

TEST(MuTFF, ReadTrackReferenceAtom) {
  MuTFFError ret;
  MuTFFTrackReferenceAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(tref_test_data, tref_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_track_reference_atom(fd, &atom);
  ASSERT_EQ(ret, tref_test_data_size);

  EXPECT_EQ(atom.track_reference_type[0].type,
            tref_test_struct.track_reference_type[0].type);
  EXPECT_EQ(atom.track_reference_type[0].track_ids[0],
            tref_test_struct.track_reference_type[0].track_ids[0]);
  EXPECT_EQ(atom.track_reference_type[0].track_ids[1],
            tref_test_struct.track_reference_type[0].track_ids[1]);
  EXPECT_EQ(atom.track_reference_type[1].type,
            tref_test_struct.track_reference_type[1].type);
  EXPECT_EQ(atom.track_reference_type[1].track_ids[0],
            tref_test_struct.track_reference_type[1].track_ids[0]);
  EXPECT_EQ(atom.track_reference_type[1].track_ids[1],
            tref_test_struct.track_reference_type[1].track_ids[1]);
  EXPECT_EQ(ftell(fd), tref_test_data_size);
}
// }}}1

// {{{1 track reference type atom unit tests
static const uint32_t track_ref_atom_test_data_size = 16;
// clang-format off
static const unsigned char track_ref_atom_test_data[track_ref_atom_test_data_size] = {
    track_ref_atom_test_data_size >> 24,  // size
    track_ref_atom_test_data_size >> 16,
    track_ref_atom_test_data_size >> 8,
    track_ref_atom_test_data_size,
    'a', 'b', 'c', 'd',           // type
    0x00, 0x01, 0x02, 0x03,       // track_ids[0]
    0x10, 0x11, 0x12, 0x13,       // track_ids[1]
};
// clang-format on
// clang-format off
static const MuTFFTrackReferenceTypeAtom track_ref_atom_test_struct = {
    MuTFF_FOURCC('a', 'b', 'c', 'd'),            // type
    2,                               // track id count
    0x00010203,                      // track ids[0]
    0x10111213,                      // track ids[1]
};
// clang-format on

TEST(MuTFF, WriteTrackReferenceTypeAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret =
      mutff_write_track_reference_type_atom(fd, &track_ref_atom_test_struct);
  ASSERT_EQ(ret, track_ref_atom_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, track_ref_atom_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], track_ref_atom_test_data[i]);
  }
}

TEST(MuTFF, ReadTrackReferenceTypeAtom) {
  MuTFFError ret;
  MuTFFTrackReferenceTypeAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(track_ref_atom_test_data, track_ref_atom_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_track_reference_type_atom(fd, &atom);
  ASSERT_EQ(ret, track_ref_atom_test_data_size);

  EXPECT_EQ(atom.type, track_ref_atom_test_struct.type);
  EXPECT_EQ(atom.track_id_count, track_ref_atom_test_struct.track_id_count);
  for (size_t i = 0; i < track_ref_atom_test_struct.track_id_count; ++i) {
    EXPECT_EQ(atom.track_ids[i], track_ref_atom_test_struct.track_ids[i]);
  }
  EXPECT_EQ(ftell(fd), track_ref_atom_test_data_size);
}
// }}}1

// {{{1 edit list atom unit tests
static const uint32_t elst_test_data_size = 40;
// clang-format off
static const unsigned char elst_test_data[elst_test_data_size] = {
    elst_test_data_size >> 24,  // size
    elst_test_data_size >> 16,
    elst_test_data_size >> 8,
    elst_test_data_size,
    'e', 'l', 's', 't',         // type
    0x00,                       // version
    0x00, 0x01, 0x02,           // flags
    0x00, 0x00, 0x00, 0x02,     // number of entries
    0x00, 0x01, 0x02, 0x03,     // entry[0].track duration
    0x10, 0x11, 0x12, 0x13,     // entry[0].media time
    0x20, 0x21, 0x22, 0x23,     // entry[0].media rate
    0x30, 0x31, 0x32, 0x33,     // entry[1].track duration
    0x40, 0x41, 0x42, 0x43,     // entry[1].media time
    0x50, 0x51, 0x52, 0x53,     // entry[1].media rate
};
// clang-format on
// clang-format off
static const MuTFFEditListAtom elst_test_struct = {
    0x00,                  // version
    0x000102,              // flags
    0x00000002,            // number of entries
    {
      {
        0x00010203,        // entry[0].track duration
        0x10111213,        // entry[0].media time
        {0x2021, 0x2223},  // entry[0].media rate
      },
      {
        0x30313233,        // entry[1].track duration
        0x40414243,        // entry[1].media time
        {0x5051, 0x5253},  // entry[1].media rate
      }
    }
};
// clang-format on

TEST(MuTFF, WriteEditListAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret = mutff_write_edit_list_atom(fd, &elst_test_struct);
  ASSERT_EQ(ret, elst_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, elst_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], elst_test_data[i]);
  }
}

TEST(MuTFF, ReadEditListAtom) {
  MuTFFError ret;
  MuTFFEditListAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(elst_test_data, elst_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_edit_list_atom(fd, &atom);
  ASSERT_EQ(ret, elst_test_data_size);

  EXPECT_EQ(atom.version, elst_test_struct.version);
  EXPECT_EQ(atom.flags, elst_test_struct.flags);
  EXPECT_EQ(atom.number_of_entries, elst_test_struct.number_of_entries);
  for (size_t i = 0; i < elst_test_struct.number_of_entries; ++i) {
    EXPECT_EQ(atom.edit_list_table[i].track_duration,
              atom.edit_list_table[i].track_duration);
    EXPECT_EQ(atom.edit_list_table[i].media_time,
              atom.edit_list_table[i].media_time);
    EXPECT_EQ(atom.edit_list_table[i].media_rate.integral,
              atom.edit_list_table[i].media_rate.integral);
    EXPECT_EQ(atom.edit_list_table[i].media_rate.fractional,
              atom.edit_list_table[i].media_rate.fractional);
  }
  EXPECT_EQ(ftell(fd), elst_test_data_size);
}
// }}}1

// {{{1 edit atom unit tests
static const uint32_t edts_test_data_size = 48;
// clang-format off
static const unsigned char edts_test_data[edts_test_data_size] = {
    edts_test_data_size >> 24,  // size
    edts_test_data_size >> 16,
    edts_test_data_size >> 8,
    edts_test_data_size,
    'e', 'd', 't', 's',         // type
    0x00, 0x00, 0x00, 40,       // elst.size
    'e', 'l', 's', 't',         // elst.type
    0x00,                       // elst.version
    0x00, 0x01, 0x02,           // elst.flags
    0x00, 0x00, 0x00, 0x02,     // elst.number of entries
    0x00, 0x01, 0x02, 0x03,     // elst.entry[0].track duration
    0x10, 0x11, 0x12, 0x13,     // elst.entry[0].media time
    0x20, 0x21, 0x22, 0x23,     // elst.entry[0].media rate
    0x30, 0x31, 0x32, 0x33,     // elst.entry[1].track duration
    0x40, 0x41, 0x42, 0x43,     // elst.entry[1].media time
    0x50, 0x51, 0x52, 0x53,     // elst.entry[1].media rate
};
// clang-format on
// clang-format off
static const MuTFFEditAtom edts_test_struct = {
    elst_test_struct,        // edit list atom
};
// clang-format on

TEST(MuTFF, WriteEditAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret = mutff_write_edit_atom(fd, &edts_test_struct);
  ASSERT_EQ(ret, edts_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, edts_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], edts_test_data[i]);
  }
}

TEST(MuTFF, ReadEditAtom) {
  MuTFFError ret;
  MuTFFEditAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(edts_test_data, edts_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_edit_atom(fd, &atom);
  ASSERT_EQ(ret, edts_test_data_size);

  EXPECT_EQ(atom.edit_list_atom.version,
            edts_test_struct.edit_list_atom.version);
  EXPECT_EQ(atom.edit_list_atom.flags, edts_test_struct.edit_list_atom.flags);
  EXPECT_EQ(atom.edit_list_atom.number_of_entries,
            edts_test_struct.edit_list_atom.number_of_entries);
  EXPECT_EQ(atom.edit_list_atom.edit_list_table[0].track_duration,
            edts_test_struct.edit_list_atom.edit_list_table[0].track_duration);
  EXPECT_EQ(atom.edit_list_atom.edit_list_table[0].media_time,
            edts_test_struct.edit_list_atom.edit_list_table[0].media_time);
  EXPECT_EQ(
      atom.edit_list_atom.edit_list_table[0].media_rate.integral,
      edts_test_struct.edit_list_atom.edit_list_table[0].media_rate.integral);
  EXPECT_EQ(
      atom.edit_list_atom.edit_list_table[0].media_rate.fractional,
      edts_test_struct.edit_list_atom.edit_list_table[0].media_rate.fractional);
  EXPECT_EQ(atom.edit_list_atom.edit_list_table[1].track_duration,
            edts_test_struct.edit_list_atom.edit_list_table[1].track_duration);
  EXPECT_EQ(atom.edit_list_atom.edit_list_table[1].media_time,
            edts_test_struct.edit_list_atom.edit_list_table[1].media_time);
  EXPECT_EQ(
      atom.edit_list_atom.edit_list_table[1].media_rate.integral,
      edts_test_struct.edit_list_atom.edit_list_table[1].media_rate.integral);
  EXPECT_EQ(
      atom.edit_list_atom.edit_list_table[1].media_rate.fractional,
      edts_test_struct.edit_list_atom.edit_list_table[1].media_rate.fractional);
  EXPECT_EQ(ftell(fd), edts_test_data_size);
}
// }}}1

// {{{1 edit list entry unit tests
static const uint32_t edit_list_entry_test_data_size = 12;
// clang-format off
static const unsigned char edit_list_entry_test_data[edit_list_entry_test_data_size] = {
    0x00, 0x01, 0x02, 0x03,  // track duration
    0x10, 0x11, 0x12, 0x13,  // media time
    0x20, 0x21, 0x22, 0x23,  // media rate
};
// clang-format on
// clang-format off
static const MuTFFEditListEntry edit_list_entry_test_struct = {
    0x00010203,        // track duration
    0x10111213,        // media time
    {0x2021, 0x2223},  // media rate
};
// clang-format on

TEST(MuTFF, WriteEditListEntry) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret =
      mutff_write_edit_list_entry(fd, &edit_list_entry_test_struct);
  ASSERT_EQ(ret, edit_list_entry_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, edit_list_entry_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], edit_list_entry_test_data[i]);
  }
}

TEST(MuTFF, ReadEditListEntry) {
  MuTFFError ret;
  MuTFFEditListEntry atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(edit_list_entry_test_data, edit_list_entry_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_edit_list_entry(fd, &atom);
  ASSERT_EQ(ret, edit_list_entry_test_data_size);

  EXPECT_EQ(atom.track_duration, edit_list_entry_test_struct.track_duration);
  EXPECT_EQ(atom.media_time, edit_list_entry_test_struct.media_time);
  EXPECT_EQ(atom.media_rate.integral,
            edit_list_entry_test_struct.media_rate.integral);
  EXPECT_EQ(atom.media_rate.fractional,
            edit_list_entry_test_struct.media_rate.fractional);
  EXPECT_EQ(ftell(fd), edit_list_entry_test_data_size);
}
// }}}1

// {{{1 sample description unit tests
static const uint32_t sample_desc_test_data_size = 20;
// clang-format off
static const unsigned char sample_desc_test_data[sample_desc_test_data_size] = {
    sample_desc_test_data_size >> 24,    // size
    sample_desc_test_data_size >> 16,
    sample_desc_test_data_size >> 8,
    sample_desc_test_data_size,
    'a', 'b', 'c', 'd',                  // data format
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // reserved
    0x00, 0x01,                          // data reference index
    0x00, 0x01, 0x02, 0x03,              // media-specific data
};
// clang-format on
// clang-format off
static const MuTFFSampleDescription sample_desc_test_struct = {
    sample_desc_test_data_size,     // size
    MuTFF_FOURCC('a', 'b', 'c', 'd'),           // data format
    0x0001,
    {
      0x00, 0x01, 0x02, 0x03,
    },
};
// clang-format on

TEST(MuTFF, WriteSampleDescription) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret =
      mutff_write_sample_description(fd, &sample_desc_test_struct);
  ASSERT_EQ(ret, sample_desc_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, sample_desc_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], sample_desc_test_data[i]);
  }
}

TEST(MuTFF, ReadSampleDescription) {
  MuTFFError ret;
  MuTFFSampleDescription atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(sample_desc_test_data, sample_desc_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_sample_description(fd, &atom);
  ASSERT_EQ(ret, sample_desc_test_data_size);

  EXPECT_EQ(atom.size, sample_desc_test_struct.size);
  EXPECT_EQ(atom.data_format, sample_desc_test_struct.data_format);
  EXPECT_EQ(atom.data_reference_index,
            sample_desc_test_struct.data_reference_index);
  for (size_t i = 0; i < atom.size - 16; ++i) {
    EXPECT_EQ(atom.additional_data[i],
              sample_desc_test_struct.additional_data[i]);
  }
  EXPECT_EQ(ftell(fd), sample_desc_test_data_size);
}
// }}}1

// {{{1 compressed matte atom unit tests
static const uint32_t kmat_test_data_size = 36;
// clang-format off
static const unsigned char kmat_test_data[kmat_test_data_size] = {
    kmat_test_data_size >> 24,           // size
    kmat_test_data_size >> 16,
    kmat_test_data_size >> 8,
    kmat_test_data_size,
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
// clang-format off
static const MuTFFCompressedMatteAtom kmat_test_struct = {
    0x00,                  // version
    0x000102,              // flags
    sample_desc_test_struct,
    4,
    {
      0x00, 0x01, 0x02, 0x03,
    }
};
// clang-format on

TEST(MuTFF, WriteCompressedMatteAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret =
      mutff_write_compressed_matte_atom(fd, &kmat_test_struct);
  ASSERT_EQ(ret, kmat_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, kmat_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], kmat_test_data[i]);
  }
}

TEST(MuTFF, ReadCompressedMatteAtom) {
  MuTFFError ret;
  MuTFFCompressedMatteAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(kmat_test_data, kmat_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_compressed_matte_atom(fd, &atom);
  ASSERT_EQ(ret, kmat_test_data_size);

  EXPECT_EQ(atom.version, kmat_test_struct.version);
  EXPECT_EQ(atom.flags, kmat_test_struct.flags);
  EXPECT_EQ(atom.matte_data_len, kmat_test_struct.matte_data_len);
  for (size_t i = 0; i < kmat_test_struct.matte_data_len; ++i) {
    EXPECT_EQ(atom.matte_data[i], kmat_test_struct.matte_data[i]);
  }
  EXPECT_EQ(ftell(fd), kmat_test_data_size);
}
// }}}1

// {{{1 track matte atom unit tests
static const uint32_t matt_test_data_size = 44;
// clang-format off
static const unsigned char matt_test_data[matt_test_data_size] = {
    matt_test_data_size >> 24,           // size
    matt_test_data_size >> 16,
    matt_test_data_size >> 8,
    matt_test_data_size,
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
// clang-format off
static const MuTFFTrackMatteAtom matt_test_struct = {
    kmat_test_struct,      // compressed matte
};
// clang-format on

TEST(MuTFF, WriteTrackMatteAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret = mutff_write_track_matte_atom(fd, &matt_test_struct);
  ASSERT_EQ(ret, matt_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, matt_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], matt_test_data[i]);
  }
}

TEST(MuTFF, ReadTrackMatteAtom) {
  MuTFFError ret;
  MuTFFTrackMatteAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(matt_test_data, matt_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_track_matte_atom(fd, &atom);
  ASSERT_EQ(ret, matt_test_data_size);

  EXPECT_EQ(ftell(fd), matt_test_data_size);
}
// }}}1

// {{{1 track encoded pixels dimensions atom unit tests
static const uint32_t enof_test_data_size = 20;
// clang-format off
static const unsigned char enof_test_data[enof_test_data_size] = {
    enof_test_data_size >> 24,  // size
    enof_test_data_size >> 16,
    enof_test_data_size >> 8,
    enof_test_data_size,
    'e', 'n', 'o', 'f',         // type
    0x00,                       // version
    0x00, 0x01, 0x02,           // flags
    0x00, 0x01, 0x02, 0x03,     // width
    0x10, 0x11, 0x12, 0x13,     // height
};
// clang-format on
// clang-format off
static const MuTFFTrackEncodedPixelsDimensionsAtom enof_test_struct = {
    0x00,                  // version
    0x000102,              // flags
    {0x0001, 0x0203},      // width
    {0x1011, 0x1213},      // height
};
// clang-format on

TEST(MuTFF, WriteTrackEncodedPixelsDimensionsAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret =
      mutff_write_track_encoded_pixels_dimensions_atom(fd, &enof_test_struct);
  ASSERT_EQ(ret, enof_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, enof_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], enof_test_data[i]);
  }
}

TEST(MuTFF, ReadTrackEncodedPixelsDimensionsAtom) {
  MuTFFError ret;
  MuTFFTrackEncodedPixelsDimensionsAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(enof_test_data, enof_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_track_encoded_pixels_dimensions_atom(fd, &atom);
  ASSERT_EQ(ret, enof_test_data_size);

  EXPECT_EQ(atom.version, enof_test_struct.version);
  EXPECT_EQ(atom.flags, enof_test_struct.flags);
  EXPECT_EQ(atom.width.integral, enof_test_struct.width.integral);
  EXPECT_EQ(atom.width.fractional, enof_test_struct.width.fractional);
  EXPECT_EQ(atom.height.integral, enof_test_struct.height.integral);
  EXPECT_EQ(atom.height.fractional, enof_test_struct.height.fractional);
  EXPECT_EQ(ftell(fd), enof_test_data_size);
}
// }}}1

// {{{1 track production aperture dimensions atom unit tests
static const uint32_t prof_test_data_size = 20;
// clang-format off
static const unsigned char prof_test_data[prof_test_data_size] = {
    prof_test_data_size >> 24,  // size
    prof_test_data_size >> 16,
    prof_test_data_size >> 8,
    prof_test_data_size,
    'p', 'r', 'o', 'f',         // type
    0x00,                       // version
    0x00, 0x01, 0x02,           // flags
    0x00, 0x01, 0x02, 0x03,     // width
    0x10, 0x11, 0x12, 0x13,     // height
};
// clang-format on
// clang-format off
static const MuTFFTrackProductionApertureDimensionsAtom prof_test_struct = {
    0x00,                  // version
    0x000102,              // flags
    {0x0001, 0x0203},      // width
    {0x1011, 0x1213},      // height
};
// clang-format on

TEST(MuTFF, WriteTrackProductionApertureDimensionsAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret = mutff_write_track_production_aperture_dimensions_atom(
      fd, &prof_test_struct);
  ASSERT_EQ(ret, prof_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, prof_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], prof_test_data[i]);
  }
}

TEST(MuTFF, ReadTrackProductionApertureDimensionsAtom) {
  MuTFFError ret;
  MuTFFTrackProductionApertureDimensionsAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(prof_test_data, prof_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_track_production_aperture_dimensions_atom(fd, &atom);
  ASSERT_EQ(ret, prof_test_data_size);

  EXPECT_EQ(atom.version, prof_test_struct.version);
  EXPECT_EQ(atom.flags, prof_test_struct.flags);
  EXPECT_EQ(atom.width.integral, prof_test_struct.width.integral);
  EXPECT_EQ(atom.width.fractional, prof_test_struct.width.fractional);
  EXPECT_EQ(atom.height.integral, prof_test_struct.height.integral);
  EXPECT_EQ(atom.height.fractional, prof_test_struct.height.fractional);
  EXPECT_EQ(ftell(fd), prof_test_data_size);
}
// }}}1

// {{{1 track clean aperture dimensions atom unit tests
static const uint32_t clef_test_data_size = 20;
// clang-format off
static const unsigned char clef_test_data[clef_test_data_size] = {
    clef_test_data_size >> 24,  // size
    clef_test_data_size >> 16,
    clef_test_data_size >> 8,
    clef_test_data_size,
    'c', 'l', 'e', 'f',         // type
    0x00,                       // version
    0x00, 0x01, 0x02,           // flags
    0x00, 0x01, 0x02, 0x03,     // width
    0x10, 0x11, 0x12, 0x13,     // height
};
// clang-format on
// clang-format off
static const MuTFFTrackCleanApertureDimensionsAtom clef_test_struct = {
    0x00,                  // version
    0x000102,              // flags
    {0x0001, 0x0203},      // width
    {0x1011, 0x1213},      // height
};
// clang-format on

TEST(MuTFF, WriteTrackCleanApertureDimensionsAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret =
      mutff_write_track_clean_aperture_dimensions_atom(fd, &clef_test_struct);
  ASSERT_EQ(ret, clef_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, clef_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], clef_test_data[i]);
  }
}

TEST(MuTFF, ReadTrackCleanApertureDimensionsAtom) {
  MuTFFError ret;
  MuTFFTrackCleanApertureDimensionsAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(clef_test_data, clef_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_track_clean_aperture_dimensions_atom(fd, &atom);
  ASSERT_EQ(ret, clef_test_data_size);

  EXPECT_EQ(atom.version, clef_test_struct.version);
  EXPECT_EQ(atom.flags, clef_test_struct.flags);
  EXPECT_EQ(atom.width.integral, clef_test_struct.width.integral);
  EXPECT_EQ(atom.width.fractional, clef_test_struct.width.fractional);
  EXPECT_EQ(atom.height.integral, clef_test_struct.height.integral);
  EXPECT_EQ(atom.height.fractional, clef_test_struct.height.fractional);
  EXPECT_EQ(ftell(fd), clef_test_data_size);
}
// }}}1

// {{{1 track aperture mode dimensions atom unit tests
static const uint32_t tapt_test_data_size = 68;
// clang-format off
static const unsigned char tapt_test_data[tapt_test_data_size] = {
    tapt_test_data_size >> 24,  // size
    tapt_test_data_size >> 16,
    tapt_test_data_size >> 8,
    tapt_test_data_size,
    't', 'a', 'p', 't',         // type
    0x00, 0x00, 0x00, 0x14,     // clef.size
    'c', 'l', 'e', 'f',         // clef.type
    0x00,                       // clef.version
    0x00, 0x01, 0x02,           // clef.flags
    0x00, 0x01, 0x02, 0x03,     // clef.width
    0x10, 0x11, 0x12, 0x13,     // clef.height
    0x00, 0x00, 0x00, 0x14,     // prof.size
    'p', 'r', 'o', 'f',         // prof.type
    0x00,                       // prof.version
    0x00, 0x01, 0x02,           // prof.flags
    0x00, 0x01, 0x02, 0x03,     // prof.width
    0x10, 0x11, 0x12, 0x13,     // prof.height
    0x00, 0x00, 0x00, 0x14,     // enof.size
    'e', 'n', 'o', 'f',         // enof.type
    0x00,                       // enof.version
    0x00, 0x01, 0x02,           // enof.flags
    0x00, 0x01, 0x02, 0x03,     // enof.width
    0x10, 0x11, 0x12, 0x13,     // enof.height
};
// clang-format on
// clang-format off
static const MuTFFTrackApertureModeDimensionsAtom tapt_test_struct = {
    clef_test_struct,
    prof_test_struct,
    enof_test_struct,
};
// clang-format on

TEST(MuTFF, WriteTrackApertureModeDimensionsAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret =
      mutff_write_track_aperture_mode_dimensions_atom(fd, &tapt_test_struct);
  ASSERT_EQ(ret, tapt_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, tapt_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], tapt_test_data[i]);
  }
}

TEST(MuTFF, ReadTrackApertureModeDimensionsAtom) {
  MuTFFError ret;
  MuTFFTrackApertureModeDimensionsAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(tapt_test_data, tapt_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_track_aperture_mode_dimensions_atom(fd, &atom);
  ASSERT_EQ(ret, tapt_test_data_size);

  EXPECT_EQ(ftell(fd), tapt_test_data_size);
}
// }}}1

// {{{1 track header atom unit tests
static const uint32_t tkhd_test_data_size = 92;
// clang-format off
static const unsigned char tkhd_test_data[tkhd_test_data_size] = {
    tkhd_test_data_size >> 24,  // size
    tkhd_test_data_size >> 16,
    tkhd_test_data_size >> 8,
    tkhd_test_data_size,
    't', 'k', 'h', 'd',         // type
    0x00,                       // version
    0x00, 0x01, 0x02,           // flags
    0x00, 0x01, 0x02, 0x03,     // creation time
    0x00, 0x01, 0x02, 0x03,     // modification time
    0x00, 0x01, 0x02, 0x03,     // track ID
    0x00, 0x00, 0x00, 0x00,     // reserved
    0x00, 0x01, 0x02, 0x03,     // duration
    0x00, 0x00, 0x00, 0x00,     // reserved
    0x00, 0x00, 0x00, 0x00,     // reserved
    0x00, 0x01,                 // layer
    0x00, 0x01,                 // alternate group
    0x00, 0x01,                 // volume
    0x00, 0x00,                 // reserved
    0x01, 0x02, 0x03, 0x04,     // matrix[0][0]
    0x05, 0x06, 0x07, 0x08,     // matrix[0][1]
    0x09, 0x0a, 0x0b, 0x0c,     // matrix[0][2]
    0x0d, 0x0e, 0x0f, 0x10,     // matrix[1][0]
    0x11, 0x12, 0x13, 0x14,     // matrix[1][1]
    0x15, 0x16, 0x17, 0x18,     // matrix[1][2]
    0x19, 0x1a, 0x1b, 0x1c,     // matrix[2][0]
    0x1d, 0x1e, 0x1f, 0x20,     // matrix[2][1]
    0x21, 0x22, 0x23, 0x24,     // matrix[2][2]
    0x00, 0x01, 0x02, 0x03,     // track width
    0x00, 0x01, 0x02, 0x03,     // track height
};
// clang-format on
// clang-format off
static const MuTFFTrackHeaderAtom tkhd_test_struct = {
    0x00,                    // version
    0x000102,                // flags
    0x00010203,              // creation time
    0x00010203,              // modification time
    0x00010203,              // track id
    0x00010203,              // duration
    0x0001,                  // layer
    0x0001,                  // alternate group
    {0x00, 0x01},            // volume
    {
      {
        0x01020304,          // matrix[0][0]
        0x05060708,          // matrix[0][1]
        0x090a0b0c,          // matrix[0][2]
      },
      {
        0x0d0e0f10,          // matrix[1][0]
        0x11121314,          // matrix[1][1]
        0x15161718,          // matrix[1][2]
      },
      {
        0x191a1b1c,          // matrix[2][0]
        0x1d1e1f20,          // matrix[2][1]
        0x21222324,          // matrix[2][2]
      }
    },
    {0x0001, 0x0203},        // track width
    {0x0001, 0x0203},        // track height
};
// clang-format on

TEST(MuTFF, WriteTrackHeaderAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret = mutff_write_track_header_atom(fd, &tkhd_test_struct);
  ASSERT_EQ(ret, tkhd_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, tkhd_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], tkhd_test_data[i]);
  }
}

TEST(MuTFF, ReadTrackHeaderAtom) {
  MuTFFError ret;
  MuTFFTrackHeaderAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(tkhd_test_data, tkhd_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_track_header_atom(fd, &atom);
  ASSERT_EQ(ret, tkhd_test_data_size);

  EXPECT_EQ(atom.version, tkhd_test_struct.version);
  EXPECT_EQ(atom.creation_time, tkhd_test_struct.creation_time);
  EXPECT_EQ(atom.modification_time, tkhd_test_struct.modification_time);
  EXPECT_EQ(atom.track_id, tkhd_test_struct.track_id);
  EXPECT_EQ(atom.duration, tkhd_test_struct.duration);
  EXPECT_EQ(atom.layer, tkhd_test_struct.layer);
  EXPECT_EQ(atom.alternate_group, tkhd_test_struct.alternate_group);
  EXPECT_EQ(atom.volume.integral, tkhd_test_struct.volume.integral);
  EXPECT_EQ(atom.volume.fractional, tkhd_test_struct.volume.fractional);
  for (size_t j = 0; j < 3; ++j) {
    for (size_t i = 0; i < 3; ++i) {
      EXPECT_EQ(atom.matrix_structure[j][i],
                tkhd_test_struct.matrix_structure[j][i]);
    }
  }
  EXPECT_EQ(atom.track_width.integral, tkhd_test_struct.track_width.integral);
  EXPECT_EQ(atom.track_width.fractional,
            tkhd_test_struct.track_width.fractional);
  EXPECT_EQ(atom.track_height.integral, tkhd_test_struct.track_height.integral);
  EXPECT_EQ(atom.track_height.fractional,
            tkhd_test_struct.track_height.fractional);
  EXPECT_EQ(ftell(fd), tkhd_test_data_size);
}
// }}}1

// {{{1 user data atom unit tests
static const uint32_t udta_test_data_size = 28;
// clang-format off
static const unsigned char udta_test_data[udta_test_data_size] = {
    udta_test_data_size >> 24,  // size
    udta_test_data_size >> 16,
    udta_test_data_size >> 8,
    udta_test_data_size,
    'u',  'd',  't',  'a',      // type
    0x00, 0x00, 0x00, 0x0c,     // user_data_list[0].size
    'a',  'b',  'c',  'd',      // user_data_list[0].type
    'e',  'f',  'g',  'h',      // user_data_list[0].data
    0x00, 0x00, 0x00, 0x08,     // user_data_list[1].size
    'i',  'j',  'k',  'l',      // user_data_list[1].type
};
// clang-format on
// clang-format off
static const MuTFFUserDataAtom udta_test_struct = {
    2,
    {
      {
          MuTFF_FOURCC('a', 'b', 'c', 'd'),
          4,
          {
            'e', 'f', 'g', 'h',
          }
      },
      {
          MuTFF_FOURCC('i', 'j', 'k', 'l'),
          0,
      },
    }
};
// clang-format on

TEST(MuTFF, WriteUserDataAtom) {
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret = mutff_write_user_data_atom(fd, &udta_test_struct);
  ASSERT_EQ(ret, udta_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, udta_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], udta_test_data[i]);
  }
}

TEST(MuTFF, ReadUserDataAtom) {
  MuTFFError ret;
  MuTFFUserDataAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(udta_test_data, udta_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_user_data_atom(fd, &atom);
  ASSERT_EQ(ret, udta_test_data_size);

  EXPECT_EQ(ftell(fd), udta_test_data_size);
}
// }}}1

// {{{1 user data list entry unit tests
static const uint32_t udta_entry_test_data_size = 16;
// clang-format off
static const unsigned char udta_entry_test_data[udta_entry_test_data_size] = {
    udta_entry_test_data_size >> 24,  // size
    udta_entry_test_data_size >> 16,
    udta_entry_test_data_size >> 8,
    udta_entry_test_data_size,
    'a', 'b', 'c', 'd',               // type
    'e', 'f', 'g', 'h',               // data
    0, 1, 2, 3,
};
// clang-format on
// clang-format off
static const MuTFFUserDataListEntry udta_entry_test_struct = {
    MuTFF_FOURCC('a', 'b', 'c', 'd'),       // type
    8,                          // data size
    {                           // data
      'e', 'f', 'g', 'h',
      0, 1, 2, 3,
    }
};
// clang-format on

TEST(MuTFF, WriteUserDataListEntry) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret =
      mutff_write_user_data_list_entry(fd, &udta_entry_test_struct);
  ASSERT_EQ(ret, udta_entry_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, udta_entry_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], udta_entry_test_data[i]);
  }
}

TEST(MuTFF, ReadUserDataListEntry) {
  MuTFFError ret;
  MuTFFUserDataListEntry atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(udta_entry_test_data, udta_entry_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_user_data_list_entry(fd, &atom);
  ASSERT_EQ(ret, udta_entry_test_data_size);

  EXPECT_EQ(atom.type, udta_entry_test_struct.type);
  for (size_t i = 0; i < atom.data_size; ++i) {
    EXPECT_EQ(atom.data[i], udta_entry_test_struct.data[i]);
  }
  EXPECT_EQ(ftell(fd), udta_entry_test_data_size);
}
// }}}1

// {{{1 color table atom unit tests
static const uint32_t ctab_test_data_size = 32;
// clang-format off
static const unsigned char ctab_test_data[ctab_test_data_size] = {
    ctab_test_data_size >> 24,                       // size
    ctab_test_data_size >> 16,
    ctab_test_data_size >> 8,
    ctab_test_data_size,
    'c',  't',  'a',  'b',                           // type
    0x00, 0x01, 0x02, 0x03,                          // seed
    0x00, 0x01,                                      // flags
    0x00, 0x01,                                      // color table size
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,  // color table[0]
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,  // color table[1]
};
// clang-format on
// clang-format off
static const MuTFFColorTableAtom ctab_test_struct = {
    0x00010203,            // color table seed
    0x0001,                // color table flags
    0x0001,                // color table size
    0x0001,                // color table[0][0]
    0x0203,                // color table[0][1]
    0x0405,                // color table[0][2]
    0x0607,                // color table[0][3]
    0x1011,                // color table[1][0]
    0x1213,                // color table[1][1]
    0x1415,                // color table[1][2]
    0x1617,                // color table[1][3]
};
// clang-format on

TEST(MuTFF, WriteColorTableAtom) {
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret = mutff_write_color_table_atom(fd, &ctab_test_struct);
  ASSERT_EQ(ret, ctab_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, ctab_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], ctab_test_data[i]);
  }
}

TEST(MuTFF, ReadColorTableAtom) {
  MuTFFError ret;
  MuTFFColorTableAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(ctab_test_data, ctab_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_color_table_atom(fd, &atom);
  ASSERT_EQ(ret, ctab_test_data_size);

  EXPECT_EQ(atom.color_table_seed, ctab_test_struct.color_table_seed);
  EXPECT_EQ(atom.color_table_flags, ctab_test_struct.color_table_flags);
  EXPECT_EQ(atom.color_table_size, ctab_test_struct.color_table_size);
  for (size_t i = 0; i < atom.color_table_size; ++i) {
    EXPECT_EQ(atom.color_array[i][0], ctab_test_struct.color_array[i][0]);
    EXPECT_EQ(atom.color_array[i][1], ctab_test_struct.color_array[i][1]);
    EXPECT_EQ(atom.color_array[i][2], ctab_test_struct.color_array[i][2]);
    EXPECT_EQ(atom.color_array[i][3], ctab_test_struct.color_array[i][3]);
  }
  EXPECT_EQ(ftell(fd), ctab_test_data_size);
}
// }}}1

// {{{1 cliping atom unit tests
static const uint32_t clip_test_data_size = 26;
// clang-format off
static const unsigned char clip_test_data[clip_test_data_size] = {
    clip_test_data_size >> 24,                       // size
    clip_test_data_size >> 16,
    clip_test_data_size >> 8,
    clip_test_data_size,
    'c', 'l', 'i', 'p',                              // type
    (clip_test_data_size - 8) >> 24,                 // size
    (clip_test_data_size - 8) >> 16,
    (clip_test_data_size - 8) >> 8,
    (clip_test_data_size - 8),
    'c', 'r', 'g', 'n',                              // region type
    0x00, 0x0a,                                      // region region size
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,  // region boundary box
};
// clang-format on
// clang-format off
static const MuTFFClippingAtom clip_test_struct = {
    {
        0x000a,
        0x0001,                   // region.top
        0x0203,                   // region.left
        0x0405,                   // region.bottom
        0x0607,                   // region.right
    }
};
// clang-format on

TEST(MuTFF, WriteClippingAtom) {
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret = mutff_write_clipping_atom(fd, &clip_test_struct);
  ASSERT_EQ(ret, clip_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, clip_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], clip_test_data[i]);
  }
}

TEST(MuTFF, ReadClippingAtom) {
  MuTFFError ret;
  MuTFFClippingAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(clip_test_data, clip_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_clipping_atom(fd, &atom);
  ASSERT_EQ(ret, clip_test_data_size);

  EXPECT_EQ(ftell(fd), clip_test_data_size);
}
// }}}1

// {{{1 clipping region atom unit tests
static const uint32_t crgn_test_data_size = 18;
// clang-format off
static const unsigned char crgn_test_data[crgn_test_data_size] = {
    crgn_test_data_size >> 24,                       // size
    crgn_test_data_size >> 16,
    crgn_test_data_size >> 8,
    crgn_test_data_size,
    'c', 'r', 'g', 'n',                              // type
    0x00, 0x0a,                                      // region size
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,  // region boundary box
};
// clang-format on
// clang-format off
static const MuTFFClippingRegionAtom crgn_test_struct = {
    0x000a,                // region.size
    0x0001,                // region.top
    0x0203,                // region.left
    0x0405,                // region.bottom
    0x0607,                // region.right
};
// clang-format on

TEST(MuTFF, WriteClippingRegionAtom) {
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret =
      mutff_write_clipping_region_atom(fd, &crgn_test_struct);
  ASSERT_EQ(ret, crgn_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, crgn_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], crgn_test_data[i]);
  }
}

TEST(MuTFF, ReadClippingRegionAtom) {
  MuTFFError ret;
  MuTFFClippingRegionAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(crgn_test_data, crgn_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_clipping_region_atom(fd, &atom);
  ASSERT_EQ(ret, crgn_test_data_size);

  EXPECT_EQ(atom.region.size, crgn_test_struct.region.size);
  EXPECT_EQ(atom.region.rect.top, crgn_test_struct.region.rect.top);
  EXPECT_EQ(atom.region.rect.left, crgn_test_struct.region.rect.left);
  EXPECT_EQ(atom.region.rect.bottom, crgn_test_struct.region.rect.bottom);
  EXPECT_EQ(atom.region.rect.right, crgn_test_struct.region.rect.right);
  for (size_t i = 0; i < atom.region.size - 10; ++i) {
    EXPECT_EQ(atom.region.data[i], crgn_test_struct.region.data[i]);
  }
  EXPECT_EQ(ftell(fd), crgn_test_data_size);
}
// }}}1

// {{{1 movie header atom unit tests
static const uint32_t mvhd_test_data_size = 108;
// clang-format off
static const unsigned char mvhd_test_data[mvhd_test_data_size] = {
    mvhd_test_data_size >> 24,     // size
    mvhd_test_data_size >> 16,
    mvhd_test_data_size >> 8,
    mvhd_test_data_size,
    'm', 'v', 'h', 'd',            // type
    0x01,                          // version
    0x01, 0x02, 0x03,              // flags
    0x01, 0x02, 0x03, 0x04,        // creation_time
    0x01, 0x02, 0x03, 0x04,        // modification_time
    0x01, 0x02, 0x03, 0x04,        // time_scale
    0x01, 0x02, 0x03, 0x04,        // duration
    0x01, 0x02, 0x03, 0x04,        // preferred_rate
    0x01, 0x02,                    // preferred_volume
    0x00, 0x00, 0x00, 0x00, 0x00,  
    0x00, 0x00, 0x00, 0x00, 0x00,  // reserved
    0x01, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08,
    0x09, 0x0a, 0x0b, 0x0c,
    0x0d, 0x0e, 0x0f, 0x10,
    0x11, 0x12, 0x13, 0x14,
    0x15, 0x16, 0x17, 0x18,
    0x19, 0x1a, 0x1b, 0x1c,
    0x1d, 0x1e, 0x1f, 0x20,
    0x21, 0x22, 0x23, 0x24,        // matrix_strucuture
    0x01, 0x02, 0x03, 0x04,        // preview_time
    0x01, 0x02, 0x03, 0x04,        // preview_duration
    0x01, 0x02, 0x03, 0x04,        // poster_time
    0x01, 0x02, 0x03, 0x04,        // selection_time
    0x01, 0x02, 0x03, 0x04,        // selection_duration
    0x01, 0x02, 0x03, 0x04,        // current_time
    0x01, 0x02, 0x03, 0x04,        // next_track_id
};
// clang-format on
// clang-format off
static const MuTFFMovieHeaderAtom mvhd_test_struct = {
    0x01,                          // version
    0x010203,                      // flags
    0x01020304,                    // creation time
    0x01020304,                    // modification time
    0x01020304,                    // time scale
    0x01020304,                    // duration
    {0x0102, 0x0304},              // preferred rate
    {0x01, 0x02},                  // preferred volume
    0x01020304,                    // matrix structure
    0x05060708,                    //
    0x090a0b0c,                    //
    0x0d0e0f10,                    //
    0x11121314,                    //
    0x15161718,                    //
    0x191a1b1c,                    //
    0x1d1e1f20,                    //
    0x21222324,                    //
    0x01020304,                    // preview time
    0x01020304,                    // preview duration
    0x01020304,                    // poster time
    0x01020304,                    // selection time
    0x01020304,                    // selection duration
    0x01020304,                    // current time
    0x01020304,                    // next track id
};
// clang-format on

TEST(MuTFF, WriteMovieHeaderAtom) {
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret = mutff_write_movie_header_atom(fd, &mvhd_test_struct);
  ASSERT_EQ(ret, mvhd_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, mvhd_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], mvhd_test_data[i]);
  }
}

TEST(MuTFF, ReadMovieHeaderAtom) {
  MuTFFError ret;
  MuTFFMovieHeaderAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(mvhd_test_data, mvhd_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_movie_header_atom(fd, &atom);
  ASSERT_EQ(ret, mvhd_test_data_size);

  EXPECT_EQ(atom.version, mvhd_test_struct.version);
  EXPECT_EQ(atom.flags, mvhd_test_struct.flags);
  EXPECT_EQ(atom.creation_time, mvhd_test_struct.creation_time);
  EXPECT_EQ(atom.modification_time, mvhd_test_struct.modification_time);
  EXPECT_EQ(atom.time_scale, mvhd_test_struct.time_scale);
  EXPECT_EQ(atom.duration, mvhd_test_struct.duration);
  EXPECT_EQ(atom.preferred_rate.integral,
            mvhd_test_struct.preferred_rate.integral);
  EXPECT_EQ(atom.preferred_rate.fractional,
            mvhd_test_struct.preferred_rate.fractional);
  EXPECT_EQ(atom.preferred_volume.integral,
            mvhd_test_struct.preferred_volume.integral);
  EXPECT_EQ(atom.preferred_volume.fractional,
            mvhd_test_struct.preferred_volume.fractional);
  for (size_t j = 0; j < 3; ++j) {
    for (size_t i = 0; i < 3; ++i) {
      EXPECT_EQ(atom.matrix_structure[j][i],
                mvhd_test_struct.matrix_structure[j][i]);
    }
  }
  EXPECT_EQ(atom.preview_time, mvhd_test_struct.preview_time);
  EXPECT_EQ(atom.preview_duration, mvhd_test_struct.preview_duration);
  EXPECT_EQ(atom.poster_time, mvhd_test_struct.poster_time);
  EXPECT_EQ(atom.selection_time, mvhd_test_struct.selection_time);
  EXPECT_EQ(atom.selection_duration, mvhd_test_struct.selection_duration);
  EXPECT_EQ(atom.current_time, mvhd_test_struct.current_time);
  EXPECT_EQ(atom.next_track_id, mvhd_test_struct.next_track_id);
  EXPECT_EQ(ftell(fd), mvhd_test_data_size);
}
// }}}1

// {{{1 preview atom unit tests
static const uint32_t pnot_test_data_size = 20;
// clang-format off
static const unsigned char pnot_test_data[pnot_test_data_size] = {
    pnot_test_data_size >> 24,  // size
    pnot_test_data_size >> 16,
    pnot_test_data_size >> 8,
    pnot_test_data_size,
    'p',  'n',  'o',  't',      // type
    0x01, 0x02, 0x03, 0x04,     // modification_time
    0x01, 0x02,                 // version
    'a',  'b',  'c',  'd',      // atom_type
    0x01, 0x02,                 // atom_index
};
// clang-format on
// clang-format off
static const MuTFFPreviewAtom pnot_test_struct = {
    0x01020304,            // modification time
    0x0102,                // version
    MuTFF_FOURCC('a', 'b', 'c', 'd'),  // atom type
    0x0102,                // atom indezcx
};
// clang-format on

TEST(MuTFF, WritePreviewAtom) {
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret = mutff_write_preview_atom(fd, &pnot_test_struct);
  ASSERT_EQ(ret, pnot_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, pnot_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], pnot_test_data[i]);
  }
}

TEST(MuTFF, ReadPreviewAtom) {
  MuTFFError ret;
  MuTFFPreviewAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(pnot_test_data, pnot_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_preview_atom(fd, &atom);
  ASSERT_EQ(ret, pnot_test_data_size);

  EXPECT_EQ(atom.modification_time, pnot_test_struct.modification_time);
  EXPECT_EQ(atom.version, pnot_test_struct.version);
  EXPECT_EQ(atom.atom_type, pnot_test_struct.atom_type);
  EXPECT_EQ(atom.atom_index, pnot_test_struct.atom_index);
  EXPECT_EQ(ftell(fd), pnot_test_data_size);
}
// }}}1

// {{{1 wide atom unit tests
static const uint32_t wide_test_data_size = 16;
// clang-format off
static const unsigned char wide_test_data[wide_test_data_size] = {
    wide_test_data_size >> 24,  // size
    wide_test_data_size >> 16,
    wide_test_data_size >> 8,
    wide_test_data_size,
    'w', 'i', 'd', 'e',         // type
    0x00, 0x00, 0x00, 0x00,     // data
    0x00, 0x00, 0x00, 0x00,
};
// clang-format on
// clang-format off
static const MuTFFWideAtom wide_test_struct = {
    8,
};
// clang-format on

TEST(MuTFF, WriteWideAtom) {
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret = mutff_write_wide_atom(fd, &wide_test_struct);
  ASSERT_EQ(ret, wide_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, wide_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], wide_test_data[i]);
  }
}

TEST(MuTFF, ReadWideAtom) {
  MuTFFError ret;
  MuTFFWideAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(wide_test_data, wide_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_wide_atom(fd, &atom);
  ASSERT_EQ(ret, wide_test_data_size);

  EXPECT_EQ(atom.data_size, wide_test_struct.data_size);
  EXPECT_EQ(ftell(fd), wide_test_data_size);
}
// }}}1

// {{{1 skip atom unit tests
static const uint32_t skip_test_data_size = 16;
// clang-format off
static const unsigned char skip_test_data[skip_test_data_size] = {
    skip_test_data_size >> 24,  // size
    skip_test_data_size >> 16,
    skip_test_data_size >> 8,
    skip_test_data_size,
    's', 'k', 'i', 'p',         // type
    0x00, 0x00, 0x00, 0x00,     // data
    0x00, 0x00, 0x00, 0x00,
};
// clang-format on
// clang-format off
static const MuTFFSkipAtom skip_test_struct = {
    8,
};
// clang-format on

TEST(MuTFF, WriteSkipAtom) {
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret = mutff_write_skip_atom(fd, &skip_test_struct);
  ASSERT_EQ(ret, skip_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, skip_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], skip_test_data[i]);
  }
}

TEST(MuTFF, ReadSkipAtom) {
  MuTFFError ret;
  MuTFFSkipAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(skip_test_data, skip_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_skip_atom(fd, &atom);
  ASSERT_EQ(ret, skip_test_data_size);

  EXPECT_EQ(atom.data_size, skip_test_struct.data_size);
  EXPECT_EQ(ftell(fd), skip_test_data_size);
}
// }}}1

// {{{1 free atom unit tests
static const uint32_t free_test_data_size = 16;
// clang-format off
static const unsigned char free_test_data[free_test_data_size] = {
    free_test_data_size >> 24,  // size
    free_test_data_size >> 16,
    free_test_data_size >> 8,
    free_test_data_size,
    'f', 'r', 'e', 'e',         // type
    0x00, 0x00, 0x00, 0x00,     // data
    0x00, 0x00, 0x00, 0x00,
};
// clang-format on
// clang-format off
static const MuTFFFreeAtom free_test_struct = {
    8,
};
// clang-format on

TEST(MuTFF, WriteFreeAtom) {
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret = mutff_write_free_atom(fd, &free_test_struct);
  ASSERT_EQ(ret, free_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, free_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], free_test_data[i]);
  }
}

TEST(MuTFF, ReadFreeAtom) {
  MuTFFError ret;
  MuTFFFreeAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(free_test_data, free_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_free_atom(fd, &atom);
  ASSERT_EQ(ret, free_test_data_size);

  EXPECT_EQ(atom.data_size, free_test_struct.data_size);
  EXPECT_EQ(ftell(fd), free_test_data_size);
}
// }}}1

// {{{1 movie data atom unit tests
static const uint32_t mdat_test_data_size = 16;
// clang-format off
static const unsigned char mdat_test_data[mdat_test_data_size] = {
    mdat_test_data_size >> 24,  // size
    mdat_test_data_size >> 16,
    mdat_test_data_size >> 8,
    mdat_test_data_size,
    'm', 'd', 'a', 't',         // type
};
// clang-format on
// clang-format off
static const MuTFFMovieDataAtom mdat_test_struct = {
    8,
};
// clang-format on

TEST(MuTFF, WriteMovieDataAtom) {
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret = mutff_write_movie_data_atom(fd, &mdat_test_struct);
  ASSERT_EQ(ret, mdat_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, mdat_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], mdat_test_data[i]);
  }
}

TEST(MuTFF, ReadMovieDataAtom) {
  MuTFFError ret;
  MuTFFMovieDataAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(mdat_test_data, mdat_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_movie_data_atom(fd, &atom);
  ASSERT_EQ(ret, mdat_test_data_size);

  EXPECT_EQ(atom.data_size, mdat_test_struct.data_size);
  EXPECT_EQ(ftell(fd), mdat_test_data_size);
}
// }}}1

// {{{1 file type compatibility atom unit tests
static const uint32_t ftyp_test_data_size = 20;
// clang-format off
static const unsigned char ftyp_test_data[ftyp_test_data_size] = {
    ftyp_test_data_size >> 24,  // size
    ftyp_test_data_size >> 16,
    ftyp_test_data_size >> 8,
    ftyp_test_data_size,
    'f', 't', 'y', 'p',         // type
    'q', 't', ' ', ' ',         // major brand
    0x14, 0x04, 0x06, 0x00,     // minor version
    'q', 't', ' ', ' ',         // compatible brands[0]
};
// clang-format on
// clang-format off
static const MuTFFFileTypeAtom ftyp_test_struct = {
    MuTFF_FOURCC('q', 't', ' ', ' '),    // major brand
    0x14040600,              // minor version
    1,                       // compatible brands count
    MuTFF_FOURCC('q', 't', ' ', ' '),    // compatible brands[0]
};
// clang-format on

TEST(MuTFF, WriteFileTypeAtom) {
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret = mutff_write_file_type_atom(fd, &ftyp_test_struct);
  ASSERT_EQ(ret, ftyp_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, ftyp_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], ftyp_test_data[i]);
  }
}

TEST(MuTFF, ReadFileTypeAtom) {
  MuTFFError ret;
  MuTFFFileTypeAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(ftyp_test_data, ftyp_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_file_type_atom(fd, &atom);
  ASSERT_EQ(ret, ftyp_test_data_size);

  EXPECT_EQ(atom.major_brand, ftyp_test_struct.major_brand);
  EXPECT_EQ(atom.minor_version, ftyp_test_struct.minor_version);
  EXPECT_EQ(atom.compatible_brands_count,
            ftyp_test_struct.compatible_brands_count);
  EXPECT_EQ(atom.compatible_brands[0], ftyp_test_struct.compatible_brands[0]);
  EXPECT_EQ(ftell(fd), ftyp_test_data_size);
}
// }}}1

// {{{1 quickdraw region unit tests
static const uint32_t quickdraw_region_test_data_size = 14;
// clang-format off
static const unsigned char quickdraw_region_test_data[quickdraw_region_test_data_size] = {
    0x00, 0x0e,
    0x00, 0x01,
    0x10, 0x11,
    0x20, 0x21,
    0x30, 0x31,
    0x40, 0x41, 0x42, 0x43,
};
// clang-format on
// clang-format off
static const MuTFFQuickDrawRegion quickdraw_region_test_struct = {
    0x000e,                  // size
    (MuTFFQuickDrawRect) {   // rect
      0x0001,  // top
      0x1011,  // left
      0x2021,  // bottom
      0x3031,  // right
    },
    0x40, 0x41, 0x42, 0x43,  // data
};
// clang-format on

TEST(MuTFF, WriteQuickDrawRegion) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret =
      mutff_write_quickdraw_region(fd, &quickdraw_region_test_struct);
  ASSERT_EQ(ret, quickdraw_region_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, quickdraw_region_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], quickdraw_region_test_data[i]);
  }
}

TEST(MuTFF, ReadQuickDrawRegion) {
  MuTFFError ret;
  MuTFFQuickDrawRegion region;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(quickdraw_region_test_data, quickdraw_region_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_quickdraw_region(fd, &region);
  ASSERT_EQ(ret, quickdraw_region_test_data_size);

  EXPECT_EQ(region.size, quickdraw_region_test_struct.size);
  EXPECT_EQ(region.rect.top, quickdraw_region_test_struct.rect.top);
  EXPECT_EQ(region.rect.left, quickdraw_region_test_struct.rect.left);
  EXPECT_EQ(region.rect.bottom, quickdraw_region_test_struct.rect.bottom);
  EXPECT_EQ(region.rect.right, quickdraw_region_test_struct.rect.right);
  for (uint16_t i = 0; i < quickdraw_region_test_struct.size - 10; ++i) {
    EXPECT_EQ(region.data[i], quickdraw_region_test_struct.data[i]);
  }
  EXPECT_EQ(ftell(fd), quickdraw_region_test_data_size);
}
// }}}1

// {{{1 quickdraw rect unit tests
static const uint32_t quickdraw_rect_test_data_size = 8;
// clang-format off
static const unsigned char quickdraw_rect_test_data[quickdraw_rect_test_data_size] = {
    0x00, 0x01,
    0x10, 0x11,
    0x20, 0x21,
    0x30, 0x31,
};
// clang-format on
// clang-format off
static const MuTFFQuickDrawRect quickdraw_rect_test_struct = {
  0x0001,
  0x1011,
  0x2021,
  0x3031,
};
// clang-format on

TEST(MuTFF, WriteQuickDrawRect) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  const MuTFFError ret =
      mutff_write_quickdraw_rect(fd, &quickdraw_rect_test_struct);
  ASSERT_EQ(ret, quickdraw_rect_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, quickdraw_rect_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], quickdraw_rect_test_data[i]);
  }
}

TEST(MuTFF, ReadQuickDrawRect) {
  MuTFFError ret;
  MuTFFQuickDrawRect rect;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(quickdraw_rect_test_data, quickdraw_rect_test_data_size, 1, fd);
  rewind(fd);
  ret = mutff_read_quickdraw_rect(fd, &rect);
  ASSERT_EQ(ret, quickdraw_rect_test_data_size);

  EXPECT_EQ(rect.top, quickdraw_rect_test_struct.top);
  EXPECT_EQ(rect.left, quickdraw_rect_test_struct.left);
  EXPECT_EQ(rect.bottom, quickdraw_rect_test_struct.bottom);
  EXPECT_EQ(rect.right, quickdraw_rect_test_struct.right);
  EXPECT_EQ(ftell(fd), quickdraw_rect_test_data_size);
}
// }}}1

TEST(MuTFF, PeekAtomHeader) {
  MuTFFAtomHeader header;
  FILE *fd = fopen("temp.mov", "w+b");
  const char data_size = 8;
  char data[data_size] = {0x01, 0x02, 0x03, 0x04, 'a', 'b', 'c', 'd'};
  fwrite(data, data_size, 1, fd);
  rewind(fd);
  mutff_peek_atom_header(fd, &header);

  EXPECT_EQ(header.size, 0x01020304);
  EXPECT_EQ(header.type, MuTFF_FOURCC('a', 'b', 'c', 'd'));
  EXPECT_EQ(ftell(fd), 0);
}

// vi:sw=2:ts=2:et:fdm=marker
