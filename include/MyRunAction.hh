#ifndef MyRunAction_h
#define MyRunAction_h 1

#include "G4UserRunAction.hh"
#include "MyEventAction.hh"
#include "globals.hh"

class MyRunAction : public G4UserRunAction {
public:
    MyRunAction(MyEventAction* eventAction);
    virtual ~MyRunAction();

    virtual void BeginOfRunAction(const G4Run* run) override;
    virtual void EndOfRunAction(const G4Run* run) override;
    
    void SetCurrentParticle(const G4String& name) { fCurrentParticleName = name; }
    void SetCurrentEnergy(G4double energy) { fCurrentEnergy = energy; }
    
    static int GetGlobalEventNumber() { return sGlobalEventNumber; }

private:
    MyEventAction* fEventAction;
    G4String       fCurrentParticleName;
    G4double       fCurrentEnergy;
    
    static int     sGlobalEventNumber;
};

#endif
