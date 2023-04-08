///
/// @file      mutff_io.h
/// @author    Frank Plowman <post@frankplowman.com>
/// @brief     MuTFF MP4/QTFF library I/O driver header file
/// @copyright 2023 Frank Plowman
/// @license   This project is released under the GNU Public License Version 3.
///            For the terms of this license, see [LICENSE.md](LICENSE.md)
///

#ifndef MUTFF_IO_H_
#define MUTFF_IO_H_

#include "mutff_error.h"

/// @addtogroup MuTFF
/// @{

///
/// @brief An I/O driver stream. Typical examples would be a file or socket.
///
typedef void mutff_file_t;

///
/// @brief I/O driver function to read data
///
/// @param [in] stream The stream to read data on.
/// @param [out] data  A pointer to where to store the read data.
/// @param [in] bytes  The number of bytes to read. If not exactly this number
///                    of bytes is read, then it is an error.
/// @return            The MuTFFError code.
///
typedef MuTFFError (*MuTFFReadFn)(mutff_file_t *stream, void *data,
                                  unsigned int bytes);

///
/// @brief I/O driver function to write data
///
/// @param [in] stream The stream to write data to.
/// @param [in] data   A pointer to the data to write.
/// @param [in] bytes  The number of bytes to write. If not exactly this number
///                    of bytes is written, then it is an error.
/// @return            The MuTFFError code.
///
typedef MuTFFError (*MuTFFWriteFn)(mutff_file_t *stream, const void *data,
                                   unsigned int bytes);

///
/// @brief I/O driver function to get current position in stream
///
/// @param [in] stream The stream.
/// @param [out] pos   A pointer to where to store the position.
/// @return            The MuTFFError code.
///
typedef MuTFFError (*MuTFFTellFn)(mutff_file_t *stream, unsigned int *pos);

///
/// @brief I/O driver function to set current position in stream
///
/// @param [in] stream The stream.
/// @param [out] pos   The relative position.
/// @return            The MuTFFError code.
///
typedef MuTFFError (*MuTFFSeekFn)(mutff_file_t *stream, long pos);

typedef struct {
  MuTFFReadFn read;
  MuTFFWriteFn write;
  MuTFFTellFn tell;
  MuTFFSeekFn seek;
} MuTFFIODriver;

/// @} MuTFF

#endif  // MUTFF_IO_H_

// vi:sw=2:ts=2:et:fdm=marker
