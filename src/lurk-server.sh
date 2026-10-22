#!/bin/bash
if (( $# == 0 )); then
    cat << _EOF_
    lurk-server - commandline files/dirs backup manager [Version beta-0.1]

    lurk is a powerful files/dirs manager, which include following functions
        * Manage a collection of scattered dirs to send-to / recveive-from(later) a location you specified where could be a local address or a network address
        * Make block comparison and Use shared memory to get the Fastest Speed, meanwhile, reduce cost of performance
        * Independent update program, to update database and generate the lastest verify files after you modify files
        * Conducted in a multi-threaded and multi-process way
        * Security, Only Read, Will not damage files
        * Use Daemon and low priority to achieve Silent operation
        * Transaction Management Method, e.g. if deliver process is interrupt, next time it will Re-Compare, and set the information if delivering is all done
        * UNIX Domain socket to manage local address, and TCP socket to manage network address

    Example:
        lurk-server --init  /path/to/your/recv/dir
        lurk-server --sync


    For a listing of options, use lurk-server -h or lurk-server --help.

_EOF_
    exit 0
fi
export PATH=$PATH:.
if (( $# == 1)); then
    if [[ $1 == '--start' || $1 == '--sync' ]]; then
        lurk_server
    fi
fi

if (( $# == 2)); then
    if [[ $1 == '-i' || $1 = '--init' || '--set-recv-dir' ]]; then
        lurk_server_init "$2"
    fi
fi
