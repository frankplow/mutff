#include <gtest/gtest.h>
#include <stdio.h>
#include <string.h>

#include <cstdio>

extern "C" {
#include "mutff.h"
}

#define ARR(...) \
  { __VA_ARGS__ }

// {{{1 quickdraw rect unit tests
static const uint32_t quickdraw_rect_test_data_size = 8;
// clang-format off
#define QUICKDRAW_RECT_TEST_DATA \
    0x00, 0x01,                  \
    0x10, 0x11,                  \
    0x20, 0x21,                  \
    0x30, 0x31
// clang-format on
static const unsigned char
    quickdraw_rect_test_data[quickdraw_rect_test_data_size] =
        ARR(QUICKDRAW_RECT_TEST_DATA);
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
  size_t bytes;
  const MuTFFError err =
      mutff_write_quickdraw_rect(fd, &bytes, &quickdraw_rect_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, quickdraw_rect_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, quickdraw_rect_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], quickdraw_rect_test_data[i]);
  }
}

static inline void expect_quickdraw_rect_eq(const MuTFFQuickDrawRect *a,
                                            const MuTFFQuickDrawRect *b) {
  EXPECT_EQ(a->top, b->top);
  EXPECT_EQ(a->left, b->left);
  EXPECT_EQ(a->bottom, b->bottom);
  EXPECT_EQ(a->right, b->right);
}

TEST(MuTFF, ReadQuickDrawRect) {
  MuTFFError err;
  size_t bytes;
  MuTFFQuickDrawRect rect;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(quickdraw_rect_test_data, quickdraw_rect_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_quickdraw_rect(fd, &bytes, &rect);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, quickdraw_rect_test_data_size);

  expect_quickdraw_rect_eq(&rect, &quickdraw_rect_test_struct);
  EXPECT_EQ(ftell(fd), quickdraw_rect_test_data_size);
}
// }}}1

// {{{1 quickdraw region unit tests
static const uint32_t quickdraw_region_test_data_size =
    quickdraw_rect_test_data_size + 6;
// clang-format off
#define QUICKDRAW_REGION_TEST_DATA \
  0x00, 0x0e,                      \
  QUICKDRAW_RECT_TEST_DATA,        \
  0x40, 0x41, 0x42, 0x43
// clang-format on
static const unsigned char
    quickdraw_region_test_data[quickdraw_region_test_data_size] =
        ARR(QUICKDRAW_REGION_TEST_DATA);
// clang-format off
static const MuTFFQuickDrawRegion quickdraw_region_test_struct = {
    0x000e,                      // size
    quickdraw_rect_test_struct,  // rect
    0x40, 0x41, 0x42, 0x43,      // data
};
// clang-format on

TEST(MuTFF, WriteQuickDrawRegion) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err =
      mutff_write_quickdraw_region(fd, &bytes, &quickdraw_region_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, quickdraw_region_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, quickdraw_region_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], quickdraw_region_test_data[i]);
  }
}

static inline void expect_quickdraw_region_eq(const MuTFFQuickDrawRegion *a,
                                              const MuTFFQuickDrawRegion *b) {
  EXPECT_EQ(a->size, b->size);
  expect_quickdraw_rect_eq(&a->rect, &b->rect);
  for (uint16_t i = 0; i < b->size - 10; ++i) {
    EXPECT_EQ(a->data[i], b->data[i]);
  }
}

TEST(MuTFF, ReadQuickDrawRegion) {
  MuTFFError err;
  size_t bytes;
  MuTFFQuickDrawRegion region;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(quickdraw_region_test_data, quickdraw_region_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_quickdraw_region(fd, &bytes, &region);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, quickdraw_region_test_data_size);

  expect_quickdraw_region_eq(&region, &quickdraw_region_test_struct);
  EXPECT_EQ(ftell(fd), quickdraw_region_test_data_size);
}
// }}}1

// {{{1 file type compatibility atom unit tests
static const uint32_t ftyp_test_data_size = 20;
// clang-format off
#define FTYP_TEST_DATA         \
    ftyp_test_data_size >> 24, \
    ftyp_test_data_size >> 16, \
    ftyp_test_data_size >> 8,  \
    ftyp_test_data_size,       \
    'f', 't', 'y', 'p',        \
    'q', 't', ' ', ' ',        \
    0x14, 0x04, 0x06, 0x00,    \
    'q', 't', ' ', ' '
// clang-format on
static const unsigned char ftyp_test_data[ftyp_test_data_size] =
    ARR(FTYP_TEST_DATA);
// clang-format off
static const MuTFFFileTypeAtom ftyp_test_struct = {
    MuTFF_FOURCC('q', 't', ' ', ' '),  // major brand
    0x14040600,                        // minor version
    1,                                 // compatible brands count
    MuTFF_FOURCC('q', 't', ' ', ' '),  // compatible brands[0]
};
// clang-format on

TEST(MuTFF, WriteFileTypeAtom) {
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err =
      mutff_write_file_type_atom(fd, &bytes, &ftyp_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, ftyp_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, ftyp_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], ftyp_test_data[i]);
  }
}

static inline void expect_filetype_eq(const MuTFFFileTypeAtom *a,
                                      const MuTFFFileTypeAtom *b) {
  EXPECT_EQ(a->major_brand, b->major_brand);
  EXPECT_EQ(a->minor_version, b->minor_version);
  EXPECT_EQ(a->compatible_brands_count, b->compatible_brands_count);
  size_t count;
  if (a->compatible_brands_count <= b->compatible_brands_count) {
    count = a->compatible_brands_count;
  } else {
    count = b->compatible_brands_count;
  }
  for (size_t i = 0; i < count; ++i) {
    EXPECT_EQ(a->compatible_brands[i], b->compatible_brands[i]);
  }
}

TEST(MuTFF, ReadFileTypeAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFFileTypeAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(ftyp_test_data, ftyp_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_file_type_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, ftyp_test_data_size);

  expect_filetype_eq(&atom, &ftyp_test_struct);
  EXPECT_EQ(ftell(fd), ftyp_test_data_size);
}
// }}}1

// {{{1 movie data atom unit tests
static const uint32_t mdat_test_data_size = 8;
// clang-format off
#define MDAT_TEST_DATA         \
    mdat_test_data_size >> 24, \
    mdat_test_data_size >> 16, \
    mdat_test_data_size >> 8,  \
    mdat_test_data_size,       \
    'm', 'd', 'a', 't'
// clang-format on
static const unsigned char mdat_test_data[mdat_test_data_size] =
    ARR(MDAT_TEST_DATA);
// clang-format off
static const MuTFFMovieDataAtom mdat_test_struct = {
    0,
};
// clang-format on

TEST(MuTFF, WriteMovieDataAtom) {
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err =
      mutff_write_movie_data_atom(fd, &bytes, &mdat_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, mdat_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, mdat_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], mdat_test_data[i]);
  }
}

static inline void expect_mdat_eq(const MuTFFMovieDataAtom *a,
                                  const MuTFFMovieDataAtom *b) {
  EXPECT_EQ(a->data_size, b->data_size);
}

TEST(MuTFF, ReadMovieDataAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFMovieDataAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(mdat_test_data, mdat_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_movie_data_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, mdat_test_data_size);

  expect_mdat_eq(&atom, &mdat_test_struct);
  EXPECT_EQ(ftell(fd), mdat_test_data_size);
}
// }}}1

// {{{1 free atom unit tests
static const uint32_t free_test_data_size = 16;
// clang-format off
#define FREE_TEST_DATA         \
    free_test_data_size >> 24, \
    free_test_data_size >> 16, \
    free_test_data_size >> 8,  \
    free_test_data_size,       \
    'f', 'r', 'e', 'e',        \
    0x00, 0x00, 0x00, 0x00,    \
    0x00, 0x00, 0x00, 0x00
// clang-format on
static const unsigned char free_test_data[free_test_data_size] =
    ARR(FREE_TEST_DATA);
// clang-format off
static const MuTFFFreeAtom free_test_struct = {
    free_test_data_size,
};
// clang-format on

TEST(MuTFF, WriteFreeAtom) {
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err = mutff_write_free_atom(fd, &bytes, &free_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, free_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, free_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], free_test_data[i]);
  }
}

static inline void expect_free_eq(const MuTFFFreeAtom *a,
                                  const MuTFFFreeAtom *b) {
  EXPECT_EQ(a->atom_size, b->atom_size);
}

TEST(MuTFF, ReadFreeAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFFreeAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(free_test_data, free_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_free_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, free_test_data_size);

  expect_free_eq(&atom, &free_test_struct);
  EXPECT_EQ(ftell(fd), free_test_data_size);
}
// }}}1

// {{{1 skip atom unit tests
static const uint32_t skip_test_data_size = 16;
// clang-format off
#define SKIP_TEST_DATA         \
    skip_test_data_size >> 24, \
    skip_test_data_size >> 16, \
    skip_test_data_size >> 8,  \
    skip_test_data_size,       \
    's', 'k', 'i', 'p',        \
    0x00, 0x00, 0x00, 0x00,    \
    0x00, 0x00, 0x00, 0x00
// clang-format on
static const unsigned char skip_test_data[skip_test_data_size] =
    ARR(SKIP_TEST_DATA);
// clang-format off
static const MuTFFSkipAtom skip_test_struct = {
    skip_test_data_size,
};
// clang-format on

TEST(MuTFF, WriteSkipAtom) {
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err = mutff_write_skip_atom(fd, &bytes, &skip_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, skip_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, skip_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], skip_test_data[i]);
  }
}

static inline void expect_skip_eq(const MuTFFSkipAtom *a,
                                  const MuTFFSkipAtom *b) {
  EXPECT_EQ(a->atom_size, b->atom_size);
}

TEST(MuTFF, ReadSkipAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFSkipAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(skip_test_data, skip_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_skip_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, skip_test_data_size);

  expect_skip_eq(&atom, &skip_test_struct);
  EXPECT_EQ(ftell(fd), skip_test_data_size);
}
// }}}1

// {{{1 wide atom unit tests
static const uint32_t wide_test_data_size = 16;
// clang-format off
#define WIDE_TEST_DATA         \
    wide_test_data_size >> 24, \
    wide_test_data_size >> 16, \
    wide_test_data_size >> 8,  \
    wide_test_data_size,       \
    'w', 'i', 'd', 'e',        \
    0x00, 0x00, 0x00, 0x00,    \
    0x00, 0x00, 0x00, 0x00
// clang-format on
static const unsigned char wide_test_data[wide_test_data_size] =
    ARR(WIDE_TEST_DATA);
// clang-format off
static const MuTFFWideAtom wide_test_struct = {
    wide_test_data_size,
};
// clang-format on

TEST(MuTFF, WriteWideAtom) {
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err = mutff_write_wide_atom(fd, &bytes, &wide_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, wide_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, wide_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], wide_test_data[i]);
  }
}

static inline void expect_wide_eq(const MuTFFWideAtom *a,
                                  const MuTFFWideAtom *b) {
  EXPECT_EQ(a->atom_size, b->atom_size);
}

TEST(MuTFF, ReadWideAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFWideAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(wide_test_data, wide_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_wide_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, wide_test_data_size);

  expect_wide_eq(&atom, &wide_test_struct);
  EXPECT_EQ(ftell(fd), wide_test_data_size);
}
// }}}1

// {{{1 preview atom unit tests
static const uint32_t pnot_test_data_size = 20;
// clang-format off
#define PNOT_TEST_DATA         \
    pnot_test_data_size >> 24, \
    pnot_test_data_size >> 16, \
    pnot_test_data_size >> 8,  \
    pnot_test_data_size,       \
    'p',  'n',  'o',  't',     \
    0x01, 0x02, 0x03, 0x04,    \
    0x01, 0x02,                \
    'a',  'b',  'c',  'd',     \
    0x01, 0x02
// clang-format on
static const unsigned char pnot_test_data[pnot_test_data_size] =
    ARR(PNOT_TEST_DATA);
// clang-format off
static const MuTFFPreviewAtom pnot_test_struct = {
    0x01020304,                        // modification time
    0x0102,                            // version
    MuTFF_FOURCC('a', 'b', 'c', 'd'),  // atom type
    0x0102,                            // atom index
};
// clang-format on

TEST(MuTFF, WritePreviewAtom) {
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err =
      mutff_write_preview_atom(fd, &bytes, &pnot_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, pnot_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, pnot_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], pnot_test_data[i]);
  }
}

static inline void expect_pnot_eq(const MuTFFPreviewAtom *a,
                                  const MuTFFPreviewAtom *b) {
  EXPECT_EQ(a->modification_time, b->modification_time);
  EXPECT_EQ(a->version, b->version);
  EXPECT_EQ(a->atom_type, b->atom_type);
  EXPECT_EQ(a->atom_index, b->atom_index);
}

TEST(MuTFF, ReadPreviewAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFPreviewAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(pnot_test_data, pnot_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_preview_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, pnot_test_data_size);

  expect_pnot_eq(&atom, &pnot_test_struct);
  EXPECT_EQ(ftell(fd), pnot_test_data_size);
}
// }}}1

// {{{1 movie header atom unit tests
static const uint32_t mvhd_test_data_size = 108;
// clang-format off
#define MVHD_TEST_DATA                                      \
    mvhd_test_data_size >> 24,     /* size */               \
    mvhd_test_data_size >> 16,                              \
    mvhd_test_data_size >> 8,                               \
    mvhd_test_data_size,                                    \
    'm', 'v', 'h', 'd',            /* type */               \
    0x01,                          /* version */            \
    0x01, 0x02, 0x03,              /* flags */              \
    0x01, 0x02, 0x03, 0x04,        /* creation_time */      \
    0x01, 0x02, 0x03, 0x04,        /* modification_time */  \
    0x01, 0x02, 0x03, 0x04,        /* time_scale */         \
    0x01, 0x02, 0x03, 0x04,        /* duration */           \
    0x01, 0x02, 0x03, 0x04,        /* preferred_rate */     \
    0x01, 0x02,                    /* preferred_volume */   \
    0x00, 0x00, 0x00, 0x00, 0x00,                           \
    0x00, 0x00, 0x00, 0x00, 0x00,  /* reserved */           \
    0x00, 0x01, 0x00, 0x02,        /* matrix_structure */   \
    0x00, 0x03, 0x00, 0x04,                                 \
    0x00, 0x00, 0x00, 0x00,                                 \
    0x00, 0x07, 0x00, 0x08,                                 \
    0x00, 0x09, 0x00, 0x0a,                                 \
    0x00, 0x00, 0x00, 0x00,                                 \
    0x00, 0x0d, 0x00, 0x0e,                                 \
    0x00, 0x0f, 0x00, 0x10,                                 \
    0x00, 0x00, 0x00, 0x00,                                 \
    0x01, 0x02, 0x03, 0x04,        /* preview_time */       \
    0x01, 0x02, 0x03, 0x04,        /* preview_duration */   \
    0x01, 0x02, 0x03, 0x04,        /* poster_time */        \
    0x01, 0x02, 0x03, 0x04,        /* selection_time */     \
    0x01, 0x02, 0x03, 0x04,        /* selection_duration */ \
    0x01, 0x02, 0x03, 0x04,        /* current_time */       \
    0x01, 0x02, 0x03, 0x04         /* next_track_id */
// clang-format on
static const unsigned char mvhd_test_data[mvhd_test_data_size] =
    ARR(MVHD_TEST_DATA);
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
    {
      {1, 2},                      // matrix structure
      {3, 4},                      //
      {0, 0},                      //
      {7, 8},                      //
      {9, 10},                     //
      {0, 0},                    //
      {13, 14},                    //
      {15, 16},                    //
      {0, 0},                    //
    },
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
  size_t bytes;
  const MuTFFError err =
      mutff_write_movie_header_atom(fd, &bytes, &mvhd_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, mvhd_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, mvhd_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], mvhd_test_data[i]);
  }
}

static inline void expect_matrix_eq(const MuTFFMatrix *a,
                                    const MuTFFMatrix *b) {
  EXPECT_EQ(a->a.integral, b->a.integral);
  EXPECT_EQ(a->a.fractional, b->a.fractional);
  EXPECT_EQ(a->b.integral, b->b.integral);
  EXPECT_EQ(a->b.fractional, b->b.fractional);
  EXPECT_EQ(a->u.integral, b->u.integral);
  EXPECT_EQ(a->u.fractional, b->u.fractional);
  EXPECT_EQ(a->c.integral, b->c.integral);
  EXPECT_EQ(a->c.fractional, b->c.fractional);
  EXPECT_EQ(a->d.integral, b->d.integral);
  EXPECT_EQ(a->d.fractional, b->d.fractional);
  EXPECT_EQ(a->v.integral, b->v.integral);
  EXPECT_EQ(a->v.fractional, b->v.fractional);
  EXPECT_EQ(a->tx.integral, b->tx.integral);
  EXPECT_EQ(a->tx.fractional, b->tx.fractional);
  EXPECT_EQ(a->ty.integral, b->ty.integral);
  EXPECT_EQ(a->ty.fractional, b->ty.fractional);
  EXPECT_EQ(a->w.integral, b->w.integral);
  EXPECT_EQ(a->w.fractional, b->w.fractional);
}

static inline void expect_mvhd_eq(const MuTFFMovieHeaderAtom *a,
                                  const MuTFFMovieHeaderAtom *b) {
  EXPECT_EQ(a->version, b->version);
  EXPECT_EQ(a->flags, b->flags);
  EXPECT_EQ(a->creation_time, b->creation_time);
  EXPECT_EQ(a->modification_time, b->modification_time);
  EXPECT_EQ(a->time_scale, b->time_scale);
  EXPECT_EQ(a->duration, b->duration);
  EXPECT_EQ(a->preferred_rate.integral, b->preferred_rate.integral);
  EXPECT_EQ(a->preferred_rate.fractional, b->preferred_rate.fractional);
  EXPECT_EQ(a->preferred_volume.integral, b->preferred_volume.integral);
  EXPECT_EQ(a->preferred_volume.fractional, b->preferred_volume.fractional);
  expect_matrix_eq(&a->matrix_structure, &b->matrix_structure);
  EXPECT_EQ(a->preview_time, b->preview_time);
  EXPECT_EQ(a->preview_duration, b->preview_duration);
  EXPECT_EQ(a->poster_time, b->poster_time);
  EXPECT_EQ(a->selection_time, b->selection_time);
  EXPECT_EQ(a->selection_duration, b->selection_duration);
  EXPECT_EQ(a->current_time, b->current_time);
  EXPECT_EQ(a->next_track_id, b->next_track_id);
}

TEST(MuTFF, ReadMovieHeaderAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFMovieHeaderAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(mvhd_test_data, mvhd_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_movie_header_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, mvhd_test_data_size);

  expect_mvhd_eq(&atom, &mvhd_test_struct);
  EXPECT_EQ(ftell(fd), mvhd_test_data_size);
}
// }}}1

// {{{1 clipping region atom unit tests
static const uint32_t crgn_test_data_size = 8 + quickdraw_region_test_data_size;
// clang-format off
#define CRGN_TEST_DATA                      \
    crgn_test_data_size >> 24,  /* size */  \
    crgn_test_data_size >> 16,              \
    crgn_test_data_size >> 8,               \
    crgn_test_data_size,                    \
    'c', 'r', 'g', 'n',         /* type */  \
    QUICKDRAW_REGION_TEST_DATA  /* region*/
// clang-format on
static const unsigned char crgn_test_data[crgn_test_data_size] =
    ARR(CRGN_TEST_DATA);
