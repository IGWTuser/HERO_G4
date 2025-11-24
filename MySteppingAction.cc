#include "MySteppingAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4Neutron.hh"
#include "G4SystemOfUnits.hh"

MySteppingAction::MySteppingAction(const std::vector<G4double>& thresholds)
 : fThresholds(thresholds)
{
    // создаём пустые карты нужного размера
    fDelayedNeutronsMaps.resize(fThresholds.size());
}

MySteppingAction::~MySteppingAction() { }

void MySteppingAction::UserSteppingAction(const G4Step* step) {
    G4Track* track = step->GetTrack();
    if (track->GetDefinition()->GetPDGEncoding() != 2112) return;
    auto volume = step->GetPostStepPoint()->GetPhysicalVolume();
    if (!volume || volume->GetName() != "Detector") return;

    G4int id = track->GetTrackID();
    G4double kinE       = step->GetPostStepPoint()->GetKineticEnergy();
    G4double globalTime = step->GetPostStepPoint()->GetGlobalTime();

    // сохраняем начальную энергию
    if (fSecondaryNeutronsMap.find(id) == fSecondaryNeutronsMap.end()) {
        fSecondaryNeutronsMap[id] = kinE;
    }

    // проверяем каждый порог
    for (size_t i = 0; i < fThresholds.size(); ++i) {
        if (globalTime >= fThresholds[i]
            && fDelayedNeutronsMaps[i].find(id) == fDelayedNeutronsMaps[i].end())
        {
            fDelayedNeutronsMaps[i][id] = kinE;
        }
    }

    // при желании можно убить трек после максимального порога
    if (globalTime >= fThresholds.back()) {
        track->SetTrackStatus(fStopAndKill);
    }
}

void MySteppingAction::Reset() {
    fSecondaryNeutronsMap.clear();
    for (auto& m : fDelayedNeutronsMaps) m.clear();
}

const std::map<G4int, G4double>&
MySteppingAction::GetSecondaryNeutrons() const {
    return fSecondaryNeutronsMap;
}

const std::vector<std::map<G4int, G4double>>&
MySteppingAction::GetDelayedNeutronsMaps() const {
    return fDelayedNeutronsMaps;
}

const std::vector<G4double>&
MySteppingAction::GetThresholds() const {
    return fThresholds;
}
