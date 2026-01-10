#!/bin/bash
# 在 Ubuntu 上查看覆盖率报告的快捷脚本
# 使用 Python HTTP 服务器查看覆盖率报告

# 默认端口
PORT=${1:-8000}

cd build

# 检查覆盖率报告是否已生成
if [ ! -f "coverage_html/index.html" ]; then
    echo "📊 覆盖率报告不存在，正在生成..."
    lcov --capture --directory . --output-file coverage_all.info 2>&1 | grep -E "(Found|Capturing|Finished)"

    echo "过滤第三方库和测试代码..."

    # 过滤掉不需要的文件，只保留 src/ 和 include/ljre/
    # 注意：路径需要匹配完整的绝对路径
    lcov --remove coverage_all.info '/usr/*' --output-file coverage.info 2>&1 | tail -1
    lcov --remove coverage.info '*/third-party/*' --output-file coverage.info 2>&1 | tail -1
    lcov --remove coverage.info '*/tests/*' --output-file coverage.info 2>&1 | tail -1

    echo "生成 HTML 报告..."
    genhtml coverage.info --output-directory coverage_html 2>&1 | tail -3
    echo ""
fi

echo ""
echo "=== 覆盖率摘要 ==="
lcov --summary coverage.info | grep -E "(lines|functions|branches)"

echo ""
echo "=== 启动 HTTP 服务器 ==="
echo "🌐 服务器地址: http://localhost:$PORT"
echo "📁 报告目录: $(pwd)/coverage_html"
echo ""
echo "在浏览器中打开上面的地址，或访问: http://localhost:$PORT"
echo ""
echo "按 Ctrl+C 停止服务器"
echo ""

# 启动 Python HTTP 服务器
cd coverage_html

# 尝试使用 python3，如果不存在则尝试 python
if command -v python3 &> /dev/null; then
    python3 -m http.server $PORT
elif command -v python &> /dev/null; then
    python -m SimpleHTTPServer $PORT
else
    echo "❌ 错误: 未找到 Python"
    echo "请安装 Python: sudo apt-get install python3"
    exit 1
fi
