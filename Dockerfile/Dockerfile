# Dockerfile to build llvm-10 developer image
FROM ubuntu:latest

# Grab wget, so that we can add the LLVM PPA
RUN apt-get update -y
RUN apt-get install wget gnupg gnupg2 gnupg1 -y

# Add PPA for LLVM
RUN echo deb http://apt.llvm.org/focal/ llvm-toolchain-focal-10 main >> /etc/apt/sources.list 
RUN echo deb-src http://apt.llvm.org/focal/ llvm-toolchain-focal-10 main >> /etc/apt/sources.list
RUN wget -qO - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - 

# Upgrade / update the whole system, and pull from the new PPAs
RUN apt-get update -y
RUN apt-get upgrade -y

# Install a large set of build tools, including gcc and llvm
RUN DEBIAN_FRONTEND=noninteractive apt install -y gnupg2 gnupg gnupg1 git man curl build-essential cmake emacs-nox screen dos2unix vim g++-multilib gdb libllvm10 llvm-10 llvm-10-dev llvm-10-doc llvm-10-examples llvm-10-runtime clang-10 clang-tools-10 clang-10-doc libclang-common-10-dev libclang-10-dev libclang1-10 clang-format-10 clangd-10 libfuzzer-10-dev lldb-10 lld-10 libc++-10-dev libc++abi-10-dev libomp-10-dev libmemcached-tools libevent-dev

# Set the working directory
WORKDIR /root

# To use this Dockerfile
# 1 - Make an image named llvm-10
#     - Go to the folder where this Dockerfile exists
#     - sudo docker build -t llvm-10 .
#       - Note: you don't need the 'sudo' part on Windows
# 2 - Set up a working folder on your host machine
#     - mkdir my-llvm-work
# 3 - Launch an interactive container, and mount your working folder
#     - docker run --privileged --rm -v /path/to/my-llvm-work:/root -it llvm-10
