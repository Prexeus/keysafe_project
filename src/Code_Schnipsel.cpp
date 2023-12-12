//geladene Bibliotheken
#include <LiquidCrystal_I2C.h>
#include <Arduino.h>




//Festlegen der Pinbelegung am Arduino 
  //Pins für 75HC595 Schieberegister
    const byte dataPin_595 = 12;     //  Din/SI -> Daten Serial Input vom Arduino; PIN 14 bei 74HC595
    const byte clockPin_595 = 10;    // Shift/SCK -> Takt für das Laden der am dataPin_595 anliegenden Zustände ins Schieberegister (für jedes Bit einmal Schalten); PIN 11 bei 74HC595
    const byte latchPin_595 = 11;    // Store/ RCK -> Freigabe der Zustände des Schieberegisters auf die Ausgänge; PIN 12 bei 74HC595
    const byte OutEnablePin_595 = 13; // |G/OE -> Freigabe der Ausgänge, wenn Pin 13 auf LOW -> Pull_UP Widerstand auf Schaltplan, um undefinierte Zusände beim Anschalten zu verhindern; PIN 13 bei 74HC595

//Pins für 75HC165 Schieberegister
    const byte dataPin_165 = 7;     // QH -> Daten Serial Output zum Arduino; PIN 9 bei 74HC165
    const byte clockPin_165 = 6;    // CLK -> Takt für das Lesen der am dataPin_165 anliegenden Zustände aus dem Schieberegister (für jedes Bit einmal Schalten); PIN 2 bei 74HC165
    const byte latchPin_165 = 8 ;    // SH/|LD -> Schiebe Stellung (High) oder Laden der Eingangszustände ins Schiebregister (LOW); PIN 1 bei 74HC165

// Anzahl Schieberegister verbaut:
    const byte No_74HC595 = 2;
    const byte No_74HC165 = 4;

// Anzahl der im LCD Display vorhandenen Zeichen pro Zeile (verwendet wird ein 20x4 Zeichen I2C Display)
    const byte LEDLINE = 20;
    LiquidCrystal_I2C lcd(0x27,20,4); //Einstellen der LCD Adresse auf 0x27, sowie für 20 Zeichen und 4 Zeilen

// Definieren der Scrollgeschwindigkeit für die 4 LED Zeilen. es sind individuelle Geschwinidkeit je Zeile möglich
     const int ledScrollSpeed[4]={300,300,300,300};
// Definieren der Scrollrichtung für ein 4 Zeilen Display (true bedeutet von rechts nach links))
     const boolean ledScrollDir[4]={true,true,true,true};   
// Definition der Textvariablen des LCD Displays - 2-dimensionales Array für 4 Zeilen mit je 178 Zeichen --> //Da das LCD Display 20 Zeilen hat müssen im String immer mindestens 20 Zeichen angegeben sein --> Ggf. mit Leerzeichen auf 20 Zeichen auffüllen! Sonst kann es zu Artefakten auf dem Display vom vorherigen Text kommen!
     char TextZeile[4][178];
     char LastTextZeile[4][178];  //{{0},{0},{0},{0}};    letzten Zustand der Textzeile merken um Veränderungen zu erkennen!
     
// Definition des Zeitintervalls zur Einschlaten der Dämpfung des Einfahrens des Knichrichtungsvorgebers (KRV) --> zuerst MagVen_KRV_raus über diese Zeitspanne auf 1, dann MagVen_KRV_raus & MagVen_KRV_rein = 1 --> Damit Dämpfungsdruck auf der Zylinderabströmseite trotz großem Leitungsvolumen aufgebaut ist!
     const int Druckaufbauzeit_Daempfung_KRV_rein = 1000; //in Millisekunden
     unsigned long nextDaempfungsende = millis(); //ersten Wert setzen

// Definition des Zeitintervalls zum Anzeigen der Fehler beim Zustandswechsel im Ablaufplan --> damit das nicht direkt erscheint, wenn man den Knopf noch gedrück hält vom vorangegangenen Schritt!
    const int wait_failure = 2000; //in Millisekunden
    unsigned long next_failure_time = millis(); //ersten Wert setzen

//Globale Variablen definieren
     byte OutSR595[No_74HC595] = {0}; //Die Ausgänge der Schieberegister 74HC595 auf Null festgelegt
     byte InSR165[No_74HC165] = {0}; // Die Eingänge der Schiebregister 74HC165auf Null festlegen
     int Zustand = 0; //Variable für die einzelnen Betriebszustände der Ablaufsteuerung
    

  // Für die Eingänge der 74HC165 die Anschlüsse, Schieberegisternummer und Bitnummer definieren! --> last** sind die alten Werte der Variablen, die für die Flankenerkennung einzelner Sensoren/Schalter notwendig sind!
     byte BWS_01 = 0; byte last_BWS_01 = 0; const byte SR_BWS_01 = 0; const byte Bit_BWS_01 = 0;    // Betriebswahlschalter Position 1 "Werkzeug in OT fahren und sichern"                          --> SR0/Bit0
     byte BWS_02 = 0; byte last_BWS_02 = 0; const byte SR_BWS_02 = 0; const byte Bit_BWS_02 = 1;    // Betriebswahlschalter Position 2 "Probe einlegen oder wechseln"                               --> SR0/Bit1
     byte BWS_03 = 0; byte last_BWS_03 = 0; const byte SR_BWS_03 = 0; const byte Bit_BWS_03 = 2;    // Betriebswahlschalter Position 3 "Knickrichtungsvorgeber in Positions fahren"                 --> SR0/Bit2
     byte BWS_04 = 0; byte last_BWS_04 = 0; const byte SR_BWS_04 = 0; const byte Bit_BWS_04 = 3;    // Betriebswahlschalter Position 4 "Versuchsbereitschaft herstellen und Versuch durchführen"    --> SR0/Bit3
     byte BWS_05 = 0; byte last_BWS_05 = 0; const byte SR_BWS_05 = 0; const byte Bit_BWS_05 = 4;    // Betriebswahlschalter Position 5 "Probenentnahme"                                             --> SR0/Bit4
     byte BWS_06 = 0; byte last_BWS_06 = 0; const byte SR_BWS_06 = 0; const byte Bit_BWS_06 = 5;    // Betriebswahlschalter Position 6 "Werkzeugwechsel - Von OT nach UT fahren"                    --> SR0/Bit5
     byte BWS_07 = 0; byte last_BWS_07 = 0; const byte SR_BWS_07 = 0; const byte Bit_BWS_07 = 6;    // Betriebswahlschalter Position 7 "Bastellstellung - manueller Betrieb"                        --> SR0/Bit6
     byte BWS_08 = 0; byte last_BWS_08 = 0; const byte SR_BWS_08 = 0; const byte Bit_BWS_08 = 7;    // Betriebswahlschalter Position 8 "not used" --> Meldung am Display                            --> SR0/Bit7

     byte BWS_09 = 0; byte last_BWS_09 = 0; const byte SR_BWS_09 = 1; const byte Bit_BWS_09 = 0;    // Betriebswahlschalter Position 9 "not used" --> Meldung am Display                            --> SR1/Bit0
     byte BWS_10 = 0; byte last_BWS_10 = 0; const byte SR_BWS_10 = 1; const byte Bit_BWS_10 = 1;    // Betriebswahlschalter Position 10 "not used" --> Meldung am Display                           --> SR1/Bit1
     byte BWS_11 = 0; byte last_BWS_11 = 0; const byte SR_BWS_11 = 1; const byte Bit_BWS_11 = 2;    // Betriebswahlschalter Position 11 "not used" --> Meldung am Display                           --> SR1/Bit2
     byte BWS_12 = 0; byte last_BWS_12 = 0; const byte SR_BWS_12 = 1; const byte Bit_BWS_12 = 3;    // Betriebswahlschalter Position 12 "not used" --> Meldung am Display                           --> SR1/Bit3
     
     byte RK_HZ_R_unten;  const byte SR_RK_HZ_R_unten = 1;  const byte Bit_RK_HZ_R_unten = 4;   // Reedkontakt Hubzylinder rechts unten                                   --> SR1/Bit4
     byte RK_HZ_R_oben;   const byte SR_RK_HZ_R_oben = 1;   const byte Bit_RK_HZ_R_oben = 5;    // Reedkontakt Hubzylinder rechts oben                                    --> SR1/Bit5
     byte RK_HZ_L_unten;  const byte SR_RK_HZ_L_unten = 1;  const byte Bit_RK_HZ_L_unten = 6;   // Reedkontakt Hubzylinder links unten                                    --> SR1/Bit6
     byte RK_HZ_L_oben;   const byte SR_RK_HZ_L_oben = 1;   const byte Bit_RK_HZ_L_oben = 7;    // Reedkontakt Hubzylinder links oben                                     --> SR1/Bit7

     byte RK_KRV_innen;                             const byte SR_RK_KRV_innen = 2;   const byte Bit_RK_KRV_innen = 0;    // Reedkontakt Knickrichtungsvorgeber innen                               --> SR2/Bit0
     byte RK_KRV_aussen;                            const byte SR_RK_KRV_aussen = 2;  const byte Bit_RK_KRV_aussen = 1;   // Reedkontakt Knickrichtungsvorgeber außen                               --> SR2/Bit1
     byte RK_WZS_innen;                             const byte SR_RK_WZS_innen = 2;   const byte Bit_RK_WZS_innen = 2;    // Reedkontakt Werkzeugsicherung innen                                    --> SR2/Bit2
     byte RK_WZS_aussen;                            const byte SR_RK_WZS_aussen = 2;  const byte Bit_RK_WZS_aussen = 3;   // Reedkontakt Werkzeugsicherung außen                                    --> SR2/Bit3
     byte S_HZ_hoch;                                const byte SR_S_HZ_hoch = 2;      const byte Bit_S_HZ_hoch = 4;       // Schalter Hubzylinder nach oben fahren                                  --> SR2/Bit4
     byte S_KRV_rein;     byte last_S_KRV_rein = 0; const byte SR_S_KRV_rein = 2;     const byte Bit_S_KRV_rein = 5;      // Schalter Knickrichtungsvorgeber nach innen fahren                      --> SR2/Bit5
     byte S_KRV_raus;                               const byte SR_S_KRV_raus = 2;     const byte Bit_S_KRV_raus = 6;      // Schalter Knickrichtungsvorgeber nach außen fahren                      --> SR2/Bit6
                                                                                                                          // LEER                                                                   --> SR2/Bit7

     byte BT; byte last_BT = 0; const byte SR_BT = 3;             const byte Bit_BT = 0;              // Bestätigungstaster                                               --> SR3/Bit0
     byte DS_Festo;             const byte SR_DS_Festo = 3;       const byte Bit_DS_Festo = 1;        // Druckschalter Festo                                              --> SR3/Bit1                                                     
     // Alle weiteren 6 Eingänge von Schieberegister 4 (74HC165er) sind frei und können bei Bedarf verwendet werden --> Schraubklemmen und Pulldown Widerstände vorhanden!

  // Ausgänge definieren an den 74HC595
    byte MagVen_HZ=0;           const byte SR_MagVen_HZ = 0;       const byte Bit_MagVen_HZ = 0;                  // Magnetventil Hubzylinder                             --> SR0/Bit0    
    byte MagVen_KRV_rein=0;     const byte SR_MagVen_KRV_rein = 0;       const byte Bit_MagVen_KRV_rein = 1;      // Magnetventil Knickrichtungsvorgeber rein             --> SR0/Bit1
    byte MagVen_KRV_raus=0;     const byte SR_MagVen_KRV_raus = 0;       const byte Bit_MagVen_KRV_raus = 2;      // Magnetventil Knickrichtungsvorgeber raus             --> SR0/Bit2   
    
    byte Led_Gruen=0;     const byte SR_Led_Gruen = 1;       const byte Bit_Led_Gruen = 0;      // Gruene Led                                                             --> SR1/Bit0    
    byte Led_Gelb=0;      const byte SR_Led_Gelb = 1;        const byte Bit_Led_Gelb = 1;       // Gelbe Led                                                              --> SR1/Bit1
    byte Led_Rot=0;       const byte SR_Led_Rot = 1;         const byte Bit_Led_Rot = 2;        // Rote Led                                                               --> SR1/Bit2

 //Variable für Flankenerkennung
  const byte TimeStepAntiBouncing = 30; //Zeit in Milliskeunden zwischen den einzelnen Überprüfungen, ob der Taster gedrückt wurde. Wichtig um Schwingungen im Schalter "Bouncing" zu kompensieren
  unsigned long nextCheckTimeFlanke = millis() + TimeStepAntiBouncing; //Setzen der ersten Checkzeit

