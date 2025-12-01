/**
 * @file MyEventAction.hh
 * @brief Управление отдельным событием симуляции
 *
 * Выполняется в начале и конце каждого события (одного выстрела частицы).
 * Отвечает за инициализацию, сбор статистики и финализацию данных события.
 *
 * Основные функции:
 * - BeginOfEventAction() - подготовка к событию (обнуление счетчиков)
 * - EndOfEventAction() - сбор и сохранение данных события
 * - CountLowEnergyNeutrons() - подсчёт вторичных нейтронов по энергии
 * - Передача параметров из главной программы (частица, энергия)
 *
 * Данные события используются для:
 * - Построения спектров энергии нейтронов
 * - Анализа временных корреляций (задержки)
 * - Отбора интересных физических событий
 * - Статистического анализа процессов взаимодействия
 *
 * @date 2024
 * @version 1.0
 *
 * @note Работает в тесной связи с MySteppingAction для доступа к данным шагов
 * @see G4UserEventAction, MySteppingAction, CSVWriter
 */

#ifndef MyEventAction_h
#define MyEventAction_h 1

#include "G4UserEventAction.hh"
#include "MySteppingAction.hh"
#include "G4String.hh"
#include "G4SystemOfUnits.hh"
#include <vector>
#include <utility>

// Форвард деклерация
class CSVWriter;

/**
 * @class MyEventAction
 * @brief Обработка события симуляции
 *
 * Вызывается для каждого события жизненного цикла:
 * 1. BeginOfEventAction() - перед генерацией первичной частицы
 * 2. (отслеживание первичной частицы и вторичных частиц)
 * 3. (вызовы MySteppingAction для каждого шага)
 * 4. EndOfEventAction() - после завершения отслеживания всех треков
 *
 * Отвечает за:
 * - **Инициализацию**: обнуление счетчиков и очистка контейнеров
 * - **Сбор информации**: получение данных из MySteppingAction
 * - **Анализ**: подсчёт вторичных частиц (нейтронов) по энергиям
 * - **Финализацию**: сохранение результатов в CSV файл
 *
 * Жизненный цикл события:
 * ```
 * BeginOfEventAction()
 *     ↓
 * PrimaryGeneratorAction::GeneratePrimaries()
 *     ↓
 * [отслеживание первичной частицы]
 *     ↓
 * [для каждого шага]
 *     MySteppingAction::UserSteppingAction()
 *         ↓
 *         [регистрация вторичных частиц, энергий]
 *     ↓
 * EndOfEventAction()
 *     ↓
 * [запись в CSV, анализ результатов]
 * ```
 *
 * **Связь с другими классами:**
 * - **MySteppingAction**: хранит данные каждого шага (энергию, время, тип частицы)
 * - **CSVWriter**: пишет результаты события в файл
 * - **PrimaryGeneratorAction**: генерирует первичную частицу
 *
 * @see MySteppingAction для доступа к данным шагов
 * @see CSVWriter для записи результатов
 */
class MyEventAction : public G4UserEventAction {
public:
    /**
     * @brief Конструктор с параметрами
     *
     * Инициализирует EventAction и связывает его со SteppingAction для доступа к данным.
     *
     * @param steppingAction Указатель на MySteppingAction для доступа к данным шагов
     *                       Содержит информацию о частицах, энергиях, времени
     *                       @warning steppingAction НЕ ДОЛЖЕН быть nullptr
     *
     * @param dataDir Путь к папке для сохранения выходных файлов (по умолчанию "../data")
     *                Если папка не существует, её нужно создать в main
     *                Относительный путь или абсолютный
     *                Пример: "../data" или "/home/user/simulation_output"
     *
     * @example
     * ```cpp
     * MySteppingAction* stepping = new MySteppingAction();
     * MyEventAction* eventAction = new MyEventAction(stepping, "../results");
     * ```
     *
     * @see SetCSVWriter(), SetCurrentParticle(), SetCurrentEnergy()
     */
    MyEventAction(MySteppingAction* steppingAction,
                  const G4String& dataDir = "../data");
    
    /**
     * @brief Деструктор
     *
     * Освобождает ресурсы и завершает работу с файлами.
     * НЕ удаляет fSteppingAction и fCSVWriter - они управляются другими объектами.
     */
    virtual ~MyEventAction();

