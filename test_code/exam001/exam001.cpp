#include "exam001.hpp"

/*
このファイルには"exam.hpp"で宣言された関数などの中身が書かれています
*/

namespace sc
{
    //! @brief Exam001というクラスを構築
    Exam001::Exam001(const I2C& i2c, I2C::SlaveAddr slave_addr):  // Exam001::というのはExam001というクラスの中の関数だという意味です．
        _i2c(i2c),  // コンストラクタでは，(...):の後に，メンバ変数名(値)のように書くことで，メンバ変数の初期化を行います
        _slave_addr(slave_addr)
    {
        if (!check_connection())  // センサと正常に通信できているかを確認
        {
            throw Error(__FILE__, __LINE__, "An error has occured in communication with the sensor");  // エラーを投げる．エラーはchatch句でキャッチされる．キャッチされなかったらプログラムが終了する．
        }

        set_measurement_method();  // 測定方法などを設定
        read_calibration_data();  // キャリブレーション用のデータを読み込み
    }

    //! @brief 測定を行う
    //! @return 測定結果
    Measurement Exam001::measure()
    {
        read_raw();  // データを受信
        Temperature temperature(calibrate_temperature());  // キャリブレーションをしてTemperature型の変数に保存

        return Measurement(temperature);  // 測定値を返す  (temperature, pressure)のようにすることで気温以外もまとめて返すことができます
    }

    //! @brief センサのチップIDを受信して接続を確認
    //! @return 正常だったらtrue, 異常だったらfalse
    bool Exam001::check_connection() noexcept  // エラーを返さない関数です
    {
        try  // 内部でエラーが投げられたらcahchでキャッチされる
        {
            const I2C::MemoryAddr ChipID_Addr(0x00);  // センサ内のチップIDが保存されているメモリアドレス
            const Binary chip_id = _i2c.read_mem(1, _slave_addr, ChipID_Addr);  // I2Cで1バイト受信してチップIDを取得
            constexpr uint8_t CorrectChipID = 0x60;  // 正しいチップID
            if (chip_id[0] == CorrectChipID)  // 正しいチップIDが読み取れたかを確認
            {
    return true;  // 正常なのでtrueを返す
            } else {
                Error(__FILE__, __LINE__, "read wrong chip ID");  // 正しくないIDだったらエラーを記録
    return false;  // 異常なのでfalseを返す
            }
        }
        catch(const std::exception& e)  // 内部でエラーが発生した場合に実行される
        {
            Error(__FILE__, __LINE__, "read wrong chip ID", e);  // エラーを記録
    return false;  // 異常なのでfalseを返す
        }
    }
    
    //! @brief 測定方法などを設定
    //! @param mode 測定モード
    void Exam001::set_measurement_method(Mode mode)  // 他にも，測定の間隔やノイズ処理などの設定項目があったら，ここで設定する
    {
        const I2C::MemoryAddr SettingMemoryAddr(0xf2);  // 設定を書き込むセンサ内のメモリアドレス
        _i2c.write_mem(Binary{static_cast<uint8_t>(mode << 2)}, _slave_addr, SettingMemoryAddr);  // 設定をセンサに書き込む
        // ここでしている計算(mode << 2)はセンサによって違います．これは適当に作った一例です
    }

    //! @brief 補正用データ読み取り
    void Exam001::read_calibration_data()
    {
        I2C::MemoryAddr CalibrationAddr(0x88);  // センサ内でキャリブレーション用のデータが保存されているメモリアドレス
        Binary calibration_data = _i2c.read_mem(6, _slave_addr, CalibrationAddr);  // キャリブレーションデータを受信
        dig_T1 = calibration_data[0] | (calibration_data[1] << 8);  // キャリブレーションデータを保存
        dig_T2 = calibration_data[2] | (calibration_data[3] << 8);  // 注：この時の計算方法やデータの数はセンサによって違います．この計算は適当に作った一例です
        dig_T3 = calibration_data[4] | (calibration_data[5] << 8);  // << はビットシフト演算子です
    }

    // 生データ読み取り (キャリブレーション前のデータを受信)
    void Exam001::read_raw()
    {
        const I2C::MemoryAddr TemperatureAddr(0x60);  // センサ内で気温が保存されているメモリアドレス
        const Binary input_data = _i2c.read_mem(3, _slave_addr, TemperatureAddr);  // センサの値を受信
        _raw_temperature = static_cast<uint32_t>(input_data[0] << 12) | static_cast<uint32_t>(input_data[1] << 4) | (input_data[2] >> 4);  // 受信した値を保存
        // 注：↑計算方法などはセンサによって違います．これは適当に作った一例です
    }

    //! @brief 気温データを補正
    //! @return 補正後の気温
    float Exam001::calibrate_temperature()
    {
        int32_t var1, var2;
        // 注：キャリブレーションの計算はセンサによって全く違います．↓は適当に作った一例です
        var1 = (static_cast<int32_t>(dig_T1 << 1) * (static_cast<int32_t>(dig_T2))) >> 11;  // キャリブレーションの計算を行っています．
        var2 = (((static_cast<int32_t>(dig_T1) * static_cast<int32_t>(dig_T1)) >> 12) * static_cast<int32_t>(dig_T3)) >> 14;
        return (_raw_temperature*var1 + var2) / 100.0;  // 補正済みの値を返す
    }
}
