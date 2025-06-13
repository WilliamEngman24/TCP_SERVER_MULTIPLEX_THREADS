FROM mcr.microsoft.com/devcontainers/cpp:0-debian-11

# Install network capture tools and development libraries
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends \
    build-essential \
    cmake \
    gdb \
    valgrind \
    clang \
    clang-format \
    clang-tidy \
    libssl-dev \
    libboost-all-dev \
    tcpdump \
    netcat \
    net-tools \
    iputils-ping \
    traceroute \
    curl \
    wget \
    nmap \
    dnsutils \
    iperf3 \
    ethtool \
    iproute2 \
    libmosquitto-dev \
    nlohmann-json3-dev \
    libspdlog-dev \
    git

# Install version control tools
RUN apt-get -y install --no-install-recommends \
    git \
    git-lfs

# Install project-specific build tools
RUN apt-get -y install --no-install-recommends \
    pkg-config \
    autoconf \
    automake \
    libtool \
    # Cleanup
    && apt-get autoremove -y \
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/*

# Create a non-root user for the container
ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=$USER_UID

# Add sudo support
RUN apt-get update \
    && apt-get install -y sudo \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME

# Add tcpdump to sudoers with NOPASSWD for specific command
RUN echo "$USERNAME ALL=(root) NOPASSWD: /usr/bin/tcpdump" >> /etc/sudoers.d/$USERNAME

# Set up a capture directory with proper permissions
RUN mkdir -p /workspaces/captures && \
    chown $USERNAME:$USERNAME /workspaces/captures

# Create helper script for packet captures
RUN echo '#!/bin/bash\n\
# Simple script to capture network traffic\n\
FILENAME="/workspaces/captures/capture-$(date +%Y%m%d-%H%M%S).pcap"\n\
echo "Starting packet capture. Press Ctrl+C to stop."\n\
echo "Capture will be saved to: $FILENAME"\n\
echo "After capture, download the file from the captures folder to analyze locally in Wireshark"\n\
sudo tcpdump -i any -w "$FILENAME" $@\n\
echo "\nCapture saved to: $FILENAME"\n\
' > /usr/local/bin/capture-packets && \
    chmod +x /usr/local/bin/capture-packets

# Set default user
USER $USERNAME

# Set up git configuration
RUN git config --global pull.rebase false
