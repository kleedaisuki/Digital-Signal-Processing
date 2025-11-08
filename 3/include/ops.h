/**
 * @file ops.h
 * @brief 序列运算接口 (Sequence operation interface)
 */

#ifndef OPS_H
#define OPS_H

#include "seq.h"

/* 加法 / Addition */
int seq_add(const seq_t *a, const seq_t *b, seq_t *out);

/* 乘法 / Multiplication */
int seq_mul(const seq_t *a, const seq_t *b, seq_t *out);

/* 线性卷积 / Linear convolution */
int seq_conv_linear(const seq_t *a, const seq_t *b, seq_t *out);

/* 圆周卷积 / Circular convolution */
int seq_conv_circular(const seq_t *a, const seq_t *b, seq_t *out);

/* 互相关 / Cross-correlation */
int seq_corr_cross(const seq_t *a, const seq_t *b, seq_t *out);

/* 滑动窗口归一化相关 / Normalized correlation (windowed) */
int seq_corr_window_norm(const seq_window_t *wa, const seq_window_t *wb, seq_sample_t *out);

#endif /* OPS_H */
