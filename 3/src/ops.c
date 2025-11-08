/**
 * @file ops.c
 * @brief 序列运算实现 / Implementation of sequence operations
 */

#include "ops.h"
#include "seq.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/* 内部工具：安全释放并清零，用于出错回滚 / internal helper to free sequence on error */
static void ops_reset_seq(seq_t *s)
{
    if (!s)
        return;
    seq_free(s);
    s->data = NULL;
    s->length = 0;
}

/**
 * @brief 序列逐点加法 / Point-wise addition of two sequences.
 *
 * @param a 输入序列 A / Input sequence A
 * @param b 输入序列 B / Input sequence B
 * @param out 输出序列，函数内部分配内存 / Output sequence, allocated inside
 * @return 0 表示成功；非 0 表示错误（参数非法或内存失败）。
 *         0 on success; non-zero on invalid arguments or allocation failure.
 *
 * @note
 * - 输出长度为 min(a->length, b->length)。
 *   Output length is min(a->length, b->length).
 * - 若任一输入为 NULL，或 min 长度为 0，则返回空序列或错误。
 */
int seq_add(const seq_t *a, const seq_t *b, seq_t *out)
{
    if (!a || !b || !out)
    {
        fprintf(stderr, "seq_add: null pointer argument.\n");
        return -1;
    }

    size_t n = (a->length < b->length) ? a->length : b->length;
    if (n == 0)
    {
        /* 合法但无输出数据：创建空序列 / Valid but empty result */
        if (seq_init(out, 0) != 0)
        {
            fprintf(stderr, "seq_add: failed to initialize empty output.\n");
            return -1;
        }
        return 0;
    }

    if (seq_init(out, n) != 0)
    {
        fprintf(stderr, "seq_add: failed to allocate output sequence.\n");
        return -1;
    }

    for (size_t i = 0; i < n; ++i)
    {
        out->data[i] = a->data[i] + b->data[i];
    }

    return 0;
}

/**
 * @brief 序列逐点乘法 / Point-wise multiplication of two sequences.
 *
 * @param a 输入序列 A / Input sequence A
 * @param b 输入序列 B / Input sequence B
 * @param out 输出序列 / Output sequence (allocated inside)
 * @return 0 表示成功；非 0 表示错误。
 *         0 on success; non-zero on error.
 *
 * @note
 * - 输出长度为 min(a->length, b->length)。
 */
int seq_mul(const seq_t *a, const seq_t *b, seq_t *out)
{
    if (!a || !b || !out)
    {
        fprintf(stderr, "seq_mul: null pointer argument.\n");
        return -1;
    }

    size_t n = (a->length < b->length) ? a->length : b->length;
    if (n == 0)
    {
        if (seq_init(out, 0) != 0)
        {
            fprintf(stderr, "seq_mul: failed to initialize empty output.\n");
            return -1;
        }
        return 0;
    }

    if (seq_init(out, n) != 0)
    {
        fprintf(stderr, "seq_mul: failed to allocate output sequence.\n");
        return -1;
    }

    for (size_t i = 0; i < n; ++i)
    {
        out->data[i] = a->data[i] * b->data[i];
    }

    return 0;
}

/**
 * @brief 线性卷积 / Linear convolution of two sequences.
 *
 * @param a 输入序列 A / Input sequence A (length = La)
 * @param b 输入序列 B / Input sequence B (length = Lb)
 * @param out 输出序列 / Output sequence
 * @return 0 表示成功；非 0 表示错误。
 *
 * @note
 * - 若 La == 0 或 Lb == 0，则输出为空序列。
 * - 输出长度为 La + Lb - 1。
 * - 定义: y[n] = sum_{k=0}^{La-1} a[k] * b[n-k]，只在合法索引内累加。
 */
