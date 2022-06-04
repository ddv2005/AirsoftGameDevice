#include "config.h"
#include "abscreen.h"
#include "esputils.h"
#include "images.h"
#include "global.h"
#include "fonts.h"

#define FONT_HEIGHT 14 // actually 13 for "ariel 10" but want a little extra space
#define FONT_HEIGHT_16 (ArialMT_Plain_16[1] + 1)
// This means the *visible* area (sh1106 can address 132, but shows 128 for example)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define TRANSITION_FRAMERATE 30 // fps
#define IDLE_FRAMERATE 1        // in fps

#define NUM_EXTRA_FRAMES 2 // text message and debug frame

static uint32_t targetFramerate = IDLE_FRAMERATE;
static char btPIN[16] = "888888";

uint8_t imgBattery[16] = {0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xE7, 0x3C};
uint8_t imgSatellite[8] = {0x70, 0x71, 0x22, 0xFA, 0xFA, 0x22, 0x71, 0x70};

uint32_t dopThresholds[5] = {2000, 1000, 500, 200, 100};

static void drawBootScreen(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y)
{
    // draw an xbm image.
    // Please note that everything that should be transitioned
    // needs to be drawn relative to x and y
    //display->drawXbm(x + 32, y, icon_width, icon_height, (const uint8_t *)icon_bits);

    display->setFont(Tahoma_16);
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->drawString(SCREEN_WIDTH/2, 16, "Airsoft");
    display->drawString(SCREEN_WIDTH/2, 16*2+4, "Game Device");
    display->setFont(Tahoma_10);
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    char buf[16];
    snprintf(buf, sizeof(buf), "Version %s",
             xstr(APP_VERSION)); // Note: we don't bother printing region or now, it makes the string too long
    display->drawString(SCREEN_WIDTH - 30, 0, buf);
}

