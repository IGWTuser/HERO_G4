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

    // УБРАЛИ сохранение начальной энергии в fSecondaryNeutronsMap
    // Теперь записываем только для задержек из массива
    
    // Проверяем каждый порог времени
    for (size_t i = 0; i < fThresholds.size(); ++i) {
        // Если глобальное время >= порога, записываем нейтрон
        if (globalTime >= fThresholds[i]) {
            // Если этот нейтрон ещё не записан в эту карту
            if (fDelayedNeutronsMaps[i].find(id) == fDelayedNeutronsMaps[i].end()) {
                fDelayedNeutronsMaps[i][id] = kinE;
            }
        }
    }
}

void MySteppingAction::Reset() {
    // Очищаем только карты задержек (начальную убрали)
    for (auto& m : fDelayedNeutronsMaps) {
        m.clear();
    }
}

const std::map<G4int, G4double>& MySteppingAction::GetSecondaryNeutrons() const {
    // Эта функция больше не используется, но оставляем для совместимости
    // Возвращаем пустую карту
    static std::map<G4int, G4double> emptyMap;
    return emptyMap;
}

const std::vector<std::map<G4int, G4double>>& MySteppingAction::GetDelayedNeutronsMaps() const {
    return fDelayedNeutronsMaps;
}

const std::vector<G4double>& MySteppingAction::GetThresholds() const {
    return fThresholds;
}