int seq_conv_linear(const seq_t *a, const seq_t *b, seq_t *out)
{
    if (!a || !b || !out)
    {
        fprintf(stderr, "seq_conv_linear: null pointer argument.\n");
        return -1;
    }

    if (a->length == 0 || b->length == 0)
    {
        if (seq_init(out, 0) != 0)
        {
            fprintf(stderr, "seq_conv_linear: failed to initialize empty output.\n");
            return -1;
        }
        return 0;
    }

    size_t la = a->length;
    size_t lb = b->length;
    size_t ly = la + lb - 1;

    if (seq_init(out, ly) != 0)
    {
        fprintf(stderr, "seq_conv_linear: failed to allocate output.\n");
        return -1;
    }

    for (size_t n = 0; n < ly; ++n)
    {
        double acc = 0.0;
        /* k runs over indices of a; n-k must be valid index of b */
        size_t k_min = (n >= lb - 1) ? (n - (lb - 1)) : 0;
        size_t k_max = (n < la - 1) ? n : (la - 1);

        for (size_t k = k_min; k <= k_max; ++k)
        {
            size_t j = n - k; /* index into b */
            acc += (double)a->data[k] * (double)b->data[j];
        }

        out->data[n] = (seq_sample_t)acc;
    }

    return 0;
}

/**
 * @brief 圆周卷积 / Circular convolution of two sequences.
 *
 * @param a 输入序列 A / Input sequence A
 * @param b 输入序列 B / Input sequence B
 * @param out 输出序列 / Output sequence
 * @return 0 表示成功；非 0 表示错误。
 *
 * @note
 * - 要求 a->length == b->length == N 且 N > 0，否则视为错误。
 * - 输出长度为 N。
 * - 定义: y[n] = sum_{k=0}^{N-1} a[k] * b[(n - k) mod N]
 */
int seq_conv_circular(const seq_t *a, const seq_t *b, seq_t *out)
{
    if (!a || !b || !out)
    {
        fprintf(stderr, "seq_conv_circular: null pointer argument.\n");
        return -1;
    }

    if (a->length == 0 || b->length == 0)
    {
        fprintf(stderr, "seq_conv_circular: input length must be > 0.\n");
        return -1;
    }

    if (a->length != b->length)
    {
        fprintf(stderr, "seq_conv_circular: input lengths must match (got %zu and %zu).\n",
                a->length, b->length);
        return -1;
    }

    size_t nlen = a->length;

    if (seq_init(out, nlen) != 0)
    {
        fprintf(stderr, "seq_conv_circular: failed to allocate output.\n");
        return -1;
    }

    for (size_t n = 0; n < nlen; ++n)
    {
        double acc = 0.0;
        for (size_t k = 0; k < nlen; ++k)
        {
            size_t j = (n + nlen - k) % nlen; /* (n-k) mod N */
            acc += (double)a->data[k] * (double)b->data[j];
        }
        out->data[n] = (seq_sample_t)acc;
    }

    return 0;
}

/**
 * @brief 互相关 / Cross-correlation between two sequences.
 *
 * @param a 输入序列 x[n] / Input sequence x[n], length La
 * @param b 输入序列 y[n] / Input sequence y[n], length Lb
 * @param out 输出相关序列 / Output correlation sequence
 * @return 0 表示成功；非 0 表示错误。
 *
 * @note
 * - 输出长度为 La + Lb - 1。
 * - 下标 n 对应的滞后量 lag = n - (Lb - 1)。
 *   So lag runs from -(Lb-1) to (La-1).
 * - 定义:
 *   r_xy[lag] = sum_n x[n] * y[n + lag]
 *   在实现中按输出索引 n = lag + (Lb - 1) 展开。
 */
int seq_corr_cross(const seq_t *a, const seq_t *b, seq_t *out)
{
    if (!a || !b || !out)
    {
        fprintf(stderr, "seq_corr_cross: null pointer argument.\n");
        return -1;
    }

    if (a->length == 0 || b->length == 0)
    {
        if (seq_init(out, 0) != 0)
        {
            fprintf(stderr, "seq_corr_cross: failed to initialize empty output.\n");
            return -1;
        }
        return 0;
    }

    size_t la = a->length;
    size_t lb = b->length;
    size_t lr = la + lb - 1;

    if (seq_init(out, lr) != 0)
    {
        fprintf(stderr, "seq_corr_cross: failed to allocate output.\n");
        return -1;
    }

    for (size_t n = 0; n < lr; ++n)
    {
        long lag = (long)n - (long)(lb - 1);
        double acc = 0.0;

        /* sum over k where indices are in range:
         * x index = k
         * y index = k + lag
         */
        size_t k_start = 0;
        if (lag < 0)
        {
            k_start = (size_t)(-lag);
        }
        size_t k_end = la - 1;
        if ((long)k_end + lag > (long)(lb - 1))
        {
            k_end = (size_t)((long)(lb - 1) - lag);
            if ((long)k_end < 0)
            {
                k_end = 0;
            }
        }

        if (k_start <= k_end)
        {
            for (size_t k = k_start; k <= k_end; ++k)
            {
                size_t idx_y = (size_t)((long)k + lag);
                acc += (double)a->data[k] * (double)b->data[idx_y];
            }
        }

        out->data[n] = (seq_sample_t)acc;
    }

    return 0;
}