void setup() {
  // put your setup code here, to run once:

  //Definition der Pins am Arduino
  // Pins für 74HC595
   pinMode(OutEnablePin_595, OUTPUT);  
   digitalWrite(OutEnablePin_595, HIGH); // Ausgänge hochohmig (sperren) --> Um zufällige Einschaltzustände zu vermeiden!
        
   pinMode(dataPin_595, OUTPUT);
   pinMode(clockPin_595, OUTPUT);
   pinMode(latchPin_595, OUTPUT);

  // Pins für 74HC165
   pinMode(dataPin_165, INPUT);
   pinMode(clockPin_165, OUTPUT);
   pinMode(latchPin_165, OUTPUT);


  //Seriellen Monitor initialisieren
   Serial.begin(9600);

  //LCD Display initialisieren und Hintergrundbeleuchtung aktivieren
   lcd.init(); 
   lcd.backlight();

 
  // Variablen Definition 
    //Variablen für Displaytext zeilenweise
  
  //Definierte Zustände (sichere Zustände) in Schieberegister für Arduino Start/ Neustart schalten! Eventuell oben in der Defintion der globalen Variablen abweichende Bitstellungen definieren -  falls "0" in allen Ausgängen nicht genügt!
   updateShiftRegister_595();
  //Schiebregister, Ausgänge anschalten  
   digitalWrite(OutEnablePin_595, LOW);


  // Prüfen des Werkzeugzustandes - um sicheren Zustand zu prüfen/definieren ...
   importShiftRegister_165();


  // Zustand Ausgeben an LEDS (grün, wenn alles OK sonst Rot oder Gelb) 




   // Willkommenstext anzeigen
   strcpy(TextZeile[0], "    IFU Stuttgart   ");
   strcpy(TextZeile[1], "  Eckstauchversuch  ");
   strcpy(TextZeile[2], " Clauss & Reichardt ");
   strcpy(TextZeile[3], "  2022 Version 2.0  ");

   //Ausgabe der Zeilen aus den Textvariablen für Zeile 1 bis 4 des LCD Displays
   task_text(TextZeile[0], 0);
   task_text(TextZeile[1], 1);
   task_text(TextZeile[2], 2);
   task_text(TextZeile[3], 3);

  
   //Wartezeit bis Loop gestartet wird
   delay(5000);
 /*    
   //Display leeren hier nur einmalig
   strcpy(TextZeile[0], "Betriebsmodus (BM) am Wahlschalter waehlen");
   strcpy(TextZeile[1], "Den Anweisungen am Display folgen");
   strcpy(TextZeile[2], "                    ");
   strcpy(TextZeile[3], "  2022 Version 2.0  ");
  
   //Ausgabe der Zeilen am Display
   task_text(TextZeile[0], 0);
   task_text(TextZeile[1], 1);
   task_text(TextZeile[2], 2);
   task_text(TextZeile[3], 3);

   //Wartezeit bis Loop gestartet wird
   delay(5000);
    
   //Display leeren hier nur einmalig
   strcpy(TextZeile[0], "Betriebsmodus (BM) am Wahlschalter waehlen");
   strcpy(TextZeile[1], "                    ");
   strcpy(TextZeile[2], "                    ");
   strcpy(TextZeile[3], "                    ");

    //Ausgabe der Zeilen am     Display
   task_text(TextZeile[0], 0);
   task_text(TextZeile[1], 1);
   task_text(TextZeile[2], 2);
   task_text(TextZeile[3], 3);

   delay(5000);
*/   
}




