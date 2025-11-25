#include "G4RunManager.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"

#include "G4PhysListFactory.hh"
#include "DetectorConstruction.hh"
#include "PrimaryGeneratorAction.hh"
#include "G4NeutronTrackingCut.hh"
#include "G4SystemOfUnits.hh"

#include "MySteppingAction.hh"
#include "MyEventAction.hh"
#include "MyRunAction.hh"
#include "CSVWriter.hh"

#include "G4ParticleTable.hh"
#include "G4ParticleGun.hh"        
#include "G4ParticleDefinition.hh"

#include <vector>
#include <chrono>
#include <cstdlib>

int main(int argc, char** argv) {
    // Создаём менеджер запуска
    G4RunManager* runManager = new G4RunManager;

    // Инициализируем детектор (геометрию)
    runManager->SetUserInitialization(new DetectorConstruction());

    // Инициализируем физический список
    G4PhysListFactory factory;
    G4VModularPhysicsList* physicsList = factory.GetReferencePhysList("FTFP_BERT_HP");
    runManager->SetUserInitialization(physicsList);

    // Настраиваем отсечку по времени для нейтронов
    auto neutronCut = new G4NeutronTrackingCut();
    neutronCut->SetTimeLimit(1.*s);
    neutronCut->SetKineticEnergyLimit(0.*eV);
    physicsList->RegisterPhysics(neutronCut);

    // Устанавливаем генератор первичных частиц
    PrimaryGeneratorAction* primaryGen = new PrimaryGeneratorAction();
    runManager->SetUserAction(primaryGen);

    // Задаём вектор порогов (время задержки для карт)
    std::vector<G4double> delays = { 
        100*ns,   // 0.1 мкс
        250*ns,   // 0.25 мкс
        750*ns,
        1*us, 
        2*us, 
        4*us, 
        8*us, 
        10*us,
        20*us,
        35*us,
        50*us,
        100*us,
        125*us,
        150*us,
        200*us
    };
    
    MySteppingAction* steppingAction = new MySteppingAction(delays);
    runManager->SetUserAction(steppingAction);
    
    MyEventAction* eventAction = new MyEventAction(steppingAction, "../data");
    runManager->SetUserAction(eventAction);
    
    // Создаём CSV файл
    G4String csvFilename = "../data/simulation_results.csv";
    CSVWriter* csvWriter = new CSVWriter(csvFilename, true);  // true = append mode
    csvWriter->WriteHeader(delays);
    
    MyRunAction* runAction = new MyRunAction(eventAction, csvWriter, csvFilename);
    runManager->SetUserAction(runAction);
    
    // Инициализируем симуляцию
    runManager->Initialize();

    // Определяем параметры для симуляции
    std::vector<G4String> particles = {"proton", "e-", "neutron", "gamma"};
    std::vector<G4double> energies = {0.1*TeV, 0.5*GeV, 1.0*GeV, 5.0*GeV, 10.0*GeV};
    
   int nEventsPerRun = 100;  // Количество событий на каждый ран
if (argc > 1) {
    // Проверяем, является ли первый аргумент числом
    std::string firstArg = argv[1];
    if (firstArg != "--reset") {
        nEventsPerRun = std::atoi(argv[1]);  // <-- ИСПРАВЛЕНО: argv[1] вместо argv[^6_1]
    }
}
    
    G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
    
    // Цикл по частицам и энергиям
    auto tStart = std::chrono::steady_clock::now();
    
    for (const auto& particleName : particles) {
        G4ParticleDefinition* particle = particleTable->FindParticle(particleName);
        if (!particle) {
            G4cerr << "Particle " << particleName << " not found!" << G4endl;
            continue;
        }
        
        for (const auto& energy : energies) {
            // Настраиваем пушку
            G4ParticleGun* gun = primaryGen->GetParticleGun();
            gun->SetParticleDefinition(particle);
            gun->SetParticleEnergy(energy);
            
            // Устанавливаем текущие параметры для RunAction
            runAction->SetCurrentParticle(particleName);
            runAction->SetCurrentEnergy(energy);
            
            G4cout << "\n========================================" << G4endl;
            G4cout << "Starting simulation:" << G4endl;
            G4cout << "  Particle: " << particleName << G4endl;
            G4cout << "  Energy: " << energy/TeV << " TeV" << G4endl;
            G4cout << "  Events: " << nEventsPerRun << G4endl;
            G4cout << "========================================\n" << G4endl;
            
            // Запускаем симуляцию
            runManager->BeamOn(nEventsPerRun);
            
            auto tNow = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(tNow - tStart).count();
            G4cout << "Total elapsed time: " << elapsed << " s\n" << G4endl;
        }
    }
    
    // Очистка ресурсов
    delete csvWriter;
    delete runManager;

    G4cout << "\n Simulation completed! Check ../data/simulation_results.csv" << G4endl;

    return 0;
}