// clang-format off
static const MuTFFClippingRegionAtom crgn_test_struct = {
  quickdraw_region_test_struct
};
// clang-format on

TEST(MuTFF, WriteClippingRegionAtom) {
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err =
      mutff_write_clipping_region_atom(fd, &bytes, &crgn_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, crgn_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, crgn_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], crgn_test_data[i]);
  }
}

static inline void expect_crgn_eq(const MuTFFClippingRegionAtom *a,
                                  const MuTFFClippingRegionAtom *b) {
  expect_quickdraw_region_eq(&a->region, &b->region);
}

TEST(MuTFF, ReadClippingRegionAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFClippingRegionAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(crgn_test_data, crgn_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_clipping_region_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, crgn_test_data_size);

  expect_crgn_eq(&atom, &crgn_test_struct);
  EXPECT_EQ(ftell(fd), crgn_test_data_size);
}
// }}}1

// {{{1 clipping atom unit tests
static const uint32_t clip_test_data_size = 8 + crgn_test_data_size;
// clang-format off
#define CLIP_TEST_DATA                     \
    clip_test_data_size >> 24,  /* size */ \
    clip_test_data_size >> 16,             \
    clip_test_data_size >> 8,              \
    clip_test_data_size,                   \
    'c', 'l', 'i', 'p',         /* type */ \
    CRGN_TEST_DATA
// clang-format on
static const unsigned char clip_test_data[clip_test_data_size] =
    ARR(CLIP_TEST_DATA);
// clang-format off
static const MuTFFClippingAtom clip_test_struct = {
  crgn_test_struct  // clipping_region
};
// clang-format on

TEST(MuTFF, WriteClippingAtom) {
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err =
      mutff_write_clipping_atom(fd, &bytes, &clip_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, clip_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, clip_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], clip_test_data[i]);
  }
}

static inline void expect_clip_eq(const MuTFFClippingAtom *a,
                                  const MuTFFClippingAtom *b) {
  expect_crgn_eq(&a->clipping_region, &b->clipping_region);
}

TEST(MuTFF, ReadClippingAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFClippingAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(clip_test_data, clip_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_clipping_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, clip_test_data_size);

  expect_clip_eq(&atom, &clip_test_struct);
  EXPECT_EQ(ftell(fd), clip_test_data_size);
}
// }}}1

// {{{1 color table atom unit tests
static const uint32_t ctab_test_data_size = 32;
// clang-format off
#define CTAB_TEST_DATA                                                      \
    ctab_test_data_size >> 24,                       /* size */             \
    ctab_test_data_size >> 16,                                              \
    ctab_test_data_size >> 8,                                               \
    ctab_test_data_size,                                                    \
    'c',  't',  'a',  'b',                           /* type */             \
    0x00, 0x01, 0x02, 0x03,                          /* seed */             \
    0x00, 0x01,                                      /* flags */            \
    0x00, 0x01,                                      /* color table size */ \
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,  /* color table[0] */   \
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17   /* color table[1] */
// clang-format on
static const unsigned char ctab_test_data[ctab_test_data_size] =
    ARR(CTAB_TEST_DATA);
// clang-format off
static const MuTFFColorTableAtom ctab_test_struct = {
    0x00010203,  // color table seed
    0x0001,      // color table flags
    0x0001,      // color table size
    0x0001,      // color table[0][0]
    0x0203,      // color table[0][1]
    0x0405,      // color table[0][2]
    0x0607,      // color table[0][3]
    0x1011,      // color table[1][0]
    0x1213,      // color table[1][1]
    0x1415,      // color table[1][2]
    0x1617,      // color table[1][3]
};
// clang-format on

TEST(MuTFF, WriteColorTableAtom) {
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err =
      mutff_write_color_table_atom(fd, &bytes, &ctab_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, ctab_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, ctab_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], ctab_test_data[i]);
  }
}

static inline void expect_ctab_eq(const MuTFFColorTableAtom *a,
                                  const MuTFFColorTableAtom *b) {
  EXPECT_EQ(a->color_table_seed, b->color_table_seed);
  EXPECT_EQ(a->color_table_flags, b->color_table_flags);
  EXPECT_EQ(a->color_table_size, b->color_table_size);
  for (size_t i = 0; i < a->color_table_size; ++i) {
    EXPECT_EQ(a->color_array[i][0], b->color_array[i][0]);
    EXPECT_EQ(a->color_array[i][1], b->color_array[i][1]);
    EXPECT_EQ(a->color_array[i][2], b->color_array[i][2]);
    EXPECT_EQ(a->color_array[i][3], b->color_array[i][3]);
  }
}

TEST(MuTFF, ReadColorTableAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFColorTableAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(ctab_test_data, ctab_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_color_table_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, ctab_test_data_size);

  expect_ctab_eq(&atom, &ctab_test_struct);
  EXPECT_EQ(ftell(fd), ctab_test_data_size);
}
// }}}1

// {{{1 user data list entry unit tests
static const uint32_t udta_entry_test_data_size = 16;
// clang-format off
#define UDTA_ENTRY_TEST_DATA                           \
    udta_entry_test_data_size >> 24,  /* size */ \
    udta_entry_test_data_size >> 16,             \
    udta_entry_test_data_size >> 8,              \
    udta_entry_test_data_size,                   \
    'a', 'b', 'c', 'd',               /* type */ \
    'e', 'f', 'g', 'h',               /* data */ \
    0, 1, 2, 3
// clang-format on
static const unsigned char udta_entry_test_data[udta_entry_test_data_size] =
    ARR(UDTA_ENTRY_TEST_DATA);
// clang-format off
static const MuTFFUserDataListEntry udta_entry_test_struct = {
    MuTFF_FOURCC('a', 'b', 'c', 'd'),  // type
    8,                                 // data size
    {                                  // data
      'e', 'f', 'g', 'h',
      0, 1, 2, 3,
    }
};
// clang-format on

TEST(MuTFF, WriteUserDataListEntry) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err =
      mutff_write_user_data_list_entry(fd, &bytes, &udta_entry_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, udta_entry_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, udta_entry_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], udta_entry_test_data[i]);
  }
}

static inline void expect_udta_entry_eq(const MuTFFUserDataListEntry *a,
                                        const MuTFFUserDataListEntry *b) {
  EXPECT_EQ(a->type, b->type);
  for (size_t i = 0; i < a->data_size; ++i) {
    EXPECT_EQ(a->data[i], b->data[i]);
  }
}

TEST(MuTFF, ReadUserDataListEntry) {
  MuTFFError err;
  size_t bytes;
  MuTFFUserDataListEntry atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(udta_entry_test_data, udta_entry_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_user_data_list_entry(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, udta_entry_test_data_size);

  expect_udta_entry_eq(&atom, &udta_entry_test_struct);
  EXPECT_EQ(ftell(fd), udta_entry_test_data_size);
}
// }}}1

// @TODO: test multiple entries
// {{{1 user data atom unit tests
static const uint32_t udta_test_data_size = 8 + udta_entry_test_data_size;
// clang-format off
#define UDTA_TEST_DATA                                  \
    udta_test_data_size >> 24,  /* size */              \
    udta_test_data_size >> 16,                          \
    udta_test_data_size >> 8,                           \
    udta_test_data_size,                                \
    'u',  'd',  't',  'a',      /* type */              \
    UDTA_ENTRY_TEST_DATA        /* user_data_list[0] */
// clang-format on
static const unsigned char udta_test_data[udta_test_data_size] =
    ARR(UDTA_TEST_DATA);
// clang-format off
static const MuTFFUserDataAtom udta_test_struct = {
    1,
    {
      udta_entry_test_struct,
    }
};
// clang-format on

TEST(MuTFF, WriteUserDataAtom) {
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err =
      mutff_write_user_data_atom(fd, &bytes, &udta_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, udta_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, udta_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], udta_test_data[i]);
  }
}

static inline void expect_udta_eq(const MuTFFUserDataAtom *a,
                                  const MuTFFUserDataAtom *b) {
  EXPECT_EQ(a->list_entries, b->list_entries);
  size_t list_entries =
      a->list_entries > b->list_entries ? b->list_entries : a->list_entries;
  for (size_t i = 0; i < list_entries; ++i) {
    expect_udta_entry_eq(&a->user_data_list[i], &b->user_data_list[i]);
  }
}

TEST(MuTFF, ReadUserDataAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFUserDataAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(udta_test_data, udta_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_user_data_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, udta_test_data_size);

  expect_udta_eq(&atom, &udta_test_struct);
  EXPECT_EQ(ftell(fd), udta_test_data_size);
}
// }}}1

// {{{1 track header atom unit tests
static const uint32_t tkhd_test_data_size = 92;
// clang-format off
#define TKHD_TEST_DATA                                  \
    tkhd_test_data_size >> 24,  /* size */              \
    tkhd_test_data_size >> 16,                          \
    tkhd_test_data_size >> 8,                           \
    tkhd_test_data_size,                                \
    't', 'k', 'h', 'd',         /* type */              \
    0x00,                       /* version */           \
    0x00, 0x01, 0x02,           /* flags */             \
    0x00, 0x01, 0x02, 0x03,     /* creation time */     \
    0x00, 0x01, 0x02, 0x03,     /* modification time */ \
    0x00, 0x01, 0x02, 0x03,     /* track ID */          \
    0x00, 0x00, 0x00, 0x00,     /* reserved */          \
    0x00, 0x01, 0x02, 0x03,     /* duration */          \
    0x00, 0x00, 0x00, 0x00,     /* reserved */          \
    0x00, 0x00, 0x00, 0x00,     /* reserved */          \
    0x00, 0x01,                 /* layer */             \
    0x00, 0x01,                 /* alternate group */   \
    0x00, 0x01,                 /* volume */            \
    0x00, 0x00,                 /* reserved */          \
    0x00, 0x01, 0x00, 0x02,     /* matrix_structure */  \
    0x00, 0x03, 0x00, 0x04,                             \
    0x00, 0x00, 0x00, 0x00,                             \
    0x00, 0x07, 0x00, 0x08,                             \
    0x00, 0x09, 0x00, 0x0a,                             \
    0x00, 0x00, 0x00, 0x00,                             \
    0x00, 0x0d, 0x00, 0x0e,                             \
    0x00, 0x0f, 0x00, 0x10,                             \
    0x00, 0x00, 0x00, 0x00,                             \
    0x00, 0x01, 0x02, 0x03,     /* track width */       \
    0x00, 0x01, 0x02, 0x03      /* track height */
// clang-format on
static const unsigned char tkhd_test_data[tkhd_test_data_size] =
    ARR(TKHD_TEST_DATA);
// clang-format off
static const MuTFFTrackHeaderAtom tkhd_test_struct = {
    0x00,              // version
    0x000102,          // flags
    0x00010203,        // creation time
    0x00010203,        // modification time
    0x00010203,        // track id
    0x00010203,        // duration
    0x0001,            // layer
    0x0001,            // alternate group
    {0x00, 0x01},      // volume
    {
      {1, 2},          // matrix structure
      {3, 4},          //
      {0, 0},          //
      {7, 8},          //
      {9, 10},         //
      {0, 0},          //
      {13, 14},        //
      {15, 16},        //
      {0, 0},          //
    },
    {0x0001, 0x0203},  // track width
    {0x0001, 0x0203},  // track height
};
// clang-format on

TEST(MuTFF, WriteTrackHeaderAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err =
      mutff_write_track_header_atom(fd, &bytes, &tkhd_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, tkhd_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, tkhd_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], tkhd_test_data[i]);
  }
}

static inline void expect_tkhd_eq(const MuTFFTrackHeaderAtom *a,
                                  const MuTFFTrackHeaderAtom *b) {
  EXPECT_EQ(a->version, b->version);
  EXPECT_EQ(a->creation_time, b->creation_time);
  EXPECT_EQ(a->modification_time, b->modification_time);
  EXPECT_EQ(a->track_id, b->track_id);
  EXPECT_EQ(a->duration, b->duration);
  EXPECT_EQ(a->layer, b->layer);
  EXPECT_EQ(a->alternate_group, b->alternate_group);
  EXPECT_EQ(a->volume.integral, b->volume.integral);
  EXPECT_EQ(a->volume.fractional, b->volume.fractional);
  expect_matrix_eq(&a->matrix_structure, &b->matrix_structure);
  EXPECT_EQ(a->track_width.integral, b->track_width.integral);
  EXPECT_EQ(a->track_width.fractional, b->track_width.fractional);
  EXPECT_EQ(a->track_height.integral, b->track_height.integral);
  EXPECT_EQ(a->track_height.fractional, b->track_height.fractional);
}

TEST(MuTFF, ReadTrackHeaderAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFTrackHeaderAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(tkhd_test_data, tkhd_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_track_header_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, tkhd_test_data_size);

  expect_tkhd_eq(&atom, &tkhd_test_struct);
  EXPECT_EQ(ftell(fd), tkhd_test_data_size);
}
// }}}1

// {{{1 track clean aperture dimensions atom unit tests
static const uint32_t clef_test_data_size = 20;
// clang-format off
#define CLEF_TEST_DATA                        \
    clef_test_data_size >> 24,  /* size */    \
    clef_test_data_size >> 16,                \
    clef_test_data_size >> 8,                 \
    clef_test_data_size,                      \
    'c', 'l', 'e', 'f',         /* type */    \
    0x00,                       /* version */ \
    0x00, 0x01, 0x02,           /* flags */   \
    0x00, 0x01, 0x02, 0x03,     /* width */   \
    0x10, 0x11, 0x12, 0x13      /* height */
// clang-format on
static const unsigned char clef_test_data[clef_test_data_size] =
    ARR(CLEF_TEST_DATA);
// clang-format off
static const MuTFFTrackCleanApertureDimensionsAtom clef_test_struct = {
    0x00,              // version
    0x000102,          // flags
    {0x0001, 0x0203},  // width
    {0x1011, 0x1213},  // height
};
// clang-format on

TEST(MuTFF, WriteTrackCleanApertureDimensionsAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err = mutff_write_track_clean_aperture_dimensions_atom(
      fd, &bytes, &clef_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, clef_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, clef_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], clef_test_data[i]);
  }
}

static inline void expect_clef_eq(
    const MuTFFTrackCleanApertureDimensionsAtom *a,
    const MuTFFTrackCleanApertureDimensionsAtom *b) {
  EXPECT_EQ(a->version, b->version);
  EXPECT_EQ(a->flags, b->flags);
  EXPECT_EQ(a->width.integral, b->width.integral);
  EXPECT_EQ(a->width.fractional, b->width.fractional);
  EXPECT_EQ(a->height.integral, b->height.integral);
  EXPECT_EQ(a->height.fractional, b->height.fractional);
}

TEST(MuTFF, ReadTrackCleanApertureDimensionsAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFTrackCleanApertureDimensionsAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(clef_test_data, clef_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_track_clean_aperture_dimensions_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, clef_test_data_size);

  expect_clef_eq(&atom, &clef_test_struct);
  EXPECT_EQ(ftell(fd), clef_test_data_size);
}
// }}}1

// {{{1 track production aperture dimensions atom unit tests
static const uint32_t prof_test_data_size = 20;
// clang-format off
#define PROF_TEST_DATA                        \
    prof_test_data_size >> 24,  /* size */    \
    prof_test_data_size >> 16,                \
    prof_test_data_size >> 8,                 \
    prof_test_data_size,                      \
    'p', 'r', 'o', 'f',         /* type */    \
    0x00,                       /* version */ \
    0x00, 0x01, 0x02,           /* flags */   \
    0x00, 0x01, 0x02, 0x03,     /* width */   \
    0x10, 0x11, 0x12, 0x13      /* height */
// clang-format on
static const unsigned char prof_test_data[prof_test_data_size] =
    ARR(PROF_TEST_DATA);
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
  size_t bytes;
  const MuTFFError err = mutff_write_track_production_aperture_dimensions_atom(
      fd, &bytes, &prof_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, prof_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, prof_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], prof_test_data[i]);
  }
}

static inline void expect_prof_eq(
    const MuTFFTrackProductionApertureDimensionsAtom *a,
    const MuTFFTrackProductionApertureDimensionsAtom *b) {
  EXPECT_EQ(a->version, b->version);
  EXPECT_EQ(a->flags, b->flags);
  EXPECT_EQ(a->width.integral, b->width.integral);
  EXPECT_EQ(a->width.fractional, b->width.fractional);
  EXPECT_EQ(a->height.integral, b->height.integral);
  EXPECT_EQ(a->height.fractional, b->height.fractional);
}

TEST(MuTFF, ReadTrackProductionApertureDimensionsAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFTrackProductionApertureDimensionsAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(prof_test_data, prof_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_track_production_aperture_dimensions_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, prof_test_data_size);

  expect_prof_eq(&atom, &prof_test_struct);
  EXPECT_EQ(ftell(fd), prof_test_data_size);
}
// }}}1

// {{{1 track encoded pixels dimensions atom unit tests
static const uint32_t enof_test_data_size = 20;
// clang-format off
#define ENOF_TEST_DATA                        \
    enof_test_data_size >> 24,  /* size */    \
    enof_test_data_size >> 16,                \
    enof_test_data_size >> 8,                 \
    enof_test_data_size,                      \
    'e', 'n', 'o', 'f',         /* type */    \
    0x00,                       /* version */ \
    0x00, 0x01, 0x02,           /* flags */   \
    0x00, 0x01, 0x02, 0x03,     /* width */   \
    0x10, 0x11, 0x12, 0x13      /* height */
// clang-format on
static const unsigned char enof_test_data[enof_test_data_size] =
    ARR(ENOF_TEST_DATA);
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
  size_t bytes;
  const MuTFFError err = mutff_write_track_encoded_pixels_dimensions_atom(
      fd, &bytes, &enof_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, enof_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, enof_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], enof_test_data[i]);
  }
}

static inline void expect_enof_eq(
    const MuTFFTrackEncodedPixelsDimensionsAtom *a,
    const MuTFFTrackEncodedPixelsDimensionsAtom *b) {
  EXPECT_EQ(a->version, b->version);
  EXPECT_EQ(a->flags, b->flags);
  EXPECT_EQ(a->width.integral, b->width.integral);
  EXPECT_EQ(a->width.fractional, b->width.fractional);
  EXPECT_EQ(a->height.integral, b->height.integral);
  EXPECT_EQ(a->height.fractional, b->height.fractional);
}

TEST(MuTFF, ReadTrackEncodedPixelsDimensionsAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFTrackEncodedPixelsDimensionsAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(enof_test_data, enof_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_track_encoded_pixels_dimensions_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, enof_test_data_size);

  expect_enof_eq(&atom, &enof_test_struct);
  EXPECT_EQ(ftell(fd), enof_test_data_size);
}
// }}}1

// {{{1 track aperture mode dimensions atom unit tests
static const uint32_t tapt_test_data_size = 68;
// clang-format off
#define TAPT_TEST_DATA                     \
    tapt_test_data_size >> 24,  /* size */ \
    tapt_test_data_size >> 16,             \
    tapt_test_data_size >> 8,              \
    tapt_test_data_size,                   \
    't', 'a', 'p', 't',         /* type */ \
    CLEF_TEST_DATA,                        \
    PROF_TEST_DATA,                        \
    ENOF_TEST_DATA