void loop() {
  // put your main code here, to run repeatedly:


//Eingänge und Ausgänge der Schieberegistern aktualisieren
      //Einlesen der Eingänge der 74HC165
      importShiftRegister_165();

      //Senden der Ausgänge an 74HC595
      updateShiftRegister_595();

      //Ausgabe Zustand Schieberegister 74HC165 am seriellen Monitor ausgeben --> Für Wartungszwecke/ Fehlersuche
      //print_InSR165();
      
      
      //Betriebswahlschalter 
      switch (Zustand){
        case 0: //keinen Zustand gefunden ... Ausgangszustand der Initialisierung ... BWS betätigen, um neuen Zustand zu generieren!
          //Ausgabe Display
          strcpy(TextZeile[0], "BM0: Kein Betriebszustand am BWS ausgewaehlt"); //Zeile 1
          strcpy(TextZeile[1], "                    "); //Zeile 2
          strcpy(TextZeile[2], "Hinweis: Laesst sich kein Betriebszustand auswaehlen liegt eine Stoerung in der Elektronik vor!"); //Zeile 3
          strcpy(TextZeile[3], "Auswahl eines Betriebszustands am BWS"); //Zeile 4

        break;


        
    //Betriebsmodus 1

        case 100: //BWS=1
        
          // Prüfen ob Initialbedingungen für den Betriebsmodus am Werkzeug vorliegen (alle Sensoren melden die richtigen Signale ... sonst manueller Betrieb notwendig)
          if (DS_Festo == 1 && RK_HZ_R_unten == 1 && RK_HZ_R_oben == 0 && RK_HZ_L_unten == 1 && RK_HZ_L_oben == 0 && RK_KRV_innen == 0 && RK_KRV_aussen == 1 && RK_WZS_innen == 0 && RK_WZS_aussen == 1) {
              //Variablen für Ausgänge setzen
              MagVen_HZ = 0;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 0;
              Led_Gelb = 0;
              Led_Rot = 1;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM1: Werkzeug in OT fahren und sichern"); // Zeile 1
              strcpy(TextZeile[1], "                    "); // Zeile 2
              strcpy(TextZeile[2], "                    "); // Zeile 3
              strcpy(TextZeile[3], "Mit Taster quittieren"); // Zeile 4
    
              // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
              if (Switch_by_risingEdge_Variable(BT, last_BT) == true) {
                Zustand = 101; 
                next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!    
              }    
              last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
          }
          else {
              //Ausgabe Display
              strcpy(TextZeile[0], "BM1: Werkzeug in OT fahren und sichern"); // Zeile 1
              strcpy(TextZeile[1], "       FEHLER       "); // Zeile 2
              strcpy(TextZeile[2], "Sensorsignal n.i.O. "); // Zeile 3
              strcpy(TextZeile[3], "DS_Festo == 1 & RK_HZ_R_unten == 1 & RK_HZ_R_oben == 0 & RK_HZ_L_unten == 1 & RK_HZ_L_oben == 0 & RK_KRV_innen == 0 & RK_KRV_aussen == 1 & RK_WZS_innen == 0 & RK_WZS_aussen == 1"); // Zeile 4
          }
        break;

        case 101:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 0;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 0;
              Led_Gelb = 1;
              Led_Rot = 1;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM1: Werkzeug in OT fahren und sichern"); // Zeile 1
              strcpy(TextZeile[1], "      ACHTUNG!      "); // Zeile 2
              strcpy(TextZeile[2], "Hubzylinder werden ausgefahren"); // Zeile 3
              strcpy(TextZeile[3], "Mit Taster quittieren"); // Zeile 4

             
          // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
          if (Switch_by_risingEdge_Variable(BT, last_BT) == true) {
            Zustand = 102;
            next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!    
          }   
          last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
        break;

        case 102:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 1;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 0;
              Led_Gelb = 1;
              Led_Rot = 1;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM1: Werkzeug in OT fahren und sichern"); // Zeile 1
              strcpy(TextZeile[1], "Hubzylinder fahren aus"); // Zeile 2
              strcpy(TextZeile[2], "                    "); // Zeile 3
              strcpy(TextZeile[3], "OT Hubzyl. quittieren"); // Zeile 4
          
          // Nächste Zeit für Fehlermeldung festlegen, nur bei positiver Flanke von BT
          if (Switch_by_risingEdge_Variable(BT, last_BT) == true && RK_HZ_R_oben == 1 && RK_HZ_L_oben == 1 && RK_WZS_aussen == 1){
            Zustand = 103;
            next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!                
            }
          //Falls Bedingungen nicht erfüllt sind und so lange BT gehalten wird, wird eine Fehlermeldung ausgegeben
          else if(millis() > next_failure_time && BT == 1 && (RK_HZ_R_oben == 0 || RK_HZ_L_oben == 0)){
                //Ausgabe Display
                strcpy(TextZeile[0], "BM1: Werkzeug in OT fahren und sichern"); // Zeile 1
                strcpy(TextZeile[1], "       FEHLER       "); // Zeile 2
                strcpy(TextZeile[2], "Sensorsignal n.i.O. "); // Zeile 3
                strcpy(TextZeile[3], "RK_HZ_R_oben & RK_HZ_L_oben & RK_WZS_aussen pruefen"); // Zeile 4
            }
          
          last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
        break;

        case 103:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 1;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 0;
              Led_Gelb = 0;
              Led_Rot = 1;

              //Ausgabe Display
              strcpy(TextZeile[0], "BM1: Werkzeug in OT fahren und sichern"); // Zeile 1
              strcpy(TextZeile[1], "                    "); // Zeile 2
              strcpy(TextZeile[2], "Sicherung einschwenken"); // Zeile 3
              strcpy(TextZeile[3], "Mit Taster quittieren"); // Zeile 4
          
          // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
          if (Switch_by_risingEdge_Variable(BT, last_BT) == true && RK_WZS_innen == 1) {
            Zustand = 104;
            next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!                
           }
          //Falls Bedingungen nicht erfüllt sind und so lange BT gehalten wird, wird eine Fehlermeldung ausgegeben
          else if(millis() > next_failure_time && BT == 1 && RK_WZS_innen == 0){
              //Ausgabe Display
              strcpy(TextZeile[0], "BM1: Werkzeug in OT fahren und sichern"); // Zeile 1
              strcpy(TextZeile[1], "       FEHLER       "); // Zeile 2
              strcpy(TextZeile[2], "Sensorsignal n.i.O. "); // Zeile 3
              strcpy(TextZeile[3], "RK_WZS_innen pruefen"); // Zeile 4
          }   
          last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
        break;

        case 104:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 1;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 0;
              Led_Gelb = 1;
              Led_Rot = 0;

              //Ausgabe Display
              strcpy(TextZeile[0], "BM1: Werkzeug in OT fahren und sichern"); // Zeile 1
              strcpy(TextZeile[1], "      ACHTUNG!      "); // Zeile 2
              strcpy(TextZeile[2], "Hubzylinder absenken?"); // Zeile 3
              strcpy(TextZeile[3], "Absenken HZ quittieren"); // Zeile 4

          // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
          if (Switch_by_risingEdge_Variable(BT, last_BT) == true && RK_WZS_innen == 1) {
            Zustand = 105;
            next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!                
          }
          //Falls Bedingungen nicht erfüllt sind und so lange BT gehalten wird, wird eine Fehlermeldung ausgegeben
          else if(millis() > next_failure_time && BT == 1 && RK_WZS_innen == 0){
              //Ausgabe Display
              strcpy(TextZeile[0], "BM1: Werkzeug in OT fahren und sichern"); // Zeile 1
              strcpy(TextZeile[1], "       FEHLER       "); // Zeile 2
              strcpy(TextZeile[2], "Sensorsignal n.i.O. "); // Zeile 3
              strcpy(TextZeile[3], "RK_WZS_innen pruefen "); // Zeile 4
          }   
          last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
        break;

        case 105:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 0;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 1;
              Led_Gelb = 0;
              Led_Rot = 0;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM1: Werkzeug in OT fahren und sichern"); // Zeile 1
              strcpy(TextZeile[1], "Hubzylinder senken ab"); // Zeile 2
              strcpy(TextZeile[2], "                    "); // Zeile 3
              strcpy(TextZeile[3], "UT Hubzyl. quittieren"); // Zeile 4

          // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
          if (Switch_by_risingEdge_Variable(BT, last_BT) == true && RK_HZ_R_unten == 1 && RK_HZ_L_unten == 1) {
            Zustand = 106;
            next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!                            
          }
          //Falls Bedingungen nicht erfüllt sind und so lange BT gehalten wird, wird eine Fehlermeldung ausgegeben
          else if(millis() > next_failure_time && BT == 1 && (RK_HZ_R_unten == 0 || RK_HZ_L_unten == 0)){
              //Ausgabe Display
              strcpy(TextZeile[0], "BM1: Werkzeug in OT fahren und sichern"); // Zeile 1
              strcpy(TextZeile[1], "       FEHLER       "); // Zeile 2
              strcpy(TextZeile[2], "Sensorsignal n.i.O. "); // Zeile 3
              strcpy(TextZeile[3], "RK_HZ_R_unten & RK_HZ_L_unten pruefen"); // Zeile 4
          }     
          last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
        break;

        case 106:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 0;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 1;
              Led_Gelb = 0;
              Led_Rot = 0;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM1: Werkzeug in OT fahren und sichern"); // Zeile 1
              strcpy(TextZeile[1], "Hubzyl. sind in UT  "); // Zeile 2
              strcpy(TextZeile[2], "                    "); // Zeile 3
              strcpy(TextZeile[3], "Betriebsmodus wechseln -> BM2"); // Zeile 4

              // Kein nachfolgender Zustand sondern neuer Betriebsmodus - daher hier keine Zuweisung der Zustand/Case Variable!
         break;



        //Betriebsmodus 2
        case 200:  //BWS=2
          // Prüfen on Initialbedingungen für den Betriebsmodus am Werkzeug vorliegen (alle Sensoren melden die richtigen Signale ... sonst manueller Betrieb notwendig)
          if (DS_Festo == 1 && RK_HZ_R_unten == 1 && RK_HZ_R_oben == 0 && RK_HZ_L_unten == 1 && RK_HZ_L_oben == 0 && RK_KRV_innen == 0 && RK_KRV_aussen == 1 && RK_WZS_innen == 1 && RK_WZS_aussen == 0) {
              //Variablen für Ausgänge setzen
              MagVen_HZ = 0;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 1;
              Led_Gelb = 0;
              Led_Rot = 0;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM2: Probe einlegen "); //Zeile 1
              strcpy(TextZeile[1], "                    "); // Zeile 2
              strcpy(TextZeile[2], "                    "); // Zeile 3
              strcpy(TextZeile[3], "Mit Taster quittieren"); //Zeile 4
    
              // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
              if (Switch_by_risingEdge_Variable(BT, last_BT) == true && RK_WZS_innen == 1) {
                Zustand = 201;
                next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!                            
              }
              last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
          }
          else {
              //Ausgabe Display
              strcpy(TextZeile[0], "BM2: Probe einlegen "); // Zeile 1
              strcpy(TextZeile[1], "       FEHLER       "); // Zeile 2
              strcpy(TextZeile[2], "Sensorsignal n.i.O. "); // Zeile 3
              strcpy(TextZeile[3], "DS_Festo == 1 & RK_HZ_R_unten == 1 & RK_HZ_R_oben == 0 & RK_HZ_L_unten == 1 & RK_HZ_L_oben == 0 & RK_KRV_innen == 0 & RK_KRV_aussen == 1 & RK_WZS_innen == 1 & RK_WZS_aussen == 0"); // Zeile 4
          }
        break;

        case 201:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 0;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 1;
              Led_Gelb = 0;
              Led_Rot = 0;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM2: Probe einlegen "); // Zeile 1
              strcpy(TextZeile[1], "Arbeitsraum mit Handrad vergroessern"); // Zeile 2
              strcpy(TextZeile[2], "Probe einlegen und nur unten einspannen!"); // Zeile 3
              strcpy(TextZeile[3], "Abschluss der Arbeitsschritte mit Taster quittieren"); // Zeile 4

             
          // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
          if (Switch_by_risingEdge_Variable(BT, last_BT) == true && RK_WZS_innen == 1) {
            Zustand = 202; 
            next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!                            
          }
          //Falls Bedingungen nicht erfüllt sind und so lange BT gehalten wird, wird eine Fehlermeldung ausgegeben
          else if(millis() > next_failure_time && BT == 1 && RK_WZS_innen == 0){
              //Ausgabe Display
              strcpy(TextZeile[0], "BM2: Probe einlegen "); // Zeile 1
              strcpy(TextZeile[1], "       FEHLER       "); // Zeile 2
              strcpy(TextZeile[2], "Sensorsignal n.i.O. "); // Zeile 3
              strcpy(TextZeile[3], "RK_WZS_innen pruefen"); // Zeile 4
          }
          last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
        break;

        case 202:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 0;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 1;
              Led_Gelb = 0;
              Led_Rot = 0;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM2: Probe einlegen "); // Zeile 1
              strcpy(TextZeile[1], "                    "); // Zeile 2
              strcpy(TextZeile[2], "Mit Handrad Oberwerkzeug auf Probe absenken"); // Zeile 3
              strcpy(TextZeile[3], "Abschluss des Arbeitsschritts mit Taster quittieren"); // Zeile 4

             
          // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
          if (Switch_by_risingEdge_Variable(BT, last_BT) == true && RK_WZS_innen == 1) {
            Zustand = 203; 
            next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!                            
          }
          //Falls Bedingungen nicht erfüllt sind und so lange BT gehalten wird, wird eine Fehlermeldung ausgegeben
          else if(millis() > next_failure_time && BT == 1 && RK_WZS_innen == 0){
              //Ausgabe Display
              strcpy(TextZeile[0], "BM2: Probe einlegen "); // Zeile 1
              strcpy(TextZeile[1], "       FEHLER       "); // Zeile 2
              strcpy(TextZeile[2], "Sensorsignal n.i.O. "); // Zeile 3
              strcpy(TextZeile[3], "RK_WZS_innen pruefen"); // Zeile 4
          }
          last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
        break;

        case 203:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 0;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 1;
              Led_Gelb = 0;
              Led_Rot = 0;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM2: Probe einlegen "); // Zeile 1
              strcpy(TextZeile[1], "                    "); // Zeile 2
              strcpy(TextZeile[2], "Probe oben einspannen"); // Zeile 3
              strcpy(TextZeile[3], "Abschluss des Arbeitsschritts mit Taster quittieren"); // Zeile 4

             
          // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
          if (Switch_by_risingEdge_Variable(BT, last_BT) == true && RK_WZS_innen == 1) {
            Zustand = 204;
            next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!                            
          }
          //Falls Bedingungen nicht erfüllt sind und so lange BT gehalten wird, wird eine Fehlermeldung ausgegeben
          else if(millis() > next_failure_time && BT == 1 && RK_WZS_innen == 0){
              //Ausgabe Display
              strcpy(TextZeile[0], "BM2: Probe einlegen "); // Zeile 1
              strcpy(TextZeile[1], "       FEHLER       "); // Zeile 2
              strcpy(TextZeile[2], "Sensorsignal n.i.O. "); // Zeile 3
              strcpy(TextZeile[3], "RK_WZS_innen pruefen"); // Zeile 4
          }
          last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
        break;

        case 204:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 0;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 0;
              Led_Gelb = 0;
              Led_Rot = 1;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM2: Probe einlegen "); // Zeile 1
              strcpy(TextZeile[1], "Sicherung mit Handrad entlasten"); // Zeile 2
              strcpy(TextZeile[2], "Sicherung ausschwenken"); // Zeile 3
              strcpy(TextZeile[3], "Mit Taster quittieren"); // Zeile 4

             
          // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
          if (Switch_by_risingEdge_Variable(BT, last_BT) == true && RK_WZS_aussen == 1) {
            Zustand = 205;
            next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!           
          }
          //Falls Bedingungen nicht erfüllt sind und so lange BT gehalten wird, wird eine Fehlermeldung ausgegeben
          else if(millis() > next_failure_time && BT == 1 && RK_WZS_aussen == 0){
              //Ausgabe Display
              strcpy(TextZeile[0], "BM2: Probe einlegen "); // Zeile 1
              strcpy(TextZeile[1], "       FEHLER       "); // Zeile 2
              strcpy(TextZeile[2], "Sensorsignal n.i.O. "); // Zeile 3
              strcpy(TextZeile[3], "RK_WZS_aussen pruefen"); // Zeile 4
          }
          last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
        break;

        case 205:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 0;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 0;
              Led_Gelb = 0;
              Led_Rot = 1;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM2: Probe einlegen "); // Zeile 1
              strcpy(TextZeile[1], "                    "); // Zeile 2
              strcpy(TextZeile[2], "Probe ist eingespannt"); // Zeile 3
              strcpy(TextZeile[3], "Betriebsmodus wechseln -> BM3"); // Zeile 4

              // Kein nachfolgender Zustand sondern neuer Betriebsmodus - daher hier keine Zuweisung der Zustand/Case Variable!
        break;

        //Betriebsmodus 3
        case 300:  //BWS=3
          // Prüfen on Initialbedingungen für den Betriebsmodus am Werkzeug vorliegen (alle Sensoren melden die richtigen Signale ... sonst manueller Betrieb notwendig)
          if (DS_Festo == 1 && RK_HZ_R_unten == 1 && RK_HZ_R_oben == 0 && RK_HZ_L_unten == 1 && RK_HZ_L_oben == 0 && RK_KRV_innen == 0 && RK_KRV_aussen == 1 && RK_WZS_innen == 0 && RK_WZS_aussen == 1) {
              //Variablen für Ausgänge setzen
              MagVen_HZ = 0;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 0;
              Led_Gelb = 0;
              Led_Rot = 1;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM3: KRV in Position fahren"); //Zeile 1
              strcpy(TextZeile[1], "                    "); // Zeile 2
              strcpy(TextZeile[2], "                    "); // Zeile 3
              strcpy(TextZeile[3], "Mit Taster quittieren"); //Zeile 4

             // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
              if (Switch_by_risingEdge_Variable(BT, last_BT) == true) {
                Zustand = 301;
                next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!            
              }    
              last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
          }
          else {
              //Ausgabe Display
              strcpy(TextZeile[0], "BM3: KRV in Position fahren"); // Zeile 1
              strcpy(TextZeile[1], "       FEHLER       "); // Zeile 2
              strcpy(TextZeile[2], "Sensorsignal n.i.O. "); // Zeile 3
              strcpy(TextZeile[3], "DS_Festo == 1 && RK_HZ_R_unten == 1 && RK_HZ_R_oben == 0 && RK_HZ_L_unten == 1 && RK_HZ_L_oben == 0 && RK_KRV_innen == 0 && RK_KRV_aussen == 1 && RK_WZS_innen == 0 && RK_WZS_aussen == 1"); // Zeile 4
          }    
        break;

        case 301:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 0;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 0;
              Led_Gelb = 0;
              Led_Rot = 1;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM3: KRV in Position fahren"); //Zeile 1
              strcpy(TextZeile[1], "                    "); // Zeile 2
              strcpy(TextZeile[2], "Spindel KRV bei Erstversuch zurueckstellen"); // Zeile 3
              strcpy(TextZeile[3], "Zurueckstellen quittieren"); //Zeile 4

             
          // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
          if (Switch_by_risingEdge_Variable(BT, last_BT) == true) {
            Zustand = 302;
            next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!            
          }   
          last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
        break;

        case 302:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 0;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 0;
              Led_Gelb = 1;
              Led_Rot = 1;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM3: KRV in Position fahren"); //Zeile 1
              strcpy(TextZeile[1], "      ACHTUNG!      "); // Zeile 2
              strcpy(TextZeile[2], "KRV einfahren? Erst Druck nach aussen, dann rein!"); // Zeile 3
              strcpy(TextZeile[3], "Einfahren KRV quittieren"); //Zeile 4

             
          // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
          if (Switch_by_risingEdge_Variable(BT, last_BT) == true && RK_KRV_aussen == 1) {
            Zustand = 303;
            next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!
            nextDaempfungsende = millis() + Druckaufbauzeit_Daempfung_KRV_rein; //Zeitpunkt der Endzeit der Dämpfung zum KRV einfahren festlegen!
          }
          //Falls Bedingungen nicht erfüllt sind und so lange BT gehalten wird, wird eine Fehlermeldung ausgegeben
          else if(millis() > next_failure_time && BT == 1 && RK_KRV_aussen == 0){
              //Ausgabe Display
              strcpy(TextZeile[0], "BM3: KRV in Position fahren"); // Zeile 1
              strcpy(TextZeile[1], "       FEHLER       "); // Zeile 2
              strcpy(TextZeile[2], "Sensorsignal n.i.O. "); // Zeile 3
              strcpy(TextZeile[3], "RK_KRV_aussen pruefen"); // Zeile 4
          }   
          last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
        break;

        case 303:
              //Variablen für Ausgänge setzen
              
              MagVen_HZ = 0;
              
              if (millis() <= nextDaempfungsende){
                MagVen_KRV_rein = 0;
                MagVen_KRV_raus = 1;
              }
              else {
                MagVen_KRV_rein = 1;
                MagVen_KRV_raus = 0;
              }
              
              Led_Gruen = 0;
              Led_Gelb = 1;
              Led_Rot = 1;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM3: KRV in Position fahren"); //Zeile 1
              strcpy(TextZeile[1], "KRV faehrt nach innen"); // Zeile 2
              strcpy(TextZeile[2], "                    "); // Zeile 3
              strcpy(TextZeile[3], "Eingerastetet Position quittieren"); //Zeile 4

             
          // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
          if (Switch_by_risingEdge_Variable(BT, last_BT) == true && RK_KRV_innen == 1) {
            Zustand = 304;
            next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!            
          }
          //Falls Bedingungen nicht erfüllt sind und so lange BT gehalten wird, wird eine Fehlermeldung ausgegeben
          else if(millis() > next_failure_time && BT == 1 && RK_KRV_innen == 0){
              //Ausgabe Display
              strcpy(TextZeile[0], "BM3: KRV in Position fahren"); // Zeile 1
              strcpy(TextZeile[1], "       FEHLER       "); // Zeile 2
              strcpy(TextZeile[2], "Sensorsignal n.i.O. "); // Zeile 3
              strcpy(TextZeile[3], "RK_KRV_innen pruefen"); // Zeile 4
          }
          last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
        break;

        case 304:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 0;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 0;
              Led_Gelb = 0;
              Led_Rot = 1;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM3: KRV in Position fahren"); //Zeile 1
              strcpy(TextZeile[1], "Pneumatik am KRV abgeschaltet"); // Zeile 2
              strcpy(TextZeile[2], "                    "); // Zeile 3
              strcpy(TextZeile[3], "Mit Taster quittieren"); //Zeile 4

             
          // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
          if (Switch_by_risingEdge_Variable(BT, last_BT) == true && RK_KRV_innen == 1) {
            Zustand = 305;
            next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!            
          }
          //Falls Bedingungen nicht erfüllt sind und so lange BT gehalten wird, wird eine Fehlermeldung ausgegeben
          else if(millis() > next_failure_time && BT == 1 && RK_KRV_innen == 0){
              //Ausgabe Display
              strcpy(TextZeile[0], "BM3: KRV in Position fahren"); // Zeile 1
              strcpy(TextZeile[1], "       FEHLER       "); // Zeile 2
              strcpy(TextZeile[2], "Sensorsignal n.i.O. "); // Zeile 3
              strcpy(TextZeile[3], "RK_KRV_innen pruefen"); // Zeile 4
          }
          last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
        break;

        case 305:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 0;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 0;
              Led_Gelb = 0;
              Led_Rot = 1;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM3: KRV in Position fahren"); //Zeile 1
              strcpy(TextZeile[1], "                    "); // Zeile 2
              strcpy(TextZeile[2], "Ggf. Andrueckkraft KRV einstellen"); // Zeile 3
              strcpy(TextZeile[3], "Mit Taster quittieren"); //Zeile 4

             
          // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
          if (Switch_by_risingEdge_Variable(BT, last_BT) == true && RK_KRV_innen == 1) {
            Zustand = 306;
            next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!            
          }
          //Falls Bedingungen nicht erfüllt sind und so lange BT gehalten wird, wird eine Fehlermeldung ausgegeben
          else if(millis() > next_failure_time && BT == 1 && RK_KRV_innen == 0){
              //Ausgabe Display
              strcpy(TextZeile[0], "BM3: KRV in Position fahren"); // Zeile 1
              strcpy(TextZeile[1], "       FEHLER       "); // Zeile 2
              strcpy(TextZeile[2], "Sensorsignal n.i.O. "); // Zeile 3
              strcpy(TextZeile[3], "RK_KRV_innen pruefen"); // Zeile 4
          }
          last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
        break;

        case 306:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 0;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 0;
              Led_Gelb = 0;
              Led_Rot = 1;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM3: KRV in Position fahren"); //Zeile 1
              strcpy(TextZeile[1], "                    "); // Zeile 2
              strcpy(TextZeile[2], "KRV in Position     "); // Zeile 3
              strcpy(TextZeile[3], "Betriebsmodus wechseln -> BM4"); //Zeile 4

        // Kein nachfolgender Zustand sondern neuer Betriebsmodus - daher hier keine Zuweisung der Zustand/Case Variable!
        break;


        case 400:  //BWS=4
          // Prüfen on Initialbedingungen für den Betriebsmodus am Werkzeug vorliegen (alle Sensoren melden die richtigen Signale ... sonst manueller Betrieb notwendig)
          if (DS_Festo == 1 && RK_HZ_R_unten == 1 && RK_HZ_R_oben == 0 && RK_HZ_L_unten == 1 && RK_HZ_L_oben == 0 && RK_KRV_innen == 1 && RK_KRV_aussen == 0 && RK_WZS_innen == 0 && RK_WZS_aussen == 1) {
              //Variablen für Ausgänge setzen
              MagVen_HZ = 0;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 0;
              Led_Gelb = 0;
              Led_Rot = 1;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM4: Versuch durchfuehren"); //Zeile 1
              strcpy(TextZeile[1], "                    "); // Zeile 2
              strcpy(TextZeile[2], "                    "); // Zeile 3
              strcpy(TextZeile[3], "Mit Taster quittieren"); //Zeile 4

             // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
          if (Switch_by_risingEdge_Variable(BT, last_BT) == true) {
              Zustand = 401;
              next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!            
          }
          last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
          }
          else {
              //Ausgabe Display
              strcpy(TextZeile[0], "BM4: Versuch durchfuehren"); // Zeile 1
              strcpy(TextZeile[1], "       FEHLER       "); // Zeile 2
              strcpy(TextZeile[2], "Sensorsignal n.i.O. "); // Zeile 3
              strcpy(TextZeile[3], "DS_Festo == 1 & RK_HZ_R_unten == 1 & RK_HZ_R_oben == 0 & RK_HZ_L_unten == 1 & RK_HZ_L_oben == 0 & RK_KRV_innen == 1 & RK_KRV_aussen == 0 & RK_WZS_innen == 0 & RK_WZS_aussen == 1"); // Zeile 4
          }    

        break;

        case 401:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 0;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 0;
              Led_Gelb = 1;
              Led_Rot = 1;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM4: Versuch durchfuehren"); //Zeile 1
              strcpy(TextZeile[1], "      ACHTUNG!      "); // Zeile 2
              strcpy(TextZeile[2], "Druckaufbau zum Vorspannen des KRV?"); // Zeile 3
              strcpy(TextZeile[3], "Mit Taster quittieren"); //Zeile 4

             
          // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
          if (Switch_by_risingEdge_Variable(BT, last_BT) == true && RK_KRV_innen == 1) {
            Zustand = 402;
            next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!            
          }
          //Falls Bedingungen nicht erfüllt sind und so lange BT gehalten wird, wird eine Fehlermeldung ausgegeben
          else if(millis() > next_failure_time && BT == 1 && RK_KRV_innen == 0){
              //Ausgabe Display
              strcpy(TextZeile[0], "BM4: Versuch durchfuehren"); // Zeile 1
              strcpy(TextZeile[1], "       FEHLER       "); // Zeile 2
              strcpy(TextZeile[2], "Sensorsignal n.i.O. "); // Zeile 3
              strcpy(TextZeile[3], "RK_KRV_innen pruefen"); // Zeile 4
          }   
          last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
        break;

        case 402:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 0;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 1;              
              Led_Gruen = 0;
              Led_Gelb = 1;
              Led_Rot = 1;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM4: Versuch durchfuehren"); //Zeile 1
              strcpy(TextZeile[1], "                    "); // Zeile 2
              strcpy(TextZeile[2], "Vorspannung KRV nach aussen aktiv"); // Zeile 3
              strcpy(TextZeile[3], "Mit Taster quittieren"); //Zeile 4

             
          // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
          if (Switch_by_risingEdge_Variable(BT, last_BT) == true && DS_Festo == 0 && RK_KRV_innen == 1 ) {
            Zustand = 403;
            next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!            
          }
          //Falls Bedingungen nicht erfüllt sind und so lange BT gehalten wird, wird eine Fehlermeldung ausgegeben
          else if(millis() > next_failure_time && BT == 1 && (DS_Festo == 1 || RK_KRV_innen == 0)){
              //Ausgabe Display
              strcpy(TextZeile[0], "BM4: Versuch durchfuehren"); // Zeile 1
              strcpy(TextZeile[1], "       FEHLER       "); // Zeile 2
              strcpy(TextZeile[2], "Sensorsignal n.i.O. "); // Zeile 3
              strcpy(TextZeile[3], "DS_Festo & RK_KRV_innen pruefen"); // Zeile 4
          }
          last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
        break;

        case 403:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 0;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 1;              
              Led_Gruen = 0;
              Led_Gelb = 1;
              Led_Rot = 1;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM4: Versuch durchfuehren"); //Zeile 1
              strcpy(TextZeile[1], "Versuchsbereit      "); // Zeile 2
              strcpy(TextZeile[2], "Versuch durchfuehren"); // Zeile 3
              strcpy(TextZeile[3], "Versuchsende mit Taster quittieren"); //Zeile 4

             
          // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
          if (Switch_by_risingEdge_Variable(BT, last_BT) == true && DS_Festo == 0 && RK_KRV_aussen == 1 ) {
            Zustand = 404;
            next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!            
          }
          //Falls Bedingungen nicht erfüllt sind und so lange BT gehalten wird, wird eine Fehlermeldung ausgegeben
          else if(millis() > next_failure_time && BT == 1 && (DS_Festo == 1 || RK_KRV_aussen == 0)){
              //Ausgabe Display
              strcpy(TextZeile[0], "BM4: Versuch durchfuehren"); // Zeile 1
              strcpy(TextZeile[1], "       FEHLER       "); // Zeile 2
              strcpy(TextZeile[2], "Sensorsignal n.i.O. "); // Zeile 3
              strcpy(TextZeile[3], "DS_Festo & RK_KRV_aussen pruefen"); // Zeile 4
          }          
          last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
        break;

        case 404:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 0;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 1;              
              Led_Gruen = 0;
              Led_Gelb = 0;
              Led_Rot = 1;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM4: Versuch durchfuehren"); //Zeile 1
              strcpy(TextZeile[1], "                    "); // Zeile 2
              strcpy(TextZeile[2], "Pneumatik am KRV abschalten?"); // Zeile 3
              strcpy(TextZeile[3], "Mit Taster quittieren"); //Zeile 4

             
          // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
          if (Switch_by_risingEdge_Variable(BT, last_BT) == true && DS_Festo == 0 && RK_KRV_aussen == 1 ) {
            Zustand = 405;
            next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!            
          }
          //Falls Bedingungen nicht erfüllt sind und so lange BT gehalten wird, wird eine Fehlermeldung ausgegeben
          else if(millis() > next_failure_time && BT == 1 && (DS_Festo == 1 || RK_KRV_aussen == 0)){
              //Ausgabe Display
              strcpy(TextZeile[0], "BM4: Versuch durchfuehren"); // Zeile 1
              strcpy(TextZeile[1], "       FEHLER       "); // Zeile 2
              strcpy(TextZeile[2], "Sensorsignal n.i.O. "); // Zeile 3
              strcpy(TextZeile[3], "DS_Festo & RK_KRV_aussen pruefen"); // Zeile 4
          }
          last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
        break;

        
// Prüfen ob Druck weg ist, sonst gehts nicht weiter
        case 405:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 0;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 0;
              Led_Gelb = 1;
              Led_Rot = 1;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM4: Versuch durchfuehren"); //Zeile 1
              strcpy(TextZeile[1], "Pneumatik am KRV ist abgeschaltet"); // Zeile 2
              strcpy(TextZeile[2], "Druckabbau dauert ca. 1 min"); // Zeile 3
              strcpy(TextZeile[3], "Mit Taster quittieren"); //Zeile 4

          // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
          if (Switch_by_risingEdge_Variable(BT, last_BT) == true && DS_Festo == 1 && RK_KRV_aussen == 1 ) {
            Zustand = 406;
            next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!            
          }
          //Falls Bedingungen nicht erfüllt sind und so lange BT gehalten wird, wird eine Fehlermeldung ausgegeben
          else if(millis() > next_failure_time && BT == 1 && RK_KRV_aussen == 0){
              //Ausgabe Display
              strcpy(TextZeile[0], "BM4: Versuch durchfuehren"); // Zeile 1
              strcpy(TextZeile[1], "       FEHLER       "); // Zeile 2
              strcpy(TextZeile[2], "Sensorsignal n.i.O. "); // Zeile 3
              strcpy(TextZeile[3], "RK_KRV_aussen pruefen"); // Zeile 4
          }             
          last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
        break;
        
       case 406:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 0;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 0;
              Led_Gelb = 0;
              Led_Rot = 1;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM4: Versuch durchfuehren"); //Zeile 1
              strcpy(TextZeile[1], "                    "); // Zeile 2
              strcpy(TextZeile[2], "Versuch beendet!    "); // Zeile 3
              strcpy(TextZeile[3], "Betriebsmodus wechseln -> BM5"); //Zeile 4

        // Kein nachfolgender Zustand sondern neuer Betriebsmodus - daher hier keine Zuweisung der Zustand/Case Variable!
        break;

        case 500:  //BWS=5
          // Prüfen on Initialbedingungen für den Betriebsmodus am Werkzeug vorliegen (alle Sensoren melden die richtigen Signale ... sonst manueller Betrieb notwendig)
          if (DS_Festo == 1 && RK_HZ_R_unten == 1 && RK_HZ_R_oben == 0 && RK_HZ_L_unten == 1 && RK_HZ_L_oben == 0 && RK_KRV_innen == 0 && RK_KRV_aussen == 1 && RK_WZS_innen == 0 && RK_WZS_aussen == 1) {
              //Variablen für Ausgänge setzen
              MagVen_HZ = 0;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 0;
              Led_Gelb = 0;
              Led_Rot = 1;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM5: Probenentnahme "); //Zeile 1
              strcpy(TextZeile[1], "                    "); // Zeile 2
              strcpy(TextZeile[2], "                    "); // Zeile 3
              strcpy(TextZeile[3], "Mit Taster quittieren"); //Zeile 4

             // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
              if (Switch_by_risingEdge_Variable(BT, last_BT) == true) {
                Zustand = 501;
                next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!            
              }    
              last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
          }
          else {
              //Ausgabe Display
              strcpy(TextZeile[0], "BM5: Probenentnahme "); // Zeile 1
              strcpy(TextZeile[1], "       FEHLER       "); // Zeile 2
              strcpy(TextZeile[2], "Sensorsignal n.i.O. "); // Zeile 3
              strcpy(TextZeile[3], "DS_Festo == 1 & RK_HZ_R_unten == 1 & RK_HZ_R_oben == 0 & RK_HZ_L_unten == 1 & RK_HZ_L_oben == 0 & RK_KRV_innen == 0 & RK_KRV_aussen == 1 & RK_WZS_innen == 0 & RK_WZS_aussen == 1"); // Zeile 4
          }    

        break;
        
        case 501:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 0;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 0;
              Led_Gelb = 0;
              Led_Rot = 1;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM5: Probenentnahme "); //Zeile 1
              strcpy(TextZeile[1], "                    "); // Zeile 2
              strcpy(TextZeile[2], "Probe oben loesen   "); // Zeile 3
              strcpy(TextZeile[3], "Geloeste Probe oben quittieren"); //Zeile 4

             
          // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
          if (Switch_by_risingEdge_Variable(BT, last_BT) == true) {
            Zustand = 502;
            next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!            
          }   
          last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
        break;   

        case 502:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 0;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 0;
              Led_Gelb = 1;
              Led_Rot = 1;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM5: Probenentnahme "); //Zeile 1
              strcpy(TextZeile[1], "                    "); // Zeile 2
              strcpy(TextZeile[2], "Presse in OT fahren "); // Zeile 3
              strcpy(TextZeile[3], "OT Presse quittieren"); //Zeile 4

             
          // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
          if (Switch_by_risingEdge_Variable(BT, last_BT) == true) {
            Zustand = 503;
            next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!            
          }   
          last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
        break; 
        
        case 503:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 0;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 0;
              Led_Gelb = 1;
              Led_Rot = 1;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM5: Probenentnahme "); //Zeile 1
              strcpy(TextZeile[1], "      ACHTUNG!      "); // Zeile 2
              strcpy(TextZeile[2], "Oberwerkzeug heben? "); // Zeile 3
              strcpy(TextZeile[3], "Mit Taster quittieren"); //Zeile 4

             
          // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
          if (Switch_by_risingEdge_Variable(BT, last_BT) == true) {
            Zustand = 504;
            next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!            
          }   
          last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
        break;   

        case 504:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 1;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 0;
              Led_Gelb = 1;
              Led_Rot = 1;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM5: Probenentnahme "); //Zeile 1
              strcpy(TextZeile[1], "Hubzylinder fahren aus"); // Zeile 2
              strcpy(TextZeile[2], "                    "); // Zeile 3
              strcpy(TextZeile[3], "OT Hubzyl. quittieren"); //Zeile 4

             
          // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
          if (Switch_by_risingEdge_Variable(BT, last_BT) == true && RK_HZ_R_oben == 1 && RK_HZ_L_oben == 1) {
            Zustand = 505;
            next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!            
          }
          //Falls Bedingungen nicht erfüllt sind und so lange BT gehalten wird, wird eine Fehlermeldung ausgegeben
          else if(millis() > next_failure_time && BT == 1 && ( RK_HZ_R_oben == 0 || RK_HZ_L_oben == 0)){
              //Ausgabe Display
              strcpy(TextZeile[0], "BM5: Probenentnahme "); // Zeile 1
              strcpy(TextZeile[1], "       FEHLER       "); // Zeile 2
              strcpy(TextZeile[2], "Sensorsignal n.i.O. "); // Zeile 3
              strcpy(TextZeile[3], "RK_HZ_R_oben & RK_HZ_L_oben pruefen"); // Zeile 4
          }
          last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
        break; 

        case 505:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 1;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 1;
              Led_Gelb = 0;
              Led_Rot = 0;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM5: Probenentnahme "); //Zeile 1
              strcpy(TextZeile[1], "                    "); // Zeile 2
              strcpy(TextZeile[2], "Sicherung einschwenken!"); // Zeile 3
              strcpy(TextZeile[3], "Mit Taster quittieren"); //Zeile 4

             
          // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
          if (Switch_by_risingEdge_Variable(BT, last_BT) == true && RK_HZ_R_oben == 1 && RK_HZ_L_oben == 1 && RK_WZS_innen == 1) {
            Zustand = 506;
            next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!            
          }
          //Falls Bedingungen nicht erfüllt sind und so lange BT gehalten wird, wird eine Fehlermeldung ausgegeben
          else if(millis() > next_failure_time && BT == 1 && (RK_HZ_R_oben == 0 || RK_HZ_L_oben == 0 || RK_WZS_innen == 0)){
              //Ausgabe Display
              strcpy(TextZeile[0], "BM5: Probenentnahme "); // Zeile 1
              strcpy(TextZeile[1], "       FEHLER       "); // Zeile 2
              strcpy(TextZeile[2], "Sensorsignal n.i.O. "); // Zeile 3
              strcpy(TextZeile[3], "RK_HZ_R_oben, RK_HZ_L_oben & RK_WZS_innen pruefen"); // Zeile 4
          }
          last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
        break;

        case 506:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 1;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 1;
              Led_Gelb = 0;
              Led_Rot = 0;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM5: Probenentnahme "); //Zeile 1
              strcpy(TextZeile[1], "                    "); // Zeile 2
              strcpy(TextZeile[2], "Mit Handrad Sicherung belasten"); // Zeile 3
              strcpy(TextZeile[3], "Mit Taster quittieren"); //Zeile 4

             
          // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
          if (Switch_by_risingEdge_Variable(BT, last_BT) == true && RK_HZ_R_oben == 1 && RK_HZ_L_oben == 1 && RK_WZS_innen == 1) {
            Zustand = 507;
            next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!            
          }
          //Falls Bedingungen nicht erfüllt sind und so lange BT gehalten wird, wird eine Fehlermeldung ausgegeben
          else if(millis() > next_failure_time && BT == 1 && (RK_HZ_R_oben == 0 || RK_HZ_L_oben == 0 || RK_WZS_innen == 0)){
              //Ausgabe Display
              strcpy(TextZeile[0], "BM5: Probenentnahme "); // Zeile 1
              strcpy(TextZeile[1], "       FEHLER       "); // Zeile 2
              strcpy(TextZeile[2], "Sensorsignal n.i.O. "); // Zeile 3
              strcpy(TextZeile[3], "RK_HZ_R_oben, RK_HZ_L_oben & RK_WZS_innen pruefen"); // Zeile 4
          }         
          last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
        break;

        case 507:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 1;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 1;
              Led_Gelb = 1;
              Led_Rot = 0;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM5: Probenentnahme "); //Zeile 1
              strcpy(TextZeile[1], "      ACHTUNG!      "); // Zeile 2
              strcpy(TextZeile[2], "Hubzylinder absenken?"); // Zeile 3
              strcpy(TextZeile[3], "Mit Taster quittieren"); //Zeile 4

             
          // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
          if (Switch_by_risingEdge_Variable(BT, last_BT) == true && RK_HZ_R_oben == 1 && RK_HZ_L_oben ==1 && RK_WZS_innen == 1) {
            Zustand = 508;
            next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!            
          }
          //Falls Bedingungen nicht erfüllt sind und so lange BT gehalten wird, wird eine Fehlermeldung ausgegeben
          else if(millis() > next_failure_time && BT == 1 && (RK_HZ_R_oben == 0 || RK_HZ_L_oben == 0 || RK_WZS_innen == 0)){
              //Ausgabe Display
              strcpy(TextZeile[0], "BM5: Probenentnahme "); // Zeile 1
              strcpy(TextZeile[1], "       FEHLER       "); // Zeile 2
              strcpy(TextZeile[2], "Sensorsignal n.i.O. "); // Zeile 3
              strcpy(TextZeile[3], "RK_HZ_R_oben, RK_HZ_L_oben & RK_WZS_innen pruefen"); // Zeile 4
          }   
          last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
        break;

        case 508:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 0;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 0;
              Led_Gelb = 1;
              Led_Rot = 1;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM5: Probenentnahme "); //Zeile 1
              strcpy(TextZeile[1], "Hubzylinder senken ab"); // Zeile 2
              strcpy(TextZeile[2], "                    "); // Zeile 3
              strcpy(TextZeile[3], "UT Hubzyl. quittieren"); //Zeile 4

             
          // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
          if (Switch_by_risingEdge_Variable(BT, last_BT) == true && RK_HZ_R_unten == 1 && RK_HZ_L_unten == 1 && RK_WZS_innen == 1) {
            Zustand = 509;
            next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!            
          }
          //Falls Bedingungen nicht erfüllt sind und so lange BT gehalten wird, wird eine Fehlermeldung ausgegeben
          else if(millis() > next_failure_time && BT == 1 && (RK_HZ_R_unten == 0 || RK_HZ_L_unten == 0 || RK_WZS_innen == 0)){
              //Ausgabe Display
              strcpy(TextZeile[0], "BM5: Probenentnahme "); // Zeile 1
              strcpy(TextZeile[1], "       FEHLER       "); // Zeile 2
              strcpy(TextZeile[2], "Sensorsignal n.i.O. "); // Zeile 3
              strcpy(TextZeile[3], "RK_HZ_R_unten, RK_HZ_L_unten & RK_WZS_innen pruefen"); // Zeile 4
          }
          last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
        break;

        case 509:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 0;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 1;
              Led_Gelb = 0;
              Led_Rot = 0;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM5: Probenentnahme "); // Zeile 1
              strcpy(TextZeile[1], "                    "); // Zeile 2
              strcpy(TextZeile[2], "Probe unten loesen und entnehmen"); // Zeile 3
              strcpy(TextZeile[3], "Mit Taster quittieren"); // Zeile 4

             
          // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
          if (Switch_by_risingEdge_Variable(BT, last_BT) == true && RK_WZS_innen == 1) {
            Zustand = 510;
            next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!            
          }
          //Falls Bedingungen nicht erfüllt sind und so lange BT gehalten wird, wird eine Fehlermeldung ausgegeben
          else if(millis() > next_failure_time && BT == 1 && RK_WZS_innen == 0){
              //Ausgabe Display
              strcpy(TextZeile[0], "BM5: Probenentnahme "); // Zeile 1
              strcpy(TextZeile[1], "       FEHLER       "); // Zeile 2
              strcpy(TextZeile[2], "Sensorsignal n.i.O. "); // Zeile 3
              strcpy(TextZeile[3], "RK_WZS_innen pruefen"); // Zeile 4
          }
          last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
        break;
        
        case 510:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 0;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 1;
              Led_Gelb = 0;
              Led_Rot = 0;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM5: Probenentnahme "); // Zeile 1
              strcpy(TextZeile[1], "Fuer Wkz. in UT BM6 "); // Zeile 2
              strcpy(TextZeile[2], "Fuer Neue Probe BM2 "); // Zeile 3
              strcpy(TextZeile[3], "Betriebsmodus wechseln"); // Zeile 4

        // Kein nachfolgender Zustand sondern neuer Betriebsmodus - daher hier keine Zuweisung der Zustand/Case Variable!
        break; 

        
        case 600:  //BWS=6
          // Prüfen on Initialbedingungen für den Betriebsmodus am Werkzeug vorliegen (alle Sensoren melden die richtigen Signale ... sonst manueller Betrieb notwendig)
          if (DS_Festo == 1 && RK_HZ_R_unten == 1 && RK_HZ_R_oben == 0 && RK_HZ_L_unten == 1 && RK_HZ_L_oben == 0 && RK_KRV_innen == 0 && RK_KRV_aussen == 1 && RK_WZS_innen == 1 && RK_WZS_aussen == 0) {
              //Variablen für Ausgänge setzen
              MagVen_HZ = 0;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 1;
              Led_Gelb = 0;
              Led_Rot = 0;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM6: Wkzg. OT -> UT ");   // Zeile 1
              strcpy(TextZeile[1], "                    ");   // Zeile 2
              strcpy(TextZeile[2], "                    ");   // Zeile 3
              strcpy(TextZeile[3], "Mit Taster quittieren"); // Zeile 4

             // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
              if (Switch_by_risingEdge_Variable(BT, last_BT) == true && RK_KRV_aussen == 1 && RK_WZS_innen == 1) {
                Zustand = 601;
                next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!            
              }    
              last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
          }
          else {
              //Ausgabe Display
              strcpy(TextZeile[0], "BM6: Wkzg. OT -> UT "); // Zeile 1
              strcpy(TextZeile[1], "       FEHLER       "); // Zeile 2
              strcpy(TextZeile[2], "Sensorsignal n.i.O. "); // Zeile 3
              strcpy(TextZeile[3], "DS_Festo == 1 & RK_HZ_R_unten == 1 & RK_HZ_R_oben == 0 & RK_HZ_L_unten == 1 & RK_HZ_L_oben == 0 & RK_KRV_innen == 0 & RK_KRV_aussen == 1 & RK_WZS_innen == 1 & RK_WZS_aussen == 0"); // Zeile 4
          }    
        break;

        case 601:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 0;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 1;
              Led_Gelb = 1;
              Led_Rot = 0;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM6: Wkzg. OT -> UT ");   // Zeile 1
              strcpy(TextZeile[1], "      ACHTUNG!      ");   // Zeile 2
              strcpy(TextZeile[2], "Hubzylinder ausfahren?");   // Zeile 3
              strcpy(TextZeile[3], "Mit Taster quittieren"); // Zeile 4

             
          // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
          if (Switch_by_risingEdge_Variable(BT, last_BT) == true && RK_KRV_aussen == 1 && RK_WZS_innen == 1 && RK_HZ_R_unten == 1 && RK_HZ_L_unten == 1) {
            Zustand = 602;
            next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!            
          }
          //Falls Bedingungen nicht erfüllt sind und so lange BT gehalten wird, wird eine Fehlermeldung ausgegeben
          else if(millis() > next_failure_time && BT == 1 && (RK_KRV_aussen == 0 || RK_WZS_innen == 0 || RK_HZ_R_unten == 0 || RK_HZ_L_unten == 0)){
              //Ausgabe Display
              strcpy(TextZeile[0], "BM6: Wkzg. OT -> UT "); // Zeile 1
              strcpy(TextZeile[1], "       FEHLER       "); // Zeile 2
              strcpy(TextZeile[2], "Sensorsignal n.i.O. "); // Zeile 3
              strcpy(TextZeile[3], "RK_KRV_aussen & RK_WZS_innen & RK_HZ_R_unten & RK_HZ_L_unten pruefen"); // Zeile 4
          }   
          last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
        break;

        case 602:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 1;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 1;
              Led_Gelb = 1;
              Led_Rot = 0;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM6: Wkzg. OT -> UT ");   // Zeile 1
              strcpy(TextZeile[1], "Hubzylinder fahren aus");   // Zeile 2
              strcpy(TextZeile[2], "                    ");   // Zeile 3
              strcpy(TextZeile[3], "OT Hubzyl. quittieren"); // Zeile 4

             
          // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
          if (Switch_by_risingEdge_Variable(BT, last_BT) == true && RK_KRV_aussen == 1 && RK_WZS_innen == 1 && RK_HZ_R_oben == 1 && RK_HZ_L_oben == 1) {
            Zustand = 603;
            next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!            
          }
          //Falls Bedingungen nicht erfüllt sind und so lange BT gehalten wird, wird eine Fehlermeldung ausgegeben
          else if(millis() > next_failure_time && BT == 1 && (RK_KRV_aussen == 0 || RK_WZS_innen == 0 || RK_HZ_R_oben == 0 || RK_HZ_L_oben == 0)){
              //Ausgabe Display
              strcpy(TextZeile[0], "BM6: Wkzg. OT -> UT "); // Zeile 1
              strcpy(TextZeile[1], "       FEHLER       "); // Zeile 2
              strcpy(TextZeile[2], "Sensorsignal n.i.O. "); // Zeile 3
              strcpy(TextZeile[3], "RK_KRV_aussen & RK_WZS_innen & RK_HZ_R_oben & RK_HZ_L_oben pruefen"); // Zeile 4
          }   
          last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
        break;

        case 603:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 1;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 0;
              Led_Gelb = 0;
              Led_Rot = 1;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM6: Wkzg. OT -> UT ");   // Zeile 1
              strcpy(TextZeile[1], "Sicherung mit Handrad entlasten");   // Zeile 2
              strcpy(TextZeile[2], "Sicherung ausschwenken");   // Zeile 3
              strcpy(TextZeile[3], "Mit Taster quittieren"); // Zeile 4

             
          // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
          if (Switch_by_risingEdge_Variable(BT, last_BT) == true && RK_KRV_aussen == 1 && RK_WZS_aussen == 1 && RK_HZ_R_oben == 1 && RK_HZ_L_oben == 1) {
            Zustand = 604;
            next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!            
          }
          //Falls Bedingungen nicht erfüllt sind und so lange BT gehalten wird, wird eine Fehlermeldung ausgegeben
          else if(millis() > next_failure_time && BT == 1 && (RK_KRV_aussen == 0 || RK_WZS_aussen == 0 || RK_HZ_R_oben == 0 || RK_HZ_L_oben == 0)){
              //Ausgabe Display
              strcpy(TextZeile[0], "BM6: Wkzg. OT -> UT "); // Zeile 1
              strcpy(TextZeile[1], "       FEHLER       "); // Zeile 2
              strcpy(TextZeile[2], "Sensorsignal n.i.O. "); // Zeile 3
              strcpy(TextZeile[3], "RK_KRV_aussen & RK_WZS_aussen & RK_HZ_R_oben & RK_HZ_L_oben pruefen"); // Zeile 4
          }
          last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
        break;

        case 604:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 1;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 0;
              Led_Gelb = 1;
              Led_Rot = 1;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM6: Wkzg. OT -> UT ");   // Zeile 1
              strcpy(TextZeile[1], "      ACHTUNG!      ");   // Zeile 2
              strcpy(TextZeile[2], "Hubzylinder absenken?");   // Zeile 3
              strcpy(TextZeile[3], "Mit Taster quittieren"); // Zeile 4

             
          // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
          if (Switch_by_risingEdge_Variable(BT, last_BT) == true && RK_KRV_aussen == 1 && RK_WZS_aussen == 1 && RK_HZ_R_oben == 1 && RK_HZ_L_oben == 1) {
            Zustand = 605;
            next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!            
          }
          //Falls Bedingungen nicht erfüllt sind und so lange BT gehalten wird, wird eine Fehlermeldung ausgegeben
          else if(millis() > next_failure_time && BT == 1 && (RK_KRV_aussen == 0 || RK_WZS_aussen == 0 || RK_HZ_R_oben == 0 || RK_HZ_L_oben == 0)){
              //Ausgabe Display
              strcpy(TextZeile[0], "BM6: Wkzg. OT -> UT "); // Zeile 1
              strcpy(TextZeile[1], "       FEHLER       "); // Zeile 2
              strcpy(TextZeile[2], "Sensorsignal n.i.O. "); // Zeile 3
              strcpy(TextZeile[3], "RK_KRV_aussen & RK_WZS_aussen & RK_HZ_R_oben & RK_HZ_L_oben pruefen"); // Zeile 4
          }
          last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
        break;

        case 605:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 0;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 0;
              Led_Gelb = 1;
              Led_Rot = 1;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM6: Wkzg. OT -> UT ");   // Zeile 1
              strcpy(TextZeile[1], "Hubzylinder senken ab");  // Zeile 2
              strcpy(TextZeile[2], "                    ");   // Zeile 3
              strcpy(TextZeile[3], "Wkzg. UT quittieren ");   // Zeile 4

             
          // Prüfen ob Taster eine steigende Flanke aufweist --> Zustand ändern!
          if (Switch_by_risingEdge_Variable(BT, last_BT) == true && RK_KRV_aussen == 1 && RK_WZS_aussen == 1 && RK_HZ_R_unten == 1 && RK_HZ_L_unten == 1) {
            Zustand = 606;
            next_failure_time = millis() + wait_failure;     //für die Fehlermeldung nach dem Zustandswechsel-  die nächste Fehleranzeigewartezeit festlegen --> wird immer beim Zustandswechsel gemacht und verzögert damit die Anzeige des Fehlers bei neuem Zustand, wenn die Sensorsignale (noch) nicht korrekt sind!           
          }
          //Falls Bedingungen nicht erfüllt sind und so lange BT gehalten wird, wird eine Fehlermeldung ausgegeben
          else if(millis() > next_failure_time && BT == 1 && (RK_KRV_aussen == 0 || RK_WZS_aussen == 0 || RK_HZ_R_unten == 0 || RK_HZ_L_unten == 0)){
              //Ausgabe Display
              strcpy(TextZeile[0], "BM6: Wkzg. OT -> UT "); // Zeile 1
              strcpy(TextZeile[1], "       FEHLER       "); // Zeile 2
              strcpy(TextZeile[2], "Sensorsignal n.i.O. "); // Zeile 3
              strcpy(TextZeile[3], "RK_KRV_aussen & RK_WZS_aussen & RK_HZ_R_unten & RK_HZ_L_unten pruefen"); // Zeile 4
          }
          last_BT = BT; //Neuen Zustand des Tasters für nächsten Durchgang merken!
        break;
        
        case 606:
              //Variablen für Ausgänge setzen
              MagVen_HZ = 0;
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;              
              Led_Gruen = 0;
              Led_Gelb = 0;
              Led_Rot = 1;
              
              //Ausgabe Display
              strcpy(TextZeile[0], "BM6: Wkzg. OT -> UT ");   // Zeile 1
              strcpy(TextZeile[1], "                    ");  // Zeile 2
              strcpy(TextZeile[2], "                    ");   // Zeile 3
              strcpy(TextZeile[3], "Betriebsmodus wechseln -> BM1");   // Zeile 4

        // Kein nachfolgender Zustand sondern neuer Betriebsmodus - daher hier keine Zuweisung der Zustand/Case Variable!
        break; 



        //Betriebsmodus 7 - Manueller Betrieb --> Crashgefahr mit Knickrichtungsvorgeber!!!
        case 700:  //BWS=7
          strcpy(TextZeile[0], "BM7: Manueller Betrieb"); // Zeile 1
          strcpy(TextZeile[1], "                    "); // Zeile 2
          strcpy(TextZeile[2], "Keine Ueberwachung! "); // Zeile 3
          strcpy(TextZeile[3], "   Crashgefahr!!!   "); // Zeile 4
        
        //Schalten der LEDS (alle 3 LEDS an) --> Manueller Modus
          Led_Gruen = 1;
          Led_Gelb = 1;
          Led_Rot = 1;
          
          //Schalten der Pneumatikventile im manuellen Modus
        //Hubzylinderventil hoch
          if (S_HZ_hoch == 1){
            MagVen_HZ = 1;
          }
          else {
            MagVen_HZ = 0;
          }



       //KRV rein --> Mit Dämpfung damit der KRV langsam nach innen fährt!
          if (Switch_by_risingEdge_Variable(S_KRV_rein, last_S_KRV_rein) == true && S_KRV_rein == 1){
            nextDaempfungsende = millis() + (1*Druckaufbauzeit_Daempfung_KRV_rein); //Zeitpunkt der Endzeit der Dämpfung zum KRV einfahren festlegen!
          }
          last_S_KRV_rein = S_KRV_rein; //Neuen Zustand des Schalters für nächsten Durchgang merken!
          
          //Dämpfung durchn >Druckaufbau nach außen aktiv
          if (nextDaempfungsende > millis() && S_KRV_rein == 1 && S_KRV_raus == 0){
                MagVen_KRV_rein = 0;
                MagVen_KRV_raus = 1;
          }
          if (nextDaempfungsende <= millis() && S_KRV_rein == 1 && S_KRV_raus == 0){
                MagVen_KRV_rein = 1;
                MagVen_KRV_raus = 0;
          }
          else {
            MagVen_KRV_rein = 0;
          }
          
          last_S_KRV_rein = S_KRV_rein; //Neuen Zustand des Schalters für nächsten Durchgang merken!



       //KRV raus --> geht nur schnell, da die Pneumatik keine Drosel hier verbaut hat, um auch im Versuch schnell aus der Gefahrenzone mit dem KRV fahren zu können!
          if (S_KRV_raus == 1 && S_KRV_rein == 0){
            MagVen_KRV_raus = 1;
          }
          else if (S_KRV_raus == 0 && S_KRV_rein == 0) { // hier darf kein else sondern nur eine else if stehen, da sonst die Dämpfung durch überschreiben der MagVen_KRV_raus überschrieben wird bevor diese aktiv werden kann
            MagVen_KRV_raus = 0;
          }
        
        break;





        case 800:  //BWS=8
          strcpy(TextZeile[0], "BM8: Not used       "); // Zeile 1
          strcpy(TextZeile[1], "                    "); // Zeile 2
          strcpy(TextZeile[2], "                    "); // Zeile 3
          strcpy(TextZeile[3], "                    "); // Zeile 4
        break;





        case 900:  //BWS=9
          strcpy(TextZeile[0], "BM9: Not used       "); // Zeile 1
          strcpy(TextZeile[1], "                    "); // Zeile 2
          strcpy(TextZeile[2], "                    "); // Zeile 3
          strcpy(TextZeile[3], "                    "); // Zeile 4
        break;






        case 1000:  //BWS=10
          strcpy(TextZeile[0], "BM10: Not used      "); // Zeile 1
          strcpy(TextZeile[1], "                    "); // Zeile 2
          strcpy(TextZeile[2], "                    "); // Zeile 3
          strcpy(TextZeile[3], "                    "); // Zeile 4
        break;





        case 1100:  //BWS=11
          strcpy(TextZeile[0], "BM11: Not used      "); // Zeile 1
          strcpy(TextZeile[1], "                    "); // Zeile 2
          strcpy(TextZeile[2], "                    "); // Zeile 3
          strcpy(TextZeile[3], "                    "); // Zeile 4
        break;





        case 1200:  //BWS=12
          strcpy(TextZeile[0], "BM12: Not used      "); // Zeile 1
          strcpy(TextZeile[1], "                    "); // Zeile 2
          strcpy(TextZeile[2], "                    "); // Zeile 3
          strcpy(TextZeile[3], "                    "); // Zeile 4
        break;
        
      }


//Optional Ausgabe der Schieberegisterinhalte am Display mit spezieller Schalterstellung der Manuellen Betätigungsschalter
//Eingangsschieberegister 74HC165 (Sensoren und Schalter) - Bedingung alle manuellen Schalter sind auf "Ein"
 if(S_KRV_rein == 1 && S_KRV_raus == 1 && S_HZ_hoch == 1 && BT == 0){
   for(byte j=0; j<No_74HC165; j++)
     {
        //Ausgabe in seriellem Monitor zur Überprüfung / Wartung - normalerweise auskommentiert!
       lcd.setCursor(0, j);
       lcd.print((String)"SR165_No." + (j+1) + ": ");
       for (byte m=0;m<8;m++)
       { 
        lcd.setCursor(m + 12,j);
        lcd.print(InSR165[j] >> m & 1, BIN);
       }
       delay(250);
     }
   }

 //Ausgangsschieberegister 74HC595 -Ausgänge (Magnetventile und LEDS) - Bedingung: Schalter für KRV rein und raus auf "Ein", Schalter Hubzylinder auf "Aus" und Bestätigungstaster auf "Ein"
 else if(S_KRV_rein == 1 && S_KRV_raus == 1 && S_HZ_hoch == 1 && BT == 1){
     for(byte j=0; j<No_74HC595; j++)
     {
        //Ausgabe in seriellem Monitor zur Überprüfung / Wartung - normalerweise auskommentiert!
       lcd.setCursor(0, j);
       lcd.print((String)"SR595_No." + (j+1) + ": ");
       for (byte m=0;m<8;m++)
       { 
        lcd.setCursor(m + 12,j);
        lcd.print(OutSR595[j] >> m & 1, BIN);
       }
       delay(250);
     }
   }
 
 else{
  //Ausgabe der Zeilen aus den Textvariablen für Zeile 1 bis 4 des LCD Displays am Ende der Void Loop Funktion!
 task_text(TextZeile[0],0);
 task_text(TextZeile[1],1);
 task_text(TextZeile[2],2);
 task_text(TextZeile[3],3);
 }
}




