#include "TargetCrossSectionBiasOperator.hh"

#include "G4BiasingProcessInterface.hh"
#include "G4BOptnChangeCrossSection.hh"
#include "G4ParticleDefinition.hh"
#include "G4ParticleTable.hh"
#include "G4ProcessManager.hh"
#include "G4VProcess.hh"

#include <cfloat>

TargetCrossSectionBiasOperator::TargetCrossSectionBiasOperator(
    const G4String& particleName, G4double enhancementFactor,
    const G4String& name)
    : G4VBiasingOperator(name),
      particle_(G4ParticleTable::GetParticleTable()->FindParticle(particleName)),
      enhancementFactor_(enhancementFactor) {
  if (!particle_ || enhancementFactor_ <= 1.) {
    G4ExceptionDescription message;
    message << "Invalid particle or cross-section enhancement factor: "
            << particleName << ", " << enhancementFactor_;
    G4Exception("TargetCrossSectionBiasOperator::TargetCrossSectionBiasOperator",
                "GEPTARGETBIAS001", FatalException, message);
  }
}

TargetCrossSectionBiasOperator::~TargetCrossSectionBiasOperator() {
  for (auto& entry : operations_) delete entry.second;
}

void TargetCrossSectionBiasOperator::StartRun() {
  if (!setup_) return;
  const auto* sharedData = G4BiasingProcessInterface::GetSharedData(
      particle_->GetProcessManager());
  if (sharedData) {
    for (const auto* wrapper : sharedData->GetPhysicsBiasingProcessInterfaces()) {
      const G4String operationName =
          "XSchange-" + wrapper->GetWrappedProcess()->GetProcessName();
      operations_[wrapper] = new G4BOptnChangeCrossSection(operationName);
    }
  }
  if (operations_.empty()) {
    G4Exception("TargetCrossSectionBiasOperator::StartRun",
                "GEPTARGETBIAS002", FatalException,
                "No wrapped physics process was found for biasing");
  }
  setup_ = false;
}

G4VBiasingOperation*
TargetCrossSectionBiasOperator::ProposeOccurenceBiasingOperation(
    const G4Track* track, const G4BiasingProcessInterface* callingProcess) {
  // Bias only the incident beam electron. Electronuclear daughters, including
  // secondary electrons, must be transported with their analog cross sections.
  if (track->GetDefinition() != particle_ || track->GetParentID() != 0) {
    return nullptr;
  }
  const G4double interactionLength =
      callingProcess->GetWrappedProcess()->GetCurrentInteractionLength();
  if (interactionLength > DBL_MAX / 10.) return nullptr;

  auto* operation = operations_.at(callingProcess);
  auto* previous = callingProcess->GetPreviousOccurenceBiasingOperation();
  const G4double biasedCrossSection = enhancementFactor_ / interactionLength;
  if (!previous || operation->GetInteractionOccured()) {
    operation->SetBiasedCrossSection(biasedCrossSection);
    operation->Sample();
  } else {
    if (previous != operation) {
      G4Exception("TargetCrossSectionBiasOperator::ProposeOccurenceBiasingOperation",
                  "GEPTARGETBIAS003", FatalException,
                  "Unexpected previous biasing operation");
    }
    operation->UpdateForStep(callingProcess->GetPreviousStepSize());
    operation->SetBiasedCrossSection(biasedCrossSection);
    operation->UpdateForStep(0.);
  }
  return operation;
}

void TargetCrossSectionBiasOperator::OperationApplied(
    const G4BiasingProcessInterface* callingProcess, G4BiasingAppliedCase,
    G4VBiasingOperation* occurrence, G4double, G4VBiasingOperation*,
    const G4VParticleChange*) {
  auto* operation = operations_.at(callingProcess);
  if (operation == occurrence) operation->SetInteractionOccured();
}
