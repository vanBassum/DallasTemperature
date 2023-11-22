#pragma once
#include "dallas/owb.h"
#include "dallas/owb_rmt.h"
#include "dallas/ds18b20.h"
#include <memory>
#include <string>

class DS18B20Wrapper {
    std::shared_ptr<OneWireBus> owb_;
    DS18B20_Info* ds18b20_info_;
    OneWireBus_ROMCode rom_code_;

    static constexpr const char* TAG = "DS18B20Wrapper";

public:
    DS18B20Wrapper(std::shared_ptr<OneWireBus> owb, const OneWireBus_ROMCode &rom_code)
        : owb_(owb)
        , rom_code_(rom_code)
    {

        ds18b20_info_ = ds18b20_malloc();


        ds18b20_init(ds18b20_info_, owb_.get(), rom_code_);
        ds18b20_use_crc(ds18b20_info_, true);
        ds18b20_set_resolution(ds18b20_info_, DS18B20_RESOLUTION_12_BIT);

        bool parasitic_power = false;
        ds18b20_check_for_parasite_power(owb.get(), &parasitic_power);
        if (parasitic_power) {
            printf("Parasitic-powered devices detected");
        }
        owb_use_parasitic_power(owb.get(), parasitic_power);
    }

    ~DS18B20Wrapper()
    {
        ds18b20_free(&ds18b20_info_);
    }

    DS18B20_ERROR readTemperature(float* temperature)
    {
        ds18b20_wait_for_conversion(ds18b20_info_);
        DS18B20_ERROR error = ds18b20_read_temp(ds18b20_info_, temperature);

        if (error != DS18B20_OK)
        {
            ESP_LOGE(TAG, "Error reading temperature: %d", error);
        }

        return error;
    }

    OneWireBus_ROMCode getROMCode() const
    {
        return rom_code_;
    }

    std::string ToString()
    {
        char rom_code_s[17];
        owb_string_from_rom_code(rom_code_, rom_code_s, sizeof(rom_code_s));
        return std::string(rom_code_s);
    }
};
