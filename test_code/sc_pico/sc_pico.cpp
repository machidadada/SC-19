/*************************************
 *************************************


このファイルは見なくてかまいません
内部の実装を知りたい場合のみ見てください


*************************************
*************************************/

#include "sc_pico.hpp"

//! @file sc_pico.cpp
//! @brief picoに関するプログラム
//! @date 2023-10-28T15:47


//! @brief ログを記録する関数です．
//! @param log 書き込む文字列
void sc::Log::write(const std::string& log) noexcept
{
    try
    {
        std::cout << log << std::flush;
    }
    catch(const std::exception& e) {Error(__FILE__, __LINE__, "Failed to save log", e);}  // ログの保存に失敗しました
    catch(...) {Error(__FILE__, __LINE__, "Failed to save log");}  // ログの保存に失敗しました
}

namespace pico
{
    /***** class PinIO *****/

    //! @brief picoの汎用入出力をセットアップ
    PinIO::PinIO(uint8_t pin_gpio, Direction direction, Pull pull):
        _pin_gpio(pin_gpio)
    {
        constexpr uint8_t MinPinGpio = 0;  // GPIOピンの最小の番号
        constexpr uint8_t MaxPinGpio = 28; // GPIOピンの最大の番号

        if (_pin_gpio < MinPinGpio || MaxPinGpio < _pin_gpio)
        {
            throw sc::Error(__FILE__, __LINE__, "Invalid pin_gpio number entered");  // 無効なピンのGPIO番号が入力されました
        }

        init_pin(pin_gpio);
        set_direction(direction);
        set_pull(pull);
    }

    //! @brief 汎用入出力のピンを初期化
    //! @param pin_gpio 初期化するピンのGPIO番号
    void PinIO::init_pin(uint8_t pin_gpio) const
    {
        gpio_init(pin_gpio);  // pico-SDKの関数  ピンの初期化を行う
    }

    //! @brief 入力か出力かを設定
    //! @param direction 入出力の方向
    void PinIO::set_direction(Direction direction) const
    {
        if (direction == Direction::in)
        {
            gpio_set_dir(_pin_gpio, GPIO_IN);  // pico-SDKの関数  通信方向の設定を行う
        } else {
            gpio_set_dir(_pin_gpio, GPIO_OUT);  // pico-SDKの関数  通信方向の設定を行う
        }
    }

    //! @brief プルアップ・プルダウンを設定
    //! @param pull プルアップ・プルダウン・使用しない
    void PinIO::set_pull(Pull pull) const
    {
        switch (pull)
        {
            case Pull::no_use:
            {
                gpio_disable_pulls(_pin_gpio);  // pico-SDKの関数  プルアップ・プルダウンを無効化
                break;
            }
            case Pull::up:
            {
                gpio_pull_up(_pin_gpio);
                break;
            }
            case Pull::down:
            {
                gpio_pull_down(_pin_gpio);
                break;
            }
        }
    }

    //! @brief 入力用ピンから読み込み
    //! @return High(1)かLow(0)か
    bool PinIO::read() const
    {
        return gpio_get(_pin_gpio);  // pico-SDKの関数  ピンがHighになっているかLowになっているかを取得する
    }

    //! @brief 出力用ピンに書き込み
    //! @param level High(1)かLow(0)か
    void PinIO::write(bool level) const
    {
        gpio_put(_pin_gpio, level);  // pico-SDKの関数  ピンにHighかLowを出力する
    }


    /***** class I2C *****/