/**
 * @brief 滑动窗口归一化相关系数 / Normalized correlation coefficient on sliding windows.
 *
 * @param wa 窗口 A / Window A
 * @param wb 窗口 B / Window B
 * @param out 输出相关系数指针 / Output correlation coefficient
 * @return 0 表示成功；非 0 表示错误。
 *
 * @note
 * - 计算当前两个窗口内样本的皮尔逊相关系数(Pearson correlation)：
 *     ρ = Σ (xa_i - mx) (yb_i - my)
 *         / sqrt( Σ (xa_i - mx)^2 * Σ (yb_i - my)^2 )
 * - 使用 L = min(wa->count, wb->count) 个样本（对齐为各自窗口的“最新 L 个”）。
 * - 要求 L > 0。
 * - 若任一方方差为 0（能量为 0），则无法归一化，视为错误。
 */
int seq_corr_window_norm(
    const seq_window_t *wa,
    const seq_window_t *wb,
    seq_sample_t *out)
{
    if (!wa || !wb || !out)
    {
        fprintf(stderr, "seq_corr_window_norm: null pointer argument.\n");
        return -1;
    }

    if (!wa->buf || !wb->buf || wa->capacity == 0 || wb->capacity == 0)
    {
        fprintf(stderr, "seq_corr_window_norm: invalid window buffer.\n");
        return -1;
    }

    size_t la = wa->count;
    size_t lb = wb->count;
    if (la == 0 || lb == 0)
    {
        fprintf(stderr, "seq_corr_window_norm: window is empty.\n");
        return -1;
    }

    size_t L = (la < lb) ? la : lb;
    if (L == 0)
    {
        fprintf(stderr, "seq_corr_window_norm: effective length is zero.\n");
        return -1;
    }

    /* 为了保持实现简单，我们使用“各自窗口中最旧的 L 个样本按时间对齐”。
     * In practice for streaming,通常 wa、wb 同样大小、同步更新，此定义是自然的。 */
    size_t offset_a = (la > L) ? (la - L) : 0;
    size_t offset_b = (lb > L) ? (lb - L) : 0;

    double sum_x = 0.0, sum_y = 0.0;

    for (size_t i = 0; i < L; ++i)
    {
        double xa = (double)seq_window_get(wa, offset_a + i);
        double yb = (double)seq_window_get(wb, offset_b + i);
        sum_x += xa;
        sum_y += yb;
    }

    double mx = sum_x / (double)L;
    double my = sum_y / (double)L;

    double num = 0.0;
    double sx2 = 0.0;
    double sy2 = 0.0;

    for (size_t i = 0; i < L; ++i)
    {
        double xa = (double)seq_window_get(wa, offset_a + i);
        double yb = (double)seq_window_get(wb, offset_b + i);
        double dx = xa - mx;
        double dy = yb - my;
        num += dx * dy;
        sx2 += dx * dx;
        sy2 += dy * dy;
    }

    if (sx2 <= 0.0 || sy2 <= 0.0)
    {
        fprintf(stderr, "seq_corr_window_norm: zero variance in window, cannot normalize.\n");
        return -1;
    }

    double denom = sqrt(sx2 * sy2);
    if (denom == 0.0)
    {
        fprintf(stderr, "seq_corr_window_norm: zero denominator.\n");
        return -1;
    }

    *out = (seq_sample_t)(num / denom);
    return 0;
}