    /**
     * @brief Инициализация события
     *
     * Вызывается в начале каждого события, ДО генерации первичной частицы.
     * Используется для обнуления счетчиков и подготовки контейнеров к новым данным.
     *
     * Типичные операции:
     * - Обнуление счетчиков энергии/времени
     * - Очистка векторов и списков
     * - Инициализация флагов события
     * - Вывод отладочной информации
     *
     * @param event Указатель на G4Event (содержит ID события, время и т.д.)
     *              Обычно используется для вывода информации о событии
     *
     * @example
     * ```cpp
     * void MyEventAction::BeginOfEventAction(const G4Event* event) {
     *     fNeutronCount = 0;
     *     fTotalEnergy = 0.;
     *     fNeutronEnergies.clear();
     *     if (event && event->GetEventID() % 1000 == 0) {
     *         G4cout << "Event " << event->GetEventID() << " started\n";
     *     }
     * }
     * ```
     *
     * @see EndOfEventAction()
     * @note Вызывается один раз в начале каждого события
     * @note Параметр event может быть nullptr
     */
    virtual void BeginOfEventAction(const G4Event* event) override;
    
    /**
     * @brief Финализация события
     *
     * Вызывается в конце события, после завершения отслеживания всех треков.
     * Используется для анализа собранных данных и сохранения результатов.
     *
     * Типичные операции:
     * 1. Получение данных из MySteppingAction
     * 2. Анализ и фильтрация (энергии, типы частиц)
     * 3. Подсчёт вторичных частиц (нейтронов) в разных энергетических окнах
     * 4. Применение критериев отбора событий
     * 5. Запись результатов в CSV через CSVWriter
     * 6. Сохранение детальных файлов (опционально)
     *
     * @param event Указатель на G4Event (содержит полную информацию о событии)
     *
     * Процесс записи:
     * ```cpp
     * void MyEventAction::EndOfEventAction(const G4Event* event) {
     *     // 1. Получить данные
     *     std::vector<int> counts = CountLowEnergyNeutrons(1.0 * eV);
     *
     *     // 2. Подготовить параметры
     *     int eventID = fCSVWriter->GetNextEventNumber();
     *     G4double energyTeV = fCurrentEnergy / TeV;
     *
     *     // 3. Записать в CSV
     *     if (fCSVWriter) {
     *         fCSVWriter->WriteRow(eventID, fCurrentParticleName, 
     *                             energyTeV, counts);
     *     }
     * }
     * ```
     *
     * @see BeginOfEventAction(), CountLowEnergyNeutrons()
     * @see CSVWriter::WriteRow() для формата записи
     * @note Вызывается один раз в конце каждого события
     * @note **КРИТИЧНО**: здесь должна быть запись в CSV!
     * @note Может быть вызвано из разных потоков (многопоточный режим)
     */
    virtual void EndOfEventAction(const G4Event* event) override;

    /**
     * @brief Сохранить общую статистику всех событий
     *
     * Сохраняет итоговую статистику по завершении всего Run.
     * Обычно вызывается из MyRunAction::EndOfRunAction().
     *
     * Может включать:
     * - Общее количество событий
     * - Среднюю энергию нейтронов
     * - Статистику по типам событий
     * - Информацию о параметрах симуляции
     *
     * @example
     * ```cpp
     * // В MyRunAction::EndOfRunAction()
     * eventAction->SaveSummaryData();
     * ```
     *
     * @see MyRunAction::EndOfRunAction()
     */
    void SaveSummaryData();
    
    /**
     * @brief Установить объект для записи в CSV
     *
     * Передаёт указатель на синглтон CSVWriter для записи результатов.
     * Должна быть вызвана до начала симуляции (в main или RunAction).
     *
     * @param writer Указатель на CSVWriter (синглтон)
     *               Используется для записи данных события в CSV
     *
     * @example
     * ```cpp
     * CSVWriter writer("results.csv", true);
     * MyEventAction* eventAction = new MyEventAction(stepping);
     * eventAction->SetCSVWriter(&writer);
     * ```
     *
     * @see CSVWriter::WriteRow()
     * @note writer НЕ удаляется в деструкторе (это синглтон)
     */
    void SetCSVWriter(CSVWriter* writer) { fCSVWriter = writer; }
    
