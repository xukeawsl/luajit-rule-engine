#!/bin/bash
# LuaJIT Rule Engine 测试脚本
# 用于快速运行测试和生成覆盖率报告

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 默认参数
BUILD_DIR="build"
COVERAGE=false
VERBOSE=false
FILTER=""
PARALLEL=$(nproc)

# 帮助信息
usage() {
    cat << EOF
用法: $0 [选项]

选项:
    -c, --coverage        生成代码覆盖率报告
    -v, --verbose         显示详细输出
    -f, --filter PATTERN  只运行匹配模式的测试
    -j, --jobs N          并行运行 N 个测试 (默认: $(nproc))
    -h, --help           显示此帮助信息

示例:
    $0                           # 运行所有测试
    $0 -v                        # 显示详细输出
    $0 -c                        # 运行测试并生成覆盖率报告
    $0 -f "lua_state"            # 只运行 lua_state 测试
    $0 -c -v                     # 生成覆盖率报告并显示详细输出

EOF
    exit 0
}

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        -c|--coverage)
            COVERAGE=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -f|--filter)
            FILTER="$2"
            shift 2
            ;;
        -j|--jobs)
            PARALLEL="$2"
            shift 2
            ;;
        -h|--help)
            usage
            ;;
        *)
            echo -e "${RED}未知选项: $1${NC}"
            usage
            ;;
    esac
done

# 检查是否在项目根目录
if [ ! -f "CMakeLists.txt" ]; then
    echo -e "${RED}错误: 请在项目根目录下运行此脚本${NC}"
    exit 1
fi

# 创建构建目录
echo -e "${BLUE}=== 创建构建目录 ===${NC}"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 配置 CMake
echo -e "${BLUE}=== 配置 CMake ===${NC}"
CMAKE_ARGS="-DLUAJIT_ROOT=/usr/local/3rd/luajit-2.1.0-beta3"
if [ "$COVERAGE" = true ]; then
    CMAKE_ARGS="$CMAKE_ARGS -DBUILD_COVERAGE=ON"
fi

cmake .. $CMAKE_ARGS

# 编译
echo -e "${BLUE}=== 编译项目 ===${NC}"
make -j$(nproc)

# 运行测试
echo -e "${BLUE}=== 运行测试 ===${NC}"
cd tests

CTEST_ARGS="-j$PARALLEL"
if [ "$VERBOSE" = true ]; then
    CTEST_ARGS="$CTEST_ARGS --verbose"
else
    CTEST_ARGS="$CTEST_ARGS --output-on-failure"
fi

if [ -n "$FILTER" ]; then
    CTEST_ARGS="$CTEST_ARGS -R $FILTER"
fi

ctest $CTEST_ARGS

# 显示测试结果
if [ $? -eq 0 ]; then
    echo -e "\n${GREEN}=== 所有测试通过 ===${NC}"
else
    echo -e "\n${RED}=== 部分测试失败 ===${NC}"
    exit 1
fi

# 生成覆盖率报告
if [ "$COVERAGE" = true ]; then
    echo -e "\n${BLUE}=== 生成覆盖率报告 ===${NC}"

    cd ..

    # 检查 lcov 是否安装
    if ! command -v lcov &> /dev/null; then
        echo -e "${YELLOW}警告: lcov 未安装，无法生成覆盖率报告${NC}"
        echo -e "${YELLOW}请安装: sudo apt-get install lcov (Ubuntu/Debian)${NC}"
        exit 0
    fi

    # 捕获覆盖率数据
    echo -e "${BLUE}捕获覆盖率数据...${NC}"
    lcov --capture --directory . --output-file coverage.info --quiet

    # 过滤掉不需要的文件
    echo -e "${BLUE}过滤覆盖率数据...${NC}"
    lcov --remove coverage.info '/usr/*' \
         --remove coverage.info 'third-party/*' \
         --remove coverage.info 'tests/*' \
         --output-file coverage_filtered.info --quiet

    # 生成 HTML 报告
    if command -v genhtml &> /dev/null; then
        echo -e "${BLUE}生成 HTML 报告...${NC}"
        genhtml coverage_filtered.info --output-directory coverage_html --quiet

        echo -e "${GREEN}覆盖率报告已生成: $BUILD_DIR/coverage_html/index.html${NC}"

        # 显示覆盖率摘要
        echo -e "\n${BLUE}=== 覆盖率摘要 ===${NC}"
        lcov --summary coverage_filtered.info
    else
        echo -e "${YELLOW}警告: genhtml 未安装，无法生成 HTML 报告${NC}"
        echo -e "${YELLOW}请安装: sudo apt-get install lcov (Ubuntu/Debian)${NC}"
    fi
fi

echo -e "\n${GREEN}=== 完成 ===${NC}"
