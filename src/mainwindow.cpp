#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QFile>
#include <QTextStream>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow), _framePositionValidator(QRegularExpression("\\d{4}")),
    _groupBoxEnabledTitle("QGroupBox::title { color: black; }"), _groupBoxDisabledTitle("QGroupBox::title { color: gray; }"),
    _filePath(QString(PROJECT_PATH).append("/resource/frame_model.fm"))
{
    ui->setupUi(this);
    Init();
    LoadFromFile();
}

void MainWindow::Init() {
    ui->xFrame->setValidator(&_framePositionValidator);
    ui->yFrame->setValidator(&_framePositionValidator);
    ui->xNewFrame->setValidator(&_framePositionValidator);
    ui->yNewFrame->setValidator(&_framePositionValidator);
    ui->slotTypeGroupBox->setStyleSheet(_groupBoxDisabledTitle);

    ui->slotFrames->setModel(&_slotFramesModel);
    ui->targetFrames->setModel(&_targetFramesModel);
    ui->framesToEdit->setModel(&_framesToEditModel);

    for (auto slotTypeButton : {ui->slotRegularType, ui->slotFrameType}) {
        connect(slotTypeButton, &QRadioButton::clicked, this, [=]() {
            const auto isSlotRegular = slotTypeButton == ui->slotRegularType;

            ui->slotNameValueLabel->setEnabled(isSlotRegular);
            ui->slotName->setEnabled(isSlotRegular);
            ui->slotValue->setEnabled(isSlotRegular);

            ui->slotFrameLabel->setEnabled(!isSlotRegular);
            ui->slotFrames->setEnabled(!isSlotRegular);

            ResetSlotInfo();
        });
    }

    connect(ui->framesToEdit, &QComboBox::currentTextChanged, this, [=](const QString& editableFrameName) {
        UpdateEditableSlotsOfFrame(editableFrameName);
    });

    connect(ui->editableSlotsOfEditableFrame, &QComboBox::currentTextChanged, this, [=](const QString& editableSlotName) {
        ui->editableSlotType->clear();
        ui->needsToChangedSlotValue->setChecked(false);
        ui->newSlotName->clear();
        ui->newValueOfRegularSlot->clear();

        if (!editableSlotName.isEmpty()) {
            const auto& editableFrameSlots = ui->frameModel->At(ui->framesToEdit->currentText()).GetSlots();

            if (std::holds_alternative<QString>(editableFrameSlots.at(editableSlotName))) {
                ui->editableSlotType->setText("?????????????? ????????");
                ui->slotEditInfoGroupBox->setEnabled(true);
            }
            else {
                ui->editableSlotType->setText("??????????");
                ui->slotEditInfoGroupBox->setEnabled(false);
            }
        }
    });
}

void MainWindow::on_addFrame_clicked() {
    const auto frameName = ui->frameName->text();
    const auto xFrame = ui->xFrame->text();
    const auto yFrame = ui->yFrame->text();

    if (frameName.isEmpty() || xFrame.isEmpty() || yFrame.isEmpty()) {
        QMessageBox::critical(nullptr, "???????????? ?????? ???????????????????? ????????????", "?????? ???????? ?????? ???????????????????? ???????????? ???????????? ???????? ??????????????????");
        return;
    }

    if (ui->frameModel->Contains(frameName)) {
        QMessageBox::critical(nullptr, "???????????? ?????? ???????????????????? ????????????", "?????????? \"" + frameName + "\" ?????? ????????????????????");
        return;
    }

    Frame frame(frameName);
    QPoint framePosition(QPoint(xFrame.toInt(), yFrame.toInt()));

    const auto* addedFrame = ui->frameModel->AddFrame(std::move(frame), framePosition);
    ui->frameModel->update();

    _slotFramesModel.AddFrame(addedFrame);
    _targetFramesModel.AddFrame(addedFrame);
    _framesToEditModel.AddFrame(addedFrame);

    ui->addSlotGroupBox->setEnabled(true);
    ui->editFrameGroupBox->setEnabled(true);
    ui->slotTypeGroupBox->setEnabled(true);
    ui->slotTypeGroupBox->setStyleSheet(_groupBoxEnabledTitle);

    ResetFrameInfo();
}

