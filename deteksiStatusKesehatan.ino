#include <Fuzzy.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

#define ONE_WIRE_BUS 2  //D4
#define BLYNK_TEMPLATE_ID "TMPL6l2FHWkPO"
#define BLYNK_TEMPLATE_NAME "Skripsi"
#define BLYNK_AUTH_TOKEN "yvNOQ845lcZTobTKcy0GQKqvbXjjkvvN"
#define ssid "Fandi"
#define pass "fandi123"
#define pulsePin A0

int treshold = 0;

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
  Serial.begin(9600);ds.begin();
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
  hitungTreshold();
}

void loop() {
  Blynk.run();
  lcd.clear();
  vlcd.clear();
  float suhu = bacaSuhu();
  if (updateBpm) {
    if(deteksiSentuh() == true){
      vlcd.print(0, 0, "Updating BPM...");
      lcd.setCursor(0, 0); lcd.print("Updating BPM...");
      bpm = hitungBpm();
      updateBpm = false;
    } else{
      vlcd.print(0, 0, "Sentuh Sensor");
      lcd.setCursor(0, 0); lcd.print("Sentuh Sensor");
    }
  }

  Serial.print("BPM : ");
  Serial.print(bpm); 
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(bpm);
  lcd.setCursor(7, 0); lcd.print("||");
  Blynk.virtualWrite(V1, bpm);

  Serial.print("\tSuhu : ");
  Serial.print(suhu);
  lcd.setCursor(11, 0); lcd.print(suhu);
  Blynk.virtualWrite(V2, suhu);
  jalankanFuzzy(bpm, suhu);
  delay(1000);
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

  FuzzyRuleAntecedent *ifRendahAndDingin = new FuzzyRuleAntecedent();
  ifRendahAndDingin->joinWithAND(rendah, dingin);
  FuzzyRuleConsequent *thenTidakSehat1 = new FuzzyRuleConsequent();
  thenTidakSehat1->addOutput(tidakSehat);
  FuzzyRule *fuzzyRule01 = new FuzzyRule(1, ifRendahAndDingin, thenTidakSehat1);
  fuzzy->addFuzzyRule(fuzzyRule01);

  FuzzyRuleAntecedent *ifRendahAndNormal = new FuzzyRuleAntecedent();
  ifRendahAndNormal->joinWithAND(rendah, normal);
  FuzzyRuleConsequent *thenKurangSehat1 = new FuzzyRuleConsequent();
  thenKurangSehat1->addOutput(kurangSehat);
  FuzzyRule *fuzzyRule02 = new FuzzyRule(2, ifRendahAndNormal, thenKurangSehat1);
  fuzzy->addFuzzyRule(fuzzyRule02);

  FuzzyRuleAntecedent *ifRendahAndpanas = new FuzzyRuleAntecedent();
  ifRendahAndpanas->joinWithAND(rendah, panas);
  FuzzyRuleConsequent *thenTidakSehat2 = new FuzzyRuleConsequent();
  thenTidakSehat2->addOutput(tidakSehat);
  FuzzyRule *fuzzyRule03 = new FuzzyRule(3, ifRendahAndpanas, thenTidakSehat2);
  fuzzy->addFuzzyRule(fuzzyRule03); 
  
  FuzzyRuleAntecedent *ifSedangAndDingin = new FuzzyRuleAntecedent();
  ifSedangAndDingin->joinWithAND(sedang, dingin);
  FuzzyRuleConsequent *thenKurangSehat2 = new FuzzyRuleConsequent();
  thenKurangSehat2->addOutput(kurangSehat);
  FuzzyRule *fuzzyRule04 = new FuzzyRule(4, ifSedangAndDingin, thenKurangSehat2);
  fuzzy->addFuzzyRule(fuzzyRule04);

  FuzzyRuleAntecedent *ifSedangAndNormal = new FuzzyRuleAntecedent();
  ifSedangAndNormal->joinWithAND(sedang, normal);
  FuzzyRuleConsequent *thenSehat1 = new FuzzyRuleConsequent();
  thenSehat1->addOutput(sehat);
  FuzzyRule *fuzzyRule05 = new FuzzyRule(5, ifSedangAndNormal, thenSehat1);
  fuzzy->addFuzzyRule(fuzzyRule05);

  FuzzyRuleAntecedent *ifSedangAndPanas = new FuzzyRuleAntecedent();
  ifSedangAndPanas->joinWithAND(sedang, panas);
  FuzzyRuleConsequent *thenKurangSehat3 = new FuzzyRuleConsequent();
  thenKurangSehat3->addOutput(kurangSehat);
  FuzzyRule *fuzzyRule06 = new FuzzyRule(6, ifSedangAndPanas, thenKurangSehat3);
  fuzzy->addFuzzyRule(fuzzyRule06);

  FuzzyRuleAntecedent *ifTinggiAndDingin = new FuzzyRuleAntecedent();
  ifTinggiAndDingin->joinWithAND(tinggi, dingin);
  FuzzyRuleConsequent *thenTidakSehat3 = new FuzzyRuleConsequent();
  thenTidakSehat3->addOutput(tidakSehat);
  FuzzyRule *fuzzyRule07 = new FuzzyRule(7, ifTinggiAndDingin, thenTidakSehat3);
  fuzzy->addFuzzyRule(fuzzyRule07);

  FuzzyRuleAntecedent *ifTinggiAndNormal = new FuzzyRuleAntecedent();
  ifTinggiAndNormal->joinWithAND(tinggi, normal);
  FuzzyRuleConsequent *thenKurangSehat4 = new FuzzyRuleConsequent();
  thenKurangSehat4->addOutput(kurangSehat);
  FuzzyRule *fuzzyRule08 = new FuzzyRule(8, ifTinggiAndNormal, thenKurangSehat4);
  fuzzy->addFuzzyRule(fuzzyRule08);
  
  FuzzyRuleAntecedent *ifTinggiAndPanas = new FuzzyRuleAntecedent();
  ifTinggiAndPanas->joinWithAND(tinggi, panas);
  FuzzyRuleConsequent *thenTidakSehat4 = new FuzzyRuleConsequent();
  thenTidakSehat4->addOutput(tidakSehat);
  FuzzyRule *fuzzyRule09 = new FuzzyRule(9, ifTinggiAndPanas, thenTidakSehat4);
  fuzzy->addFuzzyRule(fuzzyRule09);
}

