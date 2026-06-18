#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QDebug>
#include <QStatusBar>
#include <QComboBox>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QMouseEvent>
#include <QWindow>
#include <QPixmap>

MainWindow::MainWindow(GrindManager *manager, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_manager(manager)
{
    ui->setupUi(this);

    // --- Убираем стандартную рамку окна ---
    setWindowFlags(Qt::FramelessWindowHint);

    // --- Загружаем иконку для заголовка ---
    QPixmap pixmap("icon.ico");
    if (!pixmap.isNull()) {
        // Масштабируем до 32x32 с сохранением пропорций и сглаживанием
        QPixmap scaled = pixmap.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        ui->iconLabel->setPixmap(scaled);
        ui->iconLabel->setFixedSize(32, 32);
        ui->iconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    } else {
        // Если файл не найден, скрываем лейбл
        ui->iconLabel->hide();
    }

    // --- Подключаем кнопки управления ---
    connect(ui->closeBtn, &QPushButton::clicked, this, &MainWindow::onCloseClicked);
    connect(ui->minimizeBtn, &QPushButton::clicked, this, &MainWindow::onMinimizeClicked);
    connect(ui->maximizeBtn, &QPushButton::clicked, this, &MainWindow::onMaximizeClicked);

    // --- Анимация прогресс-бара ---
    m_progressAnimation = new QPropertyAnimation(ui->progressBar, "value", this);
    m_progressAnimation->setDuration(600);
    m_progressAnimation->setEasingCurve(QEasingCurve::InOutQuad);

    // --- Подключения сигналов ---
    connect(ui->dayCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onDayChanged);

    connect(ui->addDayBtn, &QPushButton::clicked, this, &MainWindow::onAddDay);
    connect(ui->removeDayBtn, &QPushButton::clicked, this, &MainWindow::onRemoveDay);
    connect(ui->renameDayBtn, &QPushButton::clicked, this, &MainWindow::onRenameDay);
    connect(ui->addExBtn, &QPushButton::clicked, this, &MainWindow::onAddExercise);
    connect(ui->removeExBtn, &QPushButton::clicked, this, &MainWindow::onRemoveExercise);
    connect(ui->renameExBtn, &QPushButton::clicked, this, &MainWindow::onRenameExercise);

    // --- Загрузка данных ---
    if (!m_manager->loadFromFile()) {
        m_manager->addDay("Грудь");
        m_manager->addDay("Спина");
        m_manager->addDay("Ноги");
        m_manager->addExercise(0, "Жим лёжа");
        m_manager->addExercise(0, "Отжимания на брусьях");
        m_manager->addExercise(0, "Разводка гантелей");
        m_manager->addExercise(1, "Подтягивания");
        m_manager->addExercise(1, "Тяга штанги");
        m_manager->addExercise(1, "Горизонтальные тяги");
        m_manager->addExercise(2, "Приседания");
        m_manager->addExercise(2, "Выпады");
        m_manager->addExercise(2, "Румынская тяга");
        m_manager->saveToFile();
    }

    connect(m_manager, &GrindManager::dataChanged, this, &MainWindow::refreshUI);
    connect(m_manager, &GrindManager::levelUp, this, [this](int day, int level){
        statusBar()->showMessage(QString("✨ Уровень дня «%1» повышен до %2!").arg(m_manager->getDayName(day)).arg(level), 3000);
    });

    refreshUI();
    if (m_manager->getDaysCount() > 0) {
        ui->dayCombo->setCurrentIndex(0);
        onDayChanged(0);
    }

    // Сохраняем нормальную геометрию
    m_normalGeometry = geometry();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ------------------ СЛОТЫ КНОПОК УПРАВЛЕНИЯ ------------------
void MainWindow::onMinimizeClicked()
{
    showMinimized();
}

void MainWindow::onMaximizeClicked()
{
    if (m_isMaximized) {
        setGeometry(m_normalGeometry);
        m_isMaximized = false;
    } else {
        m_normalGeometry = geometry();
        showMaximized();
        m_isMaximized = true;
    }
}

void MainWindow::onCloseClicked()
{
    close();
}

// ------------------ ПЕРЕТАСКИВАНИЕ ОКНА ------------------
void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (ui->titleBar->geometry().contains(event->pos())) {
            m_dragging = true;
            m_dragStartPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
            event->accept();
        }
    }
    QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging) {
        move(event->globalPosition().toPoint() - m_dragStartPos);
        event->accept();
    }
    QMainWindow::mouseMoveEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
    }
    QMainWindow::mouseReleaseEvent(event);
}

