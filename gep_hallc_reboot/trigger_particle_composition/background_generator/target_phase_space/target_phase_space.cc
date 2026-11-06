#include "G4Box.hh"
#include "G4Electron.hh"
#include "G4Event.hh"
#include "G4EventManager.hh"
#include "G4GenericBiasingPhysics.hh"
#include "G4IonTable.hh"
#include "G4LogicalVolume.hh"
#include "G4NistManager.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4PhysListFactory.hh"
#include "G4PVPlacement.hh"
#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4Sphere.hh"
#include "G4Step.hh"
#include "G4SubtractionSolid.hh"
#include "G4SystemOfUnits.hh"
#include "G4Track.hh"
#include "G4Tubs.hh"
#include "G4UnionSolid.hh"
#include "G4UserRunAction.hh"
#include "G4UserStackingAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4VModularPhysicsList.hh"
#include "Randomize.hh"
#include "globals.hh"

#include "TargetCrossSectionBiasOperator.hh"

#include "TFile.h"
#include "TTree.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr G4double kLiquidRadius = 2.5 * CLHEP::cm;
constexpr G4double kCellThickness = 0.132 * CLHEP::mm;
constexpr G4double kCenteredSourceVertexShiftZ = 1.25 * CLHEP::cm;
G4double gLiquidPath = 10. * CLHEP::cm;

G4double StraightLength() { return gLiquidPath - kLiquidRadius; }
G4double HemisphereCenterZ() { return 0.5 * StraightLength(); }
G4double LiquidEntranceZ() { return -0.5 * StraightLength(); }
G4double LiquidExitZ() { return HemisphereCenterZ() + kLiquidRadius; }

G4VSolid* MakeCellShape(const G4String& name, G4double radius,
                        G4double entranceExtension) {
  const G4double straightLength = StraightLength() + entranceExtension;
  const G4double centerZ = -0.5 * entranceExtension;
  auto* cylinder = new G4Tubs(name + "_cylinder", 0., radius,
                              0.5 * straightLength, 0., CLHEP::twopi);
  auto* hemisphere = new G4Sphere(name + "_hemisphere", 0., radius, 0.,
                                  CLHEP::twopi, 0., 0.5 * CLHEP::pi);
  return new G4UnionSolid(name, cylinder, hemisphere, nullptr,
                          G4ThreeVector(0., 0.,
                              HemisphereCenterZ() - centerZ));
}

class TargetDetectorConstruction final : public G4VUserDetectorConstruction {
 public:
  explicit TargetDetectorConstruction(G4double electronNuclearBias = 1.)
      : electronNuclearBias_(electronNuclearBias) {}

  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* vacuum = nist->FindOrBuildMaterial("G4_Galactic");
    auto* liquidHydrogen = nist->FindOrBuildMaterial("G4_lH2");
    auto* aluminum = nist->FindOrBuildMaterial("G4_Al");

    auto* worldSolid = new G4Box("World_solid", 20. * CLHEP::cm,
                                 20. * CLHEP::cm, 20. * CLHEP::cm);
    auto* worldLogical = new G4LogicalVolume(worldSolid, vacuum, "World_log");
    auto* world = new G4PVPlacement(nullptr, {}, worldLogical, "World", nullptr,
                                    false, 0, true);

    auto* liquidSolid = MakeCellShape("TargetLiquid_solid", kLiquidRadius, 0.);
    auto* outerSolid = MakeCellShape("TargetCellOuter_solid",
                                     kLiquidRadius + kCellThickness,
                                     kCellThickness);
    // The outer solid is placed upstream by half a wall thickness so its
    // entrance window extends beyond the liquid while both hemispheres share
    // the same center.  Express the liquid cavity in that outer-solid frame.
    auto* shellSolid = new G4SubtractionSolid(
        "TargetCell_solid", outerSolid, liquidSolid, nullptr,
        G4ThreeVector(0., 0., 0.5*kCellThickness));
    auto* shellLogical = new G4LogicalVolume(shellSolid, aluminum,
                                             "TargetCell_log");
    new G4PVPlacement(nullptr, G4ThreeVector(0., 0., -0.5*kCellThickness),
                      shellLogical, "TargetCell", worldLogical, false, 0, true);

