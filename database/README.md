# Create database with rocksdb or leveldb

## instal Deps
../scripts/install_leveldb.sh
../scripts/install_rocksdb.sh

# Build
make leveldb_createdb

## Create DB
./setup_tmpfs.sh; ./leveldb_createdb
