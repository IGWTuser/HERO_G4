/**
 * @file MyRunAction.hh
 * @brief Управление серией событий (Run) в симуляции
 *
 * Выполняется в начале и конце серии событий (Run).
 * Run - это совокупность нескольких событий с одинаковыми параметрами
 * (одна частица, одна энергия, один тип геометрии).
 *
 * Основные функции:
 * - BeginOfRunAction() - инициализация перед серией (открытие файлов, обнуление счетчиков)
 * - EndOfRunAction() - финализация после серии (закрытие файлов, вывод статистики)
 * - Управление глобальным счётчиком событий (static переменная)
 * - Передача параметров Run-а (частица, энергия) в EventAction
 *
 * Иерархия исполнения:
 * ```
 * Simulation (вся программа)
 * └── Run (серия событий: например, 1000 протонов 10 ГэВ)
 *     ├── Event 1 (один выстрел)
 *     ├── Event 2 (один выстрел)
 *     └── Event N (один выстрел)
 * ```
 *
 * Пример использования:
 * ```cpp
 * // 1. Первый Run: 1000 протонов 10 ГэВ
 * runAction->SetCurrentParticle("proton");
 * runAction->SetCurrentEnergy(10.0 * GeV);
 * runManager->BeamOn(1000);  // BeginOfRunAction → 1000 события → EndOfRunAction
 *
 * // 2. Второй Run: 500 нейтронов 5 ГэВ
 * runAction->SetCurrentParticle("neutron");
 * runAction->SetCurrentEnergy(5.0 * GeV);
 * runManager->BeamOn(500);   // BeginOfRunAction → 500 событий → EndOfRunAction
 * ```
 *
 * @date 2024
 * @version 1.0
 *
 * @note В многопоточном режиме (G4MTRunManager) BeginOfRunAction вызывается один раз,
 *       а EndOfRunAction вызывается для каждого потока и затем в главном потоке
 * @see G4UserRunAction, MyEventAction, CSVWriter
 */

#ifndef MyRunAction_h
#define MyRunAction_h 1

#include "G4UserRunAction.hh"
#include "MyEventAction.hh"
#include "globals.hh"

/**
 * @class MyRunAction
 * @brief Управление сериями событий (Run) в симуляции
 *
 * Наследуется из G4UserRunAction. Главные методы:
 * - BeginOfRunAction() - вызывается один раз в начале Run-а
 * - EndOfRunAction() - вызывается один раз в конце Run-а
 *
 * Отвечает за:
 * 1. **Инициализацию Run-а**
 *    - Открытие CSV файла (первый Run) или использование существующего
 *    - Запись заголовка (первый раз)
 *    - Обнуление счетчиков и статистики
 *    - Логирование параметров Run-а
 *
 * 2. **Финализацию Run-а**
 *    - Вывод итоговой статистики
 *    - Закрытие файлов (если необходимо)
 *    - Сохранение итогов в лог
 *
 * 3. **Управление глобальным счётчиком событий**
 *    - Сохранение номера последнего события
 *    - Передача информации между Run-ами
 *    - Гарантия уникальности номеров при append режиме
 *
 * 4. **Передача параметров в EventAction**
 *    - Текущий тип частицы
 *    - Текущая энергия
 *    - Глобальный номер события
 *
 * Жизненный цикл:
 * ```
 * BeginOfRunAction()
 *     ↓
 * [для каждого события в Run-е]
 *     MyEventAction::BeginOfEventAction()
 *         ↓
 *     [отслеживание частицы]
 *         ↓
 *     MyEventAction::EndOfEventAction()
 *     ↓
 * EndOfRunAction()
 *     ↓
 * [возможно начало следующего Run-а]
 * ```
 *
 * **В многопоточном режиме:**
 * - BeginOfRunAction() вызывается один раз в главном потоке перед запуском
 * - Каждый рабочий поток имеет свою копию MyEventAction
 * - EndOfRunAction() вызывается один раз в главном потоке после завершения всех потоков
 *
 * @see MyEventAction для работы с отдельными событиями
 * @see CSVWriter для записи результатов
 */
class MyRunAction : public G4UserRunAction {
public:
    /**
     * @brief Конструктор с параметром
     *
     * Инициализирует RunAction и связывает его с EventAction.
     *
     * @param eventAction Указатель на MyEventAction для передачи параметров Run-а
     *                    Содержит методы SetCurrentParticle() и SetCurrentEnergy()
     *                    @warning eventAction НЕ ДОЛЖЕН быть nullptr
     *
     * @example
     * ```cpp
     * MyEventAction* eventAction = new MyEventAction(stepping);
     * MyRunAction* runAction = new MyRunAction(eventAction);
     * 
     * // Или в ActionInitialization::Build():
     * MyEventAction* eventAction = new MyEventAction();
     * MyRunAction* runAction = new MyRunAction(eventAction);
     * SetUserAction(eventAction);
     * SetUserAction(runAction);
     * ```
     *
     * @see SetCurrentParticle(), SetCurrentEnergy()
     */
    MyRunAction(MyEventAction* eventAction);
    
