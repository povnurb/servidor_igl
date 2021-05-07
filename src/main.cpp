/**pendientes
 * hay que agregar un interruptor el cual permitira dar acceso remoto para activar las 
 * alarmas sugerencia el pin 14 
 * si es posible quitar una libreria que no se use para liberar espacio en memoria ram
  */

//este programa verifica via ethernet las alarmas y las indica tambien sin red
//en la sala pmct igl se alimenta con regulador de 7.5 volts
//funciona con tierras para activarse
//https://aprendiendoarduino.wordpress.com/tag/pullup/
//#include <Arduino.h>
#include <Adafruit_Sensor.h> //libreria Adafruit Unified Sensor
#include <DHT.h>//libreria DHT sensor library
#include <DHT_U.h>
#include <Ethernet.h>
#include <SPI.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal.h> //by arduino libreria
LiquidCrystal lcd(49, 48, 47, 46, 45, 44);//(rs,en,d4,d5,d6,d7)

//definicion de leds
//#define delectAllFinger 60 //borra todas las huellas
#define LED_puerta 42
#define Reinicio asm("jmp 0x0000") //para REINICIAR ARDUINO
//definicion del DHT22
#define DHTPIN 22
#define DHTTYPE DHT22
DHT_Unified dht(DHTPIN, DHTTYPE);
uint32_t delayMS;
float temp_c;
float humedad;
//int cont=0; //contador para evitar el hardcodeo por la url
////////////////////////////////////////////////////////////////////////
//CONFIGURACION Ethernet
////////////////////////////////////////////////////////////////////////

//IP manual settings
byte ip[] = { 
  10, 138, 139, 20 };   //13.4.30.52 trabajo, en casa 192, 168, 1, 78 
byte gateway[] = { 
  10, 138, 139, 254 }; //Manual setup only 13.4.30.254 trabajo lineas
byte subnet[] = { 
  255, 255, 255, 0 }; //Manual setup only no cambia

// if need to change the MAC address (Very Rare)
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x01 }; //variar por sala
  //esto es un registro
//0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x01 para la sala igl pmct es

//Ethernet Port
EthernetServer server = EthernetServer(80); //default html port 80

//Numero Total de alarmas maximo 10.
int outputQuantity = 10;  //should not exceed 10 

//Invert the output of the leds
boolean outputInverted = true; //true or false para que funcione con tierras tiene que ser true
// This is done in case the relay board triggers the relay on negative, rather then on positive supply

//Html page refresh
int refreshPage = 9; //default is 10sec. 
//Beware that if you make it refresh too fast, the page could become inacessable.

//Display or hide the "Switch on all Pins" buttons at the bottom of page
int switchOnAllPinsButton = true; //true or false se cambiara segun el estado del pin o switch

//Button Array
//Just for a note, varables start from 0 to 9, as 0 is counted as well, thus 10 outputs.

// Select the pinout address
int outputAddress[10] = { 9,8,7,6,5,3,14,15,16,17}; //Allocate 10 spaces and name the output pin address.
int indicadoresLed[10] = {23,25,27,29,31,33,35,37,39,41};
//PINES RESERVADOS NO SE PUEDEN USAR
//PS pin addresses 10, 11, 12 and 13 on the Duemilanove are used for the ethernet shield, therefore cannot be used.
//PS pin addresses 10, 50, 51 and 52 and 53 on the Mega are used for the ethernet shield, therefore cannot be used.
//PS pin addresses 4, are used for the SD card, therefore cannot be used.
//PS. pin address 2 is used for interrupt-driven notification, therefore could not be used.
int buzzer = 40; //pin del BUZZER
// Write the text description of the output channel se pueden cambiar
String buttonText[10] = {
  "01. F. RECT. PB MEI","02. DESCARGA DE BATERIAS.","03. BAJO VOLTAJE BAT",
  "04. G. E. BLOQUEADO","05. ALTA TEMPERATURA","06. FALLA DE COMPRESOR",
  "07. G. E. EN OPERACION","08. FALLA DE RED CFE",
  "09. LIBRE","10. LIBRE"};

// Set the output to retain the last status after power recycle.
int retainOutputStatus[10] = {0,0,0,0,0,0,0,0,0};//1-retain the last status. 0-will be off after power cut.

