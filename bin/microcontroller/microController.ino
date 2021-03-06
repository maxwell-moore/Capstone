#include <math.h>

#define X1_STEP_PIN          54 //X1->X
#define X1_DIR_PIN           55
#define X1_ENABLE_PIN        38
 
#define X2_STEP_PIN          46 //X2->Z
#define X2_DIR_PIN           48
#define X2_ENABLE_PIN        62

#define Y_STEP_PIN           60 //Y->Y
#define Y_DIR_PIN            61
#define Y_ENABLE_PIN         56

#define R_STEP_PIN           36 //R->E1
#define R_DIR_PIN            34
#define R_ENABLE_PIN         30

#define E_STEP_PIN           26 //E->E0
#define E_DIR_PIN            28
#define E_ENABLE_PIN         24

#define XMIN_PIN             16 //X min endstop 
#define XMAX_PIN             17 //X max endstop 
#define YMIN_PIN             23 //Y min endstop 
#define YMAX_PIN             25 //X max endstop 
#define RMIN_PIN             27 //R sensor 1 
#define RMAX_PIN             29 //R sensor 2

#define E_EXTEND_PIN         31 //End effector shoot
#define E_RETRACT_PIN        33 //End effector retract

#define USERBUTTON1_PIN      20 //move left button
#define USERBUTTON2_PIN      21 //move right button  
#define USERBUTTON3_PIN      35 //user control button

#define RED_LED_PIN          37 //multi-colour led -> red
#define BLUE_LED_PIN         41 //multi-colour led -> blue
#define GREEN_LED_PIN        43 //multi-colour led -> green

#define UPPERBOUND_X         13389 //Max step for for full range of motion in X
#define UPPERBOUND_Y         16921 //Max step for full range of motion in Y
#define UPPERBOUND_R         1599  //Max step for full rotation in R

#define REAL_TABLE_X         1.848        //Length of pool table in meters
#define REAL_TABLE_Y         0.921        //Width of pool table in meters
#define REAL_TABLE_R         6.283185307  //Range of rotational motion in radians

#define OOB_MOD_LENGTH       0.16   //Out of bound mod length for when ball is near edge of table   

#define X_MIN_OFFSET         950  //Steps from min X end stop to edge of pool table
#define X_MAX_OFFSET         885  //Steps from max X end stop to edge of pool table
#define Y_MIN_OFFSET         2600 //Steps from min Y end stop to edge of pool table
#define Y_MAX_OFFSET         2785 //Steps from max Y end stop to edge of pool table
#define R_OFFSET             242  //Steps from R initialization point to zero angle

#define MICRODELAY           800  //Pause between stepper motor steps
#define SLOWSTEP             300  //300 is normal slow, 0 is off 
#define SLOWRATE             2    //2 is normal slow, 1 is off  

#define REQUEST_CODE         55
#define CONFIRM_CODE         200
#define SHOT_CODE            170

int requestedX, requestedY, requestedR, requestedE,
    currentX, currentY, currentR,
    xDir, yDir, rDir,
    previousTime = millis(),
    isInitialized = 0;      //Has the system finished initialization?

bool userControl = false;   //Enable or disable user turn
bool userOneHeld = false;   //True while user button one is held down
bool userTwoHeld = false;   //True while user button two is held down
bool userThreeHeld = false; //True while user button three is held down
bool stopStart = false;     //Move left pressed while moving right, or vice versa

