#include "sc.hpp"

// Raspberry pi pico での有線通信やセンサ値の処理を簡単にするプログラムです
namespace sc
{
    /**************************************************/
    /********************ログ・エラー*******************/
    /**************************************************/

    /***** class Error *****/

    /*!
    @brief エラーを記録し，標準エラー出力に出力します
    @param FILE ＿FILE＿としてください (自動でファイル名に置き換わります)
    @param LINE ＿LINE＿としてください (自動で行番号に置き換わります)
    @param message 出力したいエラーメッセージ (自動で改行)
    */
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

    /*!
    @brief エラーについての説明文を返します
    @return エラーの説明
    */
    const char* Error::what() const noexcept
    {
        return _message.c_str();
    }

    /*!
    @brief エラーを記録し，標準エラー出力に出力します
    @param FILE ＿FILE＿としてください (自動でファイル名に置き換わります)
    @param LINE ＿LINE＿としてください (自動で行番号に置き換わります)
    @param message 出力したいエラーメッセージ (自動で改行)
    @param e キャッチした例外
    */
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

    /***** class MeasurementCollection *****/

    /*!
    @brief 測定値を保存
    @param measurement_list { }で囲んで測定値を入力
    */
    MeasurementCollection::MeasurementCollection(const std::initializer_list<Measurement>& measurement_list):
        _measurement_collection_data(
            [&measurement_list]
            {
                std::unordered_map<CommunicationDataID, Measurement*> initialization_collection_data;
                for (const Measurement& measurement : measurement_list)
                {
                    if (initialization_collection_data.count(measurement.id()))
                    {
                        delete initialization_collection_data.at(measurement.id());
                    }
                    initialization_collection_data[measurement.id()] = measurement.copy_new();
                }
                return initialization_collection_data;
            }()
        ) {}

    //! @brief newで作成したオブジェクトを削除
    MeasurementCollection::~MeasurementCollection()
    {
        for (std::pair<CommunicationDataID, Measurement*> collection_data_element : _measurement_collection_data)
        {
            delete collection_data_element.second;
        }
    }

    //! @brief 指定したIDを持つ測定値型のオブジェクトを，バイト列から新たにnew演算子で作成
    //! @param id 測定値型を識別するためのID
    //! @param binary 測定値型を作る際に基にするバイト配列
    //! @return 新たに作成したオブジェクトへのポインタ
    Measurement* MeasurementCollection::bits_to_new(CommunicationDataID id, const Binary& binary)
    {
        switch (id)
        {
            case CommunicationDataID::temperature: return new Temperature(binary);
        }
    }
    
    /**************************************************/
    /***********************通信***********************/
    /**************************************************/


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
