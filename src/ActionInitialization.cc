/**
 * @file ActionInitialization.cc
 * @brief Инициализация всех Action объектов для симуляции
 *
 * Реализация класса ActionInitialization - центрального контроллера,
 * который связывает PrimaryGeneratorAction, MySteppingAction, MyEventAction
 * и CSVWriter в единую рабочую систему.
 *
 * **Основные обязанности:**
 * 1. BuildForMaster() - инициализация главного потока (обычно не требуется)
 * 2. Build() - инициализация рабочих потоков (создание Action объектов)
 * 3. Установка параметров (частица, энергия, задержки) для каждого потока
 * 4. Связывание всех компонентов в единую систему
 *
 * **Многопоточность:**
 * В многопоточном режиме (G4MTRunManager):
 * - BuildForMaster() вызывается один раз в главном потоке
 * - Build() вызывается один раз для каждого рабочего потока
 * - Каждый рабочий поток имеет свои копии Action объектов
 * - Результаты потоков объединяются в главном потоке
 *
 * @date 2024
 * @version 1.0
 *
 * @note В однопоточном режиме (G4RunManager) BuildForMaster() не вызывается,
 *       Build() вызывается один раз
 * @see G4VUserActionInitialization, ActionInitialization.hh
 */

#include "ActionInitialization.hh"
#include "PrimaryGeneratorAction.hh"
#include "MySteppingAction.hh"
#include "MyEventAction.hh"
#include "CSVWriter.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"

/**
 * @brief Конструктор с параметрами
 *
 * Инициализирует ActionInitialization с параметрами симуляции.
 * Сохраняет все параметры как члены класса для использования в Build().
 *
 * **Параметры конструктора:**
 * @param delays Вектор временных задержек для подсчёта нейтронов
 *               Пример: {0.10 * microsecond, 0.20 * microsecond, 0.50 * microsecond}
 *               Передаётся в MySteppingAction для инициализации карт
 *               @note Должны быть отсортированы по возрастанию
 *
 * @param csvWriter Указатель на синглтон CSVWriter для записи результатов
 *                  Один общий экземпляр для всех потоков
 *                  НЕ удаляется в деструкторе (управляется main)
 *                  Передаётся в MyEventAction через SetCSVWriter()
 *
 * @param particleName Название типа первичной частицы (строка)
 *                     Примеры: "proton", "neutron", "pion0", "pion-", "pion+"
 *                     Используется для:
 *                     - Установки типа в G4ParticleGun
 *                     - Логирования
 *                     - Записи в CSV
 *
 * @param energy Кинетическая энергия первичной частицы (в единицах GEANT4, ГэВ)
 *               Примеры: 10.0 * GeV, 5.0 * GeV, 100.0 * MeV
 *               Передаётся в G4ParticleGun через SetParticleEnergy()
 *
 * **Процесс инициализации:**
 * ```cpp
 * ActionInitialization(delays, csvWriter, particleName, energy)
 *     ├─ Base class: G4VUserActionInitialization()
 *     ├─ fDelays = delays
 *     ├─ fCSVWriter = csvWriter
 *     ├─ fParticleName = particleName
 *     └─ fEnergy = energy
 * ```
 *
 * @example
 * ```cpp
 * // В main функции:
 * std::vector<G4double> delays = {0.10 * microsecond, 0.20 * microsecond, 0.50 * microsecond};
 * CSVWriter* csvWriter = new CSVWriter("results.csv", true);
 * G4String particleName = "proton";
 * G4double energy = 10.0 * GeV;
 *
 * ActionInitialization* actionInit = new ActionInitialization(delays, csvWriter, particleName, energy);
 * runManager->SetUserInitialization(actionInit);
 * ```
 *
 * @note Параметры сохраняются в членах класса для использования в Build()
 * @see Build() - использует эти параметры при создании Action объектов
 * @see ActionInitialization.hh для деклерации
 */