void msOverlay(OLEDDisplay *display, OLEDDisplayUiState *state)
{
    static char volbuffer[64];
    display->setTextAlignment(TEXT_ALIGN_LEFT);
    display->setFont(ArialMT_Plain_10);
    if(global->getPower())
    {
        if (global->getPower()->getAXP().isBatteryConnect()) {
            if(global->getPower()->getAXP().getBattVoltage()<4450)
            {
                //snprintf(volbuffer, sizeof(volbuffer), "%.2fV (%.0fmA)", global->getPower()->getAXP().getBattVoltage() / 1000.0, global->getPower()->getAXP().isChargeing() ? global->getPower()->getAXP().getBattChargeCurrent() : global->getPower()->getAXP().getBattDischargeCurrent());
                multi_heap_info_t info;
                heap_caps_get_info(&info, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
                //snprintf(volbuffer, sizeof(volbuffer), "%d%%(%.0fmA)", global->m_powerStatus->getBatteryChargePercent(), global->getPower()->getAXP().isChargeing() ? global->getPower()->getAXP().getBattChargeCurrent() : global->getPower()->getAXP().getBattDischargeCurrent());
                snprintf(volbuffer, sizeof(volbuffer), "%d%%(%0.2fV)", global->m_powerStatus->getBatteryChargePercent(), global->getPower()->getAXP().getBattVoltage()/1000.0);
                display->drawString(0, 0, volbuffer);
            }
            else
            {
                float v = global->getPower()->readBatteryADC();
                if(v>2)
                {
                    snprintf(volbuffer, sizeof(volbuffer), "%0.2fV", v);
                    display->drawString(0, 0, volbuffer);
                }
            }
#ifdef DEBUG_MEM
            snprintf(volbuffer, sizeof(volbuffer), "%u/%uKB/%uKB",  info.total_allocated_bytes / 1024, info.total_free_bytes / 1024, info.largest_free_block / 1024);
            display->drawString(0, 10, volbuffer);
#endif            
        } else {
            multi_heap_info_t info;
            heap_caps_get_info(&info, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
            snprintf(volbuffer, sizeof(volbuffer), "%u/%uKB/%uKB",  info.total_allocated_bytes / 1024, info.total_free_bytes / 1024, info.largest_free_block / 1024);
            display->drawString(0, 0, volbuffer);
        }
    }
    else
    {
            multi_heap_info_t info;
            heap_caps_get_info(&info, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
            snprintf(volbuffer, sizeof(volbuffer), "%u/%uKB/%uKB",  info.total_allocated_bytes / 1024, info.total_free_bytes / 1024, info.largest_free_block / 1024);
            display->drawString(0, 0, volbuffer);
    }
    
}

static void drawFrameBluetooth(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y)
{
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(ArialMT_Plain_16);
    display->drawString(64 + x, y, "Bluetooth");

    display->setFont(ArialMT_Plain_10);
    display->drawString(64 + x, FONT_HEIGHT + y + 2, "Enter this code");

    display->setFont(ArialMT_Plain_24);
    display->drawString(64 + x, 26 + y, btPIN);

    display->setFont(ArialMT_Plain_10);
    char buf[30];
    const char *name = "Name: ";
    strcpy(buf, name);
    strcat(buf, getDeviceName());
    display->drawString(64 + x, 48 + y, buf);
}

static Screen *screen;

/// Draw a series of fields in a column, wrapping to multiple colums if needed
static void drawColumns(OLEDDisplay *display, int16_t x, int16_t y, const char **fields)
{
    // The coordinates define the left starting point of the text
    display->setTextAlignment(TEXT_ALIGN_LEFT);

    const char **f = fields;
    int xo = x, yo = y;
    while (*f)
    {
        display->drawString(xo, yo, *f);
        yo += FONT_HEIGHT;
        if (yo > SCREEN_HEIGHT - FONT_HEIGHT)
        {
            xo += SCREEN_WIDTH / 2;
            yo = 0;
        }
        f++;
    }
}

// Draw power bars or a charging indicator on an image of a battery, determined by battery charge voltage or percentage.
static void drawBattery(OLEDDisplay *display, int16_t x, int16_t y, uint8_t *imgBuffer, const PowerStatus *powerStatus)
{
    static const uint8_t powerBar[3] = {0x81, 0xBD, 0xBD};
    static const uint8_t lightning[8] = {0xA1, 0xA1, 0xA5, 0xAD, 0xB5, 0xA5, 0x85, 0x85};
    // Clear the bar area on the battery image
    for (int i = 1; i < 14; i++)
    {
        imgBuffer[i] = 0x81;
    }
    // If charging, draw a charging indicator
    if (powerStatus->getIsCharging())
    {
        memcpy(imgBuffer + 3, lightning, 8);
        // If not charging, Draw power bars
    }
    else
    {
        for (int i = 0; i < 4; i++)
        {
            if (powerStatus->getBatteryChargePercent() >= 25 * i)
                memcpy(imgBuffer + 1 + (i * 3), powerBar, 3);
        }
    }
    display->drawFastImage(x, y, 16, 8, imgBuffer);
}

// Draw GPS status summary
static void drawGPS(OLEDDisplay *display, int16_t x, int16_t y, const GPSStatus *gps)
{
    if (!gps->getIsConnected())
    {
        display->drawString(x, y - 2, "No GPS");
        return;
    }
    display->drawFastImage(x, y, 6, 8, gps->getHasLock() ? imgPositionSolid : imgPositionEmpty);
    if (!gps->getHasLock())
    {
        display->drawString(x + 8, y - 2, "No sats");
        return;
    }
    else
    {
        char satsString[3];
        uint8_t bar[2] = {0};

        //Draw DOP signal bars
        for (int i = 0; i < 5; i++)
        {
            if (gps->getDOP() <= dopThresholds[i])
                bar[0] = ~((1 << (5 - i)) - 1);
            else
                bar[0] = 0b10000000;
            //bar[1] = bar[0];
            display->drawFastImage(x + 9 + (i * 2), y, 2, 8, bar);
        }

        //Draw satellite image
        display->drawFastImage(x + 24, y, 8, 8, imgSatellite);

        //Draw the number of satellites
        sprintf(satsString, "%d", gps->getNumSatellites());
        display->drawString(x + 34, y - 2, satsString);
    }
}

/// Ported from my old java code, returns distance in meters along the globe
/// surface (by magic?)
static float latLongToMeter(double lat_a, double lng_a, double lat_b, double lng_b)
{
    double pk = (180 / 3.14169);
    double a1 = lat_a / pk;
    double a2 = lng_a / pk;
    double b1 = lat_b / pk;
    double b2 = lng_b / pk;
    double cos_b1 = cos(b1);
    double cos_a1 = cos(a1);
    double t1 = cos_a1 * cos(a2) * cos_b1 * cos(b2);
    double t2 = cos_a1 * sin(a2) * cos_b1 * sin(b2);
    double t3 = sin(a1) * sin(b1);
    double tt = acos(t1 + t2 + t3);
    if (isnan(tt))
        tt = 0.0; // Must have been the same point?

    return (float)(6366000 * tt);
}

static inline double toRadians(double deg)
{
    return deg * PI / 180;
}

static inline double toDegrees(double r)
{
    return r * 180 / PI;
}

static const uint8_t *arialFonts[3] = { Tahoma_10,Tahoma_16,Tahoma_24};
static const uint8_t *monoFonts[3] = { Courier_New_10,Courier_New_16,Courier_New_24};

static void drawScreenItems(OLEDDisplay *display,cScreenItems &items)
{
    cScreenItemsBase::iterator itr = items.getList().begin();
    for(;itr!=items.getList().end(); itr++)
    {
        switch(itr->itemType)
        {
            case SIT_PROGRESS:
            {
                display->drawProgressBar(itr->x,itr->y + SCREEN_Y_OFFSET,itr->itemData.progressData.width,itr->itemData.progressData.height,itr->itemData.progressData.progress);
                break;
            }

            case SIT_STRING:
            {
                const uint8_t **fontSet = arialFonts;
                uint16_t fontSize = itr->itemData.stringData.fontSize;
                if(fontSize>100 && fontSize<200)
                    fontSet = monoFonts;
                fontSize = fontSize%100;
                const uint8_t *font;
                if(fontSize<=12)
                    font = fontSet[0];
                else
                    if(fontSize<=20)
                        font = fontSet[1];
                    else
                        font = fontSet[2];
                display->setFont(font);
                display->setTextAlignment((OLEDDISPLAY_TEXT_ALIGNMENT)itr->itemData.stringData.aligment);
                display->drawString(itr->x,itr->y + SCREEN_Y_OFFSET, itr->strData.c_str());
                break;
            }
        }
    }
}

static void drawNormalFrame(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y)
{
    drawGPS(display, x + (SCREEN_WIDTH * 0.63), y + 2, screen->getGlobal().m_gpsStatus);
    
    Screen *screen = (Screen*)state->userData;
    drawScreenItems(display,screen->m_items);
    drawScreenItems(display,screen->m_overlayItems);    
}
/**
 * Computes the bearing in degrees between two points on Earth.  Ported from my
 * old Gaggle android app.
 *
 * @param lat1
 * Latitude of the first point
 * @param lon1
 * Longitude of the first point
 * @param lat2
 * Latitude of the second point
 * @param lon2
 * Longitude of the second point
 * @return Bearing between the two points in radians. A value of 0 means due
 * north.
 */
static float bearing(double lat1, double lon1, double lat2, double lon2)
{
    double lat1Rad = toRadians(lat1);
    double lat2Rad = toRadians(lat2);
    double deltaLonRad = toRadians(lon2 - lon1);
    double y = sin(deltaLonRad) * cos(lat2Rad);
    double x = cos(lat1Rad) * sin(lat2Rad) - (sin(lat1Rad) * cos(lat2Rad) * cos(deltaLonRad));
    return atan2(y, x);
}

namespace
{

    /// A basic 2D point class for drawing
    class Point
    {
    public:
        float x, y;

        Point(float _x, float _y) : x(_x), y(_y) {}

        /// Apply a rotation around zero (standard rotation matrix math)
        void rotate(float radian)
        {
            float cos = cosf(radian), sin = sinf(radian);
            float rx = x * cos - y * sin, ry = x * sin + y * cos;

            x = rx;
            y = ry;
        }

        void translate(int16_t dx, int dy)
        {
            x += dx;
            y += dy;
        }

        void scale(float f)
        {
            x *= f;
            y *= f;
        }
    };

} // namespace

static void drawLine(OLEDDisplay *d, const Point &p1, const Point &p2)
{
    d->drawLine(p1.x, p1.y, p2.x, p2.y);
}

/**
 * Given a recent lat/lon return a guess of the heading the user is walking on.
 *
 * We keep a series of "after you've gone 10 meters, what is your heading since
 * the last reference point?"
 */
static float estimatedHeading(double lat, double lon)
{
    static double oldLat, oldLon;
    static float b;

    if (oldLat == 0)
    {
        // just prepare for next time
        oldLat = lat;
        oldLon = lon;

        return b;
    }

    float d = latLongToMeter(oldLat, oldLon, lat, lon);
    if (d < 10) // haven't moved enough, just keep current bearing
        return b;

    b = bearing(oldLat, oldLon, lat, lon);
    oldLat = lat;
    oldLon = lon;

    return b;
}

Screen::Screen(abGlobal &global, uint8_t address, int sda, int scl) : m_global(global), cmdQueue(32), dispdev(address, sda, scl), ui(&dispdev)
{
    screen = this;
}

void Screen::handleSetOn(bool on)
{
    if (!useDisplay)
        return;

    if (on != screenOn)
    {
        if (on)
        {
            DEBUG_MSG("Turning on screen\n");
            dispdev.displayOn();
            dispdev.displayOn();
        }
        else
        {
            DEBUG_MSG("Turning off screen\n");
            dispdev.displayOff();
        }
        screenOn = on;
    }
}

void Screen::setup()
{
    concurrency::PeriodicTask::setup();

    // We don't set useDisplay until setup() is called, because some boards have a declaration of this object but the device
    // is never found when probing i2c and therefore we don't call setup and never want to do (invalid) accesses to this device.
    useDisplay = true;

    dispdev.resetOrientation();

    // Initialising the UI will init the display too.
    ui.init();
    ui.setTimePerTransition(300); // msecs
    ui.setIndicatorPosition(BOTTOM);
    // Defines where the first frame is located in the bar.
    ui.setIndicatorDirection(LEFT_RIGHT);
    ui.setFrameAnimation(SLIDE_LEFT);
    // Don't show the page swipe dots while in boot screen.
    ui.disableAllIndicators();
    // Store a pointer to Screen so we can get to it from static functions.
    ui.getUiState()->userData = this;

    // Set the utf8 conversion function
    dispdev.setFontTableLookupFunction(customFontTableLookup);

    // Add frames.
    static FrameCallback bootFrames[] = {drawBootScreen};
    static const int bootFrameCount = sizeof(bootFrames) / sizeof(bootFrames[0]);
    ui.setFrames(bootFrames, bootFrameCount);
    // No overlays.
    ui.setOverlays(nullptr, 0);

    // Require presses to switch between frames.
    ui.disableAutoTransition();

    // Set up a log buffer with 3 lines, 32 chars each.
    dispdev.setLogBuffer(3, 32);

#ifdef FLIP_SCREEN_VERTICALLY
    dispdev.flipScreenVertically();
#endif

    // Turn on the display.
    handleSetOn(true);

    // On some ssd1306 clones, the first draw command is discarded, so draw it
    // twice initially.
    ui.update();
    ui.update();

    // Subscribe to status updates
    powerStatusObserver.observe(&m_global.m_powerStatus->onNewStatus);
    gpsStatusObserver.observe(&m_global.m_gpsStatus->onNewStatus);
}

void Screen::doTask()
{
    // If we don't have a screen, don't ever spend any CPU for us.
    if (!useDisplay)
    {
        setPeriod(0);
        return;
    }

    // Process incoming commands.
    for (;;)
    {
        CmdItem cmd;
        if (!cmdQueue.dequeue(&cmd, 0))
        {
            break;
        }
        switch (cmd.cmd)
        {
        case Cmd::SET_ON:
            handleSetOn(true);
            break;
        case Cmd::SET_OFF:
            handleSetOn(false);
            break;
        case Cmd::START_BLUETOOTH_PIN_SCREEN:
            handleStartBluetoothPinScreen(cmd.bluetooth_pin);
            break;
        case Cmd::STOP_BLUETOOTH_PIN_SCREEN:
        case Cmd::STOP_BOOT_SCREEN:
            setFrames();
            break;
        case Cmd::PRINT:
            handlePrint(cmd.print_text);
            free(cmd.print_text);
            break;
        default:
            DEBUG_MSG("BUG: invalid cmd");
        }
    }

    if (!screenOn)
    {   // If we didn't just wake and the screen is still off, then
        // stop updating until it is on again
        setPeriod(0);
        return;
    }

    // Switch to a low framerate (to save CPU) when we are not in transition
    // but we should only call setTargetFPS when framestate changes, because
    // otherwise that breaks animations.
    if (targetFramerate != IDLE_FRAMERATE && ui.getUiState()->frameState == FIXED)
    {
        // oldFrameState = ui.getUiState()->frameState;
        DEBUG_MSG("Setting idle framerate\n");
        targetFramerate = IDLE_FRAMERATE;
        ui.setTargetFPS(targetFramerate);
    }

    // While showing the bootscreen or Bluetooth pair screen all of our
    // standard screen switching is stopped.
    if (showingNormalScreen)
    {
        // standard screen loop handling here
    }

    ui.update();

    // DEBUG_MSG("want fps %d, fixed=%d\n", targetFramerate,
    // ui.getUiState()->frameState); If we are scrolling we need to be called
    // soon, otherwise just 1 fps (to save CPU) We also ask to be called twice
    // as fast as we really need so that any rounding errors still result with
    // the correct framerate
    setPeriod(1000 / targetFramerate);
}

// restore our regular frame list
void Screen::setFrames()
{
    static OverlayCallback overlays[] = { msOverlay };    
    m_normalFrame = drawNormalFrame;
    ui.setFrames(&m_normalFrame, 1);
    ui.setOverlays(overlays, 1);
    ui.disableAllIndicators();
}

void Screen::handleStartBluetoothPinScreen(uint32_t pin)
{
    DEBUG_MSG("showing bluetooth screen\n");
    showingNormalScreen = false;

    static FrameCallback btFrames[] = {drawFrameBluetooth};

    snprintf(btPIN, sizeof(btPIN), "%06u", pin);

    ui.disableAllIndicators();
    ui.setFrames(btFrames, 1);
}

void Screen::handlePrint(const char *text)
{
    DEBUG_MSG("Screen: %s", text);
    if (!useDisplay)
        return;

    dispdev.print(text);
}

// adjust Brightness cycle trough 1 to 254 as long as attachDuringLongPress is true
void Screen::adjustBrightness()
{
    if (brightness == 254)
    {
        brightness = 0;
    }
    else
    {
        brightness++;
    }
    int width = brightness / (254.00 / SCREEN_WIDTH);
    dispdev.drawRect(0, 30, SCREEN_WIDTH, 4);
    dispdev.fillRect(0, 31, width, 2);
    dispdev.display();
    dispdev.setBrightness(brightness);
}

int Screen::handleStatusUpdate(const Status *arg)
{
    setPeriod(1); // Update the screen right away
    return 0;
}

void Screen::forceUpdate()
{
    ui.getUiState()->lastUpdate=0;
    ui.update();
}