// clang-format on
static const unsigned char tapt_test_data[tapt_test_data_size] =
    ARR(TAPT_TEST_DATA);
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
  size_t bytes;
  const MuTFFError err = mutff_write_track_aperture_mode_dimensions_atom(
      fd, &bytes, &tapt_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, tapt_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, tapt_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], tapt_test_data[i]);
  }
}

static inline void expect_tapt_eq(
    const MuTFFTrackApertureModeDimensionsAtom *a,
    const MuTFFTrackApertureModeDimensionsAtom *b) {
  expect_clef_eq(&a->track_clean_aperture_dimensions,
                 &b->track_clean_aperture_dimensions);
  expect_prof_eq(&a->track_production_aperture_dimensions,
                 &b->track_production_aperture_dimensions);
  expect_enof_eq(&a->track_encoded_pixels_dimensions,
                 &b->track_encoded_pixels_dimensions);
}

TEST(MuTFF, ReadTrackApertureModeDimensionsAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFTrackApertureModeDimensionsAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(tapt_test_data, tapt_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_track_aperture_mode_dimensions_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, tapt_test_data_size);

  expect_tapt_eq(&atom, &tapt_test_struct);
  EXPECT_EQ(ftell(fd), tapt_test_data_size);
}
// }}}1

// {{{1 video sample description unit tests
static const uint32_t video_sample_desc_test_data_size = 42;
// clang-format off
#define VIDEO_SAMPLE_DESC_TEST_DATA                      \
    0x00, 0x00,              /* version */               \
    0x00, 0x00,              /* revision level */        \
    'a', 'b', 'c', 'd',      /* vendor */                \
    0x00, 0x01, 0x02, 0x03,  /* temporal quality */      \
    0x10, 0x11, 0x12, 0x13,  /* spatial quality */       \
    0x20, 0x21,              /* width */                 \
    0x30, 0x31,              /* height */                \
    0x40, 0x41, 0x42, 0x43,  /* horizontal resolution */ \
    0x50, 0x51, 0x52, 0x53,  /* vertical resolution */   \
    0x00, 0x00, 0x00, 0x00,  /* data size */             \
    0x60, 0x61,              /* frame count */           \
    'e', 'f', 'g', 'h',      /* compressor name */       \
    0x70, 0x71,              /* depth */                 \
    0x80, 0x81               /* color table id */
// clang-format on
static unsigned char
    video_sample_desc_test_data[video_sample_desc_test_data_size] =
        ARR(VIDEO_SAMPLE_DESC_TEST_DATA);
// clang-format off
static const MuTFFVideoSampleDescription video_sample_desc_test_struct = {
  0,  // version
  MuTFF_FOURCC('a', 'b', 'c', 'd'),  // vendor
  0x00010203,                        // temporal quality
  0x10111213,                        // spatial quality
  0x2021,                            // width
  0x3031,                            // height
  {0x4041, 0x4243},                  // horizontal resolution
  {0x5051, 0x5253},                  // vertical resolution
  0x6061,                            // frame count
  MuTFF_FOURCC('e', 'f', 'g', 'h'),  // compressor name
  0x7071,                            // depth
  0x8081,                            // color table id
};
// clang-format on

TEST(MuTFF, WriteVideoSampleDescription) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err = mutff_write_video_sample_description(
      fd, &bytes, &video_sample_desc_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, video_sample_desc_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, video_sample_desc_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], video_sample_desc_test_data[i]);
  }
}

static inline void expect_video_sample_desc_eq(
    const MuTFFVideoSampleDescription *a,
    const MuTFFVideoSampleDescription *b) {
  EXPECT_EQ(a->version, b->version);
  EXPECT_EQ(a->vendor, b->vendor);
  EXPECT_EQ(a->temporal_quality, b->temporal_quality);
  EXPECT_EQ(a->spatial_quality, b->spatial_quality);
  EXPECT_EQ(a->width, b->width);
  EXPECT_EQ(a->height, b->height);
  EXPECT_EQ(a->horizontal_resolution.integral,
            b->horizontal_resolution.integral);
  EXPECT_EQ(a->horizontal_resolution.fractional,
            b->horizontal_resolution.fractional);
  EXPECT_EQ(a->vertical_resolution.integral, b->vertical_resolution.integral);
  EXPECT_EQ(a->vertical_resolution.fractional,
            b->vertical_resolution.fractional);
  EXPECT_EQ(a->frame_count, b->frame_count);
  EXPECT_EQ(a->compressor_name, b->compressor_name);
  EXPECT_EQ(a->depth, b->depth);
  EXPECT_EQ(a->color_table_id, b->color_table_id);
}

TEST(MuTFF, ReadVideoSampleDescription) {
  MuTFFError err;
  size_t bytes;
  MuTFFVideoSampleDescription atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(video_sample_desc_test_data, video_sample_desc_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_video_sample_description(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, video_sample_desc_test_data_size);

  expect_video_sample_desc_eq(&atom, &video_sample_desc_test_struct);
  EXPECT_EQ(ftell(fd), video_sample_desc_test_data_size);
}
// }}}1

// {{{1 sample description unit tests
static const uint32_t sample_desc_test_data_size =
    16 + video_sample_desc_test_data_size;
// clang-format off
#define SAMPLE_DESC_TEST_DATA                                       \
    sample_desc_test_data_size >> 24,    /* size */                 \
    sample_desc_test_data_size >> 16,                               \
    sample_desc_test_data_size >> 8,                                \
    sample_desc_test_data_size,                                     \
    'r', 'a', 'w', ' ',                  /* data format */          \
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* reserved */             \
    0x00, 0x01,                          /* data reference index */ \
    VIDEO_SAMPLE_DESC_TEST_DATA
// clang-format on
static const unsigned char sample_desc_test_data[sample_desc_test_data_size] =
    ARR(SAMPLE_DESC_TEST_DATA);
// clang-format off
static const MuTFFSampleDescription sample_desc_test_struct = {
    MuTFF_FOURCC('r', 'a', 'w', ' '),           // data format
    0x0001,
    video_sample_desc_test_struct,
};
// clang-format on

TEST(MuTFF, WriteSampleDescription) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err =
      mutff_write_sample_description(fd, &bytes, &sample_desc_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, sample_desc_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, sample_desc_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], sample_desc_test_data[i]);
  }
}

static inline void expect_sample_desc_eq(const MuTFFSampleDescription *a,
                                         const MuTFFSampleDescription *b) {
  EXPECT_EQ(a->data_format, b->data_format);
  EXPECT_EQ(a->data_reference_index, b->data_reference_index);
  // @TODO: assert media-specific data is equal
}

TEST(MuTFF, ReadSampleDescription) {
  MuTFFError err;
  size_t bytes;
  MuTFFSampleDescription atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(sample_desc_test_data, sample_desc_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_sample_description(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, sample_desc_test_data_size);

  expect_sample_desc_eq(&atom, &sample_desc_test_struct);
  EXPECT_EQ(ftell(fd), sample_desc_test_data_size);
}
// }}}1

// {{{1 compressed matte atom unit tests
static const uint32_t kmat_test_data_size = 12 + sample_desc_test_data_size + 4;
// clang-format off
#define KMAT_TEST_DATA                                    \
    kmat_test_data_size >> 24,           /* size */       \
    kmat_test_data_size >> 16,                            \
    kmat_test_data_size >> 8,                             \
    kmat_test_data_size,                                  \
    'k', 'm', 'a', 't',                  /* type */       \
    0x00,                                /* version */    \
    0x00, 0x01, 0x02,                    /* flags */      \
    SAMPLE_DESC_TEST_DATA,                                \
    0x00, 0x01, 0x02, 0x03               /* matte data */
// clang-format on
static const unsigned char kmat_test_data[kmat_test_data_size] =
    ARR(KMAT_TEST_DATA);
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
  size_t bytes;
  const MuTFFError err =
      mutff_write_compressed_matte_atom(fd, &bytes, &kmat_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, kmat_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, kmat_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], kmat_test_data[i]);
  }
}

static inline void expect_kmat_eq(const MuTFFCompressedMatteAtom *a,
                                  const MuTFFCompressedMatteAtom *b) {
  EXPECT_EQ(a->version, b->version);
  EXPECT_EQ(a->flags, b->flags);
  expect_sample_desc_eq(&a->matte_image_description_structure,
                        &b->matte_image_description_structure);
  EXPECT_EQ(a->matte_data_len, b->matte_data_len);
  const size_t matte_data_len = a->matte_data_len > b->matte_data_len
                                    ? b->matte_data_len
                                    : a->matte_data_len;
  for (size_t i = 0; i < matte_data_len; ++i) {
    EXPECT_EQ(a->matte_data[i], b->matte_data[i]);
  }
}

TEST(MuTFF, ReadCompressedMatteAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFCompressedMatteAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(kmat_test_data, kmat_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_compressed_matte_atom(fd, &bytes, &atom);
  /* ASSERT_EQ(err, MuTFFErrorNone); */
  /* EXPECT_EQ(bytes, kmat_test_data_size); */

  /* expect_kmat_eq(&atom, &kmat_test_struct); */
  /* EXPECT_EQ(ftell(fd), kmat_test_data_size); */
}
// }}}1

// {{{1 track matte atom unit tests
static const uint32_t matt_test_data_size = 8 + kmat_test_data_size;
// clang-format off
#define MATT_TEST_DATA                     \
    matt_test_data_size >> 24,  /* size */ \
    matt_test_data_size >> 16,             \
    matt_test_data_size >> 8,              \
    matt_test_data_size,                   \
    'm', 'a', 't', 't',         /* type */ \
    KMAT_TEST_DATA
// clang-format on
static const unsigned char matt_test_data[matt_test_data_size] =
    ARR(MATT_TEST_DATA);
// clang-format off
static const MuTFFTrackMatteAtom matt_test_struct = {
    kmat_test_struct,      // compressed matte
};
// clang-format on

TEST(MuTFF, WriteTrackMatteAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err =
      mutff_write_track_matte_atom(fd, &bytes, &matt_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, matt_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, matt_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], matt_test_data[i]);
  }
}

static inline void expect_matt_eq(const MuTFFTrackMatteAtom *a,
                                  const MuTFFTrackMatteAtom *b) {
  expect_kmat_eq(&a->compressed_matte_atom, &b->compressed_matte_atom);
}

TEST(MuTFF, ReadTrackMatteAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFTrackMatteAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(matt_test_data, matt_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_track_matte_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, matt_test_data_size);

  expect_matt_eq(&atom, &matt_test_struct);
  EXPECT_EQ(ftell(fd), matt_test_data_size);
}
// }}}1

// {{{1 edit list entry unit tests
static const uint32_t edit_list_entry_test_data_size = 12;
// clang-format off
#define EDIT_LIST_ENTRY_TEST_DATA                 \
    0x00, 0x01, 0x02, 0x03,  /* track duration */ \
    0x10, 0x11, 0x12, 0x13,  /* media time */     \
    0x20, 0x21, 0x22, 0x23   /* media rate */
// clang-format on
static const unsigned char
    edit_list_entry_test_data[edit_list_entry_test_data_size] =
        ARR(EDIT_LIST_ENTRY_TEST_DATA);
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
  size_t bytes;
  const MuTFFError err =
      mutff_write_edit_list_entry(fd, &bytes, &edit_list_entry_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, edit_list_entry_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, edit_list_entry_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], edit_list_entry_test_data[i]);
  }
}

static inline void expect_edit_list_entry_eq(const MuTFFEditListEntry *a,
                                             const MuTFFEditListEntry *b) {
  EXPECT_EQ(a->track_duration, b->track_duration);
  EXPECT_EQ(a->media_time, b->media_time);
  EXPECT_EQ(a->media_rate.integral, b->media_rate.integral);
  EXPECT_EQ(a->media_rate.fractional, b->media_rate.fractional);
}

TEST(MuTFF, ReadEditListEntry) {
  MuTFFError err;
  size_t bytes;
  MuTFFEditListEntry atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(edit_list_entry_test_data, edit_list_entry_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_edit_list_entry(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, edit_list_entry_test_data_size);

  expect_edit_list_entry_eq(&atom, &edit_list_entry_test_struct);
  EXPECT_EQ(ftell(fd), edit_list_entry_test_data_size);
}
// }}}1

// @TODO: test multiple entries
// {{{1 edit list atom unit tests
static const uint32_t elst_test_data_size = 16 + edit_list_entry_test_data_size;
// clang-format off
#define ELST_TEST_DATA                                  \
    elst_test_data_size >> 24,  /* size */              \
    elst_test_data_size >> 16,                          \
    elst_test_data_size >> 8,                           \
    elst_test_data_size,                                \
    'e', 'l', 's', 't',         /* type */              \
    0x00,                       /* version */           \
    0x00, 0x01, 0x02,           /* flags */             \
    0x00, 0x00, 0x00, 0x01,     /* number of entries */ \
    EDIT_LIST_ENTRY_TEST_DATA
// clang-format on
static const unsigned char elst_test_data[elst_test_data_size] =
    ARR(ELST_TEST_DATA);
// clang-format off
static const MuTFFEditListAtom elst_test_struct = {
    0x00,                  // version
    0x000102,              // flags
    1,            // number of entries
    {
      edit_list_entry_test_struct
    }
};
// clang-format on

TEST(MuTFF, WriteEditListAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err =
      mutff_write_edit_list_atom(fd, &bytes, &elst_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, elst_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, elst_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], elst_test_data[i]);
  }
}

static inline void expect_elst_eq(const MuTFFEditListAtom *a,
                                  const MuTFFEditListAtom *b) {
  EXPECT_EQ(a->version, b->version);
  EXPECT_EQ(a->flags, b->flags);
  EXPECT_EQ(a->number_of_entries, b->number_of_entries);
  const size_t number_of_entries = a->number_of_entries > b->number_of_entries
                                       ? b->number_of_entries
                                       : a->number_of_entries;
  for (size_t i = 0; i < number_of_entries; ++i) {
    expect_edit_list_entry_eq(&a->edit_list_table[i], &b->edit_list_table[i]);
  }
}

TEST(MuTFF, ReadEditListAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFEditListAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(elst_test_data, elst_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_edit_list_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, elst_test_data_size);

  EXPECT_EQ(ftell(fd), elst_test_data_size);
}
// }}}1

// {{{1 edit atom unit tests
static const uint32_t edts_test_data_size = 8 + elst_test_data_size;
// clang-format off
#define EDTS_TEST_DATA                     \
    edts_test_data_size >> 24,  /* size */ \
    edts_test_data_size >> 16,             \
    edts_test_data_size >> 8,              \
    edts_test_data_size,                   \
    'e', 'd', 't', 's',         /* type */ \
    ELST_TEST_DATA
// clang-format on
static const unsigned char edts_test_data[edts_test_data_size] =
    ARR(EDTS_TEST_DATA);
// clang-format off
static const MuTFFEditAtom edts_test_struct = {
    elst_test_struct,        // edit list atom
};
// clang-format on

TEST(MuTFF, WriteEditAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err = mutff_write_edit_atom(fd, &bytes, &edts_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, edts_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, edts_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], edts_test_data[i]);
  }
}

static inline void expect_edts_eq(const MuTFFEditAtom *a,
                                  const MuTFFEditAtom *b) {
  expect_elst_eq(&a->edit_list_atom, &b->edit_list_atom);
}

TEST(MuTFF, ReadEditAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFEditAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(edts_test_data, edts_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_edit_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, edts_test_data_size);

  expect_edts_eq(&atom, &edts_test_struct);
  EXPECT_EQ(ftell(fd), edts_test_data_size);
}
// }}}1

// {{{1 track reference type atom unit tests
static const uint32_t track_ref_atom_test_data_size = 16;
// clang-format off
#define TRACK_REF_TEST_DATA                                  \
    track_ref_atom_test_data_size >> 24,  /* size */         \
    track_ref_atom_test_data_size >> 16,                     \
    track_ref_atom_test_data_size >> 8,                      \
    track_ref_atom_test_data_size,                           \
    'a', 'b', 'c', 'd',                   /* type */         \
    0x00, 0x01, 0x02, 0x03,               /* track_ids[0] */ \
    0x10, 0x11, 0x12, 0x13                /* track_ids[1] */
// clang-format on
static const unsigned char
    track_ref_atom_test_data[track_ref_atom_test_data_size] =
        ARR(TRACK_REF_TEST_DATA);
// clang-format off
static const MuTFFTrackReferenceTypeAtom track_ref_atom_test_struct = {
    MuTFF_FOURCC('a', 'b', 'c', 'd'),  // type
    2,                                 // track id count
    0x00010203,                        // track ids[0]
    0x10111213,                        // track ids[1]
};
// clang-format on

TEST(MuTFF, WriteTrackReferenceTypeAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err = mutff_write_track_reference_type_atom(
      fd, &bytes, &track_ref_atom_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, track_ref_atom_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, track_ref_atom_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], track_ref_atom_test_data[i]);
  }
}

static inline void expect_track_ref_eq(const MuTFFTrackReferenceTypeAtom *a,
                                       const MuTFFTrackReferenceTypeAtom *b) {
  EXPECT_EQ(a->type, b->type);
  EXPECT_EQ(a->track_id_count, b->track_id_count);
  const size_t track_id_count = a->track_id_count > b->track_id_count
                                    ? b->track_id_count
                                    : a->track_id_count;
  for (size_t i = 0; i < track_id_count; ++i) {
    EXPECT_EQ(a->track_ids[i], b->track_ids[i]);
  }
}

TEST(MuTFF, ReadTrackReferenceTypeAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFTrackReferenceTypeAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(track_ref_atom_test_data, track_ref_atom_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_track_reference_type_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, track_ref_atom_test_data_size);

  expect_track_ref_eq(&atom, &track_ref_atom_test_struct);
  EXPECT_EQ(ftell(fd), track_ref_atom_test_data_size);
}
// }}}1

// @TODO: test multiple entries
// {{{1 track reference atom unit tests
static const uint32_t tref_test_data_size = 8 + track_ref_atom_test_data_size;
// clang-format off
#define TREF_TEST_DATA                     \
    tref_test_data_size >> 24,  /* size */ \
    tref_test_data_size >> 16,             \
    tref_test_data_size >> 8,              \
    tref_test_data_size,                   \
    't', 'r', 'e', 'f',         /* type */ \
    TRACK_REF_TEST_DATA
// clang-format on
static const unsigned char tref_test_data[tref_test_data_size] =
    ARR(TREF_TEST_DATA);
// clang-format off
static const MuTFFTrackReferenceAtom tref_test_struct = {
    1,                            // track reference type count
    {
      track_ref_atom_test_struct
    },
};
// clang-format on

TEST(MuTFF, WriteTrackReferenceAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err =
      mutff_write_track_reference_atom(fd, &bytes, &tref_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, tref_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, tref_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], tref_test_data[i]);
  }
}

static inline void expect_tref_eq(const MuTFFTrackReferenceAtom *a,
                                  const MuTFFTrackReferenceAtom *b) {
  EXPECT_EQ(a->track_reference_type_count, b->track_reference_type_count);
  const size_t track_reference_type_count =
      a->track_reference_type_count > b->track_reference_type_count
          ? b->track_reference_type_count
          : a->track_reference_type_count;
  for (size_t i = 0; i < track_reference_type_count; ++i) {
    expect_track_ref_eq(&a->track_reference_type[i],
                        &b->track_reference_type[i]);
  }
}

TEST(MuTFF, ReadTrackReferenceAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFTrackReferenceAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(tref_test_data, tref_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_track_reference_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, tref_test_data_size);

  expect_tref_eq(&atom, &tref_test_struct);
  EXPECT_EQ(ftell(fd), tref_test_data_size);
}
// }}}1

