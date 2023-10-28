#ifndef SC19_CODE_TEST_SC_SC_HPP_
#define SC19_CODE_TEST_SC_SC_HPP_

/*************************************
 *************************************


このファイルは見なくてかまいません
内部の実装を知りたい場合のみ見てください


*************************************
*************************************/

#define _USE_MATH_DEFINES  // 円周率などの定数を使用する  math.hを読み込む前に定義する必要がある (math.hはcmathやiostreamに含まれる)
#include <cfloat>
#include <cmath>
#include <deque>
#include <initializer_list>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

//! @file sc.hpp
//! @brief プログラム全体で共通の，基本的な機能
//! @date 2023-10-29T15:29

//! @brief SCのプロジェクト全体に関わるコード
namespace sc
{
    /**************************************************/
    /********************ログ・エラー*******************/
    /**************************************************/
    
    //! @brief エラーを記録し保持
    //! エラーを記録します．例外としてこのクラスを投げることができます．
    class Error : public std::exception
    {
        const std::string _message;  // エラーメッセージ
    public:
        Error(const std::string& FILE, int LINE, const std::string& message) noexcept;

        Error(const std::string& FILE, int LINE, const std::string& message, const std::exception& e) noexcept;

        const char* what() const noexcept override;
    };

    //! @brief ログを記録
    class Log
    {
    public:
        static void write(const std::string& log) noexcept;

        //! @brief printfの形式でログを記録
        //! @param format フォーマット文字列
        //! @param args フォーマット文字列に埋め込む値
        template<typename... Args>
        static void write(const std::string& format, Args... args) noexcept
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

    //! @brief コピーを禁止するための親クラス
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
    /*****************測定値および変換******************/
    /**************************************************/

    //! @brief 通信用のバイト列
    class Binary
    {
        const std::vector<uint8_t> _binary_data;  // バイト列のデータ
    public:
        //! @brief バイト列を作成
        //! @param binary_data { }で囲んだデータ
        explicit Binary(const std::initializer_list<uint8_t>& binary_data):
            _binary_data(binary_data) {}

        //! @brief バイト列を作成
        //! @param size 配列のサイズ
        //! @param binary_data 配列
        Binary(std::size_t size, uint8_t* binary_data):
            _binary_data(binary_data, binary_data + size) {}

        //! @brief バイト列を作成
        //! @param binary_data 配列
        template<std::size_t Size>
        explicit Binary(const uint8_t (&binary_data)[Size]):
            Binary(Size, binary_data) {}

        //! @brief バイト列を作成
        //! @param binary_data vectorの配列
        explicit Binary(std::vector<uint8_t> binary_data):
            _binary_data(binary_data) {}

        //! @brief バイト列を作成
        //! @param binary_data deque型の値
        explicit Binary(std::deque<uint8_t> binary_data):
            _binary_data(binary_data.begin(), binary_data.end()) {}

        std::size_t size() const;

        const uint8_t at(std::size_t index) const;

        const uint8_t operator[](std::size_t index) const;

        std::vector<uint8_t> get_raw() const;
    };

    //! @brief 測定値に関するクラスの親クラス．
    class Quantity
    {
    public:
        virtual ~Quantity() = default;

        //! @brief データを通信用のバイト列に変換
        // 未実装
        // virtual Binary to_binary() const = 0;

        //! @brief 通信を行う際にデータの種類を識別するためのID
        enum class ID
        {
            message,
            temperature,
            pressure,
            humidity
        };
    };

    //! @brief 測定値をまとめて扱う
    class Measurement : Noncopyable
    {
        std::unordered_map<Quantity::ID, Quantity*> _measurement;  // 測定値をまとめたデータ

        //! @brief 再起関数を使い，最初の要素からmapに入れて初期化
        template<class FirstQuantity, class... RestQuantitys>
        void init_first(const FirstQuantity& first_quantity, const RestQuantitys... rest_quantitys)
        {
            if (_measurement.count(FirstQuantity::id()))
            {
                delete _measurement.at(FirstQuantity::id());
            }
            _measurement[FirstQuantity::id()] = new FirstQuantity(first_quantity);

            init_first(rest_quantitys...);
        }

        void init_first() {}  // 再起関数で初期化する際に，最後に呼び出される関数
    public:
        //! @brief 測定値を入力し初期化
        //! @param quantity_derives 複数個の保存したい測定値
        template<class... QuantityDeriveds>
        explicit Measurement(const QuantityDeriveds&... quantity_deriveds)
        {
            static_assert(std::conjunction<std::is_base_of<Quantity, QuantityDeriveds>...>::value, "\n\n<!ERROR!> The Measurement class can only handle values of child classes of type Quantity\n\n");  // MeasurementクラスではQuantity型の子クラスの値しか扱えません
            
            init_first(quantity_deriveds...);
        }

