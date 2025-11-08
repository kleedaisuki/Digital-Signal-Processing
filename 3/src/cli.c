/**
 * @file cli.c
 * @brief 命令行前端实现 / Command-line front-end implementation
 *
 * 负责解析命令行参数、读取输入序列、调用运算接口并打印结果。
 * Responsible for parsing command-line arguments, reading input sequences,
 * invoking operation functions and printing results.
 */

#include "cli.h"
#include "seq.h"
#include "ops.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ==== 内部函数声明 / Internal function declarations ==== */

static void cli_print_usage(const char *prog);

static int cli_mode_add(void);
static int cli_mode_mul(void);
static int cli_mode_conv_linear(void);
static int cli_mode_conv_circular(void);
static int cli_mode_corr(void);
static int cli_mode_corr_window(void);

static int cli_read_seq(seq_t *s);
static int cli_read_two_seqs(seq_t *a, seq_t *b);
static void cli_print_seq(const seq_t *s);

/**
 * @brief 运行 CLI 主逻辑 / Run CLI main logic.
 *
 * @param argc 参数个数 / Argument count.
 * @param argv 参数数组 / Argument vector.
 * @return 进程退出码 / Process exit code.
 *
 * @note main() 中只需调用本函数，将控制权交给 CLI。
 *       In main(), simply call this function and delegate control to CLI.
 */
int cli_run(int argc, char **argv)
{
    if (argc < 2)
    {
        cli_print_usage(argv[0]);
        return 1;
    }

    const char *mode = argv[1];

    if (strcmp(mode, "add") == 0)
    {
        return cli_mode_add();
    }
    else if (strcmp(mode, "mul") == 0)
    {
        return cli_mode_mul();
    }
    else if (strcmp(mode, "conv-linear") == 0)
    {
        return cli_mode_conv_linear();
    }
    else if (strcmp(mode, "conv-circular") == 0)
    {
        return cli_mode_conv_circular();
    }
    else if (strcmp(mode, "corr") == 0)
    {
        return cli_mode_corr();
    }
    else if (strcmp(mode, "corr-window") == 0)
    {
        return cli_mode_corr_window();
    }
    else
    {
        fprintf(stderr, "Unknown mode: %s\n", mode);
        cli_print_usage(argv[0]);
        return 1;
    }
}

/**
 * @brief 打印使用方法 / Print command usage.
 *
 * @param prog 程序名 / Program name.
 */
static void cli_print_usage(const char *prog)
{
    fprintf(stderr,
            "Usage: %s [mode]\n"
            "Modes:\n"
            "  add             Point-wise addition of two sequences\n"
            "  mul             Point-wise multiplication of two sequences\n"
            "  conv-linear     Linear convolution of two sequences\n"
            "  conv-circular   Circular convolution of two sequences (same length)\n"
            "  corr            Cross-correlation of two sequences\n"
            "  corr-window     Streaming normalized correlation using sliding windows\n"
            "\n"
            "Input format for two-sequence modes:\n"
            "  <len_a> a0 a1 ... a(len_a-1)\n"
            "  <len_b> b0 b1 ... b(len_b-1)\n"
            "\n"
            "For corr-window mode:\n"
            "  <win_size>\n"
            "  ax0 bx0\n"
            "  ax1 bx1\n"
            "  ... (pairs until EOF)\n",
            prog);
}

/* ==== 公共辅助函数 / Common helper functions ==== */

/**
 * @brief 从标准输入读取一个序列 / Read a single sequence from stdin.
 *
 * @param s 输出序列 / Output sequence.
 * @return 0 表示成功；非 0 表示失败。
 *         0 on success; non-zero on failure.
 *
 * @note 输入格式:
 *       <len> v0 v1 ... v(len-1)
 */
static int cli_read_seq(seq_t *s)
{
    size_t len = 0;
    if (scanf("%zu", &len) != 1)
    {
        fprintf(stderr, "Failed to read sequence length.\n");
        return -1;
    }

    if (seq_init(s, len) != 0)
    {
        fprintf(stderr, "Failed to allocate sequence of length %zu.\n", len);
        return -1;
    }

    for (size_t i = 0; i < len; ++i)
    {
        double v;
        if (scanf("%lf", &v) != 1)
        {
            fprintf(stderr, "Failed to read sequence element at index %zu.\n", i);
            seq_free(s);
            return -1;
        }
        s->data[i] = (seq_sample_t)v;
    }

    return 0;
}

/**
 * @brief 从标准输入读取两条序列 / Read two sequences from stdin.
 *
 * @param a 第一条序列 / First sequence.
 * @param b 第二条序列 / Second sequence.
 * @return 0 表示成功；非 0 表示失败。
 */
static int cli_read_two_seqs(seq_t *a, seq_t *b)
{
    if (cli_read_seq(a) != 0)
        return -1;

    if (cli_read_seq(b) != 0)
    {
        seq_free(a);
        return -1;
    }

    return 0;
}

/**
 * @brief 打印序列到标准输出 / Print sequence to stdout.
 *
 * @param s 要打印的序列 / Sequence to print.
 *
 * @note 格式:
 *       第一行: 长度
 *       第二行: 所有元素（空格分隔）
 *       First line: length
 *       Second line: elements separated by spaces.
 */