void MainWindow::on_addSlot_clicked() {
    auto& frameModel = *ui->frameModel;
    auto& targetFrame = frameModel.At(ui->targetFrames->currentText());

    if (ui->slotRegularType->isChecked()) {
        const auto slotName = ui->slotName->text();
        const auto slotValue = ui->slotValue->text().isEmpty() ? "????????????????" : ui->slotValue->text();

        if (slotName.isEmpty()) {
            QMessageBox::critical(nullptr, "???????????? ?????? ???????????????????? ???????????????? ??????????", "?????? ?????????? ???????????? ???????? ??????????????????");
            return;
        }

        if (targetFrame.GetName() == slotName) {
            QMessageBox::critical(nullptr, "???????????? ?????? ???????????????????? ???????????????? ??????????", "?????????? ???? ?????????? ?????????????????? ?????????????????????? ????????");
            return;
        }

        if (targetFrame.Contains(slotName)) {
            QMessageBox::critical(nullptr, "???????????? ?????? ???????????????????? ???????????????? ??????????",
                                  "?????????? \"" + targetFrame.GetName() + "\" ?????? ???????????????? ???????? \"" + slotName + "\"");
            return;
        }

        targetFrame.AddSlot(std::move(slotName), slotValue.isEmpty() ? "????????????????" : std::move(slotValue));
    }
    else {
        const auto& slotFrame = frameModel.At(ui->slotFrames->currentText());

        if (targetFrame.GetName() == slotFrame.GetName()) {
            QMessageBox::critical(nullptr, "???????????? ?????? ???????????????????? ??????????-????????????", "?????????? ???? ?????????? ?????????????????? ?????????????????????? ????????");
            return;
        }

        if (targetFrame.Contains(slotFrame.GetName())) {
            QMessageBox::critical(nullptr, "???????????? ?????? ???????????????????? ??????????-????????????",
                                  "?????????? \"" + targetFrame.GetName() + "\" ?????? ???????????????? ???????? \"" + slotFrame.GetName() + "\"");
            return;
        }

        targetFrame.AddSlot(&slotFrame);
    }

    if (targetFrame.GetName() == ui->framesToEdit->currentText()) {
        UpdateEditableSlotsOfFrame(targetFrame.GetName());
    }

    ui->frameModel->update();
    ResetSlotInfo();
}

void MainWindow::on_editFrame_clicked() {
    auto newFrameName = ui->newFrameName->text();
    ui->frameModel->ReplaceFrameCoords(ui->framesToEdit->currentText(), ui->xNewFrame->text(), ui->yNewFrame->text());
    ui->xNewFrame->clear();
    ui->yNewFrame->clear();

    if (!newFrameName.isEmpty()) {
        if (ui->frameModel->Contains(newFrameName)) {
            QMessageBox::critical(nullptr, "???????????? ?????? ???????????????????????????? ????????????", "?????????? \"" + newFrameName + "\" ?????? ????????????????????");
            return;
        }

        auto& frame = ui->frameModel->At(ui->framesToEdit->currentText());

        if (frame.Contains(newFrameName)) {
            QMessageBox::critical(nullptr, "???????????? ?????? ???????????????????????????? ????????????",
                                  "???? ???????????? \"" + frame.GetName() + "\" ?????? ???????????????????? ???????? ?? ???????????? \"" + newFrameName + "\"");
            return;
        }

        ui->frameModel->ReplaceFrameName(ui->framesToEdit->currentText(), std::move(newFrameName));
        ui->slotFrames->update();
        ui->targetFrames->update();
        ui->framesToEdit->update();
        ui->newFrameName->clear();
    }

    ui->frameModel->update();
}

void MainWindow::on_deleteFrame_clicked() {
    const auto currentEditableFrameName = ui->framesToEdit->currentText();
    const auto& frame = ui->frameModel->At(currentEditableFrameName);

    ui->frameModel->EraseFrame(currentEditableFrameName);
    ui->frameModel->update();
    ui->newFrameName->clear();

    _slotFramesModel.EraseFrame(&frame);
    _targetFramesModel.EraseFrame(&frame);
    _framesToEditModel.EraseFrame(&frame);

    if (ui->frameModel->IsEmpty()) {
        ui->addSlotGroupBox->setEnabled(false);
        ui->editFrameGroupBox->setEnabled(false);
        ui->slotTypeGroupBox->setEnabled(false);
        ui->slotTypeGroupBox->setStyleSheet(_groupBoxDisabledTitle);
    }
}