// {{{1 track exclude from autoselection atom unit tests
static const uint32_t txas_test_data_size = 8;
// clang-format off
#define TXAS_TEST_DATA                     \
    txas_test_data_size >> 24,  /* size */ \
    txas_test_data_size >> 16,             \
    txas_test_data_size >> 8,              \
    txas_test_data_size,                   \
    't', 'x', 'a', 's'          /* type */
// clang-format on
static const unsigned char txas_test_data[txas_test_data_size] =
    ARR(TXAS_TEST_DATA);
// clang-format off
static const MuTFFTrackExcludeFromAutoselectionAtom txas_test_struct = {
};
// clang-format on

TEST(MuTFF, WriteTrackExcludeFromAutoselectionAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err = mutff_write_track_exclude_from_autoselection_atom(
      fd, &bytes, &txas_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, txas_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, txas_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], txas_test_data[i]);
  }
}

static inline void expect_txas_eq(
    const MuTFFTrackExcludeFromAutoselectionAtom *a,
    const MuTFFTrackExcludeFromAutoselectionAtom *b) {}

TEST(MuTFF, ReadTrackExcludeFromAutoselectionAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFTrackExcludeFromAutoselectionAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(txas_test_data, txas_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_track_exclude_from_autoselection_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, txas_test_data_size);

  expect_txas_eq(&atom, &txas_test_struct);
  EXPECT_EQ(ftell(fd), txas_test_data_size);
}
// }}}1

// {{{1 track load settings atom unit tests
static const uint32_t load_test_data_size = 24;
// clang-format off
#define LOAD_TEST_DATA                                   \
    load_test_data_size >> 24,  /* size */               \
    load_test_data_size >> 16,                           \
    load_test_data_size >> 8,                            \
    load_test_data_size,                                 \
    'l', 'o', 'a', 'd',         /* type */               \
    0x00, 0x01, 0x02, 0x03,     /* preload start time */ \
    0x10, 0x11, 0x12, 0x13,     /* preload duration */   \
    0x20, 0x21, 0x22, 0x23,     /* preload flags */      \
    0x30, 0x31, 0x32, 0x33      /* default hints */
// clang-format on
static const unsigned char load_test_data[load_test_data_size] =
    ARR(LOAD_TEST_DATA);
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
  size_t bytes;
  const MuTFFError err =
      mutff_write_track_load_settings_atom(fd, &bytes, &load_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, load_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, load_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], load_test_data[i]);
  }
}

static inline void expect_load_eq(const MuTFFTrackLoadSettingsAtom *a,
                                  const MuTFFTrackLoadSettingsAtom *b) {
  EXPECT_EQ(a->preload_start_time, b->preload_start_time);
  EXPECT_EQ(a->preload_duration, b->preload_duration);
  EXPECT_EQ(a->preload_flags, b->preload_flags);
  EXPECT_EQ(a->default_hints, b->default_hints);
}

TEST(MuTFF, ReadTrackLoadSettingsAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFTrackLoadSettingsAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(load_test_data, load_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_track_load_settings_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, load_test_data_size);

  expect_load_eq(&atom, &load_test_struct);
  EXPECT_EQ(ftell(fd), load_test_data_size);
}
// }}}1

// {{{1 object id atom unit tests
static const uint32_t obid_test_data_size = 12;
// clang-format off
#define OBID_TEST_DATA                          \
    obid_test_data_size >> 24,  /* size */      \
    obid_test_data_size >> 16,                  \
    obid_test_data_size >> 8,                   \
    obid_test_data_size,                        \
    'o', 'b', 'i', 'd',         /* type */      \
    0x00, 0x01, 0x02, 0x03      /* object id */
// clang-format on
static const unsigned char obid_test_data[obid_test_data_size] =
    ARR(OBID_TEST_DATA);
// clang-format off
static const MuTFFObjectIDAtom obid_test_struct = {
    0x00010203,              // object id
};
// clang-format on

TEST(MuTFF, WriteObjectIDAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err =
      mutff_write_object_id_atom(fd, &bytes, &obid_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, obid_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, obid_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], obid_test_data[i]);
  }
}

static inline void expect_obid_eq(const MuTFFObjectIDAtom *a,
                                  const MuTFFObjectIDAtom *b) {
  EXPECT_EQ(a->object_id, b->object_id);
}

TEST(MuTFF, ReadObjectIDAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFObjectIDAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(obid_test_data, obid_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_object_id_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, obid_test_data_size);

  expect_obid_eq(&atom, &obid_test_struct);
  EXPECT_EQ(ftell(fd), obid_test_data_size);
}
// }}}1

// {{{1 input type atom unit tests
static const uint32_t ty_test_data_size = 12;
// clang-format off
#define TY_TEST_DATA                           \
    ty_test_data_size >> 24,  /* size */       \
    ty_test_data_size >> 16,                   \
    ty_test_data_size >> 8,                    \
    ty_test_data_size,                         \
    '\0', '\0', 't', 'y',     /* type */       \
    0x00, 0x01, 0x02, 0x03    /* input type */
// clang-format on
static const unsigned char ty_test_data[ty_test_data_size] = ARR(TY_TEST_DATA);
// clang-format off
static const MuTFFInputTypeAtom ty_test_struct = {
    0x00010203,              // input type
};
// clang-format on

TEST(MuTFF, WriteInputTypeAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err =
      mutff_write_input_type_atom(fd, &bytes, &ty_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, ty_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, ty_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], ty_test_data[i]);
  }
}

static inline void expect_ty_eq(const MuTFFInputTypeAtom *a,
                                const MuTFFInputTypeAtom *b) {
  EXPECT_EQ(a->input_type, b->input_type);
}

TEST(MuTFF, ReadInputTypeAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFInputTypeAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(ty_test_data, ty_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_input_type_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, ty_test_data_size);

  expect_ty_eq(&atom, &ty_test_struct);
  EXPECT_EQ(ftell(fd), ty_test_data_size);
}
// }}}1

// {{{1 track input atom unit tests
static const uint32_t in_test_data_size =
    20 + ty_test_data_size + obid_test_data_size;
// clang-format off
#define IN_TEST_DATA                            \
    in_test_data_size >> 24,  /* size */        \
    in_test_data_size >> 16,                    \
    in_test_data_size >> 8,                     \
    in_test_data_size,                          \
    '\0', '\0', 'i', 'n',     /* type */        \
    0x00, 0x01, 0x02, 0x03,   /* atom id */     \
    0x00, 0x00,               /* reserved */    \
    0x00, 0x02,               /* child count */ \
    0x00, 0x00, 0x00, 0x00,   /* reserved */    \
    TY_TEST_DATA,                               \
    OBID_TEST_DATA
// clang-format on
static const unsigned char in_test_data[in_test_data_size] = ARR(IN_TEST_DATA);
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
  size_t bytes;
  const MuTFFError err =
      mutff_write_track_input_atom(fd, &bytes, &in_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, in_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, in_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], in_test_data[i]);
  }
}

static inline void expect_in_eq(const MuTFFTrackInputAtom *a,
                                const MuTFFTrackInputAtom *b) {
  EXPECT_EQ(a->atom_id, b->atom_id);
  EXPECT_EQ(a->child_count, b->child_count);
  expect_ty_eq(&a->input_type_atom, &b->input_type_atom);
  EXPECT_EQ(a->object_id_atom_present, b->object_id_atom_present);
  const bool object_id_atom_present =
      a->object_id_atom_present && b->object_id_atom_present;
  if (object_id_atom_present) {
    expect_obid_eq(&a->object_id_atom, &b->object_id_atom);
  }
}

TEST(MuTFF, ReadTrackInputAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFTrackInputAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(in_test_data, in_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_track_input_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, in_test_data_size);

  expect_in_eq(&atom, &in_test_struct);
  EXPECT_EQ(ftell(fd), in_test_data_size);
}
// }}}1

// @TODO: test multiple entries
// {{{1 track input map atom unit tests
static const uint32_t imap_test_data_size = 8 + in_test_data_size;
// clang-format off
#define IMAP_TEST_DATA                     \
    imap_test_data_size >> 24,  /* size */ \
    imap_test_data_size >> 16,             \
    imap_test_data_size >> 8,              \
    imap_test_data_size,                   \
    'i', 'm', 'a', 'p',         /* type */ \
    IN_TEST_DATA
// clang-format on
static const unsigned char imap_test_data[imap_test_data_size] =
    ARR(IMAP_TEST_DATA);
// clang-format off
static const MuTFFTrackInputMapAtom imap_test_struct = {
    1,                       // track input atom count
    {                        // track input atoms
      in_test_struct,
    }
};
// clang-format on

TEST(MuTFF, WriteTrackInputMapAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err =
      mutff_write_track_input_map_atom(fd, &bytes, &imap_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, imap_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, imap_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], imap_test_data[i]);
  }
}

static inline void expect_imap_eq(const MuTFFTrackInputMapAtom *a,
                                  const MuTFFTrackInputMapAtom *b) {
  EXPECT_EQ(a->track_input_atom_count, b->track_input_atom_count);
  const size_t track_input_atom_count =
      a->track_input_atom_count > b->track_input_atom_count
          ? b->track_input_atom_count
          : a->track_input_atom_count;
  for (size_t i = 0; i < track_input_atom_count; ++i) {
    expect_in_eq(&a->track_input_atoms[i], &b->track_input_atoms[i]);
  }
}

TEST(MuTFF, ReadTrackInputMapAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFTrackInputMapAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(imap_test_data, imap_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_track_input_map_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, imap_test_data_size);

  expect_imap_eq(&atom, &imap_test_struct);
  EXPECT_EQ(ftell(fd), imap_test_data_size);
}
// }}}1

// {{{1 media header atom unit tests
static const uint32_t mdhd_test_data_size = 32;
// clang-format off
#define MDHD_TEST_DATA                                  \
    mdhd_test_data_size >> 24,  /* size */              \
    mdhd_test_data_size >> 16,                          \
    mdhd_test_data_size >> 8,                           \
    mdhd_test_data_size,                                \
    'm', 'd', 'h', 'd',         /* type */              \
    0x00,                       /* version */           \
    0x00, 0x01, 0x02,           /* flags */             \
    0x00, 0x01, 0x02, 0x03,     /* creation time */     \
    0x10, 0x11, 0x12, 0x13,     /* modification time */ \
    0x20, 0x21, 0x22, 0x23,     /* time scale */        \
    0x30, 0x31, 0x32, 0x33,     /* duration */          \
    0x40, 0x41,                 /* language */          \
    0x50, 0x51                  /* quality */
// clang-format on
static const unsigned char mdhd_test_data[mdhd_test_data_size] =
    ARR(MDHD_TEST_DATA);
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
  size_t bytes;
  const MuTFFError err =
      mutff_write_media_header_atom(fd, &bytes, &mdhd_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, mdhd_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, mdhd_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], mdhd_test_data[i]);
  }
}

static inline void expect_mdhd_eq(const MuTFFMediaHeaderAtom *a,
                                  const MuTFFMediaHeaderAtom *b) {
  EXPECT_EQ(a->version, b->version);
  EXPECT_EQ(a->flags, b->flags);
  EXPECT_EQ(a->creation_time, b->creation_time);
  EXPECT_EQ(a->modification_time, b->modification_time);
  EXPECT_EQ(a->time_scale, b->time_scale);
  EXPECT_EQ(a->duration, b->duration);
  EXPECT_EQ(a->language, b->language);
  EXPECT_EQ(a->quality, b->quality);
}

TEST(MuTFF, ReadMediaHeaderAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFMediaHeaderAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(mdhd_test_data, mdhd_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_media_header_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, mdhd_test_data_size);

  expect_mdhd_eq(&atom, &mdhd_test_struct);
  EXPECT_EQ(ftell(fd), mdhd_test_data_size);
}
// }}}1

// {{{1 extended language tag atom unit tests
static const uint32_t elng_test_data_size = 18;
// clang-format off
#define ELNG_TEST_DATA                                        \
    elng_test_data_size >> 24,      /* size */                \
    elng_test_data_size >> 16,                                \
    elng_test_data_size >> 8,                                 \
    elng_test_data_size,                                      \
    'e', 'l', 'n', 'g',             /* type */                \
    0x00,                           /* version */             \
    0x00, 0x01, 0x02,               /* flags */               \
    'e', 'n', '-', 'U', 'S', '\0'   /* language tag string */
// clang-format on
static const unsigned char elng_test_data[elng_test_data_size] =
    ARR(ELNG_TEST_DATA);
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
  size_t bytes;
  const MuTFFError err =
      mutff_write_extended_language_tag_atom(fd, &bytes, &elng_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, elng_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, elng_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], elng_test_data[i]);
  }
}

static inline void expect_elng_eq(const MuTFFExtendedLanguageTagAtom *a,
                                  const MuTFFExtendedLanguageTagAtom *b) {
  EXPECT_EQ(a->version, b->version);
  EXPECT_EQ(a->flags, b->flags);
  EXPECT_STREQ(a->language_tag_string, b->language_tag_string);
}

TEST(MuTFF, ReadExtendedLanguageTagAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFExtendedLanguageTagAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(elng_test_data, elng_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_extended_language_tag_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, elng_test_data_size);

  expect_elng_eq(&atom, &elng_test_struct);
  EXPECT_EQ(ftell(fd), elng_test_data_size);
}
// }}}1

// {{{1 handler reference atom unit tests
static const uint32_t hdlr_test_data_size = 36;
// clang-format off
#define HDLR_TEST_DATA                                       \
    hdlr_test_data_size >> 24,  /* size */                   \
    hdlr_test_data_size >> 16,                               \
    hdlr_test_data_size >> 8,                                \
    hdlr_test_data_size,                                     \
    'h', 'd', 'l', 'r',         /* type */                   \
    0x00,                       /* version */                \
    0x00, 0x01, 0x02,           /* flags */                  \
    0x00, 0x01, 0x02, 0x03,     /* component type */         \
    0x10, 0x11, 0x12, 0x13,     /* component subtype */      \
    0x20, 0x21, 0x22, 0x23,     /* component manufacturer */ \
    0x30, 0x31, 0x32, 0x33,     /* component flags */        \
    0x40, 0x41, 0x42, 0x43,     /* component flags mask */   \
    'a', 'b', 'c', 'd'          /* component name */
// clang-format on
static const unsigned char hdlr_test_data[hdlr_test_data_size] =
    ARR(HDLR_TEST_DATA);
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
  size_t bytes;
  const MuTFFError err =
      mutff_write_handler_reference_atom(fd, &bytes, &hdlr_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, hdlr_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, hdlr_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], hdlr_test_data[i]);
  }
}

static inline void expect_hdlr_eq(const MuTFFHandlerReferenceAtom *a,
                                  const MuTFFHandlerReferenceAtom *b) {
  EXPECT_EQ(a->version, b->version);
  EXPECT_EQ(a->flags, b->flags);
  EXPECT_EQ(a->component_type, b->component_type);
  EXPECT_EQ(a->component_subtype, b->component_subtype);
  EXPECT_EQ(a->component_manufacturer, b->component_manufacturer);
  EXPECT_EQ(a->component_flags, b->component_flags);
  EXPECT_EQ(a->component_flags_mask, b->component_flags_mask);
  EXPECT_STREQ(a->component_name, b->component_name);
}

TEST(MuTFF, ReadHandlerReferenceAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFHandlerReferenceAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(hdlr_test_data, hdlr_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_handler_reference_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, hdlr_test_data_size);

  expect_hdlr_eq(&atom, &hdlr_test_struct);
  EXPECT_EQ(ftell(fd), hdlr_test_data_size);
}
// }}}1

// {{{1 data reference unit tests
static const uint32_t data_ref_test_data_size = 16;
// clang-format off
#define DATA_REF_TEST_DATA                        \
    data_ref_test_data_size >> 24,  /* size */    \
    data_ref_test_data_size >> 16,                \
    data_ref_test_data_size >> 8,                 \
    data_ref_test_data_size,                      \
    'a', 'b', 'c', 'd',             /* type */    \
    0x00,                           /* version */ \
    0x00, 0x01, 0x02,               /* flags */   \
    0x00, 0x01, 0x02, 0x03          /* data */
// clang-format on
static const unsigned char data_ref_test_data[data_ref_test_data_size] =
    ARR(DATA_REF_TEST_DATA);
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
  size_t bytes;
  const MuTFFError err =
      mutff_write_data_reference(fd, &bytes, &data_ref_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, data_ref_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, data_ref_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], data_ref_test_data[i]);
  }
}

static inline void expect_data_ref_eq(const MuTFFDataReference *a,
                                      const MuTFFDataReference *b) {
  EXPECT_EQ(a->type, b->type);
  EXPECT_EQ(a->version, b->version);
  EXPECT_EQ(a->flags, b->flags);
  EXPECT_EQ(a->data_size, b->data_size);
  const size_t data_size =
      a->data_size > b->data_size ? b->data_size : a->data_size;
  for (size_t i = 0; i < data_size; ++i) {
    EXPECT_EQ(a->data[i], b->data[i]);
  }
}

TEST(MuTFF, ReadDataReference) {
  MuTFFError err;
  size_t bytes;
  MuTFFDataReference ref;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(data_ref_test_data, data_ref_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_data_reference(fd, &bytes, &ref);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, data_ref_test_data_size);

  expect_data_ref_eq(&ref, &data_ref_test_struct);
  EXPECT_EQ(ftell(fd), data_ref_test_data_size);
}
// }}}1

// @TODO: test multiple entries
// {{{1 data reference atom unit tests
static const uint32_t dref_test_data_size = 16 + data_ref_test_data_size;
// clang-format off
#define DREF_TEST_DATA                                  \
    dref_test_data_size >> 24,  /* size */              \
    dref_test_data_size >> 16,                          \
    dref_test_data_size >> 8,                           \
    dref_test_data_size,                                \
    'd', 'r', 'e', 'f',         /* type */              \
    0x00,                       /* version */           \
    0x00, 0x01, 0x02,           /* flag */              \
    0x00, 0x00, 0x00, 0x01,     /* number of entries */ \
    DATA_REF_TEST_DATA
// clang-format on
static const unsigned char dref_test_data[dref_test_data_size] =
    ARR(DREF_TEST_DATA);
// clang-format off
static const MuTFFDataReferenceAtom dref_test_struct = {
    0x00,
    0x000102,
    1,
    {
      data_ref_test_struct,
    }
};
// clang-format on

TEST(MuTFF, WriteDataReferenceAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err =
      mutff_write_data_reference_atom(fd, &bytes, &dref_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, dref_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, dref_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], dref_test_data[i]);
  }
}

static inline void expect_dref_eq(const MuTFFDataReferenceAtom *a,
                                  const MuTFFDataReferenceAtom *b) {
  EXPECT_EQ(a->version, b->version);
  EXPECT_EQ(a->flags, b->flags);
  EXPECT_EQ(a->number_of_entries, b->number_of_entries);
  const size_t number_of_entries = a->number_of_entries > b->number_of_entries
                                       ? b->number_of_entries
                                       : a->number_of_entries;
  for (size_t i = 0; i < number_of_entries; ++i) {
    expect_data_ref_eq(&a->data_references[i], &b->data_references[i]);
  }
}

