#ifndef SC19_CODE_TEST_SC_SC_HPP_
#define SC19_CODE_TEST_SC_SC_HPP_

#define _USE_MATH_DEFINES  // 円周率などの定数を使用する  math.hを読み込む前に定義する必要がある (math.hはcmathやiostreamに含まれる)
#include <cfloat>
#include <cmath>
#include <deque>
#include <initializer_list>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "hardware/uart.h"
#include "hardware/pwm.h"

/*!
    @file sc.hpp
    @brief プログラム全体で共通の，基本的な機能
*/

//! @brief SCのプロジェクト全体に関わるコード
namespace sc
{
    /**************************************************/
    /********************ログ・エラー*******************/
    /**************************************************/
    
    /*!
    @brief エラーを記録し保持
    エラーを記録します．例外としてこのクラスを投げることができます．
    */
    class Error : public std::exception
    {
        const std::string _message;
    public:
        Error(const std::string& FILE, int LINE, const std::string& message) noexcept;

        Error(const std::string& FILE, int LINE, const std::string& message, const std::exception& e) noexcept;

        const char* what() const noexcept override;
    };

    //! @brief ログを記録
    class Log
    {
    public:
        /*!
        @brief ログを記録する関数です．
        外部で定義してください．
        末尾に改行を自動で追加するような実装はしないでください．
        外部に例外(エラー)が漏れないように実装してください．
        @param log 書き込む文字列
        */
        static void write(const std::string& log) noexcept;

        /*!
        @brief printfの形式でログを記録
        @param format フォーマット文字列
        @param args フォーマット文字列に埋め込む値
        */
        template<class... Args> static void write(const std::string& format, Args... args) noexcept
        {
            try
            {
                const size_t formatted_chars_num = std::snprintf(nullptr, 0, format.c_str(), args...);  // フォーマット後の文字数を計算
                char formatted_chars[formatted_chars_num + 1];  // フォーマット済みの文字列を保存するための配列を作成
                std::snprintf(&formatted_chars[0], formatted_chars_num + 1, format.c_str(), args...);  // フォーマットを実行
                Log::write(formatted_chars);  // ログを記録
            }
            catch(const std::exception& e) {Error(__FILE__, __LINE__, "Failed to save log", e);}  // ログの保存に失敗しました
        }
        // この関数は以下の資料を参考にて作成しました
        // https://pyopyopyo.hatenablog.com/entry/2019/02/08/102456
    };

    //! @brief ゼロ除算防止
    template<typename T> inline T not0(T value) {return (value ? value : 1);}
    //! @brief ゼロ除算防止
    template<> inline float not0(float value) {return (value ? value : 1e-10);}
    //! @brief ゼロ除算防止
    template<> inline double not0(double value) {return (value ? value : 1e-10);}
    //! @brief ゼロ除算防止
    template<> inline long double not0(long double value) {return (value ? value : 1e-10);}

    //! @brief コピーコンストラクタを禁止するための親クラス
    class Noncopyable
    {
    protected:
        Noncopyable() = default;
        ~Noncopyable() = default;
        Noncopyable(const Noncopyable&) = delete;
        Noncopyable& operator=(const Noncopyable&) = delete;
    };
    // Noncopyableクラスは以下の資料を参考にして作成しました
    // https://cpp.aquariuscode.com/uncopyable-mixin

    
    /**************************************************/
    /***********************通信***********************/
    /**************************************************/

    //! @brief 通信用のバイト列
    class Binary
    {
        const std::vector<uint8_t> _binary_data;
    public:
        Binary(const std::initializer_list<uint8_t>& binary_data);

        std::size_t size();

        uint8_t* raw_data();
    };

    // ピンの設定
    class Pin
    {
    public:
        // ピンのモード
        enum Mode
        {
            MODE_NO_CHANGE = -1,  // 変更しない
            IN,  // 入力用
            OUT  // 出力用
        };

        // ピンのプルアップ設定
        enum Pull
        {
            PULL_NO_CHANGE = -1,  // 変更しない
            PULL_NONE,  // 不使用
            PULL_UP,  // プルアップ
            PULL_DOWN  // プルダウン
        };

        // ピンのGPIO番号だけをセット
        constexpr Pin(uint8_t gpio):
            _gpio(gpio) {}

        // ピンの設定
        // gpio : ピンのGPIO番号
        // mode_ : 出力用か，入力用か
        // [pull_] : ピンのプルアップ・プルダウン設定 (省略時:変更しない)
        Pin(uint8_t gpio, Mode mode_, Pull pull_ = PULL_NO_CHANGE);

        // ピンを初期化し，再設定
        // [mode_] : 出力用か，入力用か
        // [pull_] : ピンのプルアップ・プルダウン設定
        void init(Mode mode_ = MODE_NO_CHANGE, Pull pull_ = PULL_NO_CHANGE);

        // ピンのGPIO番号を取得
        uint8_t gpio() const;

        // ピンの入出力モードを取得
        Mode mode() const;

        // ピンの入出力モードを設定
        // mode_ : 出力用か，入力用か
        void mode(Mode mode_) const;

        // ピンのプルアップ・プルダウン状態を取得
        Pull pull() const;

        // ピンのプルアップ・プルダウン状態を設定
        // pull_ : ピンのプルアップ・プルダウン設定
        void pull(Pull pull_) const;

        // 出力用のピンの出力レベルを1にする
        // 出力モードになっていない場合の動作は未定義です
        void high() const;

        // 出力用のピンの出力レベルを0にする
        // 出力モードになっていない場合の動作は未定義です
        void low() const;

        // 入力用ピンの入力レベルを取得
        // 入力モードになっていない場合の動作は未定義です
        bool level() const;

        // 出力用ピンの出力レベルを設定
        // level_ : 出力レベル  0か1
        // 出力モードになっていない場合の動作は未定義です
        void level(bool level_) const;
    private:
        uint8_t _gpio;
    };

    // 通信に関するクラスの親クラス
    class Communication : Noncopyable
    {
    protected:
        static constexpr uint8_t DeviceNotSelected = 0xff;  // デバイス指定用の値を指定しなかった場合のデフォルト値
    public:
        // 受信
        virtual void read(std::size_t input_data_bytes, uint8_t *input_data, uint8_t select_device = DeviceNotSelected) const = 0;

        // 送信
        virtual void write(std::size_t output_data_bytes, uint8_t *output_data, uint8_t select_device = DeviceNotSelected) const = 0;

