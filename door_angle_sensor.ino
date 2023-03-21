/*!
   @file readAllData.ino
   @brief Through the example, you can get the sensor data by using getSensorData:
   @n     get all data of magnetometer, gyroscope, accelerometer.
   @n     With the rotation of the sensor, data changes are visible.
   @copyright  Copyright (c) 2010 DFRobot Co.Ltd (http://www.dfrobot.com)
   @license     The MIT License (MIT)
   @author [luoyufeng] (yufeng.luo@dfrobot.com)
   @maintainer [Fary](feng.yang@dfrobot.com)
   @version  V1.0
   @date  2021-10-20
   @url https://github.com/DFRobot/DFRobot_BMX160
*/
#include <DFRobot_BMX160.h>
String hostname = "CO2-LHC-06";

#include <U8g2lib.h>
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

#include <ESP32WebServer.h>    //https://github.com/Pedroalbuquerque/ESP32WebServer download and place in your Libraries folder
#include <ESPmDNS.h>
#include <WiFi.h>
#include <HTTPClient.h>
WiFiClient client;

#include "CSS.h" //Includes headers of the web and de style file

#include "FS.h"
#include "SD.h"
#include <SPI.h>



ESP32WebServer server(80);
bool   SD_present = false; //Controls if the SD card is present or not

#define servername "Vyoman" //Define the name to your server... 
char ssid[] = "Vyoman";      //  network SSID (name)
char pass[] = "Vyoman@CleanAirIndia";   //  network password
String Device_Token = "64abb8b4-678e-46a4-a78d-c99a27805b06";
int keyIndex = 0;
char tagoServer[] = "api.tago.io";
#include "RTClib.h"
RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
#include "FS.h"
#include "SD.h"
#include <Wire.h>
#include "SparkFun_SCD30_Arduino_Library.h" //Click here to get the library: http://librarymanager/All#SparkFun_SCD30
SCD30 airSensor;

int co2 = 0;
float t = 0;
float h = 0;
float magini = 0;
float heading_in_degree = 0;

int count = 0;

String dataMessage;
String timeStamp;
int wifiTime = 0;
String Dates = "";

float anglex, angley, anglez;
float initX, initY, initZ;

const int buttonPin = 27;
void connectWiFi()
{
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname(hostname.c_str()); //define hostname
  u8g2.clearBuffer();
  u8g2.sendBuffer();
  WiFi.begin(ssid, pass);
  Serial.println("Connecting");
  u8g2.drawStr(3, 30, "Connecting WiFi");
  u8g2.sendBuffer();
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
    wifiTime++;
    if (wifiTime > 20)
    {
      Serial.println("WiFi not connected. Locally logged");
    }
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  u8g2.drawStr(3, 45, "IP:");
  String ips = WiFi.localIP().toString();
  const char* ip = ips.c_str();
  u8g2.drawStr(3, 60, ip);
  u8g2.sendBuffer();

}





void cloudUpload()
{
  connectWiFi();
  String variables[] = {"CO2", "temp", "hum"};
  float values[] = {co2, t, h};
  Serial.println("\nStarting connection to server...");
  for (int i = 0; i < (sizeof(values) / sizeof(int)); i++)
  {
    // if you get a connection, report back via serial:
    //String PostData = String("{\"variable\":\"temperature\", \"value\":") + String(t)+ String(",\"unit\":\"C\"}");
    String PostData = String("{\"variable\":") + String("\"") + String(variables[i]) + String("\"") + String(",\"value\":") + String(values[i]) + String("}");
    String Dev_token = String("Device-Token: ") + String(Device_Token);
    if (client.connect(tagoServer, 80))
    {
      Serial.println("connected to server");
      // Make a HTTP request:
      client.println("POST /data? HTTP/1.1");
      client.println("Host: api.tago.io");
      client.println("_ssl: false");       // for non-secured connection, use this option "_ssl: false"
      client.println(Dev_token);
      client.println("Content-Type: application/json");
      client.print("Content-Length: ");
      client.println(PostData.length());
      client.println();
      client.println(PostData);
    }
    else
    {
      // if you couldn't make a connection:
      Serial.println("connection failed");
    }
  }
  Serial.println();
  delay(2000);
  client.stop();
  WiFi.disconnect();
  delay(2000);
}


