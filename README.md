# µTFF
![Main Build Status](https://github.com/frankplow/mutff/actions/workflows/build.yml/badge.svg)
![Main Build Status](https://github.com/frankplow/mutff/actions/workflows/static-analysis.yml/badge.svg)
![Test Coverage](https://frankplow.github.io/mutff/badges/coverage.svg)

A small QuickTime file format (QTFF) library.

This library came out of a project I was doing where I needed a simple QTFF library to run on an ARM Cortex-M microcontroller. It is [somewhat MISRA compliant](#misra-compliance).

## Examples
Here is an example of how to use the library to find the duration of a movie:
```c
#include <stdio.h>
#include <mutff.h>

int movie_duration(FILE *file) {
  MuTFFMovieFile movie_file;
  int time_scale;
  int duration;

  if (mutff_read_movie_file(file, &movie_file) < 0) {
    fprintf(stderr, "Failed to parse movie file.");
    return -1;
  }

  time_scale = movie_file.movie.movie_header.time_scale;
  duration = movie_file.movie.movie_header.duration / time_scale;

  return duration;
}
```

## MISRA Compliance
The project is _not_ [MISRA](https://www.misra.org.uk/) compliant. It intentionally violates the following rules:
* 19.2
* 21.6
* 2.7 (style advisory)
* 8.7 (advisory, inapplicable to libraries)
* 12.1 (style advisory)
* 15.5 (style advisory)
* 17.8 (style advisory)

Additionally, MISRA compliance is determined by static analysis with [cppcheck](https://cppcheck.sourceforge.io/), which is not comprehensive.

The project does aim to abide by most of the MISRA rules however. In practice it follows the most significant rules such as not performing dynamic allocation. The most significant violations are those of rule 19.2 and rule 21.6. These violations will likely be removed in the future.

## Documentation
Documentation for the latest `main` is available on the [GitHub Pages site for this project](https://frankplow.github.io/mutff).

## Building from Source
To configure, while in the project root, run:
```
$ cmake -S . -B build
```
and to compile, again in the project root, run:
```
$ cmake --build
```

The project builds a static library by default. To build a shared library, pass `-DBUILD_SHARED_LIBS=1` during the configuration step. Other options supported by the project (and their defaults) are:
* `MUTFF_BUILD_TESTS` (`ON`)
* `MUTFF_BUILD_COVERAGE` (`OFF`)
* `MUTFF_BUILD_DOCS` (`OFF`)

## References
* [QTFF specification](https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFPreface/qtffPreface.html)

## License
This project is released under the GNU Public License Version 3. For the terms of this license, see [LICENSE.md](LICENSE.md).