TEST(MuTFF, ReadDataReferenceAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFDataReferenceAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(dref_test_data, dref_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_data_reference_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, dref_test_data_size);

  expect_dref_eq(&atom, &dref_test_struct);
  EXPECT_EQ(ftell(fd), dref_test_data_size);
}
// }}}1

// {{{1 data information atom unit tests
static const uint32_t dinf_test_data_size = 8 + dref_test_data_size;
// clang-format off
#define DINF_TEST_DATA                     \
    dinf_test_data_size >> 24,  /* size */ \
    dinf_test_data_size >> 16,             \
    dinf_test_data_size >> 8,              \
    dinf_test_data_size,                   \
    'd', 'i', 'n', 'f',         /* type */ \
    DREF_TEST_DATA
// clang-format on
static const unsigned char dinf_test_data[dinf_test_data_size] =
    ARR(DINF_TEST_DATA);
// clang-format off
static const MuTFFDataInformationAtom dinf_test_struct = {
    dref_test_struct,        // data reference
};
// clang-format on

TEST(MuTFF, WriteDataInformationAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err =
      mutff_write_data_information_atom(fd, &bytes, &dinf_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, dinf_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, dinf_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], dinf_test_data[i]);
  }
}

static inline void expect_dinf_eq(const MuTFFDataInformationAtom *a,
                                  const MuTFFDataInformationAtom *b) {
  expect_dref_eq(&a->data_reference, &b->data_reference);
}

TEST(MuTFF, ReadDataInformationAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFDataInformationAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(dinf_test_data, dinf_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_data_information_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, dinf_test_data_size);

  expect_dinf_eq(&atom, &dinf_test_struct);
  EXPECT_EQ(ftell(fd), dinf_test_data_size);
}
// }}}1

// @TODO: test multiple entries
// {{{1 sample description atom unit tests
static const uint32_t stsd_test_data_size = 16 + sample_desc_test_data_size;
// clang-format off
#define STSD_TEST_DATA                                           \
    stsd_test_data_size >> 24,           /* size */              \
    stsd_test_data_size >> 16,                                   \
    stsd_test_data_size >> 8,                                    \
    stsd_test_data_size,                                         \
    's', 't', 's', 'd',                  /* type */              \
    0x00,                                /* version */           \
    0x00, 0x01, 0x02,                    /* flags */             \
    0x00, 0x00, 0x00, 0x01,              /* number of entries */ \
    SAMPLE_DESC_TEST_DATA
// clang-format on
static const unsigned char stsd_test_data[stsd_test_data_size] =
    ARR(STSD_TEST_DATA);
// clang-format off
static const MuTFFSampleDescriptionAtom stsd_test_struct = {
    0x00,
    0x000102,
    1,
    {
      sample_desc_test_struct,
    }
};
// clang-format on

TEST(MuTFF, WriteSampleDescriptionAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err =
      mutff_write_sample_description_atom(fd, &bytes, &stsd_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, stsd_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, stsd_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], stsd_test_data[i]);
  }
}

static inline void expect_stsd_eq(const MuTFFSampleDescriptionAtom *a,
                                  const MuTFFSampleDescriptionAtom *b) {
  EXPECT_EQ(a->version, b->version);
  EXPECT_EQ(a->flags, b->flags);
  EXPECT_EQ(a->number_of_entries, b->number_of_entries);
  const uint32_t number_of_entries = a->number_of_entries > b->number_of_entries
                                         ? b->number_of_entries
                                         : a->number_of_entries;
  for (uint32_t i = 0; i < number_of_entries; ++i) {
    expect_sample_desc_eq(&a->sample_description_table[i],
                          &b->sample_description_table[i]);
  }
}

TEST(MuTFF, ReadSampleDescriptionAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFSampleDescriptionAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(stsd_test_data, stsd_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_sample_description_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, stsd_test_data_size);

  expect_stsd_eq(&atom, &stsd_test_struct);
  EXPECT_EQ(ftell(fd), stsd_test_data_size);
}
// }}}1

// {{{1 time to sample table entry unit tests
static const uint32_t stts_entry_test_data_size = 8;
// clang-format off
#define STTS_ENTRY_TEST_DATA                       \
    0x00, 0x01, 0x02, 0x03,  /* sample count */    \
    0x10, 0x11, 0x12, 0x13   /* sample duration */
// clang-format on
static const unsigned char stts_entry_test_data[stts_entry_test_data_size] =
    ARR(STTS_ENTRY_TEST_DATA);
// clang-format off
static const MuTFFTimeToSampleTableEntry stts_entry_test_struct = {
    0x00010203,
    0x10111213,
};
// clang-format on

TEST(MuTFF, WriteTimeToSampleTableEntry) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err = mutff_write_time_to_sample_table_entry(
      fd, &bytes, &stts_entry_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, stts_entry_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, stts_entry_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], stts_entry_test_data[i]);
  }
}

static inline void expect_stts_entry_eq(const MuTFFTimeToSampleTableEntry *a,
                                        const MuTFFTimeToSampleTableEntry *b) {
  EXPECT_EQ(a->sample_count, b->sample_count);
  EXPECT_EQ(a->sample_duration, b->sample_duration);
}

TEST(MuTFF, ReadTimeToSampleTableEntry) {
  MuTFFError err;
  size_t bytes;
  MuTFFTimeToSampleTableEntry entry;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(stts_entry_test_data, stts_entry_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_time_to_sample_table_entry(fd, &bytes, &entry);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, stts_entry_test_data_size);

  expect_stts_entry_eq(&entry, &stts_entry_test_struct);
  EXPECT_EQ(ftell(fd), stts_entry_test_data_size);
}
// }}}1

// @TODO: test multiple entries
// {{{1 time-to-sample atom unit tests
static const uint32_t stts_test_data_size = 16 + stts_entry_test_data_size;
// clang-format off
#define STTS_TEST_DATA                                  \
    stts_test_data_size >> 24,  /* size */              \
    stts_test_data_size >> 16,                          \
    stts_test_data_size >> 8,                           \
    stts_test_data_size,                                \
    's', 't', 't', 's',         /* type */              \
    0x00,                       /* version */           \
    0x00, 0x01, 0x02,           /* flags */             \
    0x00, 0x00, 0x00, 0x01,     /* number of entries */ \
    STTS_ENTRY_TEST_DATA
// clang-format on
static const unsigned char stts_test_data[stts_test_data_size] =
    ARR(STTS_TEST_DATA);
// clang-format off
static const MuTFFTimeToSampleAtom stts_test_struct = {
    0x00,                    // version
    0x000102,                // flags
    1,                       // number of entries
    {
      stts_entry_test_struct
    }
};
// clang-format on

TEST(MuTFF, WriteTimeToSampleAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err =
      mutff_write_time_to_sample_atom(fd, &bytes, &stts_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, stts_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, stts_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], stts_test_data[i]);
  }
}

static inline void expect_stts_eq(const MuTFFTimeToSampleAtom *a,
                                  const MuTFFTimeToSampleAtom *b) {
  EXPECT_EQ(a->version, b->version);
  EXPECT_EQ(a->flags, b->flags);
  EXPECT_EQ(a->number_of_entries, b->number_of_entries);
  const uint32_t number_of_entries = a->number_of_entries > b->number_of_entries
                                         ? b->number_of_entries
                                         : a->number_of_entries;
  for (uint32_t i = 0; i < number_of_entries; ++i) {
    expect_stts_entry_eq(&a->time_to_sample_table[i],
                         &b->time_to_sample_table[i]);
  }
}

TEST(MuTFF, ReadTimeToSampleAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFTimeToSampleAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(stts_test_data, stts_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_time_to_sample_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, stts_test_data_size);

  expect_stts_eq(&atom, &stts_test_struct);
  EXPECT_EQ(ftell(fd), stts_test_data_size);
}
// }}}1

// {{{1 composition offset table entry unit tests
static const uint32_t ctts_entry_test_data_size = 8;
// clang-format off
#define CTTS_ENTRY_TEST_DATA                          \
    0x00, 0x01, 0x02, 0x03,  /* sample count */       \
    0x10, 0x11, 0x12, 0x13   /* composition offset */
// clang-format on
static const unsigned char ctts_entry_test_data[ctts_entry_test_data_size] =
    ARR(CTTS_ENTRY_TEST_DATA);
// clang-format off
static const MuTFFCompositionOffsetTableEntry ctts_entry_test_struct = {
    0x00010203,
    0x10111213,
};
// clang-format on

TEST(MuTFF, WriteCompositionOffsetTableEntry) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err = mutff_write_composition_offset_table_entry(
      fd, &bytes, &ctts_entry_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, ctts_entry_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, ctts_entry_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], ctts_entry_test_data[i]);
  }
}

static inline void expect_ctts_entry_eq(
    const MuTFFCompositionOffsetTableEntry *a,
    const MuTFFCompositionOffsetTableEntry *b) {
  EXPECT_EQ(a->sample_count, b->sample_count);
  EXPECT_EQ(a->composition_offset, b->composition_offset);
}

TEST(MuTFF, ReadCompositionOffsetTableEntry) {
  MuTFFError err;
  size_t bytes;
  MuTFFCompositionOffsetTableEntry entry;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(ctts_entry_test_data, ctts_entry_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_composition_offset_table_entry(fd, &bytes, &entry);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, ctts_entry_test_data_size);

  expect_ctts_entry_eq(&entry, &ctts_entry_test_struct);
  EXPECT_EQ(ftell(fd), ctts_entry_test_data_size);
}
// }}}1

// @TODO: test multiple entries
// {{{1 composition offset atom unit tests
static const uint32_t ctts_test_data_size = 16 + ctts_entry_test_data_size;
// clang-format off
#define CTTS_TEST_DATA                                  \
    ctts_test_data_size >> 24,  /* size */              \
    ctts_test_data_size >> 16,                          \
    ctts_test_data_size >> 8,                           \
    ctts_test_data_size,                                \
    'c', 't', 't', 's',         /* type */              \
    0x00,                       /* version */           \
    0x00, 0x01, 0x02,           /* flags */             \
    0x00, 0x00, 0x00, 0x01,     /* number of entries */ \
    CTTS_ENTRY_TEST_DATA
// clang-format on
static const unsigned char ctts_test_data[ctts_test_data_size] =
    ARR(CTTS_TEST_DATA);
// clang-format off
static const MuTFFCompositionOffsetAtom ctts_test_struct = {
    0x00,                    // version
    0x000102,                // flags
    1,                       // number of entries
    {
      ctts_entry_test_struct,
    }
};
// clang-format on

TEST(MuTFF, WriteCompositionOffsetAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err =
      mutff_write_composition_offset_atom(fd, &bytes, &ctts_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, ctts_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, ctts_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], ctts_test_data[i]);
  }
}

static inline void expect_ctts_eq(const MuTFFCompositionOffsetAtom *a,
                                  const MuTFFCompositionOffsetAtom *b) {
  EXPECT_EQ(a->version, b->version);
  EXPECT_EQ(a->flags, b->flags);
  EXPECT_EQ(a->entry_count, b->entry_count);
  const uint32_t entry_count =
      a->entry_count > b->entry_count ? b->entry_count : a->entry_count;
  for (uint32_t i = 0; i < entry_count; ++i) {
    expect_ctts_entry_eq(&a->composition_offset_table[i],
                         &b->composition_offset_table[i]);
  }
}

TEST(MuTFF, ReadCompositionOffsetAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFCompositionOffsetAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(ctts_test_data, ctts_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_composition_offset_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, ctts_test_data_size);

  expect_ctts_eq(&atom, &ctts_test_struct);
  EXPECT_EQ(ftell(fd), ctts_test_data_size);
}
// }}}1

// {{{1 composition shift least greatest atom unit tests
static const uint32_t cslg_test_data_size = 32;
// clang-format off
#define CSLG_TEST_DATA                                                           \
    cslg_test_data_size >> 24,  /* size */                                       \
    cslg_test_data_size >> 16,                                                   \
    cslg_test_data_size >> 8,                                                    \
    cslg_test_data_size,                                                         \
    'c', 's', 'l', 'g',         /* type */                                       \
    0x00,                       /* version */                                    \
    0x00, 0x01, 0x02,           /* flags */                                      \
    0x00, 0x01, 0x02, 0x03,     /* composition offset to display offset shift */ \
    0x10, 0x11, 0x12, 0x13,     /* least display offset */                       \
    0x20, 0x21, 0x22, 0x23,     /* greatest display offset */                    \
    0x30, 0x31, 0x32, 0x33,     /* start display time */                         \
    0x40, 0x41, 0x42, 0x43      /* end display time */
// clang-format on
static const unsigned char cslg_test_data[cslg_test_data_size] =
    ARR(CSLG_TEST_DATA);
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
  size_t bytes;
  const MuTFFError err = mutff_write_composition_shift_least_greatest_atom(
      fd, &bytes, &cslg_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, cslg_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, cslg_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], cslg_test_data[i]);
  }
}

static inline void expect_cslg_eq(
    const MuTFFCompositionShiftLeastGreatestAtom *a,
    const MuTFFCompositionShiftLeastGreatestAtom *b) {
  EXPECT_EQ(a->version, b->version);
  EXPECT_EQ(a->flags, b->flags);
  EXPECT_EQ(a->composition_offset_to_display_offset_shift,
            b->composition_offset_to_display_offset_shift);
  EXPECT_EQ(a->least_display_offset, b->least_display_offset);
  EXPECT_EQ(a->greatest_display_offset, b->greatest_display_offset);
  EXPECT_EQ(a->display_start_time, b->display_start_time);
  EXPECT_EQ(a->display_end_time, b->display_end_time);
}

TEST(MuTFF, ReadCompositionShiftLeastGreatestAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFCompositionShiftLeastGreatestAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(cslg_test_data, cslg_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_composition_shift_least_greatest_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, cslg_test_data_size);

  expect_cslg_eq(&atom, &cslg_test_struct);
  EXPECT_EQ(ftell(fd), cslg_test_data_size);
}
// }}}1

// {{{1 sync sample atom unit tests
static const uint32_t stss_test_data_size = 24;
// clang-format off
#define STSS_TEST_DATA                                  \
    stss_test_data_size >> 24,  /* size */              \
    stss_test_data_size >> 16,                          \
    stss_test_data_size >> 8,                           \
    stss_test_data_size,                                \
    's', 't', 's', 's',         /* type */              \
    0x00,                       /* version */           \
    0x00, 0x01, 0x02,           /* flags */             \
    0x00, 0x00, 0x00, 0x02,     /* number of entries */ \
    0x00, 0x01, 0x02, 0x03,     /* table[0] */          \
    0x10, 0x11, 0x12, 0x13      /* table[1] */
// clang-format on
static const unsigned char stss_test_data[stss_test_data_size] =
    ARR(STSS_TEST_DATA);
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
  size_t bytes;
  const MuTFFError err =
      mutff_write_sync_sample_atom(fd, &bytes, &stss_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, stss_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, stss_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], stss_test_data[i]);
  }
}

static inline void expect_stss_eq(const MuTFFSyncSampleAtom *a,
                                  const MuTFFSyncSampleAtom *b) {
  EXPECT_EQ(a->version, b->version);
  EXPECT_EQ(a->flags, b->flags);
  EXPECT_EQ(a->number_of_entries, b->number_of_entries);
  const uint32_t number_of_entries = a->number_of_entries > b->number_of_entries
                                         ? b->number_of_entries
                                         : a->number_of_entries;
  for (uint32_t i = 0; i < number_of_entries; ++i) {
    EXPECT_EQ(a->sync_sample_table[i], b->sync_sample_table[i]);
  }
}

TEST(MuTFF, ReadSyncSampleAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFSyncSampleAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(stss_test_data, stss_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_sync_sample_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, stss_test_data_size);

  expect_stss_eq(&atom, &stss_test_struct);
  EXPECT_EQ(ftell(fd), stss_test_data_size);
}
// }}}1

// {{{1 partial sync sample atom unit tests
static const uint32_t stps_test_data_size = 24;
// clang-format off
#define STPS_TEST_DATA                                  \
    stps_test_data_size >> 24,  /* size */              \
    stps_test_data_size >> 16,                          \
    stps_test_data_size >> 8,                           \
    stps_test_data_size,                                \
    's', 't', 'p', 's',         /* type */              \
    0x00,                       /* version */           \
    0x00, 0x01, 0x02,           /* flags */             \
    0x00, 0x00, 0x00, 0x02,     /* number of entries */ \
    0x00, 0x01, 0x02, 0x03,     /* table[0] */          \
    0x10, 0x11, 0x12, 0x13      /* table[1] */
// clang-format on
static const unsigned char stps_test_data[stps_test_data_size] =
    ARR(STPS_TEST_DATA);
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
  size_t bytes;
  const MuTFFError err =
      mutff_write_partial_sync_sample_atom(fd, &bytes, &stps_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, stps_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, stps_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], stps_test_data[i]);
  }
}

static inline void expect_stps_eq(const MuTFFPartialSyncSampleAtom *a,
                                  const MuTFFPartialSyncSampleAtom *b) {
  EXPECT_EQ(a->version, b->version);
  EXPECT_EQ(a->flags, b->flags);
  EXPECT_EQ(a->entry_count, b->entry_count);
  const uint32_t entry_count =
      a->entry_count > b->entry_count ? b->entry_count : a->entry_count;
  for (uint32_t i = 0; i < entry_count; ++i) {
    EXPECT_EQ(a->partial_sync_sample_table[i], b->partial_sync_sample_table[i]);
  }
}

TEST(MuTFF, ReadPartialSyncSampleAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFPartialSyncSampleAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(stps_test_data, stps_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_partial_sync_sample_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, stps_test_data_size);

  expect_stps_eq(&atom, &stps_test_struct);
  EXPECT_EQ(ftell(fd), stps_test_data_size);
}
// }}}1

// {{{1 sample-to-chunk table entry unit tests
static const uint32_t stsc_entry_test_data_size = 12;
// clang-format off
#define STSC_ENTRY_TEST_DATA \
    0x00, 0x01, 0x02, 0x03,  /* first chunk */           \
    0x10, 0x11, 0x12, 0x13,  /* samples per chunk */     \
    0x20, 0x21, 0x22, 0x23   /* sample description ID */
// clang-format on
static const unsigned char stsc_entry_test_data[stsc_entry_test_data_size] =
    ARR(STSC_ENTRY_TEST_DATA);
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
  size_t bytes;
  const MuTFFError err = mutff_write_sample_to_chunk_table_entry(
      fd, &bytes, &stsc_entry_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, stsc_entry_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, stsc_entry_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], stsc_entry_test_data[i]);
  }
}

static inline void expect_stsc_entry_eq(const MuTFFSampleToChunkTableEntry *a,
                                        const MuTFFSampleToChunkTableEntry *b) {
  EXPECT_EQ(a->first_chunk, b->first_chunk);
  EXPECT_EQ(a->samples_per_chunk, b->samples_per_chunk);
  EXPECT_EQ(a->sample_description_id, b->sample_description_id);
}

TEST(MuTFF, ReadSampleToChunkTableEntry) {
  MuTFFError err;
  size_t bytes;
  MuTFFSampleToChunkTableEntry entry;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(stsc_entry_test_data, stsc_entry_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_sample_to_chunk_table_entry(fd, &bytes, &entry);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, stsc_entry_test_data_size);

  expect_stsc_entry_eq(&entry, &stsc_entry_test_struct);
  EXPECT_EQ(ftell(fd), stsc_entry_test_data_size);
}
// }}}1

