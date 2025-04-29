#!/bin/bash

LIBPG_QUERY_PATH=$(dirname $0)"/../../deps/libpg_query"
SQL_PARSER_PATH=$(dirname $0)"/../../deps/sql-parser"
LIBRESP_PATH=$(dirname $0)"../../deps/resp.rs/"

git submodule update --init $LIBPG_QUERY_PATH

git submodule update --init $SQL_PARSER_PATH

git submodule update --init $LIBRESP_PATH
pushd $LIBRESP_PATH
cargo build --release
popd



