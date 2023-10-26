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
        @return なし
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
    /*****************測定値および変換******************/
    /**************************************************/

    //! @brief 測定値の通信用データID
    //! 測定値の種類を増やす際は更新してください
    enum class CommunicationDataID
    {
        other,
        temperature,
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

    //! @brief 通信用のバイト列
    class Binary
    {
        const std::vector<uint8_t> _binary_data;
    public:
        Binary(const std::initializer_list<uint8_t>& binary_data):
            _binary_data(binary_data) {}

        Binary(std::size_t size, uint8_t* binary_data):
            _binary_data(binary_data, binary_data + size) {}

        std::size_t size() const;

        const uint8_t at(std::size_t index) const;

        const uint8_t* raw_data();
    };

    /*!
    @brief 測定値に関するクラスの親クラス．
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
    @brief 測定値をまとめて保管するためのクラス
    */
    class MeasurementCollection : Noncopyable
    {
        const std::unordered_map<CommunicationDataID, Measurement*> _measurement_collection_data;
    public:
        MeasurementCollection(const std::initializer_list<Measurement>& measurement_list);

        ~MeasurementCollection();

        //! @brief 指定した型の要素を取得
        //! @param T 取得したい要素の型
        template<class T> T get()
        {
            if (!_measurement_collection_data.count(T::ID))
            {
                throw Error(__FILE__, __LINE__, "Cannot return a value of this type because it is not stored.");  // この型の値は保存されていないため，返せません
            }
            return *_measurement_collection_data.at(T::ID);
        }

        //! @brief 指定した型の要素を格納しているかを返す
        //! @param T 要素を格納しているか調べたい型
        //! @return T型の要素を格納しているか．しているならtrue
        template<class T> bool contains()
        {
            return _measurement_collection_data.count(T::ID);
        }
    private:
        Measurement* bits_to_new(CommunicationDataID id, const Binary& binary);
    };

    /*!
    @brief 気温の値の保存，操作に関するクラス
    */
    class Temperature final : public Measurement
    {
        const double _temperature;
        static constexpr double MinTemperature = -10.0;
        static constexpr double MaxTemperature = 45.0;
    public:
        static constexpr CommunicationDataID ID = CommunicationDataID::temperature;

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
    /***********************通信***********************/
    /**************************************************/

    //! @brief シリアル通信の親クラス
    class Serial
    {
        //! @brief シリアル通信で受信
        virtual Binary read(std::size_t size, uint8_t device_select) = 0;

        //! @brief シリアル通信で送信
        virtual void write(Binary write_data, uint8_t device_select) = 0;
    };

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

#endif  // SC19_CODE_TEST_SC_SC_HPP_