#include <AccelStepper.h>
#define HALFSTEP 8

// Motor pin definitions
#define motorPin1  2     // IN1 on the ULN2003 driver 1
#define motorPin2  3     // IN2 on the ULN2003 driver 1
#define motorPin3  4     // IN3 on the ULN2003 driver 1
#define motorPin4  5     // IN4 on the ULN2003 driver 1

#define ONE_SECOND 1000
#define ONE_MINUTE 60000
#define ONE_HOUR ONE_MINUTE * 60
#define ONE_DAY ONE_HOUR * 24
#define ONE_REV -4000 // Number of steps required for a full rotation

enum MachineState {
  WAITING,
  PUSHING,
  RETRACTING,
  TESTING
};

MachineState currentState = WAITING;
float overRevfactor = 0.25; // Factor used to push a bit more food an retract a bit more

// Initialize with pin sequence IN1-IN3-IN2-IN4 for using the AccelStepper with 28BYJ-48
AccelStepper stepper1(HALFSTEP, motorPin1, motorPin3, motorPin2, motorPin4);


long ct = millis(); // current time
long pt = 0; // previous time
long dt = 0; // delta time

long waiting_acc = 0;
long waiting_delay = 10 * ONE_SECOND;

// heart beat
long hb_acc = 0; 
long hb_delay = ONE_SECOND;
int hb_led = 13;
int hb_state = HIGH;

bool stateStarted = false;

void setup() {
  Serial.begin(9600);
  Serial.println ("Setting up...");
  
  stepper1.setMaxSpeed(3000);
  stepper1.setAcceleration(50);
  stepper1.setSpeed(300);

  Serial.println ("SETUP : Purging 4 revolutions (blocking)");
  stepper1.moveTo(4 * ONE_REV);
  stepper1.runToPosition();

  pinMode(hb_led, OUTPUT);

}//--(end setup )---


void loop() {
  ct = millis();
  dt = ct - pt;
  pt = ct;

  switch (currentState) {
    case WAITING:
      waitTask();
      break;
    case PUSHING:
      pushTask();
      break;
    case RETRACTING:
      retractTask();
      break;
    case TESTING:
      testTask();
      break;
  }

  heartbeatTask();
  stepper1.run();
}

void heartbeatTask() {
  hb_acc += dt;

  if (hb_acc >= hb_delay) {
    hb_acc = 0;

    hb_state = hb_state == HIGH ? LOW : HIGH;

    digitalWrite (hb_led, hb_state);
  }
  
}

void waitTask() {
  waiting_acc += dt;
  
  if (!stateStarted) {
    Serial.println ("Doing waiting task");
    waiting_acc= 0;
    
    stepper1.disableOutputs();

    hb_delay = ONE_SECOND;
    stateStarted = true;
  } else {
    
  }
  
  if (waiting_acc >= waiting_delay) {
    Serial.println ("Waiting done");
    waiting_acc = 0;

    stateStarted = false;
    currentState = PUSHING;
  }
}

void pushTask() {
  if (!stateStarted) {
    Serial.println ("Doing pushing task");
    stepper1.enableOutputs();
    stepper1.setCurrentPosition(0);
    stepper1.moveTo((1 + 2 * overRevfactor) * ONE_REV);

    hb_delay = 0.2 * ONE_SECOND;

    stateStarted = true;
  }
  
  if (stepper1.distanceToGo() == 0) { //add timeout
    Serial.println ("Pushing done");
    
    stateStarted = false;
    currentState = RETRACTING;
  }
}

void retractTask(){
  if (!stateStarted) {
    Serial.println ("Doing retracting task");

    stepper1.setCurrentPosition(0);
    stepper1.moveTo(-overRevfactor * ONE_REV);

    hb_delay = 0.5 * ONE_SECOND;
    stateStarted = true;
  }
  
  if (stepper1.distanceToGo() == 0) { //add timeout
    Serial.println ("Retracting done");
    
    stateStarted = false;
    currentState = WAITING;
  }  
}

void testTask() {
  
}
