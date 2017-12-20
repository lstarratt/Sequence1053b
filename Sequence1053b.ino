#include <SoftwareSerial.h>

//Define constants
#define baud 9600
#define charToInt 48
#define NumberOfChannels 1

#define MIDI_NOTE_ON  0x90
#define MIDI_NOTE_OFF 0x80
#define MIDI_CHAN_MSG 0xB0
#define MIDI_CHAN_BANK 0x00
#define MIDI_CHAN_VOLUME 0x07
#define MIDI_CHAN_PROGRAM 0xC0

#define VS1053_RX 13
#define VS1053_RESET 12
#define VS1053_POWER 11

SoftwareSerial VS1053_MIDI(0, VS1053_RX); // TX only, do not use the 'rx' side
// on a Mega/Leonardo you may have to change the pin to one that 
// software serial support uses OR use a hardware serial port!

//Pattern Struct Definitions
struct ch{
  byte chOn = 1;
  byte bank = 120;
  byte volume = 127;
  byte instrument  = 3;
  byte note[16] = {35,44,44,44,35,44,44,44,35,44,44,44,35,44,44,44};
  byte noteOn[16] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
  byte noteOnCount[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  byte duration[16] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
  byte velocity[16]  = {88,100,100,100,100,100,88,100,100,100,77,100,100,100,100,100};
};


struct CH{
  ch CH[NumberOfChannels];  
};


struct Song{
  byte tempo = 120;
  float beatLength = 0.5;
};


struct Select{
  char choice;
};




//Function definitions
void playPattern();
void stopPattern();
void clearPattern(CH &chGrp);
void play(const CH &chGrp,const float &beat);
void menu(Select &select);  
void editTempo(Song &song);
void editChannel(CH &chGrp);
byte editOn(byte on, byte channel);
byte editBank(byte bank, byte channel);
byte editInstrument(int instrument, byte channel);
void editPattern(CH &chGrp);
void displayPattern(const ch &Ch);
byte editNoteOn();
byte editVelocity();
byte editVolume();
byte editDuration();
void displayChannel(const CH &chGrp, byte channel);
void displayAllChannel(const CH &chGrp);
byte noteToNumber();
void numberToNote(byte number);
void setBank(byte bank, byte chan);
void setInstrument(byte chan, byte inst);
void setVolume(byte chan, byte vol);




//initialize Arduino settings
void setup() {
  Serial.begin(baud);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(VS1053_RESET, OUTPUT);
  pinMode(VS1053_POWER, OUTPUT);
  pinMode(8, OUTPUT);     //5V Power for
  digitalWrite(8, HIGH);  //  interrupt1
  pinMode(9, OUTPUT);     //5VPower for
  digitalWrite(9, HIGH);  //  interrupt2
  attachInterrupt(digitalPinToInterrupt(2), playPattern, RISING);
  attachInterrupt(digitalPinToInterrupt(3), stopPattern, RISING);

  VS1053_MIDI.begin(31250);
  delay(20);
  digitalWrite(VS1053_POWER, HIGH);
  delay(50);
  digitalWrite(VS1053_RESET, LOW);
  delay(20);
  digitalWrite(VS1053_RESET, HIGH);
  delay(50);
}




//Set Global for play interrupt
volatile byte PLAY = 0;
volatile byte CLEAR = 0;



//Main
void loop() {

  Song song;
  CH chGrp;

  while(1){
    while(PLAY == 1 && CLEAR == 0){
      Serial.println("PLAY");
      play(chGrp, song.beatLength);
      //delay(1000);
      }//end PLAY pattern
    
    //Edit pattern loop
    while(PLAY != 1){
  
      //Clear pattern PLAY noteOn and noteOnCount info
      if(CLEAR == 1){clearPattern(chGrp);}
  
      //Dispaly edit menu
      else if(PLAY != 1){
      
        Select select;

        Serial.print("Tempo ");
        Serial.print(song.tempo);
        Serial.println(" bpm \n");
        
        menu(select);
  
        if(select.choice == 'T' || select.choice == 't'){editTempo(song);}
        if(select.choice == 'D' || select.choice == 'd'){displayPattern(chGrp);}
        if(select.choice == 'C' || select.choice == 'c'){editChannel(chGrp);}
        if(select.choice == 'P' || select.choice == 'p'){editPattern(chGrp);}
        if(select.choice == 'K' || select.choice == 'k'){PLAY = 1;}
  
      }//end if not PLAY
            
    }//end edit pattern
  }//end program loop
}//end Main




//Menu option functions
void menu(Select &select){

  String input;

  Serial.println("Enter");
  Serial.println("T to edit tempo");
  Serial.println("D to display pattern");
  Serial.println("P to edit pattern");  
  Serial.println("C to edit channel");
  Serial.println("K to PLAY");
  Serial.print("\n");
  
  while((PLAY == 0 && CLEAR == 0) && !Serial.available()){}
  input = Serial.readString();
  select.choice = input[0];
  
}


//edit tempo Function
void editTempo(Song &song){

  Serial.print("tempo is ");
  Serial.print(song.tempo);
  Serial.print('\n');

  String tempoIn;

  while(PLAY == 0){    //get new input

    //song.tempo = 0;
    Serial.println("enter tempo between 30-250");

    while((PLAY == 0 && CLEAR == 0) && !Serial.available()){} //Wait for new input
    tempoIn = Serial.readString();              //read input
    Serial.print("tempoIn = ");
    Serial.println(tempoIn);
    
    //if not a number try again
    for (byte i = 0; i<3; i++){
      if (tempoIn[i] == '\n'){break;}
      else if ((tempoIn[i]-charToInt) < 0 || (tempoIn[i]-charToInt) > 9){
        Serial.println("try again");
        return;  
      }
    }
    
    if (tempoIn.length() >= 5){                 //if input is > 999 get new input
      Serial.println("to fast");
    }

    else if (tempoIn.length() == 1){            //if no input try again
      Serial.println("try again");
    }

    else if(tempoIn.length() == 2){   //Tempo is single digit
      Serial.println("to slow");    
    }

    else if(tempoIn.length() == 3){   //Tempo is double digit
      song.tempo = (tempoIn[0]-charToInt)*10 + (tempoIn[1]-charToInt);     
    }

    else if((tempoIn.length() == 4) &&    //set Tempo value
          ((tempoIn[0]-charToInt)*100 + (tempoIn[1]-charToInt)*10 + (tempoIn[2]-charToInt)<250)){   //Tempo is triple digit
      song.tempo = (tempoIn[0]-charToInt)*100 + (tempoIn[1]-charToInt)*10 + (tempoIn[2]-charToInt);
      Serial.println(song.tempo);     
    }
    
    if (song.tempo < 30){           //check if tempo is too slow
      ("to slow");  
    }
    
    else if (song.tempo >=30 && song.tempo <= 250){    //set if tempo is valid
      song.beatLength = 60.0/song.tempo;
      Serial.print("tempo is ");
      Serial.println(song.tempo);
      Serial.print('\n');    
      return; 
    }
    
  }//end while to get input

  return;
  
}//end editTempo


//edit channel
void editChannel(CH &chGrp){

  String input;
  byte channel = 0;
    
  //get channel to edit  
  Serial.print("Select channel 1-");
  Serial.print(NumberOfChannels);     
  while((PLAY == 0 && CLEAR == 0) && !Serial.available()){} //Wait for new input      
  input = Serial.readString();
  channel = input[0] - charToInt;
  Serial.print("selection is ");
  Serial.println(channel);
  Serial.print("\n");
  if(channel < 1 || channel > NumberOfChannels){return;}

   
  Serial.println("O - channel on");
  Serial.println("B - channel bank");
  Serial.println("I - channel instrument");
  Serial.println("V - channel volume");

  while((PLAY == 0 && CLEAR == 0) && !Serial.available()){} //Wait for new input
  input = Serial.readString();
  if(input[0] == 'O' || input[0] == 'o'){
    chGrp.CH[channel-1].chOn=editOn(chGrp.CH[channel-1].chOn, channel);
  }
  else if(input[0] == 'B' || input[0] == 'b'){
    chGrp.CH[channel-1].bank=editBank(chGrp.CH[channel-1].bank, channel);
  }
  else if(input[0] == 'I' || input[0] == 'i'){
    chGrp.CH[channel-1].instrument=editInstrument(chGrp.CH[channel-1].instrument, channel);
  }
  else if(input[0] == 'V' || input[0] == 'v'){
    chGrp.CH[channel-1].volume=editVolume();
  }
  else return;
  
}//end edit channel


//edit channel On
byte editOn(byte on, byte channel){
  
 String input;

 if(on == 0){
  Serial.print("\nChannel "); 
  Serial.print(channel);
  Serial.println(" is OFF; Y to turn on");
  while((PLAY == 0 && CLEAR == 0) && !Serial.available()){}
  input = Serial.readString();
  if(input[0] == 'Y' || input[0] == 'y'){
    Serial.println("Channel ON \n");
    return 1;
  } 
 }

 else if(on == 1){
  Serial.print("\nChannel "); 
  Serial.print(channel);
  Serial.println(" is ON; N to turn off");
  while((PLAY == 0 && CLEAR == 0) && !Serial.available()){}
  input = Serial.readString();
  if(input[0] == 'N' || input[0] == 'n'){
    Serial.println("Channel OFF \n");
    return 0;
  } 
 }
 
}//end edit Channel On

byte editVelocity(){

  String input;
  int velocity;
  while(PLAY == 0 && CLEAR == 0){ 
    
    Serial.println("Enter a velocity 1-127 ");
    while((PLAY == 0 && CLEAR == 0) && !Serial.available()){}
    input = Serial.readString();
  
    if(input.length() == 2){
      velocity = input[0] - charToInt;
    }
    else if(input.length() == 3){
      velocity = (((input[0]-charToInt)*10) + (input[1]-charToInt));     
    }
    else if(input.length() == 4){
      velocity = (((input[0]-charToInt)*100) + ((input[1]-charToInt)*10) + input[2]-charToInt);
    }
     
    if(velocity >= 0 && velocity <= 127){  
      Serial.print("New velocity - ");
      Serial.println(velocity);
      Serial.print("\n");
      return velocity;
    }
    
  }//end while 
}//end edit velocity


byte editVolume(){

  String input;
  int volume;
  while(PLAY == 0 && CLEAR == 0){ 
    
    Serial.println("Enter a volume 1-127 ");
    while((PLAY == 0 && CLEAR == 0) && !Serial.available()){}
    input = Serial.readString();
  
    if(input.length() == 2){
      volume = input[0] - charToInt;
    }
    else if(input.length() == 3){
      volume = (((input[0]-charToInt)*10) + (input[1]-charToInt));     
    }
    else if(input.length() == 4){
      volume = (((input[0]-charToInt)*100) + ((input[1]-charToInt)*10) + input[2]-charToInt);
    }
     
    if(volume >= 0 && volume <= 127){  
      Serial.print("volume - ");
      Serial.println(volume);
      Serial.print("\n");
      return volume;
    }
    
  }//end while 
}//end edit velocity

//edit duration
byte editDuration(){

  String input;
  byte duration;
  while(PLAY == 0 && CLEAR == 0){ 
    
    Serial.println("Enter a duration 1-32 ");
    while((PLAY == 0 && CLEAR == 0) && !Serial.available()){}
    input = Serial.readString();
  
    if(input.length() == 2){
      duration = input[0] - charToInt;
    }
    else if(input.length() == 3){
      duration = (((input[0]-charToInt)*10) + (input[1]-charToInt));     
    }
     
    if(duration >= 1 && duration <= 32){  
      Serial.print("duration - ");
      Serial.println(duration);
      Serial.print("\n");
      return duration;
    }
    
  }//end while 
}//end edit duration


//edit channel Bank
byte editBank(byte bank, byte channel){
  
  String input;

  Serial.print("\nChannel ");
  Serial.println(channel);
  Serial.println("Enter:");
  Serial.println("0 - Default");
  Serial.println("1 - Drum1");
  Serial.println("2 - Drum2");
  Serial.println("3 - Melody\n");

  if(bank == 0){
    Serial.println("current - Default");
    while((PLAY == 0 && CLEAR == 0) && !Serial.available()){}
    input = Serial.readString();
    if(input[0] == '0'){
      Serial.println("Bank - Default \n");
      return 0;
    }
    if(input[0] == '1'){
      Serial.println("Bank - Drum1 \n");
      return 120;
    }
    if(input[0] == '2'){
      Serial.println("Bank - Drum2 \n");
      return 121;
    }
    if(input[0] == '3'){
      Serial.println("Bank - Melodic \n");
      return 127;
    }
  }

  if(bank == 120){
    Serial.println("current - Drum1");
    while((PLAY == 0 && CLEAR == 0) && !Serial.available()){}
    input = Serial.readString();
    if(input[0] == '0'){
      Serial.println("Bank - Default \n");
      return 0;
    }
    if(input[0] == '1'){
      Serial.println("Bank - Drum1 \n");
      return 120;
    }
    if(input[0] == '2'){
      Serial.println("Bank - Drum2 \n");
      return 121;
    }
    if(input[0] == '3'){
      Serial.println("Bank - Melodic \n");
      return 127;
    }
  }

    if(bank == 121){
    Serial.println("current - Drum2");
    while((PLAY == 0 && CLEAR == 0) && !Serial.available()){}
    input = Serial.readString();
    if(input[0] == '0'){
      Serial.println("Bank - Default \n");
      return 0;
    }
    if(input[0] == '1'){
      Serial.println("Bank - Drum1 \n");
      return 120;
    }
    if(input[0] == '2'){
      Serial.println("Bank - Drum2 \n");
      return 121;
    }
    if(input[0] == '3'){
      Serial.println("Bank - Melodic \n");
      return 127;
    }
  }

  if(bank == 127){
    Serial.println("current - Melodic");
    while((PLAY == 0 && CLEAR == 0) && !Serial.available()){}
    input = Serial.readString();
    if(input[0] == '0'){
      Serial.println("Bank - Default \n");
      return 0;
    }
    if(input[0] == '1'){
      Serial.println("Bank - Drum1 \n");
      return 120;
    }
    if(input[0] == '2'){
      Serial.println("Bank - Drum2 \n");
      return 121;
    }
    if(input[0] == '3'){
      Serial.println("Bank - Melodic \n");
      return 127;
    }
  }
}//end edit channel Bank


//edit instrument
byte editInstrument(int instrument, byte channel){

  while(PLAY == 0 && CLEAR == 0){
    
    Serial.print("\nChannel ");
    Serial.print(channel);
    Serial.print(" instrument is ");
    Serial.println(instrument);
    Serial.println("Enter 1 - 128");
  
    while((PLAY == 0 && CLEAR == 0) && !Serial.available()){}
    String input = Serial.readString();
  
    if(input.length() == 2){
      instrument = (input[0] - charToInt)-1;
    }
    else if(input.length() == 3){
      instrument = (((input[0]-charToInt)*10) + (input[1]-charToInt))-1;     
    }
    else if(input.length() == 4){
      instrument = (((input[0]-charToInt)*100) + ((input[1]-charToInt)*10) + input[2]-charToInt)-1;
    }
     
    if(instrument >= 0 && instrument <= 127){  
      Serial.print("New instrument - ");
      Serial.println(instrument+1);
      Serial.print("\n");
      return instrument;
    }
  
  }//end while  
}//end edit instrument


void editPattern(CH &chGrp){
  
  byte channel;
  byte notePosition;
  
  Serial.print("select 1 through ");
  Serial.print(NumberOfChannels);

  while((PLAY == 0 && CLEAR == 0) && !Serial.available()){}
  channel = Serial.read() - charToInt;
  Serial.readString();
  if(channel >= 1 && channel <= NumberOfChannels){
    Serial.print("Channel ");
    Serial.print(channel);
    Serial.println(" selected");
    Serial.println("Select 1-16");
    while((PLAY == 0 && CLEAR == 0) && !Serial.available()){}
    String input = Serial.readString();
    if(input[1] == '\n'){notePosition = (input[0] - charToInt);}
    else(notePosition = (input[0]*10) + input[1]);
  }
  
  if(notePosition >= 1 && notePosition <= 16){
    Serial.print("Edit ");
    Serial.println(notePosition);
    Serial.println("N - Note");
    Serial.println("O - On");
    Serial.println("V - Velocity");
    Serial.println("D - Duration");
    Serial.println("A - All");
    while((PLAY == 0 && CLEAR == 0) && !Serial.available()){}
    char input = Serial.read();
    Serial.readString();

    if(input == 'N' || input == 'n'){
      chGrp.CH[channel-1].note[notePosition-1] = noteToNumber();
    }
    else if(input == 'O' || input == 'o'){
      chGrp.CH[channel-1].noteOn[notePosition-1] = editNoteOn();
    }
    else if(input == 'V' || input == 'v'){
      chGrp.CH[channel-1].velocity[notePosition-1] = editVelocity();
    }
    else if(input == 'D' || input == 'd'){
      chGrp.CH[channel-1].duration[notePosition-1] = editDuration();
    }
    else if(input == 'A' || input == 'a'){
    chGrp.CH[channel-1].note[notePosition-1] = noteToNumber();
    chGrp.CH[channel-1].noteOn[notePosition-1] = editNoteOn();
    chGrp.CH[channel-1].velocity[notePosition-1] = editVelocity();
    chGrp.CH[channel-1].duration[notePosition-1] = editDuration();
    }
    displayAllChannel(chGrp);
     
  } 
}//end edit Pattern


//edit noteOn
byte editNoteOn(){

  while(PLAY == 0 && CLEAR == 0){
    
    Serial.println("Y for note on: N for off");
    while((PLAY == 0 && CLEAR == 0) && !Serial.available()){}
    char input = Serial.read();
    Serial.readString();
    if (input == 'Y' || input == 'y'){
      return 1;
    }
    else if (input == 'N' || input == 'n'){
      return 0;
    }
    
  }//end while 
}//end editNoteOn


//select which pattern to display
void displayPattern(const CH &chGrp){

  String input;

  while (PLAY == 0 && CLEAR == 0){
    
    Serial.print("Enter 1-");
    Serial.print(NumberOfChannels);
    Serial.println(" to display");
    Serial.println("Enter A for all\n");
  
    while((PLAY == 0 && CLEAR == 0) && !Serial.available()){} //Wait for new input
    input = Serial.readString();

    if(input[0]-charToInt >= 1 && input[0]-charToInt <= 8){
      displayChannel(chGrp.CH[(input[0]-charToInt)-1], input[0]-charToInt);
      return;
    }

    else if(input[0] == 'A' || input[0] == 'a'){
      displayAllChannel(chGrp);
      return;
    }

    else{Serial.println("Try again");}

  }//end get input
}//end displayPattern


void displayChannel(const ch &Ch, byte channel){
  
  Serial.print("Channel ");
  Serial.println(channel);

  if(Ch.chOn == 0){Serial.println(" is off \n");}

  else{
    
    Serial.print(" bank is ");
    if(Ch.bank == 0){
      Serial.print("Default"); 
    }
    else if(Ch.bank == 120){
      Serial.print("Drum1");
    }
    else if(Ch.bank == 121){
      Serial.print("Drum2");
    }
    else if(Ch.bank == 127){
      Serial.print("Melodic");
    }
    Serial.print("; instrument is ");
    Serial.print(Ch.instrument+1);
    Serial.print("; volume is ");
    Serial.println(Ch.volume);
    Serial.print("             ");
    for(byte i=1; i<9; i++){
      Serial.print(i);
      for(byte j=0; j<4; j++){
        Serial.print(" ");
      }
    }
    for(byte i=9; i<17; i++){
      Serial.print(i);
      for(byte j=0; j<3; j++){
        Serial.print(" ");
      }
    }
    Serial.print("\n");
    Serial.print("note      ");
    
    //print notes
    for(int i=0; i<16; i++){
      if(Ch.noteOn[i] == 0){Serial.print("   - ");}
      else{
        Serial.print(" ");
        numberToNote(Ch.note[i]);
        Serial.print(" ");
      }
    }//end print notes 

    //print velocities
    Serial.print("\n"); 
    Serial.print("velocity  ");
      for(int i=0; i<16; i++){
        if(Ch.noteOn[i] == 0){Serial.print("   0 ");}
        else if(Ch.velocity[i] < 10){
          Serial.print("   ");
          Serial.print(Ch.velocity[i]);
          Serial.print(" "); 
        }
        else if(Ch.velocity[i] < 100){
          Serial.print("  ");
          Serial.print(Ch.velocity[i]);
          Serial.print(" ");
        }
        else if(Ch.velocity[i] >= 100){
          Serial.print(" ");
          Serial.print(Ch.velocity[i]);
          Serial.print(" ");
        }
      }//end print velocities

    //print duration
    Serial.print("\n"); 
    Serial.print("duration  ");
      for(int i=0; i<16; i++){
        if(Ch.noteOn[i] == 0){Serial.print("   0 ");}
        else if(Ch.duration[i] < 10){
          Serial.print("   ");
          Serial.print(Ch.duration[i]);
          Serial.print(" "); 
        }
        else{
          Serial.print("  ");
          Serial.print(Ch.duration[i]);
          Serial.print(" ");
        }
      }//end print duration
      
    
    Serial.print("\n");
  }//end else if channel is on
  
}//end displayChannel

void displayAllChannel(const CH &chGrp){

  for (byte i=0; i<NumberOfChannels; i++){
    displayChannel(chGrp.CH[i], i+1);
    Serial.print("\n");
  }
  
}//end displayAllChannel


//conver note name input to midi note number
byte noteToNumber(){

   char midiNotes[12][2] = {
    ' ','C','C','#',' ','D','D','#',' ','E',' ','F','F','#',' ','G','G','#',' ','A','A','#',' ','B'
   };

  while(PLAY == 0 && CLEAR == 0){
    Serial.println("Please enter C1 - B6");
    while((PLAY == 0 && CLEAR == 0) && !Serial.available()){}
    String input = Serial.readString();
  
    if(input[1] == '#'){
      ////add 24 to row to get note number C1=24
      if(input[2] ==  '1'){
        for(byte i = 0; i < 12; i++){
          if(midiNotes[i][0] == input[0]){return (i+24);}
        }
      }//end if octave 1
      
      else if(input[2] ==  '2'){
        for(byte i = 0; i < 12; i++){
          if(midiNotes[i][0] == input[0]){return (i+36);}
        }
      }//end if octave 2
      
      else if(input[2] ==  '3'){
        for(byte i = 0; i < 12; i++){
          if(midiNotes[i][0] == input[0]){return (i+48);}
        }
      }//end if octave 3

      else if(input[2] ==  '4'){
        for(byte i = 0; i < 12; i++){
          if(midiNotes[i][0] == input[0]){return (i+60);}
        }
      }//end if octave 4

      else if(input[2] ==  '5'){
        for(byte i = 0; i < 12; i++){
          if(midiNotes[i][0] == input[0]){return (i+72);}
        }
      }//end if octave 5

      else if(input[2] ==  '6'){
        for(byte i = 0; i < 12; i++){
          if(midiNotes[i][0] == input[0]){return (i+84);}
        }
      }//end if octave 6
      
    }//end if #

    else{
      if(input[1] ==  '1'){
        for(byte i = 0; i < 12; i++){
          if(midiNotes[i][1] == input[0]){return (i+24);}
        }
      }//end if octave 1
      
      else if(input[1] ==  '2'){
        for(byte i = 0; i < 12; i++){
          if(midiNotes[i][1] == input[0]){return (i+36);}
        }
      }//end if octave 2
      
      else if(input[1] ==  '3'){
        for(byte i = 0; i < 12; i++){
          if(midiNotes[i][1] == input[0]){return (i+48);}
        }
      }//end if octave 3

      else if(input[1] ==  '4'){
        for(byte i = 0; i < 12; i++){
          if(midiNotes[i][1] == input[0]){return (i+60);}
        }
      }//end if octave 4

      else if(input[1] ==  '5'){
        for(byte i = 0; i < 12; i++){
          if(midiNotes[i][1] == input[0]){return (i+72);}
        }
      }//end if octave 5

      else if(input[1] ==  '6'){
        for(byte i = 0; i < 12; i++){
          if(midiNotes[i][1] == input[0]){return (i+84);}
        }
      }//end if octave 6
    }
  }//end loop to get valid input
}//end note to number


//convert midi number to note name and print
void numberToNote(byte number){

  char first;
  char second;
  char third;

    
  char midiNotes[12][2] = {
  ' ','C','C','#',' ','D','D','#',' ','E',' ','F','F','#',' ','G','G','#',' ','A','A','#',' ','B'
  };

  if (number == 0){
    Serial.print("  0");
    return;  
  }
  
  //if MIDI octave 1
  else if (number >= 24 && number <=35){    
    //add 24 to row to get note number C1=24
    number = number - 24;    
    for(byte i; i<12; i++){
      first = midiNotes[number][0];
      second = midiNotes[number][1];
      third = '1'; 
      }
    }//end octave 1
    
  //if MIDI octave 2
  else if (number >= 36 && number <=47){    
    //add 36 to row to get note number C2=36
    number = number - 36;    
    for(byte i; i<12; i++){
      first = midiNotes[number][0];
      second = midiNotes[number][1];
      third = '2'; 
      }
    }//end octave 2 

  //if MIDI octave 3
  else if (number >= 48 && number <=59){    
    //add 48 to row to get note number C3=48
    number = number - 48;
    for(byte i; i<12; i++){    
      first = midiNotes[number][0];
      second = midiNotes[number][1];
      third = '3'; 
      }
    }//end octave 3 

  //if MIDI octave 4 
  else if (number >= 60 && number <=71){    
    //add 60 to row to get note number C4=60
    number = number - 60;
    for(byte i; i<12; i++){
      first = midiNotes[number][0];
      second = midiNotes[number][1];
      third = '4'; 
      }
    }//end octave 4 

  //if MIDI octave 5
  else if (number >= 72 && number <=83){    
    //add 72 to row to get note number C5=72
    number = number - 72;
    for(byte i; i<12; i++){
      first = midiNotes[number][0];
      second = midiNotes[number][1];
      third = '5'; 
      }
    }//end octave 3 

  //if MIDI octave 6
  else if (number >= 84 && number <=95){    
    //add 48 to row to get note number C6=84
    number = number - 84;    
    for(byte i; i<12; i++){
      first = midiNotes[number][0];
      second = midiNotes[number][1];
      third = '6'; 
      }
    }//end octave 6

    Serial.print(first);
    Serial.print(second);
    Serial.print(third);
  
}//end numberToNote


//PLAY patters function
void play(CH &chGrp, float beat){

  
  //Play setup
  Serial.println("Start PLAY"); 
  //Serial.end();
  

  //set Channel Banks
  for(byte i = 0; i<NumberOfChannels; i++){
    setBank(i, chGrp.CH[i].bank);
    
  }

  //set Channel Instruments
  for(byte i = 0; i<NumberOfChannels; i++){
    setInstrument(i, chGrp.CH[i].instrument);
  }

  //set Channel Volume
  for(byte i = 0; i<NumberOfChannels; i++){
    setVolume(i, chGrp.CH[i].volume);
  }
  //bool state = 1;
  byte currentBeat = 0;
  unsigned long offsetStart;
  unsigned long offsetEnd;

  //play loop
  while(PLAY == 1 && CLEAR == 0){
    offsetStart = millis();
    //check for Note OFF
    for(byte ch=0; ch<NumberOfChannels; ch++){
      for(byte beatNum=0; beatNum<16; beatNum++){
        if(chGrp.CH[ch].noteOnCount[beatNum]>chGrp.CH[ch].duration[beatNum]){
          noteOff(ch, chGrp.CH[ch].note[beatNum], chGrp.CH[ch].velocity[beatNum]);
          chGrp.CH[ch].noteOnCount[beatNum] = 0;
        }
      }
    }

    //check for note on and count
    for(byte ch=0; ch<NumberOfChannels; ch++){
      for(byte beatNum=0; beatNum<16; beatNum++){
        if(chGrp.CH[ch].noteOnCount[beatNum] > 0){chGrp.CH[ch].noteOnCount[beatNum]++;}
      }
    }

    //check for new note on
    for(byte ch=0; ch<NumberOfChannels; ch++){
      //Serial.println(ch);
      if(chGrp.CH[ch].noteOn[currentBeat] ==  1){
        noteOn(ch, chGrp.CH[ch].note[currentBeat], chGrp.CH[ch].velocity[currentBeat]);
        chGrp.CH[ch].noteOnCount[currentBeat]=1;
      }  
    }
    
    currentBeat = (currentBeat+1)%16;
    offsetEnd = millis();
    delay((beat*1000)-(offsetEnd-offsetStart));
    if(Serial.available()){stopPattern();}
  }//end wile PLAY==1 and CLEAR==0
  
}//end play

//Set midi bank
void setBank(byte chan, byte bank){
  if (chan > 15) return;
  if (bank > 127) return;
  
  
  VS1053_MIDI.write(MIDI_CHAN_MSG | chan);
  VS1053_MIDI.write((byte)MIDI_CHAN_BANK);
  VS1053_MIDI.write(bank);
}//end setBank

//Set midi instrument
void setInstrument(byte chan, byte inst){
  if (chan > 15) return;
  if (inst > 127) return;
  
  VS1053_MIDI.write(MIDI_CHAN_PROGRAM | chan);  
  VS1053_MIDI.write(inst);
}

//Set midi Volume
void setVolume(byte chan, byte vol){
  if (chan > 15) return;
  if (vol > 127) return;
  
  VS1053_MIDI.write(MIDI_CHAN_MSG | chan);
  VS1053_MIDI.write(MIDI_CHAN_VOLUME);
  VS1053_MIDI.write(vol);
}

//MIDI note ON
void noteOn(byte chan, byte note, byte velocity) {
  if (chan > 15) return;
  if (note > 127) return;
  if (velocity > 127) return;
  
  VS1053_MIDI.write(MIDI_NOTE_ON | chan);
  VS1053_MIDI.write(note);
  VS1053_MIDI.write(velocity);
}

//MIDI note OFF
void noteOff(byte chan, byte note, byte velocity) {
  if (chan > 15) return;
  if (note > 127) return;
  if (velocity > 127) return;
  
  VS1053_MIDI.write(MIDI_NOTE_OFF | chan);
  VS1053_MIDI.write(note);
  VS1053_MIDI.write(velocity);
}
  

//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx Interrupt Functions
//interrupt function to start Pattern Playing
void playPattern(){

  PLAY = 1;
  Serial.println("Start PLAY: Press any key to STOP"); 
  //Serial.end();
    
}//end playPattern


//interrupt function to stop Pattern Playing
void stopPattern(){
  

    PLAY = 0;
    CLEAR = 1;
    //Serial.begin(baud);
    Serial.println("STOP");

  
}//end stopPattern


//clear PLAY info
void clearPattern(CH &chGrp){
  
  Serial.println("Clearing Play info \n");
  digitalWrite(LED_BUILTIN, LOW);
  
  for(int i=0; i<NumberOfChannels; i++){
    for(int j=0; j<16; j++){
      //chGrp.CH[i].noteOn[j]=0;
      chGrp.CH[i].noteOnCount[j]=0;
    }
  }
  CLEAR = 0;
   
}//end clear PLAY



