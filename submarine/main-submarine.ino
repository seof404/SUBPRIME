// arduino MEGA2560 -- sous-marin

/*|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
// mouvement

// initialisation
#define pinMoteur_1_1 26      // pin 1 du sens de rotation du moteur 1
#define pinMoteur_1_2 28      // pin 2 du sens de rotation du moteur 1
#define pinVitesseMoteur_1 4  // pin de la vitesse du moteur 1

#define pinMoteur_2_1 34      // pin 1 du sens de rotation du moteur 2
#define pinMoteur_2_2 36      // pin 2 du sens de rotation du moteur 2
#define pinVitesseMoteur_2 5  // pin de la vitesse du moteur 2

#define pinBallast_1 50      // pin 1 du sens de rotation de la seringue
#define pinBallast_2 52      // pin 2 du sens de rotation de la seringue
#define pinVitesseBallast 7  // pin de la vitesse de la seringue

// bibliothèque de gestion du capteur de pression
#include <Wire.h>
#include <Adafruit_BMP280.h>

// variable global
int profondeurCible = 0;
float pressionRef;

Adafruit_BMP280 bmp;  // I2C

void initMouvement() {
  pinMode(pinMoteur_1_1, OUTPUT);
  pinMode(pinMoteur_1_2, OUTPUT);
  pinMode(pinVitesseMoteur_1, OUTPUT);

  pinMode(pinMoteur_2_1, OUTPUT);
  pinMode(pinMoteur_2_2, OUTPUT);
  pinMode(pinVitesseMoteur_2, OUTPUT);

  pinMode(pinBallast_1, OUTPUT);
  pinMode(pinBallast_2, OUTPUT);
  pinMode(pinVitesseBallast, OUTPUT);


  setBallast(-180);
  delay(5000);
  setBallast(0);

  bmp.begin(0x76);  // démarrer la communication I2C
  // pin du baromètre dans le ballon : noir = Vcc ; blanc = GND ; gris = SCL ; violet = SDK



  // paramètre par défaut du capteur de préssion
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_NONE,   /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  pressionRef = bmp.readPressure();  // donne la pression en hPa donc on rajoute la conversion
}

// fonction
int getProfondeur() {
  // constante local
  static const float gravite = 9.807;               // champ de pesanteur (m/s^2)
  static const uint16_t rho = 1000;                 // masse volumique de l'eau (kg/m^3)
  static const float Cste = 1E2 / (gravite * rho);  // constante de conversion Pa en cm

  long profondeurMesure = (bmp.readPressure() - pressionRef) * Cste;

  return profondeurMesure;
}

void setProfondeur(int elevation) {
  // constante local
  static const uint8_t profondeurMin = 0;   // centimètre
  static const uint8_t profondeurMax = 90;  // centimètre

  switch (elevation) {
    case 1:
      profondeurCible -= 5;
      break;
    case 2:
      profondeurCible += 5;
      break;
    case 3:
      profondeurCible -= 10;
      break;
    case 4:
      profondeurCible += 10;
      break;
  }

  if (profondeurCible < profondeurMin) {
    profondeurCible = profondeurMin;
  } else if (profondeurCible > profondeurMax) {
    profondeurCible = profondeurMax;
  }
}

void setVitesse(uint8_t vitesse) {

  analogWrite(pinVitesseMoteur_1, vitesse);  //gère la vitesse de rotation du moteur 1
  analogWrite(pinVitesseMoteur_2, vitesse);  //gère la vitesse de rotation du moteur 2
}

void setBallast(int vitesseBallast) {
  // constante local
  static const bool sensBallast[3][2] = {
    { LOW, LOW },   // arreter
    { LOW, HIGH },  // monter
    { HIGH, LOW }   // descendre
  };
  // variable local
  uint8_t indice = 0;

  if (vitesseBallast >= 50) {
    analogWrite(pinVitesseBallast, vitesseBallast);
    indice = 1;
  } else if (vitesseBallast <= -50) {
    analogWrite(pinVitesseBallast, -vitesseBallast);
    indice = 2;
  }
  digitalWrite(pinBallast_1, sensBallast[indice][0]);
  digitalWrite(pinBallast_2, sensBallast[indice][1]);
}


// tester les tableaux avec 1 pour HIGH et 0 pour LOW
void setDirection(uint8_t direction) {
  // constante local
  static const bool directionMoteur_1[5][2] = {
    { LOW, LOW },   // arreter
    { LOW, HIGH },  // avancer
    { HIGH, LOW },  // touner à gauche
    { LOW, HIGH },  // tourner à droite
    { HIGH, LOW }   // reculer
  };
  static const bool directionMoteur_2[5][2] = {
    { LOW, LOW },   // arreter
    { LOW, HIGH },  // avancer
    { LOW, HIGH },  // touner à gauche
    { HIGH, LOW },  // tourner à droite
    { HIGH, LOW }   // reculer
  };

  direction -= 1;

  digitalWrite(pinMoteur_1_1, directionMoteur_1[direction][0]);
  digitalWrite(pinMoteur_1_2, directionMoteur_1[direction][1]);
  digitalWrite(pinMoteur_2_1, directionMoteur_2[direction][0]);
  digitalWrite(pinMoteur_2_2, directionMoteur_2[direction][1]);
}


/*|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
// PID

int controlePID(uint16_t deltaTemps, bool resetCoef, int profondeurMesure) {
  // constante local
  static const int vitesseBallastMin = -180;
  static const int vitesseBallastMax = 180;
  static const float Kp = 50;   // coeficient de la proportionnel
  static const float Ki = 0.5;  // coeficient de l'integrale
  static const float Kd = 25;   // coeficient de la dérivé

  // initialisation
  static int prec_P = 0;
  static float I = 0;

  // variable local
  if (resetCoef) {
    prec_P = 0;
    I = 0;
  }

  int P = profondeurCible - profondeurMesure;
  int D = P - prec_P;
  prec_P = P;
  I += P * deltaTemps / 1E3;

  if (I > vitesseBallastMax) {
    I = vitesseBallastMax;

  } else if (I < vitesseBallastMin) {
    I = vitesseBallastMin;
  }

  int outputPID = Kp * P + Ki * I + Kd * D;

  if (outputPID > vitesseBallastMax) {
    outputPID = vitesseBallastMax;
  } else if (outputPID < vitesseBallastMin) {
    outputPID = vitesseBallastMin;
  }

  return outputPID;
}

/*|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
// communication

// initialisation
#include <RCSwitch.h>

#define protocolVitesse 8    // vitesse (de 0 à 255 => max longueur 2^8)
#define protocolElevation 6  // elevation (de 0 à 15 => max longueur 2^4)
#define protocolDirection 4  // direction ( max longueur 2^3)
#define protocolReset 10     // reset (max longueur 2^1)
#define protocolInformation 18

#define pinRecepteur 2  // pin du transmetteur
#define pinEmetteur 3   // pin du récepteur

RCSwitch comSys = RCSwitch();  // création d'une intance nommé comSys à partir de l'objet RCSwitch;

void initCommunication() {

  pinMode(pinEmetteur, OUTPUT);
  comSys.enableTransmit(pinEmetteur);  // active le transmetteur
  comSys.setRepeatTransmit(10);        // Optional set number of transmission repetitions.
  comSys.setPulseLength(433);          // Optional set pulse length.

  pinMode(pinRecepteur, INPUT);
  comSys.enableReceive(digitalPinToInterrupt(pinRecepteur));  // Receiver on interrupt 0 => that is pin #2

  emission(protocolReset, 1);
  emission(protocolReset, 1);

  return;
}


// fonction
void emission(uint8_t protocol, uint32_t message) {
  static bool ID = 0;

  // Serial.println("envoyé ID = " + String(ID) + " protocol = " + String(protocol) + " message = " + String(message));

  comSys.send(message, protocol + ID);
  ID = !ID;
}


void reception() {
  static bool dernierID = 1;
  uint8_t protocol = comSys.getReceivedBitlength();  // donne la longueur du message

  bool ID = protocol % 2;

  if (ID != dernierID) {

    uint8_t message = comSys.getReceivedValue();  // donne le message

    // Serial.println("reçu ID = " + String(ID) + " protocol = " + String(protocol) + " message = " + String(message));

    protocol -= ID;
    dernierID = ID;

    switch (protocol) {
      case protocolVitesse:
        setVitesse(message);
        return;
      case protocolElevation:
        setProfondeur(message);
        return;
      case protocolDirection:
        setDirection(message);
        return;
      case protocolReset:
        pinMode(12, OUTPUT);
        digitalWrite(12, LOW);
        return;
    }
  }
  return;
}

/*|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
// reste

void setup() {

  // Serial.begin(9600);

  initMouvement();
  initCommunication();
}

void loop() {
  static uint32_t prevActualisation = millis();
  static uint32_t prevTemps = millis();
  static uint32_t prevConnexion = millis();
  static bool resetPID = 1;

  // sécurité surface si pas de réponse

  long profondeurMesure = getProfondeur();

  if ((millis() - prevConnexion) < 120000 && profondeurMesure > -5) {

    int outputPID = controlePID(millis() - prevTemps, resetPID, profondeurMesure);
    setBallast(outputPID);
    resetPID = 0;

  } else {
    if (!resetPID) {
      profondeurCible = 1;
      resetPID = 1;
      setBallast(-255);
    }
    if (profondeurMesure > -5 && profondeurMesure < 5 && profondeurCible != 0) {
      profondeurCible = 0;
      setBallast(0);
    }
  }

  prevTemps = millis();

  // vérifie si un message est reçu
  if (comSys.available()) {
    prevConnexion = millis();
    reception();  // traite le message

    comSys.resetAvailable();  // réactive l'antenne
  }

  if (millis() - prevActualisation > 1000) {

    emission(protocolInformation, profondeurCible * 1000 + profondeurMesure);  // emettre profondeur
    prevActualisation = millis();
  }
  // Serial.println("pression = " + String(bmp.getPressureSensor()) + " profondeurMesure = " + String(profondeurMesure));
}