void readTime()
{
  DateTime now = rtc.now();
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" (");
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  Serial.print(") ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
  timeStamp = String(now.day()) + "/" + String(now.month()) + "/" + String(now.year()) + "," + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()) + ", " ;
  const char* ts = timeStamp.c_str();

  u8g2.drawStr(3, 15, ts);
  u8g2.sendBuffer();

  logSDCard();
}

void logSDCard()
{
  dataMessage =  timeStamp  +   ","  + String(heading_in_degree) + "\r\n";
  Serial.print("Save data: ");
  Serial.println(dataMessage);
  appendFile(SD, "/data.txt", dataMessage.c_str());
}

// Write to the SD card (DON'T MODIFY THIS FUNCTION)
void writeFile(fs::FS &fs, const char * path, const char * message)
{
  Serial.printf("Writing file: %s\n", path);
  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  }
  else
  {
    Serial.println("Write failed");
  }
  file.close();
}


// Append data to the SD card (DON'T MODIFY THIS FUNCTION)
void appendFile(fs::FS &fs, const char * path, const char * message)
{
  Serial.printf("Appending to file: %s\n", path);
  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
    delay(1000);
  }
  else
  {
    Serial.println("Append failed");
  }
  file.close();
}


void updateRtc() {
  int ye = Dates.substring(0, 4).toInt();
  int mo = Dates.substring(4, 6).toInt();
  int da = Dates.substring(6, 8).toInt();
  int ho = Dates.substring(8, 10).toInt();
  int mi = Dates.substring(10, 12).toInt();
  int se = Dates.substring(12, 14).toInt();                //






  rtc.adjust(DateTime( ye , mo , da , ho , mi , se ));
  Serial.println("RTC adjusted! " + String(ye) + " " + String(mo) + " " +  String(da) + " " +  String(ho) + " " +  String(mi) + " " +  String(se));


}
//Initial page of the server web, list directory and give you the chance of deleting and uploading
void SD_dir()
{
  if (SD_present)
  {
    //Action acording to post, dowload or delete, by MC 2022
    if (server.args() > 0 ) //Arguments were received, ignored if there are not arguments
    {
      Serial.println(server.arg(0));

      String Order = server.arg(0);
      Serial.println(Order);

      if (Order.indexOf("download_") >= 0)
      {
        Order.remove(0, 9);
        SD_file_download(Order);
        Serial.println(Order);
      }

      if ((server.arg(0)).indexOf("delete_") >= 0)
      {
        Order.remove(0, 7);
        SD_file_delete(Order);
        Serial.println(Order);
      }
    }

    File root = SD.open("/");
    if (root) {
      root.rewindDirectory();
      SendHTML_Header();
      webpage += F("<table align='center'>");
      webpage += F("<tr><th>Name/Type</th><th style='width:20%'>Type File/Dir</th><th>File Size</th></tr>");
      printDirectory("/", 0);
      webpage += F("</table>");
      SendHTML_Content();
      root.close();
    }
    else
    {
      SendHTML_Header();
      webpage += F("<h3>No Files Found</h3>");
    }
    append_page_footer();
    SendHTML_Content();
    SendHTML_Stop();   //Stop is needed because no content length was sent
  } else ReportSDNotPresent();
}

//Upload a file to the SD
void File_Upload()
{
  append_page_header();
  webpage += F("<h3>Select File to Upload</h3>");
  webpage += F("<FORM action='/fupload' method='post' enctype='multipart/form-data'>");
  webpage += F("<input class='buttons' style='width:25%' type='file' name='fupload' id = 'fupload' value=''>");
  webpage += F("<button class='buttons' style='width:10%' type='submit'>Upload File</button><br><br>");
  webpage += F("<a href='/'>[Back]</a><br><br>");
  append_page_footer();
  server.send(200, "text/html", webpage);
}

