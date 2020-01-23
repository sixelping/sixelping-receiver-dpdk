# How to build dpdk 19.11 on debian 10

Notes from building dpdk on debian 10

```sh
git clone https://salsa.debian.org/debian/dpdk.git
cd dpdk
git checkout debian/19.11-2
gbp buildpackage --git-dist=buster --git-export-dir=../build-area --git-debian-branch=debian/19.11-2 --git-upstream-tree=upstream -uc -us --git-ignore-branch
```