void importShiftRegister_165()  //Funktion zum Einlesen der Eingänge des Schieberegisters (Sensorzustände)
{
 
  digitalWrite(clockPin_165, HIGH);   //Voreinstellung des ClockPin
  digitalWrite(latchPin_165, HIGH);   //Paralelle Eingänge werden deaktiviert, so dass der aktuelle Zustand eingefroren wird im Schiebregister

  //Serielles Einlesen der parallelen Eingangszustände aller 74HC165 --> 8 Bitweise, da ShiftIn() nur jeweils 8 Bit einliest
  for(byte k=0; k < No_74HC165; k++)
  {
    InSR165[k] = shiftIn(dataPin_165, clockPin_165, MSBFIRST); 
  }
  
  digitalWrite(latchPin_165, LOW);   //Paralelle Eingänge werden wieder aktiviert, so dass das Schiebregister die anliegenden Zustände übernimmt!


  //Zuweisen der Bitwerte in die Variablen für die einzelnen Eingänge! --> Zustand für Startbedingung festlegen --> Wenn sich die Schaltzustände am Betriebswahschalter ändern (Flankenerkennung), nur dann wird der Zustand gesetzt!
    BWS_01 = bitRead(InSR165[SR_BWS_01], Bit_BWS_01); 
      if (BWS_01 == 1) {
        if (Switch_by_risingEdge_Variable(BWS_01, last_BWS_01) == true){
          Zustand = 100;
          //Zur Sicherheit Ventile bei Wechsel des Betriebsmodus abschalten --> Unterfunktion aufrufen
          MagVen_AUS();
        }
      }
      last_BWS_01 = BWS_01; 

    BWS_02 = bitRead(InSR165[SR_BWS_02], Bit_BWS_02);
      if (BWS_02 == 1) {
        if (Switch_by_risingEdge_Variable(BWS_02, last_BWS_02) == true){
          Zustand = 200;
          //Zur Sicherheit Ventile bei Wechsel des Betriebsmodus abschalten --> Unterfunktion aufrufen
          MagVen_AUS();
        }
      }
      last_BWS_02 = BWS_02; 
      
    BWS_03 = bitRead(InSR165[SR_BWS_03], Bit_BWS_03);
      if (BWS_03 == 1) {
        if (Switch_by_risingEdge_Variable(BWS_03, last_BWS_03) == true){
          Zustand = 300;
          //Zur Sicherheit Ventile bei Wechsel des Betriebsmodus abschalten --> Unterfunktion aufrufen
          MagVen_AUS();
        }
      }
      last_BWS_03 = BWS_03; 
      
    BWS_04 = bitRead(InSR165[SR_BWS_04], Bit_BWS_04);
    if (BWS_04 == 1) {
        if (Switch_by_risingEdge_Variable(BWS_04, last_BWS_04) == true){
          Zustand = 400;
          //Zur Sicherheit Ventile bei Wechsel des Betriebsmodus abschalten --> Unterfunktion aufrufen
          MagVen_AUS();
        }
      }
      last_BWS_04 = BWS_04; 
      
    BWS_05 = bitRead(InSR165[SR_BWS_05], Bit_BWS_05);
    if (BWS_05 == 1) {
        if (Switch_by_risingEdge_Variable(BWS_05, last_BWS_05) == true){
          Zustand = 500;
          //Zur Sicherheit Ventile bei Wechsel des Betriebsmodus abschalten --> Unterfunktion aufrufen
          MagVen_AUS();
        }
      }
      last_BWS_05 = BWS_05; 
      
    BWS_06 = bitRead(InSR165[SR_BWS_06], Bit_BWS_06);
    if (BWS_06 == 1) {
        if (Switch_by_risingEdge_Variable(BWS_06, last_BWS_06) == true){
          Zustand = 600;
          //Zur Sicherheit Ventile bei Wechsel des Betriebsmodus abschalten --> Unterfunktion aufrufen
          MagVen_AUS();
        }
      }
      last_BWS_06 = BWS_06; 
      
    BWS_07 = bitRead(InSR165[SR_BWS_07], Bit_BWS_07);
    if (BWS_07 == 1) {
        if (Switch_by_risingEdge_Variable(BWS_07, last_BWS_07) == true){
          Zustand = 700;
          //Zur Sicherheit Ventile bei Wechsel des Betriebsmodus abschalten --> Unterfunktion aufrufen
          MagVen_AUS();
        }
      }
      last_BWS_07 = BWS_07; 
      
    BWS_08 = bitRead(InSR165[SR_BWS_08], Bit_BWS_08);
    if (BWS_08 == 1) {
        if (Switch_by_risingEdge_Variable(BWS_08, last_BWS_08) == true){
          Zustand = 800;
          //Zur Sicherheit Ventile bei Wechsel des Betriebsmodus abschalten --> Unterfunktion aufrufen
          MagVen_AUS();
        }
      }
      last_BWS_08 = BWS_08; 
      
    BWS_09 = bitRead(InSR165[SR_BWS_09], Bit_BWS_09);
    if (BWS_09 == 1) {
        if (Switch_by_risingEdge_Variable(BWS_09, last_BWS_09) == true){
          Zustand = 900;
          //Zur Sicherheit Ventile bei Wechsel des Betriebsmodus abschalten --> Unterfunktion aufrufen
          MagVen_AUS();
        }
      }
      last_BWS_09 = BWS_09; 
      
    BWS_10 = bitRead(InSR165[SR_BWS_10], Bit_BWS_10);
    if (BWS_10 == 1) {
        if (Switch_by_risingEdge_Variable(BWS_10, last_BWS_10) == true){
          Zustand = 1000;
          //Zur Sicherheit Ventile bei Wechsel des Betriebsmodus abschalten --> Unterfunktion aufrufen
          MagVen_AUS();
        }
      }
      last_BWS_10 = BWS_10; 
      
    BWS_11 = bitRead(InSR165[SR_BWS_11], Bit_BWS_11);
    if (BWS_11 == 1) {
        if (Switch_by_risingEdge_Variable(BWS_11, last_BWS_11) == true){
          Zustand = 1100;
          //Zur Sicherheit Ventile bei Wechsel des Betriebsmodus abschalten --> Unterfunktion aufrufen
          MagVen_AUS();
        }
      }
      last_BWS_11 = BWS_11; 
      
    BWS_12 = bitRead(InSR165[SR_BWS_12], Bit_BWS_12);
    if (BWS_12 == 1) {
        if (Switch_by_risingEdge_Variable(BWS_12, last_BWS_12) == true){
          Zustand = 1200;
          //Zur Sicherheit Ventile bei Wechsel des Betriebsmodus abschalten --> Unterfunktion aufrufen
          MagVen_AUS();
        }
      }
      last_BWS_12 = BWS_12; 
      
    DS_Festo = bitRead(InSR165[SR_DS_Festo], Bit_DS_Festo);
    BT = bitRead(InSR165[SR_BT], Bit_BT);
    RK_HZ_R_unten = bitRead(InSR165[SR_RK_HZ_R_unten], Bit_RK_HZ_R_unten);
    RK_HZ_R_oben = bitRead(InSR165[SR_RK_HZ_R_oben], Bit_RK_HZ_R_oben);
    RK_HZ_L_unten = bitRead(InSR165[SR_RK_HZ_L_unten], Bit_RK_HZ_L_unten);
    RK_HZ_L_oben = bitRead(InSR165[SR_RK_HZ_L_oben], Bit_RK_HZ_L_oben);
    RK_KRV_innen = bitRead(InSR165[SR_RK_KRV_innen], Bit_RK_KRV_innen);
    RK_KRV_aussen = bitRead(InSR165[SR_RK_KRV_aussen], Bit_RK_KRV_aussen);
    RK_WZS_innen = bitRead(InSR165[SR_RK_WZS_innen], Bit_RK_WZS_innen);
    RK_WZS_aussen = bitRead(InSR165[SR_RK_WZS_aussen], Bit_RK_WZS_aussen);
    S_HZ_hoch = bitRead(InSR165[SR_S_HZ_hoch], Bit_S_HZ_hoch);
    S_KRV_rein = bitRead(InSR165[SR_S_KRV_rein], Bit_S_KRV_rein);
    S_KRV_raus = bitRead(InSR165[SR_S_KRV_raus], Bit_S_KRV_raus);

    //eine Rückgabe von Werten mit der Return-Funktion muss nicht erfolgen, da mit globalen Variablen gearbeitet wird!
}

     
void updateShiftRegister_595() // Funktion zum Befüllen des Schiebregisters
{
   
   //Variblenwerte in OutSR595[] schreiben
   //Magnetventile Ausgänge
   bitWrite(OutSR595[SR_MagVen_HZ], Bit_MagVen_HZ, MagVen_HZ);
   bitWrite(OutSR595[SR_MagVen_KRV_rein], Bit_MagVen_KRV_rein, MagVen_KRV_rein);
   bitWrite(OutSR595[SR_MagVen_KRV_raus], Bit_MagVen_KRV_raus, MagVen_KRV_raus);

   //LED Ausgang
   bitWrite(OutSR595[SR_Led_Gruen], Bit_Led_Gruen, Led_Gruen);
   bitWrite(OutSR595[SR_Led_Gelb], Bit_Led_Gelb, Led_Gelb);
   bitWrite(OutSR595[SR_Led_Rot], Bit_Led_Rot, Led_Rot);

 

  
   
   digitalWrite(latchPin_595, LOW);                                 //Vorbereiten der Übertragung in Ausgaberegister bei Pos. Flanke

   
   for(byte j=0; j<No_74HC595; j++)
   {
     shiftOut(dataPin_595, clockPin_595, MSBFIRST, OutSR595[j]);   //shiftOut schreibt nur 8 Bit, deshalb müssen für jedes Schieberegister die Daten des Arrays mit shiftOut() geshiftet werden.
    
     //Ausgabe in seriellem Monitor zur Überprüfung / Wartung - normalerweise auskommentiert!
     /*Serial.print((String)"SR" + j + ": ");
     for (byte m=0;m<8;m++)
     { 
      Serial.print(OutSR595[j] >> m & 1, BIN);
     }
     Serial.println();*/
   }

   
  digitalWrite(latchPin_595, HIGH);                                //Übertragen von Schiebe- in Ausgaberegister "Ausgänge Aktiv Schalten"
   
}



