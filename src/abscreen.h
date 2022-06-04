#pragma once
#include <Arduino.h>
#include <power.h>
#include <Wire.h>
#include <OLEDDisplayUi.h>
#include <SSD1306Wire.h>
#include <string>
#include "concurrency/PeriodicTask.h"
#include "TypedQueue.h"
#include "concurrency/LockGuard.h"
#include "abglobal.h"
#include "screen_common.h"

class Screen : public concurrency::PeriodicTask
{
protected:
    FrameCallback m_normalFrame;
    abGlobal &m_global;
    CallbackObserver<Screen, const Status *> powerStatusObserver = CallbackObserver<Screen, const Status *>(this, &Screen::handleStatusUpdate);
    CallbackObserver<Screen, const Status *> gpsStatusObserver = CallbackObserver<Screen, const Status *>(this, &Screen::handleStatusUpdate);
  public:
    concurrency::Lock m_screenItemsLock;
    cScreenItems m_items;
    cScreenItems m_overlayItems;
    
    Screen(abGlobal &global, uint8_t address, int sda = -1, int scl = -1);

    Screen(const Screen &) = delete;
    Screen &operator=(const Screen &) = delete;

    abGlobal &getGlobal() { return m_global; }
    void setup();

    /// Turns the screen on/off.
    void setOn(bool on)
    {
        if (!on)
            handleSetOn(
                false); // We handle off commands immediately, because they might be called because the CPU is shutting down
        else
            enqueueCmd(CmdItem{.cmd = on ? Cmd::SET_ON : Cmd::SET_OFF});
    }

    // Implementation to Adjust Brightness
    void adjustBrightness();
    uint8_t brightness = 150;

    /// Starts showing the Bluetooth PIN screen.
    //
    // Switches over to a static frame showing the Bluetooth pairing screen
    // with the PIN.
    void startBluetoothPinScreen(uint32_t pin)
    {
        CmdItem cmd;
        cmd.cmd = Cmd::START_BLUETOOTH_PIN_SCREEN;
        cmd.bluetooth_pin = pin;
        enqueueCmd(cmd);
    }

    /// Stops showing the bluetooth PIN screen.
    void stopBluetoothPinScreen() { enqueueCmd(CmdItem{.cmd = Cmd::STOP_BLUETOOTH_PIN_SCREEN}); }

    /// Stops showing the boot screen.
    void stopBootScreen() { enqueueCmd(CmdItem{.cmd = Cmd::STOP_BOOT_SCREEN}); }

    /// Writes a string to the screen.
    void print(const char *text)
    {
        CmdItem cmd;
        cmd.cmd = Cmd::PRINT;
        // TODO(girts): strdup() here is scary, but we can't use std::string as
        // FreeRTOS queue is just dumbly copying memory contents. It would be
        // nice if we had a queue that could copy objects by value.
        cmd.print_text = strdup(text);
        if (!enqueueCmd(cmd)) {
            free(cmd.print_text);
        }
    }

    /// Overrides the default utf8 character conversion, to replace empty space with question marks
    static char customFontTableLookup(const uint8_t ch) {
        // UTF-8 to font table index converter
        // Code form http://playground.arduino.cc/Main/Utf8ascii
        static uint8_t LASTCHAR;
        static bool SKIPREST;   // Only display a single unconvertable-character symbol per sequence of unconvertable characters

        if (ch < 128) { // Standard ASCII-set 0..0x7F handling
            LASTCHAR = 0;
            SKIPREST = false;
            return ch;
        }

        uint8_t last = LASTCHAR;   // get last char
        LASTCHAR = ch;

        switch (last) {    // conversion depnding on first UTF8-character
            case 0xC2: { SKIPREST = false; return (uint8_t) ch; }
            case 0xC3: { SKIPREST = false; return (uint8_t) (ch | 0xC0); }
        }

        // We want to strip out prefix chars for two-byte char formats
        if (ch == 0xC2 || ch == 0xC3 || ch == 0x82) return (uint8_t) 0;

        // If we already returned an unconvertable-character symbol for this unconvertable-character sequence, return NULs for the rest of it
        if (SKIPREST) return (uint8_t) 0;
        SKIPREST = true;

        return (uint8_t) 191; // otherwise: return ¿ if character can't be converted (note that the font map we're using doesn't stick to standard EASCII codes)
    }

    int handleStatusUpdate(const Status *arg);

    void setScreenItems(cScreenItems &items)
    {
        m_screenItemsLock.lock();
        m_items = items;
        m_screenItemsLock.unlock();
    }

    void setScreenOverlayItems(cScreenItems &items)
    {
        m_screenItemsLock.lock();
        m_overlayItems = items;
        m_screenItemsLock.unlock();
    }

    void forceUpdate();

  protected:
    /// Updates the UI.
    //
    // Called periodically from the main loop.
    void doTask() final;

  private:
    enum class Cmd {
        INVALID,
        SET_ON,
        SET_OFF,
        START_BLUETOOTH_PIN_SCREEN,
        STOP_BLUETOOTH_PIN_SCREEN,
        STOP_BOOT_SCREEN,
        PRINT,
    };
    struct CmdItem {
        Cmd cmd;
        union {
            uint32_t bluetooth_pin;
            char *print_text;
        };
    };

    /// Enques given command item to be processed by main loop().
    bool enqueueCmd(const CmdItem &cmd)
    {
        if (!useDisplay)
            return true; // claim success if our display is not in use
        else {
            bool success = cmdQueue.enqueue(cmd, 0);
            setPeriod(1); // handle ASAP
            return success;
        }
    }

    // Implementations of various commands, called from doTask().
    void handleSetOn(bool on);
    void handleStartBluetoothPinScreen(uint32_t pin);
    void handlePrint(const char *text);

    /// Rebuilds our list of frames (screens) to default ones.
    void setFrames();

    /// Queue of commands to execute in doTask.
    TypedQueue<CmdItem> cmdQueue;
    /// Whether we are using a display
    bool useDisplay = false;
    /// Whether the display is currently powered
    bool screenOn = false;
    // Whether we are showing the regular screen (as opposed to booth screen or
    // Bluetooth PIN screen)
    bool showingNormalScreen = false;

    SSD1306Wire dispdev;
    /// UI helper for rendering to frames and switching between them
    OLEDDisplayUi ui;
};
