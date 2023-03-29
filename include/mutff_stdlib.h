#ifndef MUTFF_STDLIB_H_
#define MUTFF_STDLIB_H_

#include "mutff.h"

MuTFFError mutff_read_stdlib(FILE *file, void *dest, unsigned int bytes);

MuTFFError mutff_write_stdlib(FILE *file, void *src, unsigned int bytes);

MuTFFError mutff_tell_stdlib(FILE *file, unsigned int *location);

MuTFFError mutff_seek_stdlib(FILE *file, long delta);

#endif  // MUTFF_STDLIB_H_
