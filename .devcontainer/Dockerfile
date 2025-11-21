FROM ubuntu:22.04 AS base

ARG VERSION_MAKE=4.3
ARG VERSION_LIB_READLINE=8.1
ARG VERSION_VALGRIND=3.18.1

WORKDIR /root

RUN apt-get update && \
    export DEBIAN_FRONTEND=noninteractive && \
    # Update of the package list and install utilities
    apt-get install --no-install-recommends -y \
    wget \
    git \
    pkg-config \
    binutils \
    ssh \
    netcat \
    zsh \
    libc6-dbg \
    gnupg \
    curl \
    cmake \
    meson \
    build-essential \
    software-properties-common \
    xorg \
    libxext-dev \
    libbsd-dev \
    zlib1g-dev \
    python3 \
    python3-pip \
    python3-venv \
    && \
    # Add LLVM-TOOLCHAIN repositories
    wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - && \
    add-apt-repository "deb http://apt.llvm.org/focal/ llvm-toolchain-focal-12 main" && \
    add-apt-repository ppa:ubuntu-toolchain-r/test && \
    apt-get install --no-install-recommends -y \
    # Install LLVM-TOOLCHAIN and GCC
    clang \
    clang-12 \
    clang-14 \
    clangd \
    gdb \
    lldb-12 \
    irssi \
    netcat \
    meson ninja-build \
    gcc-10=10.5.0-1ubuntu1~22.04.2 \
    g++-10=10.5.0-1ubuntu1~22.04.2 \
    gcc-11=11.4.0-1ubuntu1~22.04.2 \
    g++-11=11.4.0-1ubuntu1~22.04.2

# Changes the symbolic links for LLVM-TOOLCHAIN and GCC (and others)
RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-12 100 \
    && update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-12 100 \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 100 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-10 100 \
    && update-alternatives --install /usr/bin/cc cc /usr/bin/clang-12 100 \
    && update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++-12 100

# Install Starship prompt (non-interactive) and eza from official repo
RUN curl -sS https://starship.rs/install.sh | sh -s -- -y && \
    export DEBIAN_FRONTEND=noninteractive && \
    apt-get update && \
    apt-get install -y gpg && \
    mkdir -p /etc/apt/keyrings && \
    wget -qO- https://raw.githubusercontent.com/eza-community/eza/main/deb.asc | gpg --dearmor -o /etc/apt/keyrings/gierens.gpg && \
    echo "deb [signed-by=/etc/apt/keyrings/gierens.gpg] http://deb.gierens.de stable main" | tee /etc/apt/sources.list.d/gierens.list && \
    chmod 644 /etc/apt/keyrings/gierens.gpg /etc/apt/sources.list.d/gierens.list && \
    apt-get update && \
    apt-get install -y eza

FROM base AS make_build
# Setup and install Make
RUN wget https://ftp.gnu.org/gnu/make/make-${VERSION_MAKE}.tar.gz && \
    tar -xvzf make-${VERSION_MAKE}.tar.gz && \
    cd make-${VERSION_MAKE} && \
    ./configure && \
    make && \
    make install

FROM base AS readline_build
# Setup and install library Readline
RUN wget https://ftp.gnu.org/gnu/readline/readline-${VERSION_LIB_READLINE}.tar.gz && \
    tar -xzvf readline-${VERSION_LIB_READLINE}.tar.gz && \
    cd readline-${VERSION_LIB_READLINE} && \
    ./configure --enable-shared && \
    make && \
    make install

FROM base AS mlx_build
# Setup and install library MLX
RUN git clone https://github.com/42paris/minilibx-linux.git && \
    cd minilibx-linux && \
    make

FROM base AS final
# Copy the built libraries from the previous stages
COPY --from=make_build /usr/local/bin/make /usr/local/bin/make
COPY --from=readline_build /usr/local/lib/libreadline.* /usr/local/bin/readline
COPY --from=mlx_build /root/minilibx-linux/mlx.h /usr/local/include/mlx.h
COPY --from=mlx_build /root/minilibx-linux/libmlx.a /usr/local/lib/libmlx.a

# Run ldconfig to update the dynamic linker run-time bindings
RUN ldconfig

# Setup and install Valgrind
RUN wget https://sourceware.org/pub/valgrind/valgrind-${VERSION_VALGRIND}.tar.bz2 && \
    tar -xjf valgrind-${VERSION_VALGRIND}.tar.bz2 && \
    cd valgrind-${VERSION_VALGRIND} && \
    ./configure && \
    make && \
    make install && \
    ldconfig && \
    cd .. && \
    rm -rf valgrind-${VERSION_VALGRIND}*

# Setup virtual env python and install norminette
RUN python3 -m venv /opt/venv && \
    chown -R $USER:$USER /opt/venv
ENV PATH="$PATH:/opt/venv/bin"
RUN python3 -m pip install --upgrade pip setuptools && \
    python3 -m pip install norminette

# Shell enhancements: zoxide, Starship, eza, Oh My Zsh
RUN apt-get update && apt-get install --no-install-recommends -y zoxide && rm -rf /var/lib/apt/lists/*
RUN curl -fsSL https://starship.rs/install.sh | sh -s -- -y && \
    curl -fsSL https://raw.githubusercontent.com/eza-community/eza/main/install.sh | sh && \
    ZSH="/root/.oh-my-zsh" RUNZSH=no CHSH=no KEEP_ZSHRC=yes sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)" && \
    if [ -f /root/.oh-my-zsh/templates/zshrc.zsh-template ]; then cp /root/.oh-my-zsh/templates/zshrc.zsh-template /root/.zshrc; fi && \
    sed -i 's|^ZSH_THEME=.*|ZSH_THEME="robbyrussell"|' /root/.zshrc || true && \
    grep -q 'plugins=' /root/.zshrc || echo 'plugins=(git)' >> /root/.zshrc && \
    printf '\n# Initialize zoxide\n' >> /root/.zshrc && \
    printf 'eval "$(zoxide init zsh)"\n' >> /root/.zshrc && \
    printf '\n# Initialize starship prompt\n' >> /root/.zshrc && \
    printf 'eval "$(starship init zsh)"\n' >> /root/.zshrc && \
    printf '\n# eza aliases\n' >> /root/.zshrc && \
    printf "alias ls='eza --icons=auto'\n" >> /root/.zshrc && \
    printf "alias ll='eza -lh --group-directories-first --icons=auto'\n" >> /root/.zshrc && \
    printf "alias la='eza -la --group-directories-first --icons=auto'\n" >> /root/.zshrc


WORKDIR /app

CMD ["/bin/zsh"]
