#ifndef SC19_CODE_TEST_SC_SC_PICO_HPP_
#define SC19_CODE_TEST_SC_SC_PICO_HPP_

/*************************************
 *************************************


このファイルは見なくてかまいません
内部の実装を知りたい場合のみ見てください


*************************************
*************************************/

#include <set>
#include <deque>
#include <algorithm>

#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/spi.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"

#include "sc.hpp"

//! @file sc_pico.hpp
//! @brief picoに関するプログラム
//! @date 2023-10-28T00:37


namespace pico
{
    //! @brief picoの汎用入出力
    class PinIO : public sc::PinIO
    {
        const uint8_t _pin_gpio;  // ピンのGPIO番号
    public:
        PinIO(uint8_t pin_gpio, Direction direction, Pull pull = Pull::no_use);
        bool read() const override;
        void write(bool level) const override;
    private:
        void init_pin(uint8_t pin_gpio) const;
        void set_direction(Direction direction) const;
        void set_pull(Pull pull) const;
    };

    //! @brief picoのI2C通信
    class I2C : public sc::I2C
    {
    public:
        //! @brief I2C通信で使用するピンの番号
        class Pin
        {
            const uint8_t _sda_gpio;  // SDAピンのGPIO番号
            const uint8_t _scl_gpio;  // SCLピンのGPIO番号
        public:
            Pin(uint8_t sda_gpio, uint8_t scl_gpio);
            uint8_t get_sda_gpio() const;
            uint8_t get_scl_gpio() const;
            bool get_i2c_id() const;
        };
    private:
        const bool _i2c_id;  // I2C0かI2C1か
        const Pin _i2c_pin;  // I2Cで使用しているピン
        const uint32_t _freq;  // 周波数 (/s)
    public:
        I2C(Pin i2c_pin, uint32_t freq);
        sc::Binary read(std::size_t size, SlaveAddr slave_addr) const override;
        sc::Binary read_mem(std::size_t size, SlaveAddr slave_addr, MemoryAddr memory_addr) const override;
        void write(sc::Binary output_data, SlaveAddr slave_addr) const override;
        void write_mem(sc::Binary output_data, SlaveAddr slave_addr, MemoryAddr memory_addr) const override;
    private:
        void init_i2c();
        void set_i2c_pin();
    };

    //! @brief picoのSPI通信
    class SPI : public sc::SPI
    {
    public:
        class Pin
        {
            const uint8_t _miso_gpio;
            const uint8_t _mosi_gpio;
            const uint8_t _sck_gpio;
            const std::vector<uint8_t> _cs_gpios;
            static constexpr uint8_t MinPinGpio = 0;
            static constexpr uint8_t MaxPinGpio = 28;
        public:
            Pin(uint8_t miso_gpio, uint8_t mosi_gpio, uint8_t sck_gpio, std::initializer_list<uint8_t> cs_gpios);
            uint8_t get_miso_gpio() const;
            uint8_t get_mosi_gpio() const;
            uint8_t get_sck_gpio() const;
            std::vector<uint8_t> get_cs_gpios() const;
            bool get_spi_id() const;
        };
    private:
        const bool _spi_id;
        const Pin _spi_pin;
        const uint32_t _freq;
    public:
        SPI(Pin spi_pin, uint32_t freq);
        sc::Binary read(std::size_t size, CS_Pin cs_pin) const override;
        sc::Binary read_mem(std::size_t size, CS_Pin cs_pin, MemoryAddr memory_addr) const override;
        void write(sc::Binary output_data, CS_Pin cs_pin) const override;
        void write_mem(sc::Binary output_data, CS_Pin cs_pin, MemoryAddr memory_addr) const override;
    private:
        void init_spi();
        void set_spi_pin();
        void select_cs(CS_Pin cs_pin) const;
        void deselect_cs(CS_Pin cs_pin) const;
    };


    //! @brief picoのUART通信
    class UART : public sc::UART
    {
    public:
        //! @brief UART通信で使用するピンの番号
        class Pin
        {
            const uint8_t _tx_gpio;  // TXピンのGPIO番号
            const uint8_t _rx_gpio;  // RXピンのGPIO番号
        public:
            Pin(uint8_t tx_gpio, uint8_t rx_gpio);
            uint8_t get_tx_gpio() const;
            uint8_t get_rx_gpio() const;
            bool get_uart_id() const;
        };
    private:
        const bool _uart_id;  // UART0かUART1か
        const Pin _uart_pin;  // UARTで使用しているピン
        const uint32_t _freq;  // 周波数 (/s)
    public:
        UART(Pin uart_pin, uint32_t freq);
        sc::Binary read() const override;
        sc::Binary read(std::size_t size) const override;
        void write(sc::Binary output_data) const override;
    private:
        void init_uart();
        void set_uart_pin();
        void set_irq();
    public:
        static std::deque<uint8_t> uart0_input_data;
        static std::deque<uint8_t> uart1_input_data;
        static void uart0_handler();
        static void uart1_handler();
    };

    //! @brief picoのPWM
    class PWM : public sc::PWM
    {
    public:
        PWM(uint8_t pin_gpio, uint32_t freq);  // 未実装

        virtual void set_freq(uint32_t freq) override;  // 未実装

        void set_level(float output_level) override;  // 未実装
    };

    class SD : sc::SD
    {
        // 未実装
    };
}

#endif  // SC19_CODE_TEST_SC_SC_PICO_HPP_