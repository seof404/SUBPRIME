
// arduino UNO -- télécommande
/*|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
// mouvement

// initialisation
#include <Keypad.h>

#define padLigne1 8     // pin de la ligne 1 du pad
#define padLigne2 9     // pin de la ligne 2 du pad
#define padLigne3 10    // pin de la ligne 3 du pad
#define padColonne1 11  // pin de la colonne 1 du pad
#define padColonne2 12  // pin de la colonne 2 du pad
#define padColonne3 13  // pin de la colonne 3 du pad

#define pinPropulsion A0  // pin de la mesure du potentiomètre

const uint8_t lignes = 3;    // 3 lignes
const uint8_t colonnes = 3;  // 3 colonnes

//definit les touches du pad en partant de en haut à gauche du pad
const char hexaKeys[lignes][colonnes] = {
  { '+', '2', '*' },
  { '3', '1', '4' },
  { '-', '5', '/' }
};

uint8_t pinsLignes[lignes] = { padLigne1, padLigne2, padLigne3 };            //connect to the row pinouts of the keypad
uint8_t pinsColonnes[colonnes] = { padColonne1, padColonne2, padColonne3 };  //connect to the column pinouts of the keypad

Keypad padCommande = Keypad(makeKeymap(hexaKeys), pinsLignes, pinsColonnes, lignes, colonnes);

void initCommande() {
  pinMode(pinPropulsion, INPUT);
  return;
}

// fonction
uint8_t getVitesse() {
  return map(analogRead(pinPropulsion), 0, 1023, 25, 255);
}

uint8_t getElevation(char touche) {
  switch (touche) {
    case '+':  // monter de 5 cm
      return 1;
    case '-':  // descendre de 5 cm
      return 2;
    case '*':  // monter de 10 cm
      return 3;
    case '/':  // descendre de 10 cm
      return 4;
    default:
      return 0;  // touche inconnu
  }
}

uint8_t getDirection(char touche) {
  switch (touche) {
    case '1':  // stop
      return 1;
    case '2':  // ↑
      return 2;
    case '3':  // <-
      return 3;
    case '4':  // ->
      return 4;
    case '5':  // ↓
      return 5;
    default:  // touche inconnu
      return 0;
  }
}

/*|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
// communication

// initialisation
#include <RCSwitch.h>  // bibliothèque RC-switch

#define pinRecepteur 2  // interuption 0 => pin 2
#define pinEmetteur 3   // pin de reception des données 4

// taille du message = protocol (en bits)
#define protocolVitesse 8    // vitesse (de 0 à 255 => max longueur 2^8)
#define protocolElevation 6  // elevation (de 1 à 4 => max longueur 2^3)
#define protocolDirection 4  // direction (de 1 à 5 max longueur 2^3)
#define protocolReset 10
#define protocolInformation 18

bool handshake = 0;

RCSwitch comSys = RCSwitch();  // création d'une intance nommé comSys à partir de l'objet RCSwitch;

void initCommunication() {

  pinMode(pinEmetteur, OUTPUT);
  comSys.enableTransmit(pinEmetteur);  // transmetteur pin pinEmetteur
  comSys.setRepeatTransmit(20);        // Optional set number of transmission repetitions.
  comSys.setPulseLength(433);          // Optional set pulse length.

  pinMode(pinRecepteur, INPUT);
  comSys.enableReceive(digitalPinToInterrupt(pinRecepteur));  // Receiver on interrupt 0 => that is pin #2


  // initialisation et vérification de la communication avec le sous-marin
  while (handshake == 0) {

    setAffichage("initialisation ", "du SUB PRIME");

    emission(protocolReset, 1);
    emission(protocolReset, 1);

    uint32_t restartSubTemps = millis();

    while (true) {
      if (millis() - restartSubTemps > 60000) {
        setAffichage("handshake", "erreur");
        delay(1500);
        handshake = 1;
        break;
      } else if (comSys.available()) {
        reception();
        comSys.resetAvailable();
        break;
      }
    }
  }
  return;
}

// fonction
void emission(uint8_t protocol, uint8_t message) {
  static bool ID = 0;

  // Serial.println("envoyé ID = " + String(ID) + " protocol = " + String(protocol) + " message = " + String(message));

  comSys.send(message, protocol + ID);
  ID = !ID;
  return;
}


void reception() {
  static bool dernierID = 1;
  uint8_t protocol = comSys.getReceivedBitlength();  // donne la longueur du message

  bool ID = protocol % 2;

  if (ID != dernierID) {

    uint32_t message = comSys.getReceivedValue();  // donne le message

    // Serial.println("reçu ID = " + String(ID) + " protocol = " + String(protocol) + " message = " + String(message));

    protocol -= ID;
    dernierID = ID;

    switch (protocol) {
      case protocolReset:
        handshake = message;
        return;
      case protocolInformation:
        int profondeurMesure = message % 1000;
        int profondeurCible = message / 1000;

        setAffichage("Pm : " + String(profondeurMesure) + " cm", "Pc : " + String(profondeurCible) + " cm");
        return;
    }
  }
  return;
}



/*|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
// affichage

// initialisation
#include "LCD.h"
#include "LiquidCrystal_I2C.h"
#define I2C_ADDR 0x27  // identifiant de l'écran
#define Rs_pin 0
#define Rw_pin 1
#define En_pin 2
#define BACKLIGHT_PIN 3
#define D4_pin 4
#define D5_pin 5
#define D6_pin 6
#define D7_pin 7
LiquidCrystal_I2C lcd(I2C_ADDR, En_pin, Rw_pin, Rs_pin, D4_pin, D5_pin, D6_pin, D7_pin);

void initAffichage() {
  lcd.begin(16, 2);                              // Si 16 caractères et 2 lignes
  lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE);  // LCD Backlight ON
  lcd.setBacklight(HIGH);                        // try LOW to test
  lcd.home();                                    // go home on LCD
}

// fonction
void setAffichage(String texte1, String texte2) {
  lcd.clear();
  lcd.print(texte1);
  lcd.setCursor(0, 1);  // 1e caractère 2eme ligne
  lcd.print(texte2);    // ecrit sur l'écran LCD
}

/*|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
// autre

void setup() {

  // Serial.begin(9600);

  initAffichage();

  initCommande();

  initCommunication();

  setAffichage("welcome aboard", "INFOSCAM");
  delay(2000);
}

void loop() {

  static String texteDirection[5] = { "stopper", "avancer", "gauche", "droite", "reculer" };
  static String texteElevation[4] = { "- 5 cm", "+ 5 cm", "- 10 cm", "+ 10 cm" };
  static uint8_t prevVitesse = 0;


  uint8_t vitesse = getVitesse();
  if (prevVitesse - 1 > vitesse || vitesse > prevVitesse) {
    setAffichage("propulsion (%)", String((vitesse - 25) / 2.3, 0));
    emission(protocolVitesse, vitesse);
    prevVitesse = vitesse;
  }

  char touche = padCommande.getKey();
  if (touche) {
    uint8_t direction = getDirection(touche);
    if (direction) {
      setAffichage("direction", texteDirection[direction - 1]);
      emission(protocolDirection, direction);
    } else {
      uint8_t elevation = getElevation(touche);
      setAffichage("profondeur", texteElevation[elevation - 1]);
      emission(protocolElevation, elevation);
    }
  }

  reception();
}