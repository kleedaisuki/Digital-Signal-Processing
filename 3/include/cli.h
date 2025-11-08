/**
 * @file cli.h
 * @brief 命令行前端接口 / Command-line front-end interface
 */

#ifndef CLI_H
#define CLI_H

/**
 * @brief 运行命令行工具主逻辑 / Run CLI main logic
 * @param argc 参数个数 / Argument count
 * @param argv 参数数组 / Argument vector
 * @return 进程返回码 / Process exit code
 *
 * @note main() 中只需调用此函数，将流程委托给 CLI 实现。
 *       In main(), simply call this to delegate all CLI logic.
 */
int cli_run(int argc, char **argv);

#endif /* CLI_H */
