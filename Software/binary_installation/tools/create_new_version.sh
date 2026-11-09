#!/bin/bash
set -euo pipefail

usage() {
    echo "Usage: $0 -v 2.7.1"
}

version=""
while getopts ":v:h" option; do
    case "$option" in
        v) version=$OPTARG ;;
        h) usage; exit 0 ;;
        *) usage >&2; exit 1 ;;
    esac
done
shift $((OPTIND - 1))

if [[ $# -ne 0 || ! $version =~ ^[0-9]+(\.[0-9]+)+$ ]]; then
    usage >&2
    exit 1
fi

script_dir=$(cd "$(dirname "$0")" && pwd)
binary_dir=$(cd "$script_dir/.." && pwd)
software_dir=$(cd "$binary_dir/.." && pwd)
build_dir="$software_dir/build"
destination="$binary_dir/install_APIv$version"

if [[ -e $destination ]]; then
    echo "Release directory already exists: $destination" >&2
    exit 1
fi
if ! grep -Fqx "#define RELEASE_VERSION \"v$version\"" "$software_dir/pogolib/release.h"; then
    echo "Update pogolib/release.h to v$version before packaging" >&2
    exit 1
fi

# Check every artifact before creating the release directory.
artifacts=(
    "bootloader_pogobotv3/bootloader.bin|bootloader/bootloader_pogobotv3/bootloader.bin"
    "pogobotv3/gateware/pogobotv3.bin|pogobios/pogobotv3/gateware/pogobotv3.bin"
    "pogobotv3/software/pogobios/pogobios.bin|pogobios/pogobotv3/software/pogobios/pogobios.bin"
    "remocon_pogobotv3/gateware/remocon_pogobotv3.bin|remocon/remocon_pogobotv3/gateware/remocon_pogobotv3.bin"
    "remocon_pogobotv3/software/pogobios/pogobios.bin|remocon/remocon_pogobotv3/software/pogobios/pogobios.bin"
)
for artifact in "${artifacts[@]}"; do
    source_path=${artifact%%|*}
    if [[ ! -s "$build_dir/$source_path" ]]; then
        echo "Missing or empty build artifact: $build_dir/$source_path" >&2
        exit 1
    fi
done

mkdir -p "$destination"
cp "$script_dir/program_robot.sh" "$script_dir/program_remote.sh" "$destination/"
hash_paths=(program_robot.sh program_remote.sh)
for artifact in "${artifacts[@]}"; do
    source_path=${artifact%%|*}
    package_path=${artifact#*|}
    mkdir -p "$destination/$(dirname "$package_path")"
    cp "$build_dir/$source_path" "$destination/$package_path"
    hash_paths+=("$package_path")
done

(cd "$destination" && sha256sum "${hash_paths[@]}" > SHA256SUMS)
echo "Created $destination"
