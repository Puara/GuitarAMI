//****************************************************************************//
// GuitarAMI                                                                  //
// Input Devices and Music Interaction Laboratory (IDMIL), McGill University  //
// Edu Meneses (2021) - https://www.edumeneses.com                            //
//****************************************************************************//


#include "module.h"
Module module;

const int32_t firmware = 210901;   // YYMMDD

extern "C" {
    void app_main(void);
}

void app_main() {

    vTaskDelay(2000 / portTICK_PERIOD_MS);
    
    printf("\n");
    printf("**********************************************************\n");
    printf("* T-Stick 3G                                             *\n");
    printf("* Input Devices and Music Interaction Laboratory (IDMIL) *\n");
    printf("* Created by Edu Meneses - firmware version %d       *\n", firmware);
    printf("**********************************************************\n\n");

    module.config_spiffs();
    
    module.read_json(firmware);

    module.startWifi();

    module.start_webserver();

    printf("Done!\n");
    while (1) {
        printf("Running...\n");
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }

    // fflush(stdout);
    // esp_restart();
}