// Motor Pin Definitions
const int IN1 = 23, IN2 = 22, ENA = 4;
const int IN3 = 25, IN4 = 24, ENB = 5;
int threshold=30;
// Sensor pins
const int trigPin = 30, echoPin = 31;
const int leftLineSensor = A0, rightLineSensor = A1;

void setup() {
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(ENB, OUTPUT);
  
  pinMode(trigPin, OUTPUT); pinMode(echoPin, INPUT);
  pinMode(leftLineSensor, INPUT);
  pinMode(rightLineSensor, INPUT);
  
  Serial.begin(9600);
}

long readUltrasonicDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  return duration * 0.034 / 2;
}

void moveForward() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  analogWrite(ENA, 255);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENB, 255);
}


void reverse() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  analogWrite(ENA, 190);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  analogWrite(ENB, 190);
}

void turnRight() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
   analogWrite(ENA, 190);
}

void loop() {
  long distance = readUltrasonicDistance();
  int leftLine = analogRead(leftLineSensor);
  int rightLine = analogRead(rightLineSensor);

  // Avoid ring edges
  if (leftLine < threshold || rightLine < threshold) {
    reverse();
    delay(300);
    turnRight();
    delay(300);
    moveForward();
    delay(200);
  } else {
    // Attack if opponent detected
    if (distance < 30) {
      moveForward();
    } else {
      // Search for opponent
      turnRight();
      delay(300);
    }
  }
}
  