////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
//VARIABLES DECLARATION
////////////////////////////////////////////////////////////////////////
int outp = 0;
boolean printLastCommandOnce = false;
boolean printButtonMenuOnce = false;
boolean initialPrint = true;
String allOn = "";
String allOff = "";
boolean reading = false;
boolean outputStatus[10]; //Create a boolean array for the maximum ammount.
String rev = "V19 CONTRASEÑA";  //indica la version del firmware
String sala = "SALA PTTI IGL"; //variable que apenas voy a utilizar para variar los nombres
unsigned long timeConnectedAt;
boolean writeToEeprom = false;
boolean currentState = false;


/////////////////////////////////////////////////

String tempOutDeg;
String humOutDeg2;

//funciones que faltaron declarar en platformio pero no son necesarias en el id de arduino______________________________________________________
void initEepromValues();
void readEepromValues();
void checkForClient();
void triggerPin(int pin, EthernetClient client, int outp);
void printHtmlButtonTitle(EthernetClient client);
void printLoginTitle(EthernetClient client);
void printHtmlHeader(EthernetClient client);
void printHtmlFooter(EthernetClient client);
void writeEepromValues();
void readOutputStatuses();
void printHtmlButtons(EthernetClient client);
void printHex(int num, int precision);
void grabar();
void buz();
////////////////////////////////////////////////////////////////////////
//RUN ONCE
////////////////////////////////////////////////////////////////////////
//Beginning of Program
void setup(){
  pinMode(buzzer,OUTPUT);//salida
  digitalWrite(buzzer,HIGH);//se apaga
//////////////////////////////////
  lcd.begin(16, 2);//LCD
  ///////////////////////////////////////////////////////////////
  //Setup de lo demas
  ///////////////////////////////////////////////////////////////

  Serial.begin(9600);
  dht.begin();
  pinMode(LED_puerta,OUTPUT);
  digitalWrite(LED_puerta, LOW);
  initEepromValues();
  readEepromValues();

  //Set pins as Outputs 

  for (int var = 0; var < outputQuantity; var++){

    pinMode(indicadoresLed[var], OUTPUT); 
    //verifica que todos los led funcionan
    //digitalWrite(indicadoresLed[var],HIGH);
    //delay(250);
    digitalWrite(indicadoresLed[var],LOW);
  }
  //boolean currentState = false;
  for (int var = 0; var < outputQuantity; var++){

    pinMode(outputAddress[var], INPUT_PULLUP);       
    if(outputInverted == true) {
      if(outputStatus[var] == 0){currentState = true;}else{currentState = false;} //check outputStatus if off, switch output accordingly
      digitalWrite(outputAddress[var], currentState); 
    }
    else{
      if(outputStatus[var] == 0){currentState = false;}else{currentState = true;}//check outputStatus if off, switch output accordingly
      digitalWrite(outputAddress[var], currentState);
    }
  }

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("VERIFICAR RED");
  lcd.setCursor(0,1);
  lcd.print("ETHERNET");
  delay(200);
  //Setting up the IP address. Comment out the one you dont need.
  //Ethernet.begin(mac); //for DHCP address. (Address will be printed to serial.)
  Ethernet.begin(mac, ip, gateway, subnet); //for manual setup. (Address is the one configured above.) cambiar 

  server.begin();
  Serial.print("Server started at ");
  Serial.println(Ethernet.localIP());
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("MI IP:");
  lcd.setCursor(0,1);
  buz();
  lcd.print(Ethernet.localIP());
  delay(5000);
  buz();
}
//funcion buzzer que indica la duracion del pitido 
void buz(){
  digitalWrite(buzzer,LOW); //en realidad es la duracion del sonido
  delay(200);
  digitalWrite(buzzer,HIGH);
  delay(10);
}
////////////////////////////////////////////////////////////////////////
//LOOP
////////////////////////////////////////////////////////////////////////
//Run once
void loop(){

  float temp_c;
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  temp_c=event.temperature;
  //Serial.print(event.temperature);
  dht.humidity().getEvent(&event);
  humedad=event.relative_humidity;
  String to_send = String(temp_c);
  String to_send2 = String(humedad);
  tempOutDeg = to_send;
  humOutDeg2 = to_send2;
  lcd.setCursor(0,0);
  lcd.print("TEMP: "+ to_send +" C   ");
  lcd.setCursor(0,1);
  lcd.print("HUMEDAD: "+ to_send2 +" %");
  delay(1000); //este tiempo es necesario para evitar que la pantalla lcd se sature

  // listen for incoming clients, and process requests.
  checkForClient(); //funcion que checa si hay cliente conectado
  }
 
