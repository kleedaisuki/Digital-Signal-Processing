#include "sequence.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * @brief 内部错误日志（英文，输出到 stderr）。Internal error log (English, stderr).
 *
 * @param msg [in] 错误消息。Error message.
 */
static void seq_log_error(const char *msg)
{
    if (!msg)
    {
        return;
    }
    fprintf(stderr, "[seq] error: %s\n", msg);
}

/**
 * @brief 为输出序列准备内存。Prepare memory for output sequence.
 *
 * @param dst [in,out] 输出序列。Output sequence.
 * @param length [in] 目标长度。Target length.
 * @return 错误码。Error code.
 */
static seq_err_t seq_prepare_output(seq_t *dst, size_t length)
{
    if (!dst)
    {
        seq_log_error("seq_prepare_output: null dst");
        return SEQ_ERR_ARG;
    }
    if (!dst->data || dst->length != length)
    {
        seq_free(dst);
        return seq_alloc(dst, length);
    }
    return SEQ_OK;
}

/**
 * @brief 重置流式状态到初始值，但不假设其已被初始化。
 *        Reset streaming state to initial values without assuming prior init.
 *
 * @param st [in,out] 状态。State.
 */
static void seq_stream_reset(seq_stream_t *st)
{
    if (!st)
    {
        return;
    }

    /* 不在这里 free(st->buf)，因为 seq_stream_init 可能被用于未初始化的栈变量。
     * 真正的资源释放统一由 seq_stream_dispose 负责。
     * Do NOT free(st->buf) here because seq_stream_init may be called on
     * an uninitialized stack variable. Real cleanup is done in seq_stream_dispose.
     */

    st->active = 0;
    st->op = SEQ_OP_PAD_FRONT;
    st->param_main = 0;
    st->param_aux = 0;
    st->fill = 0.0;

    st->buf = NULL;
    st->buf_size = 0;
    st->buf_head = 0;

    st->counter = 0;
    st->remaining = 0;

    st->last = 0.0;
    st->has_last = 0;

    st->acc = 0.0;
}

seq_err_t seq_alloc(seq_t *seq, size_t length)
{
    if (!seq)
    {
        seq_log_error("seq_alloc: null seq pointer");
        return SEQ_ERR_ARG;
    }
    seq->data = NULL;
    seq->length = 0;

    if (length == 0)
    {
        return SEQ_OK;
    }

    seq->data = (double *)calloc(length, sizeof(double));
    if (!seq->data)
    {
        seq_log_error("seq_alloc: out of memory");
        return SEQ_ERR_NOMEM;
    }
    seq->length = length;
    return SEQ_OK;
}

void seq_free(seq_t *seq)
{
    if (!seq)
    {
        return;
    }
    free(seq->data);
    seq->data = NULL;
    seq->length = 0;
}

seq_err_t seq_copy(const seq_t *src, seq_t *dst)
{
    if (!src || !dst)
    {
        seq_log_error("seq_copy: null pointer");
        return SEQ_ERR_ARG;
    }
    if (src->length == 0)
    {
        seq_free(dst);
        return SEQ_OK;
    }

    if (!dst->data || dst->length != src->length)
    {
        seq_free(dst);
        if (seq_alloc(dst, src->length) != SEQ_OK)
        {
            return SEQ_ERR_NOMEM;
        }
    }

    if (!src->data || !dst->data)
    {
        seq_log_error("seq_copy: null data pointer");
        return SEQ_ERR_ARG;
    }

    memcpy(dst->data, src->data, src->length * sizeof(double));
    return SEQ_OK;
}

seq_err_t seq_pad_front(const seq_t *src, size_t zeros, seq_t *dst)
{
    if (!src || !dst)
    {
        seq_log_error("seq_pad_front: null pointer");
        return SEQ_ERR_ARG;
    }

    const size_t out_len = src->length + zeros;
    seq_err_t err = seq_prepare_output(dst, out_len);
    if (err != SEQ_OK)
    {
        return err;
    }

    size_t i = 0;
    while (i < zeros)
    {
        dst->data[i++] = 0.0;
    }
    size_t j = 0;
    while (j < src->length)
    {
        dst->data[i++] = src->data[j++];
    }
    return SEQ_OK;
}

