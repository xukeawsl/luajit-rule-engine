#!/usr/bin/env python3
"""
LuaJIT Rule Engine 性能测试报告生成器

从 Google Benchmark 的 JSON 输出生成详细的 HTML 报告
"""

import json
import os
import sys
from datetime import datetime
from pathlib import Path
import argparse
import re

# 颜色定义
COLORS = {
    'primary': '#3498db',
    'success': '#27ae60',
    'warning': '#f39c12',
    'danger': '#e74c3c',
    'info': '#16a085',
    'dark': '#2c3e50',
    'light': '#ecf0f1',
    'luajit': '#f39c12',
    'native': '#2980b9'
}

def load_json_results(results_dirs):
    """从多个目录加载 JSON 测试结果"""
    results = {}

    # 支持多个目录
    if isinstance(results_dirs, str):
        results_dirs = [results_dirs]

    for results_dir in results_dirs:
        results_path = Path(results_dir)
        if not results_path.exists():
            continue

        for json_file in results_path.glob('*.json'):
            # 跳过我们自己生成的摘要文件
            if 'benchmark_summary' in json_file.name or 'benchmark_report' in json_file.name:
                continue

            try:
                with open(json_file, 'r') as f:
                    data = json.load(f)

                    # 只处理包含 benchmarks 的文件
                    if 'benchmarks' not in data:
                        continue

                    benchmark_name = json_file.stem
                    results[benchmark_name] = data
                    print(f"  ✓ 加载: {json_file.name} ({len(data['benchmarks'])} 个测试)")
            except json.JSONDecodeError as e:
                print(f"  ✗ JSON 解析错误 {json_file}: {e}")
            except Exception as e:
                print(f"  ✗ 无法加载 {json_file}: {e}")

    return results

def parse_benchmark_name(name):
    """解析 benchmark 名称，提取关键信息"""
    # 格式: LuaJIT_SimpleRule_SmallData/iterations:1000000
    # 或: Native_SimpleRule_SmallData/iterations:1000000

    parts = name.split('/')

    # 解析主名称
    main_name = parts[0]

    # 提取实现类型
    implementation = 'Unknown'
    if main_name.startswith('LuaJIT_'):
        implementation = 'LuaJIT'
    elif main_name.startswith('Native_'):
        implementation = 'Native'

    # 提取规则类型和数据规模
    # 例如: LuaJIT_SimpleRule_SmallData
    name_parts = main_name.split('_')
    rule_type = 'Unknown'
    data_size = 'Unknown'

    if len(name_parts) >= 3:
        if implementation == 'LuaJIT':
            rule_type = name_parts[1]  # SimpleRule
            data_size = name_parts[2] if len(name_parts) > 2 else ''  # SmallData
        elif implementation == 'Native':
            rule_type = name_parts[1]
            data_size = name_parts[2] if len(name_parts) > 2 else ''

    # 提取迭代次数
    iterations = 0
    for part in parts:
        if part.startswith('iterations:'):
            iterations = int(part.split(':')[1])
            break

    return {
        'implementation': implementation,
        'rule_type': rule_type,
        'data_size': data_size,
        'iterations': iterations,
        'full_name': name
    }

def extract_comparison_data(benchmarks):
    """提取 LuaJIT vs Native 对比数据"""
    comparisons = {}

    for benchmark_name, data in benchmarks.items():
        if 'benchmarks' not in data:
            continue

        for bm in data['benchmarks']:
            info = parse_benchmark_name(bm['name'])

            # 只处理成对的测试
            if info['implementation'] == 'Unknown':
                continue

            # 创建键值
            key = f"{info['rule_type']}_{info['data_size']}"

            if key not in comparisons:
                comparisons[key] = {
                    'rule_type': info['rule_type'],
                    'data_size': info['data_size'],
                    'luajit': None,
                    'native': None
                }

            # 存储数据
            time_us = bm.get('real_time', 0)
            cpu_time_us = bm.get('cpu_time', 0)
            items_per_second = bm.get('items_per_second', 0)
            iterations = bm.get('iterations', 0)

            benchmark_data = {
                'time_us': time_us,
                'cpu_time_us': cpu_time_us,
                'items_per_second': items_per_second,
                'iterations': iterations,
                'name': bm['name']
            }

            if info['implementation'] == 'LuaJIT':
                comparisons[key]['luajit'] = benchmark_data
            elif info['implementation'] == 'Native':
                comparisons[key]['native'] = benchmark_data

    return comparisons

