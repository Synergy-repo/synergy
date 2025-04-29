
LIBPG_QUERY_DIR = ../../deps/libpg_query
LIBSQL_PARSER_DIR = ../../deps/sql-parser
LIBRESP_DIR = ../../deps/resp.rs

#common flags
CFLAGS += -Wno-unused-parameter
CFLAGS += -Wall -Wextra -O2
LDFLAGS = -pthread

PROGS = libpg_query_bench \
		sql_parser_bench \
		resp_bench

.PHONY: all clean

ALL: $(PROGS)

resp_bench: resp_bench.rs
	rustc $< -o $@ --crate-type bin -L $(LIBRESP_DIR)/target/release/ -C opt-level=2

sql_parser_bench: sql_parser_bench.o
	$(CXX) $^ -o $@ $(LDFLAGS) -L $(LIBSQL_PARSER_DIR)/ -lsqlparser -std=c++1z -lstdc++

sql_parser_bench.o: sql_parser_bench.cc
	$(CXX) -c $^ -o $@ $(CFLAGS) -I $(LIBSQL_PARSER_DIR)/src -std=c++1z -lstdc++

libpg_query_bench: libpg_query_bench.o
	$(CC) $^ -o $@ $(LDFLAGS) -L $(LIBPG_QUERY_DIR)/ -lpg_query -pthread

libpg_query_bench.o: libpg_query_bench.c
	$(CC) -c $^ -o $@ $(CFLAGS) -I $(LIBPG_QUERY_DIR)/

clean:
	rm ./*.o $(PROGS)

