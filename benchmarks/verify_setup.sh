#!/bin/bash
# Benchmark 环境验证脚本

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}LuaJIT Rule Engine Benchmark 环境验证${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""

# 检查 LuaJIT
echo -e "${YELLOW}检查 LuaJIT...${NC}"
if [ -d "/usr/local/3rd/luajit-2.1.0-beta3" ]; then
    echo -e "${GREEN}✓ LuaJIT 已安装在 /usr/local/3rd/luajit-2.1.0-beta3${NC}"
else
    echo -e "${RED}✗ LuaJIT 未找到${NC}"
    echo "  请安装 LuaJIT 到 /usr/local/3rd/luajit-2.1.0-beta3"
    echo "  或使用 -DLUAJIT_ROOT 指定路径"
fi
echo ""

# 检查 nlohmann/json
echo -e "${YELLOW}检查 nlohmann/json...${NC}"
if [ -f "third-party/json/single_include/nlohmann/json.hpp" ]; then
    echo -e "${GREEN}✓ nlohmann/json 已找到${NC}"
else
    echo -e "${RED}✗ nlohmann/json 未找到${NC}"
    echo "  请确保 third-party/json/single_include/nlohmann/json.hpp 存在"
fi
echo ""

# 检查 Google Benchmark
echo -e "${YELLOW}检查 Google Benchmark...${NC}"
if pkg-config --exists benchmark; then
    VERSION=$(pkg-config --modversion benchmark)
    echo -e "${GREEN}✓ Google Benchmark 已安装: $VERSION${NC}"
else
    echo -e "${YELLOW}⚠ Google Benchmark 未安装${NC}"
    echo "  将使用 FetchContent 自动下载"
    echo "  或手动安装: sudo apt-get install libbenchmark-dev"
fi
echo ""

# 检查 CMake
echo -e "${YELLOW}检查 CMake...${NC}"
if command -v cmake &> /dev/null; then
    CMAKE_VERSION=$(cmake --version | head -n1 | awk '{print $3}')
    echo -e "${GREEN}✓ CMake 已安装: $CMAKE_VERSION${NC}"

    # 检查 CMake 版本是否满足要求
    if [ "$(printf '%s\n' "3.15" "$CMAKE_VERSION" | sort -V | head -n1)" = "3.15" ]; then
        echo -e "${GREEN}  CMake 版本满足要求 (>= 3.15)${NC}"
    else
        echo -e "${RED}  CMake 版本过低，需要 >= 3.15${NC}"
    fi
else
    echo -e "${RED}✗ CMake 未安装${NC}"
    echo "  请安装 CMake: sudo apt-get install cmake"
fi
echo ""

# 检查编译器
echo -e "${YELLOW}检查编译器...${NC}"
if command -v g++ &> /dev/null; then
    GCC_VERSION=$(g++ --version | head -n1)
    echo -e "${GREEN}✓ GCC 已安装: $GCC_VERSION${NC}"
elif command -v clang++ &> /dev/null; then
    CLANG_VERSION=$(clang++ --version | head -n1)
    echo -e "${GREEN}✓ Clang 已安装: $CLANG_VERSION${NC}"
else
    echo -e "${RED}✗ 未找到 C++ 编译器${NC}"
    echo "  请安装 GCC 或 Clang: sudo apt-get install g++"
fi
echo ""

# 检查 Python (可选)
echo -e "${YELLOW}检查 Python (可选)...${NC}"
if command -v python3 &> /dev/null; then
    PYTHON_VERSION=$(python3 --version)
    echo -e "${GREEN}✓ Python 已安装: $PYTHON_VERSION${NC}"
else
    echo -e "${YELLOW}⚠ Python 未安装（可选，用于生成测试报告）${NC}"
fi
echo ""

# 检查 CPU 性能模式
echo -e "${YELLOW}检查 CPU 频率模式...${NC}"
if command -v cpupower &> /dev/null; then
    GOVERNOR=$(cpupower frequency-info -p | grep "governor" | awk '{print $3}')
    if [ "$GOVERNOR" = "performance" ]; then
        echo -e "${GREEN}✓ CPU 已设置为性能模式${NC}"
    else
        echo -e "${YELLOW}⚠ CPU 当前模式: $GOVERNOR${NC}"
        echo "  建议设置为性能模式以获得更稳定的测试结果:"
        echo "  sudo cpupower frequency-set -g performance"
    fi
else
    echo -e "${YELLOW}⚠ cpupower 未安装，无法检查 CPU 模式${NC}"
fi
echo ""

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}环境验证完成！${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""

# 提供下一步建议
echo -e "${YELLOW}下一步操作:${NC}"
echo "1. 编译项目:"
echo "   mkdir build && cd build"
echo "   cmake .. -DCMAKE_BUILD_TYPE=Release \\"
echo "            -DLUAJIT_ROOT=/usr/local/3rd/luajit-2.1.0-beta3 \\"
echo "            -DBUILD_BENCHMARKS=ON"
echo "   make -j\$(nproc)"
echo ""
echo "2. 运行测试:"
echo "   cd build"
echo "   ./benchmarks/basic_benchmark"
echo ""
echo "3. 或使用快速脚本:"
echo "   ./benchmarks/run_benchmarks.sh --all"
echo ""