def calculate_speedup(luajit_time, native_time):
    """计算加速比"""
    if native_time == 0:
        return 0
    return luajit_time / native_time

def format_time(us):
    """格式化时间显示"""
    if us < 1:
        return f"{us*1000:.2f} ns"
    elif us < 1000:
        return f"{us:.2f} μs"
    else:
        return f"{us/1000:.2f} ms"

def format_throughput(ops_per_sec):
    """格式化吞吐量"""
    if ops_per_sec >= 1e6:
        return f"{ops_per_sec/1e6:.2f}M ops/s"
    elif ops_per_sec >= 1e3:
        return f"{ops_per_sec/1e3:.2f}K ops/s"
    else:
        return f"{ops_per_sec:.0f} ops/s"

def get_recommendation(speedup, rule_type):
    """根据性能比率给出建议"""
    if speedup <= 1.2:
        return "LuaJIT", "success"  # 接近原生性能
    elif speedup <= 3.0:
        return "LuaJIT", "warning"  # 可接受
    elif rule_type in ['SimpleRule']:
        return "Native", "warning"  # 简单规则用 Native
    else:
        return "Native", "danger"  # 性能差距大

def generate_html_report(results, output_file):
    """生成 HTML 报告"""

    # 提取对比数据
    comparisons = extract_comparison_data(results)

    # 生成表格行
    table_rows = []
    for key, data in sorted(comparisons.items()):
        if not data['luajit'] or not data['native']:
            continue

        luajit_time = data['luajit']['time_us']
        native_time = data['native']['time_us']
        speedup = calculate_speedup(luajit_time, native_time)

        recommendation, badge_type = get_recommendation(speedup, data['rule_type'])

        rule_label = data['rule_type'].replace('Rule', '规则')
        data_label = data['data_size'].replace('Data', '数据')

        table_rows.append(f"""
                    <tr>
                        <td>{rule_label} + {data_label}</td>
                        <td><span class="badge badge-warning">{format_time(luajit_time)}</span><br><small>{format_throughput(data['luajit']['items_per_second'])}</small></td>
                        <td><span class="badge badge-success">{format_time(native_time)}</span><br><small>{format_throughput(data['native']['items_per_second'])}</small></td>
                        <td><strong>{speedup:.2f}x</strong><br>{"慢" if speedup > 1 else "快"}</td>
                        <td><span class="badge badge-{badge_type}">{recommendation}</span></td>
                    </tr>""")

    table_html = '\n'.join(table_rows)

    # 提取系统信息
    context = {}
    for data in results.values():
        if 'context' in data:
            context = data['context']
            break

    cpu_info = ""
    if context:
        num_cpus = context.get('num_cpus', 'N/A')
        mhz_per_cpu = context.get('mhz_per_cpu', 'N/A')
        cpu_info = f"{num_cpus} x {mhz_per_cpu} MHz"

    html = f"""<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>LuaJIT Rule Engine 性能测试报告</title>
    <style>
        * {{
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }}

        body {{
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            line-height: 1.6;
            color: {COLORS['dark']};
            background: {COLORS['light']};
        }}

        .container {{
            max-width: 1200px;
            margin: 0 auto;
            padding: 20px;
        }}

        header {{
            background: linear-gradient(135deg, {COLORS['primary']}, {COLORS['info']});
            color: white;
            padding: 40px 20px;
            border-radius: 10px;
            margin-bottom: 30px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
        }}

        header h1 {{
            font-size: 2.5em;
            margin-bottom: 10px;
        }}

        header .subtitle {{
            font-size: 1.1em;
            opacity: 0.9;
        }}

        .card {{
            background: white;
            border-radius: 10px;
            padding: 25px;
            margin-bottom: 25px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }}

        .card h2 {{
            color: {COLORS['primary']};
            margin-bottom: 20px;
            padding-bottom: 10px;
            border-bottom: 2px solid {COLORS['light']};
        }}

        table {{
            width: 100%;
            border-collapse: collapse;
            margin: 20px 0;
        }}

        th, td {{
            padding: 12px;
            text-align: left;
            border-bottom: 1px solid #ddd;
        }}

        th {{
            background: {COLORS['light']};
            font-weight: 600;
            color: {COLORS['dark']};
        }}

        tr:hover {{
            background: #f8f9fa;
        }}

        .badge {{
            display: inline-block;
            padding: 4px 12px;
            border-radius: 20px;
            font-size: 0.85em;
            font-weight: 600;
        }}

        .badge-success {{
            background: {COLORS['success']};
            color: white;
        }}

        .badge-warning {{
            background: {COLORS['warning']};
            color: white;
        }}

        .badge-danger {{
            background: {COLORS['danger']};
            color: white;
        }}

        .findings {{
            background: #fff3cd;
            border-left: 4px solid {COLORS['warning']};
            padding: 15px 20px;
            margin: 20px 0;
            border-radius: 4px;
        }}

        .findings h3 {{
            color: #856404;
            margin-bottom: 10px;
        }}

        .recommendations {{
            background: #d1ecf1;
            border-left: 4px solid {COLORS['info']};
            padding: 15px 20px;
            margin: 20px 0;
            border-radius: 4px;
        }}

        .recommendations h3 {{
            color: #0c5460;
            margin-bottom: 10px;
        }}

        footer {{
            text-align: center;
            padding: 30px 20px;
            color: #7f8c8d;
            font-size: 0.9em;
        }}

        .progress-bar {{
            width: 100%;
            height: 25px;
            background: {COLORS['light']};
            border-radius: 12px;
            overflow: hidden;
            margin: 10px 0;
        }}

        .progress-fill {{
            height: 100%;
            background: linear-gradient(90deg, {COLORS['success']}, {COLORS['primary']});
            transition: width 0.3s ease;
            display: flex;
            align-items: center;
            justify-content: center;
            color: white;
            font-weight: 600;
            font-size: 0.9em;
        }}
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>🚀 LuaJIT Rule Engine 性能测试报告</h1>
            <p class="subtitle">生成时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</p>
        </header>

        <div class="card">
            <h2>📊 测试结果总览</h2>

            <h3 style="margin-top: 20px;">核心性能指标对比</h3>
            <table>
                <thead>
                    <tr>
                        <th>规则类型</th>
                        <th>LuaJIT 性能</th>
                        <th>Native 性能</th>
                        <th>性能比率</th>
                        <th>推荐方案</th>
                    </tr>
                </thead>
                <tbody>
{table_html}
                </tbody>
            </table>
        </div>

        <div class="card">
            <h2>🎯 关键发现</h2>

            <div class="findings">
                <h3>核心洞察</h3>
                <ul>
                    <li><strong>基于实际测试数据</strong> - 本报告从 {len(results)} 个测试结果文件生成</li>
                    <li><strong>自动化数据提取</strong> - 直接从 Google Benchmark JSON 输出解析</li>
                    <li><strong>对比分析</strong> - LuaJIT vs Native C++ 性能对比</li>
                </ul>
            </div>

            <div class="recommendations">
                <h3>实践建议</h3>
                <table>
                    <thead>
                        <tr>
                            <th>场景</th>
                            <th>推荐方案</th>
                            <th>原因</th>
                        </tr>
                    </thead>
                    <tbody>
                        <tr>
                            <td>风控系统 (中等复杂度)</td>
                            <td><span class="badge badge-success">LuaJIT</span></td>
                            <td>性能接近原生，可动态更新规则</td>
                        </tr>
                        <tr>
                            <td>配置验证 (简单规则)</td>
                            <td><span class="badge badge-warning">Native C++</span></td>
                            <td>性能关键路径，追求极致性能</td>
                        </tr>
                        <tr>
                            <td>综合评分 (超复杂)</td>
                            <td><span class="badge badge-warning">拆分 + LuaJIT</span></td>
                            <td>或直接使用 Native C++</td>
                        </tr>
                        <tr>
                            <td>快速原型开发</td>
                            <td><span class="badge badge-success">LuaJIT</span></td>
                            <td>灵活性与开发速度 > 性能</td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </div>

        <div class="card">
            <h2>📈 详细测试数据</h2>
            <p>以下是从 Google Benchmark 生成的原始数据摘要：</p>

            <h4 style="margin-top: 20px;">所有基准测试结果</h4>
            <table>
                <thead>
                    <tr>
                        <th>测试名称</th>
                        <th>时间 (μs/op)</th>
                        <th>CPU (μs/op)</th>
                        <th>迭代次数</th>
                        <th>吞吐量</th>
                    </tr>
                </thead>
                <tbody>
"""

    # 添加所有测试的详细数据
    for benchmark_name, data in sorted(results.items()):
        if 'benchmarks' not in data:
            continue

        for bm in data['benchmarks']:
            name = bm['name']
            time_us = bm.get('real_time', 0)
            cpu_time_us = bm.get('cpu_time', 0)
            iterations = bm.get('iterations', 0)
            items_per_sec = bm.get('items_per_second', 0)

            html += f"""
                    <tr>
                        <td>{name}</td>
                        <td>{time_us:.2f}</td>
                        <td>{cpu_time_us:.2f}</td>
                        <td>{iterations:,}</td>
                        <td>{format_throughput(items_per_sec)}</td>
                    </tr>"""

    html += """
                </tbody>
            </table>
        </div>

        <footer>
            <p>Generated by LuaJIT Rule Engine Benchmark Tool</p>
            <p>测试环境: """ + cpu_info + """</p>
            <p>测试时间: """ + datetime.now().strftime('%Y-%m-%d %H:%M:%S') + """</p>
        </footer>
    </div>
</body>
</html>
"""

    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(html)

    print(f"✅ HTML 报告已生成: {output_file}")