void print_InSR165() //Ausgabe der eingelesenen Bits im Seriellen Monitor für Wartung/ Fehlersuche - normalerweise ist der Funktionsaufruf im "void loop" auskommentiert
{
     
     Serial.print("\n");

     for (byte l=0; l<No_74HC165; l++)
     {
        byte printByteIn = InSR165[l];
        Serial.print("SR" + String(l+1) + ": ");
       for (byte n=0;n<8;n++)
       {
          Serial.print(printByteIn >> n & 1, BIN);
       }
       Serial.println();
       
     }
      //delay(500); //delay zur besseren Ansicht optional
}


//Funktion für die Scrollfunktion, um den Text in der Variable zu schieben!
char charAt(char *text, int pos)
// scrolling-logic coded here
{
  if (pos<LEDLINE) return ' '; // scroll in
  else if (pos>=LEDLINE && pos<LEDLINE+strlen(text))
    return text[pos-LEDLINE]; // scroll text
  else return ' ';  // scroll out
}


//Zeitlicher Ablauf wann gescrollt werden muss und ob die Textlänge ein Scrollen erfordert prüfen und ggf. Scrollen durchführen
void task_text(char *text, byte line)
// scroll the LED lines
{
  char currenttext[LEDLINE+1];
  static unsigned long nextscroll[4];
  static int positionCounter[4]; 
  int i;
  
  //Textinhalte des 2dimensionalen Arrays vergleichen. hats sich inhaltlich ewats verändert --> Position dieser Zeile auf Null setzen
  if (strcmp(text, LastTextZeile[line]) != 0){
    positionCounter[line] = 0;
    //Hier muss der aktuelle Text in die LastTextzeile[][] Variable kopiert werden!
    strcpy(LastTextZeile[line], text);
  }
  //Beim Wechsel der Textlänge kann der positionCounter des Textes beim Scrollen zu hoch sein und sich nicht mehr zurückstellen --> Dann würde der Text nicht angezeigt werden! --> Hier muss die Position  zurückgesetzt werden!
  if (ledScrollDir[line] == true && positionCounter[line] > (strlen(text) + LEDLINE)){ //für nach links scrollen
    positionCounter[line]=0;
  }
  else if (positionCounter[line]<0){ //für nach rechts scrollen
    positionCounter[line]=strlen(text)+LEDLINE-1;
  }
  

  
  //Prüfen ob Zeitpunkt zum Scrollen jetzt vorhanden und ob die Textlänge scrollen nötig macht!
  if (millis()>nextscroll[line] && strlen(text)>LEDLINE)
  {
    nextscroll[line]=millis()+ledScrollSpeed[line]; 

    for (i=0;i<LEDLINE;i++)
      currenttext[i]=charAt(text,positionCounter[line]+i);
    currenttext[LEDLINE]=0;    

    lcd.setCursor(0,line);
    lcd.print(currenttext);
    if (ledScrollDir[line])
    {
      positionCounter[line]++;
      if (positionCounter[line]==strlen(text)+LEDLINE) positionCounter[line]=0;
    }  
    else  
    {
      positionCounter[line]--;
      if (positionCounter[line]<0) positionCounter[line]=strlen(text)+LEDLINE-1;
    }  
  }
  else if (millis() > nextscroll[line] && strlen(text) <= LEDLINE) //geändert von if -> else if
  {
    nextscroll[line]=millis()+ ledScrollSpeed[line];
    lcd.setCursor(0,line);
    lcd.print(text);
  }
}