////////////////////////////////////////////////////////////////////////
//checkForClient Function checa si hay algun cliente conectado
////////////////////////////////////////////////////////////////////////
void checkForClient(){

  EthernetClient client = server.available();

  if (client) {

    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    boolean sentHeader = false;

    while (client.connected()) {
      if (client.available()) {
        //acceso(); //quitar si es necesario
        //if header was not set send it
        
         //read user input
        char c = client.read();
        
          if(c == '*'){

          printHtmlHeader(client); //call for html header and css
          printLoginTitle(client);
          printHtmlFooter(client);
          //sentHeader = true; //no recuerdo que hace hay que verificar
          break;
        }
        if(!sentHeader){
            printHtmlHeader(client); //call for html header and css
            printHtmlButtonTitle(client); //print the button title
          //This is for the arduino to construct the page on the fly. 
          sentHeader = true;
        }

        //read user input
    //    char c = client.read();

        //if there was reading but is blank there was no reading
        if(reading && c == ' '){
          reading = false;
        }

        //si hay un ? se oprimio un boton
        if(c == '?') {
          reading = true; //found the ?, begin reading the info
        }

       // por lo tanto va a leer informacion
        if(reading){
          
          //if user input is H set output to 1
          if(c == 'H') {
            outp = 1;
            
            for (int var = 0; var < outputQuantity; var++){
              pinMode(outputAddress[var], INPUT_PULLUP); 
              
            }
          }
          //if user input is L set output to 0
          if(c == 'L') {
            outp = 0;
            //*********************************************************************  tal vez los case se puedan quitar
            for (int var = 0; var < outputQuantity; var++){
                pinMode(outputAddress[var], OUTPUT); //para activar alarmas al CNS
                digitalWrite(outputAddress[var],LOW);
            }
          }
          Serial.print(c);   //print the value of c to serial communication
          Serial.print(outp);
          switch (c) {
             case '0':
                //add code here to trigger on 0
              triggerPin(indicadoresLed[0], client, outp);
               break;            
             case '1':
             //add code here to trigger on 1
               triggerPin(indicadoresLed[1], client, outp);
               break;             
             case '2':
                //add code here to trigger on 2
               triggerPin(indicadoresLed[2], client, outp);
               break;
             case '3':
                //add code here to trigger on 3
               triggerPin(indicadoresLed[3], client, outp);
               break;
             case '4':
                //add code here to trigger on 4
               triggerPin(indicadoresLed[4], client, outp);
               break;
             case '5':
                //add code here to trigger on 5
               triggerPin(indicadoresLed[5], client, outp);
               //printHtml(client);
               break;
             case '6':
                //add code here to trigger on 6
               triggerPin(indicadoresLed[6], client, outp);
               break;
             case '7':
                //add code here to trigger on 7
               triggerPin(indicadoresLed[7], client, outp);
               break;
             case '8':
                //add code here to trigger on 8
               triggerPin(indicadoresLed[8], client, outp);
               break;
             case '9':
                //add code here to trigger on 9
               triggerPin(indicadoresLed[9], client, outp);
               break;
          } //end of switch case
        }//end of switch switch the relevant output 
        //if user input was blank
        if (c == '\n' && currentLineIsBlank){
          printLastCommandOnce = true;
          printButtonMenuOnce = true;
          triggerPin(777, client, outp); //Call to read input and print menu. 777 is used not to update any outputs
          break;
        }
      }
    }  
    printHtmlFooter(client); //Prints the html footer
  } 
  else
  {  //ACTUALIZACION DE LA PAGINA
  
    //And time of last page was served is more then a minute.
    if (millis() > (timeConnectedAt + 20000)){//tiempo que tarda en verificar si existe supervision 
      lcd.setCursor(0,1);
  lcd.print("SIN  SUPERVISION");
  delay(1000);
  //los pines se vuelven entrada y dejan de ser salida ya que si me hardcodean y salen de la pagina en automatico volveran a ser inputs--------------------------------------------------------------------------------------------------------------------------------
  for (int var = 0; var < outputQuantity; var++){

    pinMode(outputAddress[var], INPUT_PULLUP);       
    if(outputInverted == true) {
      if(outputStatus[var] == 0){currentState = true;}else{currentState = false;} //check outputStatus if off, switch output accordingly
      digitalWrite(outputAddress[var], currentState); 
    }
    else{
      if(outputStatus[var] == 0){currentState = false;}else{currentState = true;}//check outputStatus if off, switch output accordingly
      digitalWrite(outputAddress[var], currentState);
    }
  }
  
for (int var = 0; var < outputQuantity; var++)  { //-----------------------------------------------------------------------------------------------------     
    
    if (digitalRead(outputAddress[var]) == true ){                                                             //If Output is ON
      if (outputInverted == false){   //esta en true                                                        //and if output is not inverted 
        digitalWrite(indicadoresLed[var],HIGH);
      }
      else{                                   //1                                                 //else output is inverted then
        digitalWrite(indicadoresLed[var],LOW);
      }
    }
    else   //entro en este cuando son altos (1)                                                                                   //If Output is Off
    {
      if (outputInverted == false){   //error o no en el false                                                        //and if output is not inverted
        digitalWrite(indicadoresLed[var],LOW);
      }
      else{                                                                                   //else output is inverted then 
        digitalWrite(indicadoresLed[var],HIGH);
      }
    }  
  }

      if (writeToEeprom == true){ 
        writeEepromValues();  //write to EEprom the current output statuses
        Serial.println("No Clients for more then a minute - Writing statuses to Eeprom.");
        writeToEeprom = false;
      }          
    }
  }
}
////////////////////////////////////////////////////////////////////////
//triggerPin Function cambia todos los leds a encendidos y apagados
////////////////////////////////////////////////////////////////////////
//
void triggerPin(int pin, EthernetClient client, int outp){
  //Switching on or off outputs, reads the outputs and prints the buttons   


  //Setting Outputs
  if (pin != 777){  //************************************************

    if(outp == 1) {
      if (outputInverted ==false){ 
        digitalWrite(pin, HIGH);
      } 
      else{
        digitalWrite(pin, LOW);
      }
    }
    if(outp == 0){
      if (outputInverted ==false){ 
        digitalWrite(pin, LOW);
      } 
      else{
        digitalWrite(pin, HIGH);
      }
    }
  }
  //Refresh the reading of outputs
  readOutputStatuses();


  //Prints the buttons
  if (printButtonMenuOnce == true){
    printHtmlButtons(client);
    printButtonMenuOnce = false;
  }

}