// ------------------ ОСНОВНЫЕ СЛОТЫ ------------------
void MainWindow::onDayChanged(int index)
{
    if (index < 0 || index >= m_manager->getDaysCount()) return;
    m_currentDayIndex = index;
    updateExerciseList();
    updateProgressAndLevel();
}

void MainWindow::onExerciseButtonClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    int idx = m_exerciseButtons.indexOf(btn);
    if (idx == -1) return;
    m_manager->toggleExercise(m_currentDayIndex, idx);
}

void MainWindow::onAddDay()
{
    bool ok;
    QString name = QInputDialog::getText(this, "Новый день",
                                         "Введите название дня:",
                                         QLineEdit::Normal, "", &ok);
    if (ok && !name.isEmpty()) {
        m_manager->addDay(name);
        updateDayCombo();
        int idx = m_manager->getDaysCount() - 1;
        ui->dayCombo->setCurrentIndex(idx);
    }
}

void MainWindow::onRemoveDay()
{
    int idx = ui->dayCombo->currentIndex();
    if (idx < 0 || m_manager->getDaysCount() == 0) return;
    if (QMessageBox::question(this, "Удалить день",
                              "Удалить день «" + m_manager->getDayName(idx) + "»?",
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        m_manager->removeDay(idx);
        updateDayCombo();
        if (m_manager->getDaysCount() > 0) {
            ui->dayCombo->setCurrentIndex(qMin(idx, m_manager->getDaysCount()-1));
        }
    }
}

void MainWindow::onRenameDay()
{
    int idx = ui->dayCombo->currentIndex();
    if (idx < 0) return;
    bool ok;
    QString newName = QInputDialog::getText(this, "Переименовать день",
                                            "Новое название:",
                                            QLineEdit::Normal,
                                            m_manager->getDayName(idx), &ok);
    if (ok && !newName.isEmpty()) {
        m_manager->renameDay(idx, newName);
        updateDayCombo();
        ui->dayCombo->setCurrentIndex(idx);
    }
}

void MainWindow::onAddExercise()
{
    if (m_manager->getDaysCount() == 0) {
        QMessageBox::warning(this, "Нет дня", "Сначала создайте день тренировки.");
        return;
    }
    bool ok;
    QString name = QInputDialog::getText(this, "Новое упражнение",
                                         "Введите название упражнения:",
                                         QLineEdit::Normal, "", &ok);
    if (ok && !name.isEmpty()) {
        m_manager->addExercise(m_currentDayIndex, name);
    }
}

void MainWindow::onRemoveExercise()
{
    int count = m_manager->getExercisesCount(m_currentDayIndex);
    if (count == 0) {
        QMessageBox::information(this, "Нет упражнений", "В этом дне нет упражнений для удаления.");
        return;
    }
    m_manager->removeExercise(m_currentDayIndex, count - 1);
}

void MainWindow::onRenameExercise()
{
    int count = m_manager->getExercisesCount(m_currentDayIndex);
    if (count == 0) {
        QMessageBox::information(this, "Нет упражнений", "Нет упражнений для переименования.");
        return;
    }
    bool ok;
    QString newName = QInputDialog::getText(this, "Переименовать упражнение",
                                            "Новое название для последнего упражнения:",
                                            QLineEdit::Normal,
                                            m_manager->getExerciseName(m_currentDayIndex, count-1), &ok);
    if (ok && !newName.isEmpty()) {
        m_manager->renameExercise(m_currentDayIndex, count-1, newName);
    }
}

// ------------------ ОБНОВЛЕНИЕ UI ------------------
void MainWindow::refreshUI()
{
    updateDayCombo();
    if (m_currentDayIndex >= m_manager->getDaysCount()) {
        m_currentDayIndex = m_manager->getDaysCount() - 1;
    }
    if (m_currentDayIndex < 0 && m_manager->getDaysCount() > 0) {
        m_currentDayIndex = 0;
    }
    if (m_manager->getDaysCount() > 0 && m_currentDayIndex >= 0) {
        ui->dayCombo->blockSignals(true);
        ui->dayCombo->setCurrentIndex(m_currentDayIndex);
        ui->dayCombo->blockSignals(false);
        updateExerciseList();
        updateProgressAndLevel();
    } else {
        qDeleteAll(m_exerciseButtons);
        m_exerciseButtons.clear();
        QLayout *layout = ui->scrollAreaWidgetContents->layout();
        if (layout) {
            QLayoutItem *child;
            while ((child = layout->takeAt(0)) != nullptr) {
                delete child->widget();
                delete child;
            }
        }
        animateProgressBar(0);
        ui->levelLabel->setText("0");
    }
}

void MainWindow::updateDayCombo()
{
    ui->dayCombo->blockSignals(true);
    ui->dayCombo->clear();
    for (int i = 0; i < m_manager->getDaysCount(); ++i) {
        ui->dayCombo->addItem(m_manager->getDayName(i));
    }
    ui->dayCombo->blockSignals(false);
}

void MainWindow::updateExerciseList()
{
    qDeleteAll(m_exerciseButtons);
    m_exerciseButtons.clear();

    QLayout *layout = ui->scrollAreaWidgetContents->layout();
    if (!layout) return;

    QLayoutItem *child;
    while ((child = layout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    int dayIdx = m_currentDayIndex;
    if (dayIdx < 0 || dayIdx >= m_manager->getDaysCount()) return;

    int count = m_manager->getExercisesCount(dayIdx);
    for (int i = 0; i < count; ++i) {
        QString name = m_manager->getExerciseName(dayIdx, i);
        bool done = m_manager->getExerciseDone(dayIdx, i);

        QPushButton *btn = new QPushButton(name);
        btn->setProperty("class", "exercise");
        btn->setMinimumWidth(250);
        btn->setMaximumWidth(400);
        btn->setFixedHeight(44);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        btn->setEnabled(!done);

        connect(btn, &QPushButton::clicked, this, &MainWindow::onExerciseButtonClicked);

        layout->addWidget(btn);
        layout->setAlignment(btn, Qt::AlignHCenter);

        m_exerciseButtons.append(btn);
    }
}

void MainWindow::updateProgressAndLevel()
{
    int dayIdx = m_currentDayIndex;
    if (dayIdx < 0 || dayIdx >= m_manager->getDaysCount()) {
        animateProgressBar(0);
        ui->levelLabel->setText("0");
        return;
    }

    int total = m_manager->getExercisesCount(dayIdx);
    int done = 0;
    for (int i = 0; i < total; ++i) {
        if (m_manager->getExerciseDone(dayIdx, i)) done++;
    }

    int percent = (total == 0) ? 0 : (done * 100 / total);
    animateProgressBar(percent);
    ui->levelLabel->setText(QString::number(m_manager->getDayLevel(dayIdx)));
}

void MainWindow::animateProgressBar(int targetPercent)
{
    m_progressAnimation->stop();
    int current = ui->progressBar->value();
    m_progressAnimation->setStartValue(current);
    m_progressAnimation->setEndValue(targetPercent);
    m_progressAnimation->start();
}