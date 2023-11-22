#!/bin/bash
#
#SBATCH --cpus-per-task=8
#SBATCH --time=60:00
#SBATCH --mem=20G

srun scons -j 8 build/X86/gem5.opt CPU_MODELS='AtomicSimpleCPU,O3CPU,TimingSimpleCPU,MinorCPU' --gold-linker