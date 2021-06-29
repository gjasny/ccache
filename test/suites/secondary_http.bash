SUITE_secondary_http_PROBE() {
    touch pch.h empty.c
    mkdir dir

    if ! "$HTTP_SERVER" --help >/dev/null 2>&1; then
        echo "Cannot execute ($HTTP_SERVER). Python 3 might be missing installed"
    fi
}

SUITE_secondary_http_SETUP() {
    unset CCACHE_NODIRECT
    export CCACHE_SECONDARY_STORAGE="http://127.0.0.1:8080/"

    trap 'kill $(jobs -p)' EXIT

    mkdir "$PWD/secondary"
    "$HTTP_SERVER" --directory "$PWD/secondary" --bind 127.0.0.1 8080 > $ABS_TESTDIR/http.log 2>&1 &
    sleep 1 # http server startup is slower than test

    generate_code 1 test.c
}

SUITE_secondary_http() {
    # -------------------------------------------------------------------------
    TEST "Base case"

    $CCACHE_COMPILE -c test.c
    expect_stat 'cache hit (direct)' 0
    expect_stat 'cache miss' 1
    expect_stat 'files in cache' 2
    expect_file_count 2 '*' secondary # result + manifest

    $CCACHE_COMPILE -c test.c
    expect_stat 'cache hit (direct)' 1
    expect_stat 'cache miss' 1
    expect_stat 'files in cache' 2
    expect_file_count 2 '*' secondary # result + manifest

    $CCACHE -C >/dev/null
    expect_stat 'files in cache' 0
    expect_file_count 2 '*' secondary # result + manifest

    $CCACHE_COMPILE -c test.c
    expect_stat 'cache hit (direct)' 2
    expect_stat 'cache miss' 1
    expect_stat 'files in cache' 0
    expect_stat 'files in cache' 0
    expect_file_count 2 '*' secondary # result + manifest
return 0
    # -------------------------------------------------------------------------
    TEST "Two directories"

    CCACHE_SECONDARY_STORAGE+=" file://$PWD/secondary_2"
    mkdir secondary_2

    $CCACHE_COMPILE -c test.c
    expect_stat 'cache hit (direct)' 0
    expect_stat 'cache miss' 1
    expect_stat 'files in cache' 2
    expect_file_count 3 '*' secondary # CACHEDIR.TAG + result + manifest
    expect_file_count 3 '*' secondary_2 # CACHEDIR.TAG + result + manifest

    $CCACHE -C >/dev/null
    expect_stat 'files in cache' 0
    expect_file_count 3 '*' secondary # CACHEDIR.TAG + result + manifest
    expect_file_count 3 '*' secondary_2 # CACHEDIR.TAG + result + manifest

    $CCACHE_COMPILE -c test.c
    expect_stat 'cache hit (direct)' 1
    expect_stat 'cache miss' 1
    expect_stat 'files in cache' 0
    expect_file_count 3 '*' secondary # CACHEDIR.TAG + result + manifest
    expect_file_count 3 '*' secondary_2 # CACHEDIR.TAG + result + manifest

    rm -r secondary/??
    expect_file_count 1 '*' secondary # CACHEDIR.TAG

    $CCACHE_COMPILE -c test.c
    expect_stat 'cache hit (direct)' 2
    expect_stat 'cache miss' 1
    expect_stat 'files in cache' 0
    expect_file_count 1 '*' secondary # CACHEDIR.TAG
    expect_file_count 3 '*' secondary_2 # CACHEDIR.TAG + result + manifest

    # -------------------------------------------------------------------------
    TEST "Read-only"

    $CCACHE_COMPILE -c test.c
    expect_stat 'cache hit (direct)' 0
    expect_stat 'cache miss' 1
    expect_stat 'files in cache' 2
    expect_file_count 3 '*' secondary # CACHEDIR.TAG + result + manifest

    $CCACHE -C >/dev/null
    expect_stat 'files in cache' 0
    expect_file_count 3 '*' secondary # CACHEDIR.TAG + result + manifest

    CCACHE_SECONDARY_STORAGE+="|read-only"

    $CCACHE_COMPILE -c test.c
    expect_stat 'cache hit (direct)' 1
    expect_stat 'cache miss' 1
    expect_stat 'files in cache' 0
    expect_file_count 3 '*' secondary # CACHEDIR.TAG + result + manifest

    echo 'int x;' >> test.c
    $CCACHE_COMPILE -c test.c
    expect_stat 'cache hit (direct)' 1
    expect_stat 'cache miss' 2
    expect_stat 'files in cache' 2
    expect_file_count 3 '*' secondary # CACHEDIR.TAG + result + manifest

    # -------------------------------------------------------------------------
    TEST "umask"

    CCACHE_SECONDARY_STORAGE="file://$PWD/secondary|umask=022"
    rm -rf secondary
    $CCACHE_COMPILE -c test.c
    expect_perm secondary drwxr-xr-x
    expect_perm secondary/CACHEDIR.TAG -rw-r--r--

    CCACHE_SECONDARY_STORAGE="file://$PWD/secondary|umask=000"
    $CCACHE -C >/dev/null
    rm -rf secondary
    $CCACHE_COMPILE -c test.c
    expect_perm secondary drwxrwxrwx
    expect_perm secondary/CACHEDIR.TAG -rw-rw-rw-
}
