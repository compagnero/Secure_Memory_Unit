# Projektbericht    

## Table of Contents
1. [Introduction](#introduction)
2. [Work Flow](#work_flow)
3. [Dokumentation](#installation)
4. [Literaturrecherche : Fragen & Begriffe](#usage)


## Einführung
In diesem Projekt geht es um die Secure Memory Unit, welche die folgenden Aufgaben erfüllt: 
-Speichert Daten verschlüsselt ab.
-Verwendet Address-Scrambling, um Angriffe abzuwehren.
-Prüft die Korrektheit von gespeicherten Daten. 

## Work Flow

### Grundlegende Implementation:
-address_scrambler + prng: Vuslat
-encrypt_decrypt: Ahmed
-parity_checker: Aziz
-secure_memory_unit: Ahmed + Vuslat
-run_simulation + rahmenprogramm: Aziz

### Testing und Korrektion:
Alle zusammen durch Kommunikation und Briefings.

## Dokumentation

1. PARITY_CHECKER : 
Berechnet die Parität bei Schreibvorgängen und speichert sie in einer unordered_map, die durch Byteadressen indiziert ist. Beim Schreiben wird die Parität als Bits innerhalb von Bytes gespeichert, die je nach Daten gesetzt oder gelöscht werden. Beim Lesen wird die Parität überprüft. Bei der Fault Injection wird das Paritätsbit invertiert, wenn das Fault-Bit den Wert 8 hat.

2. ADDRESS_SCRAMBLER : 
**Schreibzugriff:**
Ein Scrambling-Key wird generiert und in einer std::map gespeichert. Vier Subadressen (Startadresse bis Startadresse + 3) werden aus der Eingabeadresse erstellt, mit dem Scrambling-Key gescrambled (XOR) und an die SMU gesendet.

**Lesezugriff:**
Neue Adresse: Ein neuer Scrambling-Key wird generiert und gespeichert. Die Subadressen werden erstellt, gescrambled und an die SMU gesendet, um den Zugriff zu ermöglichen.
Bereits beschriebene Adresse: Der gespeicherte Scrambling-Key wird abgerufen, die Subadressen generiert und gescrambled, bevor sie an die SMU gesendet werden.

3. ENCRYPT_DECRYPT : 
**Schreibzugriff:**
-Das Submodul erhält die Daten aus wdata, teilt sie in vier Teile auf und verschlüsselt (XORed) sie mit dem bereits generierten Verschlüsselungsschlüssel, der auch in std::map gespeichert ist. Anschließend sendet es die verschlüsselten Daten an die SMU.
**Lesezugriff:**
-Das Submodul erhält die verschlüsselten Daten aus der SMU (tatsächlich aus dem Speicher), holt den Verschlüsselungsschlüssel aus der Map, entschlüsselt (XORed) die Daten und sendet sie weiter an die SMU.

4. SMU: 
**Schreibzugriff:**
Das Modul erhält die vier verschlüsselten Bytes vom ENCRYPT_DECRYPT-Submodul und die vier gescrambleten Subadressen vom ADDRESS_SCRAMBLER-Submodul. Es speichert die verschlüsselten Daten in den physischen (gescrambleten) Subadressen des Speichers. Am Ende wird die Berechnung des Paritätsbits durch das PARITY_CHECKER-Submodul durchgeführt.
**Lesezugriff:**
Das Modul erhält die vier gescrambleten Subadressen vom ADDRESS_SCRAMBLER, liest die vier verschlüsselten Bytes und sendet sie an das ENCRYPT_DECRYPT-Submodul, um sie zu entschlüsseln. Anschließend werden die vier entschlüsselten Bytes zusammengeführt, ein Parity-Check wird vom PARITY_CHECKER durchgeführt, und am Ende wird der entsprechende Wert in rdata geschrieben.
**Fault Injection:**
Eine Fault Injection kann während des Lese- oder Schreibzugriffs oder als separate Operation erfolgen. Dabei wird ein zufälliges Bit in den Daten oder Parity-Bits invertiert, um die Korrektheit der gespeicherten Daten zu prüfen. 

5. run_simulation :
Die Funktion run_simulation simuliert die SMU und führt Speicheroperationen durch. Sie verarbeitet Anfragen mit Lese-/Schreibsignalen, Fehlern und Fehlerbits und aktualisiert die SMU-Signale entsprechend. Sie verwaltet Taktzyklen, verfolgt Fehler und berechnet die primitive Gatternutzung auf der Grundlage der durchgeführten Operationen. Die Funktion unterstützt die Erstellung von Trace-Dateien zur Fehlersuche und Visualisierung. Die Simulationsergebnisse umfassen Zyklen-, Fehler- und primitive Gatteranzahl. Das Verhalten der SMU wird bei jeder Anfrage dynamisch aktualisiert, sodass eine genaue Modellierung von sicheren Speicheroperationen unter verschiedenen Bedingungen gewährleistet ist.

## Begriffe


**Symmetrischer Verschlüsselungsalgorithmus**:
Symmetrische Verschlüsselungsverfahren verwenden einen einzigen Schlüssel, der sowohl für die Verschlüsselung als auch für die Entschlüsselung erforderlich ist. Sie arbeiten effizient und bieten bei ausreichend langen Schlüsseln ein hohes Maß an Sicherheit.[1].

 **XOR Verschlüsselung:**
Der Schlüssel wird auf Ebene einzelner Bits mit den Bits des Klartextes durch eine XOR-Operation kombiniert[2].

**Address Scrambling:**
 Keine direkte Zuordnung zwischen der physischen Adresse einer bestimmten Datei und ihrer logischen Adresse[3].

**Pseudo-Random generator:**
Pseudozufallszahlengenerator (PRNG) ist ein Algorithmus, der mathematische Formeln benutzt,  um Sequenzen von Zufallszahlen zu generieren.[4].

**Parity bits:**
Das Paritätsbit einer Bitfolge wird als Ergänzungsbit verwendet, um sicherzustellen, dass die Gesamtanzahl der auf 1 gesetzten Bits (einschließlich des Paritätsbits) entweder gerade oder ungerade ist.[5].Das Paritätsbit kann überprüft werden, um Übertragungsfehler zu erkennen.[5].

# Fragen 
**kann mit dem Parity Checker jeder Fehler gefunden werden:**
Nein, zum Beispiel:
1. Data Bit Korruption [6]:
Alice : 1010**1** ------------------> Bob: 1100**1** (Parity Bit)
Bob meldet eine korrekte Übermittlung, obwohl sie eigentlich falsch ist. 
2. Parity Bit Korruption [6]:
Alice : 1010**1** ------------------> Bob: 1010**0** (Parity Bit)
Bob meldet eine falsche Übermittlung, obwohl  sie eigentlich korreKt ist. 
3. Beide [6]:
Alice : 1010**1** ------------------> Bob: 1011**0** (Parity Bit)

**Wie sicher sind die verwendeten Sicherheitsmechanismen? Gibt es mögliche Angriffe dagegen:**
1. Brute Force (eigene Idee, keine Quelle):
Schlüssel einfach zu kurz (256 mögliche Werte) 

2. Weak-PRNG [7]: 
Schwachstellen bei Pseudozufallszahlengeneratoren (PRNG) entstehen, wenn Entwickler anstelle eines kryptografisch sicheren PRNG (CSPRNG) einen herkömmlichen PRNG für kryptografische Zwecke einsetzen.

3. XOR-Verschlüsselung [8]:
Obwohl die XOR-Verschlüsselung sehr einfach ist, hat sie einige bedeutende Schwachstellen. Eine zentrale Schwäche ist ihre Anfälligkeit gegenüber Angriffen, bei denen der Klartext bekannt ist ("Plaintext Attacks"). Erhält ein Angreifer sowohl einen Teil des Klartexts als auch den entsprechenden Chiffretext, kann er die XOR-Operation dazu nutzen, den geheimen Verschlüsselungsschlüssel zu rekonstruieren.

Quellen:
[1] https://www.elektronik-kompendium.de/sites/net/1910101.htm
[2] https://www.mrge.de/lehrer/reif/sek2/Verschuesseln/xor/index.html
[3] https://fdtc.deib.polimi.it/FDTC10/shared/FDTC-2010-session-2-2.pdf  (Seite 6)
[4] https://www.geeksforgeeks.org/pseudo-random-number-generator-prng/
[5] https://de.wikipedia.org/wiki/Parit%C3%A4tsbit
[6] https://www.inetdaemon.com/tutorials/basic_concepts/communication/error_detection/parity/limitations.shtml
[7] https://developer.android.com/privacy-and-security/risks/weak-prng#:~:text=Weak%20PRNG%20vulnerabilities%20occur%20when,%2Dsecure%20PRNG%20(CSPRNG)
[8] https://bluegoatcyber.com/blog/how-is-xor-used-in-encryption/#:~:text=Limitations%20of%20XOR%20Encryption&text=One%20major%20limitation%20is%20its,secret%20key%20used%20for%20encryption