void setup(){
  Serial.begin(9600);
  while(!Serial){
    delay(10);
  }
  
  pinMode(X1_STEP_PIN, OUTPUT);  
  pinMode(X1_DIR_PIN, OUTPUT); 
  pinMode(X1_ENABLE_PIN, OUTPUT); 
  pinMode(X2_STEP_PIN, OUTPUT);  
  pinMode(X2_DIR_PIN, OUTPUT); 
  pinMode(X2_ENABLE_PIN, OUTPUT); 
  pinMode(Y_STEP_PIN, OUTPUT);  
  pinMode(Y_DIR_PIN, OUTPUT); 
  pinMode(Y_ENABLE_PIN, OUTPUT); 
  pinMode(R_STEP_PIN, OUTPUT);  
  pinMode(R_DIR_PIN, OUTPUT); 
  pinMode(R_ENABLE_PIN, OUTPUT); 
  pinMode(E_STEP_PIN, OUTPUT);  
  pinMode(E_DIR_PIN, OUTPUT); 
  pinMode(E_ENABLE_PIN, OUTPUT); 
  pinMode(E_EXTEND_PIN, OUTPUT);
  pinMode(E_RETRACT_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);  
  pinMode(BLUE_LED_PIN, OUTPUT); 
  pinMode(GREEN_LED_PIN, OUTPUT); 

  pinMode(XMIN_PIN, INPUT);
  pinMode(XMAX_PIN, INPUT);
  pinMode(YMIN_PIN, INPUT);
  pinMode(YMAX_PIN, INPUT);
  pinMode(RMIN_PIN, INPUT);
  pinMode(RMAX_PIN, INPUT);
  pinMode(USERBUTTON1_PIN, INPUT);
  pinMode(USERBUTTON2_PIN, INPUT);
  pinMode(USERBUTTON3_PIN, INPUT);

  digitalWrite(RED_LED_PIN, HIGH);  
  digitalWrite(BLUE_LED_PIN, LOW); 
  digitalWrite(GREEN_LED_PIN, LOW); 

  while(digitalRead(USERBUTTON3_PIN) != 0){   //Wait for button press
    delay(10);
  }
  userThreeHeld = true;
  
  requestedX = 0;
  requestedY = 0;
  requestedR = -1*UPPERBOUND_R;
  
  currentX = UPPERBOUND_X;
  currentY = UPPERBOUND_Y; 
  currentR = 0;
  
  moveXYR();
  currentR = 0;
  isInitialized = 2;
}//setup()

void loop(){
  allowUserTurn();
  if(!userControl){ poseForPicture(); }
  if(!userControl){ requestPCInstruction(); }
  if(!userControl){ takeShot(); }
}//loop()

void allowUserTurn(){ //Loop during user turn
  digitalWrite(RED_LED_PIN, LOW);  
  digitalWrite(BLUE_LED_PIN, LOW); 
  digitalWrite(GREEN_LED_PIN, HIGH); 

  userControl = true;
  while(userControl){
    moveXYR();
    delay(10); 
    
    if(digitalRead(USERBUTTON1_PIN) == 0){
      userButton1();
    }else{
      userOneHeld = false;
    }
    
    if(digitalRead(USERBUTTON2_PIN) == 0){
      userButton2(); 
    }else{
      userTwoHeld = false;
    }
    
    if(digitalRead(USERBUTTON3_PIN) == 0){
      userButton3();
    }else{
      userThreeHeld = false;
    }
  }
}//allowUserTurn()

void poseForPicture(){ //Move machine so camera can see unobstructed table
  digitalWrite(RED_LED_PIN, LOW);  
  digitalWrite(BLUE_LED_PIN, HIGH); 
  digitalWrite(GREEN_LED_PIN, LOW); 

  delay(250);
  if(currentX < UPPERBOUND_X / 2){
    requestedX = 0;
  }else{
    requestedX = UPPERBOUND_X;
  }
  
  if(currentY < UPPERBOUND_Y / 2){
    requestedY = 0;
  }else{
    requestedY = UPPERBOUND_Y;
  }
  
  requestedR = 0;
  moveXYR();  
}//poseForPicture()

void requestPCInstruction(){ //Loop during PC operation
  double serialX;
  double serialY;
  double serialR;

  digitalWrite(RED_LED_PIN, LOW);  
  digitalWrite(BLUE_LED_PIN, HIGH); 
  digitalWrite(GREEN_LED_PIN, LOW); 

  while(true){ 
    Serial.println(REQUEST_CODE);

    while(Serial.available() == 0){
      if(digitalRead(USERBUTTON3_PIN) == 0){
        userButton3(); 
      }else{
        userThreeHeld = false;
      }
      
      if(userControl){
        return;
      }
    }

    if((int)Serial.parseFloat() != SHOT_CODE){
      Serial.println("Communication fault!");
    }else{
      break;
    }
  }

  while(Serial.available() == 0);
  serialX = Serial.parseFloat();
  
  while(Serial.available() == 0);
  serialY = Serial.parseFloat();
  
  while(Serial.available() == 0);
  serialR = Serial.parseFloat();
  
  while(Serial.available() == 0);
  requestedE = Serial.parseFloat();
   
  Serial.println(CONFIRM_CODE);

  mapCoordinates(serialX, serialY, serialR);
}//requestPCInstruction()

