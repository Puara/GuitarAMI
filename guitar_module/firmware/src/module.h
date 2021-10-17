//****************************************************************************//
// GuitarAMI Module - WiFi and file system functions                          //
// Input Devices and Music Interaction Laboratory (IDMIL), McGill University  //
// Edu Meneses (2021) - https://www.edumeneses.com                            //
//****************************************************************************//

#ifndef MODULE_H
#define MODULE_H

#include <stdio.h>
#include <string>
#include <cstring>
#include <ostream>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include <esp_spi_flash.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <sys/unistd.h>
#include <sys/stat.h>
#include <sys/param.h>
#include "esp_err.h"
#include "esp_spiffs.h"
#include "cJSON.h"
#include <esp_http_server.h>
#include "lwip/err.h"
#include "lwip/sys.h"

class Module {
    
    private:
        static std::string dmi_name;

        static std::string device;
        static uint16_t id;
        static std::string author;
        static std::string institution;
        static std::string color;
        static std::string APpasswd;
        static std::string wifiSSID;
        static std::string wifiPSK;
        static std::string oscIP1;
        static uint16_t oscPORT1;
        static std::string oscIP2;
        static uint16_t oscPORT2;
        static uint16_t localPORT;
        static uint16_t Tstick_FSRoffset;

        static int32_t firmware;
        static bool STA_is_connected;
        static std::string current_STA_IP;
        static std::string current_STA_MAC;
        static std::string current_AP_IP;
        static std::string current_AP_MAC;

        static EventGroupHandle_t s_wifi_event_group; // FreeRTOS event group to 
                                                      // signal when we are connected
        static const int wifi_connected_bit = BIT0;   // we are connected to the AP 
                                                      // with an IP
        static const int wifi_fail_bit = BIT1;        // fails to connect after the 
                                                      // maximum amount of retries
        static wifi_config_t wifi_config_sta;
        static wifi_config_t wifi_config_ap;
        static const uint8_t channel = 6;
        static const uint8_t max_connection = 5;
        static const uint8_t wifi_maximum_retry = 5;
        static uint8_t connect_counter;
        static void sta_event_handler(void* arg, esp_event_base_t event_base, 
                                        int32_t event_id, void* event_data);
        static void ap_event_handler(void* arg, esp_event_base_t event_base, 
                                        int32_t event_id, void* event_data);
        void wifi_init();

        static httpd_handle_t webserver;
        static httpd_config_t webserver_config;
        static httpd_uri_t index;
        static httpd_uri_t style;
        static httpd_uri_t favicon;
        static httpd_uri_t factory;
        static httpd_uri_t reboot;
        static httpd_uri_t scan;
        static httpd_uri_t update;
        static httpd_uri_t indexpost;
        static esp_err_t index_get_handler(httpd_req_t *req);
        static esp_err_t style_get_handler(httpd_req_t *req);
        static esp_err_t favicon_get_handler(httpd_req_t *req);
        static esp_err_t factory_get_handler(httpd_req_t *req);
        static esp_err_t reboot_get_handler(httpd_req_t *req);
        static esp_err_t scan_get_handler(httpd_req_t *req);
        static esp_err_t update_get_handler(httpd_req_t *req);
        static esp_err_t echo_post_handler(httpd_req_t *req);
        static void find_and_replace(std::string old_text, std::string new_text, std::string &str);
        static void find_and_replace(std::string old_text, uint32_t new_number, std::string &str);
        static esp_vfs_spiffs_conf_t spiffs_config;
        static std::string spiffs_base_path;
        static const uint8_t spiffs_max_files = 10;
        static const bool spiffs_format_if_mount_failed = false;
    
    public:

        static void config_spiffs();
        static httpd_handle_t start_webserver(void);
        static void stop_webserver(void);
        bool set_wifiSSID(std::string &SSID);
        bool set_wifiSSID(const char *SSID);
        bool set_wifiPSK(std::string &PSK);
        bool set_wifiPSK(const char *PSK);
        void startWifi();
        static void mount_spiffs();
        static void unmount_spiffs();
        void read_json(int32_t firmware_number);
};

#endif