//Prints the directory, it is called in void SD_dir()
void printDirectory(const char * dirname, uint8_t levels)
{

  File root = SD.open(dirname);

  if (!root) {
    return;
  }
  if (!root.isDirectory()) {
    return;
  }
  File file = root.openNextFile();

  int i = 0;
  while (file) {
    if (webpage.length() > 1000) {
      SendHTML_Content();
    }
    if (file.isDirectory()) {
      webpage += "<tr><td>" + String(file.isDirectory() ? "Dir" : "File") + "</td><td>" + String(file.name()) + "</td><td></td></tr>";
      printDirectory(file.name(), levels - 1);
    }
    else
    {
      webpage += "<tr><td>" + String(file.name()) + "</td>";
      webpage += "<td>" + String(file.isDirectory() ? "Dir" : "File") + "</td>";
      int bytes = file.size();
      String fsize = "";
      if (bytes < 1024)                     fsize = String(bytes) + " B";
      else if (bytes < (1024 * 1024))        fsize = String(bytes / 1024.0, 3) + " KB";
      else if (bytes < (1024 * 1024 * 1024)) fsize = String(bytes / 1024.0 / 1024.0, 3) + " MB";
      else                                  fsize = String(bytes / 1024.0 / 1024.0 / 1024.0, 3) + " GB";
      webpage += "<td>" + fsize + "</td>";
      webpage += "<td>";
      webpage += F("<FORM action='/' method='post'>");
      webpage += F("<button type='submit' name='download'");
      webpage += F("' value='"); webpage += "download_" + String(file.name()); webpage += F("'>Download</button>");
      webpage += "</td>";
      webpage += "<td>";
      webpage += F("<FORM action='/' method='post'>");
      webpage += F("<button type='submit' name='delete'");
      webpage += F("' value='"); webpage += "delete_" + String(file.name()); webpage += F("'>Delete</button>");
      webpage += "</td>";
      webpage += "</tr>";

    }
    file = root.openNextFile();
    i++;
  }
  file.close();


}

//Download a file from the SD, it is called in void SD_dir()
void SD_file_download(String filename)
{
  if (SD_present)
  {
    File download = SD.open("/" + filename);
    if (download)
    {
      server.sendHeader("Content-Type", "text/text");
      server.sendHeader("Content-Disposition", "attachment; filename=" + filename);
      server.sendHeader("Connection", "close");
      server.streamFile(download, "application/octet-stream");
      download.close();
    } else ReportFileNotPresent("download");
  } else ReportSDNotPresent();
}

//Handles the file upload a file to the SD
File UploadFile;
//Upload a new file to the Filing system
void handleFileUpload()
{
  HTTPUpload& uploadfile = server.upload(); //See https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer/srcv
  //For further information on 'status' structure, there are other reasons such as a failed transfer that could be used
  if (uploadfile.status == UPLOAD_FILE_START)
  {
    String filename = uploadfile.filename;
    if (!filename.startsWith("/")) filename = "/" + filename;
    Serial.print("Upload File Name: "); Serial.println(filename);
    SD.remove(filename);                         //Remove a previous version, otherwise data is appended the file again
    UploadFile = SD.open(filename, FILE_WRITE);  //Open the file for writing in SD (create it, if doesn't exist)
    filename = String();
  }
  else if (uploadfile.status == UPLOAD_FILE_WRITE)
  {
    if (UploadFile) UploadFile.write(uploadfile.buf, uploadfile.currentSize); // Write the received bytes to the file
  }
  else if (uploadfile.status == UPLOAD_FILE_END)
  {
    if (UploadFile)         //If the file was successfully created
    {
      UploadFile.close();   //Close the file again
      Serial.print("Upload Size: "); Serial.println(uploadfile.totalSize);
      webpage = "";
      append_page_header();
      webpage += F("<h3>File was successfully uploaded</h3>");
      webpage += F("<h2>Uploaded File Name: "); webpage += uploadfile.filename + "</h2>";
      webpage += F("<h2>File Size: "); webpage += file_size(uploadfile.totalSize) + "</h2><br><br>";
      webpage += F("<a href='/'>[Back]</a><br><br>");
      append_page_footer();
      server.send(200, "text/html", webpage);
    }
    else
    {
      ReportCouldNotCreateFile("upload");
    }
  }
}

//Delete a file from the SD, it is called in void SD_dir()
void SD_file_delete(String filename)
{
  if (SD_present) {
    SendHTML_Header();
    File dataFile = SD.open("/" + filename, FILE_READ); //Now read data from SD Card
    if (dataFile)
    {
      if (SD.remove("/" + filename)) {
        Serial.println(F("File deleted successfully"));
        webpage += "<h3>File '" + filename + "' has been erased</h3>";
        webpage += F("<a href='/'>[Back]</a><br><br>");
      }
      else
      {
        webpage += F("<h3>File was not deleted - error</h3>");
        webpage += F("<a href='/'>[Back]</a><br><br>");
      }
    } else ReportFileNotPresent("delete");
    append_page_footer();
    SendHTML_Content();
    SendHTML_Stop();
  } else ReportSDNotPresent();
}

