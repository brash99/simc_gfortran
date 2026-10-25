#!/bin/bash
#SBATCH --job-name=gep-g4-q11-10m
#SBATCH --partition=production
#SBATCH --time=02:30:00
#SBATCH --mem=24G
#SBATCH --cpus-per-task=1
#SBATCH --exclude=farm180207
#SBATCH --output=/farm_out/brash/gep_q11_nps_10m_%j.out
#SBATCH --error=/farm_out/brash/gep_q11_nps_10m_%j.err

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
  6151 beam_on_110774.mac 0 0 \
  --simc-events validation/gep_q11_nps_30cm_3m_10M.hybrid.csv 26.651 3.0 \
  < validation/gep_q11_nps_30cm_3m_10M_geant.input