def generate_markdown_report(results, output_file):
    """生成 Markdown 报告"""

    # 提取对比数据
    comparisons = extract_comparison_data(results)

    md = f"""# LuaJIT Rule Engine 性能测试报告

**生成时间**: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}

---

## 📊 执行摘要

本报告从 **{len(results)}** 个测试结果文件自动生成，包含以下测试：

"""

    # 统计信息
    total_tests = sum(len(data.get('benchmarks', [])) for data in results.values())
    md += f"- **总测试数**: {total_tests}\n"
    md += f"- **测试文件**: {', '.join(sorted(results.keys()))}\n\n"

    md += "---\n\n"

    # 生成对比表格
    md += "## 🎯 核心性能指标\n\n"
    md += "| 规则类型 | 数据规模 | LuaJIT 性能 | Native 性能 | 性能比率 | 推荐方案 |\n"
    md += "|---------|---------|------------|------------|---------|---------|\n"

    for key, data in sorted(comparisons.items()):
        if not data['luajit'] or not data['native']:
            continue

        luajit_time = data['luajit']['time_us']
        native_time = data['native']['time_us']
        speedup = calculate_speedup(luajit_time, native_time)
        recommendation, _ = get_recommendation(speedup, data['rule_type'])

        rule_label = data['rule_type'].replace('Rule', '')
        data_label = data['data_size'].replace('Data', '')

        md += f"| {rule_label} | {data_label} | {format_time(luajit_time)} | {format_time(native_time)} | {speedup:.2f}x | {recommendation} |\n"

    md += "\n---\n\n"

    # 关键发现
    md += "## 🔍 关键发现\n\n"

    best_speedup = min(
        (calculate_speedup(d['luajit']['time_us'], d['native']['time_us'])
         for d in comparisons.values() if d['luajit'] and d['native']),
        default=0
    )

    worst_speedup = max(
        (calculate_speedup(d['luajit']['time_us'], d['native']['time_us'])
         for d in comparisons.values() if d['luajit'] and d['native']),
        default=0
    )

    md += f"- **最佳性能比率**: {best_speedup:.2f}x (接近原生性能)\n"
    md += f"- **最大性能差距**: {worst_speedup:.2f}x\n"
    md += f"- **测试覆盖**: {len(comparisons)} 种规则/数据组合\n\n"

    md += "---\n\n"

    # 详细测试数据
    md += "## 📈 详细测试数据\n\n"

    for benchmark_name, data in sorted(results.items()):
        if 'benchmarks' not in data:
            continue

        md += f"### {benchmark_name}\n\n"
        md += "| 测试名称 | 时间 (μs/op) | CPU (μs/op) | 迭代次数 | 吞吐量 |\n"
        md += "|---------|-------------|------------|---------|--------|\n"

        for bm in data['benchmarks']:
            name = bm['name']
            time_us = bm.get('real_time', 0)
            cpu_time_us = bm.get('cpu_time', 0)
            iterations = bm.get('iterations', 0)
            items_per_sec = bm.get('items_per_second', 0)

            md += f"| {name} | {time_us:.2f} | {cpu_time_us:.2f} | {iterations:,} | {format_throughput(items_per_sec)} |\n"

        md += "\n"

    md += "---\n\n"

    md += f"""**报告版本**: 1.0
**最后更新**: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
"""

    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(md)

    print(f"✅ Markdown 报告已生成: {output_file}")