    liquidLogical_ = new G4LogicalVolume(liquidSolid, liquidHydrogen,
                                         "TargetLiquid_log");
    new G4PVPlacement(nullptr, {}, liquidLogical_, "TargetLiquid", worldLogical,
                      false, 0, true);
    return world;
  }

  void ConstructSDandField() override {
    if (electronNuclearBias_ <= 1.) return;
    if (!liquidLogical_) {
      G4Exception("TargetDetectorConstruction::ConstructSDandField",
                  "GEPTARGETBIAS002", FatalException,
                  "Target liquid logical volume was not constructed");
    }
    auto* bias = new TargetCrossSectionBiasOperator(
        "e-", electronNuclearBias_, "TargetLiquidElectronNuclearXSbias");
    bias->AttachTo(liquidLogical_);
    G4cout << "TARGET_BIAS electronNuclear cross-section enhancement="
           << electronNuclearBias_ << " attached to TargetLiquid_log" << G4endl;
  }

 private:
  G4double electronNuclearBias_ = 1.;
  G4LogicalVolume* liquidLogical_ = nullptr;
};

class BeamGenerator final : public G4VUserPrimaryGeneratorAction {
 public:
  explicit BeamGenerator(G4double totalEnergy)
      : gun_(1), totalEnergy_(totalEnergy) {
    const G4double kinetic = totalEnergy_ - G4Electron::Electron()->GetPDGMass();
    if (kinetic <= 0.) throw std::runtime_error("Beam energy is below e- mass");
    gun_.SetParticleDefinition(G4Electron::Electron());
    gun_.SetParticleEnergy(kinetic);
    gun_.SetParticleMomentumDirection(G4ThreeVector(0., 0., 1.));
    gun_.SetParticlePosition(G4ThreeVector(
        0., 0., LiquidEntranceZ() - kCellThickness - 1. * CLHEP::mm));
  }

  void GeneratePrimaries(G4Event* event) override {
    gun_.GeneratePrimaryVertex(event);
  }

 private:
  G4ParticleGun gun_;
  G4double totalEnergy_;
};

struct SourceParticle {
  G4int pdg = 0;
  G4double totalEnergy = 0.;
  G4ThreeVector momentum;
};

struct SourceEvent {
  Long64_t sourceId = 0;
  G4double weightHz = 0.;
  G4double beamEnergy = 0.;
  G4ThreeVector vertex;
  std::vector<SourceParticle> particles;
};

std::vector<double> ParseCsvNumbers(const std::string& line,
                                    std::size_t expected,
                                    std::size_t lineNumber) {
  std::vector<double> values;
  std::stringstream stream(line);
  std::string field;
  while (std::getline(stream, field, ',')) {
    std::size_t used = 0;
    const double value = std::stod(field, &used);
    if (used != field.size() || !std::isfinite(value)) {
      throw std::runtime_error("invalid numeric field on source line " +
                               std::to_string(lineNumber));
    }
    values.push_back(value);
  }
  if (values.size() != expected) {
    throw std::runtime_error("source line " + std::to_string(lineNumber) +
                             " has " + std::to_string(values.size()) +
                             " fields; expected " + std::to_string(expected));
  }
  return values;
}

