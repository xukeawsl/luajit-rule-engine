#!/bin/bash
# Benchmark 快速运行脚本

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 打印帮助信息
print_help() {
    echo "LuaJIT Rule Engine Benchmark 运行脚本"
    echo ""
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  --all                  运行所有测试"
    echo "  --basic                只运行基准测试"
    echo "  --stress               只运行压力测试"
    echo "  --comparison           只运行对比测试"
    echo "  --scaling              只运行扩展性测试"
    echo "  --min-time=SECONDS     设置最小运行时间（默认 30 秒）"
    echo "  --repetitions=N        重复运行次数（默认 1 次）"
    echo "  --format=FORMAT        输出格式：console, json（默认 console）"
    echo "  --output=DIR           结果输出目录（默认 benchmarks/results）"
    echo "  --help                 显示此帮助信息"
    echo ""
    echo "示例:"
    echo "  $0 --all"
    echo "  $0 --basic --min-time=60"
    echo "  $0 --all --format=json --repetitions=5"
}

# 默认参数
MIN_TIME=30
REPETITIONS=1
FORMAT="console"
OUTPUT_DIR="benchmarks/results"
RUN_ALL=false
RUN_BASIC=false
RUN_STRESS=false
RUN_COMPARISON=false
RUN_SCALING=false

# 解析参数
while [[ $# -gt 0 ]]; do
    case $1 in
        --all)
            RUN_ALL=true
            shift
            ;;
        --basic)
            RUN_BASIC=true
            shift
            ;;
        --stress)
            RUN_STRESS=true
            shift
            ;;
        --comparison)
            RUN_COMPARISON=true
            shift
            ;;
        --scaling)
            RUN_SCALING=true
            shift
            ;;
        --min-time=*)
            MIN_TIME="${1#*=}"
            shift
            ;;
        --repetitions=*)
            REPETITIONS="${1#*=}"
            shift
            ;;
        --format=*)
            FORMAT="${1#*=}"
            shift
            ;;
        --output=*)
            OUTPUT_DIR="${1#*=}"
            shift
            ;;
        --help)
            print_help
            exit 0
            ;;
        *)
            echo -e "${RED}未知选项: $1${NC}"
            print_help
            exit 1
            ;;
    esac
done

# 如果没有指定任何测试，默认运行所有测试
if [ "$RUN_ALL" = false ] && [ "$RUN_BASIC" = false ] && [ "$RUN_STRESS" = false ] && [ "$RUN_COMPARISON" = false ] && [ "$RUN_SCALING" = false ]; then
    RUN_ALL=true
fi

# 检查是否在 build 目录
if [ ! -f "benchmarks/basic_benchmark" ]; then
    echo -e "${RED}错误: 未找到 benchmark 可执行文件${NC}"
    echo "请先在 build 目录中编译："
    echo "  mkdir build && cd build"
    echo "  cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_BENCHMARKS=ON"
    echo "  make -j\$(nproc)"
    exit 1
fi

# 创建输出目录
mkdir -p "$OUTPUT_DIR"

# 获取时间戳
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# 构建通用参数
COMMON_ARGS="--benchmark_min_time=$MIN_TIME --benchmark_repetitions=$REPETITIONS"

# 运行测试函数
run_benchmark() {
    local name=$1
    local executable=$2

    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}运行 $name${NC}"
    echo -e "${GREEN}========================================${NC}"

    if [ "$FORMAT" = "json" ]; then
        local output_file="$OUTPUT_DIR/${name}_${TIMESTAMP}.json"
        echo -e "${YELLOW}输出文件: $output_file${NC}"
        $executable $COMMON_ARGS --benchmark_format=json > "$output_file"
        echo -e "${GREEN}✓ 结果已保存到: $output_file${NC}"
    else
        $executable $COMMON_ARGS
    fi
    echo ""
}

# 执行测试
if [ "$RUN_ALL" = true ] || [ "$RUN_BASIC" = true ]; then
    run_benchmark "basic_benchmark" "benchmarks/basic_benchmark"
fi

if [ "$RUN_ALL" = true ] || [ "$RUN_STRESS" = true ]; then
    run_benchmark "stress_benchmark" "benchmarks/stress_benchmark"
fi

if [ "$RUN_ALL" = true ] || [ "$RUN_COMPARISON" = true ]; then
    run_benchmark "comparison_benchmark" "benchmarks/comparison_benchmark"
fi

if [ "$RUN_ALL" = true ] || [ "$RUN_SCALING" = true ]; then
    run_benchmark "scaling_benchmark" "benchmarks/scaling_benchmark"
fi

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}所有测试完成！${NC}"
echo -e "${GREEN}========================================${NC}"

if [ "$FORMAT" = "json" ]; then
    echo -e "结果已保存到: $OUTPUT_DIR/"
    echo -e "使用以下命令查看 JSON 结果："
    echo -e "  cat $OUTPUT_DIR/*_${TIMESTAMP}.json | jq ."
fi
