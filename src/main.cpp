#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

// OLED I2C (bus principal en pines 6=SCL, 5=SDA)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 6, 5);
// Segundo OLED (bus software dedicado en pines 3=SCL, 4=SDA)
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2Secondary(U8G2_R0, /* clock = */ 3, /* data = */ 4, U8X8_PIN_NONE);

// Marco en OLED (opcional)
int width = 72;
int height = 39;
int xOffset = 28;
int yOffset = 25;

// Pines y constantes del divisor
const int pinADC = 2;    // Ajusta al pin donde va el divisor
const float R1 = 3569.0; // ohm
const float R2 = 1100.0; // ohm (divisor ajustado para tope de ~1.1 V)
const float Vmultimetro = 1.0;
const float Vesp = 1.0;
const float Kcalib = Vmultimetro / Vesp;
const adc_attenuation_t pinAttenuation = ADC_0db;

// Variables de lectura
uint16_t rawADC;
float voltADC;
float voltSensor;
float pressure;

const int numSamples = 10; // Numero de muestras para promediar

void setup()
{
  Serial.begin(9600);
  delay(1500); // esperar a que el puerto USB se estabilice
  Serial.println("Iniciando...");
  analogReadResolution(12);
  analogSetPinAttenuation(pinADC, pinAttenuation);

  // Inicializar OLED
  if (!u8g2.begin())
  {
    Serial.println("❌ Error al iniciar OLED");
    while (1)
      ;
  }
  Serial.println("✅ OLED principal inicializado correctamente.");
  u8g2.setContrast(255);
  u8g2.setBusClock(400000); // 400 kHz I2C
  u8g2.setFont(u8g2_font_6x10_tf);

  if (!u8g2Secondary.begin())
  {
    Serial.println("❌ Error al iniciar OLED secundario");
    while (1)
      ;
  }
  Serial.println("✅ OLED secundario inicializado correctamente.");
  u8g2Secondary.setContrast(255);
  u8g2Secondary.setFont(u8g2_font_6x10_tf);
}

void loop()
{
  // --- Promediar varias lecturas del ADC ---
  unsigned long sumRaw = 0;
  unsigned long sumMv = 0;
  for (int i = 0; i < numSamples; i++)
  {
    sumRaw += analogRead(pinADC);
    sumMv += analogReadMilliVolts(pinADC);
    delay(5); // pequeño delay para estabilizar lectura
  }
  rawADC = sumRaw / numSamples;

  // --- Convertir a voltaje en el pin ---
  voltADC = (sumMv / numSamples) / 1000.0f;

  // --- Calcular voltaje real del sensor ---
  voltSensor = (voltADC * ((R1 + R2) / R2)) * Kcalib;

  pressure = (5.0 * voltSensor) - 22.5;

  // --- Mostrar en OLED ---
  u8g2.clearBuffer();

  // Marco opcional
  u8g2.drawFrame(xOffset, yOffset, width, height);

  // Mostrar presión en cBar centrada
  char buffer[20];
  snprintf(buffer, sizeof(buffer), "%.2f cBar", abs(pressure));
  int pressureWidth = u8g2.getStrWidth(buffer);
  int pressureX = xOffset + (width - pressureWidth) / 2;
  u8g2.setCursor(pressureX, yOffset + 15);
  u8g2.print(buffer);

  // Mostrar voltaje del sensor centrado abajo del marco
  snprintf(buffer, sizeof(buffer), "%.2f V", voltSensor);
  int vSensorWidth = u8g2.getStrWidth(buffer);
  int vSensorX = xOffset + (width - vSensorWidth) / 2;
  int vSensorY = yOffset + height - 8; // cerca del borde inferior
  u8g2.setCursor(vSensorX, vSensorY);
  u8g2.print(buffer);

  u8g2.sendBuffer();

  // --- Segundo OLED: mensaje fijo ---
  u8g2Secondary.clearBuffer();
  const char *helloMsg = "Hello World";
  int helloWidth = u8g2Secondary.getStrWidth(helloMsg);
  int helloX = (128 - helloWidth) / 2;
  int helloY = (64 / 2); // aproximadamente centrado vertical
  u8g2Secondary.setCursor(helloX, helloY);
  u8g2Secondary.print(helloMsg);
  u8g2Secondary.sendBuffer();

  // --- Mostrar en Serial ---
  Serial.print("Raw ADC Promedio: ");
  Serial.print(rawADC);
  Serial.print(" | V_ADC: ");
  Serial.print(voltADC, 3);
  Serial.print(" V | V_sensor: ");
  Serial.print(voltSensor, 3);
  Serial.print(" V | Pressure: ");
  Serial.print(pressure, 2);
  Serial.println(" cBar");

  delay(1000);
}




