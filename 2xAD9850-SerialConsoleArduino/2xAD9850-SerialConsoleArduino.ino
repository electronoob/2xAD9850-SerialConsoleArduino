#include <util/delay.h>
/*

  Dual AD9850 DDS modules connected to Arduino, providing a Serial Console style management port.
  Use putty in Serial com port mode to connect to the device, make sure baud rate is correct...
  In this source code I have set it to 115200 in the setup() function..
  Once connected serially to the arduino .. you have 2 commands.

  set frequency
  and
  set sweep

  both commands will prompt for further information as required.

  2xAD9850-SerialConsoleArduino (git) [http://electronoob.com]
  Released under the terms of the MIT license.
  Copyright (c) 2014 electronoob.
  All rights reserved.

*/

#define HOST_NAME "µDDS"
#define HOST_ID 1
#define HOST_BANNER \
  " µß°× Dual DDS terminal interface written by electronoob. \r\n"\
  "\r\n"\
  "Press enter to begin."
#define LOGIN_PROMPT "Login: "
#define PASSWORD_PROMPT "Password: "
#define mubox_username "root"
#define mubox_password "toor"

#define CLK 9
#define FQ 10
#define DATA 11
#define RST 12

#define CLK2 8
#define FQ2 7
#define DATA2 6
#define RST2 5

//pinmode output
#define po(x) pinMode(x, OUTPUT); digitalWrite(x, LOW);
//pulse high to low
#define ph(x) digitalWrite(x, HIGH); _delay_us(1); digitalWrite(x, LOW);
//shift bit
#define sb(x) digitalWrite(DATA, x); _delay_us(1); ph(CLK);
#define sb2(x) digitalWrite(DATA2, x); _delay_us(1); ph(CLK2);
// calculate frequency - size of tuning word divided by oscillator
// clock frequency on the dds module, multiplied by the frequency
// in hertz. datasheet says 2^32 but that overflows.
// ffffffff which seems to work nicely.
#define f(x) (double)x * 0xFFFFFFFF / 125000000

boolean have_tty = 0;
boolean have_auth = 0;
int ticks = 0;

