#ifndef MUTFF_STDLIB_H_
#define MUTFF_STDLIB_H_

#include "mutff.h"

MuTFFError mutff_read_stdlib(mutff_file_t *file, void *dest,
                             unsigned int bytes);

MuTFFError mutff_write_stdlib(mutff_file_t *file, void *src,
                              unsigned int bytes);

MuTFFError mutff_tell_stdlib(mutff_file_t *file, unsigned int *location);

MuTFFError mutff_seek_stdlib(mutff_file_t *file, long delta);

#endif  // MUTFF_STDLIB_H_