seq_err_t seq_pad_back(const seq_t *src, size_t zeros, seq_t *dst)
{
    if (!src || !dst)
    {
        seq_log_error("seq_pad_back: null pointer");
        return SEQ_ERR_ARG;
    }

    const size_t out_len = src->length + zeros;
    seq_err_t err = seq_prepare_output(dst, out_len);
    if (err != SEQ_OK)
    {
        return err;
    }

    size_t i = 0;
    while (i < src->length)
    {
        dst->data[i] = src->data[i];
        i++;
    }
    while (i < out_len)
    {
        dst->data[i] = 0.0;
        i++;
    }
    return SEQ_OK;
}

seq_err_t seq_delay(const seq_t *src, size_t delay, double fill, seq_t *dst)
{
    if (!src || !dst)
    {
        seq_log_error("seq_delay: null pointer");
        return SEQ_ERR_ARG;
    }

    seq_err_t err = seq_prepare_output(dst, src->length);
    if (err != SEQ_OK)
    {
        return err;
    }

    size_t i = 0;
    while (i < src->length)
    {
        if (i < delay)
        {
            dst->data[i] = fill;
        }
        else
        {
            dst->data[i] = src->data[i - delay];
        }
        i++;
    }
    return SEQ_OK;
}

seq_err_t seq_advance(const seq_t *src, size_t advance, double fill, seq_t *dst)
{
    if (!src || !dst)
    {
        seq_log_error("seq_advance: null pointer");
        return SEQ_ERR_ARG;
    }

    seq_err_t err = seq_prepare_output(dst, src->length);
    if (err != SEQ_OK)
    {
        return err;
    }

    size_t i = 0;
    while (i < src->length)
    {
        size_t idx = i + advance;
        if (idx < src->length)
        {
            dst->data[i] = src->data[idx];
        }
        else
        {
            dst->data[i] = fill;
        }
        i++;
    }
    return SEQ_OK;
}

seq_err_t seq_reverse(const seq_t *src, seq_t *dst)
{
    if (!src || !dst)
    {
        seq_log_error("seq_reverse: null pointer");
        return SEQ_ERR_ARG;
    }

    seq_err_t err = seq_prepare_output(dst, src->length);
    if (err != SEQ_OK)
    {
        return err;
    }

    if (src->length == 0)
    {
        return SEQ_OK;
    }

    size_t i = 0;
    size_t j = src->length - 1;
    while (i < src->length)
    {
        dst->data[i] = src->data[j];
        if (j == 0)
        {
            break;
        }
        i++;
        j--;
    }
    return SEQ_OK;
}

seq_err_t seq_upsample(const seq_t *src, size_t factor, seq_t *dst)
{
    if (!src || !dst)
    {
        seq_log_error("seq_upsample: null pointer");
        return SEQ_ERR_ARG;
    }
    if (factor == 0)
    {
        seq_log_error("seq_upsample: factor must be > 0");
        return SEQ_ERR_ARG;
    }

    const size_t out_len = src->length * factor;
    seq_err_t err = seq_prepare_output(dst, out_len);
    if (err != SEQ_OK)
    {
        return err;
    }

    size_t n = 0;
    while (n < out_len)
    {
        dst->data[n] = 0.0;
        n++;
    }

    size_t i = 0;
    while (i < src->length)
    {
        dst->data[i * factor] = src->data[i];
        i++;
    }
    return SEQ_OK;
}

seq_err_t seq_downsample(const seq_t *src, size_t factor, seq_t *dst)
{
    if (!src || !dst)
    {
        seq_log_error("seq_downsample: null pointer");
        return SEQ_ERR_ARG;
    }
    if (factor == 0)
    {
        seq_log_error("seq_downsample: factor must be > 0");
        return SEQ_ERR_ARG;
    }

    const size_t out_len = src->length / factor;
    seq_err_t err = seq_prepare_output(dst, out_len);
    if (err != SEQ_OK)
    {
        return err;
    }

    size_t i = 0;
    while (i < out_len)
    {
        dst->data[i] = src->data[i * factor];
        i++;
    }
    return SEQ_OK;
}

