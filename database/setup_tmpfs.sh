sudo mkdir -p /tmpfs
mountpoint -q /tmpfs || sudo mount -t tmpfs -o size=50G,mode=1777 tmpfs /tmpfs
#mkdir -p /tmpfs/experiments/
