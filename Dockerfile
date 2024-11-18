# docker build -t <proj_name> .
# docker run -it --rm -v "$(pwd):/home/ubuntu" -e <env_var_name>=<env_var_val> -p 8000:8000 <proj_name> /bin/bash

# Use an official Ubuntu as a base image
FROM ubuntu:latest

# Install necessary packages
RUN apt-get update && apt-get install -y \
    software-properties-common \
    build-essential \
    wget \
    gnupg2 \
    cmake \ 
    libssl-dev

# Add GCC repository and install GCC 13.2.0
RUN add-apt-repository ppa:ubuntu-toolchain-r/test && \
    apt-get update && \
    apt-get install -y gcc-13 g++-13

# Set GCC 13 as default
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100

# Copy the contents of the local repository to /ubuntu/home in the image
COPY . /home/ubuntu

EXPOSE 443

# Set working directory
WORKDIR /home/ubuntu/

# Verify GCC version
RUN gcc --version

