#include "motor.h"

int target_turn_pwm = 19;
// General PID used to match encoder ticks from both motors to get it to drive straight
double pid_setpoint, pid_output, pid_input;
double Kp, Ki, Kd;
PID straightDrivePID(&pid_input, &pid_output, &pid_setpoint, Kp, Ki, Kd, DIRECT);
uint8_t leftpwm, rightpwm;

Motor::Motor(int pinA, int pinB, int encoderPin, int sleepPin): pinA(pinA), pinB(pinB), encoderPin(encoderPin), sleepPin(sleepPin), pidController(&speed, &Output, &Setpoint, Kp, Ki, Kd, DIRECT) {}

void Motor::begin() {
  pinMode(pinA, OUTPUT);
  pinMode(pinB, OUTPUT);
  pinMode(sleepPin, OUTPUT);
  // pinMode(encoderPin, INPUT);
  // configure LEDC PWM
  ledcAttachChannel(pinA, 5000, 8, 0);
  // configure LEDC PWM
  ledcAttachChannel(pinB, 5000, 8, 0);
  // pidController.SetOutputLimits(0, 255-target_turn_pwm);
  // pidController.SetMode(AUTOMATIC);
}

void Motor::enable() {
  digitalWrite(sleepPin, HIGH);
}

void Motor::disable() {
  digitalWrite(sleepPin, LOW);
}

void Motor::rotateCW(uint8_t pwm) {
  ledcWrite(pinA, pwm);
  ledcWrite(pinB, 0);
}

void Motor::rotateCCW(uint8_t pwm) {
  analogWrite(pinA, 0);
  analogWrite(pinB, pwm);
}

void Motor::coast() {
  ledcWrite(pinA, 0);
  ledcWrite(pinB, 0);
}

void Motor::activeBreak() {
  analogWrite(pinA, 255);
  analogWrite(pinB, 255);
}

void Motor::encoderUpdate() {
  // CW increments and CCW decrements
  // read from i2c
  uint16_t value = 0;
  i2cLock.lock();
  i2c.beginTransmission(encoderPin); // Use encoderPin as I2C address
  i2c.requestFrom(encoderPin, 2);    // Request 2 bytes
  if (i2c.available()) {
    value = i2c.read();          // Read high byte
    encoder = (value << 8) | i2c.read(); // Read low byte
  }
  i2c.endTransmission();
  i2cLock.unlock();
}

void Motor::calculateSpeed() { // uses rolling average filter

  int32_t curTime = micros();
  encBuf[encBufIdx] = encoder;
  encBufTime[encBufIdx] = curTime;
  encBufIdx = (encBufIdx + 1) % encBufLength;

  if (!encBufInitialized) {
    if (encBufIdx == (encBufLength-1)) {
      encBufInitialized = true;
      encBuf[encBufIdx] = encoder;
      encBufTime[encBufIdx] = curTime + 1;
      encBufIdx = (encBufIdx + 1) % encBufLength;
    } else {
      return;
    }
  }

  float sum = 0;
  for (int i = 0; i < encBufLength-1; i++) {
    int a = (i+encBufIdx) % encBufLength;
    int b = (a+1) % encBufLength;
    sum += 1e6f * (encBuf[b] - encBuf[a]) / (encBufTime[b] - encBufTime[a]);
  }

  speed = fabs(sum / (encBufLength-1) / TICKS_PER_REV); // rps

}

void updateBothEncoders() {
  leftMotor.encoderUpdate();
  rightMotor.encoderUpdate();
}

static void updateRightEncoder() {
  rightMotor.encoderUpdate();
}

static void updateLeftEncoder() {
  leftMotor.encoderUpdate();
}

void forward(uint8_t pwmL, uint8_t pwmR){
  leftMotor.rotateCCW(pwmR * leftFactor);
  rightMotor.rotateCW(pwmL * rightFactor);
}

void coast(){
  leftMotor.coast();
  rightMotor.coast();
}

void activeBreak() {
  leftMotor.activeBreak();
  rightMotor.activeBreak();
}

void calculateMotorSpeeds() {
  while (1) {
    leftMotor.calculateSpeed();
    rightMotor.calculateSpeed();
    delay(2);
  }
}

void controlMotorSpeedsForTurning() {
  leftMotor.Setpoint = 0.2;
  rightMotor.Setpoint = 0.2;
  while(1){      
    //Run the PID calculation
    leftMotor.pidController.Compute();
    rightMotor.pidController.Compute();

    //spinCCW(uint8_t pwmL, uint8_t pwmR)
    spinCCW(target_turn_pwm+leftMotor.Output, target_turn_pwm+rightMotor.Output);
    
    delay(2);
  }  
}

void driveStraight() {

  uint8_t target_pwm = 35; 
  pid_setpoint = 0;
  leftMotor.encoder = 0;
  rightMotor.encoder = 0;
  while(1) {
    //pid_setpoint is 0, defined above
    pid_input = fabs(leftMotor.encoder) - rightMotor.encoder;
    // acc_error += pid_input;
    
    // P = pid_input * Kp;
    // I = acc_error * Ki * 0.001;
    // D = Kd * (pid_input - prev_error) / 0.001;

    //Run the PID calculation
    straightDrivePID.Compute();

    if (pid_input > 0){
      // leftpwm = target_pwm; 
      // rightpwm = target_pwm + fabs(pid_output);
      forward(pwm_straight_drive, pwm_straight_drive + fabs(pid_output));
    } else {
      forward(pwm_straight_drive + fabs(pid_output), pwm_straight_drive);
      // leftpwm = target_pwm + fabs(pid_output);
      // rightpwm = target_pwm;
    }
    // prev_error = pid_input; 
    delay(2);
  }
}

void TurnInPlaceByNumDegrees(float degrees){
  uint8_t target_pwm = 35; 

  bool clockwise = degrees > 0;
  float turnDist = robotWidth*(fabs(degrees))/360.0f; // turn distance of each wheel
  uint16_t numticks = uint16_t((turnDist*TICKS_PER_REV)/(wheelDiameter)); // number of encoder ticks needed to turn provided number of degrees

  const int R_startPos = rightMotor.encoder;
  const int L_startPos = leftMotor.encoder;

  if (clockwise) { // Left wheel goes forward, right wheel goes back => both wheels spin CCW
    rightMotor.rotateCCW(target_pwm); // right motor spins slower than left motor by this factor
    leftMotor.rotateCCW(target_pwm);

  } else { // Right wheel goes forward, left wheel goes back => both wheels spin CW
    rightMotor.rotateCW(target_pwm);
    leftMotor.rotateCW(target_pwm);
  }

  bool Rturning  = true, Lturning = true;
  while (Rturning || Lturning) {
    if (fabs(rightMotor.encoder - R_startPos) >= numticks) {
      rightMotor.coast();
      Rturning = false;
    }
    if (fabs(leftMotor.encoder - L_startPos) >= numticks) {
      leftMotor.coast();
      Lturning = false;
    }
  }
}

void spinCCW(uint8_t pwmL, uint8_t pwmR) {
  pwmL *= leftFactor;
  pwmR *= rightFactor;
  leftMotor.rotateCW(pwmL);
  rightMotor.rotateCW(pwmR);
}

void spinCW(uint8_t pwmL, uint8_t pwmR) {
  pwmL *= leftFactor;
  pwmR *= rightFactor;
  leftMotor.rotateCCW(pwmL);
  rightMotor.rotateCCW(pwmR);
}