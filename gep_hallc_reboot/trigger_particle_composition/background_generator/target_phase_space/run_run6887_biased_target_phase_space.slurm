#!/bin/bash
#SBATCH --job-name=nps-6887-target-bias
#SBATCH --partition=production
#SBATCH --account=halla
#SBATCH --time=00:30:00
#SBATCH --mem=2G
#SBATCH --cpus-per-task=1
#SBATCH --output=/farm_out/brash/nps_6887_target_bias_%j.out
#SBATCH --error=/farm_out/brash/nps_6887_target_bias_%j.err

set -euo pipefail

source /etc/profile.d/modules.sh
module purge
module use /group/halla/modulefiles
module load geant4/11.2.1
module load root/6.30.04

work=/work/hallc/gep/brash/gep4_nps_geant4/target_phase_space
events=${GEP_PHASE_SPACE_EVENTS:-100}
cut=${GEP_PHASE_SPACE_CUT_GEV:-1.0}
seed=${GEP_PHASE_SPACE_SEED:-6887101}
output=${GEP_PHASE_SPACE_OUTPUT:-$work/run6887_biased_${events}_cut${cut}.root}
bias=${GEP_ELECTRONUCLEAR_BIAS:-100000}

/usr/bin/time -v "$work/build/gep_target_phase_space" \
  --bias-electronuclear "$bias" "$output" "$events" 8.456 "$cut" "$seed"