        Measurement(Measurement&& old_measurement);

        Measurement& operator=(Measurement&& old_measurement);

        ~Measurement();

        //! @brief 保存してある測定値を取得
        template<class QuantityDerived>
        QuantityDerived get() const
        {
            static_assert(std::is_base_of<Quantity, QuantityDerived>::value, "\n\n<!ERROR!> The Measurement class can only handle values of child classes of type Quantity\n\n");  // MeasurementクラスではQuantity型の子クラスの値しか扱えません

            return *dynamic_cast<QuantityDerived*>(_measurement.at(QuantityDerived::id()));
        }

        // 未実装
        // Binary to_binary() const;
    };

    //! @brief 気温の値の保存，操作．
    //! 単位：℃
    class Temperature final : public Quantity
    {
        const float _temperature;  // 気温データ
    public:
        static constexpr ID id() {return ID::temperature;}
        explicit Temperature(float temperature);
        float get() const noexcept;
    };

    //! @brief 気圧の値の保存，操作．
    //! 単位：hPa
    class Pressure final : public Quantity
    {
        const float _pressure;  // 気圧データ
    public:
        static constexpr ID id() {return ID::pressure;}
        explicit Pressure(float pressure);
        float get() const noexcept;
    };

    //! @brief 湿度の値の保存，操作．
    //! 単位：%
    class Humidity final : public Quantity
    {
        const float _humidity;  // 湿度データ
    public:
        static constexpr ID id() {return ID::humidity;}
        explicit Humidity(float humidity);
        float get() const noexcept;
    };
    
    /**************************************************/
    /***********************通信***********************/
    /**************************************************/

    //! @brief ピンによる入出力の親クラス
    class PinIO : Noncopyable
    {
    public:
        //! @brief 入力か出力か
        enum class Direction
        {
            in,  // 入力用
            out  // 出力用
        };

        //! @brief ピンのプルアップ・プルダウン設定
        enum class Pull
        {
            no_use,  // 使用しない
            up,  // プルアップ
            down  // プルダウン
        };

        //! @brief 入力用ピンから読み込み
        //! @return High(1)かLow(0)か
        virtual bool read() const = 0;

        //! @brief 出力用ピンに書き込み
        //! @param level High(1)かLow(0)か
        virtual void write(bool level) const = 0;
    };

    //! @brief I2C通信の親クラス
    class I2C : Noncopyable
    {
    public:
        //! @brief I2Cのスレーブアドレスを管理
        class SlaveAddr
        {
            const uint8_t _slave_addr;
        public:
            explicit SlaveAddr(uint8_t slave_addr);
            uint8_t get() const noexcept;
        };
    
        //! @brief スレーブ内のメモリーアドレス
        class MemoryAddr
        {
            const uint8_t _memory_addr;
        public:
            explicit MemoryAddr(uint8_t memory_addr);
            uint8_t get() const noexcept;
        };

        //! @brief I2Cによる受信
        //! @param size 受信するバイト数
        //! @param slave_addr 通信先のデバイスのスレーブアドレス
        //! @return Binary型のバイト列
        virtual Binary read(std::size_t size, SlaveAddr slave_addr) const = 0;

        //! @brief I2Cによるメモリからの受信
        //! @param size 受信するバイト数
        //! @param slave_addr 通信先のデバイスのスレーブアドレス
        //! @param memory_addr 通信先のデバイス内のメモリアドレス
        //! @return Binary型のバイト列
        virtual Binary read_mem(std::size_t size, SlaveAddr slave_addr, MemoryAddr memory_addr) const = 0;

        //! @brief I2Cによる送信
        //! @param output_data 送信するデータ
        //! @param slave_addr 通信先のデバイスのスレーブアドレス
        virtual void write(Binary output_data, SlaveAddr slave_addr) const = 0;

        //! @brief I2Cによるメモリからの送信
        //! @param output_data 送信するデータ
        //! @param slave_addr 通信先のデバイスのスレーブアドレス
        //! @param memory_addr 通信先のデバイス内のメモリアドレス
        virtual void write_mem(Binary output_data, SlaveAddr slave_addr, MemoryAddr memory_addr) const = 0;
    };

    //! @brief SPI通信の親クラス
    class SPI : Noncopyable
    {
    public:
        //! @brief SPIのCSピンを管理
        class CS_Pin
        {
            const uint8_t _cs_gpio;
        public:
            explicit CS_Pin(uint8_t cs_gpio);
            uint8_t get() const noexcept;
        };
    
