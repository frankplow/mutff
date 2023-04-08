///
/// @file      mutff_stdlib.h
/// @author    Frank Plowman <post@frankplowman.com>
/// @brief     MuTFF MP4/QTFF library standard library I/O driver header file
/// @copyright 2023 Frank Plowman
/// @license   This project is released under the GNU Public License Version 3.
///            For the terms of this license, see [LICENSE.md](LICENSE.md)
///

#ifndef MUTFF_STDLIB_H_
#define MUTFF_STDLIB_H_

#include <stdio.h>

#include "mutff.h"

/// @addtogroup MuTFF
/// @{

MuTFFError mutff_read_stdlib(mutff_file_t *file, void *dest,
                             unsigned int bytes);

MuTFFError mutff_write_stdlib(mutff_file_t *file, const void *src,
                              unsigned int bytes);

MuTFFError mutff_tell_stdlib(mutff_file_t *file, unsigned int *location);

MuTFFError mutff_seek_stdlib(mutff_file_t *file, long delta);

/// @} MuTFF

#endif  // MUTFF_STDLIB_H_

// vi:sw=2:ts=2:et:fdm=marker