    //! @brief I2C通信で使うピン番号をセットアップ
    //! @param sda_gpio SDAピンのGPIO番号
    //! @param scl_gpio SCLピンのGPIO番号
    I2C::Pin::Pin(uint8_t sda_gpio, uint8_t scl_gpio):
        _sda_gpio(sda_gpio),
        _scl_gpio(scl_gpio)
    {
        const std::set<uint8_t> PossibleSDA_0 = {0, 4, 8, 12, 16, 20};  // I2C0のSDAのピン番号が取りうる値
        const std::set<uint8_t> PossibleSCL_0 = {1, 5, 9, 13, 17, 21};  // I2C0のSCLのピン番号が取りうる値
        const std::set<uint8_t> PossibleSDA_1 = {2, 6, 10, 14, 18, 26};  // I2C1のSDAのピン番号が取りうる値
        const std::set<uint8_t> PossibleSCL_1 = {3, 7, 11, 15, 19, 20};  // I2C1のSCLのピン番号が取りうる値

        // Raspberry Pi PicoでI2C用として使用できないピンの場合はエラー
        if (PossibleSDA_0.count(_sda_gpio) && PossibleSCL_0.count(_scl_gpio))
    return;
        if (PossibleSDA_1.count(_sda_gpio) && PossibleSCL_1.count(_scl_gpio))
    return;
        throw sc::Error(__FILE__, __LINE__, "Invalid i2c gpio number entered");  // 無効なI2CのGPIO番号が入力されました
    }

    //! @brief SDAピンのGPIO番号を取得
    uint8_t I2C::Pin::get_sda_gpio() const
    {
        return _sda_gpio;
    }

    //! @brief SCLピンのGPIO番号を取得
    uint8_t I2C::Pin::get_scl_gpio() const
    {
        return _scl_gpio;
    }

    //! @brief I2C0かI2C1かを取得
    bool I2C::Pin::get_i2c_id() const
    {
        return static_cast<bool>(_sda_gpio % 4);
    }

    //! @brief I2Cのセットアップ
    //! @param i2c_id I2C0かI2C1か
    I2C::I2C(Pin i2c_pin, uint32_t freq):
        _i2c_id(i2c_pin.get_i2c_id()),
        _i2c_pin(i2c_pin),
        _freq(freq)
    {
        init_i2c();
        set_i2c_pin();
    }

    //! @brief I2C通信を初期化する
    void I2C::init_i2c()
    {
        i2c_init((_i2c_id ? i2c1 : i2c0), _freq);  // pico-SDKの関数  I2Cを初期化する
    }

    //! @brief ピンをI2Cとして有効化する
    void I2C::set_i2c_pin()
    {
        gpio_set_function(_i2c_pin.get_sda_gpio(), GPIO_FUNC_I2C);  // pico-SDKの関数  ピンの機能をI2Cモードにする
        gpio_pull_up(_i2c_pin.get_sda_gpio());  // pico-SDKの関数  プルアップ抵抗を有効にする
        gpio_set_function(_i2c_pin.get_scl_gpio(), GPIO_FUNC_I2C);  // pico-SDKの関数  ピンの機能をI2Cモードにする
        gpio_pull_up(_i2c_pin.get_scl_gpio());  // pico-SDKの関数  プルアップ抵抗を有効にする
    }

    //! @brief I2Cによる受信
    //! @param size 受信するバイト数
    //! @param slave_addr 通信先のデバイスのスレーブアドレス
    //! @return Binary型のバイト列
    sc::Binary I2C::read(std::size_t size, SlaveAddr slave_addr) const
    {
        std::vector<uint8_t> input_data(size);
        i2c_read_blocking((_i2c_id ? i2c1 : i2c0), slave_addr.get(), input_data.data(), size, false);
        return sc::Binary(input_data);
    }

    //! @brief I2Cによるメモリからの受信
    //! @param size 受信するバイト数
    //! @param slave_addr 通信先のデバイスのスレーブアドレス
    //! @param memory_addr 通信先のデバイス内のメモリアドレス
    //! @return Binary型のバイト列
    sc::Binary I2C::read_mem(std::size_t size, SlaveAddr slave_addr, MemoryAddr memory_addr) const
    {
        std::vector<uint8_t> input_data(size);
        const uint8_t memory_addr_num = memory_addr.get();
        i2c_write_blocking((_i2c_id ? i2c1 : i2c0), slave_addr.get(), &memory_addr_num, 1, true);
        i2c_read_blocking((_i2c_id ? i2c1 : i2c0), slave_addr.get(), input_data.data(), size, false);
        return sc::Binary(input_data);
    }

