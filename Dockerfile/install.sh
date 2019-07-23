# Script to build llvm-6.0 developer image

sudo apt-get update -y
sudo apt-get install wget -y

# Update repos
sudo apt-get update -y

# Upgrade and install
sudo apt-get upgrade -y
sudo  apt-get install -y git man curl build-essential cmake emacs-nox screen dos2unix vim python2.7 python-matplotlib clang-6.0 libclang-common-6.0-dev libclang-6.0-dev libclang1-6.0 libclang1-6.0-dbg libllvm6.0 libllvm6.0-dbg lldb-6.0 llvm-6.0 llvm-6.0-dev llvm-6.0-runtime clang-format-6.0 python-clang-6.0 lld-6.0 g++-multilib gdb
