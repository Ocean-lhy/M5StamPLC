/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include <Arduino.h>
#include <M5StamPLC.h>

M5Canvas canvas(&M5StamPLC.Display);
M5StamPLC_IO stamplc_io;
uint8_t expected_address      = 0;
uint8_t last_expected_address = 0;

void setup()
{
    /* Init M5StamPLC */
    M5StamPLC.begin();
    canvas.createSprite(M5StamPLC.Display.width(), M5StamPLC.Display.height());
    canvas.setTextScroll(true);
    canvas.fillScreen(TFT_BLACK);
    canvas.setTextSize(1);
    canvas.setFont(&fonts::efontCN_16);
    canvas.println("Try to find M5StamPLC IO");

    /* Init M5StamPLC IO */
    while (!stamplc_io.begin()) {
        canvas.println("M5StamPLC_IO not found, retry in 1s...");
        canvas.pushSprite(0, 0);
        delay(1000);
    }

    last_expected_address = expected_address = stamplc_io.getExpectedAddress();
    canvas.printf("M5StamPLC IO found in 0x%02X\n", stamplc_io.getCurrentAddress());
    canvas.printf("Firmware Version: 0x%02X\n", stamplc_io.getFirmwareVersion());

    uint8_t sys_status = stamplc_io.getSystemStatus();
    if (sys_status == 0) {
        canvas.setTextColor(TFT_GREEN);
        canvas.println("System status: Normal");
    } else {
        canvas.setTextColor(TFT_RED);
        canvas.printf("System status: 0x%02X\n", sys_status);
        if (sys_status & (1 << M5StamPLC_IO::SYS_CH1_INA226_ERROR)) {
            canvas.println("- CH1 INA226 Error");
        }
        if (sys_status & (1 << M5StamPLC_IO::SYS_CH2_INA226_ERROR)) {
            canvas.println("- CH2 INA226 Error");
        }
    }

    canvas.setTextColor(TFT_CYAN);
    canvas.println("INA226 Configuration:");

    uint16_t config_ch1, config_ch2;
    if (stamplc_io.readINA226Config(1, &config_ch1) == ESP_OK) {
        canvas.printf("CH1: 0x%04X", config_ch1);

        uint8_t vshct, vbusct, avg;
        stamplc_io.getINA226ConversionTime(1, &vshct, &vbusct);
        stamplc_io.getINA226Averaging(1, &avg);
        canvas.printf(" VS=%d VB=%d AVG=%d\n", vshct, vbusct, avg);
    }

    if (stamplc_io.readINA226Config(2, &config_ch2) == ESP_OK) {
        canvas.printf("CH2: 0x%04X", config_ch2);

        uint8_t vshct, vbusct, avg;
        stamplc_io.getINA226ConversionTime(2, &vshct, &vbusct);
        stamplc_io.getINA226Averaging(2, &avg);
        canvas.printf(" VS=%d VB=%d AVG=%d\n", vshct, vbusct, avg);
    }

    canvas.setTextColor(TFT_YELLOW);
    canvas.println("press BtnC to start monitoring");
    canvas.pushSprite(0, 0);
    while (!M5StamPLC.BtnC.isPressed()) {
        M5StamPLC.update();
        delay(10);
    }
    canvas.clear();
}

void loop()
{
    M5StamPLC.update();

    static unsigned long last_update = 0;
    if (millis() - last_update > 1000) {
        last_update = millis();

        // Batch read all channels data
        int16_t v1, v2;
        int32_t i1, i2;
        stamplc_io.readAllChannelsData(&v1, &i1, &v2, &i2);

        // Read IO control state
        uint8_t io_control = stamplc_io.readRegister(M5StamPLC_IO::REG_IO_CONTROL);
        uint8_t sys_status = stamplc_io.getSystemStatus();

        // Display data
        canvas.fillScreen(TFT_BLACK);
        canvas.setCursor(0, 0);

        // CH1 status
        if (sys_status & (1 << M5StamPLC_IO::SYS_CH1_INA226_ERROR)) {
            canvas.setTextColor(TFT_RED);
        } else {
            canvas.setTextColor(TFT_GREEN);
        }
        canvas.printf("CH1: %d.%02dV %duA      \n", v1 / 1000, abs((v1 % 1000)) / 10, i1);

        // CH2 status
        if (sys_status & (1 << M5StamPLC_IO::SYS_CH2_INA226_ERROR)) {
            canvas.setTextColor(TFT_RED);
        } else {
            canvas.setTextColor(TFT_GREEN);
        }
        canvas.printf("CH2: %d.%02dV %duA      \n", v2 / 1000, abs((v2 % 1000)) / 10, i2);

        canvas.setTextColor(TFT_YELLOW);
        canvas.printf("Pull-up: CH1=%s CH2=%s      \n",
                      (io_control & (1 << M5StamPLC_IO::BIT_CH1_PU_EN)) ? "ON" : "OFF",
                      (io_control & (1 << M5StamPLC_IO::BIT_CH2_PU_EN)) ? "ON" : "OFF");

        canvas.setTextColor(TFT_MAGENTA);
        canvas.printf("Address: 0x%02X->0x%02X      \n", stamplc_io.getCurrentAddress(),
                      stamplc_io.getExpectedAddress());

        // System status
        if (sys_status == 0) {
            canvas.setTextColor(TFT_GREEN);
            canvas.println("System: Normal          ");
        } else {
            canvas.setTextColor(TFT_RED);
            canvas.printf("System: Error(0x%02X)    \n", sys_status);
        }
    }

    // Button A: Toggle CH1 pull-up
    if (M5StamPLC.BtnA.wasClicked()) {
        stamplc_io.toggleIOBit(M5StamPLC_IO::BIT_CH1_PU_EN);
    }

    // Button B: Toggle CH2 pull-up
    if (M5StamPLC.BtnB.wasClicked()) {
        stamplc_io.toggleIOBit(M5StamPLC_IO::BIT_CH2_PU_EN);
    }

    canvas.pushSprite(0, 0);
    delay(10);
}
