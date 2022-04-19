
/**
 * Stucture containing the current settings of the machine
 */
typedef struct {
    uint8_t pump_pwr;
    uint8_t heat_pwr;
    uint8_t led_flags;
} MachineSettings;

/**
 * Structure of the current values of the machine sensors. Each value is -1 if missing
 */
typedef struct {
    int pressure;
    int temperature;
    int weight;
    int switches;
} MachineStatus;