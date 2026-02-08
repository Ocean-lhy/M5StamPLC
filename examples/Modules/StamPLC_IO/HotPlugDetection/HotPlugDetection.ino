/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 *
 * M5StamPLC IO Hot-Plug Detection Example
 *
 * This example demonstrates hot-plug detection for multiple M5StamPLC IO modules:
 * - Automatically scan I2C bus and detect connected modules (0x20-0x2F)
 * - Display online modules list
 * - Real-time detection of module insertion and removal
 */
#include <Arduino.h>
#include <M5StamPLC.h>

#define MAX_MODULES      16
#define SCAN_INTERVAL_MS 2000

struct ModuleInfo {
    uint8_t address;
    uint8_t firmware_version;
    bool connected;
};

ModuleInfo modules[MAX_MODULES];
uint8_t module_count         = 0;
unsigned long last_scan_time = 0;

void initModules()
{
    for (int i = 0; i < MAX_MODULES; i++) {
        modules[i].address          = 0;
        modules[i].firmware_version = 0;
        modules[i].connected        = false;
    }
    module_count = 0;
}

int findModuleSlot(uint8_t addr)
{
    for (int i = 0; i < MAX_MODULES; i++) {
        if (modules[i].address == addr && modules[i].connected) {
            return i;
        }
    }
    return -1;
}

int getEmptySlot()
{
    for (int i = 0; i < MAX_MODULES; i++) {
        if (!modules[i].connected) {
            return i;
        }
    }
    return -1;
}

void scanModules()
{
    uint8_t detected[16];
    uint8_t detected_count = 0;

    for (uint8_t addr = M5StamPLC_IO::I2C_ADDR_MIN; addr <= M5StamPLC_IO::I2C_ADDR_MAX; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            detected[detected_count++] = addr;

            int slot = findModuleSlot(addr);
            if (slot == -1) {
                slot = getEmptySlot();
                if (slot != -1) {
                    M5StamPLC_IO temp_device;
                    if (temp_device.begin(addr, false)) {
                        modules[slot].address          = addr;
                        modules[slot].firmware_version = temp_device.getFirmwareVersion();
                        modules[slot].connected        = true;
                        module_count++;
                        Serial.printf("[+] Module connected: 0x%02X (FW: 0x%02X)\n", addr,
                                      modules[slot].firmware_version);
                    }
                }
            }
        }
    }

    for (int i = 0; i < MAX_MODULES; i++) {
        if (modules[i].connected) {
            bool still_present = false;
            for (int j = 0; j < detected_count; j++) {
                if (detected[j] == modules[i].address) {
                    still_present = true;
                    break;
                }
            }

            if (!still_present) {
                Serial.printf("[-] Module disconnected: 0x%02X\n", modules[i].address);
                modules[i].connected = false;
                modules[i].address   = 0;
                module_count--;
            }
        }
    }
}

void displayModules()
{
    M5StamPLC.Display.fillScreen(TFT_BLACK);
    M5StamPLC.Display.setCursor(0, 0);

    M5StamPLC.Display.setTextColor(TFT_GREENYELLOW);
    M5StamPLC.Display.println("=== IO Hot-Plug ===");

    M5StamPLC.Display.setTextColor(TFT_CYAN);
    M5StamPLC.Display.printf("Online: %d\n\n", module_count);

    if (module_count == 0) {
        M5StamPLC.Display.setTextColor(TFT_ORANGE);
        M5StamPLC.Display.println("No modules found");
    } else {
        int displayed = 0;
        for (int i = 0; i < MAX_MODULES && displayed < 8; i++) {
            if (modules[i].connected) {
                M5StamPLC.Display.setTextColor(TFT_GREEN);
                M5StamPLC.Display.printf("%d. Addr: 0x%02X\n", displayed + 1, modules[i].address);
                M5StamPLC.Display.setTextColor(TFT_YELLOW);
                M5StamPLC.Display.printf("   FW: 0x%02X\n", modules[i].firmware_version);
                displayed++;
            }
        }
    }
}

void setup()
{
    M5StamPLC.begin();
    M5StamPLC.Display.setTextScroll(false);
    M5StamPLC.Display.setTextSize(1);
    M5StamPLC.Display.setFont(&fonts::efontCN_16);

    initModules();

    M5StamPLC.Display.fillScreen(TFT_BLACK);
    M5StamPLC.Display.setCursor(0, 0);
    M5StamPLC.Display.setTextColor(TFT_GREENYELLOW);
    M5StamPLC.Display.println("M5StamPLC IO");
    M5StamPLC.Display.println("Hot-Plug Detection");
    M5StamPLC.Display.setTextColor(TFT_CYAN);
    M5StamPLC.Display.println("\nScanning...");

    delay(1000);
    scanModules();
    displayModules();
    last_scan_time = millis();
}

void loop()
{
    M5StamPLC.update();

    if (millis() - last_scan_time > SCAN_INTERVAL_MS) {
        scanModules();
        displayModules();
        last_scan_time = millis();
    }

    delay(100);
}
