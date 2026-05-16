#ifndef NETWORK_REPORT_H
#define NETWORK_REPORT_H

#include <Arduino.h>
#include "Hardware_Config.h"

void sendReport() {
    espSerial.println("AT");
    delay(1000);
    espSerial.println("AT+CWJAP=\"SSID\",\"PASSWORD\"");
    delay(5000);
    espSerial.println("AT+CIPSTART=\"TCP\",\"api.callmebot.com\",80");
    delay(2000);
    espSerial.println("AT+CIPSEND=100");
    delay(2000);
    espSerial.println("GET /whatsapp.php?phone=+20123456789&text=Attendance+Report+Sent&apikey=123456 HTTP/1.1\r\nHost: api.callmebot.com\r\nConnection: close\r\n\r\n");
    delay(2000);
}

#endif