    //! @brief I2Cによる送信
    //! @param output_data 送信するデータ
    //! @param slave_addr 通信先のデバイスのスレーブアドレス
    void I2C::write(sc::Binary output_data, SlaveAddr slave_addr) const
    {
        i2c_write_blocking((_i2c_id ? i2c1 : i2c0), slave_addr.get(), output_data.get_raw().data(), output_data.size(), false);
    }

    //! @brief I2Cによるメモリからの送信
    //! @param output_data 送信するデータ
    //! @param slave_addr 通信先のデバイスのスレーブアドレス
    //! @param memory_addr 通信先のデバイス内のメモリアドレス
    void I2C::write_mem(sc::Binary output_data, SlaveAddr slave_addr, MemoryAddr memory_addr) const
    {
        const uint8_t memory_addr_num = memory_addr.get();
        i2c_write_blocking((_i2c_id ? i2c1 : i2c0), slave_addr.get(), &memory_addr_num, 1, true);
        i2c_write_blocking((_i2c_id ? i2c1 : i2c0), slave_addr.get(), output_data.get_raw().data(), output_data.size(), false);
    }


    /***** class SPI *****/

    //! @brief SPI通信で使うピン番号をセットアップ
    //! @param miso_gpio MISO(RX)ピンのGPIO番号  注:接続先デバイスのTX
    //! @param mosi_gpio MOSI(TX)ピンのGPIO番号  注:接続先デバイスのRX
    //! @param sck_gpio SCKピンのGPIO番号
    //! @param cs_gpios CSピンのGPIO番号  { }で囲んで複数入力
    SPI::Pin::Pin(uint8_t miso_gpio, uint8_t mosi_gpio, uint8_t sck_gpio, std::initializer_list<uint8_t> cs_gpios):
        _miso_gpio(miso_gpio),
        _mosi_gpio(mosi_gpio),
        _sck_gpio(sck_gpio),
        _cs_gpios(cs_gpios)
    {
        const std::set<uint8_t> PossibleMISO_0 = {0, 4, 16};  // SPI0のMISOのピン番号が取りうる値
        const std::set<uint8_t> PossibleMOSI_0 = {3, 7, 19};  // SPI0のMOSIのピン番号が取りうる値
        const std::set<uint8_t> PossibleSCK_0 = {2, 6, 18};  // SPI0のSCKピン番号が取りうる値
        const std::set<uint8_t> PossibleMISO_1 = {8, 12};  // SPI1のMISOのピン番号が取りうる値
        const std::set<uint8_t> PossibleMOSI_1 = {11, 15};  // SPI1のMOSIのピン番号が取りうる値
        const std::set<uint8_t> PossibleSCK_1 = {10, 14};  // SPI1のSCKピン番号が取りうる値
        constexpr uint8_t MaxCsGpio = 28;  // CSピンのGPIO番号の最大値

        // Raspberry Pi PicoでSPI用として使用できないピンの場合はエラー        
        if (std::all_of(_cs_gpios.begin(), _cs_gpios.end(), [](uint8_t cs_gpio){return cs_gpio <= MaxCsGpio;}))
        {
            if (PossibleMISO_0.count(_miso_gpio) && PossibleMOSI_0.count(_mosi_gpio) && PossibleSCK_0.count(_sck_gpio))
    return;
            if (PossibleMISO_1.count(_miso_gpio) && PossibleMOSI_1.count(_mosi_gpio) && PossibleSCK_1.count(_sck_gpio))
    return;
        }

        throw sc::Error(__FILE__, __LINE__, "Invalid spi gpio number entered");  // 無効なSPIのGPIO番号が入力されました
    }

    //! @brief MISOピンのGPIO番号を取得
    uint8_t SPI::Pin::get_miso_gpio() const
    {
        return _miso_gpio;
    }

    //! @brief MOSIピンのGPIO番号を取得
    uint8_t SPI::Pin::get_mosi_gpio() const
    {
        return _mosi_gpio;
    }

    //! @brief SCKピンのGPIO番号を取得
    uint8_t SPI::Pin::get_sck_gpio() const
    {
        return _sck_gpio;
    }

