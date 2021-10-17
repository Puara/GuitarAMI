/* 
- event_handler, wifi_init_sta, and startWifi were modified from 
  https://github.com/espressif/esp-idf/tree/master/examples/wifi/getting_started/station
- mount_spiffs, and unmount_spiffs were modified from
  https://github.com/espressif/esp-idf/tree/master/examples/storage
*/

#include <module.h>

// Defining static members
std::string Module::dmi_name;
std::string Module::device;
uint16_t Module::id;
std::string Module::author;
std::string Module::institution;
std::string Module::color;
std::string Module::APpasswd;
std::string Module::wifiSSID;
std::string Module::wifiPSK;
std::string Module::oscIP1;
uint16_t Module::oscPORT1;
std::string Module::oscIP2;
uint16_t Module::oscPORT2;
uint16_t Module::localPORT;
uint16_t Module::Tstick_FSRoffset;

int32_t Module::firmware;
std::string Module::current_STA_IP;
std::string Module::current_STA_MAC;
std::string Module::current_AP_IP;
std::string Module::current_AP_MAC;
bool Module::STA_is_connected = false;

esp_vfs_spiffs_conf_t Module::spiffs_config;
std::string Module::spiffs_base_path;
EventGroupHandle_t Module::s_wifi_event_group;
wifi_config_t Module::wifi_config_sta;
wifi_config_t Module::wifi_config_ap;
uint8_t Module::connect_counter;
httpd_handle_t Module::webserver;
httpd_config_t Module::webserver_config;
httpd_uri_t Module::index;
httpd_uri_t Module::favicon;
httpd_uri_t Module::style;
httpd_uri_t Module::factory;
httpd_uri_t Module::reboot;
httpd_uri_t Module::scan;
httpd_uri_t Module::update;
httpd_uri_t Module::indexpost;

