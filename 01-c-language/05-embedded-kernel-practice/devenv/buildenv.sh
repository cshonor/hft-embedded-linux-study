# Embedded C 开发环境 — source 本文件，勿 bash 执行
# 用法: source /path/to/devenv/buildenv.sh

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    echo "请使用: source $(basename "${BASH_SOURCE[0]}")" >&2
    exit 1
fi

export PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PATH="${PROJECT_ROOT}/scripts:${PATH}"

# 默认本地 gcc；交叉编译时取消注释并改前缀
# export CROSS_COMPILE=arm-linux-gnueabihf-
export CC="${CROSS_COMPILE}gcc"
export AR="${CROSS_COMPILE}ar"
export OBJDUMP="${CROSS_COMPILE}objdump"
export GDB="${CROSS_COMPILE}gdb"

export CFLAGS="${CFLAGS:--Wall -Wextra -std=c11 -g}"
export LDFLAGS="${LDFLAGS:-}"

build() {
    local dir="${1:-.}"
    make -C "${dir}" all
}

clean() {
    local dir="${1:-.}"
    make -C "${dir}" clean
}

echo "[buildenv] PROJECT_ROOT=${PROJECT_ROOT}"
echo "[buildenv] CC=${CC}"