class SourceEventGenerator final : public G4VUserPrimaryGeneratorAction {
 public:
  explicit SourceEventGenerator(const std::string& path) : gun_(1) {
    std::ifstream stream(path);
    if (!stream) throw std::runtime_error("cannot open source file: " + path);
    std::string version;
    if (!std::getline(stream, version)) {
      throw std::runtime_error("empty source file: " + path);
    }
    enum Format { kElectron, kPi0, kHadron } format;
    std::size_t fields = 0;
    if (version == "# simc_geant4_hybrid_v1") {
      format = kElectron; fields = 23;
    } else if (version == "# simc_geant4_pi0_v1") {
      format = kPi0; fields = 18;
    } else if (version == "# simc_geant4_hadron_v1") {
      format = kHadron; fields = 11;
    } else {
      throw std::runtime_error("unsupported source version: " + version);
    }
    std::string line;
    for (int i = 0; i < 3; ++i) {
      if (!std::getline(stream, line)) {
        throw std::runtime_error("truncated source header");
      }
    }
    std::size_t lineNumber = 4;
    while (std::getline(stream, line)) {
      ++lineNumber;
      if (line.empty()) continue;
      const auto row = ParseCsvNumbers(line, fields, lineNumber);
      SourceEvent event;
      event.sourceId = static_cast<Long64_t>(row[0]);
      event.weightHz = row[1];
      event.vertex = G4ThreeVector(
          row[2] * CLHEP::cm, row[3] * CLHEP::cm,
          row[4] * CLHEP::cm + kCenteredSourceVertexShiftZ);
      event.beamEnergy = row[5] * CLHEP::GeV;
      if (event.weightHz < 0. || event.beamEnergy <= 0.) {
        throw std::runtime_error("nonphysical source weight or beam energy");
      }
      if (format == kElectron) {
        event.particles.push_back(
            {11, row[6] * CLHEP::GeV,
             G4ThreeVector(row[7], row[8], row[9]) * CLHEP::GeV});
      } else if (format == kPi0) {
        event.particles.push_back(
            {22, row[6] * CLHEP::GeV,
             G4ThreeVector(row[7], row[8], row[9]) * CLHEP::GeV});
        event.particles.push_back(
            {22, row[10] * CLHEP::GeV,
             G4ThreeVector(row[11], row[12], row[13]) * CLHEP::GeV});
      } else {
        event.particles.push_back(
            {static_cast<G4int>(row[6]), row[7] * CLHEP::GeV,
             G4ThreeVector(row[8], row[9], row[10]) * CLHEP::GeV});
      }
      for (const auto& particle : event.particles) {
        auto* definition = G4ParticleTable::GetParticleTable()
                               ->FindParticle(particle.pdg);
        if (!definition || particle.momentum.mag2() <= 0.) {
          throw std::runtime_error("unknown or zero-momentum source particle");
        }
        const G4double expected = std::sqrt(
            particle.momentum.mag2() +
            definition->GetPDGMass() * definition->GetPDGMass());
        if (std::fabs(expected - particle.totalEnergy) > 1.e-5 * CLHEP::GeV) {
          throw std::runtime_error("source particle fails mass-shell check");
        }
      }
      totalInputRateHz_ += event.weightHz;
      if (events_.empty()) beamEnergy_ = event.beamEnergy;
      else if (std::fabs(event.beamEnergy - beamEnergy_) > 1.e-9*CLHEP::GeV) {
        throw std::runtime_error("source file mixes beam energies");
      }
      events_.push_back(std::move(event));
    }
    if (events_.empty()) throw std::runtime_error("source file has no events");
    G4cout << "SOURCE_EVENTS loaded=" << events_.size()
           << " total_input_rate_Hz=" << totalInputRateHz_ << G4endl;
  }

  void GeneratePrimaries(G4Event* event) override {
    const auto index = static_cast<std::size_t>(event->GetEventID());
    if (index >= events_.size()) {
      throw std::runtime_error("source event index is out of range");
    }
    const auto& source = events_[index];
    for (const auto& particle : source.particles) {
      auto* definition = G4ParticleTable::GetParticleTable()
                             ->FindParticle(particle.pdg);
      gun_.SetParticleDefinition(definition);
      gun_.SetParticlePosition(source.vertex);
      gun_.SetParticleEnergy(particle.totalEnergy - definition->GetPDGMass());
      gun_.SetParticleMomentumDirection(particle.momentum.unit());
      gun_.SetParticleWeight(source.weightHz);
      gun_.GeneratePrimaryVertex(event);
    }
  }

  Long64_t EventCount() const { return events_.size(); }
  G4double BeamEnergy() const { return beamEnergy_; }
  G4double TotalInputRateHz() const { return totalInputRateHz_; }

 private:
  G4ParticleGun gun_;
  std::vector<SourceEvent> events_;
  G4double beamEnergy_ = 0.;
  G4double totalInputRateHz_ = 0.;
};

