#ifndef SEQUENCE_H
#define SEQUENCE_H

#include <stddef.h>

/**
 * @brief 序列操作错误码。Sequence operation error codes.
 */
typedef enum
{
    SEQ_OK = 0,             /**< 成功。Success. */
    SEQ_ERR_ARG = 1,        /**< 参数错误。Invalid argument. */
    SEQ_ERR_NOMEM = 2,      /**< 内存不足。Out of memory. */
    SEQ_ERR_STATE = 3,      /**< 状态错误。Invalid state. */
    SEQ_ERR_UNSUPPORTED = 4 /**< 不支持的操作。Unsupported operation. */
} seq_err_t;

/**
 * @brief 序列操作类型。Sequence operation types.
 */
typedef enum
{
    SEQ_OP_PAD_FRONT = 0, /**< 前补零。Zero padding at front. */
    SEQ_OP_PAD_BACK,      /**< 后补零。Zero padding at back. */
    SEQ_OP_DELAY,         /**< 延迟。Delay. */
    SEQ_OP_ADVANCE,       /**< 提前。Advance. */
    SEQ_OP_REVERSE,       /**< 反转。Reverse. */
    SEQ_OP_UPSAMPLE,      /**< 上采样。Upsample. */
    SEQ_OP_DOWNSAMPLE,    /**< 下采样。Downsample. */
    SEQ_OP_DIFF,          /**< 差分。Difference. */
    SEQ_OP_CUMSUM         /**< 累加。Cumulative sum. */
} seq_op_type;

/**
 * @brief 离散序列结构体。Discrete-time sequence structure.
 *
 * @note data 指向 length 个 double。data points to length doubles.
 */
typedef struct
{
    double *data;  /**< 数据指针。Pointer to data. */
    size_t length; /**< 元素个数。Number of elements. */
} seq_t;

/**
 * @brief 流式（随来随处理）状态。Streaming (online) processing state.
 *
 * @note 此结构应视为不透明，仅通过提供的 API 操作。Treat as opaque; use only via API.
 */