// @TODO: test multiple entries
// {{{1 sample-to-chunk atom unit tests
static const uint32_t stsc_test_data_size = 16 + stsc_entry_test_data_size;
// clang-format off
#define STSC_TEST_DATA                                  \
    stsc_test_data_size >> 24,  /* size */              \
    stsc_test_data_size >> 16,                          \
    stsc_test_data_size >> 8,                           \
    stsc_test_data_size,                                \
    's', 't', 's', 'c',         /* type */              \
    0x00,                       /* version */           \
    0x00, 0x01, 0x02,           /* flags */             \
    0x00, 0x00, 0x00, 0x01,     /* number of entries */ \
    STSC_ENTRY_TEST_DATA
// clang-format on
static const unsigned char stsc_test_data[stsc_test_data_size] =
    ARR(STSC_TEST_DATA);
// clang-format off
static const MuTFFSampleToChunkAtom stsc_test_struct = {
    0x00,
    0x000102,
    1,
    {
      stsc_entry_test_struct,
    }
};
// clang-format on

TEST(MuTFF, WriteSampleToChunkAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err =
      mutff_write_sample_to_chunk_atom(fd, &bytes, &stsc_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, stsc_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, stsc_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], stsc_test_data[i]);
  }
}

static inline void expect_stsc_eq(const MuTFFSampleToChunkAtom *a,
                                  const MuTFFSampleToChunkAtom *b) {
  EXPECT_EQ(a->version, b->version);
  EXPECT_EQ(a->flags, b->flags);
  EXPECT_EQ(a->number_of_entries, b->number_of_entries);
  const uint32_t number_of_entries = a->number_of_entries > b->number_of_entries
                                         ? b->number_of_entries
                                         : a->number_of_entries;
  for (uint32_t i = 0; i < number_of_entries; ++i) {
    expect_stsc_entry_eq(&a->sample_to_chunk_table[i],
                         &b->sample_to_chunk_table[i]);
  }
}

TEST(MuTFF, ReadSampleToChunkAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFSampleToChunkAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(stsc_test_data, stsc_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_sample_to_chunk_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, stsc_test_data_size);

  expect_stsc_eq(&atom, &stsc_test_struct);
  EXPECT_EQ(ftell(fd), stsc_test_data_size);
}
// }}}1

// {{{1 sample size atom unit tests
static const uint32_t stsz_test_data_size = 24;
// clang-format off
#define STSZ_TEST_DATA                                  \
    stsz_test_data_size >> 24,  /* size */              \
    stsz_test_data_size >> 16,                          \
    stsz_test_data_size >> 8,                           \
    stsz_test_data_size,                                \
    's', 't', 's', 'z',         /* type */              \
    0x00,                       /* version */           \
    0x00, 0x01, 0x02,           /* flags */             \
    0x00, 0x00, 0x00, 0x00,     /* sample size */       \
    0x00, 0x00, 0x00, 0x01,     /* number of entries */ \
    0x10, 0x11, 0x12, 0x13      /* sample size table */  // clang-format on
static const unsigned char stsz_test_data[stsz_test_data_size] =
    ARR(STSZ_TEST_DATA);
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
  size_t bytes;
  const MuTFFError err =
      mutff_write_sample_size_atom(fd, &bytes, &stsz_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, stsz_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, stsz_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], stsz_test_data[i]);
  }
}

static inline void expect_stsz_eq(const MuTFFSampleSizeAtom *a,
                                  const MuTFFSampleSizeAtom *b) {
  EXPECT_EQ(a->version, b->version);
  EXPECT_EQ(a->flags, b->flags);
  EXPECT_EQ(a->sample_size, b->sample_size);
  EXPECT_EQ(a->number_of_entries, b->number_of_entries);
  const uint32_t number_of_entries = a->number_of_entries > b->number_of_entries
                                         ? b->number_of_entries
                                         : a->number_of_entries;
  for (uint32_t i = 0; i < number_of_entries; ++i) {
    EXPECT_EQ(a->sample_size_table[i], b->sample_size_table[i]);
  }
}

TEST(MuTFF, ReadSampleSizeAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFSampleSizeAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(stsz_test_data, stsz_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_sample_size_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, stsz_test_data_size);

  expect_stsz_eq(&atom, &stsz_test_struct);
  EXPECT_EQ(ftell(fd), stsz_test_data_size);
}
// }}}1

// {{{1 chunk offset atom unit tests
static const uint32_t stco_test_data_size = 20;
// clang-format off
#define STCO_TEST_DATA                                  \
    stco_test_data_size >> 24,  /* size */              \
    stco_test_data_size >> 16,                          \
    stco_test_data_size >> 8,                           \
    stco_test_data_size,                                \
    's', 't', 'c', 'o',         /* type */              \
    0x00,                       /* version */           \
    0x00, 0x01, 0x02,           /* flags */             \
    0x00, 0x00, 0x00, 0x01,     /* number of entries */ \
    0x10, 0x11, 0x12, 0x13      /* sample size table */  // clang-format on
static const unsigned char stco_test_data[stco_test_data_size] =
    ARR(STCO_TEST_DATA);
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
  size_t bytes;
  const MuTFFError err =
      mutff_write_chunk_offset_atom(fd, &bytes, &stco_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, stco_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, stco_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], stco_test_data[i]);
  }
}

static inline void expect_stco_eq(const MuTFFChunkOffsetAtom *a,
                                  const MuTFFChunkOffsetAtom *b) {
  EXPECT_EQ(a->version, b->version);
  EXPECT_EQ(a->flags, b->flags);
  EXPECT_EQ(a->number_of_entries, b->number_of_entries);
  const uint32_t number_of_entries = a->number_of_entries > b->number_of_entries
                                         ? b->number_of_entries
                                         : a->number_of_entries;
  for (uint32_t i = 0; i < number_of_entries; ++i) {
    EXPECT_EQ(a->chunk_offset_table[i], b->chunk_offset_table[i]);
  }
}

TEST(MuTFF, ReadChunkOffsetAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFChunkOffsetAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(stco_test_data, stco_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_chunk_offset_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, stco_test_data_size);

  expect_stco_eq(&atom, &stco_test_struct);
  EXPECT_EQ(ftell(fd), stco_test_data_size);
}
// }}}1

// {{{1 sample dependency flags atom unit tests
static const uint32_t sdtp_test_data_size = 14;
// clang-format off
#define SDTP_TEST_DATA                                  \
    sdtp_test_data_size >> 24,  /* size */              \
    sdtp_test_data_size >> 16,                          \
    sdtp_test_data_size >> 8,                           \
    sdtp_test_data_size,                                \
    's', 'd', 't', 'p',         /* type */              \
    0x00,                       /* version */           \
    0x00, 0x01, 0x02,           /* flags */             \
    0x10, 0x11                  /* sample size table */
// clang-format on
static const unsigned char sdtp_test_data[sdtp_test_data_size] =
    ARR(SDTP_TEST_DATA);
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
  size_t bytes;
  const MuTFFError err =
      mutff_write_sample_dependency_flags_atom(fd, &bytes, &sdtp_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, sdtp_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, sdtp_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], sdtp_test_data[i]);
  }
}

static inline void expect_sdtp_eq(const MuTFFSampleDependencyFlagsAtom *a,
                                  const MuTFFSampleDependencyFlagsAtom *b) {
  EXPECT_EQ(a->version, b->version);
  EXPECT_EQ(a->flags, b->flags);
  EXPECT_EQ(a->data_size, b->data_size);
  const uint32_t data_size =
      a->data_size > b->data_size ? b->data_size : a->data_size;
  for (uint32_t i = 0; i < data_size; ++i) {
    EXPECT_EQ(a->sample_dependency_flags_table[i],
              b->sample_dependency_flags_table[i]);
  }
}

TEST(MuTFF, ReadSampleDependencyFlagsAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFSampleDependencyFlagsAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(sdtp_test_data, sdtp_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_sample_dependency_flags_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, sdtp_test_data_size);

  expect_sdtp_eq(&atom, &sdtp_test_struct);
  EXPECT_EQ(ftell(fd), sdtp_test_data_size);
}
// }}}1

// {{{1 sample table atom unit tests
static const uint32_t stbl_test_data_size =
    8 + stsd_test_data_size + stts_test_data_size + ctts_test_data_size +
    cslg_test_data_size + stss_test_data_size + stps_test_data_size +
    stsc_test_data_size + stsz_test_data_size + stco_test_data_size +
    sdtp_test_data_size;
// clang-format off
#define STBL_TEST_DATA                            \
    stbl_test_data_size >> 24 & 0xFF,  /* size */ \
    stbl_test_data_size >> 16 & 0xFF,             \
    stbl_test_data_size >> 8 & 0xFF,              \
    stbl_test_data_size & 0xFF,                   \
    's', 't', 'b', 'l',                /* type */ \
    STSD_TEST_DATA,                               \
    STTS_TEST_DATA,                               \
    CTTS_TEST_DATA,                               \
    CSLG_TEST_DATA,                               \
    STSS_TEST_DATA,                               \
    STPS_TEST_DATA,                               \
    STSC_TEST_DATA,                               \
    STSZ_TEST_DATA,                               \
    STCO_TEST_DATA,                               \
    SDTP_TEST_DATA
// clang-format on
static const unsigned char stbl_test_data[stbl_test_data_size] =
    ARR(STBL_TEST_DATA);
// clang-format off
static const MuTFFSampleTableAtom stbl_test_struct = {
  stsd_test_struct,
  stts_test_struct,
  true,
  ctts_test_struct,
  true,
  cslg_test_struct,
  true,
  stss_test_struct,
  true,
  stps_test_struct,
  true,
  stsc_test_struct,
  true,
  stsz_test_struct,
  true,
  stco_test_struct,
  true,
  sdtp_test_struct,
};
// clang-format on

TEST(MuTFF, WriteSampleTableAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err =
      mutff_write_sample_table_atom(fd, &bytes, &stbl_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, stbl_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, stbl_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], stbl_test_data[i]);
  }
}

static inline void expect_stbl_eq(const MuTFFSampleTableAtom *a,
                                  const MuTFFSampleTableAtom *b) {
  expect_stsd_eq(&a->sample_description, &b->sample_description);
  expect_stts_eq(&a->time_to_sample, &b->time_to_sample);
  EXPECT_EQ(a->composition_offset_present, b->composition_offset_present);
  const bool composition_offset_present =
      a->composition_offset_present && b->composition_offset_present;
  if (composition_offset_present) {
    expect_ctts_eq(&a->composition_offset, &b->composition_offset);
  }
  EXPECT_EQ(a->composition_shift_least_greatest_present,
            b->composition_shift_least_greatest_present);
  const bool composition_shift_least_greatest_present =
      a->composition_shift_least_greatest_present &&
      b->composition_shift_least_greatest_present;
  if (composition_shift_least_greatest_present) {
    expect_cslg_eq(&a->composition_shift_least_greatest,
                   &b->composition_shift_least_greatest);
  }
  EXPECT_EQ(a->sync_sample_present, b->sync_sample_present);
  const bool sync_sample_present =
      a->sync_sample_present && b->sync_sample_present;
  if (sync_sample_present) {
    expect_stss_eq(&a->sync_sample, &b->sync_sample);
  }
  EXPECT_EQ(a->partial_sync_sample_present, b->partial_sync_sample_present);
  const bool partial_sync_sample_present =
      a->partial_sync_sample_present && b->partial_sync_sample_present;
  if (partial_sync_sample_present) {
    expect_stps_eq(&a->partial_sync_sample, &b->partial_sync_sample);
  }
  EXPECT_EQ(a->sample_to_chunk_present, b->sample_to_chunk_present);
  const bool sample_to_chunk_present =
      a->sample_to_chunk_present && b->sample_to_chunk_present;
  if (sample_to_chunk_present) {
    expect_stsc_eq(&a->sample_to_chunk, &b->sample_to_chunk);
  }
  EXPECT_EQ(a->sample_size_present, b->sample_size_present);
  const bool sample_size_present =
      a->sample_size_present && b->sample_size_present;
  if (sample_size_present) {
    expect_stsz_eq(&a->sample_size, &b->sample_size);
  }
  EXPECT_EQ(a->chunk_offset_present, b->chunk_offset_present);
  const bool chunk_offset_present =
      a->chunk_offset_present && b->chunk_offset_present;
  if (chunk_offset_present) {
    expect_stco_eq(&a->chunk_offset, &b->chunk_offset);
  }
  EXPECT_EQ(a->sample_dependency_flags_present,
            b->sample_dependency_flags_present);
  const bool sample_dependency_flags_present =
      a->sample_dependency_flags_present && b->sample_dependency_flags_present;
  if (sample_dependency_flags_present) {
    expect_sdtp_eq(&a->sample_dependency_flags, &b->sample_dependency_flags);
  }
}

TEST(MuTFF, ReadSampleTableAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFSampleTableAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(stbl_test_data, stbl_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_sample_table_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, stbl_test_data_size);

  expect_stbl_eq(&atom, &stbl_test_struct);
  EXPECT_EQ(ftell(fd), stbl_test_data_size);
}
// }}}1

// {{{1 video media information header atom unit tests
static const uint32_t vmhd_test_data_size = 20;
// clang-format off
#define VMHD_TEST_DATA                              \
    vmhd_test_data_size >> 24,  /* size */          \
    vmhd_test_data_size >> 16,                      \
    vmhd_test_data_size >> 8,                       \
    vmhd_test_data_size,                            \
    'v', 'm', 'h', 'd',         /* type */          \
    0x00,                       /* version */       \
    0x00, 0x01, 0x02,           /* flags */         \
    0x00, 0x01,                 /* graphics mode */ \
    0x10, 0x11,                 /* opcolor[0] */    \
    0x20, 0x21,                 /* opcolor[1] */    \
    0x30, 0x31                  /* opcolor[2] */
// clang-format on
static const unsigned char vmhd_test_data[vmhd_test_data_size] =
    ARR(VMHD_TEST_DATA);
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
  size_t bytes;
  const MuTFFError err = mutff_write_video_media_information_header_atom(
      fd, &bytes, &vmhd_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, vmhd_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, vmhd_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], vmhd_test_data[i]);
  }
}

static inline void expect_vmhd_eq(
    const MuTFFVideoMediaInformationHeaderAtom *a,
    const MuTFFVideoMediaInformationHeaderAtom *b) {
  EXPECT_EQ(a->version, b->version);
  EXPECT_EQ(a->flags, b->flags);
  EXPECT_EQ(a->graphics_mode, b->graphics_mode);
  EXPECT_EQ(a->opcolor[0], b->opcolor[0]);
  EXPECT_EQ(a->opcolor[1], b->opcolor[1]);
  EXPECT_EQ(a->opcolor[2], b->opcolor[2]);
}

TEST(MuTFF, ReadVideoMediaInformationHeaderAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFVideoMediaInformationHeaderAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(vmhd_test_data, vmhd_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_video_media_information_header_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, vmhd_test_data_size);

  expect_vmhd_eq(&atom, &vmhd_test_struct);
  EXPECT_EQ(ftell(fd), vmhd_test_data_size);
}
// }}}1

// {{{1 video media information atom unit tests
static const uint32_t video_minf_test_data_size =
    8 + vmhd_test_data_size + hdlr_test_data_size + dinf_test_data_size +
    stbl_test_data_size;
// clang-format off
#define VIDEO_MINF_TEST_DATA                            \
    video_minf_test_data_size >> 24 & 0xFF,  /* size */ \
    video_minf_test_data_size >> 16 & 0xFF,             \
    video_minf_test_data_size >> 8 & 0xFF,              \
    video_minf_test_data_size & 0xFF,                   \
    'm', 'i', 'n', 'f',                      /* type */ \
    VMHD_TEST_DATA,                                     \
    HDLR_TEST_DATA,                                     \
    DINF_TEST_DATA,                                     \
    STBL_TEST_DATA
// clang-format on
static const unsigned char video_minf_test_data[video_minf_test_data_size] =
    ARR(VIDEO_MINF_TEST_DATA);
// clang-format off
static const MuTFFVideoMediaInformationAtom video_minf_test_struct = {
  vmhd_test_struct,
  hdlr_test_struct,
  true,
  dinf_test_struct,
  true,
  stbl_test_struct,
};
// clang-format on

TEST(MuTFF, WriteVideoMediaInformationAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err = mutff_write_video_media_information_atom(
      fd, &bytes, &video_minf_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, video_minf_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, video_minf_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], video_minf_test_data[i]);
  }
}

static inline void expect_video_minf_eq(
    const MuTFFVideoMediaInformationAtom *a,
    const MuTFFVideoMediaInformationAtom *b) {
  expect_vmhd_eq(&a->video_media_information_header,
                 &b->video_media_information_header);
  expect_hdlr_eq(&a->handler_reference, &b->handler_reference);
  EXPECT_EQ(a->data_information_present, b->data_information_present);
  const bool data_information_present =
      a->data_information_present && b->data_information_present;
  if (data_information_present) {
    expect_dinf_eq(&a->data_information, &b->data_information);
  }
  EXPECT_EQ(a->sample_table_present, b->sample_table_present);
  const bool sample_table_present =
      a->sample_table_present && b->sample_table_present;
  if (sample_table_present) {
    expect_stbl_eq(&a->sample_table, &b->sample_table);
  }
}

TEST(MuTFF, ReadVideoMediaInformationAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFVideoMediaInformationAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(video_minf_test_data, video_minf_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_video_media_information_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, video_minf_test_data_size);

  expect_video_minf_eq(&atom, &video_minf_test_struct);
  EXPECT_EQ(ftell(fd), video_minf_test_data_size);
}
// }}}1

// {{{1 sound media information header atom unit tests
static const uint32_t smhd_test_data_size = 16;
// clang-format off
#define SMHD_TEST_DATA                         \
    smhd_test_data_size >> 24,  /* size */     \
    smhd_test_data_size >> 16,                 \
    smhd_test_data_size >> 8,                  \
    smhd_test_data_size,                       \
    's', 'm', 'h', 'd',         /* type */     \
    0x00,                       /* version */  \
    0x00, 0x01, 0x02,           /* flags */    \
    0xff, 0xfe,                 /* balance */  \
    0x00, 0x00                  /* reserved */
// clang-format on
static const unsigned char smhd_test_data[smhd_test_data_size] =
    ARR(SMHD_TEST_DATA);
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
  size_t bytes;
  const MuTFFError err = mutff_write_sound_media_information_header_atom(
      fd, &bytes, &smhd_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, smhd_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, smhd_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], smhd_test_data[i]);
  }
}

static inline void expect_smhd_eq(
    const MuTFFSoundMediaInformationHeaderAtom *a,
    const MuTFFSoundMediaInformationHeaderAtom *b) {
  EXPECT_EQ(a->version, b->version);
  EXPECT_EQ(a->flags, b->flags);
  EXPECT_EQ(a->balance, b->balance);
}

TEST(MuTFF, ReadSoundMediaInformationHeaderAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFSoundMediaInformationHeaderAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(smhd_test_data, smhd_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_sound_media_information_header_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, smhd_test_data_size);

  expect_smhd_eq(&atom, &smhd_test_struct);
  EXPECT_EQ(ftell(fd), smhd_test_data_size);
}
// }}}1

// {{{1 sound media information atom unit tests
static const uint32_t sound_minf_test_data_size =
    8 + smhd_test_data_size + hdlr_test_data_size + dinf_test_data_size +
    stbl_test_data_size;
