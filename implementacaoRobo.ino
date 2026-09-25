#include <Ultrasonic.h>

const int PIN_IN1 = 9;   // Motor direito
const int PIN_IN2 = 10;  // Motor direito

const int PIN_IN3 = 5;   // Motor esquerdo
const int PIN_IN4 = 6;   // Motor esquerdo

// Pinos dos Sensores
const int SOUND_SENSOR_PIN = 2;
const int PIN_ULTRASSONIC = 4;

// Variáveis de Controle
bool enginesRunning = false;
const unsigned long DELAY_DEBOUNCE = 1500;
const int safeDistanceCm = 40;
bool turnRight = true;
unsigned int currentSpeed = 150; 
bool waitingSilence = false;
bool isDecrementing = false;

// Variáveis alteradas na interrupção (devem ser 'volatile')
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
      case 1: {
        Serial.println("Acao: 1 palma - Liga/Desliga");
        enginesRunning = !enginesRunning;

        if (enginesRunning) {
          Serial.print("Ligando motores speed = ");
          Serial.println(currentSpeed);
          startEngines(currentSpeed);
        } else {
          Serial.println("Parando motores.");
          stopEngines();
        }
        break;
      }

      case 2: {
        if (!enginesRunning) break;
        Serial.println("Acao: 2 palmas - Altera Velocidade");
        
        int controller = currentSpeed;
        
        if (isDecrementing) {
          for (int i = controller; i >= controller - 50; i -= 5) {
            int safeSpeed = (i < 80) ? 80 : i;
            startEngines(safeSpeed);
            Serial.print("Diminuindo velocidade para: ");
            Serial.println(safeSpeed);
            currentSpeed = safeSpeed;
            delay(100);
            if (safeSpeed == 80) break;
          }
          if (currentSpeed <= 80) isDecrementing = false;
        } else {
          for (int i = controller; i <= controller + 50; i += 5) {
            int safeSpeed = (i > 255) ? 255 : i;
            startEngines(safeSpeed);
            Serial.print("Aumentando velocidade para: ");
            Serial.println(safeSpeed);
            currentSpeed = safeSpeed;
            delay(100);
            if (safeSpeed == 255) break;
          }
          if (currentSpeed >= 250) isDecrementing = true; // Inverte o ciclo
        }
        break;
      }

      case 3: {
        if (!enginesRunning) break;
        Serial.println("Acao: 3 palmas - Muda Direcao Manualmente");
        turnRight = !turnRight;
        changeDirection(turnRight, currentSpeed);
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
    
    if (currentDistance > 0 && currentDistance <= safeDistanceCm) {
      Serial.print("Objeto encontrado a ");
      Serial.print(currentDistance);
      Serial.println(" cm. Mudando direcao automaticamente.");
      
      turnRight = !turnRight;
      changeDirection(turnRight, currentSpeed);
    }
  }
}

void setDirectionWheels(int IN1, int IN2, int IN3, int IN4) {
  analogWrite(PIN_IN1, IN1);
  analogWrite(PIN_IN2, IN2);
  analogWrite(PIN_IN3, IN3);
  analogWrite(PIN_IN4, IN4);
}

void startEngines(int speedPWM) {
  setDirectionWheels(speedPWM, 0, 0, speedPWM);
}

void stopEngines() {
  setDirectionWheels(0, 0, 0, 0);
}

void changeDirection(bool right, int speed) {
  stopEngines();
  delay(100);

  int turnSpeed = 180; 

  if (right) {
    setDirectionWheels(0, turnSpeed, 0, turnSpeed);
  } else {
    setDirectionWheels(turnSpeed, 0, turnSpeed, 0);
  }
  
  delay(800);
  
  stopEngines();
  delay(100);
  startEngines(speed);
}
