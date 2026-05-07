# 🛠️ Manuel de Câblage — Smart Escape Box

Ce document détaille les connexions matérielles pour l'ESP32-S3 (YD-ESP32-S3 N16R8).

## 🔌 Tableau des Connexions

| Composant | Broche | GPIO ESP32 | Mode | Description |
| :--- | :--- | :--- | :--- | :--- |
| **Écran TFT** (ST7789) | CS | 10 | SPI | Selection esclave |
| | SCLK | 12 | SPI | Horloge SPI |
| | MOSI | 11 | SPI | Données SPI |
| | DC | 13 | Digital | Data/Command |
| | RST | 14 | Digital | Reset matériel |
| | BLK | 15 | PWM | Rétroéclairage |
| **Horloge RTC** (DS3231) | SDA | 8 | I2C | Données I2C |
| | SCL | 9 | I2C | Horloge I2C |
| **Joystick** | VRx | 1 | ADC | Axe Horizontal |
| | VRy | 2 | ADC | Axe Vertical |
| | SW | 3 | Digital | Bouton (Pull-up) |
| **Sonar** (HC-SR04) | TRIG | 6 | Digital | Impulsion départ |
| | ECHO | 7 | Digital | Retour d'écho |
| **Servo-Verrou** | PWM | 5 | PWM | Ouverture boîte |
| **Potentiomètre** | SIG | 4 | ADC | Énigme Radio |
| **Buzzer** (Passif) | SIG | 46 | PWM | Audio / Musique |
| **LED RGB** | R | 40 | PWM | Canal Rouge |
| | G | 41 | PWM | Canal Vert |
| | B | 42 | PWM | Canal Bleu |
| **Clavier 4x4** | R1..R4 | 18, 17, 16, 21 | Output | Rangées clavier |
| | C1..C4 | 47, 48, 38, 39 | Input | Colonnes clavier |

---

## ⚡ Alimentation & Masses

*   **VCC (3.3V)** : Alimente l'écran, la RTC, le Joystick, le Potentiomètre et le Sonar (si modèle 3.3V).
*   **VIN (5V)** : Alimente le Servo-moteur (via une source robuste).
*   **GND** : **IMPORTANT !** Reliez toutes les masses (GND) ensemble (ESP32, Alim externe, Capteurs).

## 💡 Conseils de montage

1.  **Servo** : Toujours utiliser une alimentation séparée du microcontrôleur pour le servo afin d'éviter les reboots intempestifs.
2.  **Sonar** : Si vous utilisez un HC-SR04 classique (5V), placez une résistance de 1kΩ en série sur la broche ECHO pour protéger l'ESP32.
3.  **Buzzer** : Le buzzer passif nécessite un signal PWM (géré par le `BuzzerEngine`). Ne pas le brancher directement sur une alimentation fixe.
4.  **GPIO Réservés** : Ne jamais utiliser les broches 26 à 37 sur ce modèle (N16R8), elles sont internes à la puce.