        // メモリから読み込み
        // memory_addr : 相手のデバイスの何番地のメモリーからデータを読み込むか
        // input_data_bytes : 何バイト(文字)データを読み込むか (省略した場合は，input_dataの長さだけ読み込む)
        // input_data : 受信したデータを保存するための配列
        // select_device : I2Cではスレーブアドレス，SPIではCSピンのGPIO番号，UARTでは省略
        // SPIの場合はメモリーアドレスの8ビット目を自動で0に置き換えます
        virtual void read_mem(uint8_t memory_addr, std::size_t input_data_bytes, uint8_t* input_data, uint8_t select_device = DeviceNotSelected) const
        {
            this->write(1, &memory_addr, select_device);
            this->read(input_data_bytes, input_data, select_device);
        }
        template<typename T, std::size_t Size> void read_mem(uint8_t memory_addr, T (&input_data)[Size], uint8_t select_device = DeviceNotSelected) const {read_mem(memory_addr, Size, (uint8_t*)input_data, select_device);}

        // メモリに書き込み
        // memory_addr : 相手のデバイスの何番地のメモリーにデータを書き込むか
        // output_data_bytes : 何バイト(文字)データを書き込むか (省略した場合は，output_dataの長さだけ書き込む)
        // output_data : 送信するデータの配列
        // select_device : I2Cではスレーブアドレス，SPIではCSピンのGPIO番号，UARTでは省略
        // SPIの場合はメモリーアドレスの8ビット目を自動で1に置き換えます
        virtual void write_mem(uint8_t memory_addr, std::size_t output_data_bytes, uint8_t* output_data, uint8_t select_device = DeviceNotSelected) const
        {
            this->write(1, &memory_addr, select_device);
            this->write(output_data_bytes, output_data, select_device);
        }
        template<typename T, std::size_t Size> void write_mem(uint8_t memory_addr, T (&output_data)[Size], uint8_t select_device = DeviceNotSelected) const {write_mem(memory_addr, Size, (uint8_t*)output_data, select_device);}

        // 1バイトだけメモリに書き込み
        // memory_addr : 相手のデバイスの何番地のメモリーにデータを書き込むか
        // output_data : 送信する1バイトのデータ
        // select_device : I2Cではスレーブアドレス，SPIではCSピンのGPIO番号，UARTでは省略
        void write_byte_mem(uint8_t memory_addr, uint8_t output_data, uint8_t select_device = DeviceNotSelected) const {this->write_mem(memory_addr, 1, &output_data, select_device);}

        // 改行までデータを読み込む
        // input_data_bytes : 最大で何バイト(文字)データを読み込むか (省略した場合は，最大でinput_dataの長さだけ読み込む)
        // input_data : 受信したデータを保存するための配列
        // select_device : I2Cではスレーブアドレス，SPIではCSピンのGPIO番号，UARTでは省略
        void read_line(std::size_t input_data_bytes, uint8_t* input_data, uint8_t select_device = DeviceNotSelected) const;
        template<typename T, std::size_t Size> void read_line(T (&input_data)[Size], uint8_t select_device = DeviceNotSelected) const {read_line(Size, (uint8_t*)input_data, select_device);}
    };

    // I2C通信を行います
    class I2C : public Communication
    {
    public:
        // I2Cのセットアップ  I2C0とI2C1を使う際にそれぞれ一回だけ呼び出す
        // i2c_id : I2C0かi2c1か
        // sda_pin : I2CのSDAのピン (ピン番号のみ指定したもの)
        // scl_pin : I2CのSCLのピン (ピン番号のみ指定したもの)
        // freq : I2Cの転送速度
        I2C(bool i2c_id, Pin sda_pin, Pin scl_pin, const uint32_t& freq);

        // I2Cで受信
        // input_data_bytes : 何バイト(文字)読み込むか (省略した場合はinput_dataの長さだけ読み取る)
        // input_data : 受信したデータを保存するための配列
        // slave_addr : 通信先のデバイスのCSピンのGPIO番号
        void read(std::size_t input_data_bytes, uint8_t *input_data, uint8_t slave_addr) const;
        template<typename T, std::size_t Size> void read(T (&input_data)[Size], uint8_t slave_addr) const {read(Size, (uint8_t*)input_data, slave_addr);}

        // I2Cで送信
        // output_data_bytes : 何バイト(文字)書き込むか (省略した場合はoutput_dataの長さだけ書き込む)
        // output_data : 送信するデータの配列
        // slave_addr : 通信先のデバイスのスレーブアドレス (どのデバイスにデータを書き込むか) 通常は8~119の間を使用する  7bit
        void write(std::size_t output_data_bytes, uint8_t *output_data, uint8_t slave_addr) const;
        template<typename T, std::size_t Size> void write(T (&output_data)[Size], uint8_t slave_addr) const {write(Size, (uint8_t*)output_data, slave_addr);}

    private:
        static bool AlreadyUseI2C0;
        static bool AlreadyUseI2C1;
        bool _i2c_id;
        Pin _sda_pin;
        Pin _scl_pin;
        uint32_t _freq;
    };

    // SPI通信を行います
    class SPI : public Communication
    {
    public:
        // SPIのセットアップ  SPI0とSPI1を使う際にそれぞれ一回だけ呼び出す
        // spi_id : SPI0かSPI1か
        // miso_pin : SPIのMISO(RX)ピン
        // cs_pins : SPIのCS(SS)ピン 使用するものすべて (波かっこ{}の中にカンマで区切って書く)
        // sck_pin : SPIのSCKピン
        // mosi_pin : SPIのMOSI(TX)ピン
        // freq : SPIの転送速度
        SPI(bool spi_id, Pin miso_pin, std::initializer_list<Pin> cs_pins, Pin sck_pin, Pin mosi_pin, const uint32_t& freq);

        // SPIで送信しながら受信
        // input_data_bytes : 何バイト(文字)読み込むか (省略した場合はinput_dataの長さだけ読み取る)
        // input_data : 受信したデータを保存するための配列
        // cs_pin : 通信先のデバイスに繋がるCSピン
        // output_data : データを受信している間に送信するデータ(1バイト)  データを1バイト受信するごとに1回送信する
        void read(std::size_t input_data_bytes, uint8_t *input_data, uint8_t cs_gpio, uint8_t output_data) const;
        void read(std::size_t input_data_bytes, uint8_t *input_data, Pin cs_pin, uint8_t output_data) const {read(input_data_bytes, input_data, cs_pin.gpio(), output_data);}

        // SPIで受信
        // input_data_bytes : 何バイト(文字)読み込むか (省略した場合はinput_dataの長さだけ読み取る)
        // input_data : 受信したデータを保存するための配列
        // cs_pin : 通信先のデバイスに繋がるCSピン
        void read(std::size_t input_data_bytes, uint8_t *input_data, uint8_t cs_gpio) const {read(input_data_bytes, input_data, cs_gpio, 0U);}
        void read(std::size_t input_data_bytes, uint8_t *input_data, Pin cs_pin) const {read(input_data_bytes, input_data, cs_pin.gpio());}
        template<typename T, std::size_t Size> void read(T (&input_data)[Size], uint8_t cs_gpio) const {read(Size, (uint8_t*)input_data, cs_gpio);}
        template<typename T, std::size_t Size> void read(T (&input_data)[Size], Pin cs_pin) const {read(Size, (uint8_t*)input_data, cs_pin.gpio());}

