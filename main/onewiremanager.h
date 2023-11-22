#pragma once
#include "ds18b20wrapper.h"
#include <vector>
#include <memory>
#include <driver/gpio.h>
#include <string.h>
#include <algorithm>

class OneWireManager {
    static constexpr const char* TAG = "OneWireManager";
    std::shared_ptr<OneWireBus> owb_;
    owb_rmt_driver_info rmt_driver_info_;

    static bool isSensorInVector(const OneWireBus_ROMCode& findCode, const std::vector<OneWireBus_ROMCode> &device_rom_codes)
    {
        auto it = std::find_if(device_rom_codes.begin(), device_rom_codes.end(),
            [findCode](const auto &code) { 
                return memcmp(code.bytes, findCode.bytes, 8) == 0; 
                });

        return it != device_rom_codes.end();
    }

    static bool isSensorInVector(const OneWireBus_ROMCode& findCode, const std::vector<std::shared_ptr<DS18B20Wrapper>> &devices)
    {
        auto it = std::find_if(devices.begin(), devices.end(),
            [findCode](const auto &sensor) { 
                return memcmp(sensor->getROMCode().bytes, findCode.bytes, 8) == 0; 
                });

        return it != devices.end();
    }

public:
    OneWireManager(gpio_num_t gpio_pin, rmt_channel_t rmt_channel_1, rmt_channel_t rmt_channel_0)
        : owb_(nullptr, &owb_uninitialize)
    {
         

        // Initialize the 1-Wire bus
        owb_ = std::shared_ptr<OneWireBus>(
            owb_rmt_initialize(&rmt_driver_info_, gpio_pin, rmt_channel_1, rmt_channel_0),
            &owb_uninitialize);
        owb_use_crc(owb_.get(), true);
    }

    ~OneWireManager()
    {
        // owb_ is automatically cleaned up due to std::shared_ptr
    }

    std::shared_ptr<DS18B20Wrapper> createDS18B20Wrapper(const OneWireBus_ROMCode &rom_code)
    {
        return std::make_shared<DS18B20Wrapper>(owb_, rom_code);
    }

    void findAndUpdateDevices(std::vector<std::shared_ptr<DS18B20Wrapper>> &devices)
    {
        std::vector<OneWireBus_ROMCode> device_rom_codes;
        OneWireBus_SearchState search_state = {0};
        bool found = false;
        owb_search_first(owb_.get(), &search_state, &found);
        while (found)
        {
            device_rom_codes.push_back(search_state.rom_code);
            owb_search_next(owb_.get(), &search_state, &found);
        }
        // Update the vector of DS18B20 sensors
        updateSensors(device_rom_codes, devices);
    }

    void updateSensors(const std::vector<OneWireBus_ROMCode> &device_rom_codes, std::vector<std::shared_ptr<DS18B20Wrapper>> &devices)
    {
        // Remove sensors that are no longer present
        devices.erase(std::remove_if(devices.begin(), devices.end(),
            [&device_rom_codes](const auto &sensor) {
                bool remove = !isSensorInVector(sensor->getROMCode(), device_rom_codes);
                if(remove)
                    ESP_LOGI(TAG, "Device removed: %s", sensor->ToString().c_str());
                return remove;
            }), devices.end());

        // Add new sensors
        for (const auto &rom_code : device_rom_codes)
        {
            if (!isSensorInVector(rom_code, devices))
            {
                auto newDevice = createDS18B20Wrapper(rom_code);
                devices.push_back(newDevice);
                ESP_LOGI(TAG, "New device found: %s", newDevice->ToString().c_str());
            }
        }
    }

    void startConversion()
    {
        ds18b20_convert_all(owb_.get());
    }
};
