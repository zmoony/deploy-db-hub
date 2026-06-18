#!/bin/bash

# 获取当前目录路径
SCRIPT_PATH=$(dirname "$(readlink -f "$0")")

# 默认内存大小,单位m
MEX_MEM=2048

# 定义函数：获取最新的jar文件名称
function get_latest_jar {
  # 查找当前目录下的所有jar文件，并按照时间顺序排序，取最新的一个
  local latest_jar
  latest_jar=$(find "$SCRIPT_PATH" -maxdepth 1 -name "*.jar" -type f -printf '%T@ %p\n' 2>/dev/null | sort -rn | head -n1 | cut -d' ' -f2 | xargs -n 1 basename)
  if [ -z "$latest_jar" ]; then
    echo "未找到jar文件"
    exit 1
  fi
  echo "$latest_jar"
}

# 关闭在后台运行的脚本和jar包
function stop {
    local pid_file="$SCRIPT_PATH/bashpid"
    if [ -f "$pid_file" ]; then
        local pid
        pid=$(cat "$pid_file")
        if [ -n "$pid" ]; then
            kill -9 "$pid" >/dev/null 2>&1
            rm "$pid_file" >/dev/null 2>&1
        fi
    fi
    local jar_name
    jar_name=$(get_latest_jar)
    local pid
    pid=$(pgrep -f "$jar_name")
    if [ -n "$pid" ]; then
        echo "jar包名称：$jar_name，进程$pid,kill 进程$pid"
        kill -9 "$pid" >/dev/null 2>&1
        echo "已停止运行jar包：$jar_name"
    fi
}

# 启动jar包
function start() {
    local jar_name
    jar_name=$(get_latest_jar)
    echo "启动jar包：$jar_name"
    if [ -d "$SCRIPT_PATH/libs" ]; then
        cd "$SCRIPT_PATH" || exit
        nohup java -Xms256m -Xmx$1m -jar "$jar_name" >/dev/null 2>&1 &
    else
        cd "$SCRIPT_PATH" || exit
        nohup java -Xms256m -Xmx$1m -jar "$jar_name" >/dev/null 2>&1 &
    fi

    local pid
    pid=$(pgrep -f "$jar_name")
    if [ -n "$pid" ]; then
        echo "已启动jar包:$jar_name,进程id:$pid"
    else
        sleep 1s
        pid=$(pgrep -f "$jar_name")
        echo "已启动jar包:$jar_name,进程id:$pid"
    fi
}

# 检查并重启jar包
function watchdog() {
    local jar_name
    jar_name=$(get_latest_jar)
    local interval=5
    local pid
    while true; do
        pid=$(pgrep -f "$jar_name")
        if [ -z "$pid" ]; then
           start "$1"
        fi
        sleep "$interval"
    done
}

# 定义函数
function restart_jar() {
   stop
   start "$1"
   watchdog "$1" >/dev/null 2>&1 &
   echo "$!" > "$SCRIPT_PATH/bashpid"
}

function print_help {
    echo "使用方法："
    echo "./restart.sh [stop|help|-h|--help|内存参数]"
    echo ""
    echo "参数说明："
    echo "stop             停止当前正在运行的jar进程"
    echo "help,-h,--help   显示脚本使用说明"
    echo "内存参数         指定启动jar进程时的最大内存，单位m，默认2048m"
    echo ""
    echo "示例："
    echo "./restart.sh stop             # 停止当前正在运行的jar进程"
    echo "./restart.sh 4096             # 以4096m的最大内存启动jar进程"
    echo "./restart.sh -h               # 显示脚本使用说明"
}

# 根据传入的参数执行不同的逻辑
case $1 in
    stop)
        stop
        ;;
    help|-h|--help)
        print_help
        ;;
    *)
        if [ -z "$1" ]; then
            restart_jar $MEX_MEM
        elif [[ $1 =~ ^[0-9]+$ ]]; then
            restart_jar "$1"
        else
            echo "参数错误，请输入合法的参数"
            print_help
        fi
        ;;
esac
