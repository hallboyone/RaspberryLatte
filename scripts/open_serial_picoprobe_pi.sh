#!/bin/bash
while getopts ':l' OPTION; do
    case "$OPTION" in
        l)
            log_path="~/raspberrylatte_logs/$(date +"%j.%H.%M.%S").log"
            log_path="~/raspberrylatte_logs/log"
            mkdir -p ~/raspberrylatte_logs
            cmd=(sudo minicom -D /dev/ttyACM0 -b 115200 -C "$log_path")
            echo "${cmd[@]}"
            ${cmd[@]}
            echo "Log saved at: $log_path"
            sleep 1
            exit 1
            ;;
        ?)
            echo "Invalid option. Use -l to enable logging"
            exit 1
            ;;
    esac
done
sudo minicom -D /dev/ttyACM0 -b 115200