ActionInitialization::ActionInitialization(const std::vector<G4double>& delays,
                                           CSVWriter* csvWriter,
                                           const G4String& particleName,
                                           G4double energy)
    : G4VUserActionInitialization(),
      fDelays(delays),
      fCSVWriter(csvWriter),
      fParticleName(particleName),
      fEnergy(energy)
{
}

/**
 * @brief Деструктор
 *
 * Освобождает ресурсы при удалении ActionInitialization.
 * НЕ удаляет:
 * - Action объекты (удаляются G4RunManager)
 * - fCSVWriter (управляется main)
 *
 * @note Вызывается при завершении программы (обычно автоматически)
 */
ActionInitialization::~ActionInitialization()
{
}

/**
 * @brief Инициализация главного потока (Master)
 *
 * Вызывается один раз в главном потоке при использовании многопоточного режима
 * (G4MTRunManager). В однопоточном режиме (G4RunManager) не вызывается.
 *
 * **Назначение:**
 * - Инициализация объектов, которые нужны главному потоку
 * - Обычно НЕ нужно создавать Action объекты (они создаются в рабочих потоках)
 * - Может использоваться для логирования, подготовки данных и т.д.
 *
 * **Типичная реализация:**
 * ```cpp
 * void ActionInitialization::BuildForMaster() const {
 *     // Обычно ничего не делаем - Action объекты создаются в Build() для рабочих потоков
 *     // Если нужна инициализация главного потока, делаем это здесь
 *
 *     // Пример: вывод информации о конфигурации
 *     G4cout << "Master thread initialized\n";
 * }
 * ```
 *
 * **Многопоточный режим:**
 * ```
 * Главный поток:
 *     ├─ BuildForMaster() ← ЗДЕСЬ
 *     └─ Ждёт завершения рабочих потоков
 *
 * Рабочие потоки (N потоков):
 *     ├─ Build() ← Создание Action объектов
 *     ├─ Симуляция N events
 *     └─ Отправка результатов в главный поток
 *
 * Главный поток:
 *     └─ Объединение результатов
 * ```
 *
 * @note В нашем проекте эта функция обычно пуста (комментарий в примере)
 * @note Вызывается ДО Build()
 * @note Только в многопоточном режиме (G4MTRunManager)
 *
 * @see Build() для создания Action объектов в рабочих потоках
 * @see https://geant4.web.cern.ch/geant4/application_developers/usersguides/ForApplicationDeveloper/html/Appendix_commands/threading.html
 */
void ActionInitialization::BuildForMaster() const
{
    // Master не создаёт Actions
    // (или можно добавить инициализацию специфичную для главного потока)
}

