/**
 * @file MyRunAction.cc
 * @brief Реализация обработчика серии событий (Run)
 *
 * Выполняется в начале и конце каждого Run (серии событий).
 * Используется для инициализации параметров симуляции и статистики.
 *
 * **Основные функции:**
 * - BeginOfRunAction() - инициализация перед Run (печать параметров)
 * - EndOfRunAction() - финализация после Run (подсчёт событий)
 * - SetCurrentParticle() - установить тип первичной частицы
 * - SetCurrentEnergy() - установить энергию первичной частицы
 * - Глобальный счётчик событий (sGlobalEventNumber)
 *
 * **Иерархия в GEANT4:**
 * ```
 * RunManager::BeamOn(N events)
 *     ↓
 * MyRunAction::BeginOfRunAction()
 *     ↓ Инициализация параметров
 * [For each of N events]
 *     ↓
 * MyEventAction::BeginOfEventAction()
 * MyEventAction::EndOfEventAction()
 *     ↓
 * MyRunAction::EndOfRunAction()
 *     ↓ Статистика по Run
 * ```
 *
 * **Глобальный счётчик событий:**
 * ```
 * Run 1: sGlobalEventNumber = 0
 *     ├─ Event 0-999 (local)
 *     └─ sGlobalEventNumber = 1000 (after run)
 *
 * Run 2: sGlobalEventNumber = 1000
 *     ├─ Event 1000-1999 (global)
 *     └─ sGlobalEventNumber = 2000 (after run)
 * ```
 *
 * @date 2024
 * @version 1.0
 *
 * @note Используется для передачи параметров в EventAction
 * @note Статический счётчик сохраняется между Run-ами
 * @see G4UserRunAction, MyEventAction, MySteppingAction
 */

#include "MyRunAction.hh"
#include "G4Run.hh"
#include "G4SystemOfUnits.hh"

/**
 * @brief Глобальный статический счётчик событий
 *
 * Счётчик, который сохраняется между разными Run-ами.
 * Позволяет нумеровать события глобально для всей симуляции,
 * а не локально в каждом Run.
 *
 * **Поведение:**
 * ```
 * Инициализация:  sGlobalEventNumber = 0
 * После Run 1:    sGlobalEventNumber = 1000 (обработано 1000 событий)
 * После Run 2:    sGlobalEventNumber = 2000
 * После Run N:    sGlobalEventNumber = N * 1000
 * ```
 *
 * **Использование в CSV:**
 * ```
 * Run 1 (1000 событий):
 *     Event 0-999 в CSV
 *
 * Run 2 (1000 событий):
 *     Event 1000-1999 в CSV (продолжение нумерации!)
 * ```
 *
 * **Thread-Safety:**
 * Статический счётчик разделяется между потоками.
 * В многопоточном режиме (G4MTRunManager) это может привести к проблемам.
 * Решение: использовать G4Mutex или G4AutoLock для защиты.
 *
 * @note Инициализируется один раз в начале программы
 * @note Сохраняет значение между Run-ами (NOT сбрасывается)
 * @note В APPEND режиме CSV это обеспечивает непрерывную нумерацию
 *
 * @see MyEventAction::GetNextEventNumber() для использования
 * @see CSVWriter::GetLastRunNumber() для определения последнего номера
 */
int MyRunAction::sGlobalEventNumber = 0;

/**
 * @brief Конструктор с параметрами
 *
 * Инициализирует MyRunAction и связывает его с EventAction.
 *
 * @param eventAction Указатель на MyEventAction для передачи параметров
 *                    Содержит информацию о текущем событии
 *                    @warning НЕ ДОЛЖЕН быть nullptr
 *
 * **Инициализируемые члены:**
 * - fEventAction - сохраняется для передачи данных
 * - fCurrentParticleName - "unknown" (устанавливается позже)
 * - fCurrentEnergy - 0.0 (устанавливается позже)
 *
 * **Использование в main:**
 * ```cpp
 * MyEventAction* eventAction = new MyEventAction(stepping, "../data");
 * MyRunAction* runAction = new MyRunAction(eventAction);
 * actionInit->SetUserAction(runAction);
 * ```
 *
 * @example
 * ```cpp
 * MyRunAction* run = new MyRunAction(eventAction);
 * run->SetCurrentParticle("proton");
 * run->SetCurrentEnergy(10.0 * GeV);
 * ```
 *
 * @see SetCurrentParticle(), SetCurrentEnergy()
 * @see BeginOfRunAction() где используются эти параметры
 */
