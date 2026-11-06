#!/bin/bash
#SBATCH --job-name=nps-gepq11-target
#SBATCH --partition=production
#SBATCH --account=halla
#SBATCH --time=00:30:00
#SBATCH --mem=3G
#SBATCH --cpus-per-task=1
#SBATCH --output=/farm_out/brash/nps_gepq11_target_%j.out
#SBATCH --error=/farm_out/brash/nps_gepq11_target_%j.err

set -euo pipefail

source /etc/profile.d/modules.sh
module purge
module use /group/halla/modulefiles
module load geant4/11.2.1
module load root/6.30.04

work=/work/hallc/gep/brash/gep4_nps_geant4/target_phase_space
events=1000000
cut=0.25
bias=300000
seed=1100401
stem=gepq11_xsbias3e5_${events}_cut0p25
output=$work/${stem}.root
projected=$work/${stem}_nps_projected.root
selected=$work/${stem}_selected.csv
manifest=$work/${stem}_selected_manifest.csv
summary=$work/${stem}_projection.json
thresholds=$work/${stem}_face_rates.csv

export GEP_TARGET_LENGTH_CM=30
/usr/bin/time -v "$work/build/gep_target_phase_space" \
  --bias-electronuclear "$bias" "$output" "$events" 10.700 "$cut" "$seed"

python3 "$work/project_phase_space_to_nps.py" "$output" "$projected" \
  --angle-deg 26.651 --distance-m 3.0 --current-uA 75.0 \
  --threshold-csv "$thresholds" --summary-json "$summary"

python3 "$work/export_nps_phase_space_csv.py" \
  "$output" "$selected" "$manifest" --current-uA 75.0 \
  --selected-groups-root "$projected" --face-hits-only

python3 "$work/split_phase_space_csv.py" "$selected" "$manifest" \
  "$work/${stem}" --chunks 16
