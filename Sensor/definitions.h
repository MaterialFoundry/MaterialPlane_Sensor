/**
 * Stores definitions
 */

#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#define TEST_PIN    8

#define FIRMWARE_VERSION  "3.0.4"           /* Firmware version of the sensor */
#define DEVICE_NAME       "materialsensor"  /* (Default) device name which is used to name the access point and webserver address (e.g. http://materialsensor.local) */

const String GITHUB_LINK PROGMEM = "https://github.com/CDeenen/MaterialPlane_Hardware/releases?q=webserver&expanded=true"; /* GitHub link to get firmware updates from */

#ifndef HARDWARE_VARIANT

#define HARDWARE_VARIANT "DIY Basic"
#define HARDWARE_VERSION "1.0"
#define WIIMOTE_SENSOR
#define BATTERY_LED
#define CONNECTION_LED

//I2C pins for WiiMote camera
static const uint8_t SDA_PIN = 21;
static const uint8_t SCL_PIN = 22;

//LED pins
static const uint8_t BATTERY_LED_GREEN_PIN    = 25;   //Green battery led pin
static const uint8_t BATTERY_LED_RED_PIN      = 26;    //Red battery led pin
static const uint8_t CONNECTION_LED_GREEN_PIN = 27;   //Green connection led pin
static const uint8_t CONNECTION_LED_RED_PIN   = 15;    //Red connection led pin

//LED channels
static const uint8_t BATTERY_LED_GREEN        = 0;    //Green battery led channel
static const uint8_t BATTERY_LED_RED          = 1;    //Red battery led channel
static const uint8_t CONNECTION_LED_GREEN     = 2;    //Green connection led channel
static const uint8_t CONNECTION_LED_RED       = 3;    //Red connection led channel

#endif

/* Serial modes */
enum SERIAL_MODE
{
  SERIAL_OFF,
  SERIAL_READABLE,
  SERIAL_DEFAULT
};
const String serialModeString[] PROGMEM = {"off","readable","default"};

/* General settings */
struct Settings {
  bool debug = false;
  SERIAL_MODE serialMode = SERIAL_DEFAULT;
};

/* Webserver port */
#define WEBSERVER_PORT  80

////////////////////////////////////////////////////////////////////////////////////////
//Activity Monitor
////////////////////////////////////////////////////////////////////////////////////////
#define ACTIVITY_MONITOR_STACK_SIZE 2000  /* Stack size used by the activity monitor task */
#define ACTIVITY_MONITOR_DELAY      1000  /* Delay time between each run of the activity monitor loop */
#define CPU_FREQ_MAX                240   /* Max CPU frequency (MHz) */
#define CPU_FREQ_IDLE               80    /* Idle CPU frequency (MHz) */
#define IRTRACKER_RESET_TIMEOUT     30    /* Reset timeout for the ir tracker (s) */
#define IRTRACKER_MAXRESET_TIMEOUT  90    /* max reset timeout for the ir tracker (s) */
#define IDLE_TIMEOUT                10    /* Idle timeout (s) */

const uint32_t xTalFreqLUT[3] PROGMEM = {40000000, 26000000, 24000000}; /* Lookup table for crystal frequencies to determine the minimum CPU frequency */
const uint8_t minCpuFreqLUT[3] PROGMEM = {10, 13, 12};                  /* Lookup table for the minimum CPU frequency */

/* Power modes */
enum PowerModes
{
  POWER_ACTIVE = 0,
  POWER_IDLE,
  POWER_SAVE,
  POWER_SUPERSAVE
};

////////////////////////////////////////////////////////////////////////////////////////
//Communication
////////////////////////////////////////////////////////////////////////////////////////
/* Websocket modes */
enum WS_MODE
{
  WS_MODE_OFF,
  WS_MODE_SERVER,
  WS_MODE_CLIENT
};
const String websocketModeString[] PROGMEM = {"off","server","client"};

#define COM_STACK_SIZE  10000     /* Communication task stack size */

#define WS_PORT_DEFAULT 3000      /* Default websocket port */
#define WIFI_TIMEOUT    10        /* WiFi Timeout (amount of times it tries to connect to an access point) */

#define PING_STACK_SIZE     1750  /* Ping tack stack size */
#define PING_PERIOD         500   /* Ping period in ms */
#define PING_STRING         "{\"status\":\"ping\",\"source\":\"main\"}" /* String to send with each ping */

