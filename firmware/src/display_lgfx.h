#pragma once
// ============================================================
// display_lgfx.h — Configuration LovyanGFX pour ST7789 240x320
// Smart Escape Box | YD-ESP32-S3 N16R8
// ============================================================
#include <LovyanGFX.hpp>
#include "config.h"

class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ST7789  _panel;
    lgfx::Bus_SPI       _bus;
    lgfx::Light_PWM     _light;

public:
    LGFX() {
        // ── Bus SPI ──────────────────────────────────────────
        {
            auto cfg         = _bus.config();
            cfg.spi_host     = SPI2_HOST;
            cfg.spi_mode     = 3;
            cfg.freq_write   = 80000000UL;
            cfg.freq_read    = 16000000UL;
            cfg.pin_sclk     = PIN_TFT_SCLK;
            cfg.pin_mosi     = PIN_TFT_MOSI;
            cfg.pin_miso     = -1;
            cfg.pin_dc       = PIN_TFT_DC;
            cfg.dma_channel  = SPI_DMA_CH_AUTO;
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }
        // ── Panel ST7789 ─────────────────────────────────────
        {
            auto cfg         = _panel.config();
            cfg.pin_cs       = PIN_TFT_CS;
            cfg.pin_rst      = PIN_TFT_RST;
            cfg.pin_busy     = -1;
            cfg.panel_width  = TFT_W;
            cfg.panel_height = TFT_H;
            cfg.offset_x     = 0;
            cfg.offset_y     = 0;
            cfg.offset_rotation = 0;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits  = 1;
            cfg.readable     = false;
            cfg.invert       = true;   // ST7789 couleurs inversées
            cfg.rgb_order    = false;
            cfg.dlen_16bit   = false;
            cfg.bus_shared   = false;
            _panel.config(cfg);
        }
        // ── Rétroéclairage PWM ───────────────────────────────
        {
            auto cfg         = _light.config();
            cfg.pin_bl       = PIN_TFT_BLK;
            cfg.invert       = false;
            cfg.freq         = 12000;
            cfg.pwm_channel  = 7;
            _light.config(cfg);
            _panel.setLight(&_light);
        }
        setPanel(&_panel);
    }
};
