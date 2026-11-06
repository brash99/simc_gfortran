#!/bin/bash
#SBATCH --job-name=nps-gepq11-enuc-g4
#SBATCH --partition=production
#SBATCH --account=halla
#SBATCH --array=0-15%1
#SBATCH --time=02:00:00
#SBATCH --mem=8G
#SBATCH --cpus-per-task=1
#SBATCH --exclude=farm180207
#SBATCH --output=/farm_out/brash/nps_gepq11_enuc_g4_%j.out
#SBATCH --error=/farm_out/brash/nps_gepq11_enuc_g4_%j.err

set -euo pipefail
source /etc/profile.d/modules.sh
module purge
module use /group/halla/modulefiles
module load geant4/11.2.1
module load root/6.30.04
export DVCS_GEN_DIR=/group/nps/hhuang/software/dvcs_gen
export NPS_SOFT_DIR=/group/nps/hhuang/software/NPS_SOFT
export GEP_DISABLE_SWEEPER=1

repo=/work/hallc/gep/brash/gep4_nps_geant4
phase=$repo/target_phase_space
stem=${GEP_PHASE_STEM:-gepq11_xsbias3e5_1000000_cut0p25}
chunk=$SLURM_ARRAY_TASK_ID
input=$phase/${stem}_chunk${chunk}.csv
manifest=$phase/${stem}_chunk${chunk}_manifest.csv
response=$phase/${stem}_chunk${chunk}_nps_response.csv
summary=$phase/${stem}_chunk${chunk}_nps_rates.csv
count=$(( $(wc -l < "$manifest") - 1 ))
macro=$phase/${stem}_chunk${chunk}_beam_on.mac
{
  printf '%s\n' '/control/verbose 0' '/run/verbose 0'
  if [[ ${GEP_EM_ONLY:-0} != 1 ]]; then
    printf '%s\n' '/testhadr/Physics QBBC'
  fi
  printf '%s\n' '/run/initialize' "/run/beamOn $count"
} > "$macro"

cd "$repo"
runid=$(( ${GEP_RUNID_BASE:-1100410} + chunk ))
seed_offset=${GEP_SEED_OFFSET:-0}
seed_index=$((108 + chunk + seed_offset))
seed1=$((345678912 + 10007 * (chunk + seed_offset)))
seed2=$((987654321 - 10009 * (chunk + seed_offset)))
printf '%s\n%s\n%s\n%s\n' "$runid" "$seed_index" "$seed1" "$seed2" | \
  /usr/bin/time -v ./DVCS_evt_gen/build_gep/DVCS \
    6887 "$macro" 0 0 --phase-space-events "$input" 26.651 3.0

root -l -b -q \
  "validation/extract_nps_inclusive_geant4_response.C(\"${runid}\",\"target_phase_space/${stem}_chunk${chunk}_nps_response.csv\")"

python3 "$phase/analyze_nps_response.py" "$manifest" "$response" "$summary" \
  --thresholds 2.5 3.0 3.387 3.629 3.870 4.0
