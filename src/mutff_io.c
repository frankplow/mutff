///
/// @file      mutff_io.c
/// @author    Frank Plowman <post@frankplowman.com>
/// @brief     MuTFF MP4/QTFF library I/O driver source file
/// @copyright 2022 Frank Plowman
/// @license   This project is released under the GNU Public License Version 3.
///            For the terms of this license, see [LICENSE.md](LICENSE.md)
///

#include "mutff_io.h"

MuTFFReadFn mutff_read;

void mutff_set_read_fn(MuTFFReadFn fn) { mutff_read = fn; }

MuTFFWriteFn mutff_write;

void mutff_set_write_fn(MuTFFWriteFn fn) { mutff_write = fn; }

MuTFFTellFn mutff_tell;

void mutff_set_tell_fn(MuTFFTellFn fn) { mutff_tell = fn; }

MuTFFSeekFn mutff_seek;

void mutff_set_seek_fn(MuTFFSeekFn fn) { mutff_seek = fn; }

// vi:sw=2:ts=2:et:fdm=marker
