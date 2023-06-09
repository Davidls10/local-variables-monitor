#include <WiFi.h>
#include <PubSubClient.h> 
#include <Wire.h>
#include "Adafruit_SHT31.h"
#include <Servo.h>
#include <Firebase_ESP_Client.h>
//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"
// Insert Firebase project API Key
#define API_KEY "AIzaSyDWtHN_PCBUulYWw3SCFAIsai0-P5iTW1o"
//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://esp32-temp-hum-53114-default-rtdb.firebaseio.com/" 
#define TOPICO_SUBSCRIBE "CELULA_ESP32_destinatario"   
#define TOPICO_PUBLISH "CELULA_ESP32_remetente"  
#define ID_MQTT "CELULA_ESP32_cliente_MQTT"     

// Conexão à rede wifi 
const char* SSID = "MOB"; 
const char* PASSWORD = "Nova452101"; 
  
// URL e porta do broker MQTT que deseja utilizar 
const char* BROKER_MQTT = "broker.hivemq.com"; 
int BROKER_PORT = 1883;

// Pinos utilizados do ESP32
uint8_t SHT_pin = 21;
uint8_t motor_pin = 2;
uint8_t buzzer_pin = 19;
 
// Variáveis e objetos globais 
WiFiClient espClient;
PubSubClient MQTT(espClient);
WiFiServer server(80);
Adafruit_SHT31 sht = Adafruit_SHT31();
Servo motor;
float temperature = 0;
float humidity = 0;
String header; // Armazena o request HTTP
String value_string = String(5); // Valor recebido para a posição do servo motor 
int motor_pos = 0;
unsigned long current_time = millis();
bool signupOK = false;
unsigned long previous_time = 0; 
const long timeout_time = 2000;
  
// Prototypes
void init_wifi(void);
void init_mqtt(void);
void setup_firebase(void);
void reconnect_wifi(void); 
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void mqtt_publisher(void);
void conections_verification(void);
void tone(void);
void init_buzzer(void);
void init_sht(void);
void read_stats(void);
void init_motor(void);
void init_http(void);
void http_configuration(void);
 
void setup() {
  Serial.begin(115200);

  init_wifi();
  init_motor();
  init_sht();
  init_mqtt();
  init_http();
  setup_firebase();
  init_buzzer();
}
  
void init_wifi(void) {
  delay(10);
  Serial.println("------Conexao WI-FI------");
  Serial.print("Conectando-se na rede: ");
  Serial.println(SSID);
  Serial.println("Aguarde");
  reconnect_wifi();
}
  
void init_mqtt(void) {
  // informa a qual broker e porta deve ser conectado 
  MQTT.setServer(BROKER_MQTT, BROKER_PORT); 
  /* atribui função de callback (função chamada quando qualquer informação do 
  tópico subescrito chega) */
  MQTT.setCallback(mqtt_callback);         
}
  
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String msg;

  // obtém a string do payload recebido
  for(int i = 0; i < length; i++) {
    char c = (char)payload[i];
    msg += c;
  }

  Serial.print("[MQTT] Mensagem recebida: ");
  Serial.println(msg);     
}

void mqtt_publisher() {
  char message[100];
  sprintf(message, "Temperatura: %.2f / Umidade: %.2f", temperature, humidity);
  MQTT.publish(TOPICO_PUBLISH, message);
}
  
void reconnect_mqtt(void) {
  while (!MQTT.connected()) {
    Serial.print("* Tentando se conectar ao Broker MQTT: ");
    Serial.println(BROKER_MQTT);
    if (MQTT.connect(ID_MQTT)) {
      Serial.println("Conectado com sucesso ao broker MQTT!");
      MQTT.subscribe(TOPICO_SUBSCRIBE); 
    } 
    else {
      Serial.println("Falha ao reconectar no broker.");
      Serial.println("Havera nova tentativa de conexao em 2s");
      delay(2000);
    }
  }
}
  
void reconnect_wifi() {
  /* se já está conectado a rede WI-FI, nada é feito. 
      Caso contrário, são efetuadas tentativas de conexão */
  if (WiFi.status() == WL_CONNECTED) return;
        
  WiFi.begin(SSID, PASSWORD);
    
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.print("Conectado com sucesso na rede ");
  Serial.print(SSID);
  Serial.println("IP obtido: ");
  Serial.println(WiFi.localIP());
  server.begin();
}
 
void conections_verification() {
  reconnect_wifi(); 

  if (!MQTT.connected()) reconnect_mqtt(); 
} 

void tone(char pin, int frequency, int duration){
  float period = 1000.0/frequency; //Periodo em ms
  for (int i = 0; i < duration/(period); i++){ //Executa a rotina de dentro o tanta de vezes que a frequencia desejada cabe dentro da duracao
    digitalWrite(pin, HIGH);
    delayMicroseconds(period*500); //Metade do periodo em ms
    digitalWrite(pin, LOW);
    delayMicroseconds(period*500);
  }
}

void setup_firebase() {
    /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

}

void init_buzzer() {
  pinMode (buzzer_pin, OUTPUT);
  tone(buzzer_pin, 262, 500);
  delay(500);
  tone(buzzer_pin, 262, 500);
  delay(500);
  tone(buzzer_pin, 294, 500);
  delay(500);
}

void init_sht() {
  pinMode (SHT_pin, INPUT);

  if (! sht.begin(0x44)) {
    Serial.println("Não foi possível achar SHT31");
  }

  read_stats();
}

void read_stats() {
  temperature = sht.readTemperature(); // Obtém os valores da temperatura
  humidity = sht.readHumidity(); // Obtém os valores da umidade
  if (isnan(temperature)) {
    Serial.println("Falha na leitura da temperatura");
  }
  if (isnan(humidity)) {
    Serial.println("Falha na leitura da humidade");
  }
  Serial.println(temperature);
}

void init_motor() {
  motor.attach(motor_pin);
}

void init_http() {
  server.begin();
  Serial.println("Servidor HTTP iniciado");
}

void http_configuration() {
  if (Firebase.ready() && signupOK && (millis() - current_time > 15000 || current_time == 0)){
    current_time = millis();

    if (Firebase.RTDB.setFloat(&fbdo, "monitor/temperature", temperature)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    
    if (Firebase.RTDB.setFloat(&fbdo, "monitor/humidity", humidity)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.getInt(&fbdo, "monitor/motor", &motor_pos)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
}

// Programa principal 
void loop() {   
  read_stats();

  conections_verification();

  /* Envia mensagem ao broker MQTT */
  mqtt_publisher();

  /* keep-alive da comunicação com broker MQTT */    
  MQTT.loop();

  http_configuration();
  Serial.println(motor_pos);
  motor.write(motor_pos);

  delay(500);  
}