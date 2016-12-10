﻿// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>
#include <utility>
#include <QTimer>
#include "citra_qt/configure_input.h"

ConfigureInput::ConfigureInput(QWidget* parent)
    : QWidget(parent), ui(std::make_unique<Ui::ConfigureInput>()) {

    ui->setupUi(this);

    // Initialize mapping of input enum to UI button.
    input_mapping = {
        {Settings::NativeInput::Values::A, ui->buttonA},
        {Settings::NativeInput::Values::B, ui->buttonB},
        {Settings::NativeInput::Values::X, ui->buttonX},
        {Settings::NativeInput::Values::Y, ui->buttonY},
        {Settings::NativeInput::Values::L, ui->buttonL},
        {Settings::NativeInput::Values::R, ui->buttonR},
        {Settings::NativeInput::Values::ZL, ui->buttonZL},
        {Settings::NativeInput::Values::ZR, ui->buttonZR},
        {Settings::NativeInput::Values::START, ui->buttonStart},
        {Settings::NativeInput::Values::SELECT, ui->buttonSelect},
        {Settings::NativeInput::Values::HOME, ui->buttonHome},
        {Settings::NativeInput::Values::DUP, ui->buttonDpadUp},
        {Settings::NativeInput::Values::DDOWN, ui->buttonDpadDown},
        {Settings::NativeInput::Values::DLEFT, ui->buttonDpadLeft},
        {Settings::NativeInput::Values::DRIGHT, ui->buttonDpadRight},
        {Settings::NativeInput::Values::CUP, ui->buttonCStickUp},
        {Settings::NativeInput::Values::CDOWN, ui->buttonCStickDown},
        {Settings::NativeInput::Values::CLEFT, ui->buttonCStickLeft},
        {Settings::NativeInput::Values::CRIGHT, ui->buttonCStickRight},
        {Settings::NativeInput::Values::CIRCLE_UP, ui->buttonCircleUp},
        {Settings::NativeInput::Values::CIRCLE_DOWN, ui->buttonCircleDown},
        {Settings::NativeInput::Values::CIRCLE_LEFT, ui->buttonCircleLeft},
        {Settings::NativeInput::Values::CIRCLE_RIGHT, ui->buttonCircleRight},
        {Settings::NativeInput::Values::CIRCLE_MODIFIER, ui->buttonCircleMod},
    };

    // Attach handle click method to each button click.
    for (const auto& entry : input_mapping) {
        connect(entry.second, SIGNAL(released()), this, SLOT(handleClick()));
    }
    connect(ui->buttonRestoreDefaults, SIGNAL(released()), this, SLOT(restoreDefaults()));
    setFocusPolicy(Qt::ClickFocus);
    timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, [&]() {
        key_pressed = Qt::Key_Escape;
        setKey();
    });
    this->setConfiguration();
}

void ConfigureInput::handleClick() {
    QPushButton* sender = qobject_cast<QPushButton*>(QObject::sender());
    QVariant key_variant = sender->property("key");
    previous_mapping = key_variant.value<QString>();
    sender->setText(tr("[waiting]"));
    sender->setFocus();
    grabKeyboard();
    grabMouse();
    changing_button = sender;
    timer->start(5000); // Cancel after 5 seconds
}

void ConfigureInput::applyConfiguration() {
    for (int i = 0; i < Settings::NativeInput::NUM_INPUTS; ++i) {
        QVariant key_variant = input_mapping[Settings::NativeInput::Values(i)]->property("key");
        int value = getKeyValue(key_variant.value<QString>());
        Settings::values.input_mappings[Settings::NativeInput::All[i]] = value;
    }
    Settings::Apply();
}

void ConfigureInput::setConfiguration() {
    for (int i = 0; i < Settings::NativeInput::NUM_INPUTS; ++i) {
        QString keyValue = getKeyName(Settings::values.input_mappings[i]);
        input_mapping[Settings::NativeInput::Values(i)]->setText(keyValue);
        input_mapping[Settings::NativeInput::Values(i)]->setProperty("key", keyValue);
    }
}

void ConfigureInput::keyPressEvent(QKeyEvent* event) {
    if (!changing_button)
        return;
    if (!event || event->key() == Qt::Key_unknown)
        return;
    key_pressed = event->key();
    timer->stop();
    setKey();
}

void ConfigureInput::setKey() {
    const QString key_value = getKeyName(key_pressed);
    if (key_pressed == Qt::Key_Escape) {
        changing_button->setText(previous_mapping);
        changing_button->setProperty("key", previous_mapping);
    } else {
        changing_button->setText(key_value);
        changing_button->setProperty("key", key_value);
    }
    removeDuplicates(key_value);
    key_pressed = Qt::Key_unknown;
    releaseKeyboard();
    releaseMouse();
    changing_button = nullptr;
    previous_mapping = nullptr;
}

QString ConfigureInput::getKeyName(int key_code) const {
    if (key_code == Qt::Key_Shift)
        return tr("Shift");
    if (key_code == Qt::Key_Control)
        return tr("Ctrl");
    if (key_code == Qt::Key_Alt)
        return tr("Alt");
    if (key_code == Qt::Key_Meta)
        return "";
    if (key_code == -1)
        return "";

    return QKeySequence(key_code).toString();
}

Qt::Key ConfigureInput::getKeyValue(const QString& text) const {
    if (text == "Shift")
        return Qt::Key_Shift;
    if (text == "Ctrl")
        return Qt::Key_Control;
    if (text == "Alt")
        return Qt::Key_Alt;
    if (text == "Meta")
        return Qt::Key_unknown;
    if (text == "")
        return Qt::Key_unknown;

    return Qt::Key(QKeySequence(text)[0]);
}

void ConfigureInput::removeDuplicates(const QString& newValue) {
    for (int i = 0; i < Settings::NativeInput::NUM_INPUTS; ++i) {
        if (changing_button != input_mapping[Settings::NativeInput::Values(i)]) {
            QVariant key_variant = input_mapping[Settings::NativeInput::Values(i)]->property("key");
            const QString oldValue = key_variant.value<QString>();
            if (newValue == oldValue) {
                input_mapping[Settings::NativeInput::Values(i)]->setText("");
                input_mapping[Settings::NativeInput::Values(i)]->setProperty("key", "");
            }
        }
    }
}

void ConfigureInput::restoreDefaults() {
    for (int i = 0; i < Settings::NativeInput::NUM_INPUTS; ++i) {
        const QString keyValue = getKeyName(Config::defaults[i].toInt());
        input_mapping[Settings::NativeInput::Values(i)]->setText(keyValue);
        input_mapping[Settings::NativeInput::Values(i)]->setProperty("key", keyValue);
    }
}
