///
/// @file      mutff_core.c
/// @author    Frank Plowman <post@frankplowman.com>
/// @brief     MuTFF MP4/QTFF library core systems source file
/// @copyright 2023 Frank Plowman
/// @license   This project is released under the GNU Public License Version 3.
///            For the terms of this license, see [LICENSE.md](LICENSE.md)
///

#include "mutff_core.h"

#include "mutff_error.h"
#include "mutff_io.h"

inline MuTFFError mutff_read(MuTFFContext *ctx, void *data,
                             unsigned int bytes) {
  return ctx->io.read(ctx->file, data, bytes);
}

inline MuTFFError mutff_write(MuTFFContext *ctx, const void *data,
                              unsigned int bytes) {
  return ctx->io.write(ctx->file, data, bytes);
}

inline MuTFFError mutff_tell(MuTFFContext *ctx, unsigned int *pos) {
  return ctx->io.tell(ctx->file, pos);
}

inline MuTFFError mutff_seek(MuTFFContext *ctx, long pos) {
  return ctx->io.seek(ctx->file, pos);
}

// vi:sw=2:ts=2:et:fdm=marker
