!/bin/bash

#script to run generic lhe generation tarballs
#kept as simply as possible to minimize need 
#to update the cmssw release
#(all the logic goes in the run script inside the tarball
# on frontier)
#J.Bendavid

#This script is an development version to check 
#1. support of additional parameters (nproc, maxevt) supported in > MG362
#2. autodection of backend to ensure submission portability
#J.Choi

#exit on first error
set -e

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

echo "   ______________________________________     "
echo "         Running Generic Tarball/Gridpack     "
echo "   ______________________________________     "

path=${1}
echo "gridpack tarball path = $path"

# Auto-detect best backend if path contains _AUTODETECT_
if [[ "$path" == *"_AUTODETECT_"* ]]; then
    echo "%MSG-MG5 Auto-detecting best backend..."
    path=$(detect_best_backend "$path")
    echo "%MSG-MG5 Selected gridpack path = $path"
fi

nevt=${2}
echo "%MSG-MG5 number of events requested = $nevt"

rnum=${3}
echo "%MSG-MG5 random seed used for the run = $rnum"

nproc=${4}
echo "%MSG-MG5 thread count requested = $nproc"

maxevt=${5}
echo "%MSG-MG5 max events per job = $maxevt"

echo "%MSG-MG5 residual/optional arguments = ${@:6}"

if [ -n "${6}" ]; then
  use_gridpack_env=${6}
  echo "%MSG-MG5 use_gridpack_env = $use_gridpack_env"
fi

if [ -n "${7}" ]; then
  scram_arch_version=${7}
  echo "%MSG-MG5 override scram_arch_version = $scram_arch_version"
fi

if [ -n "${8}" ]; then
  cmssw_version=${8}
  echo "%MSG-MG5 override cmssw_version = $cmssw_version"
fi

LHEWORKDIR=`pwd`

if [ "$use_gridpack_env" = false -a -n "$scram_arch_version" -a -n  "$cmssw_version" ]; then
  echo "%MSG-MG5 CMSSW version = $cmssw_version"
  export SCRAM_ARCH=${scram_arch_version}
  scramv1 project CMSSW ${cmssw_version}
  cd ${cmssw_version}/src
  eval `scramv1 runtime -sh`
  cd $LHEWORKDIR
fi

if [[ -d lheevent ]]
    then
    echo 'lheevent directory found'
    echo 'Setting up the environment'
    rm -rf lheevent
fi
mkdir lheevent; cd lheevent

#untar the tarball directly from cvmfs
tar -xaf ${path} 

# If TMPDIR is unset, set it to the condor scratch area if present
# and fallback to /tmp
export TMPDIR=${TMPDIR:-${_CONDOR_SCRATCH_DIR:-/tmp}}

#generate events
./runcmsgrid.sh $nevt $rnum $nproc $maxevt ${@:6}

mv cmsgrid_final.lhe $LHEWORKDIR/

cd $LHEWORKDIR

#cleanup working directory (save space on worker node for edm output)
rm -rf lheevent

exit 0