        // SPIで送信
        // output_data_bytes : 何バイト(文字)書き込むか (省略した場合はoutput_dataの長さだけ書き込む)
        // output_data : 送信するデータの配列
        // cs_pin : 通信先のデバイスに繋がるCSピン
        void write(std::size_t output_data_bytes, uint8_t *output_data, uint8_t cs_gpio) const;
        void write(std::size_t output_data_bytes, uint8_t *output_data, Pin cs_pin) const {write(output_data_bytes, output_data, cs_pin.gpio());}
        template<typename T, std::size_t Size> void write(T (&output_data)[Size], uint8_t cs_gpio) const {write(Size, (uint8_t*)output_data, cs_gpio);}
        template<typename T, std::size_t Size> void write(T (&output_data)[Size], Pin cs_pin) const {write(Size, (uint8_t*)output_data, cs_pin.gpio());}

        // メモリから読み込み
        // memory_addr : 相手のデバイスの何番地のメモリーからデータを読み込むか
        // input_data_bytes : 何バイト(文字)データを読み込むか (省略した場合は，input_dataの長さだけ読み込む)
        // input_data : 受信したデータを保存するための配列
        // select_device : I2Cではスレーブアドレス，SPIではCSピンのGPIO番号，UARTでは省略
        // SPIの場合はメモリーアドレスの8ビット目を自動で0に置き換えます
        void read_mem(uint8_t memory_addr, std::size_t input_data_bytes, uint8_t* input_data, uint8_t select_device = DeviceNotSelected) const
        {
            memory_addr |= 0b10000000;
            this->write(1, &memory_addr, select_device);
            this->read(input_data_bytes, input_data, select_device);
        }

        // メモリに書き込み
        // memory_addr : 相手のデバイスの何番地のメモリーにデータを書き込むか
        // output_data_bytes : 何バイト(文字)データを書き込むか (省略した場合は，output_dataの長さだけ書き込む)
        // output_data : 送信するデータの配列
        // select_device : I2Cではスレーブアドレス，SPIではCSピンのGPIO番号，UARTでは省略
        // SPIの場合はメモリーアドレスの8ビット目を自動で1に置き換えます
        void write_mem(uint8_t memory_addr, std::size_t output_data_bytes, uint8_t* output_data, uint8_t select_device = DeviceNotSelected) const
        {
            memory_addr &= 0b01111111;
            this->write(1, &memory_addr, select_device);
            this->write(output_data_bytes, output_data, select_device);
        }

    private:
        static bool AlreadyUseSPI0;
        static bool AlreadyUseSPI1;
        bool _spi_id;
        Pin _miso_pin;
        std::initializer_list<Pin> _cs_pins;
        Pin _sck_pin;
        Pin _mosi_pin;
        uint32_t _freq;
    };

    // UART通信を行います
    class UART : public Communication
    {
    public:
        // UARTのセットアップ  UART0とUART1を使う際にそれぞれ一回だけ呼び出す
        // uart_id : UART0かuart1か
        // tx_gpio : UARTのTXピン
        // rx_gpio : UARTのRXピン
        // freq : UARTの転送速度
        UART(bool uart_id, Pin tx_pin, Pin rx_pin, const uint32_t& freq);

        // UARTで受信
        // input_data_bytes : 何バイト(文字)読み込むか (省略した場合はinput_dataの長さだけ読み取る)
        // input_data : 受信したデータを保存するための配列
        // 引数No_Useは，互換性維持のためにUARTでも付けていますが，UARTでは使用しません．
        void read(std::size_t input_data_bytes, uint8_t *input_data, uint8_t No_Use = DeviceNotSelected) const;
        template<typename T, std::size_t Size> void read(T (&input_data)[Size], uint8_t No_Use = DeviceNotSelected) const {read(Size, (uint8_t*)input_data, No_Use);}

        // UARTで送信
        // output_data_bytes : 何バイト(文字)書き込むか (省略した場合はoutput_dataの長さだけ書き込む)
        // output_data : 送信するデータの配列
        // 引数No_Useは，互換性維持のためにUARTでも付けていますが，UARTでは使用しません．
        void write(std::size_t output_data_bytes, uint8_t *output_data, uint8_t No_Use = DeviceNotSelected) const;
        template<typename T, std::size_t Size> void write(T (&output_data)[Size], uint8_t No_Use = DeviceNotSelected) const {write(Size, (uint8_t*)output_data, No_Use);}

        // 割り込み処理で受信時に自動でデータを読み込み，dequeに保存
        // max_data_bytes : dequeの大きさが最大で何バイトになるまで，読み取った値をdequeの末尾に追加するかを指定します．dequeの大きさがこの値を超えた場合，先頭のデータから削除されます
        // InputData : !クローバル! 読み取った値を追加するdequeを指定します．このdequeは，グローバル変数である必要があります
        // 同じオブジェクトに対し二回以上このメソッドを使った場合の動作は未定義です
        void set_irq(std::size_t max_data_bytes, std::deque<uint8_t>& InputData) const;

    private:
        static bool AlreadyUseUART0;
        static bool AlreadyUseUART1;
        bool _uart_id;
        Pin _tx_pin;
        Pin _rx_pin;
        uint32_t _freq;
    };

    // PWM(パルス幅変調)を行います
    class PWM : Noncopyable
    {
    public:
        // PWMのセットアップ
        // out_pin : PWMで出力するためのピン
        // [freq] : 設定する周波数 (実際に設定される周波数は少しずれる)
        // [duty] : ピンの出力(ピンの出力が1になっている割合)
        PWM(Pin out_pin, uint32_t freq = 0xffff, float duty = 0.0F);

        // 周波数を設定 (実際に設定される周波数は入力した値から最大で6.25%ずれる)
        // freq : 周波数 (Hz)
        void freq(uint32_t freq_);

        // 周波数を確認
        // 戻り値 : 周波数 (Hz)
        uint32_t freq() const;

        // ピンの出力レベル(ピンの出力が1になっている割合)を設定
        // duty : 出力レベル (0.0 ~ 1.0)
        void level(float duty);

        // ピンの出力レベルを確認
        // 戻り値 : 出力レベル(duty) (0.0 ~ 1.0)
        float level() const;

        // PWMのオン/オフを切り替える
        // on : オンにするときtrue
        void enable(bool on) const;
    private:
        static const uint32_t SysClock = 125E6;  // PWMの基準として使うシステムクロックの周波数 (Hz)
        enum class Channel  // チャンネル (1つのスライスに2つのピンが対応しているため，そのどちらを指すかを識別する値)
        {
            A,
            B
        };

