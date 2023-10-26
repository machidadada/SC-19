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

    //! @brief バイト列のindex番目の要素を取得
    //! @param index 先頭の要素よりいくつ後の要素か
    //! @return バイト列のindex番目の要素
    const uint8_t Binary::at(std::size_t index) const
    {
        return _binary_data.at(index);
    }

    //! @brief バイト列のindex番目の要素を取得
    //! @param index 先頭の要素よりいくつ後の要素か
    //! @return バイト列のindex番目の要素
    const uint8_t Binary::operator[](std::size_t index) const
    {
        return _binary_data.at(index);
    }

    //! @brief 実際に保存されているvector型のバイト列のコピーを取得
    //! @return vector形式のコピーされたバイト列
    std::vector<uint8_t> Binary::get_raw() const
    {
        return _binary_data;
    }

    /***** class Measurement *****/

    Measurement::Measurement(Measurement&& old_measurement):
        _measurement(old_measurement._measurement)
    {
        for (std::pair<const sc::Quantity::ID, sc::Quantity*> old_element : old_measurement._measurement)
        {
            old_element.second = nullptr;
        }
    }

    Measurement& Measurement::operator=(Measurement&& old_measurement)
    {
        if (this == &old_measurement)
    return *this;

        for (std::pair<const Quantity::ID, Quantity*> measurement_element : _measurement)
        {
            delete measurement_element.second;
        }

        _measurement = old_measurement._measurement;

        for (std::pair<const sc::Quantity::ID, sc::Quantity*> old_element : old_measurement._measurement)
        {
            old_element.second = nullptr;
        }
    }

    //! @brief newで作った要素を削除
    Measurement::~Measurement()
    {
        for (std::pair<Quantity::ID, Quantity*> measurement_element : _measurement)
        {
            delete measurement_element.second;
        }
    }

    /***** class Temperature *****/

    //! @brief 気温の値をセットアップ
    Temperature::Temperature(float temperature):
        _temperature(temperature)
    {        
        static constexpr float MinTemperature = -10.0F;  // 気温の最小値
        static constexpr float MaxTemperature = 45.0F;  // 気温の最大値

        if (_temperature < MinTemperature || MaxTemperature < _temperature)
        {
            throw Error(__FILE__, __LINE__, "Invalid temperature value entered.");  // 無効な温度の値が入力されました
        }
    }

    //! @brief 気温を取得
    //! @return 気温
    float Temperature::get() const noexcept
    {
        return static_cast<float>(_temperature);
    }

    /***** class Pressure *****/

    //! @brief 気圧の値をセットアップ
    Pressure::Pressure(float pressure):
        _pressure(pressure)
    {
        static constexpr float MinPressure = 970.0F;  // 気圧の最小値
        static constexpr float MaxPressure = 1030.0F;  // 気圧の最大値

        if (_pressure < MinPressure || MaxPressure < _pressure)
        {
            throw Error(__FILE__, __LINE__, "Invalid pressure value entered.");  // 無効な気圧の値が入力されました
        }
    }

    //! @brief 気圧を取得
    //! @return 気圧
    float Pressure::get() const noexcept
    {
        return static_cast<float>(_pressure);
    }

    /***** class Humidity *****/

    //! @brief 湿度をセットアップ
    Humidity::Humidity(float humidity):
        _humidity(humidity)
    {
        static constexpr float MinHumidity = 0.0F;  // 湿度の最小値
        static constexpr float MaxHumidity = 100.0F;  // 湿度の最大値

        if (_humidity < MinHumidity || MaxHumidity < _humidity)
        {
            throw Error(__FILE__, __LINE__, "Invalid humidity value entered.");  // 無効な湿度の値が入力されました
        }
    }

    //! @brief 湿度を取得
    //! @return 湿度
    float Humidity::get() const noexcept
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
        static constexpr uint8_t MinDeviceSelectID = 0x00;  // デバイスのIDの最小値
        static constexpr uint8_t MaxDeviceSelectID = 0xff;  // デバイスのIDの最大値

        if (_device_select_id < MinDeviceSelectID || MaxDeviceSelectID < _device_select_id)
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
        static constexpr uint8_t MinMemoryAddr = 0x00;  // メモリーアドレスの最大値
        static constexpr uint8_t MaxMemoryAddr = 0xff;  // メモリーアドレスの最小値
        
        if (_memory_addr < MinMemoryAddr || MaxMemoryAddr < _memory_addr)
        {
            // throw Error(__FILE__, __LINE__, "Invalid memory_addr value entered.");  // 無効なメモリーアドレスの値が入力されました
        }
    }
    
    //! @brief 通信先のデバイスのメモリーアドレスを取得
    //! @return メモリーアドレス
    uint8_t Serial::MemoryAddr::get() const noexcept
    {
        return _memory_addr;
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
