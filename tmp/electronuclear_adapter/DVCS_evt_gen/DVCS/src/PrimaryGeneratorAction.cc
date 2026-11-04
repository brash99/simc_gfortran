//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
/// \file electromagnetic/TestEm4/src/PrimaryGeneratorAction.cc
/// \brief Implementation of the PrimaryGeneratorAction class
//
//
// $Id: PrimaryGeneratorAction.cc 67268 2013-02-13 11:38:40Z ihrivnac $
//
// 

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#include <math.h>//arccosine(acos), pi(M_PI)

#include "PrimaryGeneratorAction.hh"

#include "EventAction.hh"

#include "dvcsGlobals.hh"

#include "G4Event.hh"
#include "G4ParticleTable.hh"
#include "G4IonTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4Geantino.hh"
#include "G4SystemOfUnits.hh"
#include "G4DynamicParticle.hh"
#include "G4ElectroNuclearCrossSection.hh"
#include "G4ElectroVDNuclearModel.hh"
#include "G4HadFinalState.hh"
#include "G4HadProjectile.hh"
#include "G4HadSecondary.hh"
#include "G4Nucleus.hh"
#include "G4NistManager.hh"
#include "Randomize.hh"

#include "G4ParticleGun.hh"

#include "G4ParticleTable.hh"

#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
const char* kSimcVersion = "# simc_geant4_hybrid_v1";
const char* kSimcPi0Version = "# simc_geant4_pi0_v1";
const char* kSimcElectronPhotonVersion = "# simc_geant4_electron_photon_v1";
const char* kSimcHadronVersion = "# simc_geant4_hadron_v1";
const char* kGenericPhaseSpaceVersion = "# gep_nps_phase_space_v1";
const char* kSimcSchema =
    "event_id,weight,vertex_x_cm,vertex_y_cm,vertex_z_cm,vertex_beam_E_GeV,"
    "vertex_e_E_GeV,vertex_e_px_GeV,vertex_e_py_GeV,vertex_e_pz_GeV,"
    "rad_tail,rad1_GeV,rad2_GeV,rad3_GeV,target_eloss_GeV,target_teff_X0,"
    "primary_x_cm,primary_y_cm,primary_z_cm,primary_E_GeV,primary_px_GeV,"
    "primary_py_GeV,primary_pz_GeV";
const char* kSimcPi0Schema =
    "event_id,weight,vertex_x_cm,vertex_y_cm,vertex_z_cm,beam_E_GeV,"
    "gamma1_E_GeV,gamma1_px_GeV,gamma1_py_GeV,gamma1_pz_GeV,"
    "gamma2_E_GeV,gamma2_px_GeV,gamma2_py_GeV,gamma2_pz_GeV,"
    "proton_E_GeV,proton_px_GeV,proton_py_GeV,proton_pz_GeV";
const char* kSimcElectronPhotonSchema =
    "event_id,weight,vertex_x_cm,vertex_y_cm,vertex_z_cm,vertex_beam_E_GeV,"
    "vertex_e_E_GeV,vertex_e_px_GeV,vertex_e_py_GeV,vertex_e_pz_GeV,"
    "rad_tail,rad1_GeV,rad2_GeV,rad3_GeV,target_eloss_GeV,target_teff_X0,"
    "primary_x_cm,primary_y_cm,primary_z_cm,primary_E_GeV,primary_px_GeV,"
    "primary_py_GeV,primary_pz_GeV,fsr_E_GeV,fsr_px_GeV,fsr_py_GeV,fsr_pz_GeV";
const char* kSimcHadronSchema =
    "event_id,weight,vertex_x_cm,vertex_y_cm,vertex_z_cm,beam_E_GeV,"
    "pdg_id,primary_E_GeV,primary_px_GeV,primary_py_GeV,primary_pz_GeV";

