/**
 * @file seq.c
 * @brief 序列与滑动窗口实现 / Implementation of sequence and sliding window
 */

#include "seq.h"

#include <stdlib.h>
#include <stdio.h>

/**
 * @brief 初始化序列 / Initialize a sequence object.
 *
 * @param s 序列指针，不能为空。/ Sequence pointer, must not be NULL.
 * @param len 要分配的序列长度，可以为 0。/ Length to allocate, can be 0.
 * @return 0 表示成功；非 0 表示参数无效或内存分配失败。
 *         0 on success; non-zero on invalid argument or allocation failure.
 *
 * @note 若 len 为 0，则 data 置为 NULL，length 为 0，视为成功。
 *       If len is 0, data is set to NULL and length to 0; treated as success.
 */
int seq_init(seq_t *s, size_t len)
{
    if (s == NULL)
    {
        fprintf(stderr, "seq_init: sequence pointer is NULL.\n");
        return -1;
    }

    s->data = NULL;
    s->length = 0;

    if (len == 0)
    {
        /* 合法的空序列 / Valid empty sequence */
        return 0;
    }

    s->data = (seq_sample_t *)calloc(len, sizeof(seq_sample_t));
    if (s->data == NULL)
    {
        fprintf(stderr, "seq_init: failed to allocate memory for sequence.\n");
        return -1;
    }

    s->length = len;
    return 0;
}

/**
 * @brief 释放序列占用的内存 / Free memory used by a sequence.
 *
 * @param s 序列指针，可以为 NULL。/ Sequence pointer, can be NULL.
 *
 * @note 调用后 s->data 置为 NULL，length 置为 0。
 *       After the call, s->data is set to NULL and length to 0.
 */
void seq_free(seq_t *s)
{
    if (s == NULL)
        return;

    free(s->data);
    s->data = NULL;
    s->length = 0;
}

/**
 * @brief 初始化滑动窗口 / Initialize a sliding window.
 *
 * @param w 窗口指针，不能为空。/ Window pointer, must not be NULL.
 * @param capacity 窗口容量，必须 > 0。/ Window capacity, must be > 0.
 * @return 0 表示成功；非 0 表示参数无效或内存分配失败。
 *         0 on success; non-zero on invalid argument or allocation failure.
 */
int seq_window_init(seq_window_t *w, size_t capacity)
{
    if (w == NULL)
    {
        fprintf(stderr, "seq_window_init: window pointer is NULL.\n");
        return -1;
    }

    w->buf = NULL;
    w->capacity = 0;
    w->start = 0;
    w->count = 0;

    if (capacity == 0)
    {
        fprintf(stderr, "seq_window_init: capacity must be greater than zero.\n");
        return -1;
    }

    w->buf = (seq_sample_t *)calloc(capacity, sizeof(seq_sample_t));
    if (w->buf == NULL)
    {
        fprintf(stderr, "seq_window_init: failed to allocate buffer.\n");
        return -1;
    }

    w->capacity = capacity;
    /* start = 0, count = 0 已在上面初始化 / already initialized */
    return 0;
}

/**
 * @brief 释放滑动窗口内存 / Free memory used by a sliding window.
 *
 * @param w 窗口指针，可以为 NULL。/ Window pointer, can be NULL.
 *
 * @note 调用后窗口被重置为空状态。/ After the call, window is reset to empty.
 */
void seq_window_free(seq_window_t *w)
{
    if (w == NULL)
        return;

    free(w->buf);
    w->buf = NULL;
    w->capacity = 0;
    w->start = 0;
    w->count = 0;
}

/**
 * @brief 向滑动窗口推入一个新样本 / Push a new sample into the sliding window.
 *
 * @param w 窗口指针，必须已初始化。/ Window pointer, must be initialized.
 * @param x 要插入的样本值。/ Sample value to insert.
 *
 * @note
 * - 若窗口未满，则追加到当前末尾，count 增加。
 * - 若窗口已满，则覆盖最旧样本（FIFO），start 前移一位。
 * - 若传入无效窗口，会打印错误信息到 stderr 并直接返回。
 *   If the window is invalid, an error is printed to stderr and the call is ignored.
 */
void seq_window_push(seq_window_t *w, seq_sample_t x)
{
    if (w == NULL || w->buf == NULL || w->capacity == 0)
    {
        fprintf(stderr, "seq_window_push: invalid window.\n");
        return;
    }

    size_t idx;

    if (w->count < w->capacity)
    {
        /* 还有空位，写在 (start + count) 位置 */
        idx = (w->start + w->count) % w->capacity;
        w->count++;
    }
    else
    {
        /* 已满，覆盖最老元素，即 start 位置，然后 start 前移 */
        idx = w->start;
        w->start = (w->start + 1) % w->capacity;
    }

    w->buf[idx] = x;
}

/**
 * @brief 从滑动窗口中按索引读取样本 / Get a sample from the sliding window by index.
 *
 * @param w 窗口指针，必须已初始化。/ Window pointer, must be initialized.
 * @param i 索引 [0, count) / Index in range [0, count).
 * @return 若参数合法，返回对应样本值；
 *         否则返回 0.0，并在 stderr 打印错误信息。
 *         If valid, returns the sample; otherwise returns 0.0 and prints an error.
 *
 * @note 该实现进行了边界检查，以避免静默越界错误。
 *       This implementation performs bounds checking to avoid silent out-of-range access.
 */
seq_sample_t seq_window_get(const seq_window_t *w, size_t i)
{
    if (w == NULL || w->buf == NULL || w->capacity == 0)
    {
        fprintf(stderr, "seq_window_get: invalid window.\n");
        return 0.0;
    }

    if (i >= w->count)
    {
        fprintf(stderr, "seq_window_get: index out of range (i=%zu, count=%zu).\n",
                i, w->count);
        return 0.0;
    }

    size_t idx = (w->start + i) % w->capacity;
    return w->buf[idx];
}
