# Ubuntu 覆盖率查看指南

本文档专门为 Ubuntu 用户提供覆盖率查看的详细说明。

## 快速开始

### 方法1: 使用快捷脚本（最简单）

```bash
# 1. 编译并运行测试（带覆盖率）
./run_tests.sh -c

# 2. 查看覆盖率报告
./view_coverage.sh
```

脚本会自动：
- 生成覆盖率数据
- 创建 HTML 报告
- 显示覆盖率摘要
- 尝试在浏览器中打开报告

### 方法2: 手动步骤

```bash
# 1. 进入构建目录
cd build

# 2. 配置 CMake（启用覆盖率）
cmake .. -DLUAJIT_ROOT=/usr/local/3rd/luajit-2.1.0-beta3 -DBUILD_COVERAGE=ON

# 3. 编译
make -j$(nproc)

# 4. 运行测试
ctest

# 5. 生成覆盖率数据
lcov --capture --directory . --output-file coverage.info

# 6. 过滤不需要的文件
lcov --remove coverage.info '/usr/*' --output-file coverage.info
lcov --remove coverage.info 'third-party/*' --output-file coverage.info
lcov --remove coverage.info 'tests/*' --output-file coverage.info

# 7. 生成 HTML 报告
genhtml coverage.info --output-directory coverage_html
```

## 查看覆盖率报告

### 选项1: 使用 Python HTTP 服务器（推荐）

```bash
cd build/coverage_html
python3 -m http.server 8000
```

然后在浏览器中访问：`http://localhost:8000`

**优点**：
- 可以在远程服务器上使用
- 支持任何现代浏览器
- 界面友好

### 选项2: 直接在浏览器中打开

```bash
# 使用 Firefox
firefox build/coverage_html/index.html

# 使用 Chrome
google-chrome build/coverage_html/index.html

# 或者直接将文件路径复制到浏览器地址栏
# file:///root/projects/luajit-rule-engine/build/coverage_html/index.html
```

### 选项3: 使用文件管理器

1. 打开文件管理器（Nautilus）
2. 导航到 `/root/projects/luajit-rule-engine/build/coverage_html/`
3. 双击 `index.html`

### 选项4: 使用 VS Code

如果你在 VS Code 中工作：

1. 安装 "Live Server" 扩展
2. 右键点击 `index.html`
3. 选择 "Open with Live Server"

## 查看命令行摘要

如果只需要快速查看覆盖率数字：

```bash
cd build
lcov --summary coverage.info
```

输出示例：
```
Summary coverage rate:
  lines......: 90.4% (1945 of 2152 lines)
  functions..: 90.5% (813 of 898 functions)
  branches...: no data found
```

## 理解覆盖率报告

### HTML 报告界面

打开报告后，你会看到：

1. **全局概览** - 顶部显示总体覆盖率
2. **文件列表** - 每个文件的覆盖率
3. **详细视图** - 点击文件名查看每一行的覆盖情况

### 颜色标记

- 🟢 **绿色** - 已覆盖的代码
- 🔴 **红色** - 未覆盖的代码
- 🟡 **黄色** - 部分覆盖（条件分支）

### 关键指标

- **Line Coverage** - 行覆盖率（最重要）
- **Function Coverage** - 函数覆盖率
- **Branch Coverage** - 分支覆盖率

## 常见问题

### Q: genhtml 命令找不到？

```bash
# 安装 lcov（包含 genhtml）
sudo apt-get update
sudo apt-get install lcov
```

### Q: 覆盖率数据为空？

确保：
1. 使用 `-DBUILD_COVERAGE=ON` 编译
2. 运行了测试：`ctest`
3. 没有删除 `.gcda` 文件

### Q: 无法在浏览器中打开？

尝试方法：
```bash
# 使用绝对路径
python3 -m http.server 8000 -d /root/projects/luajit-rule-engine/build/coverage_html

# 或者直接复制文件路径
echo "file://$(pwd)/coverage_html/index.html"
```

### Q: 只想看特定模块的覆盖率？

```bash
# 只看 LuaState 模块
lcov --summary coverage.info | grep lua_state

# 只提取特定文件
lcov --extract coverage.info '*/src/lua_state.cpp' --output-file lua_state.info
genhtml lua_state.info --output-directory lua_state_html
```

### Q: 远程服务器上如何查看？

```bash
# 在远程服务器上
cd build/coverage_html
python3 -m http.server 8000

# 在本地机器上使用 SSH 隧道
ssh -L 8000:localhost:8000 user@remote-server

# 然后在本地浏览器访问
# http://localhost:8000
```

## 性能优化

### 加速覆盖率数据收集

```bash
# 并行编译
make -j$(nproc)

# 并行运行测试
ctest -j$(nproc)
```

### 减小报告大小

```bash
# 只查看 src/ 目录的覆盖率
lcov --extract coverage.info '*/src/*' --output-file coverage_src.info
genhtml coverage_src.info --output-directory coverage_src_html
```

## 持续监控

### 创建快捷命令

在 `~/.bashrc` 或 `~/.bash_aliases` 中添加：

```bash
# 覆盖率快捷命令
alias cov='cd /root/projects/luajit-rule-engine && ./view_coverage.sh'
alias cov-summary='cd /root/projects/luajit-rule-engine/build && lcov --summary coverage.info'
```

然后可以直接运行：
```bash
cov          # 查看完整报告
cov-summary  # 只看摘要
```

## 集成到开发流程

### Git Hook（可选）

创建 `.git/hooks/pre-commit`：

```bash
#!/bin/bash
echo "检查代码覆盖率..."
./run_tests.sh -c
lcov --summary build/coverage.info | grep "lines......:"
```

确保每次提交前都检查覆盖率。

## 相关文档

- [完整测试指南](../TESTING.md)
- [项目 README](../README.md)
- [lcov 官方文档](http://ltp.sourceforge.net/coverage/lcov.php)
