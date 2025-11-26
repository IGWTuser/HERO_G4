#ifndef PrimaryGeneratorAction_h
#define PrimaryGeneratorAction_h 1

#include "G4VUserPrimaryGeneratorAction.hh"

class G4ParticleGun;

// Класс для генерации первичных частиц (пушка)
// Создаёт по одной частице за событие с заданными параметрами
class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
public:
    PrimaryGeneratorAction();
    virtual ~PrimaryGeneratorAction();

    // Вызывается для каждого события — генерирует первичную частицу
    virtual void GeneratePrimaries(G4Event* event);
    
    // Геттер для доступа к пушке (нужен для изменения частицы/энергии в main)
    G4ParticleGun* GetParticleGun() const { return fParticleGun; }
    
private:
    G4ParticleGun* fParticleGun;
};

#endif
