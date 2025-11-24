#ifndef MySteppingAction_h
#define MySteppingAction_h 1

#include "G4UserSteppingAction.hh"
#include "globals.hh"
#include <map>
#include <vector>

class MySteppingAction : public G4UserSteppingAction {
public:
    MySteppingAction(const std::vector<G4double>& thresholds);
    virtual ~MySteppingAction();

    virtual void UserSteppingAction(const G4Step* step);
    void Reset();

    // Получение карты начальных энергий
    const std::map<G4int, G4double>& GetSecondaryNeutrons() const;

    // Получение всех задержанных карт
    const std::vector<std::map<G4int, G4double>>& GetDelayedNeutronsMaps() const;
    const std::vector<G4double>&               GetThresholds()       const;

private:
    std::map<G4int, G4double> fSecondaryNeutronsMap;

    // По одному map на каждый порог
    std::vector<G4double>                        fThresholds;
    std::vector<std::map<G4int, G4double>>       fDelayedNeutronsMaps;
};

#endif // MySteppingAction_h