        //! @brief スレーブ内のメモリーアドレス
        class MemoryAddr
        {
            const uint32_t _memory_addr;
        public:
            explicit MemoryAddr(uint8_t memory_addr);
            uint8_t get() const noexcept;
            uint8_t get_0() const noexcept;
            uint8_t get_1() const noexcept;
        };

        //! @brief SPIによる受信
        //! @param size 受信するバイト数
        //! @param cs_pin 通信先につながるCSピン
        //! @return Binary型のバイト列
        virtual Binary read(std::size_t size, CS_Pin cs_pin) const = 0;

        //! @brief SPIによるメモリからの受信
        //! @param size 受信するバイト数
        //! @param cs_pin 通信先につながるCSピン
        //! @param memory_addr 通信先のデバイス内のメモリアドレス
        //! @return Binary型のバイト列
        //! メモリアドレスの8ビット目は1として扱われます．
        virtual Binary read_mem(std::size_t size, CS_Pin cs_pin, MemoryAddr memory_addr) const = 0;

        //! @brief SPIによる送信
        //! @param output_data 送信するデータ
        //! @param cs_pin 通信先につながるCSピン
        virtual void write(Binary output_data, CS_Pin cs_pin) const = 0;

        //! @brief SPIによるメモリからの送信
        //! @param output_data 送信するデータ
        //! @param cs_pin 通通信先につながるCSピン
        //! @param memory_addr 通信先のデバイス内のメモリアドレス
        //! メモリアドレスの8ビット目は0として扱われます
        virtual void write_mem(Binary output_data, CS_Pin cs_pin, MemoryAddr memory_addr) const = 0;
    };


    //! @brief UART通信の親クラス
    class UART : Noncopyable
    {
    public:
        //! @brief UARTによる受信
        //! @return Binary型のバイト列
        //! 割り込み処理で受信していたデータを全てまとめて返す
        virtual Binary read() const = 0;

        //! @brief UARTによる受信
        //! @param size 受信するバイト数
        //! @return Binary型のバイト列
        //! 割り込み処理で受信していたデータを直近の size バイト分返す
        virtual Binary read(std::size_t size) const = 0;

        //! @brief UARTによる送信
        //! @param output_data 送信するデータ
        virtual void write(Binary output_data) const = 0;
    };

    //! @brief PWMに関する親クラス
    class PWM : Noncopyable
    {
    public:
        //! @brief 出力レベルを設定
        //! @param level 出力レベル  0.0以上1.0以下の小数
        virtual void set_level(float output_level) = 0;

        //! @brief 周波数を設定
        //! @param freq 周波数 (/s)
        virtual void set_freq(uint32_t freq) = 0;
    };

    /**************************************************/
    /**********************モーター*********************/
    /**************************************************/

    //! @brief モーターを動かす際のスピード
    class MotorSpeed
    {
        const float _speed;  // スピードのデータ
    public:
        //! @brief モータースピードをセットアップ
        //! @param +1.0~-1.0の値のスピード
        explicit MotorSpeed(float speed);  // 未実装  値のチェックも実装してください
        float speed();  // 未実装
    };

    //! @brief 単一のモーター操作
    class Motor1
    {
        const PWM* _pwm;  // モーターの操作に使用するPWM
    public:
        //! @brief モーターを動かす
        //! @param speed +1.0~-1.0，負の値とき後ろに進む
        void move(MotorSpeed speed);  // 未実装
    };

    //! @brief 左右のモーターの操作
    class Motor2
    {
        const Motor1 _left_motor;  //  左モーターの制御用
        const Motor1 _right_motor;  //  右モーターの制御用
    public:
        //! @brief 左右のモーターを動かす
        //! @param left_speed 1.0でMaxのスピード，負の値とき後ろに進む
        //! @param right_speed 左と同様
        void move(MotorSpeed left_speed, MotorSpeed right_speed);  // 未実装

        //! @brief 右に曲がる
        //! @param speed 曲がるときのスピード
        void right(MotorSpeed speed);  // 未実装

        //! @brief 左に曲がる
        //! @param speed 曲がるときのスピード
        void left(MotorSpeed speed);  // 未実装

        //! @brief 直進する
        //! @param speed 進むときのスピード，負のとき後退
        void straight(MotorSpeed speed);  // 未実装
    };

    /**************************************************/
    /************************記録***********************/
    /**************************************************/

    // SDカード関連の親クラス
    class SD : Noncopyable
    {
        virtual void write(Binary output_data) = 0;
    };

    /**************************************************/
    /**********************センサ**********************/
    /**************************************************/

    //! @brief センサの親クラス
    class Sensor : Noncopyable
    {
        virtual Measurement measure() = 0;
    };
}

#endif  // SC19_CODE_TEST_SC_SC_HPP_