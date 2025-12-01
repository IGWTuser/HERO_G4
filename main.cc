/**
 * @file main.cc
 * @brief Главная точка входа в программу HERO G4 симуляции
 *
 * Отвечает за:
 * - Парсинг аргументов командной строки
 * - Инициализацию всех компонентов GEANT4
 * - Установку параметров симуляции (частица, энергия, потоки)
 * - Запуск симуляции
 * - Управление ресурсами (выделение/освобождение памяти)
 *
 * **Использование:**
 * ```bash
 * ./HERO_G4 <threads> <events> <particle> <energy_TeV>
 *
 * # Примеры:
 * ./HERO_G4 8 100 proton 0.1      # 8 потоков, 100 событий, протон 0.1 ТеВ
 * ./HERO_G4 8 100 proton 0.25     # 0.25 ТеВ
 * ./HERO_G4 8 100 neutron 0.5     # Нейтроны 0.5 ТеВ
 * ./HERO_G4 4 1000 proton 1.0     # 4 потока, 1000 событий, 1 ТеВ
 * ```
 *
 * **Последовательность инициализации:**
 * ```
 * main()
 *     ├─ Парсинг аргументов командной строки
 *     │  ├─ nThreads (кол-во потоков для многопоточности)
 *     │  ├─ nEventsPerRun (кол-во событий в одном Run)
 *     │  ├─ particleName (тип частицы: proton, neutron и т.д.)
 *     │  └─ energyTeV (энергия в ТеВ, конвертируется в ГэВ)
 *     │
 *     ├─ Инициализация временных порогов для нейтронов
 *     │  └─ 15 временных задержек от 100нс до 200мкс
 *     │
 *     ├─ Инициализация CSV writer
 *     │  ├─ Открыть файл в режиме APPEND
 *     │  └─ Написать заголовок с колонками
 *     │
 *     ├─ Инициализация RunManager
 *     │  ├─ G4MTRunManager - многопоточный менеджер
 *     │  ├─ SetNumberOfThreads(nThreads)
 *     │  └─ SetUserInitialization(geometry, physics, actions)
 *     │
 *     ├─ Инициализация физического списка
 *     │  ├─ FTFP_BERT_HP - хаdronic physics list
 *     │  └─ G4NeutronTrackingCut - отсечка для нейтронов
 *     │
 *     ├─ Инициализация ActionInitialization
 *     │  ├─ PrimaryGeneratorAction
 *     │  ├─ MySteppingAction
 *     │  ├─ MyEventAction
 *     │  └─ MyRunAction
 *     │
 *     ├─ runManager->Initialize() - финальная инициализация
 *     │
 *     ├─ runManager->BeamOn(nEventsPerRun)
 *     │  └─ ЗАПУСК СИМУЛЯЦИИ (в многопоточном режиме!)
 *     │
 *     ├─ Подсчёт времени выполнения
 *     │
 *     └─ Освобождение памяти и выход
 * ```
 *
 * @author HERO Collaboration
 * @date 2024
 * @version 1.0
 *
 * @note Используется G4MTRunManager для многопоточности
 * @note Физический список: FTFP_BERT_HP (с поддержкой протонов и нейтронов)
 * @note CSV результаты в режиме APPEND (добавляются к существующему файлу)
 * @see DetectorConstruction, ActionInitialization, CSVWriter
 */

#include "G4MTRunManager.hh"  // Многопоточный менеджер запуска
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

/**
 * @brief Главная функция программы
 *
 * Точка входа в HERO G4 симуляцию.
 * Отвечает за инициализацию всех компонентов и запуск симуляции.
 *
 * **Аргументы командной строки:**
 * 1. threads - количество потоков для многопоточности (обычно 4-8)
 * 2. events - количество событий в одном Run (обычно 100-1000)
 * 3. particle - тип первичной частицы ("proton", "neutron", "pion0" и т.д.)
 * 4. energy_TeV - энергия в ТеВ (преобразуется в ГэВ для GEANT4)
 *
 * **Примеры вызова:**
 * ```bash
 * ./HERO_G4 8 100 proton 0.1
 * ├─ 8 потоков
 * ├─ 100 событий
 * ├─ протоны
 * └─ энергия 0.1 ТеВ = 100 ГэВ
 *
 * ./HERO_G4 4 1000 neutron 0.5
 * ├─ 4 потока
 * ├─ 1000 событий
 * ├─ нейтроны
 * └─ энергия 0.5 ТеВ = 500 ГэВ
 * ```
 *
 * @param argc Количество аргументов в командной строке
 * @param argv Массив указателей на строки аргументов
 *
 * @return int 0 при успехе, 1 при ошибке
 *
 * @see DetectorConstruction для геометрии
 * @see ActionInitialization для Actions
 * @see CSVWriter для сохранения результатов
 */
