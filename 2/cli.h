#ifndef CLI_H
#define CLI_H

/**
 * @file cli.h
 * @brief 命令行接口声明。Command line interface declarations.
 */

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief 运行命令行工具主逻辑。Run command-line tool main logic.
     *
     * 本函数解析命令行参数，从标准输入读取序列数据，调用序列操作库并输出结果。
     * This function parses command-line arguments, reads sequence data from stdin,
     * invokes the sequence operation library, and prints the result.
     *
     * @param argc [in] 参数个数。Number of arguments.
     * @param argv [in] 参数数组。Argument vector.
     * @return 0 表示成功，非 0 表示错误码。0 on success, non-zero on error.
     *
     * @note 所有错误信息使用英文并输出到 stderr。
     *       All error messages are printed in English to stderr.
     *
     * @example
     * // 例：对有限序列做差分：
     * // Example: difference of a finite sequence:
     * // echo "5\n1 2 4 7 11" | seqops diff finite
     *
     * @example
     * // 例：对流式输入做累加，使用 END 作为结束标记：
     * // Example: streaming cumulative sum with END sentinel:
     * // echo "1 2 3 4 END" | seqops cumsum stream
     */
    int cli_main(int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif /* CLI_H */