struct PhaseSpaceRow {
  Long64_t event_id = 0;
  Int_t track_id = 0;
  Int_t parent_id = 0;
  Int_t pdg_id = 0;
  Int_t surface = 0;
  Bool_t is_primary = false;
  Bool_t electronuclear_lineage = false;
  Int_t electronuclear_group = -1;
  Double_t weight = 1.;
  Double_t x_cm = 0.;
  Double_t y_cm = 0.;
  Double_t z_cm = 0.;
  Double_t vertex_x_cm = 0.;
  Double_t vertex_y_cm = 0.;
  Double_t vertex_z_cm = 0.;
  Double_t vertex_kinetic_GeV = 0.;
  Double_t px_GeV = 0.;
  Double_t py_GeV = 0.;
  Double_t pz_GeV = 0.;
  Double_t kinetic_GeV = 0.;
  Double_t total_GeV = 0.;
  Double_t time_ns = 0.;
  std::string particle;
  std::string creator_process;
};

class PhaseSpaceWriter final : public G4UserRunAction {
 public:
  PhaseSpaceWriter(std::string output, G4double beamEnergy,
                   G4double secondaryCut, Long64_t requestedEvents,
                   G4int normalizationMode = 0,
                   G4double totalInputRateHz = 0.,
                   G4double electronNuclearBias = 1.)
      : output_(std::move(output)), beamEnergy_(beamEnergy),
        secondaryCut_(secondaryCut), requestedEvents_(requestedEvents),
        normalizationMode_(normalizationMode),
        totalInputRateHz_(totalInputRateHz),
        electronNuclearBias_(electronNuclearBias) {}

  void BeginOfRunAction(const G4Run*) override {
    file_ = TFile::Open(output_.c_str(), "RECREATE");
    if (!file_ || file_->IsZombie()) {
      throw std::runtime_error("Cannot create output ROOT file: " + output_);
    }
    tree_ = new TTree("phase_space", "Particles exiting the GEp target cell");
    tree_->Branch("event_id", &row_.event_id);
    tree_->Branch("track_id", &row_.track_id);
    tree_->Branch("parent_id", &row_.parent_id);
    tree_->Branch("pdg_id", &row_.pdg_id);
    tree_->Branch("surface", &row_.surface);
    tree_->Branch("is_primary", &row_.is_primary);
    tree_->Branch("electronuclear_lineage", &row_.electronuclear_lineage);
    tree_->Branch("electronuclear_group", &row_.electronuclear_group);
    tree_->Branch("weight", &row_.weight);
    tree_->Branch("x_cm", &row_.x_cm);
    tree_->Branch("y_cm", &row_.y_cm);
    tree_->Branch("z_cm", &row_.z_cm);
    tree_->Branch("vertex_x_cm", &row_.vertex_x_cm);
    tree_->Branch("vertex_y_cm", &row_.vertex_y_cm);
    tree_->Branch("vertex_z_cm", &row_.vertex_z_cm);
    tree_->Branch("vertex_kinetic_GeV", &row_.vertex_kinetic_GeV);
    tree_->Branch("px_GeV", &row_.px_GeV);
    tree_->Branch("py_GeV", &row_.py_GeV);
    tree_->Branch("pz_GeV", &row_.pz_GeV);
    tree_->Branch("kinetic_GeV", &row_.kinetic_GeV);
    tree_->Branch("total_GeV", &row_.total_GeV);
    tree_->Branch("time_ns", &row_.time_ns);
    tree_->Branch("particle", &row_.particle);
    tree_->Branch("creator_process", &row_.creator_process);
  }

