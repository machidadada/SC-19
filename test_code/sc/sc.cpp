#include "sc.hpp"

// Raspberry pi pico での有線通信やセンサ値の処理を簡単にするプログラムです
namespace sc
{
    /**************************************************/
    /********************ログ・エラー*******************/
    /**************************************************/

    /***** class Error *****/

    //! @brief エラーを記録し，標準エラー出力に出力します
    //! @param FILE ＿FILE＿としてください (自動でファイル名に置き換わります)
    //! @param LINE ＿LINE＿としてください (自動で行番号に置き換わります)
    //! @param message 出力したいエラーメッセージ (自動で改行)
    Error::Error(const std::string& FILE, int LINE, const std::string& message) noexcept:
        _message(message)
    {
        try
        {
            const std::string output_message = "<<ERROR>>  FILE : " + std::string(FILE) + "  LINE : " + std::to_string(LINE) + "\n           MESSAGE : " + _message + "\n";  // 出力する形式に変形
            std::cerr << output_message << std::endl;  // cerrでエラーとして出力

            Log::write(output_message);  // エラーをログデータに記録 (外部で定義してください)
        }
        catch (const std::exception& e) {std::cerr << "<<ERROR>>  FILE : " << __FILE__ << "  LINE : " << __LINE__ << "/n           MESSAGE : Error logging failed.   " << e.what() << std::endl;}  // エラー：エラーログの記録に失敗しました
        catch(...) {std::cerr << "<<ERROR>>  FILE : " << __FILE__ << "  LINE : " << __LINE__ << "/n           MESSAGE : Error logging failed." << std::endl;}  // エラー：エラーログの記録に失敗しました
    }

    //! @brief エラーについての説明文を返します
    //! @return エラーの説明
    const char* Error::what() const noexcept
    {
        return _message.c_str();
    }

    //! @brief エラーを記録し，標準エラー出力に出力します
    //! @param FILE ＿FILE＿としてください (自動でファイル名に置き換わります)
    //! @param LINE ＿LINE＿としてください (自動で行番号に置き換わります)
    //! @param message 出力したいエラーメッセージ (自動で改行)
    //! @param e キャッチした例外
    Error::Error(const std::string& FILE, int LINE, const std::string& message, const std::exception& e) noexcept:
        Error(FILE, LINE, message + "   " + e.what()) {}




    /**************************************************/
    /*****************測定値および変換******************/
    /**************************************************/

    /***** class Binary *****/

    //! @brief バイト列のサイズを計算
    //! @return バイト列のサイズ
    std::size_t Binary::size() const
    {
        return _binary_data.size();
    }

    //! @brief バイト列の任意の位置の要素を取得
    //! @param index 先頭の要素よりいくつ後の要素か
    //! @return バイト列のindex番目の要素
    const uint8_t Binary::at(std::size_t index) const
    {
        return _binary_data.at(index);
    }

    //! @brief uint8_t型の配列に直接アクセス
    //! @return 保存されている配列の先頭へのポインタ
    const uint8_t* Binary::raw_data()
    {
        return _binary_data.data();
    }

    /***** class Measurement *****/

    //! @brief newで作った要素を削除
    Measurement::~Measurement()
    {
        for (std::pair<Quantity::ID, Quantity*> measurement_element : _measurement)
        {
            delete measurement_element.second;
        }
    }

    /***** class Temperature *****/

    //! @brief 気温型を作成
    Temperature::Temperature(float temperature):
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

    //! @brief 気温を取得
    //! @return 気温
    float Temperature::temperature() const noexcept
    {
        return static_cast<float>(_temperature);
    }

    /***** class Pressure *****/

    //! @brief 気圧型を作成
    Pressure::Pressure(float pressure):
        _pressure(pressure)
    {
        if (_pressure < MinPressure)
        {
            throw Error(__FILE__, __LINE__, "Invalid pressure value entered.");  // 無効な気圧の値が入力されました
        }
        if (MaxPressure < _pressure)
        {
            throw Error(__FILE__, __LINE__, "Invalid pressure value entered.");  // 無効な気圧の値が入力されました
        }
    }

    //! @brief 気圧を取得
    //! @return 気圧
    float Pressure::pressure() const noexcept
    {
        return static_cast<float>(_pressure);
    }

    /***** class Humidity *****/

    //! @brief 湿度型を作成
    Humidity::Humidity(float humidity):
        _humidity(humidity)
    {
        if (_humidity < MinHumidity)
        {
            throw Error(__FILE__, __LINE__, "Invalid humidity value entered.");  // 無効な湿度の値が入力されました
        }
        if (MaxHumidity < _humidity)
        {
            throw Error(__FILE__, __LINE__, "Invalid humidity value entered.");  // 無効な湿度の値が入力されました
        }
    }

    //! @brief 湿度を取得
    //! @return 湿度
    float Humidity::humidity() const noexcept
    {
        return static_cast<float>(_humidity);
    }
    
    /**************************************************/
    /***********************通信***********************/
    /**************************************************/

    /***** class DeviceSelect *****/

    //! @brief 通信先のデバイスを保存
    //! @param device_select_id 通信先のデバイスのID (I2Cのスレーブアドレス，SPIのCSピンのID)
    Serial::DeviceSelect::DeviceSelect(uint8_t device_select_id):
        _device_select_id(device_select_id)
    {
        if (_device_select_id < MinDeviceSelectID)
        {
            throw Error(__FILE__, __LINE__, "Invalid device_select_id value entered.");  // 無効なデバイス選択用IDの値が入力されました
        }
        if (MaxDeviceSelectID < _device_select_id)
        {
            throw Error(__FILE__, __LINE__, "Invalid device_select_id value entered.");  // 無効なデバイス選択用IDの値が入力されました
        }
    }

    //! @brief 通信先のデバイスを取得
    //! @return 通信先のデバイスのID (I2Cのスレーブアドレス，SPIのCSピンのID)
    uint8_t Serial::DeviceSelect::get() const noexcept
    {
        return _device_select_id;
    }

    //! @brief 通信先のデバイスのメモリーアドレスを設定
    Serial::MemoryAddr::MemoryAddr(uint8_t memory_addr):
        _memory_addr(memory_addr)
    {
        if (_memory_addr < MinMemoryAddr)
        {
            throw Error(__FILE__, __LINE__, "Invalid memory_addr value entered.");  // 無効なメモリーアドレスの値が入力されました
        }
        if (MaxMemoryAddr < _memory_addr)
        {
            throw Error(__FILE__, __LINE__, "Invalid memory_addr value entered.");  // 無効なメモリーアドレスの値が入力されました
        }
    }
    
    //! @brief 通信先のデバイスのメモリーアドレスを取得
    //! @return メモリーアドレス
    uint8_t Serial::MemoryAddr::get() const noexcept
    {
        return _memory_addr;
    }

    PWM::Level::Level(float output_level):
        _output_level(output_level)
    {
        if (_output_level < MinPwmOutputLevel)
        {
            throw Error(__FILE__, __LINE__, "Invalid pwm_output_level value entered.");  // 無効なPWM出力レベルの値が入力されました
        }
        if (MaxPwmOutputLevel < _output_level)
        {
            throw Error(__FILE__, __LINE__, "Invalid pwm_output_level value entered.");  // 無効なPWM出力レベルの値が入力されました
        }
    }


    /**************************************************/
    /**********************モーター*********************/
    /**************************************************/


    /**************************************************/
    /************************記録***********************/
    /**************************************************/


    /**************************************************/
    /**********************センサ**********************/
    /**************************************************/

    
}