    /**
     * @brief Установить текущий тип частицы
     *
     * Передаёт информацию о типе первичной частицы для записи в CSV.
     * Должна быть вызвана перед симуляцией события (обычно в main перед runManager->BeamOn).
     *
     * @param name Название типа частицы (строка)
     *             Стандартные значения: "proton", "neutron", "pion0", "pion-", "pion+"
     *             Полный список доступен в GEANT4 документации
     *
     * @example
     * ```cpp
     * eventAction->SetCurrentParticle("proton");
     * runManager->BeamOn(1000);  // 1000 протонов
     * ```
     *
     * @see SetCurrentEnergy()
     * @note Значение остаётся неизменным для всех событий в Run
     */
    void SetCurrentParticle(const G4String& name) { fCurrentParticleName = name; }
    
    /**
     * @brief Установить текущую энергию первичной частицы
     *
     * Передаёт информацию об энергии первичной частицы для записи в CSV.
     * Должна быть вызвана перед симуляцией события.
     *
     * @param energy Кинетическая энергия (в единицах GEANT4, обычно ГэВ)
     *               Используется для деления на TeV при записи в CSV
     *               Пример: 10.0 * GeV для 10 ГэВ
     *
     * @example
     * ```cpp
     * eventAction->SetCurrentEnergy(10.0 * GeV);
     * runManager->BeamOn(1000);
     * ```
     *
     * @see SetCurrentParticle()
     * @note Значение остаётся неизменным для всех событий в Run
     */
    void SetCurrentEnergy(G4double energy) { fCurrentEnergy = energy; }
    
    /**
     * @brief Установить глобальный номер события (если используется)
     *
     * Передаёт информацию о глобальной нумерации событий (опционально).
     * Используется если нужна кастомная нумерация вместо GetNextEventNumber().
     *
     * @param num Порядковый номер события (начиная с 1)
     *
     * @example
     * ```cpp
     * eventAction->SetGlobalEventNumber(1);
     * ```
     *
     * @note Обычно используется GetNextEventNumber() из CSVWriter
     * @see CSVWriter::GetNextEventNumber()
     */
    void SetGlobalEventNumber(int num) { fGlobalEventNumber = num; }
    
    /**
     * @brief Подсчитать вторичные нейтроны с энергией ниже порога
     *
     * Подсчитывает количество нейтронов (или других вторичных частиц)
     * с энергией ниже заданного порога для каждой временной задержки.
     *
     * Возвращает вектор, где каждый элемент соответствует одной временной задержке:
     * - counts[0] - нейтроны до первой задержки
     * - counts[1] - нейтроны до второй задержки
     * - counts[n] - нейтроны до n-й задержки
     *
     * **Алгоритм:**
     * 1. Получить список всех вторичных нейтронов из MySteppingAction
     * 2. Для каждой временной задержки:
     *    - Подсчитать нейтроны с энергией < energyThreshold
     *    - И временем < delay
     * 3. Вернуть вектор подсчётов
     *
     * @param energyThreshold Пороговая энергия для отсчета нейтронов (по умолчанию 1 эВ)
     *                        Нейтроны с энергией < этого значения учитываются
     *                        Пример: 1.0 * eV, 10.0 * keV, 1.0 * MeV
     *
     * @return std::vector<int> Вектор подсчётов нейтронов для каждой задержки
     *                          Размер = количество задержек в MySteppingAction
     *
     * @example
     * ```cpp
     * // Подсчитать нейтроны ниже 1 эВ для каждой задержки
     * std::vector<int> counts = CountLowEnergyNeutrons(1.0 * eV);
     * // counts[0] = 1520 нейтронов до 0.10 µs
     * // counts[1] = 1634 нейтронов до 0.20 µs
     * // counts[2] = 1702 нейтронов до 0.50 µs
     * 
     * // Запись в CSV
     * fCSVWriter->WriteRow(eventID, "proton", 10.0, counts);
     * ```
     *
     * @see MySteppingAction для доступа к данным нейтронов
     * @see CSVWriter::WriteRow() для использования результата
     * @note **THREAD-SAFE**: может вызваться из разных потоков
     * @note Результат зависит от данных в MySteppingAction
     */
    std::vector<int> CountLowEnergyNeutrons(G4double energyThreshold = 1.0 * eV) const;

private:
    /**
     * @brief Построить базовое имя файла из параметров события
     *
     * Создает строку для имени файла на основе типа частицы и энергии.
     * Используется для сохранения детальных данных события.
     *
     * @param event Указатель на G4Event
     * @return G4String Базовое имя вида "proton_10GeV_event1234"
     *
     * @see FormatEnergy(), FormatDelay(), Sanitize()
     */
    G4String BuildBaseNameFromPrimary(const G4Event* event) const;
    
