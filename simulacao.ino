#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>  // Include the DHT sensor library
 
// Configurações - variáveis editáveis
const char* default_SSID = "Wokwi-GUEST"; // Wi-Fi network name
const char* default_PASSWORD = ""; // Wi-Fi network password
const char* default_BROKER_MQTT = "20.206.203.162"; // MQTT Broker IP address
const int default_BROKER_PORT = 1883; // MQTT Broker port
const char* default_TOPICO_SUBSCRIBE = "/TEF/lamp001/cmd"; // MQTT subscribe topic
const char* default_TOPICO_PUBLISH_1 = "/TEF/lamp001/attrs"; // MQTT topic for publishing status
const char* default_TOPICO_PUBLISH_2 = "/TEF/lamp001/attrs/l"; // Topic for publishing luminosity
const char* default_TOPICO_PUBLISH_3 = "/TEF/lamp001/attrs/t"; // Topic for publishing temperature
const char* default_TOPICO_PUBLISH_4 = "/TEF/lamp001/attrs/h"; // Topic for publishing humidity
const char* default_ID_MQTT = "fiware_001"; // MQTT client ID
const int default_D4 = 2; // Onboard LED pin
 
// Configurações do sensor DHT
#define DHTPIN 14  // Pin where DHT is connected
#define DHTTYPE DHT22 // Specify DHT model (DHT22 or DHT11)
 
DHT dht(DHTPIN, DHTTYPE);  // Initialize the DHT sensor
 
// Declaração da variável para o prefixo do tópico
const char* topicPrefix = "lamp00x"; // Topic prefix for MQTT messages
 
// Variables for editable configurations
char* SSID = const_cast<char*>(default_SSID);
char* PASSWORD = const_cast<char*>(default_PASSWORD);
char* BROKER_MQTT = const_cast<char*>(default_BROKER_MQTT);
int BROKER_PORT = default_BROKER_PORT;
char* TOPICO_SUBSCRIBE = const_cast<char*>(default_TOPICO_SUBSCRIBE);
char* TOPICO_PUBLISH_1 = const_cast<char*>(default_TOPICO_PUBLISH_1);
char* TOPICO_PUBLISH_2 = const_cast<char*>(default_TOPICO_PUBLISH_2);
char* TOPICO_PUBLISH_3 = const_cast<char*>(default_TOPICO_PUBLISH_3);
char* TOPICO_PUBLISH_4 = const_cast<char*>(default_TOPICO_PUBLISH_4);
char* ID_MQTT = const_cast<char*>(default_ID_MQTT);
int D4 = default_D4;
 
WiFiClient espClient; // Create a Wi-Fi client
PubSubClient MQTT(espClient); // Create an MQTT client
char EstadoSaida = '0'; // State of the output (LED)
 
void initSerial() {
    Serial.begin(115200); // Initialize serial communication
}
 
void initWiFi() {
    delay(10);
    Serial.println("------Conexao WI-FI------"); // Debugging output
    Serial.print("Conectando-se na rede: ");
    Serial.println(SSID); // Display the SSID
    Serial.println("Aguarde");
    reconectWiFi(); // Attempt to connect to Wi-Fi
}
 
void initMQTT() {
    MQTT.setServer(BROKER_MQTT, BROKER_PORT); // Set the MQTT broker details
    MQTT.setCallback(mqtt_callback); // Set the MQTT callback function
}
 
void setup() {
    InitOutput(); // Initialize the output pin
    initSerial(); // Initialize serial communication
    initWiFi(); // Connect to Wi-Fi
    initMQTT(); // Initialize MQTT
    dht.begin();  // Initialize the DHT sensor
    delay(5000); // Wait for the sensor to stabilize
    MQTT.publish(TOPICO_PUBLISH_1, "s|on"); // Publish initial state
}
 
void loop() {
    VerificaConexoesWiFIEMQTT(); // Check connections
    EnviaEstadoOutputMQTT(); // Send output state via MQTT
    handleLuminosity(); // Handle luminosity readings
    handleTemperatureAndHumidity(); // Handle temperature and humidity readings
    MQTT.loop(); // Maintain MQTT connection
}
 