int main(int argc, char** argv) {
    // ========== ПАРСИНГ АРГУМЕНТОВ ==========
    
    // Проверяем количество аргументов
    // Нужны: программа + 4 аргумента = 5 всего
    if (argc < 5) {
        G4cout << "Usage: ./HERO_G4 <threads> <events> <particle> <energy_TeV>" << G4endl;
        G4cout << "Example: ./HERO_G4 8 100 proton 0.1" << G4endl;
        G4cout << "Example: ./HERO_G4 8 100 proton 0.25" << G4endl;
        return 1;
    }
    
    // **Аргумент 1: Количество потоков**
    // std::atoi - преобразовать строку в целое число
    int nThreads = std::atoi(argv[1]);
    
    // **Аргумент 2: Количество событий**
    int nEventsPerRun = std::atoi(argv[2]);
    
    // **Аргумент 3: Тип частицы**
    G4String particleName = argv[3];
    
    // **Аргумент 4: Энергия в ТеВ**
    // std::atof - преобразовать строку в double
    G4double energyTeV = std::atof(argv[4]);
    
    // Конвертируем ТеВ в ГэВ (GEANT4 единицы)
    // 1 ТеВ = 1000 ГэВ
    G4double energy = energyTeV * TeV;
    
    // ========== ПЕЧАТЬ КОНФИГУРАЦИИ ==========
    G4cout << "========================================" << G4endl;
    G4cout << "Configuration:" << G4endl;
    G4cout << "  Threads: " << nThreads << G4endl;
    G4cout << "  Events: " << nEventsPerRun << G4endl;
    G4cout << "  Particle: " << particleName << G4endl;
    G4cout << "  Energy: " << energyTeV << " TeV" << G4endl;
    G4cout << "========================================\n" << G4endl;
    
    // ========== УСТАНОВКА ВРЕМЕННЫХ ПОРОГОВ ==========
    
    /**
     * Временные задержки для подсчёта нейтронов.
     * Используются для определения "отложенных" нейтронов.
     *
     * Физика: нейтроны, появившиеся после определённого времени,
     * считаются появившимися в результате определённого механизма.
     *
     * **Примеры:**
     * - Нейтроны до 100 нс - быстрые нейтроны (прямое рождение)
     * - Нейтроны после 1 мкс - отложенные нейтроны
     * - Нейтроны после 100 мкс - очень отложенные
     */
    std::vector<G4double> delays = { 
        100*ns,    // 100 наносекунд
        250*ns,    // 250 наносекунд
        750*ns,    // 750 наносекунд
        1*us,      // 1 микросекунда
        2*us,      // 2 микросекунды
        4*us,      // 4 микросекунды
        8*us,      // 8 микросекунд
        10*us,     // 10 микросекунд
        20*us,     // 20 микросекунд
        35*us,     // 35 микросекунд
        50*us,     // 50 микросекунд
        100*us,    // 100 микросекунд
        125*us,    // 125 микросекунд
        150*us,    // 150 микросекунд
        200*us     // 200 микросекунд
    };
    
    // ========== ИНИЦИАЛИЗАЦИЯ CSV WRITER ==========
    
    /**
     * CSV файл будет содержать результаты симуляции.
     * Столбцы: Event, Particle, Energy(TeV), N<1eV@100ns, N<1eV@250ns, ...
     *
     * Режим APPEND (true) означает:
     * - Если файл уже существует - добавляем данные в конец
     * - Если файла нет - создаём новый
     * - Это позволяет продолжить симуляцию, не перезаписывая старые данные
     */
    G4String csvFilename = "../data/simulation_results.csv";
    CSVWriter* csvWriter = new CSVWriter(csvFilename, true);  // true = append mode
    csvWriter->WriteHeader(delays);  // Написать заголовок (если новый файл)
    
    // ========== ИНИЦИАЛИЗАЦИЯ RUNMANAGER ==========
    
    /**
     * G4MTRunManager - многопоточный менеджер запуска.
     * Позволяет запускать симуляцию на нескольких потоках одновременно.
     *
     * **Преимущества:**
     * - Ускорение симуляции в N раз (где N = кол-во потоков)
     * - Использует все ядра процессора
     *
     * **Осторожность:**
     * - Потокобезопасность критична!
     * - Используются G4Mutex и G4AutoLock для защиты общей памяти
     *
     * **Альтернатива:**
     * - G4RunManager - однопоточный менеджер (медленнее, но проще)
     */
    G4MTRunManager* runManager = new G4MTRunManager;
    runManager->SetNumberOfThreads(nThreads);  // Установить количество потоков
    G4cout << "Running with " << nThreads << " threads" << G4endl;

    // ========== ИНИЦИАЛИЗАЦИЯ ГЕОМЕТРИИ ==========
    
    /**
     * DetectorConstruction::Construct() создаёт:
     * - Мир (World) - куб 5x5x5 метров с вакуумом
     * - Детектор - шестиугольная призма из композитного материала
     * - Материалы - полистирен (scintillator) + вольфрам
     *
     * @see DetectorConstruction::Construct()
     */
    runManager->SetUserInitialization(new DetectorConstruction());

    // ========== ИНИЦИАЛИЗАЦИЯ ФИЗИЧЕСКОГО СПИСКА ==========
    
    /**
     * G4PhysListFactory - фабрика для создания предопределённых физических списков.
     *
     * **FTFP_BERT_HP:**
     * - FTFP = Fritiof-Tasso-Fermi-Pais (hadronic interactions at high energy)
     * - BERT = Bertini (hadronic interactions at low energy)
     * - HP = High Precision (для нейтронов)
     *
     * Это хороший выбор для высокоэнергетических частиц (ГэВ-ТеВ диапазон)
     * и низкоэнергетических нейтронов (эВ-кэВ диапазон).
     */
    G4PhysListFactory factory;
    G4VModularPhysicsList* physicsList = factory.GetReferencePhysList("FTFP_BERT_HP");
    runManager->SetUserInitialization(physicsList);

    // ========== НАСТРОЙКА ОТСЕЧКИ ДЛЯ НЕЙТРОНОВ ==========
    
    /**
     * G4NeutronTrackingCut - отсечка (cut) для отслеживания нейтронов.
     *
     * **TimeLimit(1*s):**
     * - Нейтроны, отслеживаемые дольше 1 секунды, будут остановлены
     * - Это предотвращает бесконечное отслеживание
     *
     * **KineticEnergyLimit(0*eV):**
     * - Нейтроны будут отслеживаться до энергии 0 эВ
     * - Ноль означает отслеживать всех нейтронов (без энергетической отсечки)
     *
     * **Почему это важно:**
     * - Низкоэнергетические нейтроны (< 1 эВ) интересуют в этом проекте
     * - Без этой настройки GEANT4 может не отслеживать их правильно
     */
    auto neutronCut = new G4NeutronTrackingCut();
    neutronCut->SetTimeLimit(1.*s);           // Максимум 1 секунда отслеживания
    neutronCut->SetKineticEnergyLimit(0.*eV); // Без энергетической отсечки
    physicsList->RegisterPhysics(neutronCut);
    
    // ========== ИНИЦИАЛИЗАЦИЯ ACTIONS ==========
    
    /**
     * ActionInitialization создаёт:
     * - PrimaryGeneratorAction (генерирует первичные частицы)
     * - MySteppingAction (обрабатывает каждый шаг)
     * - MyEventAction (анализирует события)
     * - MyRunAction (управляет Run-ами)
     *
     * Передаём параметры симуляции:
     * - delays - временные пороги для нейтронов
     * - csvWriter - для записи результатов
     * - particleName - тип частицы
     * - energy - энергия частицы
     */
    ActionInitialization* actionInit = 
        new ActionInitialization(delays, csvWriter, particleName, energy);
    runManager->SetUserInitialization(actionInit);
    
    // ========== ФИНАЛЬНАЯ ИНИЦИАЛИЗАЦИЯ ==========
    
    /**
     * runManager->Initialize() проверит все инициализации
     * и подготовит RunManager к запуску симуляции.
     *
     * После этого можно вызвать BeamOn() для запуска.
     */
    runManager->Initialize();
    
    // ========== ЗАСЕЧКА ВРЕМЕНИ ==========
    
    /**
     * Засекаем время начала симуляции для подсчёта длительности.
     * std::chrono::steady_clock - часы GEANT4 (не зависят от системного времени)
     */
    auto tStart = std::chrono::steady_clock::now();
    
    G4cout << "Starting simulation...\n" << G4endl;
    
    // ========== ЗАПУСК СИМУЛЯЦИИ ==========
    
    /**
     * runManager->BeamOn(N) - ГЛАВНЫЙ ВЫЗОВ!
     *
     * Это запускает N событий в многопоточном режиме.
     * Для каждого события:
     * 1. BeginOfRunAction() - начало Run
     * 2. BeginOfEventAction() - очистить данные
     * 3. GeneratePrimaries() - создать частицу
     * 4. [Отслеживание] - много шагов
     * 5. EndOfEventAction() - подсчитать и записать в CSV
     * 6. EndOfRunAction() - статистика
     *
     * В многопоточном режиме несколько потоков работают одновременно.
     * Потокобезопасность обеспечивается:
     * - G4AutoLock для защиты критических секций
     * - G4Mutex для синхронизации
     */
    runManager->BeamOn(nEventsPerRun);
    
    // ========== ПОДСЧЁТ ВРЕМЕНИ ВЫПОЛНЕНИЯ ==========
    
    /**
     * Вычисляем, сколько времени заняла симуляция.
     */
    auto tNow = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(tNow - tStart).count();
    
    G4cout << "\n========================================" << G4endl;
    G4cout << "Simulation completed in " << elapsed << " s" << G4endl;
    G4cout << "Results saved to: " << csvFilename << G4endl;
    G4cout << "========================================\n" << G4endl;
    
    // ========== ОСВОБОЖДЕНИЕ ПАМЯТИ ==========
    
    /**
     * ВАЖНО! Удаляем все объекты, которые мы выделили с помощью new.
     * Если этого не сделать - утечка памяти!
     *
     * **Порядок удаления критичен:**
     * - runManager управляет всеми Action объектами, поэтому удаляем его первым
     * - csvWriter удаляем последним (он может быть нужен другим компонентам)
     */
    delete runManager;   // Удаляет все Actions автоматически
    delete csvWriter;    // Закрывает CSV файл

    // ========== ВЫХОД ==========
    
    return 0;  // Успех!
}