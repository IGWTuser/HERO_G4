#include "G4MTRunManager.hh"  // многопоточный менеджер
#include "G4UImanager.hh"

#include "G4PhysListFactory.hh"
#include "DetectorConstruction.hh"
#include "ActionInitialization.hh"
#include "G4NeutronTrackingCut.hh"
#include "G4SystemOfUnits.hh"

#include "CSVWriter.hh"

#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"

#include <vector>
#include <chrono>
#include <cstdlib>
#include <string>

int main(int argc, char** argv) {
    // Разбираем аргументы: ./HERO_G4 [потоки] [события] [частица] [энергия_TeV]
    
    if (argc < 5) {
        G4cout << "Usage: ./HERO_G4 <threads> <events> <particle> <energy_TeV>" << G4endl;
        G4cout << "Example: ./HERO_G4 8 100 proton 0.1" << G4endl;
        G4cout << "Example: ./HERO_G4 8 100 proton 0.25" << G4endl;
        return 1;
    }
    
    // Парсим аргументы
    int nThreads = std::atoi(argv[1]);
    int nEventsPerRun = std::atoi(argv[2]);
    G4String particleName = argv[3];
    G4double energyTeV = std::atof(argv[4]);
    G4double energy = energyTeV * TeV;
    
    G4cout << "========================================" << G4endl;
    G4cout << "Configuration:" << G4endl;
    G4cout << "  Threads: " << nThreads << G4endl;
    G4cout << "  Events: " << nEventsPerRun << G4endl;
    G4cout << "  Particle: " << particleName << G4endl;
    G4cout << "  Energy: " << energyTeV << " TeV" << G4endl;
    G4cout << "========================================\n" << G4endl;
    
    // Временные задержки для подсчёта нейтронов
    std::vector<G4double> delays = { 
        100*ns, 250*ns, 750*ns,
        1*us, 2*us, 4*us, 8*us, 10*us,
        20*us, 35*us, 50*us, 100*us,
        125*us, 150*us, 200*us
    };
    
    // CSV writer (режим append - добавляем к существующему файлу)
    G4String csvFilename = "../data/simulation_results.csv";
    CSVWriter* csvWriter = new CSVWriter(csvFilename, true);  // true = append mode
    csvWriter->WriteHeader(delays);
    
    // Создаём многопоточный менеджер запуска
    G4MTRunManager* runManager = new G4MTRunManager;
    runManager->SetNumberOfThreads(nThreads);
    G4cout << "Running with " << nThreads << " threads" << G4endl;

    // Инициализируем геометрию детектора
    runManager->SetUserInitialization(new DetectorConstruction());

    // Инициализируем физический список
    G4PhysListFactory factory;
    G4VModularPhysicsList* physicsList = factory.GetReferencePhysList("FTFP_BERT_HP");
    runManager->SetUserInitialization(physicsList);

    // Настраиваем отсечку времени для нейтронов
    auto neutronCut = new G4NeutronTrackingCut();
    neutronCut->SetTimeLimit(1.*s);
    neutronCut->SetKineticEnergyLimit(0.*eV);
    physicsList->RegisterPhysics(neutronCut);
    
    // Создаём ActionInitialization с параметрами из командной строки
    ActionInitialization* actionInit = 
        new ActionInitialization(delays, csvWriter, particleName, energy);
    runManager->SetUserInitialization(actionInit);
    
    // Инициализируем RunManager
    runManager->Initialize();
    
    // Засекаем время
    auto tStart = std::chrono::steady_clock::now();
    
    G4cout << "Starting simulation...\n" << G4endl;
    
    // Запускаем симуляцию с многопоточностью
    runManager->BeamOn(nEventsPerRun);
    
    // Показываем время выполнения
    auto tNow = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(tNow - tStart).count();
    
    G4cout << "\n========================================" << G4endl;
    G4cout << "Simulation completed in " << elapsed << " s" << G4endl;
    G4cout << "Results saved to: " << csvFilename << G4endl;
    G4cout << "========================================\n" << G4endl;
    
    // Освобождаем память
    delete runManager;
    delete csvWriter;

    return 0;
}
