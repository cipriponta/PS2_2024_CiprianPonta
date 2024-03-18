#include <EEPROM.h>

// --- Defines ---
#define TEMP_SENSOR_PIN                         (A0)
#define LED_PIN                                 (13)
#define FLOOD_SENSOR_PIN                        (12)

#define FLOOD_DETECTED                          (1)
#define FLOOD_NO_EVENT                          (0)

#define COMMAND_TURN_LED_ON                     ('A')
#define COMMAND_TURN_LED_OFF                    ('S')
#define COMMAND_GET_TEMPERATURE                 ('T')
#define COMMAND_GET_LED_STATE                   ('L')
#define COMMAND_WRITE_MESSAGE                   ('N')
#define COMMAND_READ_MESSAGES                   ('M')
#define COMMAND_READ_TIMESTAMP                  ('G')
#define COMMAND_WRITE_TIMESTAMP                 ('H')
#define COMMAND_READ_FLOOD_DETECTED             ('F')
#define COMMAND_READ_FLOODS                     ('Y')
#define COMMAND_DELETE_FLOODS                   ('U')

#define EEPROM_MESSAGE_TABLE_START_ADDRESS      (0)
#define EEPROM_MESSAGE_TABLE_MESSAGE_SIZE       (21)
#define EEPROM_MESSAGE_TABLE_ENTRIES_SIZE       (10)

#define EEPROM_FLOOD_TABLE_START_ADDRESS        (512)
#define EEPROM_FLOOD_TABLE_ENTRIES_SIZE         (10)

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

typedef struct EEPROMFloodEntry
{
    uint8_t valid;
    uint32_t timestamp;
} EEPROMFloodEntry;

typedef struct EEPROMFloodTable
{
    EEPROMFloodEntry entry[EEPROM_FLOOD_TABLE_ENTRIES_SIZE];
} EEPROMFloodTable;

// --- Function Prototypes ---
static double readTemp(void);
static void setLEDState(uint8_t LEDState);
static uint8_t getLEDState(void);
static void checkFloodSensorState(void);

static void readMessageTableFromEEPROM(void);
static void writeMessageTableToEEPROM(void);
static void addEntryToMessageTable(uint32_t timestamp, char *message);

static void readFloodsTableFromEEPROM(void);
static void writeFloodsTableToEEPROM(void);
static void addEntryToFloodTable(void);
static void deleteEntryFromFloodTable(uint32_t timestamp);

static void serialCommandReadMessages(void);
static void serialCommandWriteMessage(void);
static void serialCommandReadTimestamp(void);
static void serialCommandReadFloods(void);
static void serialCommandDeleteFloods(void);
static void serialManager(void);

// --- Private Variables ---
static EEPROMMessageTable messageTable = {0};
static EEPROMFloodTable floodsTable = {0};
static uint32_t localTimestamp = 0;
static uint8_t prevFloodSensorState = LOW;
static uint8_t floodDetected = FLOOD_NO_EVENT;

// --- Function Implementations
void setup() 
{
    pinMode(LED_PIN, OUTPUT);
    pinMode(FLOOD_SENSOR_PIN, INPUT);
    Serial.begin(9600);

    readMessageTableFromEEPROM();
    readFloodsTableFromEEPROM();
}

