// --- Defines ---
#define TEMP_SENSOR_PIN         (A0)
#define LED_PIN                 (13)
#define FLOOD_SENSOR_PIN        (12)

#define TURN_LED_ON_COMMAND     ('A')
#define TURN_LED_OFF_COMMAND    ('S')

// --- Function Prototypes ---
static void readTemp(void);
static void controlLED(void);
static void readFloodSensorState(void);

// --- Function Implementations
void setup() 
{
    pinMode(LED_PIN, OUTPUT);
    pinMode(FLOOD_SENSOR_PIN, INPUT);
    Serial.begin(9600);
}

void loop()
{
    Serial.println("---------------------------------------------------");
    readTemp();
    readFloodSensorState();
    Serial.println("---------------------------------------------------");
    controlLED();
    delay(1000);
}

static void readTemp(void)
{
    uint16_t adcValue = analogRead(TEMP_SENSOR_PIN);
    double voltage = ((double)adcValue / 1024.0) * 5000.0; // Measured in mV
    double temperature = voltage / 10.0;
    Serial.println("Temperature: " + String(temperature) + "°C");   
}

static void controlLED(void)
{
    if (Serial.available())
    {
        char receivedCommand = Serial.read();

        if (TURN_LED_ON_COMMAND == receivedCommand)
        {
            digitalWrite(LED_PIN, HIGH);
        }
        else if (TURN_LED_OFF_COMMAND == receivedCommand)
        {
            digitalWrite(LED_PIN, LOW);
        }
    }
}

static void readFloodSensorState(void)
{
    uint8_t floodSensorStateValue = digitalRead(FLOOD_SENSOR_PIN);
    String floodSensorStateText = (HIGH == floodSensorStateValue) ? "FLOOD DETECTED" : "NOTHING";
    Serial.println("Flood Sensor State: " + floodSensorStateText);
}