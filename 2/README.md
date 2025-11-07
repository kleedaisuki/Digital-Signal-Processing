# seqops

一个轻量级的 C 语言离散时间序列处理命令行工具，支持有限序列与流式（随来随处理）两种模式，结构清晰、模块化、带中英双语 Doxygen 注释。

---

## 🌟 项目简介

**seqops** 实现了常见的一维信号操作，既可以对有限长度序列进行离线处理，也可以对无限输入流进行在线（随来随处理）操作。整体架构遵循 UNIX 简洁哲学与 Linux 内核式代码规范：每个函数只做一件事，并且做好它。

支持的操作如下：

| 操作名          | 功能描述    | 是否支持无限输入流（Streaming） |
| ------------ | ------- | -------------------- |
| `pad-front`  | 前补零     | ✅                    |
| `pad-back`   | 后补零     | ❌                    |
| `delay`      | 延迟（因果）  | ✅                    |
| `advance`    | 提前（非因果） | ❌                    |
| `reverse`    | 反转序列    | ❌                    |
| `upsample`   | 上采样（插零） | ✅                    |
| `downsample` | 下采样（抽取） | ✅                    |
| `diff`       | 差分      | ✅                    |
| `cumsum`     | 累加（前缀和） | ✅                    |

---

## 🧩 文件结构

| 文件           | 功能说明                            |
| ------------ | ------------------------------- |
| `sequence.h` | 定义核心数据结构与 API（含中英双语 Doxygen 注释） |
| `sequence.c` | 实现所有序列操作与流式逻辑                   |
| `cli.h`      | 命令行接口声明                         |
| `cli.c`      | 参数解析、输入输出协议、ONLINE 判定           |
| `main.c`     | 极薄入口，只调用 `cli_main()`           |
| `Makefile`   | GNU Make 构建脚本（兼容 Windows）       |

---

## ⚙️ 构建方法

### 依赖环境

* GCC（支持 C11）
* GNU Make（Windows 可用 MinGW 或 MSYS2 环境）

### 编译命令

```bash
make       # 构建 seqops.exe（自动清理 .o 文件）
make clean # 删除二进制与中间文件
```

构建成功后，会生成可执行文件：`seqops.exe`（Windows）或 `seqops`（Linux/macOS）。

---

## 🧠 使用方法（Windows 版）

### 基本语法

```
seqops <操作名> [参数...] finite
seqops <操作名> [参数...] stream
```

---

### 有限模式（finite）

有限模式用于一次性输入固定长度的序列。

输入格式：

```
N
x0 x1 x2 ... x{N-1}
```

示例（Windows CMD 或 PowerShell 中运行）：

```bat
type NUL > input.txt
echo 5>>input.txt
echo 1 2 3 4 5>>input.txt
seqops diff finite < input.txt
```

输出：

```
ONLINE:YES
1 1 1 1 1
```

> 💡 **提示**：在 Windows 上，`echo` 默认会自动添加换行，因此用重定向到临时文件的方式更安全。

---

### 流式模式（stream）

流式模式用于“随来随处理”的信号输入，输入以 `END` 结尾。

输入格式：

```
x0 x1 x2 ... END
```

示例：

```bat
echo 1 2 3 4 END | seqops cumsum stream
```

输出：

```
ONLINE:YES
1 3 6 10
```

> 💬 **说明**：`END` 是结束标记，区分大小写；处理完毕后程序会自动输出结果。

---

## 🧪 示例测试（Windows）

以下命令都可以直接在 **PowerShell 或 CMD** 中运行：

```bat
:: 延迟（finite）
(echo 4 & echo 1 2 3 4) | seqops delay 2 0 finite

:: 反转（finite）
(echo 4 & echo 1 2 3 4) | seqops reverse finite

:: 上采样（finite）
(echo 3 & echo 1 2 3) | seqops upsample 3 finite

:: 累加（stream）
echo 1 2 3 4 END | seqops cumsum stream

:: 下采样（stream）
echo 1 2 3 4 5 6 END | seqops downsample 2 stream
```

> ✅ 建议使用 PowerShell，语法更接近 UNIX shell，且以上写法无需临时文件即可直接运行。

---

## 💡 设计要点

* **中英双语 Doxygen 注释**，兼顾国内与国际开发者
* **日志英文输出**，所有错误信息统一打印到 `stderr`
* **五文件结构**，逻辑分层清晰：核心逻辑、CLI、入口完全解耦
* **流式架构**：通过 `seq_stream_t` 实现在线运算
* **可因果性判断**：通过 `seq_online_capable()` 判定是否可无限输入
* **内核式编码哲学**：

  * 函数短小（≤3 层缩进）
  * 不破坏用户空间
  * 简洁胜于复杂
