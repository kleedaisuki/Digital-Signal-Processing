#include "cli.h"
#include "sequence.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* ---------- 内部工具：日志与用法 ---------- */

/**
 * @brief 打印错误消息（英文到 stderr）。Print error message in English to stderr.
 *
 * @param msg [in] 消息字符串。Message string.
 */
static void cli_log_error(const char *msg)
{
    if (!msg)
    {
        return;
    }
    fprintf(stderr, "[cli] error: %s\n", msg);
}

/**
 * @brief 打印用法说明。Print usage information.
 */
static void cli_print_usage(void)
{
    fprintf(stderr,
            "Usage:\n"
            "  seqops <op> [params...] finite\n"
            "  seqops <op> [params...] stream\n"
            "\n"
            "Operations (op):\n"
            "  pad-front <zeros>\n"
            "  pad-back  <zeros>\n"
            "  delay     <delay> <fill>\n"
            "  advance   <advance> <fill>\n"
            "  reverse\n"
            "  upsample  <factor>\n"
            "  downsample <factor>\n"
            "  diff\n"
            "  cumsum\n"
            "\n"
            "Finite mode input (from stdin):\n"
            "  First line : N (length)\n"
            "  Second line: N double values\n"
            "\n"
            "Stream mode input (from stdin):\n"
            "  Sequence of double tokens separated by spaces/newlines,\n"
            "  terminated by the token END (case-insensitive).\n"
            "\n"
            "Output format:\n"
            "  First line : ONLINE:YES or ONLINE:NO\n"
            "  Second line: result sequence values on a single line.\n");
}

/* ---------- 内部工具：解析函数 ---------- */

/**
 * @brief 解析操作名字符串。Parse operation name string.
 *
 * @param name [in] 操作名。Operation name.
 * @param op [out] 操作类型。Parsed operation type.
 * @return 0 成功；非 0 失败。0 on success, non-zero on failure.
 */
static int cli_parse_op(const char *name, seq_op_type *op)
{
    if (!name || !op)
    {
        return -1;
    }

    if (strcmp(name, "pad-front") == 0)
    {
        *op = SEQ_OP_PAD_FRONT;
        return 0;
    }
    if (strcmp(name, "pad-back") == 0)
    {
        *op = SEQ_OP_PAD_BACK;
        return 0;
    }
    if (strcmp(name, "delay") == 0)
    {
        *op = SEQ_OP_DELAY;
        return 0;
    }
    if (strcmp(name, "advance") == 0)
    {
        *op = SEQ_OP_ADVANCE;
        return 0;
    }
    if (strcmp(name, "reverse") == 0)
    {
        *op = SEQ_OP_REVERSE;
        return 0;
    }
    if (strcmp(name, "upsample") == 0)
    {
        *op = SEQ_OP_UPSAMPLE;
        return 0;
    }
    if (strcmp(name, "downsample") == 0)
    {
        *op = SEQ_OP_DOWNSAMPLE;
        return 0;
    }
    if (strcmp(name, "diff") == 0)
    {
        *op = SEQ_OP_DIFF;
        return 0;
    }
    if (strcmp(name, "cumsum") == 0)
    {
        *op = SEQ_OP_CUMSUM;
        return 0;
    }

    return -1;
}

/**
 * @brief 字符串转 size_t。Convert string to size_t.
 *
 * @param s [in] 输入字符串。Input string.
 * @param out [out] 结果。Result.
 * @return 0 成功；非 0 失败。0 on success, non-zero on failure.
 */
