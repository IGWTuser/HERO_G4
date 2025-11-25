#include "MyRunAction.hh"
#include "G4Run.hh"
#include "G4SystemOfUnits.hh"

int MyRunAction::sGlobalEventNumber = 0;

MyRunAction::MyRunAction(MyEventAction* eventAction)
    : G4UserRunAction(),
      fEventAction(eventAction),
      fCurrentParticleName("unknown"),
      fCurrentEnergy(0.0)
{
}

MyRunAction::~MyRunAction() {
}

void MyRunAction::BeginOfRunAction(const G4Run* run) {
    G4cout << "\n========================================" << G4endl;
    G4cout << "Run started" << G4endl;
    G4cout << "Particle: " << fCurrentParticleName << G4endl;
    G4cout << "Energy: " << fCurrentEnergy / TeV << " TeV" << G4endl;
    G4cout << "Global event number starts at: " << sGlobalEventNumber << G4endl;
    G4cout << "========================================\n" << G4endl;
    
    // Передаём текущие параметры в EventAction
    if (fEventAction) {
        fEventAction->SetCurrentParticle(fCurrentParticleName);
        fEventAction->SetCurrentEnergy(fCurrentEnergy);
        fEventAction->SetGlobalEventNumber(sGlobalEventNumber);
    }
}

void MyRunAction::EndOfRunAction(const G4Run* run) {
    int numEvents = run->GetNumberOfEvent();
    sGlobalEventNumber += numEvents;
    
    G4cout << "Run ended. Processed " << numEvents << " events." << G4endl;
    G4cout << "Next global event number: " << sGlobalEventNumber << G4endl;
}
