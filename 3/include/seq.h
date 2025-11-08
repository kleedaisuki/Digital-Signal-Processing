/**
 * @file seq.h
 * @brief 序列与滑动窗口接口 (Sequence & Sliding Window Interface)
 */

#ifndef SEQ_H
#define SEQ_H

#include <stddef.h>

typedef double seq_sample_t;

/**
 * @brief 序列结构 (Sequence object)
 */
typedef struct
{
    seq_sample_t *data; /**< 数据指针 / pointer to data */
    size_t length;      /**< 序列长度 / sequence length */
} seq_t;

/**
 * @brief 滑动窗口结构 (Sliding window for streaming operations)
 */
typedef struct
{
    seq_sample_t *buf; /**< 环形缓冲区 / circular buffer */
    size_t capacity;   /**< 最大容量 / max window length */
    size_t start;      /**< 当前起点索引 / start index */
    size_t count;      /**< 当前元素数量 / current element count */
} seq_window_t;

/* === 接口声明 (Function declarations) === */
int seq_init(seq_t *s, size_t len);
void seq_free(seq_t *s);

int seq_window_init(seq_window_t *w, size_t capacity);
void seq_window_free(seq_window_t *w);
void seq_window_push(seq_window_t *w, seq_sample_t x);
seq_sample_t seq_window_get(const seq_window_t *w, size_t i);

#endif /* SEQ_H */