        uint8_t _slice;  // スライス (PWMにおいてピンを識別する番号)
        Channel _channel;  // チャンネル (1つのスライスに2つのピンが対応しているため，そのどちらを指すかを識別する値)
        uint32_t _freq;  // 周波数 (Hz) 最大125M Hz
        float _duty;  // ピンの出力  ピンの出力が1になっている割合
        uint16_t _wrap;  // 分解能 (詳しくは下記の資料へ)
        float _clk_div;  // 分周比 (詳しくは下記の資料へ)
    };
    // PWMクラスは以下の資料を参考にして作成しました
    // https://lipoyang.hatenablog.com/entry/2021/12/12/201432
    // https://rikei-tawamure.com/entry/2021/02/08/213335


    /**************************************************/
    /**********************モーター*********************/
    /**************************************************/

    // 単一モーターの制御
    class Motor1 : Noncopyable
    {
    public:
        // 単一のモーターをセットアップ
        // in1_pin : モータードライバのIN1ピン
        // in2_pin : モータードライバのIN2ピン
        // [freq] : 周波数 (省略時:0xffff)
        // 左右のモーターを扱う場合はMotor2クラスを使用してください
        Motor1(Pin in1_pin, Pin in2_pin, uint32_t freq = 0xffff);

        // モーターを動かす
        // speed : モーターの出力  -1.0以上+1.0以下の値  負の値のとき逆回転
        void run(float speed);

        // ブレーキをかける
        void brake();
    private:
        PWM _in1_pwm;
        PWM _in2_pwm;
    };
    // Motor1クラスは以下の資料を参考にして作成しました
    // https://hellobreak.net/raspberry-pi-pico-dc-motor/

    // 左右のモーターの制御
    class Motor2 : Noncopyable
    {
        Motor1 _left_motor;
        Motor1 _right_motor;
    public:
        // 左右のモーターをセットアップ
        // l_in1_pin : 左のモーターのIN1ピン
        // l_in2_pin : 左のモーターのIN2ピン
        // r_in1_pin : 右のモーターのIN1ピン
        // r_in2_pin : 右のモーターのIN2ピン
        // [freq] : 周波数 (省略時:0xffff)
        // 左右のモーターを扱う場合はMotor2クラスを使用してください
        Motor2(Pin l_in1_pin, Pin l_in2_pin, Pin r_in1_pin, Pin r_in2_pin, uint32_t freq = 0xffff):
            _left_motor(l_in1_pin, l_in2_pin, freq),
            _right_motor(r_in1_pin, r_in2_pin, freq) {}

        // 左右のモーターを同時に動かす
        // left_speed : 左モーターの出力  -1.0以上+1.0以下の値  負の値のとき逆回転
        // right_speed : 右モーターの出力  -1.0以上+1.0以下の値  負の値のとき逆回転
        void run(float left_speed, float right_speed)
        {
            _left_motor.run(left_speed);
            _right_motor.run(right_speed);
        }

        // まっすぐ進む
        // speed : モーターの出力  -1.0以上+1.0以下の値  負の値のとき逆回転
        void straight(float speed) {run(speed, speed);}

        // 左に曲がる
        // speed : モーターの出力  -1.0以上+1.0以下の値  負の値のとき逆回転
        void left(float speed) {run(0.0F, speed);}

        // 右に曲がる
        // speed : モーターの出力  -1.0以上+1.0以下の値  負の値のとき逆回転
        void right(float speed) {run(speed, 0.0F);}

        // ブレーキをかける
        void brake()
        {
            _left_motor.brake();
            _right_motor.brake();
        }
    };


    /**************************************************/
    /************************記録***********************/
    /**************************************************/

    // SDカードへの書き込み，読み込み
    // 未実装
    class SD
    {
    public:
        void write(std::string message);  // 未実装
        // read();
    };

    // pico本体のFlashメモリへの書き込み，読み込み
    // 未実装
    class Flash
    {
    public:
        void write(std::string message);  // 未実装
        // read();
    };

    
    /**************************************************/
    /*****************測定値および変換******************/
    /**************************************************/

    //! @brief 測定値の通信用データID
    //! 測定値の種類を増やす際は更新してください
    enum class CommunicationDataID
    {
        temperature = 0x01,
        pressure,
        humidity,
        latitude,
        longitude,
        altitude,
        distance,
        area,
        a_position,
        r_position,
        acceleration,
        gravity_acceleration,
        line_acceleration,
        gyro,
        magnetism,
        tilt,
        a_angle,
        r_angle
    };

    /*!
    @brief 測定値に関するクラスの親クラスです．
    このクラスのインスタンスを作ることはできません
    */
    class Measurement
    {
    public:
        virtual ~Measurement() = default;
        virtual CommunicationDataID id() const noexcept = 0;
        virtual Measurement* copy_new() const = 0;
        virtual Binary to_binary() = 0;
    };

    /*!
    @brief 測定値をまとめて保存するためのクラスです
    */
    class MeasurementCollection : Noncopyable
    {
        const std::unordered_map<CommunicationDataID, Measurement*> _measurement_collection_data;
    public:
        MeasurementCollection(const std::initializer_list<Measurement>& measurement_list);

        ~MeasurementCollection();

        template<class T> T get();

        template<class T> bool match();
    private:
        Measurement* bits_to_new(CommunicationDataID id, const Binary& binary);
    };

    /*!
    @brief 気温の値の保存，操作に関するクラスです
    */
    class Temperature final : public Measurement
    {
        const double _temperature;
        static constexpr CommunicationDataID ID = CommunicationDataID::temperature;
        static constexpr double MinTemperature = -10.0;
        static constexpr double MaxTemperature = 45.0;
    public:
        explicit Temperature(double temperature):
            _temperature(temperature)
        {
            if (_temperature < MinTemperature)
            {
                throw Error(__FILE__, __LINE__, "Invalid temperature value entered.");  // 無効な温度の値が入力されました
            }
            if (MaxTemperature < _temperature)
            {
                throw Error(__FILE__, __LINE__, "Invalid temperature value entered.");  // 無効な温度の値が入力されました
            }
        }

        explicit Temperature(Binary binary);

        Binary to_binary() override;

        CommunicationDataID id() const noexcept override
        {
            return ID;
        }

        Measurement* copy_new() const override
        {
            return new Temperature(*this);
        }

        double temperature() const noexcept
        {
            return _temperature;
        }
    };


    /**************************************************/
    /**********************センサ**********************/
    /**************************************************/

    // センサに関するクラスの親クラス
    class Sensor : private Noncopyable
    {
        virtual void read_raw() = 0;
    public:
        // 以下の関数は例外を投げません
        // エラーが発生した場合はセンサの測定値がErrorValueになります

