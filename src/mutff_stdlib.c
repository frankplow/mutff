#include "mutff_stdlib.h"

#include <errno.h>
#include <stdio.h>

#include "mutff.h"

MuTFFError mutff_read_stdlib(mutff_file_t *file, void *dest,
                             unsigned int bytes) {
  const size_t read = fread(dest, bytes, 1, file);
  if (read != 1U) {
    if (feof(file) != 0) {
      return MuTFFErrorEOF;
    }
    return MuTFFErrorIOError;
  }
  return MuTFFErrorNone;
}

MuTFFError mutff_write_stdlib(mutff_file_t *file, void *src,
                              unsigned int bytes) {
  const size_t written = fwrite(src, bytes, 1, file);
  if (written != 1U) {
    if (feof(file) != 0) {
      return MuTFFErrorEOF;
    }
    return MuTFFErrorIOError;
  }
  return MuTFFErrorNone;
}

MuTFFError mutff_tell_stdlib(mutff_file_t *file, unsigned int *location) {
  errno = 0;
  const long ret = ftell(file);
  if (errno != 0) {
    return MuTFFErrorIOError;
  }
  if (ret == -1) {
    return MuTFFErrorIOError;
  }
  *location = ret;
  return MuTFFErrorNone;
}

MuTFFError mutff_seek_stdlib(mutff_file_t *file, long delta) {
  const int err = fseek(file, delta, SEEK_CUR);
  if (err != 0) {
    return MuTFFErrorIOError;
  }
  return MuTFFErrorNone;
}
