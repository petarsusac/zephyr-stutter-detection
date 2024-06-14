# Edge AI Stutter Detection

This is the main repository related to the master's thesis project "System for Diagnosing and Monitoring the Intensity of Stuttering Using Machine Learning on Edge Devices" at UniZg-FER.

The goal of this project is to develop a wearable device to monitor the intensity of stuttering and the level of speech-related anxiety.
The device uses a neural network model to detect stuttering from speech using a microphone, and also monitors biomedical signals using sensors on a custom peripheral unit.

The application is developed using Zephyr RTOS. It targets the STM32F746 MCU.

The other repositories related to this project are:
- [stutter-detection-keras](https://github.com/petarsusac/stutter-detection-keras) - code used for training the neural network model
- [ble-cent](https://github.com/petarsusac/ble_cent) - application for the ESP32 network coprocessor
- [ble_periph](https://github.com/petarsusac/ble_periph) - application for the peripheral unit MCU