//Funktion zur Flankenerkennung (Schaltwechselerkennung) des Tasters und des Betriebswahlschalter - mit Debauncing!
bool Switch_by_risingEdge_Variable (byte &VariableStatus, byte &LastVariableStatus ){
//Returnvariable 
bool  Wechsel_erkannt = false;
  
  // Prüfen ob sich was verändert hat
  if (VariableStatus != LastVariableStatus && millis() > nextCheckTimeFlanke){
      
      
      if (VariableStatus == 1) { 
        // Wechsel von Low nach High erkannt:
        if (LastVariableStatus == 0) {
            Wechsel_erkannt = true;
        }
        else if (LastVariableStatus == 1) {
            //Nothing
        }
      }
     
      // Die nächste Prüfung der Tasterstellung um Bouncing zu filtern 
      nextCheckTimeFlanke = millis() + TimeStepAntiBouncing;      
  }


//Boolean Wert zurückgeben
  return Wechsel_erkannt;
  
}


//Funktion zum Deaktivieren der Magnetventile - Sicherheitsmaßnahme, wenn bei aktiven Ventilen der Betriebsmodus gewechselt wird, so dass die Ventile nicht mehr abschalten bzw abgeschaltetet werden können (z.B. bei manuellem Betrieb!)
void MagVen_AUS(){
              //Variablen für Ausgänge setzen
              if (RK_WZS_innen == 1){
                MagVen_HZ = 0;
              }
                             
              MagVen_KRV_rein = 0;
              MagVen_KRV_raus = 0;      

}
