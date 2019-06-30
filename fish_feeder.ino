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

void setup() {
  Serial.begin(9600);
  Serial.println ("Setting up...");
  
  stepper1.setMaxSpeed(2000);
  stepper1.setAcceleration(50);
  stepper1.setSpeed(300);

  Serial.println ("SETUP : Moving to 800 (blocking)");
  stepper1.moveTo(800);
  stepper1.runToPosition();

//  Serial.println ("SETUP : Moving to -5000 (non-blocking)");
//  stepper1.moveTo(-5000);
}//--(end setup )---

long ct = millis(); // current time
long pt = 0; // previous time
long dt = 0; // delta time

long run_acc = 0;
long run_delay = 10 * ONE_SECOND;

bool stateStarted = false;

void loop() {
  ct = millis();
  dt = ct - pt;
  pt = ct;

  run_acc += dt;


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

  stepper1.run();
}

void waitTask() {
  if (!stateStarted) {
    Serial.println ("Doing waiting task");
    run_acc = 0;
    
    stepper1.disableOutputs();
    
    stateStarted = true;
  } else {
    
  }
  
  if (run_acc >= run_delay) {
    Serial.println ("Waiting done");
    run_acc = 0;

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
