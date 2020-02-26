#include <IRrecv.h>
#include <IRsend.h>
#include <IRac.h>
#include <IRutils.h>

#include <WiFi.h>
#include <WiFiServer.h>

#include <aREST.h>

#include <M5StickC.h>

// Create aREST instance
aREST rest = aREST();

#define LED_PIN     GPIO_NUM_10
#define IR_RECV_PIN  GPIO_NUM_33
//#define IR_SEND_PIN   GPIO_NUM_9
#define IR_SEND_PIN   GPIO_NUM_26

IRrecv irrecv(IR_RECV_PIN, 1024, 50, true);
decode_results results;
IRsend irsend(IR_SEND_PIN);

#define IR_MAX_SEND_DATA 300
uint16_t send_data[IR_MAX_SEND_DATA];
uint16_t send_len = 0;

unsigned long recvStartTime;
unsigned long recvDuration = 0;

// WiFi parameters
const char* ssid = "【WiFiのSSID】";
const char* password = "【WiFiのパスワード】";

WiFiServer server(80);

void print_screen(String message, int font_size = 2){
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(font_size);

  M5.Lcd.println(message);
}

void print_screen_next(String message){
  M5.Lcd.println(message);
}

void setup()
{
  M5.begin();
  M5.IMU.Init();

  M5.Axp.ScreenBreath(9);
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);

  M5.Lcd.println("[M5StickC]");
  delay(500);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  irsend.begin();

  // Start Serial
  Serial.begin(9600);

  // Init variables and expose them to REST API

  // Function to be exposed
  rest.function("led",ledControl);
  rest.function("start", irStart);
  rest.function("stop", irStop);
  rest.function("send", irSend);
  rest.function("get", irGet);

  // Give name & ID to the device (ID should be 6 characters long)
  rest.set_id("0001");
  rest.set_name("esp32");

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());
  M5.Lcd.println(WiFi.localIP());
}

void loop() {
  M5.update();

  if ( M5.BtnA.wasReleased() ) {
    // M5Stick-Cのボタンが押下されたら、直近のリモコン信号を送信
    Serial.println("BtnA.released");
    print_screen("BtnA.released start", 2);
    if( send_len > 0 )
      irsend.sendRaw((uint16_t*)send_data, send_len, 38);
    print_screen_next("BtnA.released end");
  }

  if (irrecv.decode(&results)) {
    // リモコン信号の受信が受信された
    irrecv.resume(); // Receive the next value

    if( results.rawlen <= IR_MAX_SEND_DATA ){
      ir_stop();

      // 受信したリモコン信号をバッファに格納
      uint16_t * result = resultToRawArray(&results);
      send_len = getCorrectedRawLength(&results);
      for( int i = 0 ; i < send_len ; i++ )
        send_data[i] = result[i];
      delete[] result;

      Serial.println("IR received");
      print_screen_next("IR received");
    }else{
      Serial.println("IR size over");
      print_screen_next("IR size over");
    }

    Serial.print(resultToHumanReadableBasic(&results));
    String description = IRAcUtils::resultAcToString(&results);
    if (description.length())
      Serial.println("Mesg Desc.: " + description);
//    Serial.println(resultToTimingInfo(&results));
    Serial.println(resultToSourceCode(&results));

    delay(100);
  }

  if( recvDuration > 0 ){
    // リモコン受信待ち時間タイムアウト
    unsigned long elapsed = millis() - recvStartTime;
    if( elapsed >= recvDuration ){
      Serial.println("Expired");
      ir_stop();
    }
  }

  // Handle REST calls
  WiFiClient client = server.available();
  if (client) {
    // GET呼び出しを検知
    for( int i = 0 ; i < 10000; i += 10 ){
      if(client.available()){
        // GET呼び出しのコールバック呼び出し
        rest.handle(client);
        return;
      }
      delay(10);
    }
    // まれにGET呼び出し受付に失敗するようです。
    Serial.println("timeout");
  }
}

// Custom function accessible by the API

String ledControl(String command) {
  Serial.println("ledControl called");

  // Get state from command
  int state = command.toInt();

  digitalWrite(LED_PIN, state);

  return "OK";
}

String irSend(String command) {
  Serial.println("irSend called");

  send_len = hexstr2array(command, send_data, IR_MAX_SEND_DATA);
  if( send_len > 0 ){
    print_screen("IR send", 2);

    Serial.println(send_data[0]);
    Serial.println("...");
    Serial.println(send_data[send_len - 1]);

    irsend.sendRaw((uint16_t*)send_data, send_len, 38);

    return "OK";
  }else{
    return "NG";
  }
}

void ir_start(unsigned long duration){
  if( duration == 0 ){
    ir_stop();
  }else{
    if( recvDuration == 0 ){
      irrecv.enableIRIn();
    }

    send_len = 0;
    recvStartTime = millis();
    recvDuration = duration;

    print_screen("IR scan start", 2);
  }
}

void ir_stop(void){
  if( recvDuration > 0 ){
    pinMode(IR_RECV_PIN, OUTPUT);
    recvDuration = 0;

    print_screen_next("IR scan stopped");
  }
}

String irStart(String command){
  Serial.println("irStart called");

  unsigned long duration = command.toInt();

  ir_start(duration);

  return "OK";
}

String irStop(String command){
  Serial.println("irStop called");

  ir_stop();

  return "OK";
}

String irGet(String command){
  Serial.println("irGet called");

  if( send_len > 0 ){
    String hexstr = array2hexstr(send_data, send_len);
    return hexstr;
  }else{
    return "NG";
  }
}

int char2int(char c){
  if( c >= '0' && c <= '9' )
    return c - '0';
  if( c >= 'a' && c <= 'f' )
    return c - 'a' + 10;
  if( c >= 'A' && c <= 'F' )
    return c- 'A' + 10;

  return 0;
}

char int2char(int i){
  if( i >= 0 && i <= 9 )
    return '0' + i;
  if( i >= 10 && i <= 15 )
    return 'a' + (i - 10);

  return '0';
}

int hexstr2array(String str, uint16_t *array, int maxlen){
  int len = str.length();
  if( (len % 4) != 0 )
    return -1;
  if( len / 4 > maxlen )
    return -1;

  for( int i = 0 ; i < len ; i += 4 ){
    uint16_t value = char2int(str.charAt(i)) << 12;
    value += char2int(str.charAt(i + 1)) << 8;
    value += char2int(str.charAt(i + 2)) << 4;
    value += char2int(str.charAt(i + 3));
    array[i / 4] = value;
  }

  return len / 4;
}

String array2hexstr(uint16_t *array, int len){
  String str = "";
  for( int i = 0 ; i < len * 4 ; i++ )
    str.concat("0");

  for( int i = 0 ; i < len ; i++ ){
      str.setCharAt(i * 4, int2char((array[i] >> 12) & 0x0f));
      str.setCharAt(i * 4 + 1, int2char((array[i] >> 8) & 0x0f));
      str.setCharAt(i * 4 + 2, int2char((array[i] >> 4) & 0x0f));
      str.setCharAt(i * 4 + 3, int2char((array[i]) & 0x0f));
  }

  return str;
}