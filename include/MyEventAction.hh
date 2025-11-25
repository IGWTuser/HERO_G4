#ifndef MyEventAction_h
#define MyEventAction_h 1

#include "G4UserEventAction.hh"
#include "MySteppingAction.hh"
#include "G4String.hh"
#include "G4SystemOfUnits.hh"
#include <vector>
#include <utility>

class CSVWriter;

class MyEventAction : public G4UserEventAction {
public:
    MyEventAction(MySteppingAction* steppingAction,
                  const G4String& dataDir = "../data");
    virtual ~MyEventAction();

    virtual void BeginOfEventAction(const G4Event* event) override;
    virtual void EndOfEventAction(const G4Event* event) override;

    void SaveSummaryData();
    
    // Методы для CSV
    void SetCSVWriter(CSVWriter* writer) { fCSVWriter = writer; }
    void SetCurrentParticle(const G4String& name) { fCurrentParticleName = name; }
    void SetCurrentEnergy(G4double energy) { fCurrentEnergy = energy; }
    void SetGlobalEventNumber(int num) { fGlobalEventNumber = num; }
    
    std::vector<int> CountLowEnergyNeutrons(G4double energyThreshold = 1.0 * eV) const;

private:
    G4String BuildBaseNameFromPrimary(const G4Event* event) const;
    static G4String FormatEnergy(G4double e);
    static G4String FormatDelay(G4double t);
    static G4String Sanitize(const G4String& s);

private:
    MySteppingAction* fSteppingAction;
    G4String          fDataDirectory;
    
    // Для CSV записи
    CSVWriter*        fCSVWriter;
    G4String          fCurrentParticleName;
    G4double          fCurrentEnergy;
    int               fGlobalEventNumber;
};

#endif
