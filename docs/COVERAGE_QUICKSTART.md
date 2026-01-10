# 覆盖率查看快速指南

## 🚀 一键启动（推荐）

```bash
# 使用默认端口 8000
./view_coverage.sh

# 使用自定义端口（例如 9000）
./view_coverage.sh 9000
```

脚本会自动：
1. ✅ 生成覆盖率报告（如果不存在）
2. ✅ 显示覆盖率摘要
3. ✅ 启动 Python HTTP 服务器
4. ✅ 显示访问地址

然后在浏览器中打开显示的地址即可！

## 📊 查看覆盖率

### 命令行摘要

```bash
cd build
lcov --summary coverage.info
```

输出示例：
```
Summary coverage rate:
  lines......: 90.4% (1945 of 2152 lines)
  functions..: 90.5% (813 of 898 functions)
```

### HTML 详细报告

使用 `view_coverage.sh` 启动服务器后，在浏览器中访问：
- `http://localhost:8000` （默认端口）
- `http://localhost:9000` （如果使用端口 9000）

### 报告说明

HTML 报告中：
- 🟢 **绿色** = 已覆盖的代码
- 🔴 **红色** = 未覆盖的代码
- 🟡 **黄色** = 部分覆盖（条件分支）

点击文件名可以查看每一行的覆盖情况。

## 🔧 完整流程

如果你想完整重新生成覆盖率：

```bash
# 1. 编译（带覆盖率）
mkdir -p build && cd build
cmake .. -DLUAJIT_ROOT=/usr/local/3rd/luajit-2.1.0-beta3 -DBUILD_COVERAGE=ON
make -j$(nproc)

# 2. 运行测试
ctest

# 3. 查看覆盖率（自动生成并启动服务器）
cd ..
./view_coverage.sh
```

## 💡 常用命令

```bash
# 只看覆盖率摘要
cd build && lcov --summary coverage.info

# 重新生成覆盖率报告
./run_tests.sh -c

# 查看特定文件的覆盖率
lcov --summary coverage.info | grep lua_state
```

## 🛑 停止服务器

在运行 `view_coverage.sh` 的终端中按 `Ctrl+C`

## ❓ 常见问题

**Q: 端口被占用怎么办？**
```bash
# 使用其他端口
./view_coverage.sh 9000
```

**Q: 无法在浏览器中访问？**
```bash
# 检查服务器是否运行
# 查看终端输出中的地址
# 确保防火墙允许该端口
```

**Q: 覆盖率数据过期？**
```bash
# 重新运行测试
cd build && ctest

# 或使用测试脚本
./run_tests.sh
```

## 📚 更多信息

- [完整测试指南](TESTING.md)
- [Ubuntu 专门指南](COVERAGE_UBUNTU.md)
- [主 README](README.md)