static void cli_print_seq(const seq_t *s)
{
    if (!s)
    {
        fprintf(stderr, "cli_print_seq: NULL sequence pointer.\n");
        return;
    }

    printf("%zu\n", s->length);
    for (size_t i = 0; i < s->length; ++i)
    {
        printf("%.10g", (double)s->data[i]);
        if (i + 1 < s->length)
            printf(" ");
    }
    printf("\n");
}

/* ==== 各模式实现 / Mode handlers ==== */

/**
 * @brief 模式: 逐点加法 / Mode: point-wise addition.
 */
static int cli_mode_add(void)
{
    seq_t a = {0}, b = {0}, out = {0};

    if (cli_read_two_seqs(&a, &b) != 0)
        return 1;

    if (seq_add(&a, &b, &out) != 0)
    {
        fprintf(stderr, "Add operation failed.\n");
        seq_free(&a);
        seq_free(&b);
        return 1;
    }

    cli_print_seq(&out);

    seq_free(&a);
    seq_free(&b);
    seq_free(&out);
    return 0;
}

/**
 * @brief 模式: 逐点乘法 / Mode: point-wise multiplication.
 */
static int cli_mode_mul(void)
{
    seq_t a = {0}, b = {0}, out = {0};

    if (cli_read_two_seqs(&a, &b) != 0)
        return 1;

    if (seq_mul(&a, &b, &out) != 0)
    {
        fprintf(stderr, "Mul operation failed.\n");
        seq_free(&a);
        seq_free(&b);
        return 1;
    }

    cli_print_seq(&out);

    seq_free(&a);
    seq_free(&b);
    seq_free(&out);
    return 0;
}

/**
 * @brief 模式: 线性卷积 / Mode: linear convolution.
 */
static int cli_mode_conv_linear(void)
{
    seq_t a = {0}, b = {0}, out = {0};

    if (cli_read_two_seqs(&a, &b) != 0)
        return 1;

    if (seq_conv_linear(&a, &b, &out) != 0)
    {
        fprintf(stderr, "Linear convolution failed.\n");
        seq_free(&a);
        seq_free(&b);
        return 1;
    }

    cli_print_seq(&out);

    seq_free(&a);
    seq_free(&b);
    seq_free(&out);
    return 0;
}

/**
 * @brief 模式: 圆周卷积 / Mode: circular convolution.
 */
static int cli_mode_conv_circular(void)
{
    seq_t a = {0}, b = {0}, out = {0};

    if (cli_read_two_seqs(&a, &b) != 0)
        return 1;

    if (seq_conv_circular(&a, &b, &out) != 0)
    {
        fprintf(stderr, "Circular convolution failed.\n");
        seq_free(&a);
        seq_free(&b);
        return 1;
    }

    cli_print_seq(&out);

    seq_free(&a);
    seq_free(&b);
    seq_free(&out);
    return 0;
}

/**
 * @brief 模式: 互相关 / Mode: cross-correlation.
 */
static int cli_mode_corr(void)
{
    seq_t a = {0}, b = {0}, out = {0};

    if (cli_read_two_seqs(&a, &b) != 0)
        return 1;

    if (seq_corr_cross(&a, &b, &out) != 0)
    {
        fprintf(stderr, "Cross-correlation failed.\n");
        seq_free(&a);
        seq_free(&b);
        return 1;
    }

    cli_print_seq(&out);

    seq_free(&a);
    seq_free(&b);
    seq_free(&out);
    return 0;
}

/**
 * @brief 模式: 滑动窗口归一化相关 (流式) /
 *        Mode: streaming normalized correlation using sliding windows.
 *
 * 输入格式:
 *   <win_size>
 *   ax0 bx0
 *   ax1 bx1
 *   ...
 * 直到 EOF。
 *
 * 对于每一对输入样本，更新窗口并尝试计算当前归一化相关系数:
 *   - 若成功, 输出一行相关系数。
 *   - 若由于窗口未满或方差为 0 导致失败, 输出 "nan"。
 */
static int cli_mode_corr_window(void)
{
    size_t win_size = 0;
    if (scanf("%zu", &win_size) != 1 || win_size == 0)
    {
        fprintf(stderr, "corr-window: invalid window size.\n");
        return 1;
    }

    seq_window_t wa, wb;
    if (seq_window_init(&wa, win_size) != 0)
    {
        fprintf(stderr, "corr-window: failed to initialize window A.\n");
        return 1;
    }
    if (seq_window_init(&wb, win_size) != 0)
    {
        fprintf(stderr, "corr-window: failed to initialize window B.\n");
        seq_window_free(&wa);
        return 1;
    }

    double ax, bx;
    while (scanf("%lf %lf", &ax, &bx) == 2)
    {
        seq_window_push(&wa, (seq_sample_t)ax);
        seq_window_push(&wb, (seq_sample_t)bx);

        seq_sample_t rho = 0.0;
        int rc = seq_corr_window_norm(&wa, &wb, &rho);
        if (rc == 0)
        {
            printf("%.10g\n", (double)rho);
        }
        else
        {
            /* 无法计算时输出 nan，错误详情已在 stderr。 */
            printf("nan\n");
        }
    }

    seq_window_free(&wa);
    seq_window_free(&wb);
    return 0;
}