////////////////////////////////////////////////////////////////////////
//printHtmlButtons Function
////////////////////////////////////////////////////////////////////////
//print the html buttons to switch on/off channels
void printHtmlButtons(EthernetClient client){

  //Start to create the html table
  client.println("");
  //client.println("<p>");
  client.println("<FORM>");
  client.println("<table border=\"0\" align=\"center\">");

  //Printing the Temperature
  client.print("<tr>\n");        
  client.print("<td><h4>");
  client.print("Temperatura");
  client.print("</h4></td>\n");       
  client.print("<td>");
  client.print("<h2>");
  client.print(tempOutDeg);// valor de la temperatura
  client.print(" &deg;C</h2></td>\n");

  //Printing the Humedad
  client.print("<tr>\n");        
  client.print("<td><h4>");
  client.print("Humedad");
  client.print("</h4></td>\n");          
  client.print("<td>");
  client.print("<h2>");
  client.print(humOutDeg2);// valor de la Humedad
  client.print(" %</h2></td>\n");
  client.print("<td></td>");
  client.print("</tr>");

  //este ciclo  pone en alto o bajo todos los leds y salidas
  for (int var = 0; var < outputQuantity; var++)  { //-----------------------     

    //set command for all on/off
     allOn += "H";
     allOn += indicadoresLed[var];
     allOff += "L";
     allOff += indicadoresLed[var];

    //Print begining of row
    client.print("<tr>\n");        

    //Prints the button Text
    client.print("<td><h4>");
    client.print(buttonText[var]);
    client.print("</h4></td>\n");
    /*************************************************************************
    //quite los botones individuales ya que no los uso

    //Prints the ON Buttons  //_________________________________________
    // client.print("<td>");
    // //client.print(buttonText[var]);
    // client.print("<INPUT TYPE=\"button\" VALUE=\"ON ");
    // //client.print(buttonText[var]);
    // client.print("\" onClick=\"parent.location='/?H");
    // client.print(var);
    // client.print("'\"></td>\n");

    // //Prints the OFF Buttons 
    // client.print(" <td><INPUT TYPE=\"button\" VALUE=\"OFF");
    // //client.print(var);
    // client.print("\" onClick=\"parent.location='/?L");
    // client.print(var);
    // client.print("'\"></td>\n");
    //-----------------------------------------------------------------
    **************************************************/
    //Print first part of the Circles or the LEDs

    //Invert the LED display if output is inverted.

    ////aqui pongo en rojo o en azul segun la entrada y los leds//////////

    if (outputStatus[var] == true ){                                                            //If Output is ON
      if (outputInverted == false){   //error o no en el false                                                          //and if output is not inverted 
        client.print(" <td><div class='green-circle'><div class='glare'></div></div></td>\n"); //Print html for ON LED
        digitalWrite(indicadoresLed[var],HIGH);
      }
      else{                                                                                    //else output is inverted then
        client.print(" <td><div class='black-circle'><div class='glare'></div></div></td>\n"); //Print html for OFF LED
        digitalWrite(indicadoresLed[var],LOW);
      }
    }
    else                                                                                      //If Output is Off
    {
      if (outputInverted == false){   //error o no en el false                                                        //and if output is not inverted
        client.print(" <td><div class='black-circle'><div class='glare'></div></div></td>\n"); //Print html for OFF LED
        digitalWrite(indicadoresLed[var],LOW);
      }
      else{                                                                                   //else output is inverted then 
        client.print(" <td><div class='green-circle'><div class='glare'></div></div></td>\n"); //Print html for ON LED                    
        digitalWrite(indicadoresLed[var],HIGH);
      }
    }  
    //Print end of row
    client.print("</tr>\n");  
  }   //---------------------------------------------------------------
  //Muestra el boton para encender todas las alarmas
  if (switchOnAllPinsButton == true ){

    //Prints the OFF All Pins Button modificado
    client.print("<tr>\n<td><INPUT TYPE=\"button\" VALUE=\"NORMALIZAR");
    //client.print("\" onClick=\"parent.location='/?");
    client.print("\" onClick='apagar2()'");
    //client.print(allOn);
    client.print("'\"></td>\n");

    //Prints the ON All Pins Button modificado           
    client.print("<td><INPUT TYPE=\"button\" VALUE=\"ENCENDER");
    //client.print("\" onClick=\"parent.location='/?");
    client.print("\" onClick='encender()'");
    //client.print(allOff);
    client.print("></td>\n<td></td>\n<td></td>\n</tr>\n");
  }

  //Closing the table and form
  client.println("</table>");
  client.println("</FORM>");
  //client.println("</p>"); 

}

