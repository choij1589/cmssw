#!/bin/bash

set -e

echo "   ______________________________________     "
xrd_path=$1

if [[ $xrd_path != "root://"* ]]; then
    echo "%MSG-ExternalLHEProducer-subprocess ERROR in external process. Path must have format root://<xrd_path>/<path>"
    exit 1
fi

# Function to detect best backend based on hardware capabilities
detect_best_backend() {
    local base_path="$1"
    
    # Check for CUDA availability
    if command -v nvidia-smi &> /dev/null && nvidia-smi &> /dev/null; then
        local gpu_count=$(nvidia-smi --list-gpus | wc -l)
        if [ "$gpu_count" -gt 0 ]; then
            echo "%MSG-MG5 CUDA GPU detected, using CUDA backend" >&2
            echo "${base_path//_AUTODETECT_/_CUDA_}"
            return
        fi
    fi
    
    # Check CPU flags for vectorization support
    if grep -q avx512 /proc/cpuinfo; then
        echo "%MSG-MG5 AVX-512 support detected, using CPP512Z backend" >&2
        echo "${base_path//_AUTODETECT_/_CPP512Z_}"
        return
    elif grep -q avx2 /proc/cpuinfo; then
        echo "%MSG-MG5 AVX2 support detected, using CPPAVX2 backend" >&2
        echo "${base_path//_AUTODETECT_/_CPPAVX2_}"
        return
    else
        echo "%MSG-MG5 No advanced vectorization detected, using basic MG362(UPSTREAM)" >&2
        echo "${base_path//_AUTODETECT_/_UPSTREAM_}"
        return
    fi
}

# Auto-detect best backend if path contains _AUTODETECT_
if [[ "$xrd_path" == *"_AUTODETECT_"* ]]; then
    echo "%MSG-MG5 Auto-detecting best backend..."
    xrd_path=$(detect_best_backend "$xrd_path")
    echo "%MSG-MG5 Selected gridpack path = $xrd_path"
fi
gridpack=$(basename $xrd_path)

if [ -e $gridpack ]; then
    echo "%MSG-ExternalLHEProducer-subprocess WARNING: File $gridpack already exists, it will be overwritten."
    rm $gridpack
fi

echo "%MSG-ExternalLHEProducer-subprocess INFO: Copying gridpack $xrd_path locally using xrootd"
xrdcp $xrd_path .

path=`pwd`/$gridpack
generic_script=$(dirname ${0})/run_generic_tarball_cvmfs_autodetect.sh 
. $generic_script $path ${@:2}