    /**
     * @brief Форматировать энергию в строку
     *
     * Преобразует численное значение энергии в удобный для чтения формат.
     *
     * @param e Энергия (в единицах GEANT4)
     * @return G4String Форматированная строка (например, "10GeV", "500MeV")
     *
     * @example
     * ```cpp
     * G4String str = FormatEnergy(10.0 * GeV);  // "10GeV"
     * ```
     *
     * @see FormatDelay()
     */
    static G4String FormatEnergy(G4double e);
    
    /**
     * @brief Форматировать время/задержку в строку
     *
     * Преобразует численное значение времени в удобный для чтения формат.
     *
     * @param t Время (в единицах GEANT4)
     * @return G4String Форматированная строка (например, "0.10us", "100ns")
     *
     * @example
     * ```cpp
     * G4String str = FormatDelay(0.10 * microsecond);  // "0.10us"
     * ```
     *
     * @see FormatEnergy()
     */
    static G4String FormatDelay(G4double t);
    
    /**
     * @brief Очистить строку от недопустимых символов для имени файла
     *
     * Удаляет или заменяет символы, которые недопустимы в имени файла.
     * Например: пробелы → подчеркивание, "/" → "_", и т.д.
     *
     * @param s Исходная строка
     * @return G4String Очищенная строка, безопасная для использования в имени файла
     *
     * @example
     * ```cpp
     * G4String safe = Sanitize("proton 10 GeV");  // "proton_10_GeV"
     * ```
     *
     * @see BuildBaseNameFromPrimary()
     */
    static G4String Sanitize(const G4String& s);

private:
    /// @brief Указатель на MySteppingAction для доступа к данным каждого шага
    ///
    /// Содержит информацию о:
    /// - Всех вторичных частицах, рождённых в этом событии
    /// - Энергии каждой частицы
    /// - Времени рождения (для подсчёта по задержкам)
    /// - Типе частицы (нейтрон, фотон, и т.д.)
    ///
    /// Используется в CountLowEnergyNeutrons() для анализа
    MySteppingAction* fSteppingAction;
    
    /// @brief Путь к папке для сохранения выходных файлов
    ///
    /// По умолчанию "../data" (относительно рабочей папки)
    /// Используется для сохранения детальных данных события
    /// Может быть изменено через конструктор
    G4String fDataDirectory;
    
    /// @brief Указатель на объект для записи в CSV файл (синглтон)
    ///
    /// Один экземпляр используется для записи из всех событий
    /// Устанавливается через SetCSVWriter()
    /// НЕ удаляется в деструкторе (управляется main программой)
    CSVWriter* fCSVWriter;
    
    /// @brief Название текущего типа первичной частицы
    ///
    /// Примеры: "proton", "neutron", "pion0"
    /// Передаётся в CSV при записи события
    /// Устанавливается через SetCurrentParticle()
    G4String fCurrentParticleName;
    
    /// @brief Кинетическая энергия первичной частицы (в единицах GEANT4, ГэВ)
    ///
    /// Пример: 10.0 * GeV
    /// Передаётся в CSV при записи (преобразуется в ТеВ)
    /// Устанавливается через SetCurrentEnergy()
    G4double fCurrentEnergy;
    
    /// @brief Глобальный номер события (опционально)
    ///
    /// Используется если нужна кастомная нумерация
    /// Обычно используется GetNextEventNumber() из CSVWriter
    int fGlobalEventNumber;
};

#endif