// clang-format off
#define SOUND_MINF_TEST_DATA                            \
    sound_minf_test_data_size >> 24 & 0xFF,  /* size */ \
    sound_minf_test_data_size >> 16 & 0xFF,             \
    sound_minf_test_data_size >> 8 & 0xFF,              \
    sound_minf_test_data_size & 0xFF,                   \
    'm', 'i', 'n', 'f',                      /* type */ \
    SMHD_TEST_DATA,                                     \
    HDLR_TEST_DATA,                                     \
    DINF_TEST_DATA,                                     \
    STBL_TEST_DATA
// clang-format on
static const unsigned char sound_minf_test_data[sound_minf_test_data_size] =
    ARR(SOUND_MINF_TEST_DATA);
// clang-format off
static const MuTFFSoundMediaInformationAtom sound_minf_test_struct = {
  smhd_test_struct,
  hdlr_test_struct,
  true,
  dinf_test_struct,
  true,
  stbl_test_struct,
};
// clang-format on

TEST(MuTFF, WriteSoundMediaInformationAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err = mutff_write_sound_media_information_atom(
      fd, &bytes, &sound_minf_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, sound_minf_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, sound_minf_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], sound_minf_test_data[i]);
  }
}

static inline void expect_sound_minf_eq(
    const MuTFFSoundMediaInformationAtom *a,
    const MuTFFSoundMediaInformationAtom *b) {
  expect_smhd_eq(&a->sound_media_information_header,
                 &b->sound_media_information_header);
  expect_hdlr_eq(&a->handler_reference, &b->handler_reference);
  EXPECT_EQ(a->data_information_present, b->data_information_present);
  const bool data_information_present =
      a->data_information_present && b->data_information_present;
  if (data_information_present) {
    expect_dinf_eq(&a->data_information, &b->data_information);
  }
  EXPECT_EQ(a->sample_table_present, b->sample_table_present);
  const bool sample_table_present =
      a->sample_table_present && b->sample_table_present;
  if (sample_table_present) {
    expect_stbl_eq(&a->sample_table, &b->sample_table);
  }
}

TEST(MuTFF, ReadSoundMediaInformationAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFSoundMediaInformationAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(sound_minf_test_data, sound_minf_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_sound_media_information_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, sound_minf_test_data_size);

  expect_sound_minf_eq(&atom, &sound_minf_test_struct);
  EXPECT_EQ(ftell(fd), sound_minf_test_data_size);
}
// }}}1

// {{{1 base media info atom unit tests
static const uint32_t gmin_test_data_size = 24;
// clang-format off
#define GMIN_TEST_DATA                              \
    gmin_test_data_size >> 24,  /* size */          \
    gmin_test_data_size >> 16,                      \
    gmin_test_data_size >> 8,                       \
    gmin_test_data_size,                            \
    'g', 'm', 'i', 'n',         /* type */          \
    0x00,                       /* version */       \
    0x00, 0x01, 0x02,           /* flags */         \
    0x00, 0x01,                 /* graphics mode */ \
    0x10, 0x11,                 /* opcolor[0] */    \
    0x20, 0x21,                 /* opcolor[1] */    \
    0x30, 0x31,                 /* opcolor[2] */    \
    0x40, 0x41,                 /* balance */       \
    0x00, 0x00                  /* reserved */
// clang-format on
static const unsigned char gmin_test_data[gmin_test_data_size] =
    ARR(GMIN_TEST_DATA);
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
  size_t bytes;
  const MuTFFError err =
      mutff_write_base_media_info_atom(fd, &bytes, &gmin_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, gmin_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, gmin_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], gmin_test_data[i]);
  }
}

static inline void expect_gmin_eq(const MuTFFBaseMediaInfoAtom *a,
                                  const MuTFFBaseMediaInfoAtom *b) {
  EXPECT_EQ(a->version, b->version);
  EXPECT_EQ(a->flags, b->flags);
  EXPECT_EQ(a->graphics_mode, b->graphics_mode);
  EXPECT_EQ(a->opcolor[0], b->opcolor[0]);
  EXPECT_EQ(a->opcolor[1], b->opcolor[1]);
  EXPECT_EQ(a->opcolor[2], b->opcolor[2]);
  EXPECT_EQ(a->balance, b->balance);
}

TEST(MuTFF, ReadBaseMediaInfoAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFBaseMediaInfoAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(gmin_test_data, gmin_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_base_media_info_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, gmin_test_data_size);

  expect_gmin_eq(&atom, &gmin_test_struct);
  EXPECT_EQ(ftell(fd), gmin_test_data_size);
}
// }}}1

// {{{1 text media information atom unit tests
static const uint32_t text_test_data_size = 44;
// clang-format off
#define TEXT_TEST_DATA                                 \
    text_test_data_size >> 24,  /* size */             \
    text_test_data_size >> 16,                         \
    text_test_data_size >> 8,                          \
    text_test_data_size,                               \
    't', 'e', 'x', 't',         /* type */             \
    0x00, 0x01, 0x00, 0x02,     /* matrix_structure */ \
    0x00, 0x03, 0x00, 0x04,                            \
    0x00, 0x00, 0x00, 0x00,                            \
    0x00, 0x07, 0x00, 0x08,                            \
    0x00, 0x09, 0x00, 0x0a,                            \
    0x00, 0x00, 0x00, 0x00,                            \
    0x00, 0x0d, 0x00, 0x0e,                            \
    0x00, 0x0f, 0x00, 0x10,                            \
    0x00, 0x00, 0x00, 0x00,  // clang-format on
static const unsigned char text_test_data[text_test_data_size] =
    ARR(TEXT_TEST_DATA);
// clang-format off
static const MuTFFTextMediaInformationAtom text_test_struct = {
    {
      {1, 2},                      // matrix structure
      {3, 4},                      //
      {0, 0},                      //
      {7, 8},                      //
      {9, 10},                     //
      {0, 0},                      //
      {13, 14},                    //
      {15, 16},                    //
      {0, 0},                      //
    },
};
// clang-format on

TEST(MuTFF, WriteTextMediaInformationAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err =
      mutff_write_text_media_information_atom(fd, &bytes, &text_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, text_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, text_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], text_test_data[i]);
  }
}

static inline void expect_text_eq(const MuTFFTextMediaInformationAtom *a,
                                  const MuTFFTextMediaInformationAtom *b) {
  expect_matrix_eq(&a->matrix_structure, &b->matrix_structure);
}

TEST(MuTFF, ReadTextMediaInformationAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFTextMediaInformationAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(text_test_data, text_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_text_media_information_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, text_test_data_size);

  expect_text_eq(&atom, &text_test_struct);
  EXPECT_EQ(ftell(fd), text_test_data_size);
}
// }}}1

// {{{1 base media information header atom unit tests
static const uint32_t gmhd_test_data_size =
    8 + gmin_test_data_size + text_test_data_size;
// clang-format off
#define GMHD_TEST_DATA                     \
    gmhd_test_data_size >> 24,  /* size */ \
    gmhd_test_data_size >> 16,             \
    gmhd_test_data_size >> 8,              \
    gmhd_test_data_size,                   \
    'g', 'm', 'h', 'd',         /* type */ \
    GMIN_TEST_DATA,                        \
    TEXT_TEST_DATA
// clang-format on
static const unsigned char gmhd_test_data[gmhd_test_data_size] =
    ARR(GMHD_TEST_DATA);
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
  size_t bytes;
  const MuTFFError err = mutff_write_base_media_information_header_atom(
      fd, &bytes, &gmhd_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, gmhd_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, gmhd_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], gmhd_test_data[i]);
  }
}

static inline void expect_gmhd_eq(
    const MuTFFBaseMediaInformationHeaderAtom *a,
    const MuTFFBaseMediaInformationHeaderAtom *b) {
  expect_gmin_eq(&a->base_media_info, &b->base_media_info);
  EXPECT_EQ(a->text_media_information_present,
            b->text_media_information_present);
  const bool text_media_information_present =
      a->text_media_information_present && b->text_media_information_present;
  if (text_media_information_present) {
    expect_text_eq(&a->text_media_information, &b->text_media_information);
  }
}

TEST(MuTFF, ReadBaseMediaInformationHeaderAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFBaseMediaInformationHeaderAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(gmhd_test_data, gmhd_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_base_media_information_header_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, gmhd_test_data_size);

  expect_gmhd_eq(&atom, &gmhd_test_struct);
  EXPECT_EQ(ftell(fd), gmhd_test_data_size);
}
// }}}1

// {{{1 base media information atom unit tests
static const uint32_t base_minf_test_data_size = 8 + gmhd_test_data_size;
// clang-format off
#define BASE_MINF_TEST_DATA                     \
    base_minf_test_data_size >> 24,  /* size */ \
    base_minf_test_data_size >> 16,             \
    base_minf_test_data_size >> 8,              \
    base_minf_test_data_size,                   \
    'm', 'i', 'n', 'f',         /* type */      \
    GMHD_TEST_DATA
// clang-format on
static const unsigned char base_minf_test_data[base_minf_test_data_size] =
    ARR(BASE_MINF_TEST_DATA);
// clang-format off
static const MuTFFBaseMediaInformationAtom base_minf_test_struct = {
    gmhd_test_struct,        // base media information header
};
// clang-format on

TEST(MuTFF, WriteBaseMediaInformationAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err = mutff_write_base_media_information_atom(
      fd, &bytes, &base_minf_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, base_minf_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, base_minf_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], base_minf_test_data[i]);
  }
}

static inline void expect_base_minf_eq(const MuTFFBaseMediaInformationAtom *a,
                                       const MuTFFBaseMediaInformationAtom *b) {
  expect_gmhd_eq(&a->base_media_information_header,
                 &b->base_media_information_header);
}

TEST(MuTFF, ReadBaseMediaInformationAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFBaseMediaInformationAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(base_minf_test_data, base_minf_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_base_media_information_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, base_minf_test_data_size);

  expect_base_minf_eq(&atom, &base_minf_test_struct);
  EXPECT_EQ(ftell(fd), base_minf_test_data_size);
}
// }}}1

// {{{1 media atom unit tests
static const uint32_t video_hdlr_test_data_size = 32;
// clang-format off
#define VIDEO_HDLR_TEST_DATA                            \
  video_hdlr_test_data_size >> 24,                      \
  video_hdlr_test_data_size >> 16,                      \
  video_hdlr_test_data_size >> 8,                       \
  video_hdlr_test_data_size,                            \
  'h', 'd', 'l', 'r',                                   \
  0x00,                    /* version */                \
  0x00, 0x01, 0x02,        /* flags */                  \
  'm', 'h', 'l', 'r',      /* component type */         \
  'v', 'i', 'd', 'e',      /* component subtype */      \
  0x00, 0x00, 0x00, 0x00,  /* component manufacturer */ \
  0x00, 0x00, 0x00, 0x00,  /* component flags */        \
  0x00, 0x00, 0x00, 0x00   /* component flags mask */
// clang-format on
static const unsigned char video_hdlr_test_data[video_hdlr_test_data_size] =
    ARR(VIDEO_HDLR_TEST_DATA);
static const MuTFFHandlerReferenceAtom video_hdlr_test_struct = {
    0x00,
    0x000102,
    MuTFF_FOURCC('m', 'h', 'l', 'r'),
    MuTFF_FOURCC('v', 'i', 'd', 'e'),
    0,
    0,
    0,
    {},
};

static const uint32_t mdia_test_data_size =
    8 + mdhd_test_data_size + elng_test_data_size + video_hdlr_test_data_size +
    video_minf_test_data_size + udta_test_data_size;
// clang-format off
#define MDIA_TEST_DATA                            \
    mdia_test_data_size >> 24 & 0xFF,  /* size */ \
    mdia_test_data_size >> 16 & 0xFF,             \
    mdia_test_data_size >> 8 & 0xFF,              \
    mdia_test_data_size & 0xFF,                   \
    'm', 'd', 'i', 'a',                /* type */ \
    MDHD_TEST_DATA,                               \
    ELNG_TEST_DATA,                               \
    VIDEO_HDLR_TEST_DATA,                         \
    VIDEO_MINF_TEST_DATA,                         \
    UDTA_TEST_DATA
// clang-format on
static const unsigned char mdia_test_data[mdia_test_data_size] =
    ARR(MDIA_TEST_DATA);
// clang-format off
static const MuTFFMediaAtom mdia_test_struct = {
  mdhd_test_struct,
  true,
  elng_test_struct,
  true,
  video_hdlr_test_struct,
  true,
  video_minf_test_struct,
  {},
  {},
  true,
  udta_test_struct
};
// clang-format on

TEST(MuTFF, WriteMediaAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err = mutff_write_media_atom(fd, &bytes, &mdia_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, mdia_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, mdia_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], mdia_test_data[i]);
  }
}

static inline void expect_mdia_eq(const MuTFFMediaAtom *a,
                                  const MuTFFMediaAtom *b) {
  expect_mdhd_eq(&a->media_header, &b->media_header);
  EXPECT_EQ(a->extended_language_tag_present, b->extended_language_tag_present);
  const bool extended_language_tag_present =
      a->extended_language_tag_present && b->extended_language_tag_present;
  if (extended_language_tag_present) {
    expect_elng_eq(&a->extended_language_tag, &b->extended_language_tag);
  }
  EXPECT_EQ(a->handler_reference_present, b->handler_reference_present);
  const bool handler_reference_present =
      a->handler_reference_present && b->handler_reference_present;
  if (handler_reference_present) {
    expect_hdlr_eq(&a->handler_reference, &b->handler_reference);
  }
  EXPECT_EQ(a->media_information_present, b->media_information_present);
  const bool media_information_present =
      a->media_information_present && b->media_information_present;
  if (media_information_present) {
    MuTFFMediaType a_type;
    MuTFFMediaType b_type;
    ASSERT_EQ(mutff_media_atom_type(&a_type, a), MuTFFErrorNone);
    ASSERT_EQ(mutff_media_atom_type(&b_type, b), MuTFFErrorNone);
    EXPECT_EQ(a_type, b_type);
    switch (mutff_media_information_type(a_type)) {
      case MuTFFVideoMediaInformation:
        expect_video_minf_eq(&a->video_media_information,
                             &b->video_media_information);
        break;
      case MuTFFSoundMediaInformation:
        expect_sound_minf_eq(&a->sound_media_information,
                             &b->sound_media_information);
        break;
      case MuTFFBaseMediaInformation:
        expect_base_minf_eq(&a->base_media_information,
                            &b->base_media_information);
        break;
      default:
        FAIL();
        break;
    }
  }
  EXPECT_EQ(a->user_data_present, b->user_data_present);
  const bool user_data_present = a->user_data_present && b->user_data_present;
  if (user_data_present) {
    expect_udta_eq(&a->user_data, &b->user_data);
  }
}

TEST(MuTFF, ReadMediaAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFMediaAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(mdia_test_data, mdia_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_media_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, mdia_test_data_size);

  expect_mdia_eq(&atom, &mdia_test_struct);
  EXPECT_EQ(ftell(fd), mdia_test_data_size);
}
// }}}1

// {{{1 track atom unit tests
static const uint32_t trak_test_data_size =
    8 + tkhd_test_data_size + mdia_test_data_size + tapt_test_data_size +
    clip_test_data_size + matt_test_data_size + edts_test_data_size +
    tref_test_data_size + txas_test_data_size + load_test_data_size +
    imap_test_data_size + udta_test_data_size;
// clang-format off
#define TRAK_TEST_DATA                            \
    trak_test_data_size >> 24 & 0xFF,  /* size */ \
    trak_test_data_size >> 16 & 0xFF,             \
    trak_test_data_size >> 8 & 0xFF,              \
    trak_test_data_size & 0xFF,                   \
    't', 'r', 'a', 'k',                /* type */ \
    TKHD_TEST_DATA,                               \
    MDIA_TEST_DATA,                               \
    TAPT_TEST_DATA,                               \
    CLIP_TEST_DATA,                               \
    MATT_TEST_DATA,                               \
    EDTS_TEST_DATA,                               \
    TREF_TEST_DATA,                               \
    TXAS_TEST_DATA,                               \
    LOAD_TEST_DATA,                               \
    IMAP_TEST_DATA,                               \
    UDTA_TEST_DATA
// clang-format on
static const unsigned char trak_test_data[trak_test_data_size] =
    ARR(TRAK_TEST_DATA);
// clang-format off
static const MuTFFTrackAtom trak_test_struct = {
  tkhd_test_struct,
  mdia_test_struct,
  true,
  tapt_test_struct,
  true,
  clip_test_struct,
  true,
  matt_test_struct,
  true,
  edts_test_struct,
  true,
  tref_test_struct,
  true,
  txas_test_struct,
  true,
  load_test_struct,
  true,
  imap_test_struct,
  true,
  udta_test_struct
};
// clang-format on

TEST(MuTFF, WriteTrackAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err = mutff_write_track_atom(fd, &bytes, &trak_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, trak_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, trak_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], trak_test_data[i]);
  }
}

static inline void expect_trak_eq(const MuTFFTrackAtom *a,
                                  const MuTFFTrackAtom *b) {
  expect_tkhd_eq(&a->track_header, &b->track_header);
  expect_mdia_eq(&a->media, &b->media);
  EXPECT_EQ(a->track_aperture_mode_dimensions_present,
            b->track_exclude_from_autoselection_present);
  const bool track_aperture_mode_dimensions_present =
      a->track_aperture_mode_dimensions_present &&
      b->track_aperture_mode_dimensions_present;
  if (track_aperture_mode_dimensions_present) {
    expect_tapt_eq(&a->track_aperture_mode_dimensions,
                   &b->track_aperture_mode_dimensions);
  }
  EXPECT_EQ(a->clipping_present, b->clipping_present);
  const bool clipping_present = a->clipping_present && b->clipping_present;
  if (clipping_present) {
    expect_clip_eq(&a->clipping, &b->clipping);
  }
  EXPECT_EQ(a->track_matte_present, b->track_matte_present);
  const bool track_matte_present =
      a->track_matte_present && b->track_matte_present;
  if (track_matte_present) {
    expect_matt_eq(&a->track_matte, &b->track_matte);
  }
  EXPECT_EQ(a->edit_present, b->edit_present);
  const bool edit_present = a->edit_present && b->edit_present;
  if (edit_present) {
    expect_edts_eq(&a->edit, &b->edit);
  }
  EXPECT_EQ(a->track_reference_present, b->track_reference_present);
  const bool track_reference_present =
      a->track_reference_present && b->track_reference_present;
  if (track_reference_present) {
    expect_tref_eq(&a->track_reference, &b->track_reference);
  }
  EXPECT_EQ(a->track_exclude_from_autoselection_present,
            b->track_exclude_from_autoselection_present);
  const bool track_exclude_from_autoselection_present =
      a->track_exclude_from_autoselection_present &&
      b->track_exclude_from_autoselection_present;
  if (track_exclude_from_autoselection_present) {
    expect_txas_eq(&a->track_exclude_from_autoselection,
                   &b->track_exclude_from_autoselection);
  }
  EXPECT_EQ(a->track_load_settings_present, b->track_load_settings_present);
  const bool track_load_settings_present =
      a->track_load_settings_present && b->track_load_settings_present;
  if (track_load_settings_present) {
    expect_load_eq(&a->track_load_settings, &b->track_load_settings);
  }
  EXPECT_EQ(a->track_input_map_present, b->track_input_map_present);
  const bool track_input_map_present =
      a->track_input_map_present && b->track_input_map_present;
  if (track_input_map_present) {
    expect_imap_eq(&a->track_input_map, &b->track_input_map);
  }
  EXPECT_EQ(a->user_data_present, b->user_data_present);
  const bool user_data_present = a->user_data_present && b->user_data_present;
  if (user_data_present) {
    expect_udta_eq(&a->user_data, &b->user_data);
  }
}

