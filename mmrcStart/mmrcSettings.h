/**
  * Settings for this specific MMRC client
  */
// Access point
#define APNAME "MMRC-start"
#define APPASSWORD "mmrc1234"

// Define which pin to use for the pushbutton
#define BUTTON_PIN D5

// Defince which pin to use for LED output
#define LED_PIN LED_BUILTIN

// Configuration pin
// When CONFIG_PIN is pulled to ground on startup, the client will use the initial
// password to buld an AP. (E.g. in case of lost password)
//#define CONFIG_PIN D2

// Status indicator pin
// First it will light up (kept LOW), on Wifi connection it will blink
// and when connected to the Wifi it will turn off (kept HIGH).
//#define STATUS_PIN LED_BUILTIN
