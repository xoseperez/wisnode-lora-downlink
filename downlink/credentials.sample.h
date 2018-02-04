#define WORK_MODE       LoRaWAN
#define JOIN_MODE       OTAA    //  OTAA or ABP

#if JOIN_MODE == ABP
    #define NWKSKEY     ""
    #define APPSKEY     ""
    #define DEVADDR     ""
#endif

#if JOIN_MODE == OTAA
    #define DEVEUI      ""
    #define APPEUI      ""
    #define APPKEY      ""
#endif
