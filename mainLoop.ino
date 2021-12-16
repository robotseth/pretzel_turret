#include <SPI.h>
#include <HighPowerStepperDriver.h>

#define INPUT_SIZE 10
#define puncherDirPin 3
#define puncherStepPin 2
#define yDirPin 6
#define yStepPin 7
#define xDirPin 9
#define xStepPin 8
#define CSPin 4
#define pCSPin 5                                                                                          

unsigned long previousMicrosX = 0;
unsigned long previousMicrosY = 0;
unsigned long previousMicrosP = 0;
long xProportion = 300000; // the value that is divided by error to get delay between steps
byte xShift = 1000; // the number that is added to the delay between steps value
long yProportion = 150000; // the value that is divided by error to get delay between steps
byte yShift = 1000; // the number that is added to the delay between steps value
int fireSteps = 0;
bool fireTriggered = 0;
bool fPin = 0;
int xError = 0;
int yError = 0;
int prevXError = 0;
int prevYError = 0;
int prevErrorsY[5] = {0, 0, 0, 0, 0};
int errorIndexY = 0;
int dGainX = 1000;
int dGainY = 1000;

HighPowerStepperDriver sd;
HighPowerStepperDriver psd;

void setup() {

  SPI.begin();
  sd.setChipSelectPin(CSPin);
  psd.setChipSelectPin(pCSPin);

  // Give the driver some time to power up.
  delay(1);

  // Reset the driver to its default settings and clear latched status
  // conditions.
  sd.resetSettings();
  sd.clearStatus();
  psd.resetSettings();
  psd.clearStatus();

  // Select auto mixed decay.  TI's DRV8711 documentation recommends this mode
  // for most applications, and we find that it usually works well.
  sd.setDecayMode(HPSDDecayMode::AutoMixed);
  psd.setDecayMode(HPSDDecayMode::AutoMixed);

  // Set the current limit. You should change the number here to an appropriate
  // value for your particular system.
  sd.setCurrentMilliamps36v4(2000);
  psd.setCurrentMilliamps36v4(4000);

  // Set the number of microsteps that correspond to one full step.
  sd.setStepMode(HPSDStepMode::MicroStep16);
  psd.setStepMode(HPSDStepMode::MicroStep8);
  
  // Enable the motor outputs.
  sd.enableDriver();
  psd.enableDriver();
  
  pinMode(xStepPin, OUTPUT);
  digitalWrite(xStepPin, LOW);
  pinMode(yStepPin, OUTPUT);
  digitalWrite(yStepPin, LOW);
  pinMode(puncherStepPin, OUTPUT);
  digitalWrite(puncherStepPin, LOW);
  pinMode(xDirPin, OUTPUT);
  digitalWrite(xDirPin, LOW);
  pinMode(yDirPin, OUTPUT);
  digitalWrite(yDirPin, LOW);
  pinMode(puncherDirPin, OUTPUT);
  digitalWrite(puncherDirPin, HIGH);
  
  Serial.begin(115200);

  // This is untested but should maybe help
  while (!Serial)
    {}  // wait for Serial comms to become ready
    Serial.println("Fab");
delay(3000);
  
}

void parseCommands() {
  int errorSumY = 0;
  prevErrorsY[errorIndexY] = yError;
  errorIndexY++;
  if (errorIndexY > 5){
    errorIndexY = 0;
  }
  for(int i = 0; i<5 ; i++){
    errorSumY+=prevErrorsY[i];
  }
  prevXError = xError;
  prevYError = errorSumY / 5;
  // Get next command from Serial (add 1 for final 0)
  char input[INPUT_SIZE + 1];
  byte size = Serial.readBytes(input, INPUT_SIZE);
  // Add the final 0 to end the C string
  input[size] = 0;

  Serial.print("The input recieved is: ");
  Serial.println(input);
  
  // Read each command pair 
  char* command = strtok(input, "&");
  while (command != 0)
  {
      // Split the command in two values
      char* separator = strchr(command, ':');
      if (separator != 0)
      {
          // Actually split the string in 2: replace ':' with 0
          *separator = 0;
          xError = atoi(command);
          ++separator;
          yError = atoi(separator);
  
          // Do something with servoId and position
      }
      // Find the next command in input string
      command = strtok(0, "&");
  }
}

void moveMotors() {
  // set direction
  if (xError >= 0){
    digitalWrite(xDirPin, HIGH);
  }
  else{
    digitalWrite(xDirPin, LOW);
  }
  if (yError >= 0){
    digitalWrite(yDirPin, LOW);
  }
  else{
    digitalWrite(yDirPin, HIGH);
  }
  
  // calculate motor delay from data
  int stepTimeX = 0;
  int stepTimeY = 0;
  if (xError == 0){
    stepTimeX = 999999;
  }else{
    stepTimeX = (xProportion / abs(xError)) + (dGainX / (xError - prevXError)); // microseconds
  }
  if (yError == 0){
    stepTimeY = 999999;
  }else{
    stepTimeY = yProportion / abs(yError) + (dGainY / (yError - prevYError)); // microseconds
  }  
  
  if (stepTimeX > 200000){
    stepTimeX = 200000;
  }else if(stepTimeX < 2000){
    stepTimeX = 2000;
  }
  if (stepTimeY > 200000){
    stepTimeY = 200000;
  }else if(stepTimeY < 10000){
    stepTimeY = 10000;
  }

  // give step direction commands
  unsigned long currentMicros = micros();
  if (currentMicros - previousMicrosX >= stepTimeX) {
    previousMicrosX = currentMicros;
    digitalWrite(xStepPin, HIGH);
    delayMicroseconds(3);
    digitalWrite(xStepPin, LOW);
    delayMicroseconds(3);
  }
  if (currentMicros - previousMicrosY >= stepTimeY) {
    previousMicrosY = currentMicros;
    digitalWrite(yStepPin, HIGH);
    delayMicroseconds(3);
    digitalWrite(yStepPin, LOW);
    delayMicroseconds(3);
  }  
}

void fire() {
  unsigned long currentMicros = micros();
  if (currentMicros - previousMicrosP >= 2000) {
  previousMicrosP = currentMicros;
    if (fireSteps < 3200) {
      digitalWrite(puncherStepPin, HIGH);
      delayMicroseconds(3);
      digitalWrite(puncherStepPin, LOW);
      delayMicroseconds(3);
      fireSteps++;
    }
    else{
      fireSteps = 0;
      fPin = 0;
      fireTriggered = 0;
      digitalWrite(puncherStepPin, fPin);
    }
  }
}

// Not the smoothest movement
// Increase speed slightly if possible
// Y axis struggles - add better counterbalance?
// Test fire()
// test at longer range with natural lighting

void loop() {
  if (Serial.available() > 0){
    parseCommands();
  }
  moveMotors();
  
  if (abs(xError) <= 10 && abs(yError) <= 10) {
    fireTriggered = 1;
  }
  if (fireTriggered == 1){
        fire();
  }
  
}
