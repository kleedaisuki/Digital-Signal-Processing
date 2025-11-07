/**
 * @file main.c
 * @brief 程序入口，委托给命令行逻辑。Program entry, delegates to CLI logic.
 */

#include "cli.h"

/**
 * @brief 主函数入口。Main entry point.
 *
 * @param argc [in] 参数个数。Number of arguments.
 * @param argv [in] 参数数组。Argument vector.
 * @return 进程退出码，与 cli_main 一致。Process exit code, same as cli_main.
 *
 * @note 所有实际逻辑在 cli_main 中实现，保持 main 精简。
 *       All real logic is implemented in cli_main to keep main minimal.
 */
int main(int argc, char **argv)
{
    return cli_main(argc, argv);
}
