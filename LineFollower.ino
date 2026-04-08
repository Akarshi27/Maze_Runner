/*
 * FINAL FIX: SAWTOOTH/ZIG-ZAG SOLVER + BLACK BOX STOP + WIDE SENSOR CATCH
 * Added: 2 extra side IR sensors that only activate when the main 8-array reads 0.
 * Modified: Case 1 updated to prioritize the extra side sensors for recovery.
 */

// ================= PIN DEFINITIONS =================
const int sensorPins[] = {26, 25, 33, 32, 35, 34, 39, 36};
const int sensorCount = 8;
const int weights[] = {-12, -7, -3, -1, 1, 3, 7, 12};
const int ledPin = 2; // LED connected to D2

// ================= NEW EXTRA SENSORS =================
// Assign these to the actual ESP32 pins you are using for the 2 new sensors
const int extraLeftPin = 12;  
const int extraRightPin = 14; 

// Motor Pins
const int PWMA = 18;
const int AIN1 = 5;
const int AIN2 = 21;
const int PWMB = 19;
const int BIN1 = 22;
const int BIN2 = 4;

// ================= TUNING PARAMETERS =================
float Kp = 18; 
float Kd = 25; 

int normalSpeed = 200;
int turnSpeed   = 180; 
int pivotSpeed  = 180; // Adjusted for snappy center rotation

// ================= VARIABLES =================
float lastError = 0;
int lastCorner = 1; 

// Stop Timer Variables
unsigned long blackStartTime = 0; 
bool isTimerRunning = false;
bool robotStopped = false; 

void setup() {
  for(int i = 0; i < sensorCount; i++) {
    pinMode(sensorPins[i], INPUT);
  }

  // Initialize new extra sensors
  pinMode(extraLeftPin, INPUT);
  pinMode(extraRightPin, INPUT);

  pinMode(ledPin, OUTPUT); // Initialize LED pin
  digitalWrite(ledPin, LOW); // Ensure LED is off at start

  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);

  ledcAttach(PWMA, 15000, 8);
  ledcAttach(PWMB, 15000, 8);
}

void loop() {
  if (robotStopped) {
    move(0, 0);
    digitalWrite(ledPin, HIGH); // Keep LED glowing
    return;
  }

  int active = 0;
  int sum = 0;
  bool s[8];

  for(int i = 0; i < sensorCount; i++) {
    s[i] = (digitalRead(sensorPins[i]) == HIGH); 
    if(s[i]) {
      active++;
      sum += weights[i];
    }
  }

  // ================= BLACK BOX DETECTION =================
  if (active >= 7) {
    if (!isTimerRunning) {
      blackStartTime = millis(); 
      isTimerRunning = true;
    } else {
      if (millis() - blackStartTime >= 75) {
        robotStopped = true;
        move(0, 0);
        digitalWrite(ledPin, HIGH); // Glow LED on D2
        return;
      }
    }
  } else {
    isTimerRunning = false;
  }

  // ================= MEMORY UPDATE =================
  if (s[0]) lastCorner = -1; 
  if (s[7]) lastCorner = 1;  

  // ================= CASE 1: LINE LOST (EXTRA SENSORS ACTIVE) =================
  if(active == 0) {
    // Read the two new extra sensors ONLY when main array is 0
    bool extLeft = (digitalRead(extraLeftPin) == HIGH);
    bool extRight = (digitalRead(extraRightPin) == HIGH);

    if (extLeft && !extRight) {
      // Only Left is HIGH -> Spin Left to catch the line
      move(-pivotSpeed, pivotSpeed); 
    } 
    else if (!extLeft && extRight) {
      // Only Right is HIGH -> Spin Right to catch the line
      move(pivotSpeed, -pivotSpeed); 
    } 
    else {
      // Both HIGH or Both LOW -> Move Straight (Per your specific instructions)
      move(normalSpeed, normalSpeed); 
      
      /* * CANDID NOTE: Moving straight when ALL 10 sensors are LOW means the bot 
       * will drive forward blindly if it gets completely picked up or lost. 
       * If you ever want to revert to your old memory-spin when all 10 are lost, 
       * delete the move(normalSpeed, normalSpeed); above and uncomment this:
       *
       * if(lastCorner == 1) {
       * move(pivotSpeed, -pivotSpeed); 
       * } else {
       * move(-pivotSpeed, pivotSpeed); 
       * }
       */
    }
    return;
  }

  // ================= CHECK PATH STATUS =================
  bool straightPathExists = (s[2] || s[3] || s[4] || s[5]);
  
  // ================= CASE 2: SHARP 90 DEGREE TURNS =================
  if (!straightPathExists) {
    if (s[0] && !s[7]) { 
      move(-120, 120); 
      return;
    }
    if (s[7] && !s[0]) { 
      move(120, -120); 
      return;
    }
  }

  // ================= CASE 3: PID CONTROL =================
  float error = (float)sum / active;
  float derivative = error - lastError;
  float correction = (Kp * error) + (Kd * derivative);
  
  lastError = error;

  int effectiveSpeed = normalSpeed;
  if (abs(error) > 2) effectiveSpeed = 150; 

  move(effectiveSpeed + correction, effectiveSpeed - correction);
}

// ================= MOTOR HELPER =================
void move(int left, int right) {
  left  = constrain(left, -255, 255);
  right = constrain(right, -255, 255);

  digitalWrite(AIN1, left >= 0);
  digitalWrite(AIN2, left < 0);
  ledcWrite(PWMA, abs(left));

  digitalWrite(BIN1, right >= 0);
  digitalWrite(BIN2, right < 0);
  ledcWrite(PWMB, abs(right));
}void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
