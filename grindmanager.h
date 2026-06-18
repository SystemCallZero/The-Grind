#ifndef GRINDMANAGER_H
#define GRINDMANAGER_H

#include <QObject>
#include <QVector>
#include <QString>

// Структура одного упражнения
struct Exercise {
    QString name;
    bool isDone = false;
};

// Структура дня тренировки
struct WorkoutDay {
    QString dayName;
    QVector<Exercise> exercises;
    int level = 0;          // текущий уровень (количество выполненных «кругов»)
};

class GrindManager : public QObject
{
    Q_OBJECT
public:
    explicit GrindManager(QObject *parent = nullptr);

    // Загрузка / сохранение
    bool loadFromFile(const QString &filePath = "grind_data.json");
    bool saveToFile(const QString &filePath = "grind_data.json");

    // Доступ к данным (только чтение)
    QVector<WorkoutDay> getAllWorkouts() const { return m_workouts; }
    int getDaysCount() const { return m_workouts.size(); }
    QString getDayName(int index) const;
    int getDayLevel(int index) const;
    int getExercisesCount(int dayIndex) const;
    QString getExerciseName(int dayIndex, int exIndex) const;
    bool getExerciseDone(int dayIndex, int exIndex) const;

    // Операции изменения
    void addDay(const QString &name);
    void removeDay(int index);
    void renameDay(int index, const QString &newName);

    void addExercise(int dayIndex, const QString &name);
    void removeExercise(int dayIndex, int exIndex);
    void renameExercise(int dayIndex, int exIndex, const QString &newName);

    // Переключить галочку – основная логика с повышением уровня
    void toggleExercise(int dayIndex, int exIndex);

signals:
    void dataChanged();      // сигнал, что данные изменились (для обновления UI)
    void levelUp(int dayIndex, int newLevel); // для анимации (опционально)

private:
    QVector<WorkoutDay> m_workouts;
    QString m_currentFilePath;

    // Внутренний метод для проверки и повышения уровня
    void checkAndLevelUp(int dayIndex);
};

#endif // GRINDMANAGER_H