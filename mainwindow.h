#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QList>
#include <QPoint>
#include <QRect>
#include "grindmanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(GrindManager *manager, QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void onDayChanged(int index);
    void onExerciseButtonClicked();
    void onAddDay();
    void onRemoveDay();
    void onRenameDay();
    void onAddExercise();
    void onRemoveExercise();
    void onRenameExercise();
    void refreshUI();

    void onMinimizeClicked();
    void onMaximizeClicked();
    void onCloseClicked();

private:
    Ui::MainWindow *ui;
    GrindManager *m_manager;
    int m_currentDayIndex = 0;

    QPropertyAnimation *m_progressAnimation;
    QList<QPushButton*> m_exerciseButtons;

    bool m_dragging = false;
    QPoint m_dragStartPos;

    // Для корректного переключения максимизации
    QRect m_normalGeometry;
    bool m_isMaximized = false;

    void updateDayCombo();
    void updateExerciseList();
    void updateProgressAndLevel();
    void animateProgressBar(int targetPercent);
};

#endif // MAINWINDOW_H