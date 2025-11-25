#ifndef MyRunAction_h
#define MyRunAction_h 1

#include "G4UserRunAction.hh"
#include "MyEventAction.hh"
#include "globals.hh"

class CSVWriter;

class MyRunAction : public G4UserRunAction {
public:
    MyRunAction(MyEventAction* eventAction, CSVWriter* csvWriter, const G4String& csvFilename);
    virtual ~MyRunAction();

    virtual void BeginOfRunAction(const G4Run* run) override;
    virtual void EndOfRunAction(const G4Run* run) override;
    
    void SetCurrentParticle(const G4String& name) { fCurrentParticleName = name; }
    void SetCurrentEnergy(G4double energy) { fCurrentEnergy = energy; }

private:
    MyEventAction* fEventAction;
    CSVWriter*     fCSVWriter;
    G4String       fCurrentParticleName;
    G4double       fCurrentEnergy;
    G4int          fRunNumber;
    
    static G4int   sGlobalRunNumber;
};

#endif