#include <Ultrasonic.h>

const int PIN_IN1 = 9;   // Motor direito
const int PIN_IN2 = 10;  // Motor direito

const int PIN_IN3 = 5;   // Motor esquerdo
const int PIN_IN4 = 6;   // Motor esquerdo

// Pinos dos Sensores
const int SOUND_SENSOR_PIN = 2;
const int PIN_ULTRASSONIC = 4;

// Pinos do LED RGB
const int PIN_RGB_R = 11; // Vermelho
const int PIN_RGB_G = 12; // Verde
const int PIN_RGB_B = 8;  // Azul

// Variáveis de Controle
bool enginesRunning = false;
const unsigned long DELAY_DEBOUNCE = 1500;
const int safeDistanceCm = 40;
bool turnRight = true;
unsigned int currentSpeed = 150; 
bool waitingSilence = false;
bool isDecrementing = false;
volatile bool ignoreClap = false;

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

  // Configuração dos pinos do LED RGB
  pinMode(PIN_RGB_R, OUTPUT);
  pinMode(PIN_RGB_G, OUTPUT);
  pinMode(PIN_RGB_B, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(SOUND_SENSOR_PIN), registerClap, FALLING);

  stopEngines();
}

void registerClap() {
  if (ignoreClap) return; // Ignora palmas se estiver virando

  unsigned long currentTime = millis();

  if ((currentTime - lastClapTime) > 150) { 
    if (countClap == 0) {
      firstClapTime = currentTime; 
    }
    countClap++; 
    lastClapTime = currentTime;
  }
}

void setLedColor(bool red, bool green, bool blue) {
  digitalWrite(PIN_RGB_R, red ? HIGH : LOW);
  digitalWrite(PIN_RGB_G, green ? HIGH : LOW);
  digitalWrite(PIN_RGB_B, blue ? HIGH : LOW);
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
          setLedColor(false, true, false); // Verde
          startEngines(currentSpeed);
        } else {
          Serial.println("Parando motores.");
          setLedColor(false, false, false); // Desligado
          stopEngines();
        }
        break;
      }

      case 2: {
        if (!enginesRunning) break;
        Serial.println("Acao: 2 palmas - Altera Velocidade");
        
        int controller = currentSpeed;
        
        if (isDecrementing) {
          setLedColor(true, false, false); // Vermelho
          for (int i = controller; i >= controller - 50; i -= 5) {
            int safeSpeed = (i < 80) ? 80 : i;
            startEngines(safeSpeed);
            Serial.print("Diminuindo velocidade para: ");
            Serial.println(safeSpeed);
            currentSpeed = safeSpeed;
            delay(100);
            setLedColor(true, false, false); // Vermelho
            if (safeSpeed == 80) break;
          }
          if (currentSpeed <= 80) isDecrementing = false;
          setLedColor(false, true, false); // Verde
        } else {
          setLedColor(false, true, false); // Verde
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
          setLedColor(false, true, false); // Verde
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
      setLedColor(true, true, false); // Amarelo
      ignoreClap = true; // Ignora palmas enquanto vira
      turnRight = !turnRight;
      // changeDirection(turnRight, currentSpeed);

      // Gira enquanto houver obstáculo à frente
      spinRobot(turnRight);
      while(currentDistance > 0 && currentDistance <= safeDistanceCm) {
        delay(100);
        currentDistance = ultrasonic.MeasureInCentimeters();
      }

      // Espera um pouco antes de retomar a direção original
      stopEngines();
      delay(100);
      ignoreClap = false; // Permite palmas novamente
      setLedColor(false, true, false); // Verde
      startEngines(currentSpeed);
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
  setLedColor(true, true, false); // Amarelo
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

  setLedColor(false, true, false); // Verde
  startEngines(speed);
}

void spinRobot(bool right) {
  int turnSpeed = 150; // Precisa validar esse valor, a IA disse que era bom

  if (right) {
    setDirectionWheels(0, turnSpeed, 0, turnSpeed); // Direita
  } else {
    setDirectionWheels(turnSpeed, 0, turnSpeed, 0); // Esquerda
  }
}