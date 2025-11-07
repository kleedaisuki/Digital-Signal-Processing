#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/**
 * @brief 输入结束标记（停止记号）字符串；用户输入该字符串时结束序列输入。Stop token string; inputting this ends the sequence input.
 */
#define STOP_TOKEN "STOP"

/**
 * @brief 初始容量；用于可扩展序列的起始分配大小。Initial capacity for dynamic sequence allocation.
 */
#define INITIAL_CAPACITY 8

/**
 * @brief 信号序列结构体，支持自定义起始下标和动态长度。Signal sequence with custom start index and dynamic length.
 */
typedef struct
{
    int start;        /**< 序列起始下标；可以为负数。Start index, can be negative. */
    int length;       /**< 当前已使用长度。Current logical length. */
    int capacity;     /**< 已分配容量。Allocated capacity. */
    int allow_expand; /**< 是否允许扩展（1=可无限追加，0=固定长度）。Whether expansion is allowed. */
    double *data;     /**< 实际数据存储区。Underlying data buffer. */
} SignalSeq;

/**
 * @brief 初始化信号序列。Initialize a signal sequence.
 *
 * @param seq [in,out] 序列指针；必须为非空。Pointer to sequence; must not be NULL.
 * @param start [in] 起始下标（可为负）。Start index (may be negative).
 * @param initial_length [in] 初始长度（固定模式下为目标长度；可扩展模式下通常为0）。Initial length.
 * @param allow_expand [in] 是否允许扩展（非0为可扩展）。Whether expansion is allowed.
 * @return 0 表示成功；非0表示失败。0 on success; non-zero on failure.
 * @note 若失败，seq->data 将为 NULL。On failure, seq->data will be NULL.
 */
static int signal_seq_init(SignalSeq *seq, int start, int initial_length, int allow_expand)
{
    if (seq == NULL)
    {
        fprintf(stderr, "Error: signal_seq_init received NULL pointer.\n");
        return -1;
    }

    if (initial_length < 0)
    {
        fprintf(stderr, "Error: initial length must be non-negative.\n");
        return -1;
    }

    int capacity = initial_length;
    if (allow_expand)
    {
        if (capacity < INITIAL_CAPACITY)
        {
            capacity = INITIAL_CAPACITY;
        }
    }

    double *buffer = NULL;
    if (capacity > 0)
    {
        buffer = (double *)malloc((size_t)capacity * sizeof(double));
        if (buffer == NULL)
        {
            fprintf(stderr, "Error: failed to allocate memory for sequence.\n");
            return -1;
        }
    }

    seq->start = start;
    seq->length = initial_length;
    seq->capacity = capacity;
    seq->allow_expand = allow_expand ? 1 : 0;
    seq->data = buffer;

    return 0;
}

/**
 * @brief 释放信号序列占用的内存。Free resources owned by a signal sequence.
 *
 * @param seq [in,out] 需要释放的序列；可为NULL。Sequence to free; may be NULL.
 * @note 调用后 seq->data 置为 NULL，长度与容量清零。Data pointer will be NULL after this call.
 */
static void signal_seq_free(SignalSeq *seq)
{
    if (seq == NULL)
    {
        return;
    }
    if (seq->data != NULL)
    {
        free(seq->data);
        seq->data = NULL;
    }
    seq->start = 0;
    seq->length = 0;
    seq->capacity = 0;
    seq->allow_expand = 0;
}

/**
 * @brief 将逻辑下标转换为物理下标。Map logical index to physical index.
 *
 * @param seq [in] 序列指针。Sequence pointer.
 * @param logical_index [in] 逻辑下标。Logical index.
 * @param physical_index [out] 物理下标输出。Mapped physical index.
 * @return 0 表示成功；非0 表示越界或参数错误。0 on success; non-zero on error.
 */
static int signal_seq_logical_to_physical(const SignalSeq *seq, int logical_index, int *physical_index)
{
    if (seq == NULL || physical_index == NULL)
    {
        fprintf(stderr, "Error: NULL pointer in logical_to_physical.\n");
        return -1;
    }

    int offset = logical_index - seq->start;
    if (offset < 0 || offset >= seq->length)
    {
        fprintf(stderr, "Error: logical index %d is out of range [%d, %d].\n",
                logical_index, seq->start, seq->start + seq->length - 1);
        return -1;
    }

    *physical_index = offset;
    return 0;
}

/**
 * @brief 设置指定逻辑下标的值。Set value at given logical index.
 *
 * @param seq [in,out] 序列指针。Sequence pointer.
 * @param logical_index [in] 逻辑下标。Logical index.
 * @param value [in] 要写入的值。Value to set.
 * @return 0 表示成功；非0 表示越界或错误。0 on success; non-zero on error.
 */
static int signal_seq_set(SignalSeq *seq, int logical_index, double value)
{
    int pos = 0;
    if (signal_seq_logical_to_physical(seq, logical_index, &pos) != 0)
    {
        return -1;
    }
    seq->data[pos] = value;
    return 0;
}

