start_http_server() {
    local HOST="127.0.0.1"
    local PORT="8080"
    local URL="http://${HOST}:${PORT}/"
    export CCACHE_SECONDARY_STORAGE="${URL}"

    trap 'terminate_all_children' EXIT # maybe move to global run

    mkdir "secondary"
    pushd "secondary" >/dev/null
    echo "=== TEST ${CURRENT_TEST} ===" >> $ABS_TESTDIR/http-server.log
    "$HTTP_SERVER" --bind "${HOST}" "${PORT}" >> $ABS_TESTDIR/http-server.log 2>&1 &
    popd >/dev/null

    echo "=== TEST ${CURRENT_TEST} ===" >> $ABS_TESTDIR/http-client.log
    "$HTTP_CLIENT" "${URL}" >> $ABS_TESTDIR/http-client.log 2>&1 || test_failed_internal "Cannot connect to http server"
}

SUITE_secondary_http_PROBE() {
    if ! "$HTTP_SERVER" --help >/dev/null 2>&1; then
        echo "Cannot execute ($HTTP_SERVER). Python 3 might be missing installed"
    fi
}

SUITE_secondary_http_SETUP() {
    unset CCACHE_NODIRECT

    start_http_server

    generate_code 1 test.c
}

SUITE_secondary_http_TEARDOWN() {
    terminate_all_children
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

    # -------------------------------------------------------------------------
    TEST "Read-only"

    $CCACHE_COMPILE -c test.c
    expect_stat 'cache hit (direct)' 0
    expect_stat 'cache miss' 1
    expect_stat 'files in cache' 2
    expect_file_count 2 '*' secondary # result + manifest

    $CCACHE -C >/dev/null
    expect_stat 'files in cache' 0
    expect_file_count 2 '*' secondary # result + manifest

    CCACHE_SECONDARY_STORAGE+="|read-only"

    $CCACHE_COMPILE -c test.c
    expect_stat 'cache hit (direct)' 1
    expect_stat 'cache miss' 1
    expect_stat 'files in cache' 0
    expect_file_count 2 '*' secondary # result + manifest

    echo 'int x;' >> test.c
    $CCACHE_COMPILE -c test.c
    expect_stat 'cache hit (direct)' 1
    expect_stat 'cache miss' 2
    expect_stat 'files in cache' 2
    expect_file_count 2 '*' secondary # result + manifest
}
