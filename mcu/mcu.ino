#include <EEPROM.h>

// --- Defines ---
#define TEMP_SENSOR_PIN                         (A0)
#define LED_PIN                                 (13)
#define FLOOD_SENSOR_PIN                        (12)

#define COMMAND_TURN_LED_ON                     ('A')
#define COMMAND_TURN_LED_OFF                    ('S')
#define COMMAND_GET_TEMPERATURE                 ('T')
#define COMMAND_GET_LED_STATE                   ('L')
#define COMMAND_WRITE_MESSAGE                   ('N')
#define COMMAND_READ_MESSAGES                   ('M')

#define EEPROM_MESSAGE_TABLE_MESSAGE_SIZE       (21)
#define EEPROM_MESSAGE_TABLE_ENTRIES_SIZE       (10)
#define EEPROM_MESSAGE_TABLE_START_ADDRESS      (0)

#define EEPROM_FLOOD_TABLE                      (512)

#define SERIAL_MESSAGE_BUFFER_SIZE              (128)

// --- Type Definitions ---
typedef struct EEPROMMessageEntry
{
    uint8_t valid;
    uint32_t timestamp;
    char message[EEPROM_MESSAGE_TABLE_MESSAGE_SIZE];
} EEPROMMessageEntry;

typedef struct EEPROMMessageTable
{
    EEPROMMessageEntry entry[EEPROM_MESSAGE_TABLE_ENTRIES_SIZE];
} EEPROMMessageTable;

// --- Function Prototypes ---
static double readTemp(void);
static void setLEDState(uint8_t LEDState);
static uint8_t getLEDState(void);
static void readFloodSensorState(void);

static void readMessageTableFromEEPROM(void);
static void writeMessageTableFromEEPROM(void);
static void addEntryToMessageTable(uint32_t timestamp, char *message);

static void serialCommandReadMessages(void);
static void serialCommandWriteMessage(void);
static void serialManager(void);

// --- Private Variables ---
static EEPROMMessageTable messageTable = {0};

// --- Function Implementations
void setup() 
{
    pinMode(LED_PIN, OUTPUT);
    pinMode(FLOOD_SENSOR_PIN, INPUT);
    Serial.begin(9600);

    readMessageTableFromEEPROM();
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

static void readMessageTableFromEEPROM(void)
{
    EEPROM.get(EEPROM_MESSAGE_TABLE_START_ADDRESS, messageTable);
}

static void writeMessageTableFromEEPROM(void)
{
    EEPROM.put(EEPROM_MESSAGE_TABLE_START_ADDRESS, messageTable);
}

static void addEntryToMessageTable(uint32_t timestamp, char *message)
{
    uint8_t entryIDMinTimestamp = 0;
    uint32_t minTimestamp = 0xFFFFFFFF;
    for(uint8_t entryID = 0; entryID < EEPROM_MESSAGE_TABLE_ENTRIES_SIZE; entryID++)
    {
        // Search for a free field
        if (0 == messageTable.entry[entryID].valid)
        {
            messageTable.entry[entryID].valid = 1;
            messageTable.entry[entryID].timestamp = timestamp;
            strcpy(messageTable.entry[entryID].message, message);

            // Field found
            return;
        }

        if (minTimestamp > messageTable.entry[entryID].timestamp)
        {
            entryIDMinTimestamp = entryID;
            minTimestamp = messageTable.entry[entryID].timestamp;
        }
    }

    // If there isn't any field that is free,
    // we will clear the oldest one and add the new one on it's slot
    messageTable.entry[entryIDMinTimestamp].valid = 1;
    messageTable.entry[entryIDMinTimestamp].timestamp = timestamp;
    strcpy(messageTable.entry[entryIDMinTimestamp].message, message);
}

static void serialCommandReadMessages(void)
{
    for(uint8_t message_nr = 0; message_nr < EEPROM_MESSAGE_TABLE_ENTRIES_SIZE; message_nr++)
    {
        // Make sure that each message has a string terminator
        messageTable.entry[message_nr].message[EEPROM_MESSAGE_TABLE_MESSAGE_SIZE - 1] = '\0';
        Serial.println("Message ID: " + String(message_nr) + 
                       "|Valid: " + String(messageTable.entry[message_nr].valid) + 
                       "|Timestamp: " + String(messageTable.entry[message_nr].timestamp) + 
                       "|" + String(messageTable.entry[message_nr].message));
    }
}

static void serialCommandWriteMessage(void)
{
    char buffer[SERIAL_MESSAGE_BUFFER_SIZE];
    char *token;
    uint32_t timestamp;
    char message[EEPROM_MESSAGE_TABLE_MESSAGE_SIZE];
    char message2[]="CCACA";

    int bufferLength = Serial.readBytesUntil('!', buffer, SERIAL_MESSAGE_BUFFER_SIZE);
    if(SERIAL_MESSAGE_BUFFER_SIZE == bufferLength)
    {
        bufferLength =  SERIAL_MESSAGE_BUFFER_SIZE - 1;
    }
    buffer[bufferLength] = '\0';
    
    token = strtok(buffer, "|");
    timestamp = atol(token);
    token = strtok(NULL, "|\n");
    memset(message, 0, sizeof(message));
    strncpy(message, token, EEPROM_MESSAGE_TABLE_MESSAGE_SIZE - 1);

    addEntryToMessageTable(timestamp, message);
    writeMessageTableFromEEPROM();
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
            case COMMAND_READ_MESSAGES:
            {
                serialCommandReadMessages();
                break;
            }
            case COMMAND_WRITE_MESSAGE:
            {
                serialCommandWriteMessage();
                break;
            }
            default:
                break;
        }
    }
}