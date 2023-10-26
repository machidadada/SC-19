#include "sc.hpp"

void sc::Log::write(const std::string& log) noexcept
{
    try
    {
        std::cout << log;
    }
    catch(const std::exception& e) {Error(__FILE__, __LINE__, "Failed to save log", e);}  // ログの保存に失敗しました
    catch(...) {Error(__FILE__, __LINE__, "Failed to save log");}  // ログの保存に失敗しました
}