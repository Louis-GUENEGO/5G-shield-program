// Projet météo 5G
// Programme de test de la conversion analogique numerique de la mesure de puissance, fonctionne avec shield détecteur de puissance fait maison
// Louis GUENEGO, louis.guenego@gmail.com
// Professeur RIVET & Professeur FERRE
// ENSEIRB-MATMECA, 29/04/2021


// Configuration CAN
#define PIN_ANALOGIQUE 15 // 15 => pin A0
#define RESOLUTION 12 // ADC 12 bits => 4096 values
#define PERIODE_CAN_MS 10 // période d'écahtillonage (en négligeant les traitements)
#define NOMBRE_MOYENNE 100


// Cartographie du capteur et étalonage
#define NBR_POINT 10
double  carto_dBm[NBR_POINT] = {   -30,   -25,   -20,  -15,   -10,    -5,    0,    5,   10,  12}; // correspond à l'axe y de la cartographie
double carto_Volt[NBR_POINT] = { 0.124, 0.129, 0.146, 0.19, 0.315, 0.543, 1.05, 1.94, 3.29, 3.3}; // @3.6GHz, correpond à l'axe x de la cartographie // carte connecteur SMA
// double carto_Volt[NBR_POINT] = { 0.1192, 0.1224, 0.1329, 0.1643, 0.255, 0.465, 0.904, 1.669, 3.02 }; // @3.6GHz, correpond à l'axe x de la cartographie // carte connecteur UMCC

#define GAIN_ANTENNE 32.74 // rapport dBm -> dBm/m


#define VISTESSE_USB 115200 // vitesse de l'USB


// Variables
unsigned short int i; // multi usage
unsigned short int moyenne = 1; // compter le nombre de moyenne
unsigned short int resultCAN = 0; // le résultat de la conversion A/N
double mesure = 0; // Le résultat de la conversion en Volts
double mesure_moy = 0; // Le résultat moyenné
double mesure_max = 0; // résultat max
double puissance_dBm = 0; // La puissance en dBm avant l'antenne
double puissance_dBm_max = 0;
double puissance = 0; // La puissance en W/m2 dans l'air
double puissance_max = 0;
double champElectrique = 0; // Le champs electrique en V/m dans l'air
double champElectrique_max = 0;



void setup() {
  pinMode(LED_BUILTIN, OUTPUT); // configuration de la LED
  
  analogReadResolution(RESOLUTION); // configuration du CAN
}




void loop() {
  
  resultCAN = analogRead(PIN_ANALOGIQUE); // Conversion A/N

  mesure = (double)resultCAN * 3.3 / 4095; // conversion en Volts

  if (mesure > mesure_max) {
    mesure_max = mesure;
  }

  mesure_moy = (mesure_moy*(moyenne-1) + mesure) / (moyenne) ; // moyenne des mesures

  moyenne++;
  
  if (moyenne >= NOMBRE_MOYENNE){ // lorsqu'on a suiffisament moyenné


    
    for (i = 0 ; i < NBR_POINT ; i++){ // scrutation de la cartographie
      if (mesure_moy <= carto_Volt [i]) {
        break;
      }
    }

    if (i == 0){ // approximation par interpolation linéaire de la cartographie
      puissance_dBm = carto_dBm[0];
    } else {
      puissance_dBm = (mesure_moy - carto_Volt[i-1]) * ( (carto_dBm[i]-carto_dBm[i-1]) / (carto_Volt[i]-carto_Volt[i-1]) ) + carto_dBm[i-1];
    }
    
    for (i = 0 ; i < NBR_POINT ; i++){ // scrutation de la cartographie
      if (mesure_max <= carto_Volt [i]) {
        break;
      }
    }

    if (i == 0){ // approximation par interpolation linéaire de la cartographie
      puissance_dBm_max = carto_dBm[0];
    } else {
      puissance_dBm_max = (mesure_max - carto_Volt[i-1]) * ( (carto_dBm[i]-carto_dBm[i-1]) / (carto_Volt[i]-carto_Volt[i-1]) ) + carto_dBm[i-1];
    }
    
    // Calcul des grandeur électromagnétiques
    puissance = pow (10, (puissance_dBm - 30 + GAIN_ANTENNE) / 10);
    puissance_max = pow (10, (puissance_dBm_max - 30 + GAIN_ANTENNE) / 10);
    champElectrique = sqrt(puissance * 377);
    champElectrique_max = sqrt(puissance_max * 377);

    // affichages des résultats sur le port série (terminal ordinateur) (partie inutile)
    Serial.begin(VISTESSE_USB);
    Serial.print(resultCAN); // cette valeur n'est pas moyennée
    Serial.print(" quantum\n");
    Serial.print(mesure, 3);
    Serial.print(" V\n");
    if ((puissance_dBm < -28.5) && (puissance_dBm_max < -24.5)){
      Serial.print("< -28.5 dBm  /!\\ Puissance trop faible\n");
      Serial.print("< 2.65 mW/m2  /!\\ Puissance trop faible\n");
      Serial.print("< 1 V/m    /!\\ Champ trop faible\n");
      Serial.print("\n");
    } else if (puissance > 5) {
      Serial.print(puissance_dBm, 3);
      Serial.print(" dBm   /!\\ PUISSANCE DANGEREUSE NE PAS DÉPASSER 12 dBm /!\\\n");
      Serial.print(puissance*1000, 3);
      Serial.print(" mW/m2  /!\\ PUISSANCE DANGEREUSE /!\\\n");
      Serial.print(champElectrique, 3);
      Serial.print(" V/m  /!\\ CHAMP DANGEREUX /!\\\n");
      Serial.print("\n");
    } else {
      Serial.print(puissance_dBm, 3);
      Serial.print(" dBm (max : ");
      Serial.print(puissance_dBm_max, 3);
      Serial.print(")\n");
      Serial.print(puissance*1000, 3);
      Serial.print(" mW/m2 (max : ");
      Serial.print(puissance_max*1000, 3);
      Serial.print(")\n");
      Serial.print(champElectrique, 3);
      Serial.print(" V/m  (max : ");
      Serial.print(champElectrique_max, 3);
      Serial.print(")\n");
      Serial.print("\n");
    }
    Serial.end();
    
    // réinitialisation des variables de la mesure et nombre de moyennes
    moyenne = 1;
    mesure_moy = 0;
    mesure_max = 0;
    

    // signal lumineux de la fin de conversion et temporisation avant nouvelle convesion A/N
    digitalWrite(LED_BUILTIN, HIGH);
    delay(PERIODE_CAN_MS);

  } else {

    // temporisation avant nouvelle conversion A/N
    digitalWrite(LED_BUILTIN, LOW); // On éteint la LED qui a été allumé lorqu'on a calculé un point de mesure
    delay(PERIODE_CAN_MS);
    
  }
  
}
