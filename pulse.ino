#include <pulseSimple.h>
#include <Fuzzy.h>
#include <Wire.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <LiquidCrystal_I2C.h>

#define ONE_WIRE_BUS 2  //D4
#define BLYNK_TEMPLATE_ID "TMPL6l2FHWkPO"
#define BLYNK_TEMPLATE_NAME "Skripsi"
#define BLYNK_AUTH_TOKEN "yvNOQ845lcZTobTKcy0GQKqvbXjjkvvN"
#define ssid "IoT"
#define pass "mhmdroji123"

const int pulsePin = A0;
int treshold = 0;
const int toleransi = 60;

pulseSimple pulse(pulsePin, treshold);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds(&oneWire);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Fuzzy *fuzzy = new Fuzzy();
WidgetLCD vlcd(V0);

FuzzySet *rendah = new FuzzySet(-90, -10, 60, 80);
FuzzySet *sedang = new FuzzySet(60, 80, 100, 120);
FuzzySet *tinggi = new FuzzySet(100, 120, 210, 290);

FuzzySet *dingin = new FuzzySet(-30, -10, 15, 25);
FuzzySet *normal = new FuzzySet(15, 25, 35, 45);
FuzzySet *panas = new FuzzySet(35, 45, 55, 70);

FuzzySet *tidakSehat = new FuzzySet(-45, -5, 20, 45);
FuzzySet *kurangSehat = new FuzzySet(30, 40, 60, 70);
FuzzySet *sehat = new FuzzySet(55, 75, 105, 145);


float bpm = 0;
bool updateBpm;
int valButton = 0;

BLYNK_WRITE(V3){
  int tombolBlynk = param.asInt();
  if (tombolBlynk == 1){
    updateBpm = true;
  }
}

void setup() {
  Serial.begin(9600);
  pulse.begin(); ds.begin();
  lcd.begin(16, 2); lcd.init(); lcd.backlight();
  WiFi.begin(ssid, pass); WiFi.mode(WIFI_STA);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Menghubungkan ke WiFi...");
    lcd.setCursor(0,0);lcd.print("Menghubungkan...");
  }
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  lcd.clear(); lcd.setCursor(0, 0); lcd.print("Terhubung ke...");
  lcd.setCursor(0, 1); lcd.print(ssid); delay(2000);
  konfigFuzzy();
  lcd.clear();lcd.setCursor(0,0); lcd.print("Konfigurasi"); lcd.setCursor(0,1);lcd.print("sensor");
  pulse.hitungTreshold();
}

void loop() {
  Blynk.run();
  lcd.clear();
  vlcd.clear();
  float suhu = bacaSuhu();
  Serial.print("BPM : ");
  Serial.print(pulse.hitungBpm()); 
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("BPM : ");
  lcd.setCursor(8, 0); lcd.print(bpm);
  Blynk.virtualWrite(V1, bpm);

  Serial.print("\tSuhu : ");
  Serial.print(suhu);
  lcd.setCursor(0, 1); lcd.print("Suhu : ");
  lcd.setCursor(8, 1); lcd.print(suhu);
  Blynk.virtualWrite(V2, suhu);
  jalankanFuzzy(bpm, suhu);
  delay(100);
}

float bacaSuhu(){
  ds.requestTemperatures();
  float x = ds.getTempCByIndex(0);
  return x == -127 ? 0 : x;
}

