#!/bin/bash
#SBATCH --job-name=nps-6168-fsr
#SBATCH --partition=production
#SBATCH --account=halla
#SBATCH --array=0-1
#SBATCH --time=01:00:00
#SBATCH --mem=24G
#SBATCH --cpus-per-task=1
#SBATCH --exclude=farm180207
#SBATCH --output=/farm_out/brash/nps_6168_fsr_%A_%a.out
#SBATCH --error=/farm_out/brash/nps_6168_fsr_%A_%a.err

set -euo pipefail

source /etc/profile.d/modules.sh
module purge
module use /group/halla/modulefiles
module load geant4/11.2.1
module load root/6.30.04

export DVCS_GEN_DIR=/group/nps/hhuang/software/dvcs_gen
export NPS_SOFT_DIR=/group/nps/hhuang/software/NPS_SOFT

repo=/work/hallc/gep/brash/gep4_nps_geant4
labels=(electron_only electron_fsr)
i=$SLURM_ARRAY_TASK_ID
label=${labels[$i]}

cd "$repo"
/usr/bin/time -v ./DVCS_evt_gen/build_gep/DVCS \
  6151 validation/beam_on_4852.mac 0 0 \
  --simc-events "validation/nps_run6168_elastic_${label}.hybrid.csv" \
  18.56 9.5 \
  < "validation/nps_run6168_elastic_${label}.input"