////////////////////////////////////////////////////////////////////////
//readOutputStatuses Function
////////////////////////////////////////////////////////////////////////
//Esta funcion lee el estado de los botones por lo tanto lo cambiare al 
//estado de las entradas
void readOutputStatuses(){
  for (int var = 0; var < outputQuantity; var++)  { 
    outputStatus[var] = digitalRead(outputAddress[var]);
    //Serial.print(outputStatus[var]);
  }

}

////////////////////////////////////////////////////////////////////////
//readEepromValues Function tal vez se pueda quitar para tener mas memoria
////////////////////////////////////////////////////////////////////////
//Read EEprom values and save to outputStatus
void readEepromValues(){
    for (int adr = 0; adr < outputQuantity; adr++)  { 
    outputStatus[adr] = EEPROM.read(adr); 
    }
}

////////////////////////////////////////////////////////////////////////
//writeEepromValues Function, tal vez se pueda quitar esta funcion para tener mas memoria----------------------------
////////////////////////////////////////////////////////////////////////
//Write EEprom values
void writeEepromValues(){
    for (int adr = 0; adr < outputQuantity; adr++)  { 
    EEPROM.write(adr, outputStatus[adr]);
    }

} 

////////////////////////////////////////////////////////////////////////
//initEepromValues Function tal vez se pueda quitar esta funcion ------------------------------------------------------
////////////////////////////////////////////////////////////////////////
//Initialiaze EEprom values
//if eeprom values are not the correct format ie not euqual to 0 or 1 (thus greater then 1) initialize by putting 0
void initEepromValues(){
  for (int adr = 0; adr < outputQuantity; adr++){        
    if (EEPROM.read(adr) > 1){
      EEPROM.write(adr, 0);
    } 

  }
  
}


