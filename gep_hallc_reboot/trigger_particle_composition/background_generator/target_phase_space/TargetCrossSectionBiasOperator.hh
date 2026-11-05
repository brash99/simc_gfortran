#ifndef GEP_TARGET_CROSS_SECTION_BIAS_OPERATOR_HH
#define GEP_TARGET_CROSS_SECTION_BIAS_OPERATOR_HH

// Adapted from Geant4 11.2.1 extended/biasing/GB01.

#include "G4VBiasingOperator.hh"

#include <map>

class G4BOptnChangeCrossSection;
class G4ParticleDefinition;

class TargetCrossSectionBiasOperator final : public G4VBiasingOperator {
 public:
  TargetCrossSectionBiasOperator(const G4String& particleName,
                                 G4double enhancementFactor,
                                 const G4String& name);
  ~TargetCrossSectionBiasOperator() override;

  void StartRun() override;

 private:
  G4VBiasingOperation* ProposeOccurenceBiasingOperation(
      const G4Track*, const G4BiasingProcessInterface*) override;
  G4VBiasingOperation* ProposeFinalStateBiasingOperation(
      const G4Track*, const G4BiasingProcessInterface*) override {
    return nullptr;
  }
  G4VBiasingOperation* ProposeNonPhysicsBiasingOperation(
      const G4Track*, const G4BiasingProcessInterface*) override {
    return nullptr;
  }

  using G4VBiasingOperator::OperationApplied;
  void OperationApplied(const G4BiasingProcessInterface*, G4BiasingAppliedCase,
                        G4VBiasingOperation*, G4double, G4VBiasingOperation*,
                        const G4VParticleChange*) override;

  std::map<const G4BiasingProcessInterface*, G4BOptnChangeCrossSection*>
      operations_;
  G4bool setup_ = true;
  const G4ParticleDefinition* particle_ = nullptr;
  G4double enhancementFactor_ = 1.;
};

#endif