//SendHTML_Header
void SendHTML_Header()
{
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", ""); //Empty content inhibits Content-length header so we have to close the socket ourselves.
  append_page_header();
  server.sendContent(webpage);
  webpage = "";
}

//SendHTML_Content
void SendHTML_Content()
{
  server.sendContent(webpage);
  webpage = "";
}

//SendHTML_Stop
void SendHTML_Stop()
{
  server.sendContent("");
  server.client().stop(); //Stop is needed because no content length was sent
}

//ReportSDNotPresent
void ReportSDNotPresent()
{
  SendHTML_Header();
  webpage += F("<h3>No SD Card present</h3>");
  webpage += F("<a href='/'>[Back]</a><br><br>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}

//ReportFileNotPresent
void ReportFileNotPresent(String target)
{
  SendHTML_Header();
  webpage += F("<h3>File does not exist</h3>");
  webpage += F("<a href='/"); webpage += target + "'>[Back]</a><br><br>";
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}

//ReportCouldNotCreateFile
void ReportCouldNotCreateFile(String target)
{
  SendHTML_Header();
  webpage += F("<h3>Could Not Create Uploaded File (write-protected?)</h3>");
  webpage += F("<a href='/"); webpage += target + "'>[Back]</a><br><br>";
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}

//File size conversion
String file_size(int bytes)
{
  String fsize = "";
  if (bytes < 1024)                 fsize = String(bytes) + " B";
  else if (bytes < (1024 * 1024))      fsize = String(bytes / 1024.0, 3) + " KB";
  else if (bytes < (1024 * 1024 * 1024)) fsize = String(bytes / 1024.0 / 1024.0, 3) + " MB";
  else                              fsize = String(bytes / 1024.0 / 1024.0 / 1024.0, 3) + " GB";
  return fsize;
}
void timeSet() {
  append_page_header();
  webpage += F("<h3>Update RTC (YYYYMMDDHHMMSS)</h3>");
  webpage += F("<FORM action='/get' method='get'>");
  webpage += F("<input type='text'name='setTime' id='setTime' value='' style='width:25%'>");
  webpage += F("<button class='buttons' style='width:10%' type='submit'>Update</button><br><br>");
  webpage += F("<a href='/'>[Back]</a><br><br>");
  append_page_footer();
  server.send(200, "text/html", webpage);
}

void dataTransfer()
{
  digitalWrite(33, HIGH);
  u8g2.clearBuffer();
  u8g2.drawStr(3, 15, "FTP Enabled");
  u8g2.sendBuffer();

  if (WiFi.status() != WL_CONNECTED)
  {
    connectWiFi();

    server.on("/",         SD_dir);
    server.on("/upload",   File_Upload);
    server.on("/setTime",  timeSet);
    server.on("/get", HTTP_GET, []() {


      server.send(200, "text/plain", "this works as well");

      Serial.println(server.arg(0));
      Dates = server.arg(0);
      Serial.println(server.arg(0));
      updateRtc();
    });
    server.on("/fupload",  HTTP_POST, []() {
      server.send(200);
    }, handleFileUpload);
    server.begin();

    //Set your preferred server name, if you use "mcserver" the address would be http://mcserver.local/
    /*if (!MDNS.begin(servername))
      {
      Serial.println(F("Error setting up MDNS responder!"));
      ESP.restart();
      }*/
  }
  for (;;) {
    server.handleClient();
  }
}

DFRobot_BMX160 bmx160;
void setup() {
  pinMode(32, INPUT_PULLUP);

  pinMode(33, OUTPUT);
  digitalWrite(33, HIGH);
  Serial.begin(115200);
  delay(100);

  //init the hardware bmx160
  if (bmx160.begin() != true) {
    Serial.println("init false");
    while (1);
  }
  //bmx160.setLowPower();   //disable the gyroscope and accelerometer sensor
  //bmx160.wakeUp();        //enable the gyroscope and accelerometer sensor
  //bmx160.softReset();     //reset the sensor

  /**
     enum{eGyroRange_2000DPS,
           eGyroRange_1000DPS,
           eGyroRange_500DPS,
           eGyroRange_250DPS,
           eGyroRange_125DPS
           }eGyroRange_t;
   **/
  //bmx160.setGyroRange(eGyroRange_500DPS);

  /**
      enum{eAccelRange_2G,
           eAccelRange_4G,
           eAccelRange_8G,
           eAccelRange_16G
           }eAccelRange_t;
  */
  //bmx160.setAccelRange(eAccelRange_4G);
  delay(100);

  u8g2.begin();

  u8g2.clearBuffer();         // clear the internal memory
  u8g2.setFont(u8g2_font_ncenB10_tr); // choose a suitable font
  u8g2.drawStr(3, 15, "Initializing");
  u8g2.sendBuffer();



  Serial.print(F("Initializing SD card..."));

  delay(2000);

  //see if the card is present and can be initialised.
  //Note: Using the ESP32 and SD_Card readers requires a 1K to 4K7 pull-up to 3v3 on the MISO line, otherwise may not work
  if (!SD.begin(4))
  {
    Serial.println("SD Card Initialization Failed!");
    u8g2.drawStr(3, 30, "SD Failed");
    u8g2.sendBuffer();
  }
  else
  {
    Serial.println(F("Card initialised... file access enabled..."));
    u8g2.drawStr(3, 30, "SD Enabled");
    u8g2.sendBuffer();
    SD_present = true;
  }

  /*********  Server Commands  **********/
  delay(2000);


  if (! rtc.begin())
  {
    Serial.println("Couldn't find RTC");
    u8g2.drawStr(3, 45, "RTC Error");
    u8g2.sendBuffer();
  }
  u8g2.drawStr(3, 45, "RTC Enabled");
  u8g2.sendBuffer();
  //Uncomment to adjust the date and time. Comment it again after uploading in the node.
  //rtc.adjust(DateTime(2022, 5, 3, 11, 20, 0));



  delay(2000);
  sBmx160SensorData_t Omagn, Ogyro, Oaccel;

  /* Get a new sensor event */
  bmx160.getAllData(&Omagn, &Ogyro, &Oaccel);

  magini = 180 * atan2(Omagn.y, Omagn.x) / PI;
  if ( magini < 0) {
    magini = 360 + magini;
  }

  delay(2000);
  digitalWrite(33, LOW);
  u8g2.clearBuffer();
  u8g2.sendBuffer();


}

void loop() {

  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (digitalRead(32) == 0)
  {
    digitalWrite(33, HIGH);
    delay(1000);
    digitalWrite(33, LOW);
    delay(1000);
    dataTransfer();
  }

  digitalWrite(33, LOW);
  sBmx160SensorData_t Omagn, Ogyro, Oaccel;

  /* Get a new sensor event */
  bmx160.getAllData(&Omagn, &Ogyro, &Oaccel);

  /* Display the magnetometer results (magn is magnetometer in uTesla) */
  Serial.print("M ");
  Serial.print("X: "); Serial.print(Omagn.x); Serial.print("  ");
  Serial.print("Y: "); Serial.print(Omagn.y); Serial.print("  ");
  Serial.print("Z: "); Serial.print(Omagn.z); Serial.print("  ");
  Serial.println("uT");

  /* Display the gyroscope results (gyroscope data is in g) */
  Serial.print("G ");
  Serial.print("X: "); Serial.print(Ogyro.x); Serial.print("  ");
  Serial.print("Y: "); Serial.print(Ogyro.y); Serial.print("  ");
  Serial.print("Z: "); Serial.print(Ogyro.z); Serial.print("  ");
  Serial.println("g");

  /* Display the accelerometer results (accelerometer data is in m/s^2) */
  Serial.print("A ");
  Serial.print("X: "); Serial.print(Oaccel.x    ); Serial.print("  ");
  Serial.print("Y: "); Serial.print(Oaccel.y    ); Serial.print("  ");
  Serial.print("Z: "); Serial.print(Oaccel.z    ); Serial.print("  ");
  Serial.println("m/s^2");

  Serial.println("");
  char buffers[10];
  heading_in_degree = (180 * atan2(Omagn.y, Omagn.x) / PI);
  if ( heading_in_degree < 0) {
    heading_in_degree = 360 + heading_in_degree;
  }
  heading_in_degree = heading_in_degree - magini;
  Serial.print("mag angle: ");
  Serial.println(heading_in_degree);
  String datas = "\"" + String( heading_in_degree) + "\"";
  const char* astar = datas.c_str();
  u8g2.clearBuffer();
  u8g2.drawStr(3, 30, astar);
  u8g2.drawStr(3, 50, "LEFT");
  u8g2.sendBuffer();

  readTime();
  count++;
  Serial.println();


  delay(500);



}
