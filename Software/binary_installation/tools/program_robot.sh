#!/bin/bash
set -u

echo "using iceprog to program the robot"

package_dir=$(cd "$(dirname "$0")" && pwd) || exit 1
marker_file=$(mktemp) || exit 1
trap 'rm -f "$marker_file"' EXIT
printf 'FlashIsOK\n' > "$marker_file" || exit 1

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

check_return () {
    if [[ "$1" = 0 ]]; then
        echo -e " ${GREEN}-> OK${NC}"
    else
        echo -e " ${RED}-> NOK${NC}"
        # Propagate the programmer failure while the EXIT trap removes the marker.
        exit "$1"
    fi
}

# iceprog -o takes physical flash offsets, not CPU-mapped addresses.
iceprog -o 0x0 "$package_dir/bootloader/bootloader_pogobotv3/bootloader.bin"
check_return $?

iceprog -o 0x40000 "$package_dir/pogobios/pogobotv3/gateware/pogobotv3.bin"
check_return $?

iceprog -o 0x60000 "$package_dir/pogobios/pogobotv3/software/pogobios/pogobios.bin"
check_return $?

iceprog -o 0x88000 "$marker_file"
check_return $?