/**
 * @brief 获取指定逻辑下标的值。Get value at given logical index.
 *
 * @param seq [in] 序列指针。Sequence pointer.
 * @param logical_index [in] 逻辑下标。Logical index.
 * @param out_value [out] 输出值指针。Pointer to store the value.
 * @return 0 表示成功；非0 表示失败。0 on success; non-zero on error.
 */
static int signal_seq_get(const SignalSeq *seq, int logical_index, double *out_value)
{
    int pos = 0;
    if (out_value == NULL)
    {
        fprintf(stderr, "Error: out_value is NULL in signal_seq_get.\n");
        return -1;
    }
    if (signal_seq_logical_to_physical(seq, logical_index, &pos) != 0)
    {
        return -1;
    }
    *out_value = seq->data[pos];
    return 0;
}

/**
 * @brief 在可扩展序列末尾追加一个值。Append a value to an expandable sequence.
 *
 * @param seq [in,out] 序列指针。Sequence pointer.
 * @param value [in] 要追加的值。Value to append.
 * @return 0 表示成功；非0 表示失败。0 on success; non-zero on error.
 * @note 固定长度序列不允许追加。Fixed-length sequences cannot be appended.
 */
static int signal_seq_append(SignalSeq *seq, double value)
{
    if (seq == NULL)
    {
        fprintf(stderr, "Error: NULL sequence in signal_seq_append.\n");
        return -1;
    }

    if (!seq->allow_expand)
    {
        if (seq->length >= seq->capacity)
        {
            fprintf(stderr, "Error: sequence is fixed-length; cannot append.\n");
            return -1;
        }
    }

    if (seq->length >= seq->capacity)
    {
        int new_capacity = (seq->capacity > 0) ? (seq->capacity * 2) : INITIAL_CAPACITY;
        double *new_data = (double *)realloc(seq->data, (size_t)new_capacity * sizeof(double));
        if (new_data == NULL)
        {
            fprintf(stderr, "Error: failed to reallocate memory when appending.\n");
            return -1;
        }
        seq->data = new_data;
        seq->capacity = new_capacity;
    }

    seq->data[seq->length] = value;
    seq->length += 1;
    return 0;
}

/**
 * @brief 读取固定长度序列的输入。Read input for a fixed-length sequence.
 *
 * @param seq [in,out] 目标序列，已设置 start 和 length。Target sequence with start and length set.
 * @return 0 表示成功；非0 表示失败。0 on success; non-zero on error.
 * @example
 * 用户将依次输入 length 个值，每行一个。User inputs `length` values, one per line.
 */
static int input_fixed_length(SignalSeq *seq)
{
    if (seq == NULL || seq->data == NULL)
    {
        fprintf(stderr, "Error: invalid sequence in input_fixed_length.\n");
        return -1;
    }

    printf("Please enter %d values, one per line.\n", seq->length);

    char buffer[256];
    for (int i = 0; i < seq->length; ++i)
    {
        int logical_index = seq->start + i;
        while (1)
        {
            printf("value[%d] (index=%d): ", i, logical_index);
            if (fgets(buffer, sizeof(buffer), stdin) == NULL)
            {
                fprintf(stderr, "Error: failed to read input for fixed-length sequence.\n");
                return -1;
            }

            char *endptr = NULL;
            errno = 0;
            double v = strtod(buffer, &endptr);
            if (endptr == buffer || errno != 0)
            {
                fprintf(stderr, "Error: invalid number, please try again.\n");
                continue;
            }

            seq->data[i] = v;
            break;
        }
    }

    return 0;
}

/**
 * @brief 读取可扩展序列的输入，直到遇到停止记号。Read an unbounded sequence until stop token is seen.
 *
 * @param seq [in,out] 目标序列，建议 initial_length=0 且 allow_expand=1。Target expandable sequence.
 * @param stop_token [in] 停止记号字符串。Stop token string.
 * @return 0 表示成功；非0 表示失败。0 on success; non-zero on error.
 * @example
 * 用户逐行输入数值，输入 "STOP" 后结束。User inputs values line by line; type "STOP" to finish.
 */