    /**
     * @brief Деструктор
     *
     * Освобождает ресурсы при удалении объекта.
     * НЕ удаляет fEventAction - он управляется G4RunManager.
     */
    virtual ~MyRunAction();

    /**
     * @brief Инициализация серии событий (Run)
     *
     * Вызывается один раз в начале каждого Run-а (перед первым событием).
     * Используется для подготовки к серии событий.
     *
     * Типичные операции:
     * 1. **Инициализация CSV записи**
     *    - Получить CSVWriter синглтон
     *    - Записать заголовок (если первый Run или новый файл)
     *    - Получить начальный номер события
     *
     * 2. **Обнуление счетчиков**
     *    - Счётчик событий в этом Run-е
     *    - Счётчик вторичных частиц
     *    - Статистика по энергиям
     *
     * 3. **Логирование**
     *    - Вывод информации о параметрах Run-а
     *    - Вывод информации об используемых параметрах
     *    - Информация о файле вывода
     *
     * 4. **Передача параметров в EventAction**
     *    - Установить текущую частицу в eventAction
     *    - Установить текущую энергию в eventAction
     *    - Установить CSV writer
     *
     * Пример реализации:
     * ```cpp
     * void MyRunAction::BeginOfRunAction(const G4Run* run) {
     *     // Получить CSV writer
     *     CSVWriter& writer = CSVWriter::GetInstance("results.csv");
     *     
     *     // Записать заголовок в первый раз
     *     if (run->GetRunID() == 0) {
     *         writer.WriteHeader(fDelayTimes);
     *     }
     *     
     *     // Передать параметры в EventAction
     *     fEventAction->SetCSVWriter(&writer);
     *     fEventAction->SetCurrentParticle(fCurrentParticleName);
     *     fEventAction->SetCurrentEnergy(fCurrentEnergy);
     *     
     *     // Логирование
     *     G4cout << "\n=== Run " << run->GetRunID() << " started ===" << G4endl;
     *     G4cout << "Particle: " << fCurrentParticleName << G4endl;
     *     G4cout << "Energy: " << fCurrentEnergy / GeV << " GeV" << G4endl;
     * }
     * ```
     *
     * @param run Указатель на G4Run (содержит ID Run-а, количество событий и т.д.)
     *            Используется для определения номера Run-а (первый, второй и т.д.)
     *
     * @see EndOfRunAction()
     * @note Вызывается один раз в начале каждого Run-а
     * @note В многопоточном режиме вызывается только один раз в главном потоке
     * @note **ВАЖНО**: здесь нужно инициализировать CSV и передать параметры в EventAction
     *
     * @see MyEventAction::SetCSVWriter(), SetCurrentParticle(), SetCurrentEnergy()
     * @see CSVWriter::GetInstance(), WriteHeader()
     */
    virtual void BeginOfRunAction(const G4Run* run) override;
    
    /**
     * @brief Финализация серии событий (Run)
     *
     * Вызывается один раз в конце каждого Run-а (после последнего события).
     * Используется для завершения обработки серии и вывода статистики.
     *
     * Типичные операции:
     * 1. **Сбор статистики**
     *    - Получить количество событий в Run-е
     *    - Получить информацию о вторичных частицах
     *    - Рассчитать средние значения
     *
     * 2. **Логирование статистики**
     *    - Вывести количество обработанных событий
     *    - Вывести статистику взаимодействий
     *    - Вывести среднюю энергию вторичных частиц
     *    - Вывести время исполнения (если отслеживается)
     *
     * 3. **Финализация данных**
     *    - Сохранить итоговую статистику (опционально)
     *    - Закрыть промежуточные файлы (если используются)
     *    - Очистить временные данные
     *
     * 4. **Подготовка к следующему Run-у**
     *    - Обновить глобальный счётчик событий
     *    - Логирование переднего Run-а
     *
     * Пример реализации:
     * ```cpp
     * void MyRunAction::EndOfRunAction(const G4Run* run) {
     *     G4int nofEvents = run->GetNumberOfEvent();
     *     if (nofEvents == 0) return;  // Нет событий
     *     
     *     // Вывести статистику
     *     G4cout << "\n=== Run " << run->GetRunID() << " completed ===" << G4endl;
     *     G4cout << "Total events: " << nofEvents << G4endl;
     *     G4cout << "Global event number: " << GetGlobalEventNumber() << G4endl;
     *     
     *     // Сохранить итоги
     *     fEventAction->SaveSummaryData();
     *     
     *     // Обновить счётчик
     *     sGlobalEventNumber += nofEvents;
     * }
     * ```
     *
     * @param run Указатель на G4Run (содержит количество событий, время и т.д.)
     *
     * @see BeginOfRunAction()
     * @see MyEventAction::SaveSummaryData()
     * @note Вызывается один раз в конце каждого Run-а
     * @note В многопоточном режиме вызывается после завершения всех потоков
     * @note **ВАЖНО**: здесь нужно вывести финальную статистику!
     */
    virtual void EndOfRunAction(const G4Run* run) override;
    