def generate_json_summary(results, output_file):
    """生成 JSON 格式的摘要"""

    comparisons = extract_comparison_data(results)

    summary = {
        "generated_at": datetime.now().isoformat(),
        "source_files": list(results.keys()),
        "total_benchmarks": sum(len(data.get('benchmarks', [])) for data in results.values()),
        "benchmarks": {}
    }

    for key, data in comparisons.items():
        if not data['luajit'] or not data['native']:
            continue

        rule_key = data['rule_type'].lower().replace('rule', '_rule')
        summary["benchmarks"][rule_key] = {
            "luajit_us": round(data['luajit']['time_us'], 2),
            "native_us": round(data['native']['time_us'], 2),
            "speedup": round(calculate_speedup(data['luajit']['time_us'], data['native']['time_us']), 2),
            "luajit_throughput": round(data['luajit']['items_per_second'], 0),
            "native_throughput": round(data['native']['items_per_second'], 0),
            "recommendation": get_recommendation(
                calculate_speedup(data['luajit']['time_us'], data['native']['time_us']),
                data['rule_type']
            )[0]
        }

    # 提取系统上下文
    for data in results.values():
        if 'context' in data:
            summary["context"] = {
                "date": data['context'].get('date'),
                "host_name": data['context'].get('host_name'),
                "num_cpus": data['context'].get('num_cpus'),
                "mhz_per_cpu": data['context'].get('mhz_per_cpu'),
                "cpu_scaling_enabled": data['context'].get('cpu_scaling_enabled'),
            }
            break

    with open(output_file, 'w', encoding='utf-8') as f:
        json.dump(summary, f, indent=2, ensure_ascii=False)

    print(f"✅ JSON 摘要已生成: {output_file}")

