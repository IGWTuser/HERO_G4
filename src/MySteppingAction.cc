#include "MySteppingAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4Neutron.hh"
#include "G4SystemOfUnits.hh"

MySteppingAction::MySteppingAction(const std::vector<G4double>& thresholds)
: fThresholds(thresholds)
{
    // Создаём пустые карты для каждого временного порога
    fDelayedNeutronsMaps.resize(fThresholds.size());
}

MySteppingAction::~MySteppingAction() { }

void MySteppingAction::UserSteppingAction(const G4Step* step) {
    G4Track* track = step->GetTrack();
    
    // Интересуют только нейтроны (PDG код 2112)
    if (track->GetDefinition()->GetPDGEncoding() != 2112) return;
    
    // Только те, что находятся в детекторе
    auto volume = step->GetPostStepPoint()->GetPhysicalVolume();
    if (!volume || volume->GetName() != "Detector") return;

    G4int id = track->GetTrackID();
    G4double kinE       = step->GetPostStepPoint()->GetKineticEnergy();
    G4double globalTime = step->GetPostStepPoint()->GetGlobalTime();

    // Начальное распределение больше не записываем (было убрано для упрощения)
    
    // Проверяем каждый временной порог
    for (size_t i = 0; i < fThresholds.size(); ++i) {
        // Если время >= порога, записываем нейтрон в соответствующую карту
        if (globalTime >= fThresholds[i]) {
            // Записываем только если этот нейтрон ещё не был записан
            if (fDelayedNeutronsMaps[i].find(id) == fDelayedNeutronsMaps[i].end()) {
                fDelayedNeutronsMaps[i][id] = kinE;
            }
        }
    }
}

void MySteppingAction::Reset() {
    // Очищаем все карты перед новым событием
    for (auto& m : fDelayedNeutronsMaps) {
        m.clear();
    }
}

const std::map<G4int, G4double>& MySteppingAction::GetSecondaryNeutrons() const {
    // Возвращаем пустую карту (начальное распределение больше не используется)
    static std::map<G4int, G4double> emptyMap;
    return emptyMap;
}

const std::vector<std::map<G4int, G4double>>& MySteppingAction::GetDelayedNeutronsMaps() const {
    return fDelayedNeutronsMaps;
}

const std::vector<G4double>& MySteppingAction::GetThresholds() const {
    return fThresholds;
}