MyRunAction::MyRunAction(MyEventAction* eventAction)
    : G4UserRunAction(),
      fEventAction(eventAction),
      fCurrentParticleName("unknown"),
      fCurrentEnergy(0.0)
{
}

/**
 * @brief Деструктор
 *
 * Освобождает ресурсы при удалении объекта.
 * Обычно пуст, так как GEANT4 управляет памятью.
 *
 * @note Вызывается при завершении симуляции (обычно автоматически)
 */
MyRunAction::~MyRunAction() {
}

/**
 * @brief Инициализация Run-а (серии событий)
 *
 * Вызывается в начале каждого Run, ДО первого события.
 * Используется для печати параметров симуляции и инициализации.
 *
 * **Основные операции:**
 * 1. Напечатать разделитель и стартовое сообщение
 * 2. Напечатать параметры текущего Run:
 *    - Тип первичной частицы (fCurrentParticleName)
 *    - Энергия частицы (fCurrentEnergy в ТеВ)
 *    - Начальный номер события (sGlobalEventNumber)
 * 3. Передать параметры в EventAction
 * 4. Напечатать завершающий разделитель
 *
 * **Вывод (пример):**
 * ```
 * ========================================
 * Run started
 * Particle: proton
 * Energy: 10.000 TeV
 * Global event number starts at: 0
 * ========================================
 * ```
 *
 * **Передача параметров в EventAction:**
 * ```cpp
 * if (fEventAction) {
 *     fEventAction->SetCurrentParticle(fCurrentParticleName);
 *     fEventAction->SetCurrentEnergy(fCurrentEnergy);
 *     fEventAction->SetGlobalEventNumber(sGlobalEventNumber);
 * }
 * ```
 *
 * Это КРИТИЧНО! Без этого EventAction не будет знать,
 * какую частицу и с какой энергией симулировать.
 *
 * @param run Указатель на G4Run (содержит информацию о Run)
 *            Пример: run->GetRunID(), run->GetNumberOfEvent()
 *
 * @note Вызывается один раз в начале каждого Run
 * @note **ВАЖНО**: здесь передаются параметры в EventAction!
 * @note Может быть несколько Run-ов в одной программе
 *
 * **Многоэвентный сценарий:**
 * ```cpp
 * // main.cc
 * runManager->SetUserAction(new MyRunAction(eventAction));
 *
 * runManager->BeamOn(1000);  // Run 1: 1000 событий
 * runAction->SetCurrentParticle("neutron");
 * runManager->BeamOn(500);   // Run 2: 500 событий
 * ```
 *
 * @see EndOfRunAction() - финализация Run
 * @see SetCurrentParticle() для установки типа частицы
 * @see SetCurrentEnergy() для установки энергии
 */
void MyRunAction::BeginOfRunAction(const G4Run* run) {
    // Печатаем стартовый баннер
    G4cout << "\n========================================" << G4endl;
    G4cout << "Run started" << G4endl;
    G4cout << "Particle: " << fCurrentParticleName << G4endl;
    G4cout << "Energy: " << fCurrentEnergy / TeV << " TeV" << G4endl;
    G4cout << "Global event number starts at: " << sGlobalEventNumber << G4endl;
    G4cout << "========================================\n" << G4endl;
    
    // КРИТИЧНО: передаём текущие параметры в EventAction
    // Без этого EventAction не будет знать, какую частицу симулировать
    if (fEventAction) {
        fEventAction->SetCurrentParticle(fCurrentParticleName);
        fEventAction->SetCurrentEnergy(fCurrentEnergy);
        fEventAction->SetGlobalEventNumber(sGlobalEventNumber);
    }
}