void Module::sta_event_handler(void* arg, esp_event_base_t event_base, 
                               int32_t event_id, void* event_data) {
    //int counter = 0;
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && 
               event_id == WIFI_EVENT_STA_DISCONNECTED) {
        printf("%d, %d", Module::connect_counter, Module::wifi_maximum_retry);
        if (Module::connect_counter < Module::wifi_maximum_retry) {
            Module::connect_counter++;
            esp_wifi_connect();
            ESP_LOGI("wifi", "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, Module::wifi_fail_bit);
        }
        ESP_LOGI("wifi","connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI("wifi", "got ip:" IPSTR, IP2STR(&event->ip_info.ip));

        std::stringstream tempBuf;
        tempBuf << esp_ip4_addr1_16(&event->ip_info.ip) << ".";
        tempBuf << esp_ip4_addr2_16(&event->ip_info.ip) << ".";
        tempBuf << esp_ip4_addr3_16(&event->ip_info.ip) << ".";
        tempBuf << esp_ip4_addr4_16(&event->ip_info.ip);
        Module::current_STA_IP = tempBuf.str();

        Module::connect_counter = 0;
        xEventGroupSetBits(s_wifi_event_group, Module::wifi_connected_bit);
    }
}

void Module::wifi_init() {
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap(); // saving pointer to 
                                                                // retrieve AP ip later

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Set device hostname
    esp_err_t setname = tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, 
                                                   dmi_name.c_str());
    if(setname != ESP_OK ){
        ESP_LOGE("wifi","failed to set hostname:%s",dmi_name.c_str());  
    } else {
        ESP_LOGE("wifi","hostname:%s",dmi_name.c_str());  
    }

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &Module::sta_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &Module::sta_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    ESP_LOGI("wifi", "setting wifi mode");
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_LOGI("wifi", "loading STA config");
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config_sta) );
    ESP_LOGI("wifi", "loading AP config");
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config_ap));
    ESP_LOGI("wifi", "esp_wifi_start");
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI("wifi", "wifi_init finished.");

    /* Waiting until either the connection is established (Module::wifi_connected_bit)
     * or connection failed for the maximum number of re-tries (Module::wifi_fail_bit).
     * The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            Module::wifi_connected_bit | Module::wifi_fail_bit,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we
     * can test which event actually happened. */
    if (bits & Module::wifi_connected_bit) {
        ESP_LOGI("wifi", "Connected to SSID: %s", Module::wifiSSID.c_str());
        Module::STA_is_connected = true;
    } else if (bits & Module::wifi_fail_bit) {
        ESP_LOGI("wifi", "Failed to connect to SSID:%s", Module::wifiSSID.c_str());
        Module::STA_is_connected = false;
    } else {
        ESP_LOGE("wifi", "UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, 
                                                          IP_EVENT_STA_GOT_IP, 
                                                          instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, 
                                                          ESP_EVENT_ANY_ID, 
                                                          instance_any_id));
    vEventGroupDelete(s_wifi_event_group);

    // getting extra info
    unsigned char temp_info[6] = {0};
    esp_wifi_get_mac(WIFI_IF_STA, temp_info);
    std::ostringstream tempBuf;
    tempBuf << std::setfill('0') << std::setw(2) << std::hex << (int)temp_info[0] << ":";
    tempBuf << std::setfill('0') << std::setw(2) << std::hex << (int)temp_info[1] << ":";
    tempBuf << std::setfill('0') << std::setw(2) << std::hex << (int)temp_info[2] << ":";
    tempBuf << std::setfill('0') << std::setw(2) << std::hex << (int)temp_info[3] << ":";
    tempBuf << std::setfill('0') << std::setw(2) << std::hex << (int)temp_info[4] << ":";
    tempBuf << std::setfill('0') << std::setw(2) << std::hex << (int)temp_info[5];
    Module::current_STA_MAC = tempBuf.str();
    tempBuf.clear();            // preparing the ostringstream 
    tempBuf.str(std::string()); // buffer for reuse
    esp_wifi_get_mac(WIFI_IF_AP, temp_info);
    tempBuf << std::setfill('0') << std::setw(2) << std::hex << (int)temp_info[0] << ":";
    tempBuf << std::setfill('0') << std::setw(2) << std::hex << (int)temp_info[1] << ":";
    tempBuf << std::setfill('0') << std::setw(2) << std::hex << (int)temp_info[2] << ":";
    tempBuf << std::setfill('0') << std::setw(2) << std::hex << (int)temp_info[3] << ":";
    tempBuf << std::setfill('0') << std::setw(2) << std::hex << (int)temp_info[4] << ":";
    tempBuf << std::setfill('0') << std::setw(2) << std::hex << (int)temp_info[5];
    Module::current_AP_MAC = tempBuf.str();

    esp_netif_ip_info_t ip_temp_info;
    esp_netif_get_ip_info(ap_netif, &ip_temp_info);
    tempBuf.clear();
    tempBuf.str(std::string());
    tempBuf << std::dec << esp_ip4_addr1_16(&ip_temp_info.ip) << ".";
    tempBuf << std::dec << esp_ip4_addr2_16(&ip_temp_info.ip) << ".";
    tempBuf << std::dec << esp_ip4_addr3_16(&ip_temp_info.ip) << ".";
    tempBuf << std::dec << esp_ip4_addr4_16(&ip_temp_info.ip);
    Module::current_AP_IP = tempBuf.str();
}

void Module::startWifi() {

    strncpy((char *) Module::wifi_config_sta.sta.ssid, Module::wifiSSID.c_str(),
            Module::wifiSSID.length() + 1);
    strncpy((char *) Module::wifi_config_sta.sta.password, Module::wifiPSK.c_str(),
            Module::wifiPSK.length() + 1);
    strncpy((char *) Module::wifi_config_ap.ap.ssid, Module::dmi_name.c_str(),
            Module::dmi_name.length() + 1);
    Module::wifi_config_ap.ap.ssid_len = Module::dmi_name.length();
    Module::wifi_config_ap.ap.channel = Module::channel;
    strncpy((char *) Module::wifi_config_ap.ap.password, Module::APpasswd.c_str(), 
            Module::APpasswd.length() + 1);
    Module::wifi_config_ap.ap.max_connection = Module::max_connection;
    Module::wifi_config_ap.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;

    //Initialize NVS
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        }
        ESP_ERROR_CHECK(ret);

    ESP_LOGI("wifi", "Starting WiFi config");
    Module::connect_counter = 0;
    wifi_init();

}

