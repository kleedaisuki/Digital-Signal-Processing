# 🌸 DSP Sequence Operations （数字信号序列运算框架）

> 使用 C 语言实现的序列加法、乘法、卷积与相关分析系统
> 支持实时滑动窗口（streaming window）模式，兼容 Windows 与 GNU Make 构建环境。
>
> 🧠 关键词：离散序列、线性卷积、圆周卷积、互相关、滑动窗口归一化相关

---

## 🧩 一、项目结构

```
project-root/
├─ include/
│   ├─ seq.h          # 序列与滑动窗口结构定义
│   ├─ ops.h          # 序列运算接口
│   └─ cli.h          # 命令行接口定义
│
├─ src/
│   ├─ seq.c          # 序列与滑动窗口实现
│   ├─ ops.c          # 加法、乘法、卷积、相关算法实现
│   ├─ cli.c          # 命令行交互逻辑
│   └─ main.c         # 主入口，仅委托 cli_run()
│
├─ bin/               # 可执行文件输出目录
├─ obj/               # 中间目标文件目录
├─ Makefile           # Windows 兼容的构建脚本
└─ README.md          # 项目说明文档（你现在看到的！）
```

---

## ⚙️ 二、构建与运行

### 🪟 Windows 环境（推荐使用 MSYS2 或 MinGW）

```bash
# 构建优化版
make

# 构建调试版（带符号信息）
make debug

# 清理生成文件
make clean
```

编译完成后可执行文件生成于：

```
bin\dsp_seq.exe
```

---

## 🎮 三、使用方法

运行程序需指定模式（mode），
随后从标准输入提供两个序列或实时数据流。

### 支持的模式

| 模式名称            | 功能描述                                        | 输入格式         |
| --------------- | ------------------------------------------- | ------------ |
| `add`           | 逐点加法                                        | 两条有限序列       |
| `mul`           | 逐点乘法                                        | 两条有限序列       |
| `conv-linear`   | 线性卷积（Linear Convolution）                    | 两条有限序列       |
| `conv-circular` | 圆周卷积（Circular Convolution）                  | 两条等长序列       |
| `corr`          | 互相关（Cross-Correlation）                      | 两条有限序列       |
| `corr-window`   | 滑动窗口归一化相关（Streaming Normalized Correlation） | 窗口大小 + 实时输入流 |

---

### 🌟 示例 1：加法模式

输入文件 `add.txt`：

```
5 1 2 3 4 5
5 10 20 30 40 50
```

运行：

```bash
dsp_seq.exe add < add.txt
```

输出：

```
5
11 22 33 44 55
```

---

### 🌟 示例 2：线性卷积模式

输入 `conv.txt`：

```
3 1 2 3
2 4 5
```

运行：

```bash
dsp_seq.exe conv-linear < conv.txt
```

输出：

```
4
4 13 22 15
```

---

### 🌟 示例 3：滑动窗口归一化相关模式

输入 `corr_stream.txt`：

```
3
1 2
2 4
3 6
4 8
5 10
```

运行：

```bash
dsp_seq.exe corr-window < corr_stream.txt
```

输出（每一行对应当前时刻的相关系数）：

```
nan
nan
1
1
1
```

> 前两行窗口未满，因此输出 nan。
> 从第三个时刻起，两个序列完全线性相关，相关系数恒为 1。

---

## 🧮 四、算法说明

### 1️⃣ 线性卷积 (Linear Convolution)

$$ y[n] = \sum_{k=0}^{L_a-1} a[k] \cdot b[n-k] $$
仅在合法索引范围内累加，输出长度为 (L_a + L_b - 1)。

### 2️⃣ 圆周卷积 (Circular Convolution)

$$ y[n] = \sum_{k=0}^{N-1} a[k] \cdot b[(n-k)\ \text{mod}\ N] $$
要求两输入序列长度相同。

### 3️⃣ 互相关 (Cross-Correlation)

$$ r_{xy}[\tau] = \sum_n x[n] \cdot y[n+\tau] $$
输出长度同卷积：(L_x + L_y - 1)。

### 4️⃣ 滑动窗口归一化相关 (Normalized Correlation)

对实时数据流中的滑动窗口计算皮尔逊系数 (Pearson correlation coefficient)：
$$
\rho =
\frac{\sum_i (x_i - \bar{x})(y_i - \bar{y})}
{\sqrt{\sum_i (x_i - \bar{x})^2 \sum_i (y_i - \bar{y})^2}}
$$

---

## 💡 五、实现特色

* 🧱 **模块化结构**：`seq` / `ops` / `cli` 分层清晰
* 🧮 **算法纯净**：全时域实现，无外部依赖
* 🧵 **实时流支持**：滑动窗口（`seq_window_t`）支持无限长序列处理
* 🧸 **安全与健壮**：所有函数均做 `NULL` 与越界检查
* 💬 **中英双语注释**：Doxygen 规范，可自动生成文档
* 💻 **跨平台**：在 Windows / Linux / macOS 下均可编译运行