/**
 * @brief Инициализация рабочих потоков (Build Action объекты)
 *
 * Вызывается один раз для каждого рабочего потока при использовании многопоточного режима.
 * В однопоточном режиме вызывается один раз.
 *
 * **Основная обязанность:**
 * Создание всех Action объектов (PrimaryGeneratorAction, MySteppingAction, MyEventAction)
 * и установка их параметров через SetUserAction().
 *
 * **Алгоритм:**
 * 1. Создать PrimaryGeneratorAction
 * 2. Получить G4ParticleGun и установить параметры (частица, энергия)
 * 3. Вызвать SetUserAction(primaryGen) - регистрация в RunManager
 * 4. Создать MySteppingAction с временными задержками
 * 5. Вызвать SetUserAction(steppingAction)
 * 6. Создать MyEventAction с указателем на SteppingAction
 * 7. Передать параметры в EventAction (CSVWriter, частица, энергия)
 * 8. Вызвать SetUserAction(eventAction)
 *
 * **Структура связей:**
 * ```
 * PrimaryGeneratorAction
 *     └─ G4ParticleGun (создаёт первичные частицы)
 *
 * MySteppingAction
 *     └─ fDelays (временные задержки)
 *
 * MyEventAction
 *     ├─ MySteppingAction (доступ к данным нейтронов)
 *     ├─ CSVWriter (запись результатов)
 *     ├─ fCurrentParticleName
 *     └─ fCurrentEnergy
 *
 * G4RunManager
 *     ├─ SetUserAction(primaryGen)
 *     ├─ SetUserAction(steppingAction)
 *     └─ SetUserAction(eventAction)
 * ```
 *
 * **Пошаговая реализация:**
 * ```cpp
 * void ActionInitialization::Build() const {
 *     // ========== 1. PRIMARY GENERATOR ==========
 *     // Создание и инициализация пушки
 *     PrimaryGeneratorAction* primaryGen = new PrimaryGeneratorAction();
 *
 *     // Получить пушку и установить параметры
 *     G4ParticleGun* gun = primaryGen->GetParticleGun();
 *     G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
 *     G4ParticleDefinition* particle = particleTable->FindParticle(fParticleName);
 *
 *     // Установить частицу и энергию
 *     gun->SetParticleDefinition(particle);
 *     gun->SetParticleEnergy(fEnergy);
 *
 *     // Регистрация в RunManager
 *     SetUserAction(primaryGen);
 *
 *     // ========== 2. STEPPING ACTION ==========
 *     // Создание обработчика шагов с временными задержками
 *     MySteppingAction* steppingAction = new MySteppingAction(fDelays);
 *     SetUserAction(steppingAction);
 *
 *     // ========== 3. EVENT ACTION ==========
 *     // Создание обработчика событий
 *     MyEventAction* eventAction = new MyEventAction(steppingAction, "../data");
 *
 *     // Передача параметров в EventAction
 *     eventAction->SetCSVWriter(fCSVWriter);
 *     eventAction->SetCurrentParticle(fParticleName);
 *     eventAction->SetCurrentEnergy(fEnergy);
 *
 *     // Регистрация в RunManager
 *     SetUserAction(eventAction);
 * }
 * ```
 *
 * **Многопоточный режим:**
 * - Build() вызывается один раз для каждого рабочего потока
 * - Каждый рабочий поток получает свои копии Action объектов
 * - CSVWriter остаётся общим для всех потоков (одна копия)
 * - Параметры (частица, энергия, задержки) одинаковы для всех потоков
 *
 * @note Вызывается один раз в начале симуляции (однопоточный режим)
 *       или один раз для каждого рабочего потока (многопоточный режим)
 * @note Объекты, созданные здесь, управляются G4RunManager
 * @note НЕ удаляйте объекты вручную!
 *
 * @see BuildForMaster() для инициализации главного потока
 * @see SetUserAction() для регистрации Action объектов
 * @see PrimaryGeneratorAction, MySteppingAction, MyEventAction
 */
void ActionInitialization::Build() const
{
    // ========== 1. PRIMARY GENERATOR ACTION ==========
    // Создание генератора первичных частиц
    PrimaryGeneratorAction* primaryGen = new PrimaryGeneratorAction();
    
    // Получить пушку и установить её параметры
    G4ParticleGun* gun = primaryGen->GetParticleGun();
    G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
    G4ParticleDefinition* particle = particleTable->FindParticle(fParticleName);
    
    // Установить тип частицы и энергию для этого работника
    gun->SetParticleDefinition(particle);
    gun->SetParticleEnergy(fEnergy);
    
    // Регистрация в RunManager
    SetUserAction(primaryGen);
    
    // ========== 2. STEPPING ACTION ==========
    // Создание обработчика шагов с временными задержками для подсчёта нейтронов
    MySteppingAction* steppingAction = new MySteppingAction(fDelays);
    SetUserAction(steppingAction);
    
    // ========== 3. EVENT ACTION ==========
    // Создание обработчика событий для анализа и записи результатов
    MyEventAction* eventAction = new MyEventAction(steppingAction, "../data");
    eventAction->SetCSVWriter(fCSVWriter);
    eventAction->SetCurrentParticle(fParticleName);
    eventAction->SetCurrentEnergy(fEnergy);
    SetUserAction(eventAction);
}