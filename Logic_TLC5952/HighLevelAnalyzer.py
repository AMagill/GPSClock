# High Level Analyzer
# For more information and documentation, please go to https://support.saleae.com/extensions/high-level-analyzer-extensions

from saleae.analyzers import HighLevelAnalyzer, AnalyzerFrame, StringSetting, NumberSetting, ChoicesSetting


# High level analyzers must subclass the HighLevelAnalyzer class.
class Hla(HighLevelAnalyzer):

    # An optional list of types this analyzer produces, providing a way to customize the way frames are displayed in Logic 2.
    result_types = {
        'mytype': {
            'format': 'Output type: {{type}}, Input type: {{data.input_type}}'
        }
    }

    def __init__(self):
        pass

    def val_to_digit(val):
        digits = {
            0xEE: '0',
            0x82: '1',
            0xDC: '2',
            0xD6: '3',
            0xB2: '4',
            0x76: '5',
            0x7E: '6',
            0xC2: '7',
            0xFE: '8',
            0xF6: '9',
            0x00: '_' 
        }
        ch = digits.get(val & 0xFE, '?')
        if val & 1:
            ch += '.'
        return ch


    def decode(self, frame: AnalyzerFrame):
        v = int.from_bytes(frame.data['mosi'], byteorder='big')

        if (v >> 24) & 0x01:
            type = 'Bright'
            va = v & 0x7F
            vb = (v >>  7) & 0x7F
            vc = (v >> 14) & 0x7F
        else:
            type = 'On-off'
            va = Hla.val_to_digit(v & 0xFF)
            vb = Hla.val_to_digit((v >> 8)  & 0xFF)
            vc = Hla.val_to_digit((v >> 16) & 0xFF)

        # Return the data frame itself
        return AnalyzerFrame(type, frame.start_time, frame.end_time, {
            'A': va, 'B': vb, 'C': vc,
        })