    /**
     * @brief Установить текущий тип частицы для Run-а
     *
     * Передаёт информацию о типе первичной частицы для всех событий в этом Run-е.
     * Должна быть вызвана перед BeamOn() и перед BeginOfRunAction().
     *
     * @param name Название типа частицы (строка)
     *             Стандартные значения: "proton", "neutron", "pion0", "pion-", "pion+"
     *             Список доступных частиц в GEANT4 документации
     *
     * @example
     * ```cpp
     * runAction->SetCurrentParticle("proton");
     * eventAction->SetCurrentParticle("proton");  // передать в EventAction
     * runManager->BeamOn(1000);  // 1000 протонов
     * ```
     *
     * @see SetCurrentEnergy()
     * @note Значение остаётся неизменным для всех событий в Run-е
     * @note Должна быть передана в EventAction перед событиями
     */
    void SetCurrentParticle(const G4String& name) { fCurrentParticleName = name; }
    
    /**
     * @brief Установить текущую энергию первичной частицы для Run-а
     *
     * Передаёт информацию об энергии первичной частицы для всех событий в этом Run-е.
     * Должна быть вызвана перед BeamOn() и перед BeginOfRunAction().
     *
     * @param energy Кинетическая энергия (в единицах GEANT4, обычно ГэВ)
     *               Используется для CSV записи
     *               Пример: 10.0 * GeV, 5.0 * GeV, 100.0 * MeV
     *
     * @example
     * ```cpp
     * runAction->SetCurrentEnergy(10.0 * GeV);
     * eventAction->SetCurrentEnergy(10.0 * GeV);  // передать в EventAction
     * runManager->BeamOn(1000);
     * ```
     *
     * @see SetCurrentParticle()
     * @note Значение остаётся неизменным для всех событий в Run-е
     * @note Должна быть передана в EventAction перед событиями
     */
    void SetCurrentEnergy(G4double energy) { fCurrentEnergy = energy; }
    
    /**
     * @brief Получить глобальный номер последнего события
     *
     * Возвращает количество всех обработанных событий во всех Run-ах.
     * Используется для отслеживания общего прогресса симуляции.
     *
     * Поведение:
     * - После первого Run-а с 1000 событиями: вернёт 1000
     * - После второго Run-а с 500 событиями: вернёт 1500
     * - И т.д.
     *
     * @return int Глобальный номер последнего события (cumulative count)
     *
     * @example
     * ```cpp
     * // После первого Run-а (1000 протонов)
     * int total1 = MyRunAction::GetGlobalEventNumber();  // вернёт 1000
     * 
     * // После второго Run-а (500 нейтронов)
     * int total2 = MyRunAction::GetGlobalEventNumber();  // вернёт 1500
     * 
     * G4cout << "Всего обработано " << total2 << " событий\n";
     * ```
     *
     * @see sGlobalEventNumber
     * @note Это статический метод - вызывается как MyRunAction::GetGlobalEventNumber()
     * @note Значение увеличивается в EndOfRunAction()
     */
    static int GetGlobalEventNumber() { return sGlobalEventNumber; }

private:
    /// @brief Указатель на MyEventAction для передачи параметров Run-а
    ///
    /// Используется для вызова:
    /// - SetCurrentParticle() - передать тип частицы
    /// - SetCurrentEnergy() - передать энергию
    /// - SetCSVWriter() - передать объект для записи
    ///
    /// НЕ удаляется в деструкторе (управляется G4RunManager)
    MyEventAction* fEventAction;
    
    /// @brief Название текущего типа первичной частицы
    ///
    /// Примеры: "proton", "neutron", "pion0", "pion-", "pion+"
    /// Передаётся в EventAction через BeginOfRunAction()
    /// Остаётся неизменным для всех событий в Run-е
    /// Устанавливается через SetCurrentParticle()
    G4String fCurrentParticleName;
    
    /// @brief Кинетическая энергия первичной частицы (в единицах GEANT4, ГэВ)
    ///
    /// Пример: 10.0 * GeV
    /// Передаётся в EventAction через BeginOfRunAction()
    /// Остаётся неизменным для всех событий в Run-е
    /// Используется для CSV записи (преобразуется в ТеВ)
    /// Устанавливается через SetCurrentEnergy()
    G4double fCurrentEnergy;
    
    /// @brief Статический счётчик глобального номера события
    ///
    /// Сохраняет количество всех обработанных событий во всех Run-ах.
    /// Увеличивается в EndOfRunAction() на количество событий в текущем Run-е.
    ///
    /// Инициализация:
    /// - Начальное значение: 0
    /// - После первого Run-а (1000 событий): 1000
    /// - После второго Run-а (500 событий): 1500
    /// - И т.д.
    ///
    /// Используется для:
    /// - Глобальной нумерации событий
    /// - Логирования прогресса
    /// - Определения начального номера при append режиме CSV
    ///
    /// @note Это СТАТИЧЕСКАЯ переменная - одна копия на весь класс
    /// @note Объявление: static int sGlobalEventNumber;
    /// @note Инициализация должна быть в .cc файле: int MyRunAction::sGlobalEventNumber = 0;
    static int sGlobalEventNumber;
};

#endif