void reconectWiFi() {
    if (WiFi.status() == WL_CONNECTED) // If already connected, exit function
        return;
    WiFi.begin(SSID, PASSWORD); // Start Wi-Fi connection
    while (WiFi.status() != WL_CONNECTED) { // Wait for connection
        delay(100);
        Serial.print("."); // Print dot for each attempt
    }
    Serial.println();
    Serial.println("Conectado com sucesso na rede "); // Confirm connection
    Serial.print(SSID); // Display SSID
    Serial.println("IP obtido: "); // Display IP address
    Serial.println(WiFi.localIP()); // Print local IP
 
    digitalWrite(D4, LOW); // Ensure the LED is off on connection
}
 
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    String msg; // Create a string to hold the message
    for (int i = 0; i < length; i++) { // Construct message from payload
        char c = (char)payload[i];
        msg += c; // Append each character
    }
    Serial.print("- Mensagem recebida: ");
    Serial.println(msg); // Print the received message
 
    // Define expected topics for commands
    String onTopic = String(topicPrefix) + "@on|"; // ON command topic
    String offTopic = String(topicPrefix) + "@off|"; // OFF command topic
 
    // Compare received message with predefined topics
    if (msg.equals(onTopic)) {
        digitalWrite(D4, HIGH); // Turn LED ON
        EstadoSaida = '1'; // Update state
    }
 
    if (msg.equals(offTopic)) {
        digitalWrite(D4, LOW); // Turn LED OFF
        EstadoSaida = '0'; // Update state
    }
}
 
void VerificaConexoesWiFIEMQTT() {
    if (!MQTT.connected()) // Check if MQTT is connected
        reconnectMQTT(); // Reconnect if not
    reconectWiFi(); // Ensure Wi-Fi is also connected
}
 
void EnviaEstadoOutputMQTT() {
    if (EstadoSaida == '1') {
        MQTT.publish(TOPICO_PUBLISH_1, "s|on"); // Publish ON state
        Serial.println("- Led Ligado"); // Debug output
    }
 
    if (EstadoSaida == '0') {
        MQTT.publish(TOPICO_PUBLISH_1, "s|off"); // Publish OFF state
        Serial.println("- Led Desligado"); // Debug output
    }
    Serial.println("- Estado do LED onboard enviado ao broker!"); // Confirm state sent
    delay(1000); // Delay between updates
}
 
void InitOutput() {
    pinMode(D4, OUTPUT); // Set D4 as an output pin
    digitalWrite(D4, HIGH); // Initialize LED to OFF
    boolean toggle = false; // Toggle variable for blinking
 
    for (int i = 0; i <= 10; i++) { // Blink the LED 10 times
        toggle = !toggle; // Toggle state
        digitalWrite(D4, toggle); // Update LED state
        delay(200); // Delay between toggles
    }
}
 
void reconnectMQTT() {
    while (!MQTT.connected()) { // Attempt to connect to MQTT
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT); // Display connection attempt
        if (MQTT.connect(ID_MQTT)) { // Try to connect
            Serial.println("Conectado com sucesso ao broker MQTT!"); // Success message
            MQTT.subscribe(TOPICO_SUBSCRIBE); // Subscribe to the command topic
        } else {
            Serial.println("Falha ao reconectar no broker."); // Failure message
            Serial.println("Haverá nova tentativa de conexão em 2s"); // Retry notice
            delay(2000); // Delay before retrying
        }
    }
}
 
void handleLuminosity() {
    const int potPin = 34; // Pin for the luminosity sensor (e.g., potentiometer)
    int sensorValue = analogRead(potPin); // Read sensor value
    int luminosity = map(sensorValue, 0, 4095, 0, 100); // Map value to 0-100
    String mensagem = String(luminosity); // Create message string
    Serial.print("Valor da luminosidade: "); // Print luminosity value
    Serial.println(mensagem.c_str());
    MQTT.publish(TOPICO_PUBLISH_2, mensagem.c_str()); // Publish luminosity
}
 
// Function to handle temperature and humidity readings
void handleTemperatureAndHumidity() {
    float temperature = dht.readTemperature(); // Read temperature
    float humidity = dht.readHumidity(); // Read humidity
 
    // Check if readings are valid
    if (isnan(temperature) || isnan(humidity)) {
        Serial.println("Falha ao ler do sensor DHT!"); // Error message
        return; // Exit if error
    }
 
    String tempMessage = String(temperature); // Create temperature message
    String humMessage = String(humidity); // Create humidity message
 
    Serial.print("Temperatura: "); // Print temperature
    Serial.println(tempMessage.c_str());
    Serial.print("Umidade: "); // Print humidity
    Serial.println(humMessage.c_str());
 
    MQTT.publish(TOPICO_PUBLISH_3, tempMessage.c_str());  // Publish temperature
    MQTT.publish(TOPICO_PUBLISH_4, humMessage.c_str());  // Publish humidity
}