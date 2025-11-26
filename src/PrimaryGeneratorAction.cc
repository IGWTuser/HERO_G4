#include "PrimaryGeneratorAction.hh"

#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4Proton.hh"
#include "G4SystemOfUnits.hh"
#include "G4Event.hh"
#include "G4ThreeVector.hh"
#include "Randomize.hh"

PrimaryGeneratorAction::PrimaryGeneratorAction() : G4VUserPrimaryGeneratorAction() {
    // Создаём пушку, генерирующую 1 частицу за событие
    fParticleGun = new G4ParticleGun(1);

    // По умолчанию стреляем протонами
    G4ParticleDefinition* particle = G4ParticleTable::GetParticleTable()->FindParticle("proton");
    fParticleGun->SetParticleDefinition(particle);
    
    // Позиция пушки: 2 метра перед детектором по оси Z
    fParticleGun->SetParticlePosition(G4ThreeVector(0, 0, -2*m));
    
    // Направление: вдоль оси Z (прямо на детектор)
    fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    
    // Начальная энергия (будет изменяться в main)
    fParticleGun->SetParticleEnergy(100*GeV);
}

PrimaryGeneratorAction::~PrimaryGeneratorAction() {
    delete fParticleGun;
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event) {
    // Генерируем одну частицу с текущими настройками пушки
    fParticleGun->GeneratePrimaryVertex(event);
}