////////////////////////////////////////////////////////////////////////
//htmlHeader Function
////////////////////////////////////////////////////////////////////////
//Prints html header
 void printHtmlHeader(EthernetClient client){
          Serial.print("Serving html Headers at ms -");
          timeConnectedAt = millis(); //Record the time when last page was served.
          Serial.print(timeConnectedAt); // Print time for debbugging purposes
          writeToEeprom = true; // page loaded so set to action the write to eeprom
          
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connnection: close");
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<head>");

          // add page title 
          //client.println("<title>SALA PMCT IGL</title>");
          client.print("<title>");
          client.print(sala);
          client.println("</title>");
          //client.println("<meta charset='UTF-8' name=\"description\" content=\"PMCT IGL\"/>");
          client.print("<meta charset='UTF-8' name=\"description\" content=\"");
          client.print(sala);
          client.println("\"/>");
          // add a meta refresh tag, so the browser pulls again every x seconds:
          client.print("<meta http-equiv=\"refresh\" content=\"");
          client.print(refreshPage);
          client.println("; url=/\">");

          // add other browser configuration
          client.println("<meta name=\"apple-mobile-web-app-capable\" content=\"yes\">");
          client.println("<meta name=\"apple-mobile-web-app-status-bar-style\" content=\"default\">");
          client.println("<meta name=\"viewport\" content=\"width=device-width, user-scalable=no\">");          

          //inserting the styles data, usually found in CSS files.
          client.println("<style type=\"text/css\">");
          client.println("");

          //This will set how the page will look graphically
          client.println("html { height:100%; }");  

          client.println("  body {");
          client.println("    height: 100%;");
          client.println("    margin: 0;");
          client.println("    font-family: helvetica, sans-serif;");
          client.println("    -webkit-text-size-adjust: none;");
          client.println("   }");
          client.println("");
          client.println("body {");
          client.println("    -webkit-background-size: 100% 21px;");
          client.println("    background-color: #6996b9;"); //#4682b4
          // client.println("    background-image:");
          // client.println("    -webkit-gradient(linear, left top, right top,");
          // client.println("    color-stop(.75, transparent),");
          // client.println("    color-stop(.75, rgba(255,255,255,.1)) );");
          // client.println("    -webkit-background-size: 7px;");
          client.println("   }");
          client.println("");
          client.println(".view {");
          client.println("    min-height: 100%;");
          client.println("    overflow: auto;");
          client.println("   }");
          client.println("");
          client.println(".header-wrapper {");
          client.println("    height: 44px;");
          client.println("    font-weight: bold;");
          client.println("    text-shadow: rgba(0,0,0,0.7) 0 -1px 0;");
          client.println("    border-top: solid 1px rgba(255,255,255,0.6);");
          client.println("    border-bottom: solid 1px rgba(0,0,0,0.6);");
          client.println("    color: #fff;");
          client.println("    background-color: #8195af;");
          client.println("    background-image:");
          client.println("    -webkit-gradient(linear, left top, left bottom,");
          client.println("    from(rgba(255,255,255,.4)),");
          client.println("    to(rgba(255,255,255,.05)) ),");
          client.println("    -webkit-gradient(linear, left top, left bottom,");
          client.println("    from(transparent),");
          client.println("    to(rgba(0,0,64,.1)) );");
          client.println("    background-repeat: no-repeat;");
          client.println("    background-position: top left, bottom left;");
          client.println("    -webkit-background-size: 100% 21px, 100% 22px;");
          client.println("    -webkit-box-sizing: border-box;");
          client.println("   }");
          client.println("");
          client.println(".header-wrapper h1 {");
          client.println("    text-align: center;");
          client.println("    font-size: 20px;");
          client.println("    line-height: 44px;");
          client.println("    margin: 0;");
          client.println("   }");
          client.println("");
          client.println(".group-wrapper {");
          client.println("    margin: 9px;");
          client.println("    }");
          client.println("");
          client.println(".group-wrapper h2 {");
          client.println("    text-align: center;");
          client.println("    color: #000;");
          client.println("    font-size: 17px;");
          client.println("    line-height: 0.8;");
          client.println("    font-weight: bold;");
          client.println("    text-shadow: #fff 0 1px 0;");
          client.println("    margin: 20px 10px 12px;");
          client.println("   }");
          client.println("");
          client.println(".group-wrapper h3 {");
          client.println("    color: #000;");
          client.println("    font-size: 12px;");
          client.println("    line-height: 1;");
          client.println("    font-weight: bold;");
          client.println("    text-shadow: #fff 0 1px 0;");
          client.println("    margin: 20px 10px 12px;");
          client.println("   }");
          client.println("");
          client.println(".group-wrapper h4 {");  //Text for description
          client.println("    color: #212121;");
          client.println("    font-size: 14px;");
          client.println("    line-height: 1;");
          client.println("    font-weight: bold;");
          client.println("    text-shadow: #aaa 1px 1px 3px;");
          client.println("    margin: 5px 5px 5px;");
          client.println("   }");
          client.println(""); 
          client.println(".group-wrapper table {");
          client.println("    background-color: #fff;");
          client.println("    -webkit-border-radius: 10px;");
          client.println("    -moz-border-radius: 10px;");
          client.println("    -khtml-border-radius: 10px;");
          client.println("    border-radius: 10px;");

          client.println("    font-size: 17px;");
          client.println("    line-height: 20px;");
          client.println("    margin: 9px 0 20px;");
          client.println("    border: solid 1px #a9abae;");
          client.println("    padding: 11px 3px 12px 3px;");
          client.println("    margin-left:auto;");
          client.println("    margin-right:auto;");

          client.println("    -moz-transform :scale(1);"); //Code for Mozilla Firefox
          client.println("    -moz-transform-origin: 0 0;");

          client.println("   }");
          client.println("");

          //how the green (ON) LED will look
          client.println(".green-circle {");
          client.println("    display: block;");
          client.println("    height: 29px;");
          client.println("    width: 29px;");
          //client.println("    background-color: #f00;");
          client.println("    background-color: rgba(254, 0, 0);");
          client.println("    -moz-border-radius: 12px;");
          client.println("    -webkit-border-radius: 12px;");
          client.println("    -khtml-border-radius: 12px;");
          client.println("    border-radius: 12px;");
          client.println("    margin: auto;"); //margin-left: 2px;

          client.println("    background-image: -webkit-gradient(linear, 0% 0%, 0% 90%, from(rgba(184, 46, 0, 0.8)), to(rgba(255, 148, 112, .9)));@");
          client.println("    border: 2px solid #ccc;");
          client.println("    -webkit-box-shadow: rgba(11, 140, 27, 0.5) 0px 10px 16px;");
          client.println("    -moz-box-shadow: rgba(11, 140, 27, 0.5) 0px 10px 16px; /* FF 3.5+ */");
          client.println("    box-shadow: rgba(11, 140, 27, 0.5) 0px 10px 16px; /* FF 3.5+ */");
          client.println("    }");
          client.println("");

          //how the black (off)LED will look
          client.println(".black-circle {");
          client.println("    display: block;");
          client.println("    height: 23px;");
          client.println("    width: 23px;");
          client.println("    background-color: #0ad;");//era 40
          client.println("    -moz-border-radius: 11px;");
          client.println("    -webkit-border-radius: 11px;");
          client.println("    -khtml-border-radius: 11px;");
          client.println("    border-radius: 11px;");
          client.println("    margin: auto;");//margin-left: 1px;"
          client.println("    -webkit-box-shadow: rgba(11, 140, 27, 0.5) 0px 10px 16px;");
          client.println("    -moz-box-shadow: rgba(11, 140, 27, 0.5) 0px 10px 16px; /* FF 3.5+ */"); 
          client.println("    box-shadow: rgba(11, 140, 27, 0.5) 0px 10px 16px; /* FF 3.5+ */");
          client.println("    }");
          client.println("");

          //this will add the glare to both of the LEDs
          client.println("   .glare {");
          client.println("      position: relative;");
          client.println("      top: 1;");
          client.println("      left: 5px;");
          client.println("      -webkit-border-radius: 10px;");
          client.println("      -moz-border-radius: 10px;");
          client.println("      -khtml-border-radius: 10px;");
          client.println("      border-radius: 10px;");
          client.println("      height: 1px;");
          client.println("      width: 13px;");
          client.println("      padding: 5px 0;"); 
          client.println("      background-color: rgba(200, 200, 200, 0.25);");
          client.println("      background-image: -webkit-gradient(linear, 0% 0%, 0% 95%, from(rgba(255, 255, 255, 0.7)), to(rgba(255, 255, 255, 0)));");
          client.println("    }");
          client.println("");

          //and finally this is the end of the style data and header
          client.println("</style>");
          client.println("</head>");

          //now printing the page itself
          client.println("<body>");
          client.println("<div class=\"view\">");
          client.println("    <div class=\"header-wrapper\">");
          //client.println("      <h1>SALA PMCT IGL</h1>");
          client.print("<h1>");
          client.print(sala);
          client.println("</h1>");
          client.println("    </div>");

//////

 } //end of htmlHeader

