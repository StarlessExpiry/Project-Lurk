#!/bin/bash
if (( $# == 0 )); then
    cat << _EOF_
    lurk-client for macos - commandline files/dirs backup manager [Version beta-0.1]


    Usage:
        dli --args [dirs..]
        dli [options]

    lurk is a high-performance file and directory management system that offers the following core features
        * Manage a collection of scattered dirs to send-to / recveive-from a location you specified where could be a local address or a network address
        * Make block comparison and Use shared memory to get the Fastest Speed, meanwhile, minimizing system resource overhead
        * Independent update program, to update database and generate up-to-date verify files after you modify files
        * Conducted in a multi-threaded and multi-process way
        * Non-Destructive & Read-Only Safety
        * Use Daemon and low priority to achieve Silent operation
        * Transaction Management Method, e.g. if deliver process is interrupt, next time it will Re-Compare, and set the information if delivering is all done
        * UNIX Domain socket to manage local address, and TCP socket to manage network address

    Example:
        lurk-client --set-dirs /path/to/your/dir1 /path/to/your/dir2 ...

    For a listing of options, use lurk-client -h or lurk-client --help.

_EOF_
    exit 0
fi
export PATH=$PATH:.
if (( $# == 1 )); then
    if [[ $1 == "-s" || $1 == "--show" ]]; then
        OLD_IFS=$IFS
        IFS=$'\n'
        line_no=0
        for lines in $(lurk_show_dirs | head -n 10); do
            ((line_no++))
            if (( line_no == 1 )); then
                echo "🫡Here is your managing dirs (head 10): "
            fi
            echo -e "\t $lines"
        done
        if (( line_no == 0)); then
            echo "😓No dirs to manage.";
        fi
        if (( line_no == 10 )); then
            echo "Maybe more, use lurk -sa or lurk --show-all to print all"
        fi

        IFS=$OLD_IFS
    fi

    if [[ $1 == '-sa' || $1 == "--show-all" ]]; then
        lurk_show_dirs | less
    fi

    if [[ $1 == '-u' || $1 == '--update' ]]; then
        # 应在此处使用“文件上锁”方式，避免同名进程也在运行
        lurk_update
    fi

    if [[ $1 == '--start' || $1 == '--sync' ]]; then
        # ./lurk_client 2>&1 | tee -a "lurk_log_$(date +%Y%m%d_%Hh_%Mm_%Ss).log" | tail -n 10
        # mkdir -p ~/.config/lurk/client/
        # ./lurk_client >> "$HOME/.config/lurk/client/lurk_log_$(date +%Y%m%d_%Hh_%Mm_%Ss).log" 2>&1 &
        # sleep 1
        # tail -n 10 -f $HOME/.config/lurk/client/lurk_log_$(date +%Y%m%d_%Hh_%Mm_)*.log
        lurk_client
    fi

elif (( $# >= 2)); then
    if [[ $1 == '-sd' || $1 == '--set-dir' ]]; then
        while (( $# >= 2)); do
            lurk_set_dir $2
            shift
        done
    elif [[ $1 == '-' ]]; then
        :
    fi
fi
exit 0