TEST(MuTFF, ReadTrackAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFTrackAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(trak_test_data, trak_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_track_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, trak_test_data_size);

  expect_trak_eq(&atom, &trak_test_struct);
  EXPECT_EQ(ftell(fd), trak_test_data_size);
}
// }}}1

// {{{1 movie atom unit tests
static const uint32_t moov_test_data_size =
    8 + mvhd_test_data_size + trak_test_data_size + clip_test_data_size +
    ctab_test_data_size + udta_test_data_size;
// clang-format off
#define MOOV_TEST_DATA                            \
    moov_test_data_size >> 24 & 0xFF,  /* size */ \
    moov_test_data_size >> 16 & 0xFF,             \
    moov_test_data_size >> 8 & 0xFF,              \
    moov_test_data_size & 0xFF,                   \
    'm', 'o', 'o', 'v',                /* type */ \
    MVHD_TEST_DATA,                               \
    TRAK_TEST_DATA,                               \
    CLIP_TEST_DATA,                               \
    CTAB_TEST_DATA,                               \
    UDTA_TEST_DATA
// clang-format on
static const unsigned char moov_test_data[moov_test_data_size] =
    ARR(MOOV_TEST_DATA);
// clang-format off
static const MuTFFMovieAtom moov_test_struct = {
  mvhd_test_struct,
  1,
  {
    trak_test_struct,
  },
  true,
  clip_test_struct,
  true,
  ctab_test_struct,
  true,
  udta_test_struct,
};
// clang-format on

TEST(MuTFF, WriteMovieAtom) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err = mutff_write_movie_atom(fd, &bytes, &moov_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, moov_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, moov_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], moov_test_data[i]);
  }
}

static inline void expect_moov_eq(const MuTFFMovieAtom *a,
                                  const MuTFFMovieAtom *b) {
  expect_mvhd_eq(&a->movie_header, &b->movie_header);
  EXPECT_EQ(a->track_count, b->track_count);
  const size_t track_count =
      a->track_count > b->track_count ? b->track_count : a->track_count;
  for (size_t i = 0; i < track_count; ++i) {
    expect_trak_eq(&a->track[i], &b->track[i]);
  }
  EXPECT_EQ(a->clipping_present, b->clipping_present);
  const bool clipping_present = a->clipping_present && b->clipping_present;
  if (clipping_present) {
    expect_clip_eq(&a->clipping, &b->clipping);
  }
  EXPECT_EQ(a->color_table_present, b->color_table_present);
  const bool color_table_present =
      a->color_table_present && b->color_table_present;
  if (color_table_present) {
    expect_ctab_eq(&a->color_table, &b->color_table);
  }
  EXPECT_EQ(a->user_data_present, b->user_data_present);
  const bool user_data_present = a->user_data_present && b->user_data_present;
  if (user_data_present) {
    expect_udta_eq(&a->user_data, &b->user_data);
  }
}

TEST(MuTFF, ReadMovieAtom) {
  MuTFFError err;
  size_t bytes;
  MuTFFMovieAtom atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(moov_test_data, moov_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_movie_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, moov_test_data_size);

  expect_moov_eq(&atom, &moov_test_struct);
  EXPECT_EQ(ftell(fd), moov_test_data_size);
}
// }}}1

// {{{1 movie file unit tests
static const uint32_t file_test_data_size =
    ftyp_test_data_size + wide_test_data_size + mdat_test_data_size +
    free_test_data_size + skip_test_data_size + moov_test_data_size +
    pnot_test_data_size;
// clang-format off
#define FILE_TEST_DATA \
  FTYP_TEST_DATA,      \
  MOOV_TEST_DATA,      \
  MDAT_TEST_DATA,      \
  FREE_TEST_DATA,      \
  SKIP_TEST_DATA,      \
  WIDE_TEST_DATA,      \
  PNOT_TEST_DATA
static const unsigned char file_test_data[file_test_data_size] =
    ARR(FILE_TEST_DATA);
// clang-format off
static const MuTFFMovieFile file_test_struct = {
  true,
  ftyp_test_struct,
  moov_test_struct,
  1,
  {
    mdat_test_struct,
  },
  1,
  {
    free_test_struct,
  },
  1,
  {
    skip_test_struct,
  },
  1,
  {
    wide_test_struct,
  },
  true,
  pnot_test_struct
};
// clang-format on

TEST(MuTFF, WriteMovieFile) {
  // clang-format on
  FILE *fd = fopen("temp.mov", "w+b");
  size_t bytes;
  const MuTFFError err = mutff_write_movie_file(fd, &bytes, &file_test_struct);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, file_test_data_size);

  const size_t file_size = ftell(fd);
  rewind(fd);
  unsigned char data[file_size];
  fread(data, file_size, 1, fd);
  EXPECT_EQ(file_size, file_test_data_size);
  for (size_t i = 0; i < file_size; ++i) {
    EXPECT_EQ(data[i], file_test_data[i]);
  }
}

static inline void expect_file_eq(const MuTFFMovieFile *a,
                                  const MuTFFMovieFile *b) {
  EXPECT_EQ(a->file_type_present, b->file_type_present);
  const bool file_type_present = a->file_type_present && b->file_type_present;
  if (file_type_present) {
    expect_filetype_eq(&a->file_type, &b->file_type);
  }
  expect_moov_eq(&a->movie, &b->movie);
  EXPECT_EQ(a->movie_data_count, b->movie_data_count);
  const size_t movie_data_count = a->movie_data_count > b->movie_data_count
                                      ? b->movie_data_count
                                      : a->movie_data_count;
  for (size_t i = 0; i < movie_data_count; ++i) {
    expect_mdat_eq(&a->movie_data[i], &b->movie_data[i]);
  }
  EXPECT_EQ(a->free_count, b->free_count);
  const size_t free_count =
      a->free_count > b->free_count ? b->free_count : a->free_count;
  for (size_t i = 0; i < free_count; ++i) {
    expect_free_eq(&a->free[i], &b->free[i]);
  }
  EXPECT_EQ(a->skip_count, b->skip_count);
  const size_t skip_count =
      a->skip_count > b->skip_count ? b->skip_count : a->skip_count;
  for (size_t i = 0; i < skip_count; ++i) {
    expect_skip_eq(&a->skip[i], &b->skip[i]);
  }
  EXPECT_EQ(a->wide_count, b->wide_count);
  const size_t wide_count =
      a->wide_count > b->wide_count ? b->wide_count : a->wide_count;
  for (size_t i = 0; i < wide_count; ++i) {
    expect_wide_eq(&a->wide[i], &b->wide[i]);
  }
  EXPECT_EQ(a->preview_present, b->preview_present);
  const bool preview_present = a->preview_present && b->preview_present;
  if (preview_present) {
    expect_pnot_eq(&a->preview, &b->preview);
  }
}

TEST(MuTFF, ReadMovieFile) {
  MuTFFError err;
  size_t bytes;
  MuTFFMovieFile atom;
  FILE *fd = fopen("temp.mov", "w+b");
  fwrite(file_test_data, file_test_data_size, 1, fd);
  rewind(fd);
  err = mutff_read_movie_file(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, file_test_data_size);

  expect_file_eq(&atom, &file_test_struct);
  EXPECT_EQ(ftell(fd), file_test_data_size);
}
// }}}1

// {{{1 test.mov tests
// {{{2 common
class TestMov : public ::testing::Test {
 protected:
  FILE *fd;
  size_t size;

  // @TODO: check test.mov opened correctly
  void SetUp() override { fd = fopen("test.mov", "rb"); }

  void TearDown() override { fclose(fd); }
};
// }}}2

// {{{2 MovieFile
TEST_F(TestMov, MovieFile) {
  MuTFFMovieFile movie_file;
  size_t bytes;
  const MuTFFError err = mutff_read_movie_file(fd, &bytes, &movie_file);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, 29036);

  EXPECT_EQ(movie_file.file_type_present, true);
  EXPECT_EQ(movie_file.movie_data_count, 1);
  EXPECT_EQ(movie_file.free_count, 0);
  EXPECT_EQ(movie_file.skip_count, 0);
  EXPECT_EQ(movie_file.wide_count, 1);
  EXPECT_EQ(movie_file.preview_present, false);
  EXPECT_EQ(ftell(fd), 29036);
}
// }}}2

// {{{2 MovieAtom
TEST_F(TestMov, MovieAtom) {
  const size_t offset = 28330;
  MuTFFMovieAtom movie_atom;
  fseek(fd, offset, SEEK_SET);
  size_t bytes;
  const MuTFFError err = mutff_read_movie_atom(fd, &bytes, &movie_atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, 706);

  EXPECT_EQ(movie_atom.track_count, 1);

  EXPECT_EQ(ftell(fd), offset + 706);
}
// }}}2

// {{{2 MovieHeaderAtom
TEST_F(TestMov, MovieHeaderAtom) {
  const size_t offset = 28338;
  MuTFFMovieHeaderAtom movie_header_atom;
  fseek(fd, offset, SEEK_SET);
  size_t bytes;
  const MuTFFError err =
      mutff_read_movie_header_atom(fd, &bytes, &movie_header_atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, 108);

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
// }}}2

// {{{2 FileTypeAtom
TEST_F(TestMov, FileTypeAtom) {
  const size_t offset = 0;
  MuTFFFileTypeAtom file_type_atom;
  fseek(fd, offset, SEEK_SET);
  size_t bytes;
  const MuTFFError err = mutff_read_file_type_atom(fd, &bytes, &file_type_atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, 20);

  EXPECT_EQ(file_type_atom.major_brand, MuTFF_FOURCC('q', 't', ' ', ' '));
  EXPECT_EQ(file_type_atom.minor_version, 0x00000200);
  EXPECT_EQ(file_type_atom.compatible_brands_count, 1);
  EXPECT_EQ(file_type_atom.compatible_brands[0],
            MuTFF_FOURCC('q', 't', ' ', ' '));

  EXPECT_EQ(ftell(fd), offset + 20);
}
// }}}2

// @TODO: Add tests for special sizes
// {{{2 MovieDataAtom
TEST_F(TestMov, MovieDataAtom) {
  const size_t offset = 28;
  MuTFFMovieDataAtom movie_data_atom;
  fseek(fd, offset, SEEK_SET);
  size_t bytes;
  const MuTFFError err =
      mutff_read_movie_data_atom(fd, &bytes, &movie_data_atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, 28302);

  EXPECT_EQ(ftell(fd), offset + 28302);
}
// }}}2

// {{{2 WideAtom
TEST_F(TestMov, WideAtom) {
  const size_t offset = 20;
  MuTFFWideAtom wide_atom;
  fseek(fd, offset, SEEK_SET);
  size_t bytes;
  const MuTFFError err = mutff_read_wide_atom(fd, &bytes, &wide_atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, 8);

  EXPECT_EQ(ftell(fd), offset + 8);
}
// }}}2

// {{{2 TrackAtom
TEST_F(TestMov, TrackAtom) {
  const size_t offset = 28446;
  MuTFFTrackAtom track_atom;
  fseek(fd, offset, SEEK_SET);
  size_t bytes;
  const MuTFFError err = mutff_read_track_atom(fd, &bytes, &track_atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, 557);

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
// }}}2

// {{{2 TrackHeaderAtom
TEST_F(TestMov, TrackHeaderAtom) {
  const size_t offset = 28454;
  MuTFFTrackHeaderAtom atom;
  fseek(fd, offset, SEEK_SET);
  size_t bytes;
  const MuTFFError err = mutff_read_track_header_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, 92);

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
  EXPECT_EQ(atom.matrix_structure.a.integral, 1);
  EXPECT_EQ(atom.matrix_structure.a.fractional, 0);
  EXPECT_EQ(atom.matrix_structure.b.integral, 0);
  EXPECT_EQ(atom.matrix_structure.b.fractional, 0);
  EXPECT_EQ(atom.matrix_structure.u.integral, 0);
  EXPECT_EQ(atom.matrix_structure.u.fractional, 0);
  EXPECT_EQ(atom.matrix_structure.c.integral, 0);
  EXPECT_EQ(atom.matrix_structure.c.fractional, 0);
  EXPECT_EQ(atom.matrix_structure.d.integral, 1);
  EXPECT_EQ(atom.matrix_structure.d.fractional, 0);
  EXPECT_EQ(atom.matrix_structure.v.integral, 0);
  EXPECT_EQ(atom.matrix_structure.v.fractional, 0);
  EXPECT_EQ(atom.matrix_structure.tx.integral, 0);
  EXPECT_EQ(atom.matrix_structure.tx.fractional, 0);
  EXPECT_EQ(atom.matrix_structure.ty.integral, 0);
  EXPECT_EQ(atom.matrix_structure.ty.fractional, 0);
  EXPECT_EQ(atom.matrix_structure.w.integral, 1);
  EXPECT_EQ(atom.matrix_structure.w.fractional, 0);
  EXPECT_EQ(atom.track_width.integral, 640);
  EXPECT_EQ(atom.track_width.fractional, 0);
  EXPECT_EQ(atom.track_height.integral, 480);
  EXPECT_EQ(atom.track_height.fractional, 0);

  EXPECT_EQ(ftell(fd), offset + 92);
}
// }}}2

// {{{2 EditAtom
TEST_F(TestMov, EditAtom) {
  const size_t offset = 28546;
  MuTFFEditAtom atom;
  fseek(fd, offset, SEEK_SET);
  size_t bytes;
  const MuTFFError err = mutff_read_edit_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, 36);

  EXPECT_EQ(atom.edit_list_atom.version, 0x00);
  EXPECT_EQ(atom.edit_list_atom.flags, 0x000000);
  EXPECT_EQ(atom.edit_list_atom.number_of_entries, 1);
  EXPECT_EQ(atom.edit_list_atom.edit_list_table[0].track_duration, 0x048f);
  EXPECT_EQ(atom.edit_list_atom.edit_list_table[0].media_time, 0);
  EXPECT_EQ(atom.edit_list_atom.edit_list_table[0].media_rate.integral, 1);
  EXPECT_EQ(atom.edit_list_atom.edit_list_table[0].media_rate.fractional, 0);

  EXPECT_EQ(ftell(fd), offset + 36);
}
// }}}2

// {{{2 MediaAtom
TEST_F(TestMov, MediaAtom) {
  const size_t offset = 28582;
  MuTFFMediaAtom atom;
  fseek(fd, offset, SEEK_SET);
  size_t bytes;
  const MuTFFError err = mutff_read_media_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, 421);

  EXPECT_EQ(atom.extended_language_tag_present, false);
  EXPECT_EQ(atom.handler_reference_present, true);
  EXPECT_EQ(atom.media_information_present, true);
  EXPECT_EQ(atom.user_data_present, false);

  EXPECT_EQ(ftell(fd), offset + 421);
}
// }}}2

// {{{2 MediaHeaderAtom
TEST_F(TestMov, MediaHeaderAtom) {
  const size_t offset = 28590;
  MuTFFMediaHeaderAtom atom;
  fseek(fd, offset, SEEK_SET);
  size_t bytes;
  const MuTFFError err = mutff_read_media_header_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, 32);

  EXPECT_EQ(ftell(fd), offset + 32);
}
// }}}2

// {{{2 MediaHandlerReferenceAtom
TEST_F(TestMov, MediaHandlerReferenceAtom) {
  const size_t offset = 28622;
  MuTFFHandlerReferenceAtom atom;
  fseek(fd, offset, SEEK_SET);
  size_t bytes;
  const MuTFFError err = mutff_read_handler_reference_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, 45);

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
// }}}2

// {{{2 VideoMediaInformationHeader
TEST_F(TestMov, VideoMediaInformationHeader) {
  const size_t offset = 28675;
  MuTFFVideoMediaInformationHeaderAtom atom;
  fseek(fd, offset, SEEK_SET);
  size_t bytes;
  const MuTFFError err =
      mutff_read_video_media_information_header_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, 20);

  EXPECT_EQ(ftell(fd), offset + 20);
}
// }}}2

// {{{2 VideoMediaInformationHandlerReference
TEST_F(TestMov, VideoMediaInformationHandlerReference) {
  const size_t offset = 28695;
  MuTFFHandlerReferenceAtom atom;
  fseek(fd, offset, SEEK_SET);
  size_t bytes;
  const MuTFFError err = mutff_read_handler_reference_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, 44);

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
// }}}2

// {{{2 VideoMediaInformationDataInformation
TEST_F(TestMov, VideoMediaInformationDataInformation) {
  const size_t offset = 28739;
  MuTFFDataInformationAtom atom;
  fseek(fd, offset, SEEK_SET);
  size_t bytes;
  const MuTFFError err = mutff_read_data_information_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, 36);

  EXPECT_EQ(ftell(fd), offset + 36);
}
// }}}2

// {{{2 VideoMediaInformationSampleTable
TEST_F(TestMov, VideoMediaInformationSampleTable) {
  const size_t offset = 28775;
  MuTFFSampleTableAtom atom;
  fseek(fd, offset, SEEK_SET);
  size_t bytes;
  const MuTFFError err = mutff_read_sample_table_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, 228);

  EXPECT_EQ(ftell(fd), offset + 228);
}
// }}}2

// {{{2 VideoMediaInformationSampleTableDescription
TEST_F(TestMov, VideoMediaInformationSampleTableDescription) {
  const size_t offset = 28783;
  MuTFFSampleDescriptionAtom atom;
  fseek(fd, offset, SEEK_SET);
  size_t bytes;
  const MuTFFError err = mutff_read_sample_description_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, 128);

  EXPECT_EQ(ftell(fd), offset + 128);
}
// }}}2

// {{{2 TimeToSample
TEST_F(TestMov, TimeToSample) {
  const size_t offset = 28911;
  MuTFFTimeToSampleAtom atom;
  fseek(fd, offset, SEEK_SET);
  size_t bytes;
  const MuTFFError err = mutff_read_time_to_sample_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, 24);

  EXPECT_EQ(ftell(fd), offset + 24);
}
// }}}2

// {{{2 SampleToChunk
TEST_F(TestMov, SampleToChunk) {
  const size_t offset = 28935;
  MuTFFSampleToChunkAtom atom;
  fseek(fd, offset, SEEK_SET);
  size_t bytes;
  const MuTFFError err = mutff_read_sample_to_chunk_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, 28);

  EXPECT_EQ(ftell(fd), offset + 28);
}
// }}}2

// {{{2 SampleSize
TEST_F(TestMov, SampleSize) {
  const size_t offset = 28963;
  MuTFFSampleSizeAtom atom;
  fseek(fd, offset, SEEK_SET);
  size_t bytes;
  const MuTFFError err = mutff_read_sample_size_atom(fd, &bytes, &atom);
  ASSERT_EQ(err, MuTFFErrorNone);
  EXPECT_EQ(bytes, 20);

  EXPECT_EQ(atom.version, 0x00);
  EXPECT_EQ(atom.flags, 0x000000);
  EXPECT_EQ(atom.sample_size, 0x07e5);
  EXPECT_EQ(atom.number_of_entries, 0x0e);

  EXPECT_EQ(ftell(fd), offset + 20);
}
// }}}2
// }}}1

// vi:sw=2:ts=2:et:fdm=marker