void konfigFuzzy(){
  FuzzyInput *BPM = new FuzzyInput(1);
  BPM->addFuzzySet(rendah);
  BPM->addFuzzySet(sedang);
  BPM->addFuzzySet(tinggi);
  fuzzy->addFuzzyInput(BPM);

  FuzzyInput *suhu = new FuzzyInput(2);
  suhu->addFuzzySet(dingin);
  suhu->addFuzzySet(normal);
  suhu->addFuzzySet(panas);
  fuzzy->addFuzzyInput(suhu);

  FuzzyOutput *status = new FuzzyOutput(1);
  status->addFuzzySet(tidakSehat);
  status->addFuzzySet(kurangSehat);
  status->addFuzzySet(sehat);
  fuzzy->addFuzzyOutput(status);

  FuzzyRuleAntecedent *ifBpmRendah1 = new FuzzyRuleAntecedent();
  ifBpmRendah1->joinSingle(rendah);
  FuzzyRuleConsequent *thenSuhuDingin1 = new FuzzyRuleConsequent();
  thenSuhuDingin1->addOutput(tidakSehat);
  FuzzyRule *fuzzyRule01 = new FuzzyRule(1, ifBpmRendah1, thenSuhuDingin1);
  fuzzy->addFuzzyRule(fuzzyRule01);

  FuzzyRuleAntecedent *ifBpmRendah2 = new FuzzyRuleAntecedent();
  ifBpmRendah2->joinSingle(rendah);
  FuzzyRuleConsequent *thenSuhuNormal1 = new FuzzyRuleConsequent();
  thenSuhuNormal1->addOutput(kurangSehat);
  FuzzyRule *fuzzyRule02 = new FuzzyRule(2, ifBpmRendah2, thenSuhuNormal1);
  fuzzy->addFuzzyRule(fuzzyRule02);

  FuzzyRuleAntecedent *ifBpmRendah3 = new FuzzyRuleAntecedent();
  ifBpmRendah3->joinSingle(rendah);
  FuzzyRuleConsequent *thenSuhuPanas1 = new FuzzyRuleConsequent();
  thenSuhuPanas1->addOutput(tidakSehat);
  FuzzyRule *fuzzyRule03 = new FuzzyRule(3, ifBpmRendah3, thenSuhuPanas1);
  fuzzy->addFuzzyRule(fuzzyRule03); 
  
  FuzzyRuleAntecedent *ifBpmSedang1 = new FuzzyRuleAntecedent();
  ifBpmSedang1->joinSingle(sedang);
  FuzzyRuleConsequent *thenSuhuDingin2 = new FuzzyRuleConsequent();
  thenSuhuDingin2->addOutput(kurangSehat);
  FuzzyRule *fuzzyRule04 = new FuzzyRule(4, ifBpmSedang1, thenSuhuDingin2);
  fuzzy->addFuzzyRule(fuzzyRule04);

  FuzzyRuleAntecedent *ifBpmSedang2 = new FuzzyRuleAntecedent();
  ifBpmSedang2->joinSingle(sedang);
  FuzzyRuleConsequent *thenSuhuNormal2 = new FuzzyRuleConsequent();
  thenSuhuNormal2->addOutput(sehat);
  FuzzyRule *fuzzyRule05 = new FuzzyRule(5, ifBpmSedang2, thenSuhuNormal2);
  fuzzy->addFuzzyRule(fuzzyRule05);

  FuzzyRuleAntecedent *ifBpmSedang3 = new FuzzyRuleAntecedent();
  ifBpmSedang3->joinSingle(sedang);
  FuzzyRuleConsequent *thenSuhuPanas2 = new FuzzyRuleConsequent();
  thenSuhuPanas2->addOutput(kurangSehat);
  FuzzyRule *fuzzyRule06 = new FuzzyRule(6, ifBpmSedang3, thenSuhuPanas2);
  fuzzy->addFuzzyRule(fuzzyRule06);

  FuzzyRuleAntecedent *ifBpmTinggi1 = new FuzzyRuleAntecedent();
  ifBpmTinggi1->joinSingle(tinggi);
  FuzzyRuleConsequent *thenSuhuDingin3 = new FuzzyRuleConsequent();
  thenSuhuDingin3->addOutput(tidakSehat);
  FuzzyRule *fuzzyRule07 = new FuzzyRule(7, ifBpmTinggi1, thenSuhuDingin3);
  fuzzy->addFuzzyRule(fuzzyRule07);

  FuzzyRuleAntecedent *ifBpmTinggi2 = new FuzzyRuleAntecedent();
  ifBpmTinggi2->joinSingle(tinggi);
  FuzzyRuleConsequent *thenSuhuNormal3 = new FuzzyRuleConsequent();
  thenSuhuNormal3->addOutput(kurangSehat);
  FuzzyRule *fuzzyRule08 = new FuzzyRule(8, ifBpmTinggi2, thenSuhuNormal3);
  fuzzy->addFuzzyRule(fuzzyRule08);
  
  FuzzyRuleAntecedent *ifBpmTinggi3 = new FuzzyRuleAntecedent();
  ifBpmTinggi3->joinSingle(tinggi);
  FuzzyRuleConsequent *thenSuhuPanas3 = new FuzzyRuleConsequent();
  thenSuhuPanas3->addOutput(tidakSehat);
  FuzzyRule *fuzzyRule09 = new FuzzyRule(9, ifBpmTinggi3, thenSuhuPanas3);
  fuzzy->addFuzzyRule(fuzzyRule09);
}

void jalankanFuzzy(float x, float y){
  fuzzy->setInput(1, x);
  fuzzy->setInput(2, y);
  fuzzy->fuzzify(); 

  float output = fuzzy->defuzzify(1);
  Serial.print("\tOutput :");
  Serial.println(output);
  Blynk.virtualWrite(V4, output);
}


