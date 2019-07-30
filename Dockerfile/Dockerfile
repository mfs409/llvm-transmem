# Dockerfile to build llvm-6.0 developer image

FROM ubuntu

MAINTAINER Michael Spear (spear@cse.lehigh.edu)

# Initialize repos
RUN apt-get update -y

# Grab wget
RUN apt-get install wget -y

# Update repos
RUN apt-get update -y

# Upgrade and install
RUN apt-get upgrade -y
RUN apt-get install -y git man curl build-essential cmake emacs-nox screen dos2unix vim python2.7 python-matplotlib clang-6.0 libclang-common-6.0-dev libclang-6.0-dev libclang1-6.0 libclang1-6.0-dbg libllvm6.0 libllvm6.0-dbg lldb-6.0 llvm-6.0 llvm-6.0-dev llvm-6.0-runtime clang-format-6.0 python-clang-6.0 lld-6.0 g++-multilib gdb libevent-dev

# To use this Dockerfile
# 1 - Make an image named llvm-6.0
#     - Go to the folder where this Dockerfile exists
#     - sudo docker build -t llvm-6.0 .
#       - Note: you don't need the 'sudo' part on Windows
# 2 - Set up a working folder on your host machine
#     - mkdir my-llvm-work
# 3 - Launch an interactive container, and mount your working folder
#     - docker run --privileged --name llvm-dev -v /path/to/my-llvm-work:/root -it llvm-6.0
# 4 - That's it. You should have an interactive shell to your container
