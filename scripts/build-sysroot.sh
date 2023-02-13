
TARGET="$1"

export CC="$2"
export CFLAGS="$3"

# musl
mkdir -p musl-$TARGET
(
    cd musl-$TARGET

    ../musl/configure --disable-shared --prefix=$PWD/../sysroot-$TARGET

    make install -j
)
