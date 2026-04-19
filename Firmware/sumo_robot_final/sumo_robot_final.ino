/****************************************************
  Sumo Robot - Front & Back Ultrasonic + 4 IR Line Sensors
  Priority:
  1) White line escape
  2) Attack enemy with max speed
  3) Search: spin 360, then move forward 0.5s, repeat
****************************************************/

// ======================= Motors =======================
const int R_Front_IN1 = 2;
const int R_Front_IN2 = 3;
const int R_Back_IN3  = 4;
const int R_Back_IN4  = 5;
const int R_Speed_PWM = 10;

const int L_Front_IN1 = 6;
const int L_Front_IN2 = 7;
const int L_Back_IN3  = 8;
const int L_Back_IN4  = 9;
const int L_Speed_PWM = 11;

// ======================= Ultrasonic =======================
const int trigFront = 26;
const int echoFront = 27;

const int trigBack  = 28;
const int echoBack  = 29;

// ======================= IR Sensors =======================
const int IR_Front_Right = 22;
const int IR_Front_Left  = 23;
const int IR_Back_Right  = 24;
const int IR_Back_Left   = 25;

// ======================= Tuning =======================
// Change this if your white line sensor logic is inverted
const int LINE_WHITE_STATE = LOW;

// Enemy detection distance in cm
const int DETECT_DIST_CM = 45;

// Speeds
const int ATTACK_SPEED  = 255;  // Maximum attack speed
const int ESCAPE_SPEED  = 220;  // Escape speed from line
const int SEARCH_SPEED   = 240;   // Search speed

// Search timing
const unsigned long SPIN_360_MS       = 850; // Adjust for ~360° rotation
const unsigned long SEARCH_FORWARD_MS = 500; // Move forward for 0.5s

// Search state machine
enum SearchState {
  SEARCH_SPIN,
  SEARCH_FORWARD
};

SearchState searchState = SEARCH_SPIN;
unsigned long searchStateStart = 0;

// ======================= Helper Functions =======================

void setMotorRight(int speed) {
  speed = constrain(speed, -255, 255);

  if (speed > 0) {
    digitalWrite(R_Front_IN1, HIGH);
    digitalWrite(R_Front_IN2, LOW);
    digitalWrite(R_Back_IN3,  HIGH);
    digitalWrite(R_Back_IN4,  LOW);
    analogWrite(R_Speed_PWM, speed);
  } else if (speed < 0) {
    digitalWrite(R_Front_IN1, LOW);
    digitalWrite(R_Front_IN2, HIGH);
    digitalWrite(R_Back_IN3,  LOW);
    digitalWrite(R_Back_IN4,  HIGH);
    analogWrite(R_Speed_PWM, -speed);
  } else {
    digitalWrite(R_Front_IN1, LOW);
    digitalWrite(R_Front_IN2, LOW);
    digitalWrite(R_Back_IN3,  LOW);
    digitalWrite(R_Back_IN4,  LOW);
    analogWrite(R_Speed_PWM, 0);
  }
}

void setMotorLeft(int speed) {
  speed = constrain(speed, -255, 255);

  if (speed > 0) {
    digitalWrite(L_Front_IN1, HIGH);
    digitalWrite(L_Front_IN2, LOW);
    digitalWrite(L_Back_IN3,  HIGH);
    digitalWrite(L_Back_IN4,  LOW);
    analogWrite(L_Speed_PWM, speed);
  } else if (speed < 0) {
    digitalWrite(L_Front_IN1, LOW);
    digitalWrite(L_Front_IN2, HIGH);
    digitalWrite(L_Back_IN3,  LOW);
    digitalWrite(L_Back_IN4,  HIGH);
    analogWrite(L_Speed_PWM, -speed);
  } else {
    digitalWrite(L_Front_IN1, LOW);
    digitalWrite(L_Front_IN2, LOW);
    digitalWrite(L_Back_IN3,  LOW);
    digitalWrite(L_Back_IN4,  LOW);
    analogWrite(L_Speed_PWM, 0);
  }
}

// rightSpeed and leftSpeed: from -255 to 255
void drive(int rightSpeed, int leftSpeed) {
  setMotorRight(rightSpeed);
  setMotorLeft(leftSpeed);
}

void stopMotors() {
  drive(0, 0);
}

void forward(int speed) {
  drive(speed, speed);
}

void backward(int speed) {
  drive(-speed, -speed);
}

void spinRight(int speed) {
  drive(-speed, speed);
}

void spinLeft(int speed) {
  drive(speed, -speed);
}

