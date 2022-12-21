# µUTFF
A small QuickTime file format (QTFF) library.

This library came out of a project I was doing where I needed a simple, relatively efficient QTFF library to run on a Cortex-M microcontroller. I discovered there were few QTFF libraries readily available and none which met my needs.

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
* `BUILD_TESTS` (`ON`)
* `BUILD_DOCS` (`OFF`)

## References
* [QTFF specification](https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFPreface/qtffPreface.html)

## License
This project is released under the GNU Public License Version 3. For the terms of this license, see [LICENSE.md](LICENSE.md).
