Building LibHaru PDF Library:

cmake .. -DZLIB_INCLUDE_DIR=/path/to/include -DZLIB_LIBRARY=/path/to/lib.a  -DBUILD_SHARED_LIBS=OFF -DCMAKE_DISABLE_FIND_PACKAGE_PNG=ON -DCMAKE_INSTALL_PREFIX=/usr

Dependencies:

ZLIB:
./configure --prefix=/usr --libdir=/usr/lib64 --static
make -j2
DESTDIR=/some/path make install

