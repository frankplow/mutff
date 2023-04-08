///
/// @file      mutff_core.h
/// @author    Frank Plowman <post@frankplowman.com>
/// @brief     MuTFF MP4/QTFF library core systems header file
/// @copyright 2023 Frank Plowman
/// @license   This project is released under the GNU Public License Version 3.
///            For the terms of this license, see [LICENSE.md](LICENSE.md)
///

#ifndef MUTFF_CORE_H_
#define MUTFF_CORE_H_

#include <stddef.h>
#include <stdint.h>

#include "mutff_error.h"
#include "mutff_io.h"

/// @addtogroup MuTFF
/// @{

///
/// @brief Generic function to write an atom
///
/// @param [in] stream The stream to write data to.
/// @param [out] bytes The number of bytes written.
/// @param [in] data   Any atom data.
/// @return            The MuTFFError code.
///
typedef MuTFFError (*MuTFFAtomReadFn)(mutff_file_t *stream, size_t *bytes,
                                      void *data);

///
/// @brief Generic function to read an atom
///
/// @param [in] stream The stream to read data from.
/// @param [out] bytes The number of bytes read.
/// @param [out] data  A pointer to where to store the data.
/// @return            The MuTFFError code.
///
typedef MuTFFError (*MuTFFAtomWriteFn)(mutff_file_t *stream, size_t *bytes,
                                       const void *data);

///
/// @brief Generic function to get the size of an atom
///
/// @param [out] size A pointer to where to store the atom's size.
/// @param [in] data  A pointer to the atom data.
/// @return           The MuTFFError code.
///
typedef MuTFFError (*MuTFFAtomSizeFn)(uint64_t *size, const void *data);

/// @} MuTFF

#endif  // MUTFF_CORE_H_

// vi:sw=2:ts=2:et:fdm=marker