    //! @brief CSピンのGPIO番号を取得
    std::vector<uint8_t> SPI::Pin::get_cs_gpios() const
    {
        return _cs_gpios;
    }

    //! @brief SPI0かSPI1かを取得
    bool SPI::Pin::get_spi_id() const
    {
        return static_cast<bool>(_miso_gpio == 8 || _miso_gpio == 12);
    }

    //! @brief SPIのセットアップ
    //! @param spi_id SPI0かSPI1か
    SPI::SPI(Pin spi_pin, uint32_t freq):
        _spi_id(spi_pin.get_spi_id()),
        _spi_pin(spi_pin),
        _freq(freq)
    {
        init_spi();
        set_spi_pin();
    }

    //! SPI通信を初期化する
    void SPI::init_spi()
    {
        spi_init((_spi_id ? spi1 : spi0), _freq);  // pico-SDKの関数  SPIを初期化する
    }

    //! @brief ピンをSPIとして有効化する
    void SPI::set_spi_pin()
    {
        gpio_set_function(_spi_pin.get_mosi_gpio(), GPIO_FUNC_SPI);  // pico-SDKの関数  ピンの機能をSPIモードにする
        gpio_set_function(_spi_pin.get_miso_gpio(), GPIO_FUNC_SPI);  // pico-SDKの関数  ピンの機能をSPIモードにする
        gpio_set_function(_spi_pin.get_sck_gpio(), GPIO_FUNC_SPI);  // pico-SDKの関数  ピンの機能をSPIモードにする
        for (uint8_t cs_gpio : _spi_pin.get_cs_gpios())  // CSピンを設定
        {
            gpio_init(cs_gpio);
            gpio_set_dir(cs_gpio, GPIO_OUT);
            gpio_put(cs_gpio, 1);
        }
    }

    //! @brief 通信先につながるCSピンの出力レベルを0にして通信相手を選択
    //! @param cs_gpio 選択したいCSピン
    void SPI::select_cs(CS_Pin cs_gpio) const
    {
        gpio_put(cs_gpio.get(), 0);
    }

    //! @brief 通信先につながるCSピンの出力レベルを1にして通信相手の選択を解除
    //! @param cs_gpio 選択を解除したいCSピン
    void SPI::deselect_cs(CS_Pin cs_gpio) const
    {
        gpio_put(cs_gpio.get(), 1);
    }

    //! @brief SPIによる受信
    //! @param size 受信するバイト数
    //! @param cs_pin 通信先につながるCSピン
    //! @return Binary型のバイト列
    sc::Binary SPI::read(std::size_t size, CS_Pin cs_pin) const
    {
        std::vector<uint8_t> input_data(size);
        SPI::select_cs(cs_pin);
        spi_read_blocking((_spi_id ? spi1 : spi0), 0U, input_data.data(), size);
        SPI::deselect_cs(cs_pin);
        return sc::Binary(input_data);
    }

    //! @brief SPIによるメモリからの受信
    //! @param size 受信するバイト数
    //! @param cs_pin 通信先につながるCSピン
    //! @param memory_addr 通信先のデバイス内のメモリアドレス
    //! @return Binary型のバイト列
    //! メモリアドレスの8ビット目は自動的に1になります．
    sc::Binary SPI::read_mem(std::size_t size, CS_Pin cs_pin, MemoryAddr memory_addr) const
    {
        std::vector<uint8_t> input_data(size);
        const uint8_t memory_addr_num = memory_addr.get_1();
        SPI::select_cs(cs_pin);
        spi_write_blocking((_spi_id ? spi1 : spi0), &memory_addr_num, 1);
        spi_read_blocking((_spi_id ? spi1 : spi0), 0U, input_data.data(), size);
        SPI::deselect_cs(cs_pin);
        return sc::Binary(input_data);
    }