/**
 * @brief Финализация Run-а
 *
 * Вызывается в конце каждого Run, после завершения всех событий.
 * Используется для подсчёта обработанных событий и обновления глобального счётчика.
 *
 * **Основные операции:**
 * 1. Получить количество обработанных событий из G4Run
 * 2. Обновить глобальный счётчик:
 *    sGlobalEventNumber += numEvents
 * 3. Напечатать статистику
 * 4. Напечатать следующий номер события
 *
 * **Алгоритм:**
 * ```cpp
 * int numEvents = run->GetNumberOfEvent();     // Получить кол-во событий
 * sGlobalEventNumber += numEvents;             // Обновить счётчик
 * // Напечатать результаты
 * ```
 *
 * **Вывод (пример):**
 * ```
 * Run ended. Processed 1000 events.
 * Next global event number: 1000
 * ```
 *
 * **Глобальный счётчик:**
 * Этот счётчик сохраняется между Run-ами и используется для
 * ГЛОБАЛЬНОЙ нумерации событий во всей симуляции.
 *
 * Пример для нескольких Run-ов:
 * ```
 * Run 1:
 *     BeginOfRunAction: sGlobalEventNumber = 0
 *     [обработать 1000 событий]
 *     EndOfRunAction: sGlobalEventNumber = 0 + 1000 = 1000
 *
 * Run 2:
 *     BeginOfRunAction: sGlobalEventNumber = 1000
 *     [обработать 500 событий]
 *     EndOfRunAction: sGlobalEventNumber = 1000 + 500 = 1500
 * ```
 *
 * **В CSV файле это будет:**
 * ```
 * Event,Particle,Energy(TeV),N<1eV@0.10us,...
 * 0,proton,10.000,1520,...       ← Run 1, Event 0
 * 999,proton,10.000,1502,...     ← Run 1, Event 999
 * 1000,neutron,5.000,850,...     ← Run 2, Event 0 (глобальный номер 1000)
 * 1499,neutron,5.000,875,...     ← Run 2, Event 499 (глобальный номер 1499)
 * ```
 *
 * **Связь с APPEND режимом CSV:**
 * В режиме APPEND CSVWriter использует GetLastRunNumber() для определения
 * последнего номера события в файле. С глобальным счётчиком это гарантирует
 * непрерывную нумерацию при продолжении симуляции.
 *
 * @param run Указатель на G4Run
 *            - run->GetRunID() - идентификатор Run
 *            - run->GetNumberOfEvent() - количество обработанных событий
 *
 * @note Вызывается один раз в конце каждого Run
 * @note Обновляет статический счётчик (сохраняется между Run-ами)
 * @note Может быть несколько Run-ов в одной программе
 *
 * **Примеры использования:**
 * ```cpp
 * // В main.cc
 * for (int i = 0; i < 10; ++i) {
 *     runAction->SetCurrentEnergy((i+1) * GeV);
 *     runManager->BeamOn(1000);  // EndOfRunAction обновит счётчик
 * }
 * // После всех Run-ов: sGlobalEventNumber = 10000
 * ```
 *
 * @see BeginOfRunAction() - начало Run
 * @see sGlobalEventNumber для глобальной нумерации
 * @see MyEventAction::GetNextEventNumber() для использования счётчика
 */
void MyRunAction::EndOfRunAction(const G4Run* run) {
    // Получаем количество обработанных событий в этом Run
    int numEvents = run->GetNumberOfEvent();
    
    // КРИТИЧНО: обновляем глобальный счётчик для следующего Run
    // Это гарантирует глобальную нумерацию событий
    sGlobalEventNumber += numEvents;
    
    // Печатаем статистику
    G4cout << "Run ended. Processed " << numEvents << " events." << G4endl;
    G4cout << "Next global event number: " << sGlobalEventNumber << G4endl;
}