void loop()
{
    checkFloodSensorState();
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

static void checkFloodSensorState(void)
{
    uint8_t floodSensorState = digitalRead(FLOOD_SENSOR_PIN);

    if (LOW == prevFloodSensorState && HIGH == floodSensorState)
    {
        floodDetected = FLOOD_DETECTED;
    }

    prevFloodSensorState = floodSensorState;
}

static void readMessageTableFromEEPROM(void)
{
    EEPROM.get(EEPROM_MESSAGE_TABLE_START_ADDRESS, messageTable);
}

static void writeMessageTableToEEPROM(void)
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

static void readFloodsTableFromEEPROM(void)
{
    EEPROM.get(EEPROM_FLOOD_TABLE_START_ADDRESS, floodsTable);
}

static void writeFloodsTableToEEPROM(void)
{
    EEPROM.put(EEPROM_FLOOD_TABLE_START_ADDRESS, floodsTable);
}

static void addEntryToFloodTable(void)
{
    uint8_t entryIDMinTimestamp = 0;
    uint32_t minTimestamp = 0xFFFFFFFF;
    for(uint8_t entryID = 0; entryID < EEPROM_FLOOD_TABLE_ENTRIES_SIZE; entryID++)
    {
        // Search for a free field
        if (0 == floodsTable.entry[entryID].valid)
        {
            floodsTable.entry[entryID].valid = 1;
            floodsTable.entry[entryID].timestamp = localTimestamp;

            // Field found
            return;
        }

        if (minTimestamp > floodsTable.entry[entryID].timestamp)
        {
            entryIDMinTimestamp = entryID;
            minTimestamp = floodsTable.entry[entryID].timestamp;
        }
    }

    // If there isn't any field that is free,
    // we will clear the oldest one and add the new one on it's slot
    floodsTable.entry[entryIDMinTimestamp].valid = 1;
    floodsTable.entry[entryIDMinTimestamp].timestamp = localTimestamp;
}

static void deleteEntryFromFloodTable(uint32_t timestamp)
{
    for(uint8_t entryID = 0; entryID < EEPROM_FLOOD_TABLE_ENTRIES_SIZE; entryID++)
    {
        if (floodsTable.entry[entryID].timestamp == timestamp)
        {
            floodsTable.entry[entryID].valid = 0;
        }
    }
}

static void serialCommandReadMessages(void)
{
    for(uint8_t message_nr = 0; message_nr < EEPROM_MESSAGE_TABLE_ENTRIES_SIZE; message_nr++)
    {
        // Make sure that each message has a string terminator
        messageTable.entry[message_nr].message[EEPROM_MESSAGE_TABLE_MESSAGE_SIZE - 1] = '\0';
        Serial.println(String(messageTable.entry[message_nr].valid) + "|" + 
                       String(messageTable.entry[message_nr].timestamp) + "|" + 
                       String(messageTable.entry[message_nr].message));
    }
}

static void serialCommandWriteMessage(void)
{
    char buffer[SERIAL_MESSAGE_BUFFER_SIZE];
    char *token;
    uint32_t timestamp;
    char message[EEPROM_MESSAGE_TABLE_MESSAGE_SIZE];

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
    writeMessageTableToEEPROM();
}

static void serialCommandReadTimestamp(void)
{   
    char buffer[SERIAL_MESSAGE_BUFFER_SIZE];
    char *token;
    char message[EEPROM_MESSAGE_TABLE_MESSAGE_SIZE];

    int bufferLength = Serial.readBytesUntil('!', buffer, SERIAL_MESSAGE_BUFFER_SIZE);
    if(SERIAL_MESSAGE_BUFFER_SIZE == bufferLength)
    {
        bufferLength =  SERIAL_MESSAGE_BUFFER_SIZE - 1;
    }

    buffer[bufferLength] = '\0';
    token = strtok(buffer, "|");
    localTimestamp = atol(token);
}

static void serialCommandReadFloods(void)
{
    for(uint8_t entryID = 0; entryID < EEPROM_FLOOD_TABLE_ENTRIES_SIZE; entryID++)
    {
        Serial.println(String(floodsTable.entry[entryID].valid) + "|" + 
                       String(floodsTable.entry[entryID].timestamp));
    }
}

static void serialCommandDeleteFloods(void)
{
    char buffer[SERIAL_MESSAGE_BUFFER_SIZE];
    char *token;
    char message[EEPROM_MESSAGE_TABLE_MESSAGE_SIZE];
    uint32_t timestamp;

    int bufferLength = Serial.readBytesUntil('!', buffer, SERIAL_MESSAGE_BUFFER_SIZE);
    if(SERIAL_MESSAGE_BUFFER_SIZE == bufferLength)
    {
        bufferLength =  SERIAL_MESSAGE_BUFFER_SIZE - 1;
    }

    buffer[bufferLength] = '\0';
    token = strtok(buffer, "|");
    timestamp = atol(token);

    deleteEntryFromFloodTable(timestamp);
    writeFloodsTableToEEPROM();
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
            case COMMAND_READ_TIMESTAMP:
            {
                Serial.println(localTimestamp);
                break;
            }
            case COMMAND_WRITE_TIMESTAMP:
            {
                serialCommandReadTimestamp();
                break;
            }
            case COMMAND_READ_FLOOD_DETECTED:
            {
                Serial.println(((floodDetected) ? ("FLOOD_DETECTED") : ("FLOOD_NO_EVENT")));
                if (FLOOD_DETECTED == floodDetected)
                {
                    floodDetected = FLOOD_NO_EVENT;
                    addEntryToFloodTable();
                    writeFloodsTableToEEPROM();
                }
                break;
            }
            case COMMAND_READ_FLOODS:
            {
                serialCommandReadFloods();
                break;
            }
            case COMMAND_DELETE_FLOODS:
            {
                serialCommandDeleteFloods();
                break;
            }
            default:
                break;
        }
    }
}