void MainWindow::on_editSlot_clicked() {
    auto currentSlotName = ui->editableSlotsOfEditableFrame->currentText();

    // ???????? ?? ???????????????????????????? ???????????? ???????? ?????????? (?? ?????????? ???????????? ?? ???????????????????? ?????????? ????????????????)
    if (!currentSlotName.isEmpty()) {
        // ?????????? ?????????????? ?? ui->needsToChangedSlotValue ?????? ?????????????????? ?????????? ?????????? ???? ?????????????????? (???????????? ?????? ???????????????????? ????????????
        // currentTextChanged, ?????????????? ?????????????????? ?? connect'??, ???????????????????????? ?? ???????????? Init, ?????????????? ???????????? ??????????????)
        ui->editableSlotsOfEditableFrame->blockSignals(true);

        auto newSlotName = ui->newSlotName->text();
        auto& editableFrame = ui->frameModel->At(ui->framesToEdit->currentText());

        // ?? ???????????????? ReplaceSlotName ?? ReplaceSlotValue ?????????? ???????????????????? ?? ???????? ??????????
        if (!newSlotName.isEmpty()) {
            if (ui->frameModel->Contains(newSlotName)) {
                QMessageBox::critical(nullptr, "???????????? ?????? ???????????????????????????? ??????????", "?????????? \"" + newSlotName + "\" ?????? ????????????????????");
                return;
            }

            if (editableFrame.Contains(newSlotName)) {
                QMessageBox::critical(nullptr, "???????????? ?????? ???????????????????????????? ??????????",
                                      "?????????? \"" + editableFrame.GetName() + "\" ?????? ???????????????? ???????? \"" + newSlotName + "\"");
                return;
            }

            editableFrame.ReplaceSlotName(currentSlotName, newSlotName);
            currentSlotName = std::move(newSlotName);
            ui->editableSlotsOfEditableFrame->setItemText(ui->editableSlotsOfEditableFrame->currentIndex(), currentSlotName);
        }

        if (ui->needsToChangedSlotValue->isChecked()) {
            auto newSlotValue = ui->newValueOfRegularSlot->text();
            editableFrame.ReplaceSlotValue(currentSlotName, newSlotValue.isEmpty() ? "????????????????" : std::move(newSlotValue));
        }

        ui->needsToChangedSlotValue->setChecked(false);
        ui->newSlotName->clear();
        ui->newValueOfRegularSlot->clear();
        ui->frameModel->update();
        ui->editableSlotsOfEditableFrame->blockSignals(false);
    }
}

void MainWindow::on_deleteSlot_clicked() {
    auto currentSlotName = ui->editableSlotsOfEditableFrame->currentText();

    // ???????? ?? ???????????????????????????? ???????????? ???????? ?????????? (?? ?????????? ???????????? ?? ???????????????????? ?????????? ????????????????)
    if (!currentSlotName.isEmpty()) {
        auto& editableFrame = ui->frameModel->At(ui->framesToEdit->currentText());
        editableFrame.EraseSlot(currentSlotName);
        ui->editableSlotsOfEditableFrame->removeItem(ui->editableSlotsOfEditableFrame->currentIndex());
        ui->frameModel->update();
    }
}

void MainWindow::on_syntaxSearch_clicked() {
    const auto syntaxSearchSlotNamesText = ui->syntaxSearchSlotNames->text();

    if (syntaxSearchSlotNamesText.isEmpty()) {
        QMessageBox::critical(nullptr, "???????????? ?????????????????????????????? ????????????", "?????????????? ?????????? ???????????? ?????????? ?????????? ?? ?????????????? ?????? ?????????????????????????????? ????????????");
        return;
    }

    const auto syntaxSearchSlotNames = syntaxSearchSlotNamesText.split(';');
    const auto syntaxSearchResult = ui->frameModel->SyntaxSearch(syntaxSearchSlotNames);

    QMessageBox::information(nullptr, "?????????????????? ?????????????????????????????? ????????????", syntaxSearchResult);
}

void MainWindow::on_semanticSearch_clicked() {
    const auto semanticSearchSlotValuesText = ui->semanticSearchSlotValues->text();

    if (semanticSearchSlotValuesText.isEmpty()) {
        QMessageBox::critical(nullptr, "???????????? ???????????????????????????? ????????????", "?????????????? ???????????????? ???????????? ?????????? ?????????? ?? ?????????????? ?????? ???????????????????????????? ????????????");
        return;
    }

    const auto semanticSearchSlotValues = semanticSearchSlotValuesText.split(';');
    const auto semanticSearchResult = ui->frameModel->SemanticSearch(semanticSearchSlotValues);

    QMessageBox::information(nullptr, "?????????????????? ???????????????????????????? ????????????", semanticSearchResult);
}

void MainWindow::ResetFrameInfo() {
    ui->frameName->clear();
    ui->xFrame->clear();
    ui->yFrame->clear();
}

void MainWindow::ResetSlotInfo() {
    ui->slotName->clear();
    ui->slotValue->clear();
    ui->slotFrames->setCurrentIndex(0);
    ui->targetFrames->setCurrentIndex(0);
}

