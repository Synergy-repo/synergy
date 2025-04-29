
# synergy

See [all](https://github.com/Synergy-repo/all) repository to run paper experiments.

## Build library
```sh
./install_requirements.sh
make dpdk
make
```

## Build apps
```bash
make concord

# To fake work
make fakework
make -C apps/fake/  # build apps

# To levelDB
make leveldb
make -C apps/leveldb/  # build apps
```

## Machine setup
```sh
# edit MACHINE_CONFIG or copy CLOUDLAB_CONFIG example
./scripts/set_boot_params.sh
reboot
./scripts/setup.sh
```

## Run
```sh
make run -C apps/fake/     # run fake work
make run -C apps/leveldb/  # run levelDB server
```

## Dirs
* **apps:** applications than make use of synergy library
* **database:** programs to generate database for tests
* **deps:** dependencys
* **kmod:** synergy kernel module
* **scripts:** general scripts
* **shim:** shim layer to intercept memory operations and locks
* **src:** synergy library source code
* **tests:** general tests