  void Record(const G4Step* step, G4int electronNuclearGroup) {
    const auto* track = step->GetTrack();
    const auto* post = step->GetPostStepPoint();
    const auto position = post->GetPosition();
    const auto momentum = post->GetMomentum();
    const auto* definition = track->GetParticleDefinition();
    const auto* creator = track->GetCreatorProcess();
    row_.event_id = G4EventManager::GetEventManager()
                        ->GetConstCurrentEvent()->GetEventID();
    row_.track_id = track->GetTrackID();
    row_.parent_id = track->GetParentID();
    row_.pdg_id = definition->GetPDGEncoding();
    row_.is_primary = track->GetParentID() == 0;
    row_.electronuclear_lineage = electronNuclearGroup >= 0;
    row_.electronuclear_group = electronNuclearGroup;
    row_.weight = track->GetWeight();
    row_.x_cm = position.x() / CLHEP::cm;
    row_.y_cm = position.y() / CLHEP::cm;
    row_.z_cm = position.z() / CLHEP::cm;
    row_.vertex_x_cm = track->GetVertexPosition().x() / CLHEP::cm;
    row_.vertex_y_cm = track->GetVertexPosition().y() / CLHEP::cm;
    row_.vertex_z_cm = track->GetVertexPosition().z() / CLHEP::cm;
    row_.vertex_kinetic_GeV = track->GetVertexKineticEnergy() / CLHEP::GeV;
    row_.px_GeV = momentum.x() / CLHEP::GeV;
    row_.py_GeV = momentum.y() / CLHEP::GeV;
    row_.pz_GeV = momentum.z() / CLHEP::GeV;
    row_.kinetic_GeV = post->GetKineticEnergy() / CLHEP::GeV;
    row_.total_GeV = post->GetTotalEnergy() / CLHEP::GeV;
    row_.time_ns = post->GetGlobalTime() / CLHEP::ns;
    row_.particle = definition->GetParticleName();
    row_.creator_process = creator ? creator->GetProcessName() : "primary";
    if (position.z() < LiquidEntranceZ()) row_.surface = -1;
    else if (position.z() > LiquidExitZ()) row_.surface = 1;
    else row_.surface = 0;
    tree_->Fill();
    ++particlesWritten_;
  }

  void EndOfRunAction(const G4Run* run) override {
    Long64_t completedEvents = run->GetNumberOfEvent();
    auto* metadata = new TTree("run_info", "Target phase-space normalization");
    metadata->Branch("requested_events", &requestedEvents_);
    metadata->Branch("completed_events", &completedEvents);
    metadata->Branch("particles_written", &particlesWritten_);
    metadata->Branch("beam_total_energy_GeV", &beamEnergy_);
    metadata->Branch("secondary_kinetic_cut_GeV", &secondaryCut_);
    metadata->Branch("normalization_mode", &normalizationMode_);
    metadata->Branch("total_input_event_rate_Hz", &totalInputRateHz_);
    metadata->Branch("electronNuclear_xs_enhancement", &electronNuclearBias_);
    Double_t sourceVertexShiftCm = normalizationMode_ == 1
        ? kCenteredSourceVertexShiftZ / CLHEP::cm : 0.;
    metadata->Branch("source_vertex_z_shift_cm", &sourceVertexShiftCm);
    Double_t liquidPathCm = gLiquidPath / CLHEP::cm;
    Double_t cellThicknessMm = kCellThickness / CLHEP::mm;
    metadata->Branch("liquid_path_cm", &liquidPathCm);
    metadata->Branch("cell_thickness_mm", &cellThicknessMm);
    metadata->Fill();
    file_->cd();
    tree_->Write();
    metadata->Write();
    file_->Close();
    G4cout << "PHASE_SPACE_SUMMARY completed_events=" << completedEvents
           << " particles_written=" << particlesWritten_
           << " output=" << output_ << G4endl;
  }

 private:
  std::string output_;
  Double_t beamEnergy_;
  Double_t secondaryCut_;
  Long64_t requestedEvents_;
  Int_t normalizationMode_;
  Double_t totalInputRateHz_;
  Double_t electronNuclearBias_;
  Long64_t particlesWritten_ = 0;
  PhaseSpaceRow row_;
  TFile* file_ = nullptr;
  TTree* tree_ = nullptr;
};

class TrackClassifier final : public G4UserStackingAction {
 public:
  explicit TrackClassifier(G4double cut) : cut_(cut) {}

  void PrepareNewEvent() override {
    trackGroups_.clear();
    directGroups_.clear();
  }

  G4ClassificationOfNewTrack ClassifyNewTrack(const G4Track* track) override {
    const auto* creator = track->GetCreatorProcess();
    const G4bool directElectronNuclear = creator &&
        creator->GetProcessName().find("electronNuclear") != std::string::npos;
    if (directElectronNuclear) {
      G4int group = -1;
      for (const auto& candidate : directGroups_) {
        const G4double scale = std::max(std::fabs(candidate.weight),
                                        std::fabs(track->GetWeight()));
        if (candidate.parent == track->GetParentID() &&
            std::fabs(candidate.weight - track->GetWeight()) <=
                1.e-12 * std::max(scale, 1.e-300)) {
          group = candidate.id;
          break;
        }
      }
      if (group < 0) {
        group = static_cast<G4int>(directGroups_.size());
        directGroups_.push_back({track->GetParentID(), track->GetWeight(), group});
      }
      trackGroups_[track->GetTrackID()] = group;
    } else {
      const auto parent = trackGroups_.find(track->GetParentID());
      if (parent != trackGroups_.end()) {
        trackGroups_[track->GetTrackID()] = parent->second;
      }
    }
    if (track->GetParentID() > 0 && track->GetKineticEnergy() < cut_) {
      return fKill;
    }
    return fUrgent;
  }

