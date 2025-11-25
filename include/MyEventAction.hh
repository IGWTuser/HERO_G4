#ifndef MyEventAction_h
#define MyEventAction_h 1

#include "G4UserEventAction.hh"
#include "MySteppingAction.hh"
#include "G4String.hh"
#include <vector>
#include <utility>

class MyEventAction : public G4UserEventAction {
public:
    MyEventAction(MySteppingAction* steppingAction,
                  const G4String& dataDir = "../data");
    virtual ~MyEventAction();

    virtual void BeginOfEventAction(const G4Event* event) override;
    virtual void EndOfEventAction(const G4Event* event) override;

    void SaveSummaryData();
    
    // Новые методы для подсчёта нейтронов < 1 эВ
    std::vector<int> CountLowEnergyNeutrons(G4double energyThreshold = 1.0 * eV) const;
    void ResetRunStatistics();
    
    // Геттеры для статистики по всему рану
    const std::vector<int>& GetTotalNeutronCounts() const { return fTotalNeutronCounts; }

private:
    G4String BuildBaseNameFromPrimary(const G4Event* event) const;
    static G4String FormatEnergy(G4double e);
    static G4String FormatDelay(G4double t);
    static G4String Sanitize(const G4String& s);

private:
    MySteppingAction* fSteppingAction;
    G4String          fDataDirectory;
    
    // Статистика для CSV (накапливается за все события рана)
    std::vector<int> fTotalNeutronCounts;
};

#endif