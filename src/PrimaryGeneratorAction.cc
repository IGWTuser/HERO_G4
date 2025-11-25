#include "PrimaryGeneratorAction.hh"

#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4Proton.hh"
#include "G4SystemOfUnits.hh"
#include "G4Event.hh"
#include "G4ThreeVector.hh"
#include "Randomize.hh"

PrimaryGeneratorAction::PrimaryGeneratorAction() : G4VUserPrimaryGeneratorAction() {
    // Создаем пушку для генерации одной частицы за событие
    fParticleGun = new G4ParticleGun(1); // теперь только 1 частица за событие!

    // Получаем определение протона из таблицы частиц
    G4ParticleDefinition* particle = G4ParticleTable::GetParticleTable()->FindParticle("proton");
    fParticleGun->SetParticleDefinition(particle);
    
    // Задаем начальную позицию и направление частиц
    fParticleGun->SetParticlePosition(G4ThreeVector(0, 0, -2*m));
    fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    
    // Начальная энергия, можно задать случайным образом позже
    fParticleGun->SetParticleEnergy(100*GeV);
}

PrimaryGeneratorAction::~PrimaryGeneratorAction() {
    delete fParticleGun;
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event) {
    // Здесь можно задать случайную энергию или другие параметры:
    // Например, раскомментируй эту строку для случайной энергии в диапазоне от 1 до 100 MeV:
    // fParticleGun->SetParticleEnergy(1*MeV + G4UniformRand()*(100*MeV - 1*MeV));
    
    // Генерируем вершину события
    fParticleGun->GeneratePrimaryVertex(event);
}