void setup() {
  Serial.begin(115200);
  /* disable local echo - turn on send-receive mode */
  vt100_ESC("e[12h");
  show_tty_welcome();
}
void loop() {
  // checks if connected yet - if not, show mubox header
  show_banner(); 
  // next we make sure authorized
  if (show_authorize()) {
    // repl loop
    if ((have_tty) && (have_auth)) {
      vt100_color_dim_white_normal ();
      repl_loop();
      ticks = 0;
      have_tty = 0;
      have_auth = 0;
      vt100_color_dim_green_normal ();
      echo ("\r\nLogged out.\r\n");
      delay(2000);
      show_tty_welcome();
      return;
    }
  } else {
    delay(3000);
    ticks = 0;
    have_tty = 0;
    have_auth = 0;
    show_tty_welcome();
    return;
  }
  delay(100);
  ticks++;
}
void repl_loop(){
  for(;;) {
    echo ("\r\nDDS µ ");
    char *line = get_line(0);
    repl_parse(line);
  }
}
void repl_parse(char *line) {
  if (strcmp(line, "set") == 0){
    echo ("\r\nArguments for set are either 'frequency' or 'sweep'\r\n ");
    return;
  }
  if (strcmp(line, "set frequency") == 0){
    echo ("\r\ndds mode, enter frequency in hz? ");
    char *freq = get_line(0);
    double freq_d = strtod (freq ,NULL);
    echo ("\r\nwhich /output/ to modify? ");
    freq = get_line(0);
    long freq_module = strtol (freq ,NULL,10);
    if(freq_module == 1) {
      po(CLK);po(FQ);po(DATA);po(RST);
      //init
      ph(RST);ph(CLK);ph(FQ);
    } else {
      po(CLK2);po(FQ2);po(DATA2);po(RST2);
      //init
      ph(RST2);ph(CLK2);ph(FQ2);
    }
    int32_t tw = f(freq_d);
    for (int i=0; i<32; i++, tw >>=1) {
      if(freq_module == 1) {
        sb(tw & 1);
      }else{
        sb2(tw & 1);
      }
    }
    // send 00000000's 
    for (int i=0; i<8; i++) {
      if(freq_module == 1) {
        sb(0);
      } else {
        sb2(0);
      }
    }
    if(freq_module == 1) {
      ph(FQ);
    } else {
      ph(FQ2);
    }
    echo ("\r\nDDS Setup and configured for frequency ");
    Serial.print(freq_d);
    echo ("hz.\r\n");
    return;
  }
  if (strcmp(line, "set sweep") == 0){
    echo ("\r\ndds mode, enter /start/ frequency in hz? ");
    char *freq = get_line(0);
    double freq_start = strtod (freq ,NULL);
    echo ("\r\nenter /end/ frequency in hz? ");
    freq = get_line(0);
    double freq_end = strtod (freq ,NULL);
    echo ("\r\nenter /step/ frequency in hz? ");
    freq = get_line(0);
    double freq_step = strtod (freq ,NULL);
    echo ("\r\nhow many /loop/ iterations? ");
    freq = get_line(0);
    long freq_count = strtol (freq ,NULL,10);
    echo ("\r\nwhich /output/ to modify? ");
    freq = get_line(0);
    long freq_module = strtol (freq ,NULL,10);
    if(freq_module == 1) {
      po(CLK);po(FQ);po(DATA);po(RST);
      //init
      ph(RST);ph(CLK);ph(FQ);
    } else {
      po(CLK2);po(FQ2);po(DATA2);po(RST2);
      //init
      ph(RST2);ph(CLK2);ph(FQ2);
    }
    //echo("\r\nSweeping from %lhz to %lhz step size %lhz", freq_start, freq_end, freq_step);
    //    char buffer[128] = "";
    //    sprintf(buffer, "\r\nSweeping from %lhz to %lhz step size %lhz\r\n", freq_start, freq_end, freq_step);
    //    echo (buffer);
    echo("\r\nSweep started.\r\n");
    double counts = 0;
    
    for (counts = 0; counts<freq_count; counts++) {
      Serial.print(((double)100 / freq_count) * counts);
      Serial.println("%");
      double range = 0;
      for (range = freq_start; range<=freq_end; range += freq_step) {
        int32_t tw = f(range);
        for (int i=0; i<32; i++, tw >>=1) {
          if(freq_module == 1) {
            sb(tw & 1);
          } else {
            sb2(tw & 1);
          }
        }
        // send 00000000's 
        for (int i=0; i<8; i++) {
          if(freq_module == 1) {
            sb(0);
          } else {
            sb2(0);
          }
        }
        if(freq_module == 1) {
          ph(FQ);
        } else {
          ph(FQ2);
        }
        delay(100);
      }
    }
    echo ("\r\nDDS sweep completed.\r\n");
    if(freq_module == 1) {
      po(CLK);po(FQ);po(DATA);po(RST);
      ph(RST);ph(CLK);ph(FQ);
    } else {
      po(CLK2);po(FQ2);po(DATA2);po(RST2);
      ph(RST2);ph(CLK2);ph(FQ2);
    }
    return;
  }
}
int show_authorize() {
if (get_key(0x0D))
  if (!have_tty) {
    have_tty = 1;
    // need to show login
    vt100_ESC("e[1K");
    int failures=0;
    while ( !get_auth_prompt() ) {
      failures++;
      if(failures == 3) { have_tty = 0; return 0; }
    }
    have_tty = 1;
    return 1;
  }
}
int get_key(char key) {
	if(Serial.available()>0) {
    	while(Serial.available() >0 ) {
    		char ch = Serial.read();
			if (ch == key) return 1;
		}
	}
	return 0;
}
void show_banner() {
  if(ticks>100) {
    if (!have_tty) { 
      /* display connection banner */
      show_tty_welcome();
    }
  ticks=0;
  }
  
}
void echo(char buffer[40]) {
  Serial.print(buffer);
}
int get_auth_prompt() {
  //begin
  //vt100_clear();
  boolean loop = 0;

    echo("\r\n");
    vt100_ESC("e[1K");
    echo(LOGIN_PROMPT);
    char username[128] = {0};
    char *source = get_line(0);
    memcpy(username, source, 128);
    
    echo("\r\n");
    vt100_ESC("e[1K");
    echo(PASSWORD_PROMPT);
    char password[128] = {0};
    source[0] = 0;
    source = get_line(1);
    memcpy(password, source, 128);
    echo("\r\n");
    vt100_ESC("e[1K");
    
    if ((memcmp(username, mubox_username, 5) == 0) && (memcmp(password, mubox_password, 5) == 0)) {
      vt100_color_dim_white_normal ();
      have_auth=1;
      return 1;
    }
    delay(1000);
    return 0;
}
void vt100_clear() {
  vt100_ESC("e[2J");
}
void vt100_ESC(char *buffer) {
  buffer[0] = 0x1B;
  echo(buffer); 
}
void vt100_color_bright_red_inverted () {
  vt100_ESC("e[7;1;31m");
}
void vt100_color_dim_white_normal () {
  vt100_ESC("e[0;2;37m");
}
void vt100_color_bright_white_inverted () {
  vt100_ESC("e[7;1;37m");
}
void vt100_color_dim_green_normal () {
  vt100_ESC("e[0;2;32m");
}

void vt100_home() {  
  char buffer[] = "e[H";
  buffer[0] = 0x1B;
  echo(buffer);
}

void show_tty_welcome () {
  // enable scrolling? -- key to getting the page to start at top :D
  //e[r
  char scrolling[] = "e[r";
  scrolling[0] = 0x1B;
  echo(scrolling);
  vt100_clear();
  vt100_color_bright_red_inverted ();
  echo(HOST_BANNER); 
  vt100_color_bright_white_inverted ();
}

// blocking
char *get_line(boolean secret) {
  //echo("begin get line");
  char buffer[128] = {0};
  char ch = 0;
  int i = 0;
  while (ch != 0x0D) {
    if(Serial.available() >0){
      ch = Serial.read();
      if(ch!=0x0D) {
        /* let's try to handle delete at the very least */
        if (ch != 0x7F) {
          buffer[i++] = ch;
        } else {
          if (i > 0) {
            i--;
            buffer[i] = 0x00;
            if(!secret) {
              vt100_ESC("e[D");
              vt100_ESC("e[K");
            }
            continue;
          } else {
            continue;
          }
        }
        if(!secret) {echo(&buffer[i-1]);}
      }
    }
  }
  return buffer;
}
