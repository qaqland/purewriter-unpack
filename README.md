# pwexport

This project is a tool to export plain text files from *pure writer* 's backup.

小工具用来从纯纯写作的备份文件中导出纯文本。

Writen by C and based on libzip and sqlite3

# build 

    gcc -o pwexport pwexport.c -lsqlite3 -lzip -DSQLITE_CORE

# usage 

    ./pwexport foobar.pwb

# CAUTION

- **Don't Change** the name of `*.pwb` file.
- **Delete** the folders near the `*.pwb` file before use.
- **One** file one time, one use one clean
- Make a **backup**, we are not responsible for any loss.

# license
```
Copyright 起床睡觉 2022 

Apache License 2.0
```