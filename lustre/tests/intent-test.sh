#!/bin/bash -x

OST=2
MDS=3

mkdir /mnt/lustre/foo
mkdir /mnt/lustre/foo2

mkdir /mnt/lustre/foo

umount /mnt/lustre
mount -t lustre_lite -o ost=$OST,mds=$MDS none /mnt/lustre

mkdir /mnt/lustre/foo

umount /mnt/lustre
mount -t lustre_lite -o ost=$OST,mds=$MDS none /mnt/lustre

./mcreate /mnt/lustre/bar
./mcreate /mnt/lustre/bar

umount /mnt/lustre
mount -t lustre_lite -o ost=$OST,mds=$MDS none /mnt/lustre

./mcreate /mnt/lustre/bar

umount /mnt/lustre
mount -t lustre_lite -o ost=$OST,mds=$MDS none /mnt/lustre

ls -l /mnt/lustre/bar

umount /mnt/lustre
mount -t lustre_lite -o ost=$OST,mds=$MDS none /mnt/lustre

cat /mnt/lustre/bar
./mcreate /mnt/lustre/bar2
cat /mnt/lustre/bar2
./mcreate /mnt/lustre/bar3

umount /mnt/lustre
mount -t lustre_lite -o ost=$OST,mds=$MDS none /mnt/lustre

./tchmod 777 /mnt/lustre/bar3

umount /mnt/lustre
mount -t lustre_lite -o ost=$OST,mds=$MDS none /mnt/lustre

./mcreate /mnt/lustre/bar4
./tchmod 777 /mnt/lustre/bar4

umount /mnt/lustre
mount -t lustre_lite -o ost=$OST,mds=$MDS none /mnt/lustre

ls -l /mnt/lustre/bar4
./tchmod 777 /mnt/lustre/bar4

umount /mnt/lustre
mount -t lustre_lite -o ost=$OST,mds=$MDS none /mnt/lustre

cat /mnt/lustre/bar4
./tchmod 777 /mnt/lustre/bar4

umount /mnt/lustre
mount -t lustre_lite -o ost=$OST,mds=$MDS none /mnt/lustre

touch /mnt/lustre/bar5
touch /mnt/lustre/bar6
touch /mnt/lustre/bar5

umount /mnt/lustre
mount -t lustre_lite -o ost=$OST,mds=$MDS none /mnt/lustre

touch /mnt/lustre/bar5

umount /mnt/lustre
mount -t lustre_lite -o ost=$OST,mds=$MDS none /mnt/lustre

echo "ready debugger"
read

echo foo >> /mnt/lustre/bar
cat /mnt/lustre/bar

umount /mnt/lustre
mount -t lustre_lite -o ost=$OST,mds=$MDS none /mnt/lustre

cat /mnt/lustre/bar

exit;

echo foo >> /mnt/lustre/iotest
echo bar >> /mnt/lustre/iotest
cat /mnt/lustre/iotest

umount /mnt/lustre
mount -t lustre_lite -o ost=$OST,mds=$MDS none /mnt/lustre

cat /mnt/lustre/iotest
echo baz >> /mnt/lustre/iotest

umount /mnt/lustre
mount -t lustre_lite -o ost=$OST,mds=$MDS none /mnt/lustre

ls /mnt/lustre

umount /mnt/lustre
mount -t lustre_lite -o ost=$OST,mds=$MDS none /mnt/lustre

mkdir /mnt/lustre/new
ls /mnt/lustre

umount /mnt/lustre
mount -t lustre_lite -o ost=$OST,mds=$MDS none /mnt/lustre

ls /mnt/lustre
mkdir /mnt/lustre/newer
ls /mnt/lustre

umount /mnt/lustre
mount -t lustre_lite -o ost=$OST,mds=$MDS none /mnt/lustre

cat /mnt/lustre/iotest