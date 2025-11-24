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

#include <vector>
#include <chrono>
#include <cstdlib>

int main(int argc, char** argv) {
    // Определяем, работаем ли в интерактивном режиме
    G4UIExecutive* ui = nullptr;
    if (argc == 1) {
        ui = new G4UIExecutive(argc, argv);
    }

    // Создаём менеджер запуска
    G4RunManager* runManager = new G4RunManager;

    // Инициализируем детектор (геометрию)
    runManager->SetUserInitialization(new DetectorConstruction());

    // Инициализируем физический список
    G4PhysListFactory factory;
    G4VModularPhysicsList* physicsList = factory.GetReferencePhysList("FTFP_BERT_HPT");
    runManager->SetUserInitialization(physicsList);

    // Настраиваем отсечку по времени для нейтронов
    auto neutronCut = new G4NeutronTrackingCut();
    neutronCut->SetTimeLimit(1.*s);
    neutronCut->SetKineticEnergyLimit(0.*eV);
    physicsList->RegisterPhysics(neutronCut);

    // Устанавливаем генератор первичных частиц
    runManager->SetUserAction(new PrimaryGeneratorAction());

    // Задаём вектор порогов (время задержки для карт)
    std::vector<G4double> delays = { 
        100*ns,
        250*ns,
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
    runManager->SetUserAction(new MyEventAction(steppingAction, "../data"));

    // Инициализируем симуляцию
    runManager->Initialize();

    // Визуализация (если нужна)
    G4VisExecutive* visManager = new G4VisExecutive;
    visManager->Initialize();

    // Запуск макросов
    G4UImanager* UImanager = G4UImanager::GetUIpointer();
    if (ui) {
        UImanager->ApplyCommand("/control/macroPath /home/vibecoding/CW/");
        UImanager->ApplyCommand("/control/execute init_vis.mac");
        ui->SessionStart();
        delete ui;
    } else {
        UImanager->ApplyCommand("/control/execute vis.mac");
    }

    // Определяем число событий для запуска (передаётся первым аргументом в batch-режиме)
    int nEvents = 0;
    if (argc > 1) {
        // в batch режиме можно передать число событий
        nEvents = std::atoi(argv[1]);
    }
    if (nEvents <= 0) {
        G4cout << "No events requested. Exiting." << G4endl;
    } else {
        // Запускаем события по одному, чтобы отчет каждые 10
        auto tStart = std::chrono::steady_clock::now();
        for (int i = 0; i < nEvents; ++i) {
            runManager->BeamOn(1);
            if ((i+1) % 10 == 0) {
                auto tNow = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(tNow - tStart).count();
                G4cout << "Processed " << (i+1)
                       << " events, elapsed: " << elapsed << " s" << G4endl;
            }
        }
    }

    // Очистка ресурсов
    delete visManager;
    delete runManager;

    return 0;
}