std::vector<double> ParseSimcRow(const std::string& line, long line_number,
                                 std::size_t expected_fields)
{
    std::vector<double> values;
    std::stringstream stream(line);
    std::string field;
    while (std::getline(stream, field, ',')) {
        std::size_t used = 0;
        double value = 0.;
        try {
            value = std::stod(field, &used);
        } catch (const std::exception&) {
            throw std::runtime_error("Invalid numeric field in SIMC CSV line " +
                                     std::to_string(line_number));
        }
        if (used != field.size() || !std::isfinite(value)) {
            throw std::runtime_error("Invalid numeric field in SIMC CSV line " +
                                     std::to_string(line_number));
        }
        values.push_back(value);
    }
    if (expected_fields != 0 && values.size() != expected_fields) {
        throw std::runtime_error("Unexpected field count in SIMC CSV line " +
                                 std::to_string(line_number) + ", found " +
                                 std::to_string(values.size()));
    }
    return values;
}
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PrimaryGeneratorAction::PrimaryGeneratorAction(EventAction* evt, long seed1, long seed2)
    : G4VUserPrimaryGeneratorAction(),
        fEventAction(evt),
        M_targ(-999.),
        fParticleGun(0),
        gEv(0),
        L_elec_init(0), L_elec_scat(0), L_phot_final(0), L_prot_final(0),
        fSimcLineNumber(0),
        fSimcPi0(false),
        fSimcElectronPhoton(false),
        fSimcHadron(false),
        fGenericPhaseSpace(false),
        fElectronuclearModel(0),
        fElectronuclearCrossSection(0),
        fElectronuclearSecondaryCut(0.),
        fElectronuclearGeneratorOnly(false),
        fElectronuclearEvents(0),
        fElectronuclearSecondaries(0),
        fElectronuclearDiscarded(0),
        fElectronuclearDiscardedKineticEnergy(0.)
{
    //let's use dvcsGlobals with [mm], [rad] and [GeV] units
    G4double BeamEnergy = dvcsGlobals::Ebeam;
    G4double HMSangle = dvcsGlobals::HMS_angle;
    G4double HMSmomentum = dvcsGlobals::HMS_momentum;
    G4double Calo_distance = 10.*dvcsGlobals::Calo_distance;//[cm] to [mm]
    G4double Calo_angle = dvcsGlobals::Calo_angle;
    target_gen_proc_type = dvcsGlobals::target_gen_proc_type;
    G4double target_density = dvcsGlobals::target_density;
    G4double target_length = dvcsGlobals::target_length;

    G4cout<<"PrimaryGeneratorAction Calo_angle : "<<Calo_angle*TMath::RadToDeg()<<G4endl;

    if( target_gen_proc_type == 0 || target_gen_proc_type == 1 || target_gen_proc_type ==2){
        if(target_gen_proc_type == 0) {M_targ = 0.9383;}
        else if( target_gen_proc_type == 1 ) {M_targ = 0.939565378;}
        else if( target_gen_proc_type == 2 ) {M_targ = 1.875612859;}
    }
    else{
        cout<<"Primary Generator Action!! Error: wrong target mass "<<endl;
    }

    if (dvcsGlobals::primary_mode == 0) {
        gEv=new TGenDVCS(BeamEnergy, target_gen_proc_type, seed1, seed2);//(beam-energy [GeV], target type, seed1, seed2)//But I do not think the seed1 and seed2 is actually used in TGenDVCS.
        gEv->SetTargetParam(target_length, target_density);//(target-length [cm][DVCSGen takes cm to calculate bremstraulung], target density[g/cm3][geant4 default])

        gEv->SetGeometry(HMSangle, HMSmomentum, Calo_angle, Calo_distance);//(spectrometer angle [rad](cos^-1(1-Q^2/2*k*k')), spec-central momentum [GeV], calo-angle [rad], calo-face distance from targ [mm in TDVCSgen], [mm in simulation and NPS_SOFT])

        gEv->TGenGeo::Print();
    }

    if (dvcsGlobals::primary_mode == 2) {
        fSimcInput.open(dvcsGlobals::simc_event_file.c_str());
        if (!fSimcInput) {
            throw std::runtime_error("Cannot open SIMC event file: " +
                                     dvcsGlobals::simc_event_file);
        }
        std::string line;
        if (!std::getline(fSimcInput, line) ||
            (line != kSimcVersion && line != kSimcPi0Version &&
             line != kSimcElectronPhotonVersion &&
             line != kSimcHadronVersion &&
             line != kGenericPhaseSpaceVersion))
            throw std::runtime_error("Unsupported or missing SIMC CSV version");
        fSimcPi0 = line == kSimcPi0Version;
        fSimcElectronPhoton = line == kSimcElectronPhotonVersion;
        fSimcHadron = line == kSimcHadronVersion;
        fGenericPhaseSpace = line == kGenericPhaseSpaceVersion;
        ++fSimcLineNumber;
        for (int i = 0; i < 2; ++i) {
            if (!std::getline(fSimcInput, line) || line.compare(0, 2, "# ") != 0)
                throw std::runtime_error("Malformed SIMC CSV metadata header");
            ++fSimcLineNumber;
        }
        const char* expected_schema = fSimcPi0 ? kSimcPi0Schema :
            (fSimcElectronPhoton ? kSimcElectronPhotonSchema :
             (fSimcHadron ? kSimcHadronSchema : kSimcSchema));
        if (!std::getline(fSimcInput, line) ||
            (fGenericPhaseSpace
                ? line.compare(0, 40, "event_id,weight_Hz,n_particles,particle_") != 0
                : line != expected_schema))
            throw std::runtime_error("SIMC CSV schema does not match version");
        ++fSimcLineNumber;
    }

    if (dvcsGlobals::primary_mode == 4) {
        fElectronuclearModel = new G4ElectroVDNuclearModel();
        fElectronuclearCrossSection = new G4ElectroNuclearCrossSection();
        const char* cut = std::getenv("GEP_TARGET_SECONDARY_CUT_GEV");
        if (cut && *cut) {
            std::size_t used = 0;
            const double value = std::stod(cut, &used);
            if (used != std::string(cut).size() || !std::isfinite(value) ||
                value < 0.) {
                throw std::runtime_error(
                    "Invalid GEP_TARGET_SECONDARY_CUT_GEV=" +
                    std::string(cut));
            }
            fElectronuclearSecondaryCut = value * CLHEP::GeV;
        }
        const char* generatorOnly = std::getenv("GEP_DIRECT_GENERATOR_ONLY");
        fElectronuclearGeneratorOnly = generatorOnly &&
            std::string(generatorOnly) != "0";
        G4cout << "DIRECT_ELECTRONUCLEAR secondary production-energy cut [GeV]: "
               << fElectronuclearSecondaryCut / CLHEP::GeV
               << " generator_only=" << fElectronuclearGeneratorOnly
               << G4endl;
    }

    G4int n_particle = 1;
    fParticleGun  = new G4ParticleGun(n_particle);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
    if (fElectronuclearModel) {
        G4cout << "DIRECT_ELECTRONUCLEAR_SUMMARY events="
               << fElectronuclearEvents
               << " generated_secondaries=" << fElectronuclearSecondaries
               << " discarded_secondaries=" << fElectronuclearDiscarded
               << " discarded_kinetic_GeV="
               << fElectronuclearDiscardedKineticEnergy / CLHEP::GeV
               << G4endl;
    }
    delete fElectronuclearModel;
    delete fElectronuclearCrossSection;
    delete fParticleGun;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent)
{
    dvcsGlobals::hit_HMS_CALO_flag = false;
    G4bool hit_HMS = false;
    G4ParticleTable *particleTable = G4ParticleTable::GetParticleTable();

    if (dvcsGlobals::primary_mode == 4) {
        const G4double electronMass = 0.510999 * CLHEP::MeV;
        const G4double totalEnergy = dvcsGlobals::mono_energy * CLHEP::GeV;
        const G4double kineticEnergy = totalEnergy - electronMass;
        const G4ThreeVector beamDirection(0., 0., 1.);
        G4DynamicParticle incident(particleTable->FindParticle(11),
                                   beamDirection, kineticEnergy);
        G4HadProjectile projectile(incident);
        G4Nucleus proton(1, 1);
        G4HadFinalState* finalState =
            fElectronuclearModel->ApplyYourself(projectile, proton);
        if (!finalState) {
            throw std::runtime_error(
                "G4ElectroVDNuclearModel returned a null final state");
        }

        // The corrected beam-mode target has a 10.0 cm liquid path on the
        // Hall-z beam axis.  These limits were independently established by
        // the geometry-boundary trace, including the 0.132 mm entrance cell.
        const G4double targetEntranceZ = -3.7434 * CLHEP::cm;
        const G4double targetLength = 10.0 * CLHEP::cm;
        const G4double vertexZ = targetEntranceZ +
                                 G4UniformRand() * targetLength;
        const G4ThreeVector vertex(0., 0., vertexZ);

        G4Material* liquidHydrogen =
            G4NistManager::Instance()->FindOrBuildMaterial("G4_lH2");
        const G4double microscopicCrossSection =
            fElectronuclearCrossSection->GetElementCrossSection(
                &incident, 1, liquidHydrogen);
        const G4double macroscopicCrossSection =
            microscopicCrossSection * liquidHydrogen->GetTotNbOfAtomsPerVolume();
        const G4double interactionProbability =
            1. - std::exp(-macroscopicCrossSection * targetLength);

        const G4double outgoingKinetic = finalState->GetEnergyChange();
        const G4ThreeVector outgoingDirection =
            finalState->GetMomentumChange().unit();
        const G4double outgoingTotal = outgoingKinetic + electronMass;
        const G4double outgoingMomentum = std::sqrt(std::max(
            0., outgoingTotal * outgoingTotal - electronMass * electronMass));

        TLorentzVector initialElectron(0., 0.,
            std::sqrt(totalEnergy * totalEnergy - electronMass * electronMass) /
                CLHEP::GeV,
            totalEnergy / CLHEP::GeV);
        TLorentzVector outgoingElectron(
            outgoingMomentum * outgoingDirection.x() / CLHEP::GeV,
            outgoingMomentum * outgoingDirection.y() / CLHEP::GeV,
            outgoingMomentum * outgoingDirection.z() / CLHEP::GeV,
            outgoingTotal / CLHEP::GeV);
        TLorentzVector placeholderPhoton(1.e-9, 0., 0., 1.e-9);
        TLorentzVector placeholderProton(0., 0., 0., 0.9383);
        fEventAction->DefineWeights(interactionProbability, 0., 0., 0.);
        fEventAction->DefinePrimaries(&initialElectron, &outgoingElectron,
                                      &placeholderPhoton, &placeholderProton,
                                      0., 0., vertexZ / CLHEP::cm);
        dvcsGlobals::hit_HMS_CALO_flag = true;

        if (!fElectronuclearGeneratorOnly &&
            finalState->GetStatusChange() == isAlive && outgoingKinetic > 0.) {
            fParticleGun->SetParticleDefinition(particleTable->FindParticle(11));
            fParticleGun->SetParticleEnergy(outgoingKinetic);
            fParticleGun->SetParticlePosition(vertex);
            fParticleGun->SetParticleMomentumDirection(outgoingDirection);
            fParticleGun->GeneratePrimaryVertex(anEvent);
        }

        const std::size_t count = finalState->GetNumberOfSecondaries();
        fElectronuclearSecondaries += count;
        for (std::size_t i = 0; i < count; ++i) {
            const G4DynamicParticle* secondary =
                finalState->GetSecondary(i)->GetParticle();
            if (!secondary) continue;
            const G4double secondaryKinetic = secondary->GetKineticEnergy();
            if (secondaryKinetic < fElectronuclearSecondaryCut) {
                ++fElectronuclearDiscarded;
                fElectronuclearDiscardedKineticEnergy +=
                    secondaryKinetic;
            } else if (!fElectronuclearGeneratorOnly) {
                fParticleGun->SetParticleDefinition(secondary->GetDefinition());
                fParticleGun->SetParticleEnergy(secondaryKinetic);
                fParticleGun->SetParticlePosition(vertex);
                fParticleGun->SetParticleMomentumDirection(
                    secondary->GetMomentumDirection());
                fParticleGun->GeneratePrimaryVertex(anEvent);
            }
            // ApplyYourself normally transfers these heap objects to the
            // hadronic process.  Direct callers own them instead; the gun has
            // already copied the particle state into the event.
            delete const_cast<G4DynamicParticle*>(secondary);
        }
        finalState->ClearSecondaries();
        ++fElectronuclearEvents;
        if (fElectronuclearEvents <= 10) {
            G4cout << "DIRECT_ELECTRONUCLEAR event=" << fElectronuclearEvents
                   << " vertex_z_cm=" << vertexZ / CLHEP::cm
                   << " cross_section_barn="
                   << microscopicCrossSection / CLHEP::barn
                   << " interaction_probability=" << interactionProbability
                   << " outgoing_e_kinetic_GeV="
                   << outgoingKinetic / CLHEP::GeV
                   << " secondaries=" << count << G4endl;
        }
        return;
    }

    if (dvcsGlobals::primary_mode == 3) {
        const G4double electron_mass_GeV = 0.510999e-3;
        const G4double energy = dvcsGlobals::mono_energy;
        const G4double momentum = std::sqrt(
            energy * energy - electron_mass_GeV * electron_mass_GeV);
        const G4double vertex_z = dvcsGlobals::mono_vertex_z;
        const G4ThreeVector vertex(0., 0., vertex_z * CLHEP::cm);
        const G4ThreeVector direction(0., 0., 1.);

        TLorentzVector initial_electron(0., 0., momentum, energy);
        TLorentzVector placeholder_electron(0., 0., 1.e-9, 1.e-9);
        TLorentzVector placeholder_photon(1.e-9, 0., 0., 1.e-9);
        TLorentzVector placeholder_proton(0., 0., 0., 0.9383);

        dvcsGlobals::hit_HMS_CALO_flag = true;
        fEventAction->DefineWeights(1., 0., 0., 0.);
        fEventAction->DefinePrimaries(&initial_electron, &placeholder_electron,
                                      &placeholder_photon, &placeholder_proton,
                                      0., 0., vertex_z);

        fParticleGun->SetParticleDefinition(particleTable->FindParticle(11));
        fParticleGun->SetParticleEnergy(
            (energy - electron_mass_GeV) * CLHEP::GeV);
        fParticleGun->SetParticlePosition(vertex);
        fParticleGun->SetParticleMomentumDirection(direction);
        fParticleGun->GeneratePrimaryVertex(anEvent);
        return;
    }

    if (dvcsGlobals::primary_mode == 1) {
        const G4double electron_mass_GeV = 0.510999e-3;
        const G4double energy = dvcsGlobals::mono_energy;
        const G4double momentum = std::sqrt(energy * energy -
                                            electron_mass_GeV * electron_mass_GeV);
        const G4double theta = dvcsGlobals::mono_angle;
        const G4double vertex_z = dvcsGlobals::mono_vertex_z;
        const G4double local_x = dvcsGlobals::mono_local_x;
        const G4double local_y = dvcsGlobals::mono_local_y;
        const G4double distance = dvcsGlobals::Calo_distance * CLHEP::cm;
        const G4ThreeVector vertex(0., 0., vertex_z * CLHEP::cm);
        const G4ThreeVector face_center(distance * std::sin(theta), 0.,
                                        distance * std::cos(theta));
        const G4ThreeVector local_x_axis(std::cos(theta), 0.,
                                         -std::sin(theta));
        const G4ThreeVector face_point = face_center
                                         + local_x * CLHEP::mm * local_x_axis
                                         + G4ThreeVector(0., local_y * CLHEP::mm, 0.);
        const G4ThreeVector direction = (face_point - vertex).unit();

        TLorentzVector initial_electron(0., 0., dvcsGlobals::Ebeam,
                                        dvcsGlobals::Ebeam);
        TLorentzVector scattered_electron(momentum * direction.x(),
                                           momentum * direction.y(),
                                           momentum * direction.z(), energy);
        // Placeholders preserve the historical output schema. They are not
        // physics particles and are not fired into GEANT4 in mono mode.
        TLorentzVector placeholder_photon(1.e-9, 0., 0., 1.e-9);
        TLorentzVector placeholder_proton(0., 0., 0., 0.9383);

        dvcsGlobals::hit_HMS_CALO_flag = true;
        fEventAction->DefineWeights(1., 0., 0., 0.);
        fEventAction->DefinePrimaries(&initial_electron, &scattered_electron,
                                      &placeholder_photon, &placeholder_proton,
                                      0., 0., vertex_z);

        fParticleGun->SetParticleDefinition(particleTable->FindParticle(11));
        fParticleGun->SetParticleEnergy((energy - electron_mass_GeV) * CLHEP::GeV);
        fParticleGun->SetParticlePosition(vertex);
        fParticleGun->SetParticleMomentumDirection(direction);
        fParticleGun->GeneratePrimaryVertex(anEvent);
        return;
    }

    if (dvcsGlobals::primary_mode == 2) {
        std::string line;
        if (!std::getline(fSimcInput, line)) {
            throw std::runtime_error("SIMC event stream exhausted after " +
                                     std::to_string(fSimcLineNumber - 4) +
                                     " events");
        }
        ++fSimcLineNumber;
        if (fGenericPhaseSpace) {
            const std::vector<double> row = ParseSimcRow(
                line, fSimcLineNumber, 0);
            if (row.size() < 3 || row[0] < 0. || row[1] < 0. ||
                row[2] < 1. || row[2] != std::floor(row[2])) {
                throw std::runtime_error(
                    "Malformed generic phase-space event in CSV line " +
                    std::to_string(fSimcLineNumber));
            }
            const std::size_t count = static_cast<std::size_t>(row[2]);
            if (row.size() != 3 + 8 * count) {
                throw std::runtime_error(
                    "Generic phase-space particle count mismatch in CSV line " +
                    std::to_string(fSimcLineNumber));
            }
            TLorentzVector initialElectron(0., 0., dvcsGlobals::Ebeam,
                                           dvcsGlobals::Ebeam);
            TLorentzVector placeholderElectron(0., 0., 1.e-9, 1.e-9);
            TLorentzVector placeholderPhoton(1.e-9, 0., 0., 1.e-9);
            TLorentzVector placeholderProton(0., 0., 0., 0.9383);
            fEventAction->DefineWeights(row[1], 0., 0., 0.);
            fEventAction->DefinePrimaries(
                &initialElectron, &placeholderElectron, &placeholderPhoton,
                &placeholderProton, 0., 0., 0.);
            dvcsGlobals::hit_HMS_CALO_flag = true;
            for (std::size_t i = 0; i < count; ++i) {
                const std::size_t offset = 3 + 8 * i;
                const int pdg = static_cast<int>(row[offset]);
                if (row[offset] != pdg) {
                    throw std::runtime_error("Nonintegral PDG ID in CSV line " +
                                             std::to_string(fSimcLineNumber));
                }
                auto* particle = particleTable->FindParticle(pdg);
                if (!particle) {
                    throw std::runtime_error("Unknown PDG ID in CSV line " +
                                             std::to_string(fSimcLineNumber));
                }
                const G4double energy = row[offset + 1];
                const G4ThreeVector hallPosition(
                    row[offset + 2], row[offset + 3], row[offset + 4]);
                const G4ThreeVector hallMomentum(
                    row[offset + 5], row[offset + 6], row[offset + 7]);
                const G4double mass = particle->GetPDGMass() / CLHEP::GeV;
                const G4double expected = std::sqrt(
                    hallMomentum.mag2() + mass * mass);
                if (energy < mass || hallMomentum.mag2() <= 0. ||
                    std::fabs(energy - expected) > 1.e-5) {
                    throw std::runtime_error(
                        "Unphysical generic phase-space particle in CSV line " +
                        std::to_string(fSimcLineNumber));
                }
                // Hall axes: x down, y left, z downstream. GEANT4 axes:
                // x left, y up, z downstream.
                const G4ThreeVector position(
                    hallPosition.y() * CLHEP::cm,
                    -hallPosition.x() * CLHEP::cm,
                    hallPosition.z() * CLHEP::cm);
                const G4ThreeVector momentum(
                    hallMomentum.y(), -hallMomentum.x(), hallMomentum.z());
                fParticleGun->SetParticleDefinition(particle);
                fParticleGun->SetParticlePosition(position);
                fParticleGun->SetParticleEnergy(
                    (energy - mass) * CLHEP::GeV);
                fParticleGun->SetParticleMomentumDirection(momentum.unit());
                fParticleGun->SetParticleWeight(row[1]);
                fParticleGun->GeneratePrimaryVertex(anEvent);
            }
            return;
        }
        const std::vector<double> row = ParseSimcRow(
            line, fSimcLineNumber,
            fSimcPi0 ? 18 : (fSimcElectronPhoton ? 27 :
                             (fSimcHadron ? 11 : 23)));
        if (fSimcPi0) {
            const G4ThreeVector hall_p1(row[7], row[8], row[9]);
            const G4ThreeVector hall_p2(row[11], row[12], row[13]);
            if (row[1] < 0. || row[6] <= 0. || row[10] <= 0. ||
                hall_p1.mag2() <= 0. || hall_p2.mag2() <= 0. ||
                std::fabs(row[6] * row[6] - hall_p1.mag2()) > 1.e-10 ||
                std::fabs(row[10] * row[10] - hall_p2.mag2()) > 1.e-10) {
                throw std::runtime_error("Unphysical SIMC pi0 photon in CSV line " +
                                         std::to_string(fSimcLineNumber));
            }
            const G4ThreeVector p1(row[8], -row[7], row[9]);
            const G4ThreeVector p2(row[12], -row[11], row[13]);
            const G4ThreeVector vertex(row[3] * CLHEP::cm,
                                       -row[2] * CLHEP::cm,
                                       row[4] * CLHEP::cm);
            TLorentzVector initial_electron(0., 0., row[5], row[5]);
            TLorentzVector placeholder_electron(0., 0., 1.e-9, 1.e-9);
            TLorentzVector first_photon(row[7], row[8], row[9], row[6]);
            TLorentzVector proton(row[15], row[16], row[17], row[14]);

            dvcsGlobals::hit_HMS_CALO_flag = true;
            fEventAction->DefineWeights(row[1], 0., 0., 0.);
            fEventAction->DefinePrimaries(&initial_electron, &placeholder_electron,
                                          &first_photon, &proton,
                                          row[2], row[3], row[4]);
            fParticleGun->SetParticleDefinition(particleTable->FindParticle(22));
            fParticleGun->SetParticlePosition(vertex);
            fParticleGun->SetParticleEnergy(row[6] * CLHEP::GeV);
            fParticleGun->SetParticleMomentumDirection(p1.unit());
            fParticleGun->GeneratePrimaryVertex(anEvent);
            fParticleGun->SetParticleEnergy(row[10] * CLHEP::GeV);
            fParticleGun->SetParticleMomentumDirection(p2.unit());
            fParticleGun->GeneratePrimaryVertex(anEvent);
            return;
        }
        if (fSimcHadron) {
            const int pdg_id = static_cast<int>(row[6]);
            if (row[6] != pdg_id || pdg_id != -211) {
                throw std::runtime_error("Unsupported hadron PDG ID in CSV line " +
                                         std::to_string(fSimcLineNumber));
            }
            auto* particle = particleTable->FindParticle(pdg_id);
            if (!particle) {
                throw std::runtime_error("Unknown hadron PDG ID in CSV line " +
                                         std::to_string(fSimcLineNumber));
            }
            const G4double mass_GeV = particle->GetPDGMass() / CLHEP::GeV;
            const G4double energy = row[7];
            const G4ThreeVector hall_momentum(row[8], row[9], row[10]);
            const G4double expected_energy = std::sqrt(
                hall_momentum.mag2() + mass_GeV * mass_GeV);
            if (row[1] < 0. || energy <= mass_GeV ||
                hall_momentum.mag2() <= 0. ||
                std::fabs(energy - expected_energy) > 1.e-6) {
                throw std::runtime_error("Unphysical hadron primary in CSV line " +
                                         std::to_string(fSimcLineNumber));
            }
            const G4ThreeVector momentum(row[9], -row[8], row[10]);
            const G4ThreeVector vertex(row[3] * CLHEP::cm,
                                       -row[2] * CLHEP::cm,
                                       row[4] * CLHEP::cm);
            TLorentzVector initial_electron(0., 0., row[5], row[5]);
            TLorentzVector placeholder_electron(0., 0., 1.e-9, 1.e-9);
            TLorentzVector placeholder_photon(1.e-9, 0., 0., 1.e-9);
            TLorentzVector placeholder_proton(0., 0., 0., 0.9383);

            dvcsGlobals::hit_HMS_CALO_flag = true;
            fEventAction->DefineWeights(row[1], 0., 0., 0.);
            fEventAction->DefinePrimaries(&initial_electron, &placeholder_electron,
                                          &placeholder_photon, &placeholder_proton,
                                          row[2], row[3], row[4]);
            fParticleGun->SetParticleDefinition(particle);
            fParticleGun->SetParticlePosition(vertex);
            fParticleGun->SetParticleEnergy((energy - mass_GeV) * CLHEP::GeV);
            fParticleGun->SetParticleMomentumDirection(momentum.unit());
            fParticleGun->GeneratePrimaryVertex(anEvent);
            return;
        }
        const G4double electron_mass_GeV = 0.510999e-3;
        const G4double energy = row[19];
        const G4ThreeVector hall_momentum(row[20], row[21], row[22]);
        const G4double p2 = hall_momentum.mag2();
        const G4double mass_shell = energy * energy - p2;
        const G4double electron_m2 = electron_mass_GeV * electron_mass_GeV;
        if (row[1] < 0. || energy <= electron_mass_GeV || p2 <= 0. ||
            std::fabs(mass_shell - electron_m2) > 1.e-10) {
            throw std::runtime_error("Unphysical SIMC primary in CSV line " +
                                     std::to_string(fSimcLineNumber));
        }
        G4ThreeVector hall_fsr;
        if (fSimcElectronPhoton) {
            hall_fsr = G4ThreeVector(row[24], row[25], row[26]);
            if (row[23] < 0. || (row[23] > 0. &&
                (hall_fsr.mag2() <= 0. ||
                 std::fabs(row[23] * row[23] - hall_fsr.mag2()) > 1.e-10))) {
                throw std::runtime_error("Unphysical SIMC FSR photon in CSV line " +
                                         std::to_string(fSimcLineNumber));
            }
        }

        // SIMC Hall axes are x down, y left, z downstream. This GEANT4
        // geometry uses x left, y up, z downstream.
        const G4ThreeVector momentum(row[21], -row[20], row[22]);
        const G4ThreeVector vertex(row[17] * CLHEP::cm,
                                   -row[16] * CLHEP::cm,
                                   row[18] * CLHEP::cm);
        TLorentzVector initial_electron(0., 0., row[5], row[5]);
        TLorentzVector scattered_electron(row[20], row[21], row[22], energy);
        TLorentzVector placeholder_photon(1.e-9, 0., 0., 1.e-9);
        TLorentzVector fsr_photon;
        TLorentzVector* output_photon = &placeholder_photon;
        if (fSimcElectronPhoton && row[23] > 0.) {
            fsr_photon.SetPxPyPzE(row[24], row[25], row[26], row[23]);
            output_photon = &fsr_photon;
        }
        TLorentzVector placeholder_proton(0., 0., 0., 0.9383);

        dvcsGlobals::hit_HMS_CALO_flag = true;
        fEventAction->DefineWeights(row[1], 0., 0., 0.);
        fEventAction->DefinePrimaries(&initial_electron, &scattered_electron,
                                      output_photon, &placeholder_proton,
                                      row[16], row[17], row[18]);
        fParticleGun->SetParticleDefinition(particleTable->FindParticle(11));
        fParticleGun->SetParticleEnergy((energy - electron_mass_GeV) * CLHEP::GeV);
        fParticleGun->SetParticlePosition(vertex);
        fParticleGun->SetParticleMomentumDirection(momentum.unit());
        fParticleGun->GeneratePrimaryVertex(anEvent);
        if (fSimcElectronPhoton && row[23] > 0.) {
            const G4ThreeVector fsr_momentum(row[25], -row[24], row[26]);
            fParticleGun->SetParticleDefinition(particleTable->FindParticle(22));
            fParticleGun->SetParticleEnergy(row[23] * CLHEP::GeV);
            fParticleGun->SetParticleMomentumDirection(fsr_momentum.unit());
            fParticleGun->GeneratePrimaryVertex(anEvent);
        }
        return;
    }

    gEv->GenerateVertex();//Generates the vertex //[cm]
    gEv->ExtBrem();//Make external pre-vertex radiative corrections
    gEv->GenKin();//Computes scattered electron kinematics
    gEv->IntRCBef();//Internal radiative corrections (before vertex)

    if(gEv->ComputeElectron()){//Check hor spectro accep.

        gEv->IntRCAft();//Internal radiative corrections (after vertex)

        TLorentzVector L_b(0, 0, dvcsGlobals::Ebeam, dvcsGlobals::Ebeam);
        TLorentzVector L_scat_el = *gEv->GetScatteredElectron();
        TLorentzVector L_init_phot = L_b - L_scat_el;
        G4double nu = L_init_phot.E();

        TVector3 k_v = TVector3(L_b.Vect());
        TVector3 kp_v = TVector3(L_scat_el.Vect());

        G4double Q2 = 2*k_v.Mag()*kp_v.Mag()*(1 - cos(k_v.Angle(kp_v))); // Q2 = 2p1p2*(1 - cos(theta_12))

        G4double xB = Q2/(2*M_targ*nu);
        G4double q3=TMath::Sqrt(Q2+TMath::Power(nu,2.));
        G4double q0primemax=0.5*Q2*(1.-xB)/(xB*(M_targ+nu-q3));
        G4double q0primemin=0.5*Q2*(1.-xB)/(xB*(M_targ+nu+q3));

        G4double tmax=-Q2-2.*q0primemax*(nu-q3);
        G4double tmin=-Q2-2.*q0primemin*(nu+q3);

        gEv->Settmin(tmax - 2.); //Use this method to change tmin (default -2 GeV)
        gEv->Settmax(0.);  //Use this method to change tmax (default 0 GeV) 

        gEv->ComputeDVCS();//Compute the gamma*p->gamma p collision

        gEv->ApplySpecVerAcc();//Rotates all final vectors around the beam axis

        hit_HMS = false;

        if(gEv->HitsSpectro(gEv->GetScatteredElectron()) && gEv->HitsCalo(gEv->GetFinalPhoton())){ //Checks calo acceptance

            hit_HMS = true;

            L_elec_init  = gEv->GetInitialElectron();
            L_elec_scat  = gEv->GetScatteredElectron();
            L_phot_final = gEv->GetFinalPhoton(); 
            L_prot_final = gEv->GetFinalProton(); 

            if(hit_HMS){
                dvcsGlobals::hit_HMS_CALO_flag = true;

                if(target_gen_proc_type == 0){ //proton
                    G4double PSF    = gEv->GetPSF();
                    G4double X_sum  = gEv->XSecSum(0);
                    G4double X_diff = gEv->XSecDif();
                    G4double X_BH   = gEv->XSecSum(1);
                    fEventAction->DefineWeights(PSF, X_sum, X_diff, X_BH);
                }
                else if( target_gen_proc_type == 1 || target_gen_proc_type == 2 ){ //neutron or deutron
                    fEventAction->DefineWeights(gEv->GetPSF(), 0., 0., 0.);
                }
                else{
                    G4cout<<"Target types is not set correctly, "<<target_gen_proc_type<<G4endl;
                    G4cout<<"The program is exiting"<<G4endl;
                    exit(1);
                }

                fEventAction->DefinePrimaries(L_elec_init, L_elec_scat, L_phot_final, L_prot_final, gEv->GetVertex()->X(), gEv->GetVertex()->Y(), gEv->GetVertex()->Z());

                //unit change [cm] to [mm]
                fParticleGun->SetParticleDefinition(particleTable->FindParticle(11));//11 : electron
                fParticleGun->SetParticleEnergy(gEv->GetScatteredElectron()->E()*CLHEP::GeV - 0.510999*CLHEP::MeV);
                fParticleGun->SetParticlePosition(G4ThreeVector(gEv->GetVertex()->X()*CLHEP::cm, gEv->GetVertex()->Y()*CLHEP::cm, gEv->GetVertex()->Z()*CLHEP::cm));
                fParticleGun->SetParticleMomentumDirection(G4ThreeVector((gEv->GetScatteredElectron()->Vect()).Px(), gEv->GetScatteredElectron()->Vect().Py(), gEv->GetScatteredElectron()->Vect().Pz()));

                fParticleGun->GeneratePrimaryVertex(anEvent);

                fParticleGun->SetParticleDefinition(particleTable->FindParticle(22));//22 : photon
                fParticleGun->SetParticleEnergy(gEv->GetFinalPhoton()->E()*CLHEP::GeV);
                fParticleGun->SetParticlePosition(G4ThreeVector(gEv->GetVertex()->X()*CLHEP::cm, gEv->GetVertex()->Y()*CLHEP::cm, gEv->GetVertex()->Z()*CLHEP::cm));
                fParticleGun->SetParticleMomentumDirection(G4ThreeVector((gEv->GetFinalPhoton()->Vect()).Px(), gEv->GetFinalPhoton()->Vect().Py(), gEv->GetFinalPhoton()->Vect().Pz()));

                fParticleGun->GeneratePrimaryVertex(anEvent);

            }
        }
    }
  gEv->Clear();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

