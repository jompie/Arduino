// Inleveropdracht 2 PIT2
// 

#include <LiquidCrystal.h>
//Initialize the library with the numbers of the interface pins
LiquidCrystal lcd(11,10,5,4,3,2);

//Button pins
int deleteButton = 9;
int inputButton = 8;
int sendButton = 7;

//Communication Pins
int outputWire = 13;    //is used to send morse code to another device
int inputWire = 12;     //is used to receive morse code from another device

// Morse code timings
int dotDuration = 200;                // Duration of a dot in milliseconds
int dashDuration = dotDuration * 3;   // Duration of a dash in milliseconds
int signalGap = dotDuration;          // Duration of the gap between two signals (dots and dashes)
int letterPause = dotDuration * 3;    // Duration of the gap between two letters in milliseconds
int wordPause = dotDuration * 7;      // Duration of the gap between two words in milliseconds

//Signal indentifiers
int DOT = 1;
int DASH = 2;
int NONE = 0;

byte signalBuffer[5];     // Holds up to 5 recent signals(dots and/or dashes), is used to process input user input and received morse messages
int bufferIndex = 0;      // Index of signal buffer

boolean buttonWasPressed = false;   //remembers the most recent state of the inputbutton
boolean signalReceived = false;     //remembers the most recent state of the receive wire

String plainTextMessage = "";     // Holds the written/received message so it can be sent to another arduino


int screenCol = 0;      //Holds the columm position of the lcd cursor
int screenRow = 0;      //Holds the row position of the lcd cursor

//Time stamps used to time intervals, and to determine wheter a signal is a dot or a dash
long currentTimestamp = 0;    // Most recent timestamp
long lastTimestamp = 0;       // Second most recent timestamp
int duration = 0;             // The difference between the current time stamp and last time stamp


void setup() {
  pinMode(deleteButton, INPUT);
  pinMode(inputButton, INPUT);
  pinMode(sendButton, INPUT);
  pinMode(outputWire, OUTPUT);
  pinMode(inputWire, INPUT);

  lcd.begin(16,2);
  lcd.clear();
  lcd.print("hallo");
  writeToDisplay("Welkom", 0, 400);
  enterStandBy();

  Serial.begin(9600);

}

void loop() {
  currentTimestamp = millis();
  if (digitalRead(inputButton) == HIGH){
    delay(100);
    writeToDisplay("Writing Mode", 1, 0);
    delay(500);
    lastTimestamp = currentTimestamp;
    writeMorseMessage();
  }
  else if(digitalRead(inputWire) == HIGH && currentTimestamp > 5000){
    delay(100);
    digitalWrite(outputWire, HIGH);
    delay(50);
    digitalWrite(outputWire, LOW);
    writeToDisplay("Receiving Mode", 1, 0);
    lastTimestamp = currentTimestamp;
    receiveMessage();
  }

}

void writeMorseMessage(){
  resetBuffer();
  boolean messageSent = false;
  while(!messageSent){
    currentTimestamp = millis();
    duration = currentTimestamp - lastTimestamp;
    if (digitalRead(deleteButton) == HIGH){                   // The delete button is pressed
      delay(200);
      if(digitalRead(deleteButton) == LOW){
        plainTextMessage.remove(plainTextMessage.length()-1);   // remove last character of the message
        writeMessageToDisplay();
        lastTimestamp = currentTimestamp;       
      }
      else{
        messageSent = true;
      }               
    }
    else if(digitalRead(sendButton) == HIGH){                 //The send button is pressed
      delay(100);
      writeToDisplay("Sending...", 1, 0);
      messageSent = sendMessage();
    }
    else if (digitalRead(inputButton) == HIGH){               //The inputbutton is pressed
      if(!buttonWasPressed){
        lastTimestamp = currentTimestamp;
        buttonWasPressed = true;
        if (duration > wordPause && plainTextMessage.length() > 0){
          plainTextMessage += " ";
          writeMessageToDisplay(); 
        }
      }
    }
    else{
      if(buttonWasPressed && duration > signalGap){           //The input button was just released and the time between button presses is greater that the signal gap
        if(duration < dotDuration + signalGap){               
          signalBuffer[bufferIndex] = DOT;                    //If the input button was pressed shorter than the dotduration, add a dot to the buffer
          bufferIndex++;
        }
        else{
          signalBuffer[bufferIndex] = DASH;                   //If the input button was pressed longer than the dotduration, add a dash to the buffer
          bufferIndex++;
        }
        buttonWasPressed = false;
        lastTimestamp = currentTimestamp;
      }
      else{
        if(bufferIndex > 0){
          if(duration > letterPause || bufferIndex > 4){   //The input button has been released longer than the word pause duration or the buffer is full
            plainTextMessage += bufferToLetter();          //Convert buffer to a letter and add letter to plain text message
            writeMessageToDisplay();
            resetBuffer();
          }
        }
      }
    }
  }
  enterStandBy();
}