    //! @brief SPIによる送信
    //! @param output_data 送信するデータ
    //! @param cs_pin 通信先につながるCSピン
    void SPI::write(sc::Binary output_data, CS_Pin cs_pin) const
    {
        SPI::select_cs(cs_pin);
        spi_write_blocking((_spi_id ? spi1 : spi0), output_data.get_raw().data(), output_data.size());
        SPI::deselect_cs(cs_pin);
    }

    //! @brief SPIによるメモリからの送信
    //! @param output_data 送信するデータ
    //! @param cs_pin 通通信先につながるCSピン
    //! @param memory_addr 通信先のデバイス内のメモリアドレス
    //! メモリアドレスの8ビット目は自動的に0になります
    void SPI::write_mem(sc::Binary output_data, CS_Pin cs_pin, MemoryAddr memory_addr) const
    {
        const uint8_t memory_addr_num = memory_addr.get_0();
        SPI::select_cs(cs_pin);
        spi_write_blocking((_spi_id ? spi1 : spi0), &memory_addr_num, 1);
        spi_write_blocking((_spi_id ? spi1 : spi0), output_data.get_raw().data(), output_data.size());
        SPI::deselect_cs(cs_pin);
    }


    /***** class UART *****/

    std::deque<uint8_t> UART::uart0_input_data;
    std::deque<uint8_t> UART::uart1_input_data;

    //! @brief UART通信で使うピン番号をセットアップ
    //! @param tx_gpio TXピンのGPIO番号
    //! @param rx_gpio RXピンのGPIO番号
    UART::Pin::Pin(uint8_t tx_gpio, uint8_t rx_gpio):
        _tx_gpio(tx_gpio),
        _rx_gpio(rx_gpio)
    {
        const std::set<uint8_t> PossibleTX_0 = {0, 12, 16};  // UART0のTXのピン番号が取りうる値
        const std::set<uint8_t> PossibleRX_0 = {1, 13, 17};  // UART0のRXのピン番号が取りうる値
        const std::set<uint8_t> PossibleTX_1 = {4, 8};  // UART1のTXのピン番号が取りうる値
        const std::set<uint8_t> PossibleRX_1 = {5, 9};  // UART1のRXのピン番号が取りうる値

        // Raspberry Pi PicoでUART用として使用できないピンの場合はエラー
        if (PossibleTX_0.count(_tx_gpio) && PossibleRX_0.count(_rx_gpio))
    return;
        if (PossibleTX_1.count(_tx_gpio) && PossibleRX_1.count(_rx_gpio))
    return;
        throw sc::Error(__FILE__, __LINE__, "Invalid uart gpio number entered");  // 無効なUARTのGPIO番号が入力されました
    }

    //! @brief TXピンのGPIO番号を取得
    uint8_t UART::Pin::get_tx_gpio() const
    {
        return _tx_gpio;
    }

    //! @brief RXピンのGPIO番号を取得
    uint8_t UART::Pin::get_rx_gpio() const
    {
        return _rx_gpio;
    }

    //! @brief UART0かUART1かを取得
    bool UART::Pin::get_uart_id() const
    {
        return static_cast<bool>(_tx_gpio == 4 || _tx_gpio == 8);
    }

    //! @brief UARTのセットアップ
    //! @param uart_id UART0かUART1か
    UART::UART(Pin uart_pin, uint32_t freq):
        _uart_id(uart_pin.get_uart_id()),
        _uart_pin(uart_pin),
        _freq(freq)
    {
        init_uart();
        set_uart_pin();
        set_irq();
    }

    //! @brief UART通信を初期化する
    void UART::init_uart()
    {
        uart_init((_uart_id ? uart1 : uart0), _freq);  // pico-SDKの関数  UARTを初期化する
    }

    //! @brief ピンをUARTとして有効化する
    void UART::set_uart_pin()
    {
        gpio_set_function(_uart_pin.get_tx_gpio(), GPIO_FUNC_UART);  // pico-SDKの関数  ピンの機能をUARTモードにする
        gpio_set_function(_uart_pin.get_rx_gpio(), GPIO_FUNC_UART);  // pico-SDKの関数  ピンの機能をUARTモードにする
    }

