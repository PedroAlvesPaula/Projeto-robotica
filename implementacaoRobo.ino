#include <Ultrasonic.h>

// Pinos dos motores
const int PIN_IN1 = 9;   // PWM - Motor esquerdo
const int PIN_IN2 = 10;  // PWM - Motor esquerdo

const int PIN_IN3 = 5;   // PWM - Motor direito
const int PIN_IN4 = 6;   // PWM - Motor direito

// Pino Sensores
const int SOUND_SENSOR_PIN = 2;
const int PIN_ULTRASSONIC = 4;

// Variáveis de Controle
bool enginesRunning = false;
const unsigned long DELAY_DEBOUNCE = 2000;
const int safeDistanceCm = 80;
bool turnRight = true;
unsigned int currentSpeed = 150;
bool waitingSilence = false;

// Variaveis alteradas na interrupcao
volatile unsigned long lastClapTime = 0;
volatile unsigned long firstClapTime = 0;
volatile int countClap = 0;

Ultrasonic ultrasonic(PIN_ULTRASSONIC);

const int PIN_LED = LED_BUILTIN;

void setup() {
  Serial.begin(9600);

  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  pinMode(PIN_IN3, OUTPUT);
  pinMode(PIN_IN4, OUTPUT);

  pinMode(SOUND_SENSOR_PIN, INPUT);

  pinMode(PIN_LED, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(SOUND_SENSOR_PIN), registerClap, FALLING);

  stopEngines();
}

void registerClap() {
  unsigned long currentTime = millis();

  if ((currentTime - lastClapTime) > 150) { 
    if (countClap == 0) {
      firstClapTime = currentTime; 
    }
    countClap++; 
    lastClapTime = currentTime;
  }
}

void loop() {
  unsigned long currentTime = millis();
  
  if (countClap > 0 && (currentTime - firstClapTime > DELAY_DEBOUNCE)) {
    
    noInterrupts();
    int numberOfClaps = countClap;
    countClap = 0;
    interrupts();

    Serial.print("Numero de palmas contadas: ");
    Serial.println(numberOfClaps);

    switch (numberOfClaps) {
      case 1:{
        Serial.println("Acao: 1 palma");
        enginesRunning = !enginesRunning;

        if (enginesRunning) {
          Serial.print("Ligando motores speed = .");
          startEngines(currentSpeed);
          Serial.println(currentSpeed);
        } else {
          Serial.println("Parando motores.");
          stopEngines();
        }
        digitalWrite(PIN_LED, HIGH);
        delay(100);
        digitalWrite(PIN_LED, LOW);
        break;
      }

      case 2:{
        Serial.println("Acao: 2 palmas");
        int controller = currentSpeed;
        if(controller >= 250) {
          for(int i = controller; i > 0; i -= 5) {
            int safeSpeed = (i <= 0) ? 0 : i;
            changeSpeed(safeSpeed, safeSpeed);
            Serial.print("Aumentando velocidade para: ");
            Serial.println(safeSpeed);
            currentSpeed = safeSpeed;
            delay(100);
            if (safeSpeed == 0) break;
          }
        } else {
            for(int i = controller; i <= controller + 51; i += 5) {
            int safeSpeed = (i > 255) ? 255 : i;
            changeSpeed(safeSpeed, safeSpeed);
            Serial.print("Aumentando velocidade para: ");
            Serial.println(safeSpeed);
            currentSpeed = safeSpeed;
            delay(100);
            if (safeSpeed == 255) break;
          }
        }
        break;
      }

      case 3:{
        Serial.println("Acao: 3 palmas");
        turnRight = !turnRight;
        Serial.print("TurnRight: ");
        Serial.println(turnRight);
        changeDirection(turnRight);
        // delay(1000);
        // startEngines(currentSpeed > 150 ? currentSpeed : 150);
        break;
      }

      default:
        Serial.print(numberOfClaps);
        Serial.println(" palmas - Comando nao reconhecido.");
        break;
    } 
  }

  if (enginesRunning) {
    long currentDistance = ultrasonic.MeasureInCentimeters();
    
    // delay(10);
    
    if (currentDistance > 0 && currentDistance <= safeDistanceCm) {
      Serial.print("Objeto encontrado a ");
      changeDirection(turnRight);
      Serial.println(" cm de distancia, mudando direcao.");
      
      delay(500); 
    }
  }
}

void changeSpeed(int speedA, int speedB) {
  if (turnRight) {
    analogWrite(PIN_IN1, 0);
    analogWrite(PIN_IN2, speedA);

    analogWrite(PIN_IN3, speedB);
    analogWrite(PIN_IN4, 0);
  }
  else {
    analogWrite(PIN_IN1, speedA);
    analogWrite(PIN_IN2, 0);

    analogWrite(PIN_IN3, 0);
    analogWrite(PIN_IN4, speedB);
  }

  currentSpeed = speedA;
}

void startEngines(int speedPWM) {
  changeSpeed(speedPWM, speedPWM);
}

void stopEngines() {
  analogWrite(PIN_IN1, 0);
  analogWrite(PIN_IN2, 0);
  analogWrite(PIN_IN3, 0);
  analogWrite(PIN_IN4, 0);
}

void changeDirection(bool right) {
  stopEngines();
  turnRight = right;

  changeSpeed(101, 101);

  if (right) {
    Serial.println("Primeiro");
  }
  else {
    Serial.println("Segundo");
  }
}
