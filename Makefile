
DB_PATH=/tmpfs/my_db
DATABASE=rocksdb

.PHONY: *

# build synergy library and shim library
synergy:
	$(MAKE) -C ./src
	$(MAKE) -C ./shim

clean:
	$(MAKE) -C ./src clean
	$(MAKE) -C ./shim clean

database:
	$(MAKE) -C ./database
	sudo rm -rf $(DB_PATH)
	./database/$(DATABASE)_createdb

database-clean:
	$(MAKE) -C ./database clean

setup:
	./scripts/setup.sh

submodules: dpdk concord fakework rocksdb

submodules-clean: dpdk-clean fakework-clean

dune:
	./scripts/install_dune.sh

shinjuku_dune:
	./scripts/install_shinjuku_dune.sh

kmod_ipi:
	./kmod/load_ipi.sh

dpdk:
	./scripts/install_dpdk.sh

dpdk-clean:
	./scripts/install_dpdk.sh clean

leveldb:
	./scripts/install_concord_leveldb.sh
	./scripts/install_leveldb.sh

rocksdb:
	./scripts/install_rocksdb.sh

rocksdb-clean:
	./scripts/install_rocksdb.sh clean

concord:
	./scripts/install_concord.sh

fakework:
	$(MAKE) -C ./deps/fake-work

fakework-clean:
	$(MAKE) clean -C ./deps/fake-work
