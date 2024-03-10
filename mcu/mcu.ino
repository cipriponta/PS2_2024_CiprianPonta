// --- Defines ---
#define TEMP_SENSOR_PIN             (A0)
#define LED_PIN                     (13)
#define FLOOD_SENSOR_PIN            (12)

#define COMMAND_TURN_LED_ON         ('A')
#define COMMAND_TURN_LED_OFF        ('S')
#define COMMAND_GET_TEMPERATURE     ('T')
#define COMMAND_GET_LED_STATE       ('L')

// --- Function Prototypes ---
static double readTemp(void);
static void setLEDState(uint8_t LEDState);
static uint8_t getLEDState(void);
static void readFloodSensorState(void);
static void serialManager(void);

// --- Function Implementations
void setup() 
{
    pinMode(LED_PIN, OUTPUT);
    pinMode(FLOOD_SENSOR_PIN, INPUT);
    Serial.begin(9600);
}

void loop()
{
    serialManager();
}

static double readTemp(void)
{
    uint16_t adcValue = analogRead(TEMP_SENSOR_PIN);
    double voltage = ((double)adcValue / 1024.0) * 5000.0; // Measured in mV
    double temperature = voltage / 10.0;
    return temperature;   
}

static void setLEDState(uint8_t LEDState)
{
    digitalWrite(LED_PIN, LEDState);
}

static uint8_t getLEDState(void)
{
    return digitalRead(LED_PIN);
}

static void readFloodSensorState(void)
{
    uint8_t floodSensorStateValue = digitalRead(FLOOD_SENSOR_PIN);
    String floodSensorStateText = (HIGH == floodSensorStateValue) ? "FLOOD DETECTED" : "NOTHING";
    Serial.println("Flood Sensor State: " + floodSensorStateText);
}

static void serialManager(void)
{
    if (Serial.available())
    {
        char receivedCommand = Serial.read();

        switch(receivedCommand)
        {
            case COMMAND_TURN_LED_ON:
            {
                setLEDState(HIGH);
                break;
            }
            case COMMAND_TURN_LED_OFF:
            {
                setLEDState(LOW);
                break;
            }
            case COMMAND_GET_TEMPERATURE:
            {
                double temperature = readTemp();
                Serial.println(String(temperature));
                break;
            }
            case COMMAND_GET_LED_STATE:
            {
                uint8_t LEDState = getLEDState();
                Serial.println(((LEDState) ? ("ON") : ("OFF")));
                break;
            }
            default:
                break;
        }
    }
}