    //! 割り込み処理を設定する
    void UART::set_irq()
    {
        if (_uart_id)
        {
            uart_set_hw_flow(uart1, false, false);  // フロー制御(受信準備が終わるまで送信しないで待つ機能)を無効にする
            uart_set_format(uart1, 8, 1, UART_PARITY_NONE);  // UART通信の設定をする
            uart_set_fifo_enabled(uart1, false);  // FIFO(受信したデータを一時的に保管する機能)をオフにし，1文字ずつ受信する
            irq_set_exclusive_handler(UART1_IRQ, uart1_handler);  // 割り込み処理で実行する関数をセット
            irq_set_enabled(UART1_IRQ, true);  // 割り込み処理を有効にする
            uart_set_irq_enables(uart1, true, false);  // 割り込み処理を受信のみで使用する
        } else {
            uart_set_hw_flow(uart0, false, false);  // フロー制御(受信準備が終わるまで送信しないで待つ機能)を無効にする
            uart_set_format(uart0, 8, 1, UART_PARITY_NONE);  // UART通信の設定をする
            uart_set_fifo_enabled(uart0, false);  // FIFO(受信したデータを一時的に保管する機能)をオフにし，1文字ずつ受信する
            irq_set_exclusive_handler(UART0_IRQ, uart0_handler);  // 割り込み処理で実行する関数をセット
            irq_set_enabled(UART0_IRQ, true);  // 割り込み処理を有効にする
            uart_set_irq_enables(uart0, true, false);  // 割り込み処理を受信のみで使用する
        }
    }

    //! @brief 割り込み処理でUART0の受信をする際に呼び出される関数
    void UART::uart0_handler()
    {
        constexpr std::size_t Uart0MaxLen = 100;
        while (uart_is_readable(uart0))
        {
            uart0_input_data.push_back(uart_getc(uart0));
        }
        if (Uart0MaxLen < uart0_input_data.size())
        {
            uart0_input_data.pop_front();
        }
    }

    //! @brief 割り込み処理でUART1の受信をする際に呼び出される関数
    void UART::uart1_handler()
    {
        constexpr std::size_t Uart1MaxLen = 100;
        while (uart_is_readable(uart1))
        {
            uart1_input_data.push_back(uart_getc(uart1));
        }
        if (Uart1MaxLen < uart1_input_data.size())
        {
            uart1_input_data.pop_front();
        }
    }

    //! @brief UARTによる受信
    //! @return Binary型のバイト列．
    //! 割り込み処理で受信していたデータを全てまとめて返す．受信したデータは削除される．
    sc::Binary UART::read() const
    {
        if (_uart_id)
        {
            std::deque<uint8_t> input_data(uart0_input_data);
            uart0_input_data.clear();
            return sc::Binary(input_data);
        } else {
            std::deque<uint8_t> input_data(uart1_input_data);
            uart1_input_data.clear();
            return sc::Binary(input_data);
        }
    }

    //! @brief UARTによる受信
    //! @param size 受信するバイト数
    //! @return Binary型のバイト列
    //! 割り込み処理で受信していたデータを直近の size バイト分返す
    sc::Binary UART::read(std::size_t size) const
    {
        std::deque<uint8_t> input_data;
        if (_uart_id)
        {
            if (size < uart1_input_data.size())
            {
                for (unsigned int i = 0; i < size; ++i)
                {
                    input_data.push_back(uart1_input_data.front());
                    uart1_input_data.pop_front();
                }
    return sc::Binary(input_data);
            } else {
    return this->read();
            }
        } else {
            if (size < uart0_input_data.size())
            {
                for (unsigned int i = 0; i < size; ++i)
                {
                    input_data.push_back(uart0_input_data.front());
                    uart0_input_data.pop_front();
                }
    return sc::Binary(input_data);
            } else {
    return this->read();
            }
        }
    }

    //! @brief UARTによる送信
    //! @param output_data 送信するデータ
    //! @param no_use 不要．互換性維持のためにある
    void UART::write(sc::Binary output_data) const
    {
        uart_write_blocking((_uart_id ? uart1 : uart0), output_data.get_raw().data(), output_data.size());
    }
}