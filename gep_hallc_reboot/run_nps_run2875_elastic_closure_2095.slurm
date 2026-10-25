#!/bin/bash
#SBATCH --job-name=gep-g4-2875-closure
#SBATCH --partition=production
#SBATCH --time=00:30:00
# Some farm nodes transiently consume about 12.7 GB during Geant4 startup;
# the steady-state transport footprint is normally about 0.4 GB.
#SBATCH --mem=24G
#SBATCH --cpus-per-task=1
#SBATCH --exclude=farm180207
#SBATCH --output=/farm_out/brash/nps_run2875_closure_2095_%j.out
#SBATCH --error=/farm_out/brash/nps_run2875_closure_2095_%j.err

set -euo pipefail

source /etc/profile.d/modules.sh
module purge
module use /group/halla/modulefiles
module load geant4/11.2.1
module load root/6.30.04

export DVCS_GEN_DIR=/group/nps/hhuang/software/dvcs_gen
export NPS_SOFT_DIR=/group/nps/hhuang/software/NPS_SOFT

repo=/work/hallc/gep/brash/gep4_nps_geant4
cd "$repo"

/usr/bin/time -v ./DVCS_evt_gen/build_gep/DVCS \
  2875 validation/beam_on_2095.mac 0 0 \
  --simc-events validation/nps_run2875_elastic_closure.hybrid.csv 16.991 9.5 \
  < validation/nps_run2875_elastic_closure_2095.input