void Module::config_spiffs() {
    spiffs_base_path = "/spiffs";
}

void Module::mount_spiffs() {

    if (!esp_spiffs_mounted(spiffs_config.partition_label)) {
        ESP_LOGI("spiffs", "Initializing SPIFFS");

        spiffs_config.base_path = Module::spiffs_base_path.c_str();
        spiffs_config.max_files = Module::spiffs_max_files;
        spiffs_config.partition_label = NULL;
        spiffs_config.format_if_mount_failed = Module::spiffs_format_if_mount_failed;

        // Use settings defined above to initialize and mount SPIFFS filesystem.
        // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
        esp_err_t ret = esp_vfs_spiffs_register(&spiffs_config);

        if (ret != ESP_OK) {
            if (ret == ESP_FAIL) {
                ESP_LOGE("spiffs", "Failed to mount or format filesystem");
            } else if (ret == ESP_ERR_NOT_FOUND) {
                ESP_LOGE("spiffs", "Failed to find SPIFFS partition");
            } else {
                ESP_LOGE("spiffs", "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
            }
            return;
        }

        size_t total = 0, used = 0;
        ret = esp_spiffs_info(spiffs_config.partition_label, &total, &used);
        if (ret != ESP_OK) {
            ESP_LOGE("spiffs", "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
        } else {
            ESP_LOGI("spiffs", "Partition size: total: %d, used: %d", total, used);
        }
    } else {
        ESP_LOGI("spiffs", "SPIFFS already initialized");
    }
}

void Module::unmount_spiffs() {
    // All done, unmount partition and disable SPIFFS
    if (esp_spiffs_mounted(spiffs_config.partition_label)) {
        esp_vfs_spiffs_unregister(spiffs_config.partition_label);
        ESP_LOGI("spiffs", "SPIFFS unmounted");
    } else {
        ESP_LOGI("spiffs", "SPIFFS not found");
    }
}

void Module::read_json(int32_t firmware_number) { // Deserialize
                                                  // also copying firmware number 
                                                  // for future access
    
    ESP_LOGI("json", "Mounting FS");
    Module::mount_spiffs();

    ESP_LOGI("json", "Opening json file");
    FILE* f = fopen("/spiffs/config.json", "r");
    if (f == NULL) {
        ESP_LOGE("json", "Failed to open file");
        return;
    }

    ESP_LOGI("json", "Reading json file");
    std::ifstream in("/spiffs/config.json");
    std::string contents((std::istreambuf_iterator<char>(in)), 
    std::istreambuf_iterator<char>());

    ESP_LOGI("json", "Getting data");
    cJSON *root = cJSON_Parse(contents.c_str());
    if (cJSON_GetObjectItem(root, "device")) {
        Module::device = cJSON_GetObjectItem(root,"device")->valuestring;
    }
    if (cJSON_GetObjectItem(root, "id")) {
        Module::id = cJSON_GetObjectItem(root,"id")->valueint;
    }
    if (cJSON_GetObjectItem(root, "author")) {
        Module::author = cJSON_GetObjectItem(root,"author")->valuestring;
    }
    if (cJSON_GetObjectItem(root, "institution")) {
        Module::institution = cJSON_GetObjectItem(root,"institution")->valuestring;
    }
    if (cJSON_GetObjectItem(root, "color")) {
        Module::color = cJSON_GetObjectItem(root,"color")->valuestring;
    }
    if (cJSON_GetObjectItem(root, "APpasswd")) {
        Module::APpasswd = cJSON_GetObjectItem(root,"APpasswd")->valuestring;
    }
    if (cJSON_GetObjectItem(root, "wifiSSID")) {
        Module::wifiSSID = cJSON_GetObjectItem(root,"wifiSSID")->valuestring;
    }
    if (cJSON_GetObjectItem(root, "wifiPSK")) {
        Module::wifiPSK = cJSON_GetObjectItem(root,"wifiPSK")->valuestring;
    }
    if (cJSON_GetObjectItem(root, "oscIP1")) {
        Module::oscIP1 = cJSON_GetObjectItem(root,"oscIP1")->valuestring;
    }
    if (cJSON_GetObjectItem(root, "oscPORT1")) {
        Module::oscPORT1 = cJSON_GetObjectItem(root,"oscPORT1")->valueint;
    }
    if (cJSON_GetObjectItem(root, "oscIP2")) {
        Module::oscIP2 = cJSON_GetObjectItem(root,"oscIP2")->valuestring;
    }
    if (cJSON_GetObjectItem(root, "oscPORT2")) {
        Module::oscPORT2 = cJSON_GetObjectItem(root,"oscPORT2")->valueint;
    }
    if (cJSON_GetObjectItem(root, "localPORT")) {
        Module::localPORT = cJSON_GetObjectItem(root,"localPORT")->valueint;
    }
    if (cJSON_GetObjectItem(root, "Tstick_FSRoffset")) {
        Module::Tstick_FSRoffset = cJSON_GetObjectItem(root,"Tstick_FSRoffset")->valueint;
    }
    
    ESP_LOGI("json", "Data collected:");
    printf("device: %s\n", device.c_str());
    printf("id: %i\n", id);
    printf("author: %s\n", author.c_str());
    printf("institution: %s\n", institution.c_str());
    printf("color: %s\n", color.c_str());
    printf("APpasswd: %s\n", APpasswd.c_str());
    printf("wifiSSID: %s\n", wifiSSID.c_str());
    printf("wifiPSK: %s\n", wifiPSK.c_str());
    printf("oscIP1: %s\n", oscIP1.c_str());
    printf("oscPORT1: %i\n", oscPORT1);
    printf("oscIP2: %s\n", oscIP2.c_str());
    printf("oscPORT2: %i\n", oscPORT2);
    printf("localPORT: %i\n", localPORT);
    printf("Tstick_FSRoffset: %i\n\n", Tstick_FSRoffset);
    
    cJSON_Delete(root);

    std::stringstream tempBuf;
    tempBuf << Module::device << "_" << std::setfill('0') << std::setw(3) << Module::id;
    Module::dmi_name = tempBuf.str();
    printf("Device unique name defined: %s\n",dmi_name.c_str());

    Module::firmware = firmware_number;

    Module::unmount_spiffs();
}

// TODO: Check favicon, icon not being sent
esp_err_t Module::favicon_get_handler(httpd_req_t *req) {

    Module::mount_spiffs();
    const char* resp_str = (const char*) req->user_ctx;
    //std::string temp(resp_str);
    FILE* file = fopen(resp_str, "rb");
    if(file) {
        long int size = ftell(file);
        unsigned char * in = (unsigned char *) malloc(size);
        fread(in, sizeof(unsigned char), size, file);
        httpd_resp_send(req, (const char *)in, size);
        free(in);
        Module::unmount_spiffs();
    } else {ESP_LOGI("http (spiffs)", "Favicon file is invalid");}
    fclose(file);
    
    return ESP_OK;
}

esp_err_t Module::index_get_handler(httpd_req_t *req) {

    const char* resp_str = (const char*) req->user_ctx;
    Module::mount_spiffs();
    ESP_LOGI("http (spiffs)", "Reading index file");
    std::ifstream in(resp_str);
    std::string contents((std::istreambuf_iterator<char>(in)), 
    std::istreambuf_iterator<char>());
    //httpd_resp_set_type(req, "text/html");

    // Put the module info on the HTML before send response
    Module::find_and_replace("%DMINAME%", Module::dmi_name, contents);
    if (Module::STA_is_connected) {
        Module::find_and_replace("%STATUS%", "Currently connected on "
                                             "<strong style=\"color:Tomato;\">" + 
                                             Module::wifiSSID + "</strong> network", 
                                             contents);
    } else {
        Module::find_and_replace("%STATUS%", "Currently not connected to any network", contents);
    }
    Module::find_and_replace("%CURRENTSSID%", Module::wifiSSID, contents);
    Module::find_and_replace("%CURRENTPSK%", Module::wifiPSK, contents);
    Module::find_and_replace("%DEVICENAME%", Module::device, contents);
    Module::find_and_replace("%CURRENTOSC1%", Module::oscIP1, contents);
    Module::find_and_replace("%CURRENTPORT1%", Module::oscPORT1, contents);
    Module::find_and_replace("%CURRENTOSC2%", Module::oscIP2, contents);
    Module::find_and_replace("%CURRENTPORT2%", Module::oscPORT1, contents);
    Module::find_and_replace("%CURRENTSSID2%", Module::wifiSSID, contents);
    Module::find_and_replace("%CURRENTIP%", Module::current_STA_IP, contents);
    Module::find_and_replace("%CURRENTAPIP%", Module::current_AP_IP, contents);
    Module::find_and_replace("%CURRENTSTAMAC%", Module::current_STA_MAC, contents);
    Module::find_and_replace("%CURRENTAPMAC%", Module::current_AP_MAC, contents);
    std::ostringstream tempBuf;
    tempBuf << std::setfill('0') << std::setw(3) << std::hex << Module::id;
    Module::find_and_replace("%MODULEID%", tempBuf.str(), contents);
    Module::find_and_replace("%MODULEAUTH%", Module::author, contents);
    Module::find_and_replace("%MODULEINST%", Module::institution, contents);
    Module::find_and_replace("%MODULEVER%", Module::firmware, contents);

    httpd_resp_sendstr(req, contents.c_str());
    
    Module::unmount_spiffs();

    return ESP_OK;
}

// esp_err_t Module::index_post_handler(httpd_req_t *req)
// {
//     char buf[200];
//     int ret, remaining = req->content_len;

//     while (remaining > 0) {
//         /* Read the data for the request */
//         if ((ret = httpd_req_recv(req, buf,
//                         MIN(remaining, sizeof(buf)))) <= 0) {
//             if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
//                 /* Retry receiving if timeout occurred */
//                 continue;
//             }
//             return ESP_FAIL;
//         }

//         /* Send back the same data */
//         httpd_resp_send_chunk(req, buf, ret);
//         remaining -= ret;

//         /* Log data received */
//         ESP_LOGI("http (spiffs)", "=========== RECEIVED DATA ==========");
//         ESP_LOGI("http (spiffs)", "%.*s", ret, buf);
//         ESP_LOGI("http (spiffs)", "====================================");
//     }

//     // End response
//     httpd_resp_send_chunk(req, NULL, 0);
//     return ESP_OK;
// }

esp_err_t Module::style_get_handler(httpd_req_t *req) {

    const char* resp_str = (const char*) req->user_ctx;
    Module::mount_spiffs();
    ESP_LOGI("http (spiffs)", "Reading style.css file");
    std::ifstream in(resp_str);
    std::string contents((std::istreambuf_iterator<char>(in)), 
    std::istreambuf_iterator<char>());
    httpd_resp_set_type(req, "text/css");
    httpd_resp_sendstr(req, contents.c_str());
    
    Module::unmount_spiffs();

    return ESP_OK;
}

esp_err_t Module::factory_get_handler(httpd_req_t *req) {

    const char* resp_str = (const char*) req->user_ctx;
    Module::mount_spiffs();
    ESP_LOGI("http (spiffs)", "Reading factory.html file");
    std::ifstream in(resp_str);
    std::string contents((std::istreambuf_iterator<char>(in)), 
    std::istreambuf_iterator<char>());
    //httpd_resp_set_type(req, "text/html");
    httpd_resp_sendstr(req, contents.c_str());
    
    Module::unmount_spiffs();

    return ESP_OK;
}

esp_err_t Module::reboot_get_handler(httpd_req_t *req) {

    const char* resp_str = (const char*) req->user_ctx;
    Module::mount_spiffs();
    ESP_LOGI("http (spiffs)", "Reading reboot.html file");
    std::ifstream in(resp_str);
    std::string contents((std::istreambuf_iterator<char>(in)), 
    std::istreambuf_iterator<char>());
    //httpd_resp_set_type(req, "text/html");
    httpd_resp_sendstr(req, contents.c_str());
    
    Module::unmount_spiffs();

    return ESP_OK;
}

esp_err_t Module::scan_get_handler(httpd_req_t *req) {

    const char* resp_str = (const char*) req->user_ctx;
    Module::mount_spiffs();
    ESP_LOGI("http (spiffs)", "Reading scan.html file");
    std::ifstream in(resp_str);
    std::string contents((std::istreambuf_iterator<char>(in)), 
    std::istreambuf_iterator<char>());
    //httpd_resp_set_type(req, "text/html");
    httpd_resp_sendstr(req, contents.c_str());
    
    Module::unmount_spiffs();

    return ESP_OK;
}

esp_err_t Module::update_get_handler(httpd_req_t *req) {

    const char* resp_str = (const char*) req->user_ctx;
    Module::mount_spiffs();
    ESP_LOGI("http (spiffs)", "Reading update.html file");
    std::ifstream in(resp_str);
    std::string contents((std::istreambuf_iterator<char>(in)), 
    std::istreambuf_iterator<char>());
    //httpd_resp_set_type(req, "text/html");
    httpd_resp_sendstr(req, contents.c_str());
    
    Module::unmount_spiffs();

    return ESP_OK;
}

/* An HTTP POST handler */
esp_err_t Module::echo_post_handler(httpd_req_t *req)
{
    char buf[200];
    char * token;
    char * field;

    int api_return, remaining = req->content_len;

    while (remaining > 0) {
        /* Read the data for the request */
        if ((api_return = httpd_req_recv(req, buf,
                        MIN(remaining, sizeof(buf)))) <= 0) {
            if (api_return == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry receiving if timeout occurred */
                continue;
            }
            return ESP_FAIL;
        }

        /* Log data received */
        ESP_LOGI("http (spiffs)", "=========== RECEIVED DATA ==========");
        ESP_LOGI("http (spiffs)", "%.*s", api_return, buf);
        ESP_LOGI("http (spiffs)", "====================================");

        token = strtok(buf,"&");

        if (token != NULL) {
            field = strtok(token,"=");

        }

        
        printf("%s\n", field_data.c_str());

        /* Send back the same data */
        httpd_resp_send_chunk(req, buf, api_return);
        remaining -= api_return;


    }

    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

void Module::find_and_replace(std::string old_text, std::string new_text, std::string & str) {

    std::size_t old_text_position = str.find(old_text);
    if (old_text_position!=std::string::npos) {
        str.replace(old_text_position,old_text.length(),new_text);
        ESP_LOGI("http (find_and_replace)", "Success");
    } else {
        ESP_LOGI("http (find_and_replace)", "Could not find the requested string");
    }
}

void Module::find_and_replace(std::string old_text, uint32_t new_number, std::string & str) {

    std::size_t old_text_position = str.find(old_text);
    if (old_text_position!=std::string::npos) {
        std::string conversion = std::to_string(new_number);
        str.replace(old_text_position,old_text.length(),conversion);
        ESP_LOGI("http (find_and_replace)", "Success");
    } else {
        ESP_LOGI("http (find_and_replace)", "Could not find the requested string");
    }
}

httpd_handle_t Module::start_webserver(void) {
    
    Module::webserver = NULL;

    Module::webserver_config.task_priority      = tskIDLE_PRIORITY+5;
    Module::webserver_config.stack_size         = 4096;
    Module::webserver_config.core_id            = tskNO_AFFINITY;
    Module::webserver_config.server_port        = 80;
    Module::webserver_config.ctrl_port          = 32768;
    Module::webserver_config.max_open_sockets   = 7;
    Module::webserver_config.max_uri_handlers   = 8;
    Module::webserver_config.max_resp_headers   = 8;
    Module::webserver_config.backlog_conn       = 5;
    Module::webserver_config.lru_purge_enable   = true;
    Module::webserver_config.recv_wait_timeout  = 5;
    Module::webserver_config.send_wait_timeout  = 5;
    Module::webserver_config.global_user_ctx = NULL;
    Module::webserver_config.global_user_ctx_free_fn = NULL;
    Module::webserver_config.global_transport_ctx = NULL;
    Module::webserver_config.global_transport_ctx_free_fn = NULL;
    Module::webserver_config.open_fn = NULL;
    Module::webserver_config.close_fn = NULL;
    Module::webserver_config.uri_match_fn = NULL;

    Module::index.uri = "/";
    Module::index.method    = HTTP_GET,
    Module::index.handler   = index_get_handler,
    Module::index.user_ctx  = (char*)"/spiffs/index.html";

    Module::indexpost.uri = "/";
    Module::indexpost.method    = HTTP_POST,
    Module::indexpost.handler   = echo_post_handler,
    Module::indexpost.user_ctx  = (char*)"/spiffs/index.html";

    Module::favicon.uri = "/favicon.ico";
    Module::favicon.method    = HTTP_GET,
    Module::favicon.handler   = favicon_get_handler,
    Module::favicon.user_ctx  = (char*)"/spiffs/favicon.ico";

    Module::style.uri = "/style.css";
    Module::style.method    = HTTP_GET,
    Module::style.handler   = style_get_handler,
    Module::style.user_ctx  = (char*)"/spiffs/style.css";

    Module::factory.uri = "/factory.html";
    Module::factory.method    = HTTP_GET,
    Module::factory.handler   = factory_get_handler,
    Module::factory.user_ctx  = (char*)"/spiffs/factory.html";

    Module::reboot.uri = "/reboot.html";
    Module::reboot.method    = HTTP_GET,
    Module::reboot.handler   = reboot_get_handler,
    Module::reboot.user_ctx  = (char*)"/spiffs/reboot.html";

    Module::scan.uri = "/scan.html";
    Module::scan.method    = HTTP_GET,
    Module::scan.handler   = scan_get_handler,
    Module::scan.user_ctx  = (char*)"/spiffs/scan.html";

    Module::update.uri = "/update.html";
    Module::update.method    = HTTP_GET,
    Module::update.handler   = update_get_handler,
    Module::update.user_ctx  = (char*)"/spiffs/update.html";

    // Start the httpd server
    ESP_LOGI("webserver", "Starting server on port: '%d'", webserver_config.server_port);
    if (httpd_start(&webserver, &webserver_config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI("webserver", "Registering URI handlers");
        httpd_register_uri_handler(webserver, &index);
        httpd_register_uri_handler(webserver, &indexpost);
        httpd_register_uri_handler(webserver, &favicon);
        httpd_register_uri_handler(webserver, &style);
        httpd_register_uri_handler(webserver, &scan);
        httpd_register_uri_handler(webserver, &factory);
        httpd_register_uri_handler(webserver, &reboot);
        httpd_register_uri_handler(webserver, &update);
        return webserver;
    }

    ESP_LOGI("webserver", "Error starting server!");
    return NULL;
}

void Module::stop_webserver(void) {
    // Stop the httpd server
    httpd_stop(webserver);
}

bool Module::set_wifiSSID(std::string &SSID) {
    Module::wifiSSID = SSID;

    return 1;
}

bool Module::set_wifiSSID(const char *SSID) {
    Module::wifiSSID = SSID;

    return 1;
}

bool Module::set_wifiPSK(std::string &PSK) {
    Module::wifiPSK = PSK;

    return 1;
}

bool Module::set_wifiPSK(const char *PSK) {
    Module::wifiPSK = PSK;

    return 1;
}