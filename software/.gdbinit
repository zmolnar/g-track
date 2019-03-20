file build/gtrack.elf
target extended-remote /dev/ttyBmpGdb
monitor connect_srst enable
monitor swdp_scan
attach 1
