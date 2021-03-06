#include <MultiMap.h>
#include <can.h>
#include <mcp2515.h>
#include <ADC.h>
#include <ADC_util.h>

//#define PRINT_RAW

#define CAN_ID 0x4
#define SOC_PACKET_ADDRESS (CAN_ID << 5) | 0x1
#define VOLTAGE_PACKET_ADDRESS (CAN_ID << 5) | 0x2

uint16_t socOutMap[] = {0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50,
                        55, 60, 65, 70, 75, 80, 85, 90, 95, 100};
uint16_t voltInMap[] = {9820, 10830, 11060, 11120, 11180,
                   11240, 11300, 11360, 11390, 11450,
                   11510, 11560, 11620, 11740, 11860,
                   11950, 12070, 12250, 12330, 12430, 12600};

float valDivisor = 0.058;
uint16_t voltage, soc;
struct can_frame frame;
MCP2515 mcp2515(10);
ADC *adc = new ADC();

int adcPin = A1;

void setup()
{                
  Serial.begin(115200);
  SPI.setSCK(14);
  pinMode(13, OUTPUT);
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  adc->adc0->setAveraging(32);
  adc->adc0->setResolution(10);
  adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_LOW_SPEED);
  adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::LOW_SPEED);

  delay(500);
  
  digitalWrite(13, HIGH);
}

void loop()                     
{
  uint16_t rawVal = adc->adc0->analogRead(adcPin);
  voltage = (uint16_t) (rawVal / valDivisor);
  soc = multiMap(voltage, voltInMap, socOutMap, 21);

#ifdef PRINT_RAW
  Serial.print("Raw: ");
  Serial.print(rawVal);
  Serial.println(" counts");
#endif
  Serial.print("SoC: ");
  Serial.print(soc);
  Serial.println("%");
  Serial.print("Voltage: ");
  Serial.print(voltage);
  Serial.println(" mv");

  if(adc->adc0->fail_flag != ADC_ERROR::CLEAR) {
    Serial.print("ADC0: "); Serial.println(getStringADCError(adc->adc0->fail_flag));
    adc->resetError();
  }

  frame.can_id = SOC_PACKET_ADDRESS;
  frame.can_dlc = 2;
  frame.data[0] = soc & 0xFF;
  frame.data[1] = (soc >> 8) & 0xFF;
  mcp2515.sendMessage(&frame);
  
  frame.can_id = VOLTAGE_PACKET_ADDRESS;
  frame.can_dlc = 2;
  frame.data[0] = voltage & 0xFF;
  frame.data[1] = (voltage >> 8) & 0xFF;
  mcp2515.sendMessage(&frame);
    
  delay(250);
}