boolean sendMessage(){
  digitalWrite(outputWire, HIGH);
  delay(75);
  digitalWrite(outputWire, LOW);
  delay(50);
  if(digitalRead(inputWire) == HIGH){
    for(int i=0; i < plainTextMessage.length(); i++){
      letterToMorse(plainTextMessage[i]);
    }
    digitalWrite(outputWire, HIGH);
    delay(2000);
    digitalWrite(outputWire, LOW);
    plainTextMessage = "";
    writeToDisplay("Message sent", 1, 0);
    return true;
  }
  else{
    writeToDisplay("Failed to send", 1, 400);
    return false;
  }
}

void receiveMessage(){
  Serial.print("receiving");
  boolean messageReceived = false;
  while (!messageReceived){
    currentTimestamp = millis();
    duration = currentTimestamp - lastTimestamp;
    if (digitalRead(inputWire) == HIGH){
      if(!signalReceived){
        lastTimestamp = currentTimestamp;
        signalReceived = true;
        if(duration > wordPause){
          plainTextMessage += " ";
        }
      }
      else if (signalReceived){
        if(duration > 1500){
          writeToDisplay("Message received", 1, 400);
          delay(500);
          messageReceived = true;
          
        }
      }
    }
    else{
      if(signalReceived && duration > signalGap){
        if(duration < dotDuration + signalGap){
          signalBuffer[bufferIndex] = DOT;
          bufferIndex++;
        }
        else{
          signalBuffer[bufferIndex] = DASH;
          bufferIndex++;
        }
        signalReceived = false;
        lastTimestamp = currentTimestamp;
      }
      else{
        if(bufferIndex > 0){
          if (duration > letterPause || bufferIndex > 4){
            plainTextMessage += bufferToLetter();
            writeMessageToDisplay();
            Serial.println(plainTextMessage);
            resetBuffer();
          }
        }
      }
    }
    
  }
  enterStandBy();
}

// Writes the plain text message to the display
void writeMessageToDisplay(){
  clearLine(0);
  if (plainTextMessage.length() < 17){
    for (int i=0; i< plainTextMessage.length(); i++){
      lcd.setCursor(i,0);
      lcd.print(plainTextMessage[i]);
    }
  }
  if (plainTextMessage.length() > 16){
    for(int i = 16; i >0; i--){
      lcd.setCursor(16-i,0);
      lcd.print(plainTextMessage[plainTextMessage.length()-i]);
    }
  }
}


void writeToDisplay(String text, int col, int scrollSpeed){
  clearLine(col);
  lcd.print(text);
  if(text.length() > 16){
    for(int i = 0; i < text.length()-16; i++){
      delay(scrollSpeed);
      lcd.scrollDisplayLeft();
    }
  }
}

void enterStandBy(){
  delay(1000);
  lcd.clear();
  writeToDisplay("Standby Mode", 1, 0);
  plainTextMessage = "";
  resetBuffer();
  delay(1000);
}

// clears a line of the lcd display and places the cursor at the start of that line
void clearLine(int col){
  for (int i = 0; i < 16; i++){
    lcd.setCursor(i, col);
    lcd.print(' ');
  }
  lcd.setCursor(0,col);
}

void resetBuffer(){
  for(int i=0; i<5; i++){
    signalBuffer[i] = 0;
  }
  bufferIndex = 0;
}