// Smart escape movement from white line
void escapeLine(bool fr, bool fl, bool br, bool bl) {
  // Priority: front sensors
  if (fr || fl) {
    // If front right sees line, move backward with slight left turn
    if (fr && !fl) {
      drive(-ESCAPE_SPEED, -(ESCAPE_SPEED / 3));
    }
    // If front left sees line, move backward with slight right turn
    else if (fl && !fr) {
      drive(-(ESCAPE_SPEED / 3), -ESCAPE_SPEED);
    }
    // If both front sensors see line, move backward strongly
    else {
      backward(ESCAPE_SPEED);
    }
    return;
  }

  // If no front detection, check rear
  if (br || bl) {
    // If rear right sees line, move forward with slight left turn
    if (br && !bl) {
      drive(ESCAPE_SPEED, ESCAPE_SPEED / 3);
    }
    // If rear left sees line, move forward with slight right turn
    else if (bl && !br) {
      drive(ESCAPE_SPEED / 3, ESCAPE_SPEED);
    }
    // If both rear sensors see line, move forward strongly
    else {
      forward(ESCAPE_SPEED);
    }
    return;
  }
}

int readDistanceCM(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  unsigned long duration = pulseIn(echoPin, HIGH, 12000UL); // fast timeout
  if (duration == 0) return 999; // no reading

  int distance = (int)(duration * 0.0343 / 2.0);
  return distance;
}

bool isWhite(int pin) {
  return digitalRead(pin) == LINE_WHITE_STATE;
}

// ======================= Setup =======================
void setup() {
  delay (5000);
  Serial.begin(9600);

  pinMode(R_Front_IN1, OUTPUT);
  pinMode(R_Front_IN2, OUTPUT);
  pinMode(R_Back_IN3,  OUTPUT);
  pinMode(R_Back_IN4,  OUTPUT);
  pinMode(R_Speed_PWM, OUTPUT);

  pinMode(L_Front_IN1, OUTPUT);
  pinMode(L_Front_IN2, OUTPUT);
  pinMode(L_Back_IN3,  OUTPUT);
  pinMode(L_Back_IN4,  OUTPUT);
  pinMode(L_Speed_PWM, OUTPUT);

  pinMode(trigFront, OUTPUT);
  pinMode(echoFront, INPUT);

  pinMode(trigBack, OUTPUT);
  pinMode(echoBack, INPUT);

  pinMode(IR_Front_Right, INPUT);
  pinMode(IR_Front_Left, INPUT);
  pinMode(IR_Back_Right, INPUT);
  pinMode(IR_Back_Left, INPUT);

  stopMotors();
  searchStateStart = millis();
}

// ======================= Main Loop =======================
void loop() {
  // ---------- Read line sensors ----------
  bool frontRightWhite = isWhite(IR_Front_Right);
  bool frontLeftWhite  = isWhite(IR_Front_Left);
  bool backRightWhite  = isWhite(IR_Back_Right);
  bool backLeftWhite   = isWhite(IR_Back_Left);

  bool frontLine = frontRightWhite || frontLeftWhite;
  bool backLine  = backRightWhite || backLeftWhite;

  // ---------- Priority 1: escape from white line ----------
  if (frontLine || backLine) {
    escapeLine(frontRightWhite, frontLeftWhite, backRightWhite, backLeftWhite);
    return; // no other decisions before escaping
  }

  // ---------- Read ultrasonic ----------
  int distFront = readDistanceCM(trigFront, echoFront);
  int distBack  = readDistanceCM(trigBack, echoBack);

  bool enemyFront = (distFront > 0 && distFront <= DETECT_DIST_CM);
  bool enemyBack  = (distBack  > 0 && distBack  <= DETECT_DIST_CM);

  // ---------- Priority 2: attack ----------
  if (enemyFront || enemyBack) {
    if (enemyFront && enemyBack) {
      // If both detect enemy, attack the closer one
      if (distFront <= distBack) {
        forward(ATTACK_SPEED);
      } else {
        backward(ATTACK_SPEED);
      }
    } else if (enemyFront) {
      forward(ATTACK_SPEED);
    } else {
      backward(ATTACK_SPEED);
    }
    return;
  }

  // ---------- Priority 3: search ----------
  unsigned long now = millis();

  if (searchState == SEARCH_SPIN) {
    spinRight(SEARCH_SPEED);

    if (now - searchStateStart >= SPIN_360_MS) {
      searchState = SEARCH_FORWARD;
      searchStateStart = now;
    }
  } else {
    forward(SEARCH_SPEED);

    if (now - searchStateStart >= SEARCH_FORWARD_MS) {
      searchState = SEARCH_SPIN;
      searchStateStart = now;
    }
  }
}