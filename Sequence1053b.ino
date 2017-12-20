//Define constants
#define baud 9600
#define charToInt 48
#define NumberOfChannels 4



//Pattern Struct Definitions
struct ch{
  byte chOn = 0;
  byte bank = 1;
  byte instrument  = 1;
  byte note[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  byte noteOn[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  byte noteOnCount[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  byte duration[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  byte velocity[16]  = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
};

struct Ch{
  ch ch[NumberOfChannels];  
};

struct Song{
  byte tempo = 120;
  float beatLength = 0.5;
};



//Function definitions
char menu();



//setup
void setup() {
  Serial.begin(baud);
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println("Start the Arduino Audio Sequencer for the 1053b GM synth on chip \n\n");
}//end setup



//MAIN
void loop() {

  Song song;
  Ch chGrp;

  //Program Loop
  while(1){

    
  }//end Program Loop
  

}//end MAIN





