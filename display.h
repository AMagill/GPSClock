#include <cstdint>
#include <array>
#include <string_view>
#include "hardware/i2c.h"

class Display
{
public:
    void init(i2c_inst_t* i2c, uint8_t addr);
    void set_brightness(uint8_t bright);  // 0-15
    void set_char(uint8_t n, char ch, bool dot);
    void set_disp(std::string_view str);
    void set_dots(bool a, bool b, bool c, bool d);
    void write_display();

private:
    i2c_inst_t* m_i2c;
    uint8_t m_addr;
    std::array<uint16_t, 4> m_display_buf;
};