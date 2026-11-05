#!/bin/bash
#SBATCH --job-name=nps-6887-cut-g4
#SBATCH --partition=production
#SBATCH --account=halla
#SBATCH --array=0-1
#SBATCH --time=01:00:00
#SBATCH --mem=8G
#SBATCH --cpus-per-task=1
#SBATCH --exclude=farm180207
#SBATCH --output=/farm_out/brash/nps_6887_cut_g4_%A_%a.out
#SBATCH --error=/farm_out/brash/nps_6887_cut_g4_%A_%a.err

set -euo pipefail
source /etc/profile.d/modules.sh
module purge
module use /group/halla/modulefiles
module load geant4/11.2.1
module load root/6.30.04
export DVCS_GEN_DIR=/group/nps/hhuang/software/dvcs_gen
export NPS_SOFT_DIR=/group/nps/hhuang/software/NPS_SOFT

repo=/work/hallc/gep/brash/gep4_nps_geant4
phase=$repo/target_phase_space
labels=(0p5 0p25)
runids=(6887202 6887203)
counts=(48607 74724)
label=${labels[$SLURM_ARRAY_TASK_ID]}
runid=${runids[$SLURM_ARRAY_TASK_ID]}
count=${counts[$SLURM_ARRAY_TASK_ID]}
input=$phase/run6887_xsbias3e5_1000000_cut${label}_selected.csv
manifest=$phase/run6887_xsbias3e5_1000000_cut${label}_selected_manifest.csv
response=$phase/run6887_xsbias3e5_1000000_cut${label}_nps_response.csv
summary=$phase/run6887_xsbias3e5_1000000_cut${label}_nps_rates.csv

cd "$repo"
printf '%s\n%s\n%s\n%s\n' "$runid" 108 345678912 987654321 | \
  /usr/bin/time -v ./DVCS_evt_gen/build_gep/DVCS \
    6887 "validation/beam_on_${count}.mac" 0 0 \
    --phase-space-events "$input" 16.57 3.57

root -l -b -q \
  "validation/extract_nps_inclusive_geant4_response.C(\"${runid}\",\"target_phase_space/run6887_xsbias3e5_1000000_cut${label}_nps_response.csv\")"

python3 "$phase/analyze_nps_response.py" "$manifest" "$response" "$summary"