// Convert input signal to a letter
char bufferToLetter(){
  if (matchInputBuffer(DOT, DASH, NONE, NONE, NONE))  { return 'A'; }
  if (matchInputBuffer(DASH, DOT, DOT, DOT, NONE))    { return 'B'; }
  if (matchInputBuffer(DASH, DOT, DASH, DOT, NONE))   { return 'C'; }
  if (matchInputBuffer(DASH, DOT, DOT, NONE, NONE))   { return 'D'; }
  if (matchInputBuffer(DOT, NONE, NONE, NONE, NONE))  { return 'E'; }
  if (matchInputBuffer(DOT, DOT, DASH, DOT, NONE))    { return 'F'; }
  if (matchInputBuffer(DASH, DASH, DOT, NONE, NONE))  { return 'G'; }
  if (matchInputBuffer(DOT, DOT, DOT, DOT, NONE))     { return 'H'; }
  if (matchInputBuffer(DOT, DOT, NONE, NONE, NONE))   { return 'I'; }
  if (matchInputBuffer(DOT, DASH, DASH, DASH, NONE))  { return 'J'; }
  if (matchInputBuffer(DASH, DOT, DASH, NONE, NONE))  { return 'K'; }
  if (matchInputBuffer(DOT, DASH, DOT, DOT, NONE))    { return 'L'; }
  if (matchInputBuffer(DASH, DASH, NONE, NONE, NONE)) { return 'M'; }
  if (matchInputBuffer(DASH, DOT, NONE, NONE, NONE))  { return 'N'; }
  if (matchInputBuffer(DASH, DASH, DASH, NONE, NONE)) { return 'O'; }
  if (matchInputBuffer(DOT, DASH, DASH, DOT, NONE))   { return 'P'; }
  if (matchInputBuffer(DASH, DASH, DOT, DASH, NONE))  { return 'Q'; }
  if (matchInputBuffer(DOT, DASH, DOT, NONE, NONE))   { return 'R'; }
  if (matchInputBuffer(DOT, DOT, DOT, NONE, NONE))    { return 'S'; }
  if (matchInputBuffer(DASH, NONE, NONE, NONE, NONE)) { return 'T'; }
  if (matchInputBuffer(DOT, DOT, DASH, NONE, NONE))   { return 'U'; }
  if (matchInputBuffer(DOT, DOT, DOT, DASH, NONE))    { return 'V'; }
  if (matchInputBuffer(DOT, DASH, DASH, NONE, NONE))  { return 'W'; }
  if (matchInputBuffer(DASH, DOT, DOT, DASH, NONE))   { return 'X'; }
  if (matchInputBuffer(DASH, DOT, DASH, DASH, NONE))  { return 'Y'; }
  if (matchInputBuffer(DASH, DASH, DOT, DOT, NONE))   { return 'Z'; }
  if (matchInputBuffer(DOT, DASH, DASH, DASH, DASH))  { return '1'; }
  if (matchInputBuffer(DOT, DOT, DASH, DASH, DASH))   { return '2'; }
  if (matchInputBuffer(DOT, DOT, DOT, DASH, DASH))    { return '3'; }
  if (matchInputBuffer(DOT, DOT, DOT, DOT, DASH))     { return '4'; }
  if (matchInputBuffer(DOT, DOT, DOT, DOT, DOT))      { return '5'; }
  if (matchInputBuffer(DASH, DOT, DOT, DOT, DOT))     { return '6'; }
  if (matchInputBuffer(DASH, DASH, DOT, DOT, DOT))    { return '7'; }
  if (matchInputBuffer(DASH, DASH, DASH, DOT, DOT))   { return '8'; }
  if (matchInputBuffer(DASH, DASH, DASH, DASH, DOT))  { return '9'; }
  if (matchInputBuffer(DASH, DASH, DASH, DASH, DASH)) { return '0'; }
  return '?';
}

// return true if all bytes match the bytes in the signal buffer
boolean matchInputBuffer(byte s0, byte s1, byte s2, byte s3, byte s4) {
  return ((signalBuffer[0] == s0) && 
          (signalBuffer[1] == s1) && 
          (signalBuffer[2] == s2) && 
          (signalBuffer[3] == s3) &&  
          (signalBuffer[4] == s4));
}

