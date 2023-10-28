#include "sc_pico/sc_pico.hpp"

#include "exam001.hpp"

int main()
{
    stdio_init_all();  // pico-SDKを初期化
    
    pico::I2C i2c(pico::I2C::Pin(4, 5), 500*1000); // GPIO4とGPIO5のピンを使う，500kHzのI2C通信をセットアップ
    sc::Exam001 exam001(i2c, sc::I2C::SlaveAddr(0x05));  // センサExam001をセットアップ．このセンサは渡されたi2cを使って通信する．

    sc::Measurement measured_data;
    while (true)
    {
        measured_data = exam001.measure();  // Exam001で測定を行い，Measurement型の値を受け取る  Measurement型には複数の測定値を保存できる
        sc::Temperature measured_temperature = measured_data.get<sc::Temperature>();  // Measurement型の中からTemperature型の値を取り出す
        sc::Log::write("exam001 temperature: %f\n", measured_temperature.get());  // 気温を出力する  Log::writeでSDカードにも保存されるようにする予定
    }
}