seq_err_t seq_diff(const seq_t *src, seq_t *dst)
{
    if (!src || !dst)
    {
        seq_log_error("seq_diff: null pointer");
        return SEQ_ERR_ARG;
    }

    seq_err_t err = seq_prepare_output(dst, src->length);
    if (err != SEQ_OK)
    {
        return err;
    }

    if (src->length == 0)
    {
        return SEQ_OK;
    }

    dst->data[0] = src->data[0];
    size_t i = 1;
    while (i < src->length)
    {
        dst->data[i] = src->data[i] - src->data[i - 1];
        i++;
    }
    return SEQ_OK;
}

seq_err_t seq_cumsum(const seq_t *src, seq_t *dst)
{
    if (!src || !dst)
    {
        seq_log_error("seq_cumsum: null pointer");
        return SEQ_ERR_ARG;
    }

    seq_err_t err = seq_prepare_output(dst, src->length);
    if (err != SEQ_OK)
    {
        return err;
    }

    double acc = 0.0;
    size_t i = 0;
    while (i < src->length)
    {
        acc += src->data[i];
        dst->data[i] = acc;
        i++;
    }
    return SEQ_OK;
}

int seq_online_capable(seq_op_type op, int infinite_input)
{
    if (infinite_input)
    {
        switch (op)
        {
        case SEQ_OP_PAD_FRONT:
        case SEQ_OP_DELAY:
        case SEQ_OP_UPSAMPLE:
        case SEQ_OP_DOWNSAMPLE:
        case SEQ_OP_DIFF:
        case SEQ_OP_CUMSUM:
            return 1;
        case SEQ_OP_PAD_BACK:
        case SEQ_OP_ADVANCE:
        case SEQ_OP_REVERSE:
        default:
            return 0;
        }
    }

    /* 对有限输入：只要需要有限存储即可实现，全部认为可行。        */
    /* For finite input: finite-memory realizable ops are considered feasible. */
    switch (op)
    {
    case SEQ_OP_PAD_FRONT:
    case SEQ_OP_PAD_BACK:
    case SEQ_OP_DELAY:
    case SEQ_OP_ADVANCE:
    case SEQ_OP_REVERSE:
    case SEQ_OP_UPSAMPLE:
    case SEQ_OP_DOWNSAMPLE:
    case SEQ_OP_DIFF:
    case SEQ_OP_CUMSUM:
        return 1;
    default:
        return 0;
    }
}

seq_err_t seq_stream_init(seq_stream_t *st,
                          seq_op_type op,
                          size_t param_main,
                          size_t param_aux,
                          double fill)
{
    (void)param_aux;

    if (!st)
    {
        seq_log_error("seq_stream_init: null state");
        return SEQ_ERR_ARG;
    }

    seq_stream_reset(st);
    st->op = op;
    st->fill = fill;

    switch (op)
    {
    case SEQ_OP_PAD_FRONT:
        st->param_main = param_main;
        st->remaining = param_main;
        st->active = 1;
        return SEQ_OK;

    case SEQ_OP_DELAY:
        st->param_main = param_main;
        if (param_main == 0)
        {
            st->active = 1;
            return SEQ_OK;
        }
        st->buf = (double *)malloc(param_main * sizeof(double));
        if (!st->buf)
        {
            seq_log_error("seq_stream_init: delay buffer oom");
            return SEQ_ERR_NOMEM;
        }
        st->buf_size = param_main;
        st->buf_head = 0;
        {
            size_t i = 0;
            while (i < st->buf_size)
            {
                st->buf[i] = fill;
                i++;
            }
        }
        st->active = 1;
        return SEQ_OK;

    case SEQ_OP_UPSAMPLE:
        if (param_main == 0)
        {
            seq_log_error("seq_stream_init: upsample factor must be > 0");
            return SEQ_ERR_ARG;
        }
        st->param_main = param_main;
        st->remaining = 0;
        st->active = 1;
        return SEQ_OK;

    case SEQ_OP_DOWNSAMPLE:
        if (param_main == 0)
        {
            seq_log_error("seq_stream_init: downsample factor must be > 0");
            return SEQ_ERR_ARG;
        }
        st->param_main = param_main;
        st->counter = 0;
        st->active = 1;
        return SEQ_OK;

    case SEQ_OP_DIFF:
        st->has_last = 0;
        st->active = 1;
        return SEQ_OK;

    case SEQ_OP_CUMSUM:
        st->acc = 0.0;
        st->active = 1;
        return SEQ_OK;

    case SEQ_OP_PAD_BACK:
    case SEQ_OP_ADVANCE:
    case SEQ_OP_REVERSE:
    default:
        seq_log_error("seq_stream_init: unsupported op for streaming");
        return SEQ_ERR_UNSUPPORTED;
    }
}