#define STATUS_PERIOD       5000  /* Status update period in ms */

/* Struct to store all network settings */
struct NetworkConfig {
  String ssid = "";
  String name = DEVICE_NAME;
  WS_MODE websocketMode = WS_MODE_SERVER;
  uint16_t websocketPort = WS_PORT_DEFAULT;
};

////////////////////////////////////////////////////////////////////////////////////////
//LEDs
////////////////////////////////////////////////////////////////////////////////////////
#define LED_G_MAX 255       /* Maximum PWM value for green LED */
#define LED_R_MAX 100       /* Maximum PWM value for red LED */
#define LED_FADE_STEP 0.05  /* Stepsize for led fading */
#define LED_FADE_MIN 0.1    /* Minimum value for led fading */
#define LED_FADE_MAX 1.0    /* Maximum value for led fading */
#define LED_FREQ    1000    /* PWM frequency of LEDs */

////////////////////////////////////////////////////////////////////////////////////////
//Power management
////////////////////////////////////////////////////////////////////////////////////////
/********* Battery definitions ***********/
#define BAT_CAP                 1200    /* Battery capacity in mAh */
#define BAT_R_SENSE             5       /* Sense resistor in mOhm */
#define BAT_TERM_CURR           100     /* Termination current in mA */
#define BAT_EMPTY_VOLTAGE       3100    /* Empty voltage in mV */
#define BAT_EMPTY_RECOV_VOLTAGE 3500    /* Empty recovery voltage in mV */
#define BAT_FULL_SOC_THR        95      /* Full State of Charge in % */
#define BAT_RCOMP_0             0x004d
#define BAT_TEMP_CO             0x223e

/* State of charge threshold enum */
enum SOC_TH
{
  SOC_BELOW,
  SOC_NORMAL,
  SOC_ABOVE
};

//=============================================================
//IR ID sensor
//=============================================================
#define RMT_STACK_SIZE 2000 /* Stack size of RMT task */

//=============================================================
//IR Tracker
//=============================================================
#define IR_SENSOR_STACK_SIZE  3500  /* Stack size of IR tracker task */

const int framePeriodLUT[] PROGMEM = {200, 55, 45, 35, 25, 15}; /* Lookup table to determine frame period of sensor based on the update rate */

/* Default values */
#define CAL_EN          false
#define CALOFFS_EN      false
#define MIRX            false
#define MIRY            false
#define ROT             false
#define OFFSETX         0
#define OFFSETY         0
#define SCALEX          1
#define SCALEY          1
#define AVG             10
#define UPDATE          5
#define BRIGHT          70
#define MINBRIGHT       50
#define OBJECT_NUM      8
#define MIN_AREA        0
#define MAX_AREA        20
#define NOISE           5

/* Calibration values */
#define CAL_AVG         20

/* Struct to store sensor settings */
struct SensorConfig {
  bool calibrationEnable = CAL_EN;
  bool calibrationOffsetEnable = CALOFFS_EN;
  bool mirrorX = MIRX;
  bool mirrorY = MIRY;
  bool rotation = ROT;
  int16_t offsetX = OFFSETX;
  int16_t offsetY = OFFSETY;
  float scaleX = SCALEX;
  float scaleY = SCALEY;
  uint8_t average = AVG;
  uint8_t updateRate = UPDATE;
  uint8_t brightness = BRIGHT;
  uint8_t minBrightness = MINBRIGHT;
};

/* Enum with calibration modes */
enum CalibrationModes
{
  CAL_MODE_SINGLE,
  CAL_MODE_MULTI,
  CAL_MODE_OFFSET 
};

/* Enum with calibration statuses */
enum CalibrationStatus
{
  CAL_INACTIVE,
  CAL_STARTING,
  CAL_ACTIVE,
  CAL_NEXT,
  CAL_CANCEL,
  CAL_STOP
};

/* Enum with auto exposure statuses */
enum AutoExposureStatus
{
  AUTOEXP_INACTIVE,
  AUTOEXP_STARTING,
  AUTOEXP_PREACTIVE,
  AUTOEXP_ACTIVE,
  AUTOEXP_CANCEL,
  AUTOEXP_STOP
};

#endif /* DEFINITIONS */