void mapCoordinates(double serialX, double serialY, double serialR){ //Map from real coordinates to step coordinates
  serialX -= OOB_MOD_LENGTH*cos(serialR);
  serialY -= OOB_MOD_LENGTH*sin(serialR);
  
  requestedX = (serialX * (UPPERBOUND_X - X_MIN_OFFSET - X_MAX_OFFSET) / REAL_TABLE_X) + X_MIN_OFFSET; 
  requestedY = (serialY * (UPPERBOUND_Y - Y_MIN_OFFSET - Y_MAX_OFFSET) / REAL_TABLE_Y) + Y_MIN_OFFSET;
  requestedR = (int)((REAL_TABLE_R - serialR) * UPPERBOUND_R / REAL_TABLE_R + R_OFFSET) % (UPPERBOUND_R + 1);

  if(requestedX < 0){
    requestedX = 0;
  }else if(requestedX > UPPERBOUND_X){
    requestedX = UPPERBOUND_X;
  }
  
  if(requestedY < 0){
    requestedY = 0;
  }else if(requestedY > UPPERBOUND_Y){
    requestedY = UPPERBOUND_Y;
  }
  
  if(requestedR < 0){
    requestedR = 0;
  }else if(requestedR > UPPERBOUND_R){
    requestedR = UPPERBOUND_R;
  }
}//mapCoordinates()

void takeShot(){
  digitalWrite(RED_LED_PIN, HIGH);  
  digitalWrite(BLUE_LED_PIN, HIGH); 
  digitalWrite(GREEN_LED_PIN, HIGH); 
  
  moveXYR();
  delay(500);
  digitalWrite(E_EXTEND_PIN, HIGH);
  delay(25);
  digitalWrite(E_EXTEND_PIN, LOW);
  delay(200);
  digitalWrite(E_RETRACT_PIN, HIGH);
  delay(25);
  digitalWrite(E_RETRACT_PIN, LOW);
}//takeShot()