////////////////////////////////////////////////////////////////////////
//htmlFooter Function y funciones javaScript
////////////////////////////////////////////////////////////////////////
//Prints html footer
void printHtmlFooter(EthernetClient client){
    //Set Variables Before Exiting 
    printLastCommandOnce = false;
    printButtonMenuOnce = false;
    //allOn = "";
    //allOff = "";
    //printing last part of the html
    client.println("\n<h3 align=\"center\">SE VE MEJOR EN CHROME<br> Coatza - Abril - 2021 - ");
    client.println(rev);
    client.println("</h3></div>\n</div>");
    client.println("<script>");
    client.println("var tiempo;");  //tiempo
    client.println("var normal = localStorage.getItem('normal');");
    client.println("console.log(normal);");
    //client.println("console.log('inicio del programa');");
    //---------funcion encender 
    client.println("function encender(){");
    client.println("var response = prompt('Aviso: Por cuestiones de seguridad en dos minutos se normalizarán las alarmas. ¡Escribe la Contraseña!');");
    client.println("if(response == 'Com Coatzacoalcos'){");
    client.println("tiempo = Date.now()+120000;"); //valor en milisegundo ahorita 2 minutos aqui se modifica el tiempo que estara prendida las alarmas
    client.println("console.log('en prueba con contraseña');");
    client.println("localStorage.setItem('tiempo',tiempo);");
    client.println("localStorage.setItem('normal',0);"); //esta corriendo en prueba por eso normal es 0
    client.print(" window.location=");
    client.print("'");
    client.print("/?");
    client.print(allOff); //aqui quedo encendido
    client.print("'");
    client.println("}");
    client.println("}");
    //------------------termina funcion encender
    //------------------funcion del boton
    client.println("evaluar();");
    client.println("function apagar2(){");
    client.println("localStorage.setItem('normal',1);"); //esta corriendo en normal por eso es 1
    client.println("console.log('se debe apagar');");
    client.print(" window.location=");
    client.print("'");
    client.print("/?");
    client.print(allOn); //aqui apago
    client.print("'");
    client.println("}");
    //----------------------------funcion que evalua si hay que apagar o encender las alarmas----------------------------------- 
    client.println("function evaluar(){");
    client.println("normal=localStorage.getItem('normal');"); //empieza a correr normal si es 1 quiere decir que no se a primido el boton de encender
    client.println("tiempo = localStorage.getItem('tiempo');");//obtenemos el tiempo de localStorage
    //-------------------------------si esta iniciando el programa--------------------------------------
    client.println("if(normal==null){"); //si esta en normalquiere decir que no han oprimido el boton encender, y si el date.now() es mayor que el tiempo en localStorage
    client.println("localStorage.setItem('normal',1);"); //esta corriendo en normal por eso es 1
    client.println("}");
    //---------------------------------------------------------------------
    client.println("if(normal == 1 && Date.now()>tiempo){"); //si esta en normalquiere decir que no han oprimido el boton encender, y si el date.now() es mayor que el tiempo en localStorage
    client.println("tiempo = Date.now()+1000;"); //incremento el tiempo para que este siempre sea mayor creo que para evitar un hardcode
    client.println("localStorage.setItem('tiempo',tiempo);"); //y lo guardo en localStorage
    client.println("console.log('aumento el tiempo en storage y sigue en condicion normal');");
    client.println("setTimeout(apagar2,9000);");
    //client.print(" window.location=");
    //client.print("'");
    //client.print("/?");
    //client.print(allOn); //sigo apagado
    //client.print("'");
    client.println("}");
    //---------------------------------termina apagando Estado = 0 y tiempo < => Estado = 0 y tiempo >
    //---------------------------------------------------------------- se mantiene encendido sin cambios
    client.println("if(normal==0 && Date.now()>tiempo){"); //si esta normal en 0 quiere decir que esta en prueba y si tiempo es mayor se a cumplido el tiempo y hay que apagar
    client.println("localStorage.setItem('normal',1);"); //apago y pongo en normal por eso es 1
    client.println("console.log('se acabó el tiempo por eso se apaga y se pone en condicion normal')");
    client.println("console.log(tiempo)");
    client.print(" window.location=");
    client.print("'");
    client.print("/?");
    client.print(allOn); //es apagado
    client.print("'");
    client.println("}");
    client.println("}");
    client.println("</script>");
    client.println("</body>\n</html>");

    delay(1); // give the web browser time to receive the data

    client.stop(); // close the connection:
    
    Serial.println(" - Done, Closing Connection.");
    
    delay (2); //delay so that it will give time for client buffer to clear and does not repeat multiple pages.
    allOn="";
    allOff = "";
 } //end of htmlFooter

////////////////////////////////////////////////////////////////////////
//printHtmlButtonTitle Function
////////////////////////////////////////////////////////////////////////
//Prints html button title
void printHtmlButtonTitle(EthernetClient client){
          client.println("<div  class=\"group-wrapper\">");
          client.println("    <h2>ALARMAS</h2>");
          client.println();
}


////////////////////////////////////////////////////////////////////////
//printLoginTitle Function talvez se pueda quitar esta funcion  -----------------------------------------------------
////////////////////////////////////////////////////////////////////////
//Prints html button title
void printLoginTitle(EthernetClient client){
      //    client.println("<div  class=\"group-wrapper\">");
          client.println("    <h2>Please enter the user data to login.</h2>");
          client.println();
}