static int cli_parse_size(const char *s, size_t *out)
{
    char *end = NULL;
    unsigned long long v;

    if (!s || !out)
    {
        return -1;
    }

    errno = 0;
    v = strtoull(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0')
    {
        return -1;
    }
    *out = (size_t)v;
    return 0;
}

/**
 * @brief 字符串转 double。Convert string to double.
 *
 * @param s [in] 输入字符串。Input string.
 * @param out [out] 结果。Result.
 * @return 0 成功；非 0 失败。0 on success, non-zero on failure.
 */
static int cli_parse_double(const char *s, double *out)
{
    char *end = NULL;
    double v;

    if (!s || !out)
    {
        return -1;
    }

    errno = 0;
    v = strtod(s, &end);
    if (errno != 0 || end == s || *end != '\0')
    {
        return -1;
    }
    *out = v;
    return 0;
}

/* ---------- 有限模式处理 ---------- */

/**
 * @brief 从 stdin 读取有限序列。Read finite sequence from stdin.
 *
 * @param seq [out] 输出序列。Output sequence.
 * @return 0 成功；非 0 失败。0 on success, non-zero on failure.
 */
static int cli_read_finite(seq_t *seq)
{
    size_t n = 0;
    size_t i = 0;

    if (!seq)
    {
        cli_log_error("cli_read_finite: null seq");
        return -1;
    }

    if (scanf("%zu", &n) != 1)
    {
        cli_log_error("failed to read length N for finite mode");
        return -1;
    }
    if (seq_alloc(seq, n) != SEQ_OK)
    {
        cli_log_error("memory allocation failed for finite sequence");
        return -1;
    }
    while (i < n)
    {
        if (scanf("%lf", &seq->data[i]) != 1)
        {
            cli_log_error("not enough samples for finite sequence");
            seq_free(seq);
            return -1;
        }
        i++;
    }
    return 0;
}

/**
 * @brief 打印序列到 stdout。Print sequence to stdout.
 *
 * @param seq [in] 序列。Sequence.
 */
static void cli_print_sequence(const seq_t *seq)
{
    size_t i;

    if (!seq || !seq->data)
    {
        printf("\n");
        return;
    }

    if (seq->length == 0)
    {
        printf("\n");
        return;
    }

    for (i = 0; i < seq->length; ++i)
    {
        if (i > 0)
        {
            putchar(' ');
        }
        printf("%.10g", seq->data[i]);
    }
    putchar('\n');
}

/**
 * @brief 执行有限模式操作。Execute operation in finite mode.
 *
 * @param op 操作类型。Operation type.
 * @param param_main 主参数。Main parameter.
 * @param fill 填充值。Fill value.
 * @return 0 成功；非 0 失败。0 on success, non-zero on failure.
 */
static int cli_run_finite(seq_op_type op, size_t param_main, double fill)
{
    seq_t src = {0}, dst = {0};
    seq_err_t err;

    if (cli_read_finite(&src) != 0)
    {
        return 1;
    }

    switch (op)
    {
    case SEQ_OP_PAD_FRONT:
        err = seq_pad_front(&src, param_main, &dst);
        break;
    case SEQ_OP_PAD_BACK:
        err = seq_pad_back(&src, param_main, &dst);
        break;
    case SEQ_OP_DELAY:
        err = seq_delay(&src, param_main, fill, &dst);
        break;
    case SEQ_OP_ADVANCE:
        err = seq_advance(&src, param_main, fill, &dst);
        break;
    case SEQ_OP_REVERSE:
        err = seq_reverse(&src, &dst);
        break;
    case SEQ_OP_UPSAMPLE:
        err = seq_upsample(&src, param_main, &dst);
        break;
    case SEQ_OP_DOWNSAMPLE:
        err = seq_downsample(&src, param_main, &dst);
        break;
    case SEQ_OP_DIFF:
        err = seq_diff(&src, &dst);
        break;
    case SEQ_OP_CUMSUM:
        err = seq_cumsum(&src, &dst);
        break;
    default:
        cli_log_error("unsupported operation in finite mode");
        seq_free(&src);
        return 1;
    }

    if (err != SEQ_OK)
    {
        cli_log_error("sequence operation failed in finite mode");
        seq_free(&src);
        seq_free(&dst);
        return 1;
    }

    /* 对有限输入的“在线可实现性”判定。 */
    printf("ONLINE:%s\n", seq_online_capable(op, 0) ? "YES" : "NO");
    cli_print_sequence(&dst);

    seq_free(&src);
    seq_free(&dst);
    return 0;
}

/* ---------- 流式模式处理 ---------- */

/**
 * @brief 是否是 END 标记（忽略大小写）。Check if token is END sentinel (case-insensitive).
 */
static int cli_is_end_token(const char *s)
{
    if (!s)
    {
        return 0;
    }
    return (strcasecmp(s, "END") == 0);
}

/**
 * @brief 执行流式模式操作。Execute operation in stream mode.
 *
 * @param op 操作类型。Operation type.
 * @param param_main 主参数。Main parameter.
 * @param fill 填充值。Fill value.
 * @return 0 成功；非 0 失败。0 on success, non-zero on failure.
 */
static int cli_run_stream(seq_op_type op, size_t param_main, double fill)
{
    seq_stream_t st;
    char token[128];
    int rc;
    double x, y;
    int has_output;

    if (!seq_online_capable(op, 1))
    {
        printf("ONLINE:NO\n");
        cli_log_error("operation not supported for online infinite input");
        return 1;
    }

    rc = seq_stream_init(&st, op, param_main, 0, fill);
    if (rc != SEQ_OK)
    {
        printf("ONLINE:NO\n");
        cli_log_error("failed to initialize streaming state");
        return 1;
    }

    printf("ONLINE:YES\n");

    /* 对于需要起始补零的操作（如 pad-front），先冲刷前缀输出。 */
    while (1)
    {
        rc = seq_stream_step(&st, 0, 0.0, &y, &has_output);
        if (rc != SEQ_OK)
        {
            cli_log_error("streaming step failed during initial flush");
            seq_stream_dispose(&st);
            return 1;
        }
        if (!has_output)
        {
            break;
        }
        printf("%.10g ", y);
    }

    /* 读取流式输入，直到 END。*/
    while (scanf("%127s", token) == 1)
    {
        if (cli_is_end_token(token))
        {
            break;
        }

        if (cli_parse_double(token, &x) != 0)
        {
            cli_log_error("invalid numeric token in stream input");
            seq_stream_dispose(&st);
            return 1;
        }

        rc = seq_stream_step(&st, 1, x, &y, &has_output);
        if (rc != SEQ_OK)
        {
            cli_log_error("streaming step failed");
            seq_stream_dispose(&st);
            return 1;
        }
        if (has_output)
        {
            printf("%.10g ", y);
        }

        /* 对可能产生额外输出的操作（上采样等）进行尾随冲刷。*/
        while (1)
        {
            rc = seq_stream_step(&st, 0, 0.0, &y, &has_output);
            if (rc != SEQ_OK)
            {
                cli_log_error("streaming step failed during extra flush");
                seq_stream_dispose(&st);
                return 1;
            }
            if (!has_output)
            {
                break;
            }
            printf("%.10g ", y);
        }
    }

    /* 当前设计的流式算子不会在 END 后继续产生无限尾部输出。
       若后续扩展需要，可在此位置增加有限次数 flush。 */
    putchar('\n');

    seq_stream_dispose(&st);
    return 0;
}

/* ---------- 对外主入口 ---------- */

int cli_main(int argc, char **argv)
{
    seq_op_type op;
    const char *mode;
    size_t param_main = 0;
    double fill = 0.0;

    if (argc < 3)
    {
        cli_log_error("not enough arguments");
        cli_print_usage();
        return 1;
    }

    /* 最后一个参数固定视为模式：finite 或 stream */
    mode = argv[argc - 1];

    if (cli_parse_op(argv[1], &op) != 0)
    {
        cli_log_error("unknown operation");
        cli_print_usage();
        return 1;
    }

    /* 解析各操作参数：argv[2] .. argv[argc-2]（不含 mode） */
    {
        int param_count = argc - 3; /* ✅ 正确减掉 程序名+操作名+模式 */
        const char **params = (const char **)&argv[2];

        if (param_count < 0)
        {
            param_count = 0; /* 理论上不该出现，仅作为防御性保护 */
        }

        switch (op)
        {
        case SEQ_OP_PAD_FRONT:
        case SEQ_OP_PAD_BACK:
        case SEQ_OP_UPSAMPLE:
        case SEQ_OP_DOWNSAMPLE:
            if (param_count != 1)
            {
                cli_log_error("missing or extra parameter");
                cli_print_usage();
                return 1;
            }
            if (cli_parse_size(params[0], &param_main) != 0)
            {
                cli_log_error("invalid size parameter");
                return 1;
            }
            break;

        case SEQ_OP_DELAY:
        case SEQ_OP_ADVANCE:
            if (param_count != 2)
            {
                cli_log_error("missing or extra parameters for delay/advance");
                cli_print_usage();
                return 1;
            }
            if (cli_parse_size(params[0], &param_main) != 0)
            {
                cli_log_error("invalid delay/advance parameter");
                return 1;
            }
            if (cli_parse_double(params[1], &fill) != 0)
            {
                cli_log_error("invalid fill parameter");
                return 1;
            }
            break;

        case SEQ_OP_REVERSE:
        case SEQ_OP_DIFF:
        case SEQ_OP_CUMSUM:
            if (param_count != 0)
            {
                cli_log_error("too many parameters for this operation");
                cli_print_usage();
                return 1;
            }
            break;

        default:
            cli_log_error("unsupported operation");
            return 1;
        }
    }

    /* 根据模式选择运行方式 */
    if (strcmp(mode, "finite") == 0)
    {
        return cli_run_finite(op, param_main, fill);
    }
    if (strcmp(mode, "stream") == 0)
    {
        return cli_run_stream(op, param_main, fill);
    }

    cli_log_error("unknown mode (expected 'finite' or 'stream')");
    cli_print_usage();
    return 1;
}