void moveXYR(){ //Move X Y and R simultaneously
  int speedUp;
  
  if(requestedX-currentX != 0 || requestedY-currentY != 0 || requestedR-currentR != 0){
    speedUp = 0;
      
    if(requestedX > currentX){
      xDir = 1;
    }else{
      xDir = -1;
    }

    digitalWrite(X1_DIR_PIN, xDir > 0);
    digitalWrite(X2_DIR_PIN, xDir < 0);

    if(requestedY > currentY){
      yDir = 1;
    }else{
      yDir = -1;
    }
  
    digitalWrite(Y_DIR_PIN, yDir > 0);

    if(requestedR > currentR){
      rDir = 1;
    }else{
      rDir = -1;
    }
    
    digitalWrite(R_DIR_PIN, rDir > 0);

    while(requestedX-currentX != 0
          || requestedY-currentY != 0
          || requestedR-currentR != 0){
      if(xDir == -1 && digitalRead(XMIN_PIN) == 0){ //XMIN endstop hit 
        currentX = 0;
        xDir = 1;
        digitalWrite(X1_DIR_PIN, HIGH);
        digitalWrite(X2_DIR_PIN, LOW); 
        delay(250); 
      }

      if(xDir == 1 && digitalRead(XMAX_PIN) == 0){ //XMAX endstop hit 
        currentX = UPPERBOUND_X;
        xDir = -1;
        digitalWrite(X1_DIR_PIN, LOW);
        digitalWrite(X2_DIR_PIN, HIGH);  
        delay(250); 
      }

      if(yDir == -1 && digitalRead(YMIN_PIN) == 0){ //YMIN endstop hit 
        currentY = 0;
        yDir = 1;
        digitalWrite(Y_DIR_PIN, HIGH);
      }
    
      if(yDir == 1 && digitalRead(YMAX_PIN) == 0){ //YMAX endstop hit
        currentY = UPPERBOUND_Y;
        yDir = -1;
        digitalWrite(Y_DIR_PIN, LOW);
      }

      if(rDir == -1 && currentR < UPPERBOUND_R/2 && digitalRead(RMIN_PIN) == 0){ //R reference point hit
          currentR = 0;
          if(requestedR < 0) requestedR = 0; //For initialization 
          rDir = 1;
          digitalWrite(R_DIR_PIN, HIGH);
      }
         
      if(requestedX-currentX != 0){
        digitalWrite(X1_STEP_PIN, HIGH);
        digitalWrite(X2_STEP_PIN, HIGH);
      }

      if(requestedY-currentY != 0){
        digitalWrite(Y_STEP_PIN, HIGH);
      }

      if(requestedR-currentR != 0){
        digitalWrite(R_STEP_PIN, HIGH);
      }

      if(abs(currentX-requestedX) < SLOWSTEP && currentX-requestedX != 0){
        delayMicroseconds(MICRODELAY*(abs(currentX-requestedX)*((1.0-2.0)/SLOWSTEP)+SLOWRATE));
      }else if(speedUp < SLOWSTEP){
        delayMicroseconds(MICRODELAY*(speedUp*((1.0-2.0)/SLOWSTEP)+SLOWRATE));
      }else{
        delayMicroseconds(MICRODELAY);
      }

      if(requestedX-currentX != 0){
        digitalWrite(X1_STEP_PIN, LOW);
        digitalWrite(X2_STEP_PIN, LOW);
        currentX += xDir;
      }

      if(requestedY-currentY != 0){
        digitalWrite(Y_STEP_PIN, LOW);
        currentY += yDir;
      }

      if(requestedR-currentR != 0){
        digitalWrite(R_STEP_PIN, LOW);
        currentR += rDir;
      }
    
      if(abs(currentX-requestedX) < SLOWSTEP && currentX-requestedX != 0){
        delayMicroseconds(MICRODELAY*(abs(currentX-requestedX)*((1.0-2.0)/SLOWSTEP)+SLOWRATE));
      }else if(speedUp < SLOWSTEP){
        delayMicroseconds(MICRODELAY*(speedUp*((1.0-2.0)/SLOWSTEP)+SLOWRATE));
      }else{
        delayMicroseconds(MICRODELAY);
      }

      if(digitalRead(USERBUTTON1_PIN) == 0){
        userButton1();
      }else{
        userOneHeld = false;
      }
      
      if(digitalRead(USERBUTTON2_PIN) == 0){
        userButton2(); 
      }else{
        userTwoHeld = false;
      }
      
      if(digitalRead(USERBUTTON3_PIN) == 0){
        userButton3();
      }else{
        userThreeHeld = false;
      }

      if(speedUp < SLOWSTEP){
        speedUp++;
      }    
    }

    if(stopStart){
      if(xDir > 0){
        requestedX = 0;
      }else{
        requestedX = UPPERBOUND_X; 
      }
      stopStart = false;
      delay(250);
    }
  }
}//moveXYR()

void userButton1(){ //Tell machine to move left during user turn
  int currentTime;
  
  if(!userControl || userOneHeld){
    return;
  }
  
  userOneHeld = true;
  currentTime = millis();
    
  if(abs(currentTime - previousTime) <= 250){
    return;
  }
     
  previousTime = currentTime;
      
  if(requestedX != currentX) {
    if(abs(requestedX-currentX) >= 300){   //Stop Moving
      requestedX = currentX + (xDir*300);
    }
    
    if(requestedX > currentX){             //Change directions, must stop and start
      stopStart = true;
    }
  }else{
    requestedX = 0;
  }
}//userButton1()

void userButton2(){ //Tell machine to move right during user turn
  int currentTime;
  
  if(!userControl || userTwoHeld){
    return;
  }
   
  userTwoHeld = true;
  currentTime = millis();
  
  if(abs(currentTime - previousTime) <= 250){
    return;
  }
      
  previousTime = currentTime;

  if(requestedX != currentX){
    if(abs(requestedX-currentX) >= 300){   //Stop Moving
      requestedX = currentX + (xDir*300);
    }
    
    if(requestedX < currentX){             //Change directions, must stop and start
      stopStart = true;
    }
  }else{
    requestedX = UPPERBOUND_X;
  }
}//userButton2()

void userButton3(){ //User turn, machine turn
  int currentTime;
  
  if(userThreeHeld || isInitialized == 1){
    return;
  }else if(isInitialized == 0){
    isInitialized = 1;
  }
  
  userThreeHeld = true;
  currentTime = millis();
  
  if(abs(currentTime - previousTime) <= 250){
    return;
  }
  
  previousTime = currentTime;
  userControl = !userControl;
  
  if(abs(requestedX - currentX) >= 300){
    requestedX = currentX + 300 * xDir;
  }
  
  requestedY = currentY;
  requestedR = currentR;
}//userButton3()