seq_err_t seq_stream_step(seq_stream_t *st,
                          int has_input,
                          double x,
                          double *y,
                          int *has_output)
{
    if (!st || !has_output)
    {
        seq_log_error("seq_stream_step: null pointer");
        return SEQ_ERR_ARG;
    }
    if (!st->active)
    {
        seq_log_error("seq_stream_step: state not active");
        return SEQ_ERR_STATE;
    }

    *has_output = 0;

    switch (st->op)
    {
    case SEQ_OP_PAD_FRONT:
        if (st->remaining > 0)
        {
            if (has_input)
            {
                seq_log_error("seq_stream_step: input not allowed during front padding");
                return SEQ_ERR_STATE;
            }
            if (y)
            {
                *y = 0.0;
            }
            st->remaining--;
            *has_output = 1;
            return SEQ_OK;
        }
        if (has_input && y)
        {
            *y = x;
            *has_output = 1;
        }
        return SEQ_OK;

    case SEQ_OP_DELAY:
        if (!has_input)
        {
            return SEQ_OK;
        }
        if (!y)
        {
            return SEQ_ERR_ARG;
        }
        if (st->param_main == 0)
        {
            *y = x;
            *has_output = 1;
            return SEQ_OK;
        }
        *y = st->buf[st->buf_head];
        st->buf[st->buf_head] = x;
        st->buf_head++;
        if (st->buf_head >= st->buf_size)
        {
            st->buf_head = 0;
        }
        *has_output = 1;
        return SEQ_OK;

    case SEQ_OP_UPSAMPLE:
        if (st->remaining > 0)
        {
            if (has_input)
            {
                seq_log_error("seq_stream_step: input not allowed while flushing upsample zeros");
                return SEQ_ERR_STATE;
            }
            if (y)
            {
                *y = 0.0;
            }
            st->remaining--;
            *has_output = 1;
            return SEQ_OK;
        }
        if (!has_input)
        {
            return SEQ_OK;
        }
        if (!y)
        {
            return SEQ_ERR_ARG;
        }
        *y = x;
        *has_output = 1;
        if (st->param_main > 1)
        {
            st->remaining = st->param_main - 1;
        }
        return SEQ_OK;

    case SEQ_OP_DOWNSAMPLE:
        if (!has_input)
        {
            return SEQ_OK;
        }
        if (st->param_main == 0)
        {
            seq_log_error("seq_stream_step: invalid downsample factor");
            return SEQ_ERR_STATE;
        }
        if ((st->counter % st->param_main) == 0)
        {
            if (!y)
            {
                return SEQ_ERR_ARG;
            }
            *y = x;
            *has_output = 1;
        }
        st->counter++;
        return SEQ_OK;

    case SEQ_OP_DIFF:
        if (!has_input)
        {
            return SEQ_OK;
        }
        if (!y)
        {
            return SEQ_ERR_ARG;
        }
        if (!st->has_last)
        {
            *y = x;
            st->last = x;
            st->has_last = 1;
        }
        else
        {
            *y = x - st->last;
            st->last = x;
        }
        *has_output = 1;
        return SEQ_OK;

    case SEQ_OP_CUMSUM:
        if (!has_input)
        {
            return SEQ_OK;
        }
        if (!y)
        {
            return SEQ_ERR_ARG;
        }
        st->acc += x;
        *y = st->acc;
        *has_output = 1;
        return SEQ_OK;

    case SEQ_OP_PAD_BACK:
    case SEQ_OP_ADVANCE:
    case SEQ_OP_REVERSE:
    default:
        seq_log_error("seq_stream_step: unsupported op");
        return SEQ_ERR_UNSUPPORTED;
    }
}

void seq_stream_dispose(seq_stream_t *st)
{
    if (!st)
    {
        return;
    }
    free(st->buf);
    st->buf = NULL;
    st->buf_size = 0;
    st->buf_head = 0;
    st->active = 0;
    st->param_main = 0;
    st->param_aux = 0;
    st->fill = 0.0;
    st->counter = 0;
    st->remaining = 0;
    st->last = 0.0;
    st->has_last = 0;
    st->acc = 0.0;
}