        virtual Measurements& measure() noexcept = 0;
        virtual bool check_connection() noexcept = 0;
    };











/*


    // 測定値を保存するための親クラス
    // DoubleValueクラスと違い，double型に変換できない
    class MeasuredValue
    {
    protected:
        MeasuredValue() = default;
    public:
        virtual ~MeasuredValue() = default;
    };

    struct Temperature : DoubleValue, MeasuredValue {using DoubleValue::DoubleValue;};  // 気温 (℃)
    struct Pressure : DoubleValue, MeasuredValue {using DoubleValue::DoubleValue;};  // 気圧 (hPa)
    struct Humidity : DoubleValue, MeasuredValue {using DoubleValue::DoubleValue;};  // 湿度 (%)
    struct Latitude : DoubleValue, MeasuredValue {using DoubleValue::DoubleValue;};  // 緯度 (dd.dd...°)  正:北緯, 負:南緯
    struct Longitude : DoubleValue, MeasuredValue {using DoubleValue::DoubleValue;};  // 経度 (ddd.dd...°)  正:東経, 負:西経
    struct Altitude : DoubleValue, MeasuredValue {using DoubleValue::DoubleValue;};  // 標高 (m)  海水面(ジオイド)からの高さ
    struct Distance : DoubleValue, MeasuredValue {using DoubleValue::DoubleValue;};  // 距離 (m)
    struct Area : DoubleValue, MeasuredValue {using DoubleValue::DoubleValue;};  // 面積 (カメラを想定) (px)

    struct A_Position : MeasuredValue  // 絶対(Absolute)直交座標上の位置 (m)  電源を入れた位置を原点とし，+y:北, +x:東, +z:天頂 とする右手系
    {
        struct X : DoubleValue {using DoubleValue::DoubleValue;};  // 絶対(Absolute)直交座標上のX座標 (m)  電源を入れた位置を原点とし，+y:北, +x:東, +z:天頂 とする右手系
        struct Y : DoubleValue {using DoubleValue::DoubleValue;};  // 絶対(Absolute)直交座標上のY座標 (m)  電源を入れた位置を原点とし，+y:北, +x:東, +z:天頂 とする右手系
        struct Z : DoubleValue {using DoubleValue::DoubleValue;};  // 絶対(Absolute)直交座標上のZ座標 (m)  電源を入れた位置を原点とし，+y:北, +x:東, +z:天頂 とする右手系
        X x = ErrorValue;
        Y y = ErrorValue;
        Z z = ErrorValue;
        A_Position() = default;
        constexpr A_Position(X x_, Y y_, Z z_ = 0.0) : x(x_), y(y_), z(z_) {}
        constexpr explicit A_Position(double value) : A_Position(value, value, value) {}
    };
    struct R_Position : MeasuredValue  // 相対(Relative)直交座標上の位置 (m)  今の機体の位置を原点とし，+y:前, +x:右, +z:上 とする右手系
    {
        struct X : DoubleValue {using DoubleValue::DoubleValue;};  // 相対(Relative)直交座標上のX座標 (m)  今の機体の位置を原点とし，+y:前, +x:右, +z:上 とする右手系
        struct Y : DoubleValue {using DoubleValue::DoubleValue;};  // 相対(Relative)直交座標上のY座標 (m)  今の機体の位置を原点とし，+y:前, +x:右, +z:上 とする右手系
        struct Z : DoubleValue {using DoubleValue::DoubleValue;};  // 相対(Relative)直交座標上のZ座標 (m)  今の機体の位置を原点とし，+y:前, +x:右, +z:上 とする右手系
        X x = ErrorValue;
        Y y = ErrorValue;
        Z z = ErrorValue;
        R_Position() = default;
        constexpr R_Position(X x_, Y y_, Z z_ = 0.0) : x(x_), y(y_), z(z_) {}
        constexpr explicit R_Position(double value) : R_Position(value, value, value) {}
    };

    struct VectorXYZ  // XYZの成分を持つベクトル量  今の機体の位置を原点とし，+y:前, +x:右, +z:上 とする右手系
    {
        struct X : DoubleValue {using DoubleValue::DoubleValue;};  // ベクトル量のX成分  今の機体の位置を原点とし，+y:前, +x:右, +z:上 とする右手系
        struct Y : DoubleValue {using DoubleValue::DoubleValue;};  // ベクトル量のY成分  今の機体の位置を原点とし，+y:前, +x:右, +z:上 とする右手系
        struct Z : DoubleValue {using DoubleValue::DoubleValue;};  // ベクトル量のZ成分  今の機体の位置を原点とし，+y:前, +x:右, +z:上 とする右手系
        struct Magnitude : DoubleValue {using DoubleValue::DoubleValue;};  // ベクトル量の大きさ
        X x = ErrorValue;
        Y y = ErrorValue;
        Z z = ErrorValue;
        VectorXYZ() = default;
        constexpr VectorXYZ(X x_, Y y_, Z z_) : x(x_), y(y_), z(z_) {}
        constexpr explicit VectorXYZ(double value) : VectorXYZ(value, value, value) {}
    };
    struct Acceleration : VectorXYZ, MeasuredValue {using VectorXYZ::VectorXYZ;};  // 加速度 (m/s/s) (今の機体の位置を原点とし，+y:前, +x:右, +z:上 とする右手系)
    struct GravityAcceleration : VectorXYZ, MeasuredValue {using VectorXYZ::VectorXYZ;};  // 重力加速度 (m/s/s) (今の機体の位置を原点とし，+y:前, +x:右, +z:上 とする右手系)
    struct LineAcceleration : VectorXYZ, MeasuredValue {using VectorXYZ::VectorXYZ;};  // 重力を除いた加速度 (m/s/s) (今の機体の位置を原点とし，+y:前, +x:右, +z:上 とする右手系)
    struct Gyro : VectorXYZ, MeasuredValue {using VectorXYZ::VectorXYZ;};  // 角速度 (rad/s) +x:X軸の右ネジの向き, +y:Y軸の右ネジの向き, +z:Z軸の右ネジの向き (今の機体の位置を原点とし，+y:前, +x:右, +z:上 とする右手系)
    struct Magnetism : VectorXYZ, MeasuredValue {using VectorXYZ::VectorXYZ;};  // 磁気 (mT) (今の機体の位置を原点とし，+y:前, +x:右, +z:上 とする右手系)

    struct Tilt : MeasuredValue  // 機体の傾き (°) (-180°<, <=180°)  今の機体の位置を原点とし，+y:北, +x:東, +z:天頂 とする右手系
    {
        struct Pitch : DoubleValue {using DoubleValue::DoubleValue;};  // X軸の右ネジの向きへの傾き(°) (-180°<, <=180°) (今の機体の位置を原点とし，+y:北, +x:東, +z:天頂 とする右手系)
        struct Roll : DoubleValue {using DoubleValue::DoubleValue;};  // Y軸の右ネジの向きへの傾き(°) (-180°<, <=180°) (今の機体の位置を原点とし，+y:北, +x:東, +z:天頂 とする右手系)
        struct Yaw : DoubleValue {using DoubleValue::DoubleValue;};  // Z軸の右ネジの向きへの傾き(°) (-180°<, <=180°) (今の機体の位置を原点とし，+y:北, +x:東, +z:天頂 とする右手系)
        Pitch pitch = ErrorValue;
        Roll roll = ErrorValue;
        Yaw yaw = ErrorValue;
        Tilt() = default;
        constexpr Tilt(Pitch pitch_, Roll roll_, Yaw yaw_) : pitch(pitch_), roll(roll_), yaw(yaw_) {}
        constexpr explicit Tilt(double value) : Tilt(value, value, value) {}
    };

    struct A_Angle : MeasuredValue  // 空間における向き (°)  方位に対する絶対的(Absolute)な向き
    {
        struct Direction : DoubleValue {using DoubleValue::DoubleValue;};  // 絶対的な方角 (°)  水平面における角度  0°:北, 90°:東, 180°:南, 270°:西
        struct Elevation : DoubleValue {using DoubleValue::DoubleValue;};  // 絶対的な仰角 (°)  水平面に対する角度(-180°<, <=180°)  A_Directionの方向を向いた状態から天頂方向に何度傾いた角度であるか
        Direction direction = ErrorValue;  // 絶対的な方角 (°)  重力の方向と垂直な面における角度  0°:北, 90°:東, 180°:南, 270°:西
        Elevation elevation = ErrorValue;  // 絶対的な仰角 (°)  水平面に対する角度(-180°<, <=180°)  A_Directionの方向を向いた状態から天頂方向に何度傾いた角度であるか
        A_Angle() = default;
        constexpr A_Angle(Direction direction_, Elevation elevation_) : direction(direction_), elevation(elevation_) {}
        constexpr explicit A_Angle(double value) : A_Angle(value, value) {}
    };
    struct R_Angle : MeasuredValue  // 空間における向き (°)  機体に対する相対的(Relative)な向き
    {
        struct Direction : DoubleValue {using DoubleValue::DoubleValue;};  // 相対的な方角 (°)  機体のある面における角度  0°:前, 90°:右, 180°:後, 270°:左
        struct Elevation : DoubleValue {using DoubleValue::DoubleValue;};  // 相対的な仰角 (°)  機体のある平面に対する角度(-180°<, <=180°)  R_Directionの方向を向いた状態から，機体に対しての真上方向に何度傾いた角度であるか
        Direction direction = ErrorValue;  // 相対的な方角 (°)  機体のある面における角度  0°:前, 90°:右, 180°:後, 270°:左
        Elevation elevation = ErrorValue;  // 相対的な仰角 (°)  機体のある平面に対する角度(-180°<, <=180°)  R_Directionの方向を向いた状態から，機体に対しての真上方向に何度傾いた角度であるか
        R_Angle() = default;
        constexpr R_Angle(Direction direction_, Elevation elevation_) : direction(direction_), elevation(elevation_) {}
        constexpr explicit R_Angle(double value) : R_Angle(value, value) {}
    };

    // 測定値をまとめて扱うためのクラス
    // .temperature() のようにして測定値にアクセスする
    class Measurements
    {
        // mapのキー
        enum class Index {temperature, pressure, humidity, latitude, longitude, altitude, distance, area, a_position, r_position, acceleration, gravity_acceleration, line_acceleration, gyro, magnetism, tilt, a_angle, r_angle};

        // 測定値を保存するmap
        std::map<Index, MeasuredValue*> _measurements;
    public:
        Measurements() = default;
        Measurements(Measurements&) = delete;
        Measurements& operator=(Measurements&) = delete;
        Measurements(Measurements&&) = default;  // ムーブのみ許可
        Measurements& operator=(Measurements&&) = default;  // ムーブのみ許可
        ~Measurements()
        {
            for (std::pair<const Index, MeasuredValue*> measurement : _measurements)
            {
                delete measurement.second;
            }
        }
        Temperature& temperature() {if (!_measurements.count(Index::temperature)) _measurements[Index::temperature] = new Temperature(); return *(dynamic_cast<Temperature*>(_measurements.at(Index::temperature)));}
        Pressure& pressure() {if (!_measurements.count(Index::pressure)) _measurements[Index::pressure] = new Pressure(); return *(dynamic_cast<Pressure*>(_measurements.at(Index::pressure)));}
        Humidity& humidity() {if (!_measurements.count(Index::humidity)) _measurements[Index::humidity] = new Humidity(); return *(dynamic_cast<Humidity*>(_measurements.at(Index::humidity)));}
        Latitude& latitude() {if (!_measurements.count(Index::latitude)) _measurements[Index::latitude] = new Latitude(); return *(dynamic_cast<Latitude*>(_measurements.at(Index::latitude)));}
        Longitude& longitude() {if (!_measurements.count(Index::longitude)) _measurements[Index::longitude] = new Longitude(); return *(dynamic_cast<Longitude*>(_measurements.at(Index::longitude)));}
        Altitude& altitude() {if (!_measurements.count(Index::altitude)) _measurements[Index::altitude] = new Altitude(); return *(dynamic_cast<Altitude*>(_measurements.at(Index::altitude)));}
        Distance& distance() {if (!_measurements.count(Index::distance)) _measurements[Index::distance] = new Distance(); return *(dynamic_cast<Distance*>(_measurements.at(Index::distance)));}
        Area& area() {if (!_measurements.count(Index::area)) _measurements[Index::area] = new Area(); return *(dynamic_cast<Area*>(_measurements.at(Index::area)));}
        A_Position& a_position() {if (!_measurements.count(Index::a_position)) _measurements[Index::a_position] = new A_Position(); return *(dynamic_cast<A_Position*>(_measurements.at(Index::a_position)));}
        R_Position& r_position() {if (!_measurements.count(Index::r_position)) _measurements[Index::r_position] = new R_Position(); return *(dynamic_cast<R_Position*>(_measurements.at(Index::r_position)));}
        Acceleration& acceleration() {if (!_measurements.count(Index::acceleration)) _measurements[Index::acceleration] = new Acceleration(); return *(dynamic_cast<Acceleration*>(_measurements.at(Index::acceleration)));}
        GravityAcceleration& gravity_acceleration() {if (!_measurements.count(Index::gravity_acceleration)) _measurements[Index::gravity_acceleration] = new GravityAcceleration(); return *(dynamic_cast<GravityAcceleration*>(_measurements.at(Index::gravity_acceleration)));}
        LineAcceleration& line_acceleration() {if (!_measurements.count(Index::line_acceleration)) _measurements[Index::line_acceleration] = new LineAcceleration(); return *(dynamic_cast<LineAcceleration*>(_measurements.at(Index::line_acceleration)));}
        Gyro& gyro() {if (!_measurements.count(Index::gyro)) _measurements[Index::gyro] = new Gyro(); return *(dynamic_cast<Gyro*>(_measurements.at(Index::gyro)));}
        Magnetism& magnetism() {if (!_measurements.count(Index::magnetism)) _measurements[Index::magnetism] = new Magnetism(); return *(dynamic_cast<Magnetism*>(_measurements.at(Index::magnetism)));}
        Tilt& tilt() {if (!_measurements.count(Index::tilt)) _measurements[Index::tilt] = new Tilt(); return *(dynamic_cast<Tilt*>(_measurements.at(Index::tilt)));}
        A_Angle& a_angle() {if (!_measurements.count(Index::a_angle)) _measurements[Index::a_angle] = new A_Angle(); return *(dynamic_cast<A_Angle*>(_measurements.at(Index::a_angle)));}
        R_Angle& r_angle() {if (!_measurements.count(Index::r_angle)) _measurements[Index::r_angle] = new R_Angle(); return *(dynamic_cast<R_Angle*>(_measurements.at(Index::r_angle)));}

        const Temperature& temperature() const {if (!_measurements.count(Index::temperature)) return Temperature(); else return *(dynamic_cast<Temperature*>(_measurements.at(Index::temperature)));}
        const Pressure& pressure() const {if (!_measurements.count(Index::pressure)) return Pressure(); else return *(dynamic_cast<Pressure*>(_measurements.at(Index::pressure)));}
        const Humidity& humidity() const {if (!_measurements.count(Index::humidity)) return Humidity(); else return *(dynamic_cast<Humidity*>(_measurements.at(Index::humidity)));}
        const Latitude& latitude() const {if (!_measurements.count(Index::latitude)) return Latitude(); else return *(dynamic_cast<Latitude*>(_measurements.at(Index::latitude)));}
        const Longitude& longitude() const {if (!_measurements.count(Index::longitude)) return Longitude(); else return *(dynamic_cast<Longitude*>(_measurements.at(Index::longitude)));}
        const Altitude& altitude() const {if (!_measurements.count(Index::altitude)) return Altitude(); else return *(dynamic_cast<Altitude*>(_measurements.at(Index::altitude)));}
        const Distance& distance() const {if (!_measurements.count(Index::distance)) return Distance(); else return *(dynamic_cast<Distance*>(_measurements.at(Index::distance)));}
        const Area& area() const {if (!_measurements.count(Index::area)) return Area(); else return *(dynamic_cast<Area*>(_measurements.at(Index::area)));}
        const A_Position& a_position() const {if (!_measurements.count(Index::a_position)) return A_Position(); else return *(dynamic_cast<A_Position*>(_measurements.at(Index::a_position)));}
        const R_Position& r_position() const {if (!_measurements.count(Index::r_position)) return R_Position(); else return *(dynamic_cast<R_Position*>(_measurements.at(Index::r_position)));}
        const Acceleration& acceleration() const {if (!_measurements.count(Index::acceleration)) return Acceleration(); else return *(dynamic_cast<Acceleration*>(_measurements.at(Index::acceleration)));}
        const GravityAcceleration& gravity_acceleration() const {if (!_measurements.count(Index::gravity_acceleration)) return GravityAcceleration(); else return *(dynamic_cast<GravityAcceleration*>(_measurements.at(Index::gravity_acceleration)));}
        const LineAcceleration& line_acceleration() const {if (!_measurements.count(Index::line_acceleration)) return LineAcceleration(); else return *(dynamic_cast<LineAcceleration*>(_measurements.at(Index::line_acceleration)));}
        const Gyro& gyro() const {if (!_measurements.count(Index::gyro)) return Gyro(); else return *(dynamic_cast<Gyro*>(_measurements.at(Index::gyro)));}
        const Magnetism& magnetism() const {if (!_measurements.count(Index::magnetism)) return Magnetism(); else return *(dynamic_cast<Magnetism*>(_measurements.at(Index::magnetism)));}
        const Tilt& tilt() const {if (!_measurements.count(Index::tilt)) return Tilt(); else return *(dynamic_cast<Tilt*>(_measurements.at(Index::tilt)));}
        const A_Angle& a_angle() const {if (!_measurements.count(Index::a_angle)) return A_Angle(); else return *(dynamic_cast<A_Angle*>(_measurements.at(Index::a_angle)));}
        const R_Angle& r_angle() const {if (!_measurements.count(Index::r_angle)) return R_Angle(); else return *(dynamic_cast<R_Angle*>(_measurements.at(Index::r_angle)));}
    };

    // 標高を計算するときの基準点をセット
    class Altitude0
    {
        static Pressure _Pressure0;  // 標高を計算する際の基準点の気圧
        static Temperature _Temperature0;  // 標高を計算する際の基準点の気温
        static Altitude _Altitude0;  // 標高を計算する際の基準点の標高
    public:
        // 標高を計算するときの基準点をセット
        // pressure0 : 標高の基準点の気圧
        // [temperature0] : 標高の基準点の気温 (省略時:25.0)
        // [altitude0] : 標高の基準点の標高 (省略時:0.0)
        constexpr explicit Altitude0(const Pressure& pressure0, const Temperature& temperature0 = 25.0, const Altitude& altitude0 = 0.0) noexcept;

        static Pressure pressure0() noexcept {return _Pressure0;}
        static Temperature temperature0() noexcept {return _Temperature0;}
        static Altitude altitude0() noexcept {return _Altitude0;}
    };

    // 位置を計算するときの基準点をセット
    class Position0
    {
        static Latitude _Latitude0;  // 位置を計算する際の基準点の緯度
        static Longitude _Longitude0;  // 位置を計算する際の基準点の経度
        static Altitude _Altitude0;  // 位置を計算する際の基準点の標高
    public:
        // 位置を計算するときの基準点をセット
        // latitude0 : 位置の基準点の緯度
        // longitude0 : 位置の基準点の経度
        // [altitude0] : 位置の基準点の標高 (省略時:0.0)
        Position0(const Latitude& latitude0, const Longitude& longitude0, const Altitude& altitude0 = 0.0) noexcept;

        static Latitude latitude0() noexcept {return _Latitude0;}
        static Longitude longitude0() noexcept {return _Longitude0;}
        static Altitude altitude0() noexcept {return _Altitude0;}
    };


    #ifndef M_PI  // もしM_PIが定義されていなかったら
    constexpr double M_PI = 3.14159265358979323846;  // 円周率 PI
    #endif
    
    constexpr double EarthG = 6.67259E-11;  // 万有引力定数 (m^3 s^-2 kg^-1)
    constexpr double Earth_g = 9.7803278;  // 赤道における正規重力 (m s^-2) (zero-frequency tide system)
    constexpr double EarthR = 6378136.62;  // 地球の赤道半径 (m) (zero-frequency tide system)
    constexpr double EarthF = 298.25642;  // 地球楕円体の逆扁平率 (zero-frequency tide system)
    constexpr double EarthGM = 3.986004418E14;  // 大気を含めた地心引力定数 (m^3 s^-2) (万有引力定数×地球の質量)

    // ラジアンを度に変換
    inline double to_deg(double rad) {return rad / M_PI * 180.0;}

    // 度をラジアンに変換
    inline double to_rad(double deg) {return deg / 180.0 * M_PI;}

    // エラー値が含まれているかを判断
    // values : エラー値であるかを確かめる値  {1.0, 2.0, 3.0}のように複数の値を入力
    bool is_error(std::initializer_list<DoubleValue> values);

    // エラー値が含まれているかを判断
    // value : エラー値であるを確かめる値
    inline bool is_error(DoubleValue value) {return is_error({value});}

    // 複数回 関数を実行して戻り値の平均を計算
    // func : 引数を持たない関数オブジェクト (戻り値の型はdouble型との足し算でdouble型を返す必要がある．void型だとエラー)
    // count : 繰り返す回数
    // メンバ関数などの平均を求める際はfuncの部分を次のようにしてください
    // [&オブジェクト名]{return オブジェクト名.メンバ関数名();}
    template<typename Func> double average(Func func, unsigned count = 10)
    {
        double sum = 0;
        for (unsigned i = 0; i < count; ++i) sum = func() + sum;
        return sum / static_cast<double>(count);
    }

    // XYZの3つの値から大きさを計算
    inline VectorXYZ::Magnitude to_magnitude(VectorXYZ xyz) {return sqrt(xyz.x*xyz.x + xyz.y*xyz.y + xyz.z*xyz.z);}

    // 気温と気圧から標高を計算
    // pressure : 測定した気圧
    // temperature : 測定した気温
    // [pressure0] : 基準点の気圧  (省略時:Altitude0でセットした気圧)
    // [altitude0] : 基準点の気圧  (省略時:Altitude0でセットした標高)
    Altitude to_altitude (const Pressure& pressure, const Temperature& temperature, const Pressure& pressure0 = Altitude0::pressure0(), const Altitude& altitude0 = Altitude0::altitude0()) noexcept;

    // 気圧から標高を計算
    // pressure : 測定した気圧
    // [pressure0] : 基準点の気圧  (省略時:Altitude0でセットした気圧)
    // [temperature0] : 基準点の気圧  (省略時:Altitude0でセットした気温)
    // [altitude0] : 基準点の気圧  (省略時:Altitude0でセットした標高)
    Altitude to_altitude (const Pressure& pressure, const Pressure& pressure0 = Altitude0::pressure0(), const Temperature& temperature0 = Altitude0::temperature0(), const Altitude& altitude0 = Altitude0::altitude0()) noexcept;

    // 気温から標高を計算
    // ！非推奨！精度が悪いです
    // temperature : 測定した気温
    // [temperature0] : 基準点の気圧  (省略時:Altitude0でセットした気温)
    // [altitude0] : 基準点の気圧  (省略時:Altitude0でセットした標高)
    Altitude to_altitude (const Temperature& temperature, const Temperature& temperature0 = Altitude0::temperature0(), const Altitude& altitude0 = Altitude0::altitude0()) noexcept;

    // 重力加速度から標高を計算
    // ！非推奨！精度が悪いです
    // gravity : 測定した重力加速度
    // [gravity0] : 基準点の重力加速度の大きさ (省略時:赤道における正規重力)
    // [altitude0] : 基準点の標高 (省略時: 0.0)
    Altitude to_altitude(const GravityAcceleration& gravity, const GravityAcceleration::Magnitude& gravity0 = Earth_g, const Altitude& altitude = 0.0) noexcept;

    // 重力加速度から標高を計算
    // ！非推奨！精度が悪いです
    // gravity : 測定した重力加速度
    // latitude : 測定した地点の緯度
    Altitude to_altitude(const GravityAcceleration& gravity, const Latitude& latitude) noexcept;

    // 緯度 経度 標高をXYZ直交座標へ変換
    // latitude : 緯度
    // longitude : 経度
    // [altitude] : 標高 (省略時:0.0)
    // [latitude0] : 原点の緯度 (省略時:Position0でセットした緯度)
    // [longitude0] : 原点の経度 (省略時:Position0でセットした経度)
    // [altitude0] : 原点の標高 (省略時:Position0でセットした標高)
    A_Position to_position(const Latitude& latitude, const Longitude& longitude, const Altitude& altitude = 0.0, const Latitude& latitude0 = Position0::latitude0(), const Longitude& longitude0 = Position0::longitude0(), const Altitude& altitude0 = Position0::altitude0()) noexcept;
    // この関数の作成にあたり以下の資料を参考にしました
    // https://vldb.gsi.go.jp/sokuchi/surveycalc/surveycalc/algorithm/bl2xy/bl2xy.htm

    // 緯度 経度 標高をXYZ直交座標へ変換 (高速，簡易版)
    // latitude : 緯度
    // longitude : 経度
    // [altitude] : 標高 (省略時:0.0)
    // [latitude0] : 原点の緯度
    // [longitude0] : 原点の経度
    // [altitude0] : 原点の標高
    A_Position to_position_simple(const Latitude& latitude, const Longitude& longitude, const Altitude& altitude = 0.0, const Latitude& latitude0 = Position0::latitude0(), const Longitude& longitude0 = Position0::longitude0(), const Altitude& altitude0 = Position0::altitude0()) noexcept;

    // 2点のXYZ直交座標から直線距離を計算
    // position : XYZ直交座標での位置  1つ目
    // position0 : XYZ直交座標での位置  2つ目
    Distance to_distance(const A_Position& position, const A_Position& position0) noexcept;

    // 2点の緯度と経度から距離を計算
    // latitude : 1つ目の位置の 緯度
    // longitude : 1つ目の位置の 経度
    // [latitude0] : 2つ目の位置の 緯度 (省略時:Position0でセットした緯度)
    // [longitude0] : 2つ目の位置の 経度 (省略時:Position0でセットした経度)
    Distance to_distance(const Latitude& latitude, const Longitude& longitude, const Latitude& latitude0 = Position0::latitude0(), const Longitude& longitude0 = Position0::longitude0()) noexcept;

    // 2点の緯度と経度から距離を計算 (高速・簡易版)
    // latitude : 1つ目の位置の 緯度
    // longitude : 1つ目の位置の 経度
    // [latitude0] : 2つ目の位置の 緯度
    // [longitude0] : 2つ目の位置の 経度
    Distance to_distance_simple(const Latitude& latitude, const Longitude& longitude, const Latitude& latitude0 = Position0::latitude0(), const Longitude& longitude0 = Position0::longitude0()) noexcept;
    // この関数の作成にあたり以下の資料を参考にしました
    // https://orsj.org/wp-content/corsj/or60-12/or60_12_701.pdf

    // 2点の座標から方位を計算
    A_Angle to_angle(const A_Position& position, const A_Position& position0) noexcept;

    // 2つの方位から相対的な向きを計算
    R_Angle to_r_angle(const A_Angle& angle, const A_Angle& angle0) noexcept;

    // 相対的な向きと基準の方位から方位を計算


*/


















}

#endif  // SC19_CODE_TEST_SC_SC_HPP_