void jalankanFuzzy(float x, float y){
  fuzzy->setInput(1, x);
  fuzzy->setInput(2, y);
  fuzzy->fuzzify(); 

  float output = fuzzy->defuzzify(1);
  float derajatSehat = sehat->getPertinence();
  float derajatKurangSehat = kurangSehat->getPertinence();
  float derajatTidakSehat = tidakSehat->getPertinence();
  if(derajatTidakSehat > 0.5 ){
    printTidakSehat();
  } else if(derajatKurangSehat > 0.5){
    printKurangSehat();
  }
  else{
    printSehat();
  }
  Serial.print("\tOutput :");
  Serial.println(output);
  Blynk.virtualWrite(V4, output);
}

void hitungTreshold() {
  int totalReading = 0;
  int numReadings = 50;

  for (int i = 0; i < numReadings; i++) {
    totalReading += analogRead(pulsePin);
    delay(50);
  }
  treshold = totalReading / numReadings;
}

bool deteksiSentuh() {
  return (analogRead(pulsePin) > treshold);
}

int hitungBpm() {
  int totalPulse = 0;
  int numReadings = 100;

  for (int i = 0; i < numReadings; i++) {
    int reading = analogRead(pulsePin);
    if (reading > 0) {
      totalPulse += reading;
    } else {
      numReadings--;}
    delay(100);
  }

  return numReadings > 0 ? totalPulse / (numReadings * 10) : 0;
}

void printSehat(){
  Serial.print("\tSehat");
  lcd.setCursor(0, 1); lcd.print("Sehat");
  vlcd.print(0, 1, "Sehat");
}

void printKurangSehat(){
  Serial.print("\tKurang Sehat");
  lcd.setCursor(0, 1); lcd.print("Kurang Sehat");
  vlcd.print(0, 1, "Kurang Sehat");
}

void printTidakSehat(){
  Serial.print("\tTidak Sehat");
  lcd.setCursor(0, 1); lcd.print("Tidak Sehat");
  vlcd.print(0, 1, "Tidak Sehat");
}