def main():
    parser = argparse.ArgumentParser(description='生成性能测试报告')
    parser.add_argument('--results-dir',
                       nargs='+',
                       default=['build/benchmarks/results', 'benchmarks/results'],
                       help='测试结果目录 (默认: build/benchmarks/results benchmarks/results)')
    parser.add_argument('--output-dir',
                       default='benchmarks/results',
                       help='报告输出目录 (默认: benchmarks/results)')
    parser.add_argument('--format',
                       choices=['html', 'markdown', 'json', 'all'],
                       default='all',
                       help='报告格式 (默认: all)')

    args = parser.parse_args()

    # 确保输出目录存在
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    # 加载测试结果
    print("📊 正在生成性能测试报告...")
    print(f"   搜索目录: {args.results_dir}")
    results = load_json_results(args.results_dir)

    if not results:
        print("❌ 未找到测试结果！")
        print("\n💡 请先运行 benchmark 测试:")
        print("   cd build/benchmarks")
        print("   ./basic_benchmark --benchmark_format=json > results/basic.json")
        print("   ./comparison_benchmark --benchmark_format=json > results/comparison.json")
        return 1

    timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')

    # 生成报告
    if args.format in ['html', 'all']:
        html_file = output_dir / f"benchmark_report_{timestamp}.html"
        generate_html_report(results, html_file)

    if args.format in ['markdown', 'all']:
        md_file = output_dir / f"benchmark_report_{timestamp}.md"
        generate_markdown_report(results, md_file)

    if args.format in ['json', 'all']:
        json_file = output_dir / f"benchmark_summary_{timestamp}.json"
        generate_json_summary(results, json_file)

    print(f"\n✅ 报告生成完成！输出目录: {output_dir}")
    print(f"\n💡 查看报告:")
    print(f"   HTML:     {output_dir}/benchmark_report_{timestamp}.html")
    print(f"   Markdown: {output_dir}/benchmark_report_{timestamp}.md")
    print(f"   JSON:     {output_dir}/benchmark_summary_{timestamp}.json")

    return 0

if __name__ == '__main__':
    sys.exit(main())
