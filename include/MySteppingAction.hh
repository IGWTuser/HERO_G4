#ifndef MySteppingAction_h
#define MySteppingAction_h 1

#include "G4UserSteppingAction.hh"
#include "globals.hh"
#include <map>
#include <vector>

// Класс для отслеживания нейтронов на каждом шаге симуляции
// Записывает нейтроны в детекторе для разных временных задержек
class MySteppingAction : public G4UserSteppingAction {
public:
    MySteppingAction(const std::vector<G4double>& thresholds);
    virtual ~MySteppingAction();

    // Вызывается на каждом шаге каждой частицы
    virtual void UserSteppingAction(const G4Step* step);
    
    // Очищает карты перед новым событием
    void Reset();

    // Получение карты начальных энергий (не используется для CSV)
    const std::map<G4int, G4double>& GetSecondaryNeutrons() const;

    // Получение всех карт нейтронов для разных задержек
    const std::vector<std::map<G4int, G4double>>& GetDelayedNeutronsMaps() const;
    const std::vector<G4double>& GetThresholds() const;

private:
    // Карта начальных энергий нейтронов (не используется после изменений)
    std::map<G4int, G4double> fSecondaryNeutronsMap;

    // Временные пороги для подсчёта (например, 0.1 мкс, 0.25 мкс, и т.д.)
    std::vector<G4double> fThresholds;
    
    // По одной карте (TrackID → энергия) для каждого порога
    std::vector<std::map<G4int, G4double>> fDelayedNeutronsMaps;
};

#endif
