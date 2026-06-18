#include "grindmanager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

GrindManager::GrindManager(QObject *parent) : QObject(parent) {}

// ------------------ ЗАГРУЗКА ------------------
bool GrindManager::loadFromFile(const QString &filePath)
{
    m_currentFilePath = filePath;
    QFile file(filePath);
    if (!file.exists()) {
        qDebug() << "Файл не найден, будут созданы тренировки по умолчанию";
        return false;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Не удалось открыть файл для чтения";
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) return false;

    QJsonArray rootArr = doc.array();
    m_workouts.clear();

    for (const QJsonValue &dayVal : rootArr) {
        QJsonObject dayObj = dayVal.toObject();
        WorkoutDay day;
        day.dayName = dayObj["dayName"].toString();
        day.level = dayObj["level"].toInt();

        QJsonArray exArr = dayObj["exercises"].toArray();
        for (const QJsonValue &exVal : exArr) {
            QJsonObject exObj = exVal.toObject();
            Exercise ex;
            ex.name = exObj["name"].toString();
            ex.isDone = exObj["isDone"].toBool();
            day.exercises.append(ex);
        }
        m_workouts.append(day);
    }
    emit dataChanged();
    return true;
}

// ------------------ СОХРАНЕНИЕ ------------------
bool GrindManager::saveToFile(const QString &filePath)
{
    if (!filePath.isEmpty()) m_currentFilePath = filePath;

    QJsonArray rootArr;
    for (const WorkoutDay &day : m_workouts) {
        QJsonObject dayObj;
        dayObj["dayName"] = day.dayName;
        dayObj["level"] = day.level;

        QJsonArray exArr;
        for (const Exercise &ex : day.exercises) {
            QJsonObject exObj;
            exObj["name"] = ex.name;
            exObj["isDone"] = ex.isDone;
            exArr.append(exObj);
        }
        dayObj["exercises"] = exArr;
        rootArr.append(dayObj);
    }

    QJsonDocument doc(rootArr);
    QFile file(m_currentFilePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Не удалось открыть файл для записи";
        return false;
    }
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

// ------------------ ГЕТТЕРЫ ------------------
QString GrindManager::getDayName(int index) const
{
    if (index < 0 || index >= m_workouts.size()) return QString();
    return m_workouts[index].dayName;
}

int GrindManager::getDayLevel(int index) const
{
    if (index < 0 || index >= m_workouts.size()) return 0;
    return m_workouts[index].level;
}

int GrindManager::getExercisesCount(int dayIndex) const
{
    if (dayIndex < 0 || dayIndex >= m_workouts.size()) return 0;
    return m_workouts[dayIndex].exercises.size();
}

QString GrindManager::getExerciseName(int dayIndex, int exIndex) const
{
    if (dayIndex < 0 || dayIndex >= m_workouts.size()) return QString();
    if (exIndex < 0 || exIndex >= m_workouts[dayIndex].exercises.size()) return QString();
    return m_workouts[dayIndex].exercises[exIndex].name;
}

bool GrindManager::getExerciseDone(int dayIndex, int exIndex) const
{
    if (dayIndex < 0 || dayIndex >= m_workouts.size()) return false;
    if (exIndex < 0 || exIndex >= m_workouts[dayIndex].exercises.size()) return false;
    return m_workouts[dayIndex].exercises[exIndex].isDone;
}

// ------------------ ОПЕРАЦИИ ИЗМЕНЕНИЯ ------------------
void GrindManager::addDay(const QString &name)
{
    WorkoutDay newDay;
    newDay.dayName = name;
    newDay.level = 0;
    m_workouts.append(newDay);
    saveToFile();
    emit dataChanged();
}

void GrindManager::removeDay(int index)
{
    if (index < 0 || index >= m_workouts.size()) return;
    m_workouts.removeAt(index);
    saveToFile();
    emit dataChanged();
}

void GrindManager::renameDay(int index, const QString &newName)
{
    if (index < 0 || index >= m_workouts.size()) return;
    m_workouts[index].dayName = newName;
    saveToFile();
    emit dataChanged();
}

void GrindManager::addExercise(int dayIndex, const QString &name)
{
    if (dayIndex < 0 || dayIndex >= m_workouts.size()) return;
    Exercise newEx;
    newEx.name = name;
    newEx.isDone = false;
    m_workouts[dayIndex].exercises.append(newEx);
    saveToFile();
    emit dataChanged();
}

void GrindManager::removeExercise(int dayIndex, int exIndex)
{
    if (dayIndex < 0 || dayIndex >= m_workouts.size()) return;
    if (exIndex < 0 || exIndex >= m_workouts[dayIndex].exercises.size()) return;
    m_workouts[dayIndex].exercises.removeAt(exIndex);
    saveToFile();
    emit dataChanged();
}

void GrindManager::renameExercise(int dayIndex, int exIndex, const QString &newName)
{
    if (dayIndex < 0 || dayIndex >= m_workouts.size()) return;
    if (exIndex < 0 || exIndex >= m_workouts[dayIndex].exercises.size()) return;
    m_workouts[dayIndex].exercises[exIndex].name = newName;
    saveToFile();
    emit dataChanged();
}

// ------------------ ПЕРЕКЛЮЧЕНИЕ ГАЛОЧКИ (основная магия) ------------------
void GrindManager::toggleExercise(int dayIndex, int exIndex)
{
    if (dayIndex < 0 || dayIndex >= m_workouts.size()) return;
    if (exIndex < 0 || exIndex >= m_workouts[dayIndex].exercises.size()) return;

    // Инвертируем состояние
    m_workouts[dayIndex].exercises[exIndex].isDone =
        !m_workouts[dayIndex].exercises[exIndex].isDone;

    // Проверяем, все ли упражнения в этом дне выполнены
    checkAndLevelUp(dayIndex);

    // Сохраняем и уведомляем
    saveToFile();
    emit dataChanged();
}

void GrindManager::checkAndLevelUp(int dayIndex)
{
    WorkoutDay &day = m_workouts[dayIndex];
    bool allDone = true;
    for (const Exercise &ex : day.exercises) {
        if (!ex.isDone) {
            allDone = false;
            break;
        }
    }

    if (allDone && !day.exercises.isEmpty()) {
        // Повышаем уровень
        day.level += 1;
        // Сбрасываем все галочки
        for (Exercise &ex : day.exercises) {
            ex.isDone = false;
        }
        // Испускаем сигнал для анимации (можно показать всплывашку)
        emit levelUp(dayIndex, day.level);
        qDebug() << "LEVEL UP! День:" << day.dayName << "новый уровень:" << day.level;
    }
}