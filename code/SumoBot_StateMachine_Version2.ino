// ==== CONFIGURATION & PIN DEFINITIONS ====
const int IN1 = 8, IN2 = 7, ENA = 9;           // Left Motor
const int IN3 = 5, IN4 = 4, ENB = 3;           // Right Motor

const int trigPin1 = 10, echoPin1 = 11;        // Ultrasonic sensor 1
const int trigPin2 = 12, echoPin2 = 13;        // Ultrasonic sensor 2 (if used)
const int proxPin = A2;                        // Proximity sensor (digital or analog)
const int leftLineSensor = A0, rightLineSensor = A1;

// === Adjustable parameters ===
const int lineThreshold = 300;        // IR line sensor threshold (calibrate!)
const int searchTurnTime = 250;       // ms to turn during SEARCH
const int avoidTime = 350;            // ms for reversing on edge approach
const int attackDistance = 30;        // cm to start attack
const int superPushDistance = 8;      // cm to activate "super push"
const int ultraSampleTimeout = 2000;  // us, pulseIn timeout for US (short = more responsive)
const int minTurnTime = 150;          // ms, minimum turn after edge avoid

// === State Definitions ===
enum State { SEARCH, ATTACK, EDGE_AVOID, SUPER_PUSH, RECOVER };
State botState = SEARCH;
unsigned long stateStartTime = 0;

// === Utility macros ===
#define BOTH_EDGES (getLineSensor(leftLineSensor) || getLineSensor(rightLineSensor))

// ==== SENSOR READ FUNCTIONS ====
// IR Line Sensor - returns true if line detected (black = 0)
bool getLineSensor(int pin) {
    return (analogRead(pin) < lineThreshold);
}

// Ultrasonic Distance
long readUltrasonicCM(int trigPin, int echoPin) {
    digitalWrite(trigPin, LOW); delayMicroseconds(2);
    digitalWrite(trigPin, HIGH); delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    long duration = pulseIn(echoPin, HIGH, ultraSampleTimeout);
    if (duration == 0) return 999; // timeout: no echo = nothing detected
    return duration * 0.034 / 2;
}

// Proximity sensor (stub, logic can be digital or analog!)
bool detectProximity() {
    // Example: if prox sensor is digital
    return digitalRead(proxPin) == LOW;
    // For analog proximity, use analogRead and threshold instead
}

// ==== MOTOR COMMANDS ====
void moveForward(int speed = 255) {
    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW); analogWrite(ENA, speed);
    digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW); analogWrite(ENB, speed);
}
void stopMotors() {
    digitalWrite(IN1, LOW); digitalWrite(IN2, LOW); analogWrite(ENA, 0);
    digitalWrite(IN3, LOW); digitalWrite(IN4, LOW); analogWrite(ENB, 0);
}
void reverse(int speed = 255) {
    digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH); analogWrite(ENA, speed);
    digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH); analogWrite(ENB, speed);
}
void turnRight(int speed = 200) {
    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW); analogWrite(ENA, speed);
    digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH); analogWrite(ENB, speed);
}
void turnLeft(int speed = 200) {
    digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH); analogWrite(ENA, speed);
    digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW); analogWrite(ENB, speed);
}

// ==== SETUP ====
void setup() {
    // Motor pins
    pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT); pinMode(ENA, OUTPUT);
    pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT); pinMode(ENB, OUTPUT);
    // Sensors
    pinMode(trigPin1, OUTPUT); pinMode(echoPin1, INPUT);
    pinMode(trigPin2, OUTPUT); pinMode(echoPin2, INPUT);
    pinMode(proxPin, INPUT);
    pinMode(leftLineSensor, INPUT);
    pinMode(rightLineSensor, INPUT);
    Serial.begin(9600);
    stopMotors();
}

// ==== MAIN LOOP: NON-BLOCKING STATE MACHINE ====
void loop() {
    unsigned long now = millis();
    
    // --- Sensor reads ---
    long enemyL = readUltrasonicCM(trigPin1, echoPin1);
    long enemyR = readUltrasonicCM(trigPin2, echoPin2);
    bool edgeLeft = getLineSensor(leftLineSensor);
    bool edgeRight = getLineSensor(rightLineSensor);
    bool nearProx = detectProximity();

    // --- State transitions and core logic ---
    switch (botState) {
        case SEARCH:
            // Bot rotates until enemy is seen (either US sensor)
            turnRight();
            if (enemyL < attackDistance || enemyR < attackDistance) {
                botState = ATTACK;
                stateStartTime = now;
            }
            if (edgeLeft || edgeRight) {
                botState = EDGE_AVOID;
                stateStartTime = now;
            }
            if (now - stateStartTime > searchTurnTime) {
                stateStartTime = now; // Continue SEARCH turn in steps
            }
            break;
        case ATTACK:
            // Move forward, check for line or close proximity
            moveForward();
            if (edgeLeft || edgeRight) {
                botState = EDGE_AVOID;
                stateStartTime = now;
                stopMotors();
            } else if (enemyL > attackDistance && enemyR > attackDistance) {
                botState = SEARCH;
                stateStartTime = now;
                stopMotors();
            } else if (nearProx || enemyL < superPushDistance || enemyR < superPushDistance) {
                botState = SUPER_PUSH;
                stateStartTime = now;
            }
            break;
        case SUPER_PUSH:
            // Ram forward at high speed for a short time, unless line sensor triggers
            moveForward(255);
            if (edgeLeft || edgeRight) {
                botState = EDGE_AVOID;
                stateStartTime = now;
                stopMotors();
            }
            // After 400ms, return to ATTACK
            if (now - stateStartTime > 400) {
                botState = ATTACK;
                stateStartTime = now;
            }
            break;
        case EDGE_AVOID:
            // Reverse and turn away from edge, non-blocking
            reverse();
            if (now - stateStartTime < avoidTime) {
                // Keep reversing
            } else if (now - stateStartTime < (avoidTime + minTurnTime)) {
                // Then turn in place to get away from edge
                if (edgeLeft) turnRight();
                else if (edgeRight) turnLeft();
                else turnRight();
            } else {
                // After avoidance, search for enemy again
                stopMotors();
                botState = SEARCH;
                stateStartTime = now;
            }
            break;
        default:
            stopMotors();
            botState = SEARCH;
            stateStartTime = now;
            break;
    }

#ifdef VERBOSE
    // Serial debug block for tuning/calibration
    Serial.print("STATE: "); Serial.print(botState);
    Serial.print(" | LLine: "); Serial.print(edgeLeft);
    Serial.print(" | RLine: "); Serial.print(edgeRight);
    Serial.print(" | USL: "); Serial.print(enemyL);
    Serial.print(" | USR: "); Serial.print(enemyR);
    Serial.print(" | Prox: "); Serial.println(nearProx);
#endif
}