typedef struct
{
    int active;     /**< 状态是否已初始化。Whether state is initialized. */
    seq_op_type op; /**< 操作类型。Operation type. */

    size_t param_main; /**< 主参数，如补零个数、延迟量、因子等。Main parameter. */
    size_t param_aux;  /**< 辅助参数，保留扩展用。Aux parameter, reserved. */
    double fill;       /**< 边界填充值。Boundary fill value. */

    double *buf;     /**< 循环缓冲区或内部存储。Internal buffer. */
    size_t buf_size; /**< 缓冲区容量。Buffer size. */
    size_t buf_head; /**< 缓冲读写位置。Buffer index. */

    size_t counter;   /**< 计数器，用于采样因子等。Counter. */
    size_t remaining; /**< 剩余要输出的样本数量，用于补零等。Remaining outputs. */

    double last;  /**< 上一个样本，用于差分等。Last sample. */
    int has_last; /**< 是否已有上一个样本。Whether last is valid. */

    double acc; /**< 累加器，用于前缀和。Accumulator. */
} seq_stream_t;

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief 分配指定长度序列。Allocate sequence of given length.
     *
     * @param seq [out] 输出序列结构体。Output sequence struct.
     * @param length [in] 序列长度（元素个数）。Sequence length in elements.
     * @return SEQ_OK 成功；或错误码。SEQ_OK on success, otherwise error code.
     */
    seq_err_t seq_alloc(seq_t *seq, size_t length);

    /**
     * @brief 释放序列内存并重置。Free sequence memory and reset.
     *
     * @param seq [in,out] 要释放的序列。Sequence to free.
     */
    void seq_free(seq_t *seq);

    /**
     * @brief 拷贝序列内容。Copy sequence contents.
     *
     * @param src [in] 源序列。Source sequence.
     * @param dst [out] 目标序列；若 data 为空将自动分配。Destination sequence; allocated if data is NULL.
     * @return SEQ_OK 成功；或错误码。SEQ_OK on success, otherwise error code.
     */
    seq_err_t seq_copy(const seq_t *src, seq_t *dst);

    /**
     * @brief 前补零操作（离线）。Zero pad at front (offline).
     *
     * @param src [in] 输入序列。Input sequence.
     * @param zeros [in] 补零个数。Number of zeros to prepend.
     * @param dst [out] 输出序列。Output sequence.
     * @return SEQ_OK 或错误码。SEQ_OK or error code.
     */
    seq_err_t seq_pad_front(const seq_t *src, size_t zeros, seq_t *dst);

    /**
     * @brief 后补零操作（离线）。Zero pad at back (offline).
     *
     * @param src [in] 输入序列。Input sequence.
     * @param zeros [in] 补零个数。Number of zeros to append.
     * @param dst [out] 输出序列。Output sequence.
     * @return SEQ_OK 或错误码。SEQ_OK or error code.
     */
    seq_err_t seq_pad_back(const seq_t *src, size_t zeros, seq_t *dst);

    /**
     * @brief 序列延迟（离线）：y[n] = x[n-delay]，超出部分使用 fill。Delay sequence (offline).
     *
     * @param src 输入序列。Input sequence.
     * @param delay 延迟样本数。Delay in samples.
     * @param fill 边界填充值。Boundary fill value.
     * @param dst 输出序列。Output sequence.
     */
    seq_err_t seq_delay(const seq_t *src, size_t delay, double fill, seq_t *dst);

    /**
     * @brief 序列提前（离线）：y[n] = x[n+advance]，超出部分使用 fill。Advance sequence (offline).
     *
     * @param src 输入序列。Input sequence.
     * @param advance 提前样本数。Advance in samples.
     * @param fill 边界填充值。Boundary fill value.
     * @param dst 输出序列。Output sequence.
     */
    seq_err_t seq_advance(const seq_t *src, size_t advance, double fill, seq_t *dst);

    /**
     * @brief 序列反转（离线）。Reverse sequence (offline).
     *
     * @param src 输入序列。Input sequence.
     * @param dst 输出序列。Output sequence.
     */
    seq_err_t seq_reverse(const seq_t *src, seq_t *dst);

    /**
     * @brief 上采样（离线）：插入 (factor-1) 个 0。Upsample (offline) with zero insertion.
     *
     * @param src 输入序列。Input sequence.
     * @param factor 上采样因子 (>0)。Upsampling factor (>0).
     * @param dst 输出序列。Output sequence.
     */
    seq_err_t seq_upsample(const seq_t *src, size_t factor, seq_t *dst);

    /**
     * @brief 下采样（离线）：每 factor 个取一个（从 0 开始）。Downsample (offline).
     *
     * @param src 输入序列。Input sequence.
     * @param factor 下采样因子 (>0)。Downsampling factor (>0).
     * @param dst 输出序列。Output sequence.
     */
    seq_err_t seq_downsample(const seq_t *src, size_t factor, seq_t *dst);

    /**
     * @brief 差分（离线）：y[n] = x[n] - x[n-1]，x[-1] 视为 0。Difference (offline).
     *
     * @param src 输入序列。Input sequence.
     * @param dst 输出序列。Output sequence.
     */
    seq_err_t seq_diff(const seq_t *src, seq_t *dst);

    /**
     * @brief 累加（离线）：前缀和。Cumulative sum (offline).
     *
     * @param src 输入序列。Input sequence.
     * @param dst 输出序列。Output sequence.
     */
    seq_err_t seq_cumsum(const seq_t *src, seq_t *dst);

    /**
     * @brief 判断操作在给定条件下是否支持随来随处理。Check if op supports online streaming.
     *
     * @details infinite_input!=0 时，以可能无限长输入为前提，仅当操作可由有界状态的因果系统实现时返回非 0；
     *          infinite_input==0 时，只要可在有限存储下对有限序列完成，即返回非 0。
     *          When infinite_input!=0, returns non-zero only for causal, bounded-state ops suitable
     *          for potentially infinite input. When infinite_input==0, returns non-zero for ops
     *          implementable on finite input with finite memory.
     *
     * @param op [in] 操作类型。Operation type.
     * @param infinite_input [in] 非 0 表示可能为无限长输入。Non-zero if input may be infinite.
     * @return 非 0 表示支持；0 表示不支持。Non-zero if supported, 0 otherwise.
     */
    int seq_online_capable(seq_op_type op, int infinite_input);

    /**
     * @brief 初始化流式状态。Initialize streaming state.
     *
     * @param st [out] 状态对象。State object.
     * @param op [in] 操作类型。Operation type.
     * @param param_main [in] 主参数，如补零个数、延迟量、因子。Main parameter.
     * @param param_aux [in] 辅助参数，当前保留为 0。Aux parameter, reserved.
     * @param fill [in] 边界填充值，部分操作会使用。Boundary fill value.
     * @return SEQ_OK 或错误码。SEQ_OK or error code.
     *
     * @example
     * // 例：初始化差分的流式处理。
     * // Example: init streaming difference.
     * seq_stream_t st;
     * seq_stream_init(&st, SEQ_OP_DIFF, 0, 0, 0.0);
     */
    seq_err_t seq_stream_init(seq_stream_t *st,
                              seq_op_type op,
                              size_t param_main,
                              size_t param_aux,
                              double fill);

    /**
     * @brief 流式处理一步。One step of streaming processing.
     *
     * @param st [in,out] 流式状态。Streaming state.
     * @param has_input [in] 非 0 表示本次提供输入样本 x；0 表示仅请求额外输出（用于补零/上采样等）。
     *                       Non-zero if x is valid; 0 means request extra output only.
     * @param x [in] 输入样本，仅当 has_input 非 0 时有效。Input sample.
     * @param y [out] 若有输出，写入此处。If output exists, written here.
     * @param has_output [out] 非 0 表示 y 有效；0 表示本次无输出。Non-zero if y is valid.
     * @return SEQ_OK 或错误码。SEQ_OK or error code.
     *
     * @note 输入结束后，调用者可使用 has_input=0 重复调用以获取尾部输出（如上采样插入零）。
     *       After input ends, caller may call with has_input=0 to flush remaining outputs.
     */
    seq_err_t seq_stream_step(seq_stream_t *st,
                              int has_input,
                              double x,
                              double *y,
                              int *has_output);

    /**
     * @brief 释放流式状态中的内部资源。Free resources in streaming state.
     *
     * @param st [in,out] 流式状态。Streaming state.
     */
    void seq_stream_dispose(seq_stream_t *st);

#ifdef __cplusplus
}
#endif

#endif /* SEQUENCE_H */