  G4int ElectronNuclearGroup(const G4Track* track) const {
    const auto found = trackGroups_.find(track->GetTrackID());
    return found == trackGroups_.end() ? -1 : found->second;
  }

 private:
  struct DirectGroup {
    G4int parent;
    G4double weight;
    G4int id;
  };
  G4double cut_;
  std::unordered_map<G4int, G4int> trackGroups_;
  std::vector<DirectGroup> directGroups_;
};

class BoundaryRecorder final : public G4UserSteppingAction {
 public:
  BoundaryRecorder(PhaseSpaceWriter* writer, G4double secondaryCut,
                   const TrackClassifier* classifier,
                   G4bool killAtTargetExit = true)
      : writer_(writer), secondaryCut_(secondaryCut), classifier_(classifier),
        killAtTargetExit_(killAtTargetExit) {}

  void UserSteppingAction(const G4Step* step) override {
    auto* track = step->GetTrack();
    const auto* preVolume = step->GetPreStepPoint()->GetPhysicalVolume();
    const auto* postVolume = step->GetPostStepPoint()->GetPhysicalVolume();
    if (!preVolume || !postVolume) return;
    const G4String& preName = preVolume->GetName();
    if (postVolume->GetName() == "World" &&
        (preName == "TargetCell" || preName == "TargetLiquid")) {
      if (track->GetParentID() == 0 ||
          step->GetPostStepPoint()->GetKineticEnergy() >= secondaryCut_) {
        writer_->Record(step, classifier_->ElectronNuclearGroup(track));
      }
      // A force-collision branch must leave the biased volume normally so
      // Geant4 can close its weight bookkeeping. The surrounding world is
      // small and empty, so allowing those tracks to reach its boundary has
      // negligible cost. Analog/source modes retain the early kill.
      if (killAtTargetExit_) track->SetTrackStatus(fStopAndKill);
    }
  }

 private:
  PhaseSpaceWriter* writer_;
  G4double secondaryCut_;
  const TrackClassifier* classifier_;
  G4bool killAtTargetExit_;
};

Long64_t ParsePositiveInteger(const char* text, const char* label) {
  std::size_t used = 0;
  const std::string value(text);
  const long long parsed = std::stoll(value, &used);
  if (used != value.size() || parsed <= 0) {
    throw std::runtime_error(std::string(label) + " must be a positive integer");
  }
  return parsed;
}

double ParsePositiveDouble(const char* text, const char* label,
                           bool allowZero = false) {
  std::size_t used = 0;
  const std::string value(text);
  const double parsed = std::stod(value, &used);
  if (used != value.size() || !std::isfinite(parsed) ||
      (allowZero ? parsed < 0. : parsed <= 0.)) {
    throw std::runtime_error(std::string(label) + " is invalid");
  }
  return parsed;
}

}  // namespace