// convert a letter to morse code
void letterToMorse(char letter){
  switch(letter){
    case 'A': sendLetterInMorse(DOT, DASH, NONE, NONE, NONE); break;
    case 'B': sendLetterInMorse(DASH, DOT, DOT, DOT, NONE); break;
    case 'C': sendLetterInMorse(DASH, DOT, DASH, DOT, NONE); break;
    case 'D': sendLetterInMorse(DASH, DOT, DOT, NONE, NONE); break;
    case 'E': sendLetterInMorse(DOT, NONE, NONE, NONE, NONE); break;
    case 'F': sendLetterInMorse(DOT, DOT, DASH, DOT, NONE); break;
    case 'G': sendLetterInMorse(DASH, DASH, DOT, NONE, NONE); break;
    case 'H': sendLetterInMorse(DOT, DOT, DOT, DOT, NONE); break;
    case 'I': sendLetterInMorse(DOT, DOT, NONE, NONE, NONE); break;
    case 'J': sendLetterInMorse(DOT, DASH, DASH, DASH, NONE); break;
    case 'K': sendLetterInMorse(DASH, DOT, DASH, NONE, NONE); break;
    case 'L': sendLetterInMorse(DOT, DASH, DOT, DOT, NONE); break;
    case 'M': sendLetterInMorse(DASH, DASH, NONE, NONE, NONE); break;
    case 'N': sendLetterInMorse(DASH, DOT, NONE, NONE, NONE); break;
    case 'O': sendLetterInMorse(DASH, DASH, DASH, NONE, NONE); break;
    case 'P': sendLetterInMorse(DOT, DASH, DASH, DOT, NONE); break;
    case 'Q': sendLetterInMorse(DASH, DASH, DOT, DASH, NONE); break;
    case 'R': sendLetterInMorse(DOT, DASH, DOT, NONE, NONE); break;
    case 'S': sendLetterInMorse(DOT, DOT, DOT, NONE, NONE); break;
    case 'T': sendLetterInMorse(DASH, NONE, NONE, NONE, NONE); break;
    case 'U': sendLetterInMorse(DOT, DOT, DASH, NONE, NONE); break;
    case 'V': sendLetterInMorse(DOT, DOT, DOT, DASH, NONE); break;
    case 'W': sendLetterInMorse(DOT, DASH, DASH, NONE, NONE); break;
    case 'X': sendLetterInMorse(DASH, DOT, DOT, DASH, NONE); break;
    case 'Y': sendLetterInMorse(DASH, DOT, DASH, DASH, NONE); break;
    case 'Z': sendLetterInMorse(DASH, DASH, DOT, DOT, NONE); break;
    case '1': sendLetterInMorse(DOT, DASH, DASH, DASH, DASH); break;
    case '2': sendLetterInMorse(DOT, DOT, DASH, DASH, DASH); break;
    case '3': sendLetterInMorse(DOT, DOT, DOT, DASH, DASH); break;
    case '4': sendLetterInMorse(DOT, DOT, DOT, DOT, DASH); break;
    case '5': sendLetterInMorse(DOT, DOT, DOT, DOT, DOT); break;
    case '6': sendLetterInMorse(DASH, DOT, DOT, DOT, DOT); break;
    case '7': sendLetterInMorse(DASH, DASH, DOT, DOT, DOT); break;
    case '8': sendLetterInMorse(DASH, DASH, DASH, DOT, DOT); break;
    case '9': sendLetterInMorse(DASH, DASH, DASH, DASH, DOT); break;
    case '0': sendLetterInMorse(DASH, DASH, DASH, DASH, DASH); break;
    case ' ': delay(wordPause); break;
    default: 
      Serial.print("Don't understand [");
      Serial.print((char) letter);
      Serial.print("]");
  }
}

void sendLetterInMorse(byte s0, byte s1, byte s2, byte s3, byte s4){
  if (sendSignal(s0)){
    delay(signalGap * 0.8);
    if(sendSignal(s1)){
      delay(signalGap * 0.8);
      if(sendSignal(s2)){
        delay(signalGap * 0.8);
        if(sendSignal(s3)){
          delay(signalGap * 0.8);
          sendSignal(s4);
        }
      }
    }
  }
  delay(letterPause);
}

boolean sendSignal(byte morseSignal){
  switch(morseSignal){
    case 1: //in case of a dot
      digitalWrite(outputWire, HIGH);
      delay(dotDuration);
      digitalWrite(outputWire, LOW);
      return true;
    case 2: //in case of a dash
      digitalWrite(outputWire, HIGH);
      delay(dashDuration);
      digitalWrite(outputWire, LOW);
      return true;
    default:
      return false;
  }
}
