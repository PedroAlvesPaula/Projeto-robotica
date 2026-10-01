#include <Ultrasonic.h>

#define PIN_IN1 9   // Motor direito
#define PIN_IN2 10  // Motor direito
#define PIN_IN3 5   // Motor esquerdo
#define PIN_IN4 6   // Motor esquerdo

// Pinos dos Sensores
#define SOUND_SENSOR_PIN 2
#define PIN_ULTRASSONIC 4

// Pinos do LED RGB
#define PIN_RGB_R 11 // Vermelho
#define PIN_RGB_G 12 // Verde
#define PIN_RGB_B 8  // Azul

// Variáveis de Controle
bool enginesRunning = false;
const unsigned long DELAY_DEBOUNCE = 1500;
const int safeDistanceCm = 80;
bool turnRight = true;
unsigned int currentSpeed = 80;
bool isDecrementing = false;

// Variáveis alteradas na interrupção (devem ser 'volatile')
volatile unsigned long lastClapTime = 0;
volatile unsigned long firstClapTime = 0;
volatile int countClap = 0;

Ultrasonic ultrasonic(PIN_ULTRASSONIC);

void setup() {
  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  pinMode(PIN_IN3, OUTPUT);
  pinMode(PIN_IN4, OUTPUT);

  pinMode(SOUND_SENSOR_PIN, INPUT);
  
  pinMode(PIN_RGB_R, OUTPUT);
  pinMode(PIN_RGB_G, OUTPUT);
  pinMode(PIN_RGB_B, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(SOUND_SENSOR_PIN), registerClap, FALLING);

  stopEngines();
}

void loop() {
  processSoundCommands();
  avoidObstacles();
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

void processSoundCommands() {
  unsigned long currentTime = millis();
  
  if (countClap > 0 && (currentTime - firstClapTime > DELAY_DEBOUNCE)) {
    noInterrupts();
    int numberOfClaps = countClap;
    countClap = 0;
    interrupts();

    executeClapCommand(numberOfClaps);
  }
}

void executeClapCommand(int claps) {
  switch (claps) {
    case 1: 
      togglePower(); 
      break;
    case 2: 
      manualTurn();
      break;
    case 3: 
      changeSpeed(); 
      break;
    default: 
      break;
  }
}

void togglePower() {
  enginesRunning = !enginesRunning;
  if (enginesRunning) {
    setLedColor(false, true, false); // Verde
    startEngines(currentSpeed);
  } else {
    setLedColor(false, false, false); // Desligado
    stopEngines();
  }
}

void changeSpeed() {
  if (!enginesRunning) return;
  
  int controller = currentSpeed;
  
  if (isDecrementing) {
    setLedColor(true, false, false); // Vermelho
    for (int i = controller; i >= controller - 50; i -= 5) {
      int safeSpeed = (i < 80) ? 80 : i;
      startEngines(safeSpeed);
      currentSpeed = safeSpeed;
      delay(100);
      setLedColor(true, false, false); 
      if (safeSpeed == 80) break;
    }
    if (currentSpeed <= 80) isDecrementing = false;
    setLedColor(false, true, false); // Verde
  } else {
    setLedColor(false, true, false); // Verde
    for (int i = controller; i <= controller + 50; i += 5) {
      int safeSpeed = (i > 200) ? 200 : i;
      startEngines(safeSpeed);
      currentSpeed = safeSpeed;
      delay(50);
      if (safeSpeed == 255) break;
    }
    if (currentSpeed >= 200) isDecrementing = true;
    setLedColor(false, true, false); 
  }
}

void manualTurn() {
  if (!enginesRunning) return;
  turnRight = !turnRight;
  changeDirection(turnRight, currentSpeed);
}

void avoidObstacles() {
  if (!enginesRunning) return;

  long currentDistance = ultrasonic.MeasureInCentimeters();
  
  if (currentDistance > 0 && currentDistance <= safeDistanceCm) {
    setLedColor(true, true, false); // Amarelo
    
    detachInterrupt(digitalPinToInterrupt(SOUND_SENSOR_PIN));
    
    turnRight = !turnRight;
    spinRobot(turnRight);
    
    while(currentDistance > 0 && currentDistance <= safeDistanceCm) {
      delay(100);
      currentDistance = ultrasonic.MeasureInCentimeters();
    }
    
    stopEngines();
    delay(100);
    
    attachInterrupt(digitalPinToInterrupt(SOUND_SENSOR_PIN), registerClap, FALLING);
    
    setLedColor(false, true, false); // Verde
    startEngines(currentSpeed);
  }
}

// FUNÇÕES DOS MOTORES E LEDS
void setLedColor(bool red, bool green, bool blue) {
  digitalWrite(PIN_RGB_R, red ? HIGH : LOW);
  digitalWrite(PIN_RGB_G, green ? HIGH : LOW);
  digitalWrite(PIN_RGB_B, blue ? HIGH : LOW);
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

  int turnSpeed = 100; 

  if (right) {
    setDirectionWheels(0, turnSpeed, 0, turnSpeed);
  } else {
    setDirectionWheels(turnSpeed, 0, turnSpeed, 0);
  }
  
  delay(700);
  
  stopEngines();
  delay(100);

  setLedColor(false, true, false); // Verde
  startEngines(speed);
}

void spinRobot(bool right) {
  int turnSpeed = 100; 

  if (right) {
    setDirectionWheels(0, turnSpeed, 0, turnSpeed); 
  } else {
    setDirectionWheels(turnSpeed, 0, turnSpeed, 0); 
  }
}