#ifndef SC19_CODE_TEST_EXAM001_EXAM001_HPP_  // 同じファイルを2回読み込まないためのものです
#define SC19_CODE_TEST_EXAM001_EXAM001_HPP_

#include "sc_pico/sc.hpp"

/*
Exam001という名前の，気温を測定するセンサのサンプルプログラムです．
関数の中身は"exam001.cpp"ファイルに書かれています
*/

namespace sc  // scという名前空間を使用します．(他のライブラリなどとの名前かぶりを防止するため)
{
    //! @brief Exam001による測定を行うクラス
    class Exam001 : public Sensor  // Exam001という名前のセンサのプログラムは，Exam001というクラスの中に書きます．Exam001型を作っています．
    {
        // ここには宣言しか書かれていません．関数の中身は"exam.cpp"ファイルに書かれています(分割コンパイルという)

        const I2C& _i2c;  // I2C通信を行うためのI2C型のオブジェクトです  定数にすることで意図しない変更を防いでいます
        const I2C::SlaveAddr _slave_addr;  // I2C通信では通信相手のアドレスを一緒に送り誰宛の通信かを識別しています．そのためのこのセンサのアドレスです．  定数にすることで意図しない変更を防いでいます
        int32_t _raw_temperature;
    public:  // publicなメンバはこのクラスの外からアクセスできます
        Exam001(const I2C& i2c, I2C::SlaveAddr slave_addr);  // このクラスの初期設定を行います(コンストラクタという)
        Measurement measure();  // 測定を行う関数
    private:
        // 測定のモード
        enum Mode  // 定数に名前を付けてわかりやすくしています．(列挙体)
        {
            MODE_SLEEP,
            MODE_FORCED,
            MODE_NORMAL = 3
        };

        //  キャリブレーション(補正)を行うためのデータ
        uint16_t dig_T1 = 1;  // 気温補正用データ
        int16_t dig_T2 = 1, dig_T3 = 3;  // 気温補正用データ
        // ↑　一度補正用データを受信して，デフォルトの値を設定しておく（うまく読み込めなかった時のため）

        bool check_connection() noexcept;  // センサが正常に接続されていることを確認

        void set_measurement_method(Mode mode = MODE_NORMAL);  // 測定方法などを設定

        void read_calibration_data();  // 補正用データ読み取り

        void read_raw();  // 生データ読み取り (キャリブレーション前のデータを受信)

        float calibrate_temperature();  // 気温データを補正
    };
}

#endif // SC19_CODE_TEST_EXAM001_EXAM001_HPP_