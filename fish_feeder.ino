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

#define PUSH_REV 0.2 * ONE_REV
#define RETRACT_REV 0.05 * ONE_REV

enum MachineState {
  BOOTING,
  WAITING,
  PUSHING,
  RETRACTING,
  TESTING
};



MachineState currentState = BOOTING;

enum BlinkingState {
  HOURS,
  MINUTES,
  HEARTBEAT,
  END_MESSAGE,
  PAUSE,
  NONE
  
};

BlinkingState blinkingState = HEARTBEAT;
int blink_count = 0;

float overRevfactor = 0.25; // Factor used to push a bit more food an retract a bit more

// Initialize with pin sequence IN1-IN3-IN2-IN4 for using the AccelStepper with 28BYJ-48
AccelStepper stepper1(HALFSTEP, motorPin1, motorPin3, motorPin2, motorPin4);


long ct = millis(); // current time
long pt = 0; // previous time
long dt = 0; // delta time

long waiting_acc = 0;
long waiting_delay = ONE_DAY;

// heart beat
long hb_acc = 0; 
long hb_delay = ONE_SECOND;
int hb_led = 13;
int hb_state = HIGH;

long pause_acc = 0;
long pause_delay = ONE_SECOND;

long msg_end_acc = 0;
long msg_end_delay = ONE_SECOND;
long msg_end_blink_delay = ONE_SECOND >> 3;

long msg_time_delay = ONE_SECOND / 3;

long booting_acc = 0;
long booting_delay = ONE_SECOND * 5;

bool stateStarted = false;

void setup() {
  Serial.begin(9600);
  Serial.println ("Setting up...");
  
  stepper1.setMaxSpeed(3000);
  stepper1.setAcceleration(50);
  stepper1.setSpeed(300);

//  Serial.println ("SETUP : Purging 4 revolutions (blocking)");
//  stepper1.moveTo(5 * ONE_REV);
//  stepper1.runToPosition();

  pinMode(hb_led, OUTPUT);

}//--(end setup )---


void loop() {
  ct = millis();
  dt = ct - pt;
  pt = ct;

  switch (currentState) {
    case BOOTING:
      bootingTask();
      break;
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


/// Just a blinking task
/// Change hb_delay in other task
void heartbeatTask() {
  hb_acc += dt;

  if (hb_acc >= hb_delay) {
    hb_acc = 0;

    blink_count++;

    hb_state = hb_state == HIGH ? LOW : HIGH;

    digitalWrite (hb_led, hb_state);
  }
  
}

int waiting_left = 0;
int waiting_left_last = 0;
int waiting_left_hours = 0;
int waiting_left_tens = 0; // tens of minutes

int start_blink_count = 0;

void waitTask() {
  waiting_acc += dt;
  
  if (!stateStarted) {
    Serial.println ("Doing waiting task");
    waiting_acc= 0;
    blink_count = 0;
    
    stepper1.disableOutputs();

    hb_delay = msg_time_delay;

    stateStarted = true;
  } else {

    // Blinking time left in hours and tens minutes
    waiting_left = (int)((waiting_delay - waiting_acc) / 60000); // Get time left in minutes
    waiting_left_tens = (waiting_left % 60) / 10;
    
    if (waiting_left_tens != waiting_left_last) {
      Serial.print ("Blinking delay : ");
      Serial.println (hb_delay);
      
      waiting_left_hours = waiting_left / 60;

      hb_delay = msg_time_delay;

      start_blink_count = blink_count;
      blinkingState = HOURS;

      Serial.print("Time left --> ");
      Serial.print(waiting_left_hours);
      Serial.print(" hours ");
      Serial.print(waiting_left_tens);
      Serial.println(" tens minute");
    }

    int blink_delta = (blink_count - start_blink_count) / 2;

    switch (blinkingState) {
      case HOURS:
        if (blink_delta >= waiting_left_hours){
          Serial.println ("Blinking hours left");
          hb_delay = ONE_SECOND * 2;
          blinkingState = PAUSE;
          start_blink_count = blink_count;
        }        
        break;
      case PAUSE:
        pause_acc += dt;
        if (pause_acc >= pause_delay) {
          pause_acc = 0;
          blinkingState = MINUTES;
          hb_delay = msg_time_delay;
        }
        break;
      case MINUTES:
        if (blink_delta >= waiting_left_tens){
          Serial.println ("Blinking tens of minutes left");
          hb_delay = msg_end_blink_delay;
          blinkingState = END_MESSAGE;
          start_blink_count = blink_count;
        }              
        break;
      case END_MESSAGE:
        msg_end_acc += dt;
        if (msg_end_acc >= msg_end_delay){
          msg_end_acc = 0;
          
          Serial.println ("Blinking end_message");
          hb_delay = msg_time_delay;
          blinkingState = HOURS;
          start_blink_count = blink_count;
        }              
        break;
      default:
        break;
    }
    
    waiting_left_last = waiting_left_tens;
  }

  // Done waiting going to next state
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
    stepper1.moveTo(PUSH_REV);

    hb_delay = 0.1 * ONE_SECOND;

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
    stepper1.moveTo(RETRACT_REV);

    hb_delay = 0.5 * ONE_SECOND;
    stateStarted = true;
  }
  
  if (stepper1.distanceToGo() == 0) { //add timeout
    Serial.println ("Retracting done");
    
    stateStarted = false;
    currentState = WAITING;
  }  
}

void bootingTask() {
  if (!stateStarted) {
    Serial.println ("Booting for 5 seconds");

    booting_acc = 0;
    blink_count = 0;
    
    stepper1.disableOutputs();

    hb_delay = msg_time_delay;

    stateStarted = true;
  } else {
    booting_acc += dt;

    if (booting_acc >= booting_delay) {
      booting_acc = 0;

      stateStarted = false;
      currentState = PUSHING;
    }
  }
  
}

void testTask() {
  
}