static int input_unbounded(SignalSeq *seq, const char *stop_token)
{
    if (seq == NULL)
    {
        fprintf(stderr, "Error: NULL sequence in input_unbounded.\n");
        return -1;
    }
    if (!seq->allow_expand)
    {
        fprintf(stderr, "Error: sequence is not marked expandable.\n");
        return -1;
    }
    if (stop_token == NULL || stop_token[0] == '\0')
    {
        fprintf(stderr, "Error: invalid stop token.\n");
        return -1;
    }

    printf("Enter values one per line. Type %s to stop.\n", stop_token);

    char buffer[256];
    int i = 0;
    for (;;)
    {
        int logical_index = seq->start + i;
        printf("value[%d] (index=%d): ", i, logical_index);

        if (fgets(buffer, sizeof(buffer), stdin) == NULL)
        {
            fprintf(stderr, "Error: failed to read input (EOF or error).\n");
            return -1;
        }

        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n')
        {
            buffer[len - 1] = '\0';
            --len;
        }

        if (strcmp(buffer, stop_token) == 0)
        {
            break;
        }

        char *endptr = NULL;
        errno = 0;
        double v = strtod(buffer, &endptr);
        if (endptr == buffer || *endptr != '\0' || errno != 0)
        {
            fprintf(stderr, "Error: invalid number, please try again.\n");
            continue;
        }

        if (signal_seq_append(seq, v) != 0)
        {
            return -1;
        }
        ++i;
    }

    return 0;
}

/**
 * @brief 打印序列内容，用于调试和验证。Print sequence for debugging.
 *
 * @param seq [in] 序列指针。Sequence pointer.
 * @note 日志输出使用英文。Logs are printed in English.
 */
static void print_sequence(const SignalSeq *seq)
{
    if (seq == NULL)
    {
        fprintf(stderr, "Error: cannot print a NULL sequence.\n");
        return;
    }

    printf("Sequence summary:\n");
    printf("  start index: %d\n", seq->start);
    printf("  length     : %d\n", seq->length);

    if (seq->length == 0)
    {
        printf("  values     : (empty sequence)\n");
        return;
    }

    if (seq->data == NULL)
    {
        fprintf(stderr, "Error: sequence data is NULL while length > 0.\n");
        return;
    }

    printf("  values     :\n");

    for (int i = 0; i < seq->length; ++i)
    {
        int logical_index = seq->start + i;
        printf("    x[%d] = %.6g\n", logical_index, seq->data[i]);
    }
}

/**
 * @brief 主函数：提供命令行交互，演示固定长度与无限长度信号序列输入与访问。Main entry: CLI to demonstrate fixed and unbounded signal sequences.
 *
 * @return 程序退出码；0 表示成功，非0 表示出现错误。Process exit code; 0 for success, non-zero for errors.
 */
int main(void)
{
    printf("Signal Sequence CLI Demo\n");
    printf("This program supports:\n");
    printf("  (1) Fixed-length sequence with custom start index.\n");
    printf("  (2) Unbounded sequence input with stop token '%s'.\n", STOP_TOKEN);
    printf("Please select mode: 1 for fixed-length, 2 for unbounded: ");

    char buffer[256];
    if (fgets(buffer, sizeof(buffer), stdin) == NULL)
    {
        fprintf(stderr, "Error: failed to read mode.\n");
        return 1;
    }

    int mode = 0;
    if (sscanf(buffer, "%d", &mode) != 1 || (mode != 1 && mode != 2))
    {
        fprintf(stderr, "Error: invalid mode, must be 1 or 2.\n");
        return 1;
    }

    SignalSeq seq;
    memset(&seq, 0, sizeof(seq));

    if (mode == 1)
    {
        /* Fixed-length mode */
        printf("You selected fixed-length mode.\n");
        printf("Enter start index (can be negative): ");
        if (fgets(buffer, sizeof(buffer), stdin) == NULL)
        {
            fprintf(stderr, "Error: failed to read start index.\n");
            return 1;
        }
        int start = 0;
        if (sscanf(buffer, "%d", &start) != 1)
        {
            fprintf(stderr, "Error: invalid start index.\n");
            return 1;
        }

        printf("Enter length (>0): ");
        if (fgets(buffer, sizeof(buffer), stdin) == NULL)
        {
            fprintf(stderr, "Error: failed to read length.\n");
            return 1;
        }
        int length = 0;
        if (sscanf(buffer, "%d", &length) != 1 || length <= 0)
        {
            fprintf(stderr, "Error: invalid length; must be positive.\n");
            return 1;
        }

        if (signal_seq_init(&seq, start, length, 0) != 0)
        {
            /* error message already printed */
            return 1;
        }

        if (input_fixed_length(&seq) != 0)
        {
            signal_seq_free(&seq);
            return 1;
        }

        print_sequence(&seq);
    }
    else
    {
        /* Unbounded mode */
        printf("You selected unbounded mode.\n");
        printf("Enter start index for the first element (can be negative): ");
        if (fgets(buffer, sizeof(buffer), stdin) == NULL)
        {
            fprintf(stderr, "Error: failed to read start index.\n");
            return 1;
        }
        int start = 0;
        if (sscanf(buffer, "%d", &start) != 1)
        {
            fprintf(stderr, "Error: invalid start index.\n");
            return 1;
        }

        if (signal_seq_init(&seq, start, 0, 1) != 0)
        {
            return 1;
        }

        if (input_unbounded(&seq, STOP_TOKEN) != 0)
        {
            signal_seq_free(&seq);
            return 1;
        }

        print_sequence(&seq);
    }

    signal_seq_free(&seq);
    return 0;
}
