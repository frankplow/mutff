///
/// @file      mutff_error.h
/// @author    Frank Plowman <post@frankplowman.com>
/// @brief     MuTFF MP4/QTFF library errors header file
/// @copyright 2023 Frank Plowman
/// @license   This project is released under the GNU Public License Version 3.
///            For the terms of this license, see [LICENSE.md](LICENSE.md)
///

#ifndef MUTFF_ERROR_H_
#define MUTFF_ERROR_H_

/// @addtogroup MuTFF
/// @{

///
/// @brief A generic error in the MuTFF library
///
typedef enum {
  MuTFFErrorNone,
  MuTFFErrorIOError,
  MuTFFErrorEOF,
  MuTFFErrorBadFormat,
  MuTFFErrorOutOfMemory,
} MuTFFError;

/// @} MuTFF

#endif  // MUTFF_ERROR_H_

// vi:sw=2:ts=2:et:fdm=marker
