#include "TargetForceCollisionOperator.hh"

#include "G4BOptrForceCollision.hh"
#include "G4BiasingProcessInterface.hh"
#include "G4ParticleDefinition.hh"
#include "G4ParticleTable.hh"

TargetForceCollisionOperator::TargetForceCollisionOperator(const G4String& name)
    : G4VBiasingOperator(name), fCurrentOperator(0) {}

TargetForceCollisionOperator::~TargetForceCollisionOperator() {
  for (auto& entry : fOperators) delete entry.second;
}

void TargetForceCollisionOperator::AddParticle(const G4String& particleName) {
  const G4ParticleDefinition* particle =
      G4ParticleTable::GetParticleTable()->FindParticle(particleName);
  if (!particle) {
    G4ExceptionDescription message;
    message << "Particle " << particleName << " was not found";
    G4Exception("TargetForceCollisionOperator::AddParticle",
                "GEPELECTRONUCLEAR002", FatalException, message);
  }
  fOperators[particle] = new G4BOptrForceCollision(
      particleName, GetName() + "For" + particleName);
}

void TargetForceCollisionOperator::StartTracking(const G4Track* track) {
  // Force only the incident beam electron. Applying force collision to
  // nuclear-interaction daughters recursively creates an exponential track
  // population. Photonuclear production is handled in a separate weighted
  // photon-source calculation; every daughter here is transported analogously.
  const G4int pdg = track->GetParticleDefinition()->GetPDGEncoding();
  const G4bool primaryElectron = pdg == 11 && track->GetParentID() == 0;
  if (!primaryElectron) {
    fCurrentOperator = 0;
    return;
  }
  const auto found = fOperators.find(track->GetParticleDefinition());
  fCurrentOperator = found == fOperators.end() ? 0 : found->second;
}

G4VBiasingOperation*
TargetForceCollisionOperator::ProposeOccurenceBiasingOperation(
    const G4Track* track, const G4BiasingProcessInterface* process) {
  return fCurrentOperator
             ? fCurrentOperator->GetProposedOccurenceBiasingOperation(track, process)
             : 0;
}

G4VBiasingOperation*
TargetForceCollisionOperator::ProposeNonPhysicsBiasingOperation(
    const G4Track* track, const G4BiasingProcessInterface* process) {
  return fCurrentOperator
             ? fCurrentOperator->GetProposedNonPhysicsBiasingOperation(track, process)
             : 0;
}

G4VBiasingOperation*
TargetForceCollisionOperator::ProposeFinalStateBiasingOperation(
    const G4Track* track, const G4BiasingProcessInterface* process) {
  return fCurrentOperator
             ? fCurrentOperator->GetProposedFinalStateBiasingOperation(track, process)
             : 0;
}

void TargetForceCollisionOperator::OperationApplied(
    const G4BiasingProcessInterface* process, G4BiasingAppliedCase biasCase,
    G4VBiasingOperation* operation, const G4VParticleChange* change) {
  if (fCurrentOperator)
    fCurrentOperator->ReportOperationApplied(process, biasCase, operation, change);
}

void TargetForceCollisionOperator::OperationApplied(
    const G4BiasingProcessInterface* process, G4BiasingAppliedCase biasCase,
    G4VBiasingOperation* occurrence, G4double weight,
    G4VBiasingOperation* finalState, const G4VParticleChange* change) {
  if (fCurrentOperator)
    fCurrentOperator->ReportOperationApplied(process, biasCase, occurrence, weight,
                                              finalState, change);
}

void TargetForceCollisionOperator::ExitBiasing(
    const G4Track* track, const G4BiasingProcessInterface* process) {
  if (fCurrentOperator) fCurrentOperator->ExitingBiasing(track, process);
}