void MainWindow::UpdateEditableSlotsOfFrame(const QString& editableFrameName) {
    ui->editableSlotsOfEditableFrame->clear();

    if (!editableFrameName.isEmpty()) {
        for (const auto& [slotFrameName, _] : ui->frameModel->At(editableFrameName).GetSlots()) {
            ui->editableSlotsOfEditableFrame->addItem(slotFrameName);
        }
    }
}

void MainWindow::LoadFromFile() {
/* |  0  |    1   |  2 | 3 |    <--- ?????????????? ?? ???????????? ?? ????????????
 *  ?????????? ???????????????? 1125 150     <--- ?????? ???????????????? ?? ?????????? ???????????? ?? ????????????
 */

/* |  0 |   1   |    2   |      3     |      4      |    5   |  <--- ?????????????? ?? ???????????? ?? ??????????
 *  ???????? ?????????????? ???????????????? ??????????-???????????? ??????????????_?????????? ????????????????   <--- ?????? ???????????????? ?? ?????????? ???????????? ?? ??????????-????????????
 *
 * |  0 |     1    |    2   |  3 |      4      |                     5                    |  <--- ?????????????? ?? ???????????? ?? ??????????
 *  ???????? GPS-???????????? ???????????????? ???????? ??????????????_?????????? ????????????????????_??????_??????????????????_????????????????????_????????????   <--- ?????? ???????????????? ?? ?????????? ???????????? ???? ?????????????? ??????????
 */

    QFile file(_filePath);

    if (file.open(QFile::ReadOnly)) {
        QTextStream in(&file);

        while (!in.atEnd()) {
            QString line = in.readLine();
            QStringList splitLine = line.split(' ');

            if (splitLine[0] == "??????????") {
                Frame frame(splitLine[1].replace('_', ' '));
                const auto* addedFrame = ui->frameModel->AddFrame(std::move(frame), QPoint(splitLine[2].toInt(), splitLine[3].toInt()));

                _slotFramesModel.AddFrame(addedFrame);
                _targetFramesModel.AddFrame(addedFrame);
                _framesToEditModel.AddFrame(addedFrame);
            }
            else if (splitLine[0] == "????????"){
                if (splitLine[3] == "??????????-????????????") {
                    const auto& frameSlot = ui->frameModel->At(splitLine[1].replace('_', ' '));
                    ui->frameModel->At(splitLine[5].replace('_', ' ')).AddSlot(&frameSlot);
                }
                else {
                    ui->frameModel->At(splitLine[5].replace('_', ' ')).AddSlot(splitLine[1].replace('_', ' '), splitLine[3].replace('_', ' '));
                }
            }
        }

        file.close();

        if (!ui->frameModel->IsEmpty()) {
            ui->addSlotGroupBox->setEnabled(true);
            ui->editFrameGroupBox->setEnabled(true);
            ui->slotTypeGroupBox->setEnabled(true);
            ui->slotTypeGroupBox->setStyleSheet(_groupBoxEnabledTitle);
            UpdateEditableSlotsOfFrame(ui->framesToEdit->currentText());
        }
    }
}

void MainWindow::SaveToFile() {
    QFile file(_filePath);
    file.open(QFile::WriteOnly);
    QTextStream out(&file);

    // ?????????????? ???????????????????? ???????????? ???????? ??????????????
    for (const auto& [frameName, frameWithPosition] : ui->frameModel->GetFrames()) {
        out << QString::fromUtf8("?????????? ") << QString(frameName).replace(' ', '_') << " " <<
               QString::number(frameWithPosition.second.x()) << ' ' << QString::number(frameWithPosition.second.y()) << '\n';
    }

    // ???????????????????? ???????? ???????????? ???????? ??????????????
    for (const auto& [frameName, frameWithPosition] : ui->frameModel->GetFrames()) {
        for (const auto& [slotName, frameSlot] : frameWithPosition.first.GetSlots()) {
            QString slotValue;

            if (std::holds_alternative<QString>(frameSlot))
                slotValue = QString(std::get<QString>(frameSlot)).replace(' ', '_');
            else
                slotValue = QString::fromUtf8("??????????-????????????");

            out << QString::fromUtf8("???????? ") << QString(slotName).replace(' ', '_') <<
                   QString::fromUtf8(" ???????????????? ") << slotValue <<
                   QString::fromUtf8(" ??????????????_?????????? ") << QString(frameName).replace(' ', '_') << '\n';
        }
    }

    file.close();
}

MainWindow::~MainWindow() {
    SaveToFile();
    delete ui;
}
