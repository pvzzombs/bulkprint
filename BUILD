Building bulkprint:
mkdir build && cd build && cmake .. -DDEPS_ROOT=/path/to/usr
cmake --build .

Dependencies:
  Building LibHaru PDF Library:
  cmake .. -DZLIB_INCLUDE_DIR=/path/to/include -DZLIB_LIBRARY=/path/to/lib.a  -DBUILD_SHARED_LIBS=OFF -DCMAKE_DISABLE_FIND_PACKAGE_PNG=ON -DCMAKE_INSTALL_PREFIX=/usr
  LibHaru dependencies:
    ZLIB:
    ./configure --prefix=/usr --libdir=/usr/lib64 --static
    make -j2
    DESTDIR=/some/path make install