int main(int argc, char** argv) {
  const bool sourceMode = argc == 6 && std::string(argv[1]) == "--source-events";
  const bool biasedBeamMode =
      argc == 8 && std::string(argv[1]) == "--bias-electronuclear";
  if ((!sourceMode && !biasedBeamMode && argc != 6) ||
      (sourceMode && argc != 6) || (biasedBeamMode && argc != 8)) {
    std::cerr << "Usage: " << argv[0]
              << " output.root n_events beam_total_energy_GeV"
              << " secondary_kinetic_cut_GeV seed\n"
              << "   or: " << argv[0]
              << " --source-events input.csv output.root"
              << " secondary_kinetic_cut_GeV seed\n"
              << "   or: " << argv[0]
              << " --bias-electronuclear enhancement output.root n_events"
              << " beam_total_energy_GeV secondary_kinetic_cut_GeV seed\n";
    return 2;
  }
  try {
    if (const char* targetLength = std::getenv("GEP_TARGET_LENGTH_CM")) {
      gLiquidPath = ParsePositiveDouble(targetLength, "target length") *
                    CLHEP::cm;
    }
    if (gLiquidPath <= kLiquidRadius) {
      throw std::runtime_error(
          "target length must exceed the 2.5 cm downstream-hemisphere radius");
    }
    const std::string output(sourceMode ? argv[3] :
                             (biasedBeamMode ? argv[3] : argv[1]));
    Long64_t events = sourceMode ? 0 : ParsePositiveInteger(
        biasedBeamMode ? argv[4] : argv[2], "n_events");
    G4double beamEnergy = sourceMode ? 0. :
        ParsePositiveDouble(biasedBeamMode ? argv[5] : argv[3],
                            "beam energy") * CLHEP::GeV;
    const G4double cut = ParsePositiveDouble(
        biasedBeamMode ? argv[6] : argv[4], "secondary cut", true) * CLHEP::GeV;
    const long seed = ParsePositiveInteger(
        biasedBeamMode ? argv[7] : argv[5], "seed");
    const G4double biasFactor = biasedBeamMode
        ? ParsePositiveDouble(argv[2], "electronNuclear enhancement") : 1.;
    if (biasedBeamMode && biasFactor <= 1.) {
      throw std::runtime_error("electronNuclear enhancement must exceed one");
    }
    CLHEP::HepRandom::setTheSeed(seed);

    auto runManager = std::make_unique<G4RunManager>();
    runManager->SetUserInitialization(
        new TargetDetectorConstruction(biasFactor));
    G4PhysListFactory factory;
    auto* physics = factory.GetReferencePhysList("QBBC");
    if (!physics) throw std::runtime_error("QBBC physics list is unavailable");
    if (biasedBeamMode) {
      auto* modular = dynamic_cast<G4VModularPhysicsList*>(physics);
      if (!modular) {
        throw std::runtime_error("QBBC is not a modular physics list");
      }
      auto* biasing = new G4GenericBiasingPhysics();
      biasing->Bias("e-", std::vector<G4String>{"electronNuclear"});
      modular->RegisterPhysics(biasing);
    }
    runManager->SetUserInitialization(physics);
    G4VUserPrimaryGeneratorAction* generator = nullptr;
    G4double totalInputRateHz = 0.;
    G4int normalizationMode = 0;
    if (sourceMode) {
      auto* sourceGenerator = new SourceEventGenerator(argv[2]);
      generator = sourceGenerator;
      events = sourceGenerator->EventCount();
      beamEnergy = sourceGenerator->BeamEnergy();
      totalInputRateHz = sourceGenerator->TotalInputRateHz();
      normalizationMode = 1;
    } else {
      generator = new BeamGenerator(beamEnergy);
      if (biasedBeamMode) normalizationMode = 2;
    }
    auto* writer = new PhaseSpaceWriter(
        output, beamEnergy / CLHEP::GeV, cut / CLHEP::GeV, events,
        normalizationMode, totalInputRateHz, biasFactor);
    runManager->SetUserAction(generator);
    runManager->SetUserAction(writer);
    auto* classifier = new TrackClassifier(cut);
    runManager->SetUserAction(new BoundaryRecorder(writer, cut, classifier));
    runManager->SetUserAction(classifier);
    runManager->Initialize();
    G4cout << "PHASE_SPACE_CONFIG beam_total_GeV=" << beamEnergy/CLHEP::GeV
           << " target_length_cm=" << gLiquidPath/CLHEP::cm
           << " secondary_cut_GeV=" << cut/CLHEP::GeV
           << " events=" << events << " seed=" << seed << G4endl;
    if (biasedBeamMode) {
      G4cout << "PHASE_SPACE_NORMALIZATION weighted_per_incident_electron"
             << " electronNuclear_enhancement=" << biasFactor << G4endl;
    }
    runManager->BeamOn(events);
  } catch (const std::exception& error) {
    std::cerr << "ERROR: " << error.what() << '\n';
    return 1;
  }
  return 0;
}

