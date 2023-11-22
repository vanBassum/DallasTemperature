#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_mac.h>
#include <string>
#include "onewiremanager.h"
#include "influx.h"
#include "startup.h"
#include "config.h"


#define TAG					  "MAIN"

//#define INFLUXDB_URL            "MyData"
//#define INFLUXDB_TOKEN          "MyData"
//#define INFLUXDB_ORG            "MyData"
//#define INFLUXDB_BUCKET         "MyData"
//#define WIFI_SSID 			  "MyData"
//#define WIFI_PASS 			  "MyData"
//
//#define GPIO_DS18B20_DATA       GPIO_NUM_12
//#define GPIO_DS18B20_VCC        GPIO_NUM_14
//#define SAMPLE_PERIOD           1000   // milliseconds



std::string readMacAddress() {
    uint8_t macRaw[6];
    esp_read_mac(macRaw, ESP_MAC_WIFI_STA);

    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             macRaw[0], macRaw[1], macRaw[2], macRaw[3], macRaw[4], macRaw[5]);

    return std::string(macStr);
}



void Test(void *pvParameters)
{	
    // Use a gpio pin for VCC, not a great idea, but good enough for now.
#ifdef GPIO_DS18B20_VCC
    gpio_reset_pin(GPIO_DS18B20_VCC);
    gpio_set_direction(GPIO_DS18B20_VCC, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_DS18B20_VCC, 1);
#endif

	Influx::Client client(INFLUXDB_URL, INFLUXDB_TOKEN, INFLUXDB_ORG, INFLUXDB_BUCKET);
    OneWireManager oneWireManager(GPIO_DS18B20_DATA, RMT_CHANNEL_1, RMT_CHANNEL_0);
    std::vector<std::shared_ptr<DS18B20Wrapper>> devices;


    std::string mac = readMacAddress();
    ESP_LOGI(TAG, "Device MAC: %s", mac.c_str());

    // Periodically update and read sensors
    while (true)
    {
        oneWireManager.findAndUpdateDevices(devices);
        oneWireManager.startConversion();

        // Access sensors and perform other operations as needed
        for (auto sensor : devices)
        {
            float temperature = 0;
            if(sensor->readTemperature(&temperature) == DS18B20_OK)
            {
                ESP_LOGI(TAG, "Temperature: %.2f", temperature);
                Influx::Point("Dallas")
                    .AddTag("ESP-MAC", mac)	
                    .AddTag("Dallas-ROMCode", sensor->ToString())		
                    .AddField("Temperature", temperature)
                    .SetTimestampNow()
                    .Post(client);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_PERIOD));
    }







    //OneWireBus_ROMCode devices[MAX_DEVICES];
    //DS18B20_Info * devices[MAX_DEVICES] = {0};
//
	//while(1)
    //{
    //    int num_devices = FindDevices(devices, MAX_DEVICES);
//
    //    for (int i = 0; i < num_devices; ++i)
    //    {
    //        DS18B20_Info * ds18b20_info = ds18b20_malloc();  // heap allocation
    //        devices[i] = ds18b20_info;
//
    //        if (num_devices == 1)
    //        {
    //            printf("Single device optimisations enabled\n");
    //            ds18b20_init_solo(ds18b20_info, owb);          // only one device on bus
    //        }
    //        else
    //        {
    //            ds18b20_init(ds18b20_info, owb, device_rom_codes[i]); // associate with bus and device
    //        }
    //        ds18b20_use_crc(ds18b20_info, true);           // enable CRC check on all reads
    //        ds18b20_set_resolution(ds18b20_info, DS18B20_RESOLUTION);
    //    }
//
//
    //    Influx::Point("Dallas")
    //        .AddTag("ID", "0")		
    //        .AddField("Temperature", 12.6f)
    //        .SetTimestampNow()
    //        .Post(client);
    //    
    //    
    //}

	ESP_LOGI(TAG, "Out of work");	
	while(1)
		vTaskDelay(pdMS_TO_TICKS(1000));
}

extern "C" void app_main(void)
{
    init_flash();
    init_wifi(WIFI_SSID, WIFI_PASS, true);
    init_timezone("CEST");
    init_sntp("pool.ntp.org");

	xTaskCreate(&Test, "Test", 8192, NULL, 5, NULL);

	while(1)
		vTaskDelay(pdMS_TO_TICKS(1000));
}


// 'Dallas,ID=0 Temperature=12.600000 1700653008000000000'
//                                    1697727036000000000
