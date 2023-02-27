//-----Projeto do controlador de Etileno-----////
///-----GK AUTOMACAO-----////

#include <LiquidCrystal.h> /// incluir biblioteca do LCD
#include <EEPROM.h>  /// biblioteca da EEPROM

byte sqr[8] = {
  B00000,
  B00000,
  B01110,
  B01110,
  B01110,
  B01110,
  B00000,
  B00000
};

///-----VARIÁVEIS E CONSTANTES REFERENTES AO PT100 E RESISTENCIA------///

int vpt; /// tensão lida no pt100
const int vt1 = 198; ///tensão lida quando pt100 está em 270ºC
const int vt2 = 201; ///tensão lida quando pt100 está em 305ºC
const int resistencia = 3; /// pino digital para resistencia
const int pt_pin = 5;
int valv_delay = 0;

///-----VARIÁVEIS E CONSTANTES REFERENTES À VÁLVULA------///

const int valvula = 2; /// pino digital para a valvula
const long int pulse_address = 30; // endereço de memória do pulso selecionado
const long int m_adress = 40;  // endereço do modo selecionado
const long int long_pulse = 200; // pulso longo (200 ms)
const long int short_pulse = 90;  /// pulso longo (90 ms)
const long int m1 = 12000;  /// modo 1: valvula off por 12 segundos
const long int m2 = 24000; /// modo 2: valvula off por 24 segundos
const long int m3 = 36000;  /// modo 3: valvula off por 36 segundos
const long int m4 = 48000;  /// modo 2: valvula off por 48 segundos
long int t_on; // tempo em que a valvula abre (default: 90ms)
long int t_off; /// tempo em que a valvula está desligada (default: 12s)
long int t_valve; /// variável para controlar acionamento da válvula

///------VARIÁVEIS REFERENTES AO CICLO DE OPERAÇÃO---///

long int current; /// variável utilizada para contar elapsed_time
long int elapsed_time = 0; // variável que guarda o tempo decorrido de operação
const int elapsed_address = 50;  /// variável que guarda o endereço de elapsed_time
const int ctrl_address = 60;  /// variável que guarda o endereço de cycle_ctrl
int cycle_ctrl;  // variável utilizada para controlar o fluxo do programa em caso de queda de energia
int on = 0;
const int cycle_address = 80;  /// variável que guarda o endereço de cycle
const long int cycle_6 = 21600; ///ciclo de 6 horas (21600) (em segundos)
const long int cycle_12 = 43200; // ciclo de 12 horas (43200)(em segundos)
long int cycle = cycle_12; // ciclo de operação da maquina (default: 12 horas)
long int tempo;
int horas, minutos, segundos;
 
//-----PARÂMETROS PARA CALIBRAÇÃO E CÁLCULOS------//

const int CALIBRATION_SAMPLE_TIME = 1000;  ///intervalo de tempo para tomar amostras para calibração
const int CALIBRATION_SAMPLES = 60;  //// número de amostras a serem tomadas para calibração
const double rl = 10000;  /// valor da resistência rl em série com o sensor
const double vcc = 5;  /// tensão de alimentação do sensor
double r0_calc; /// Ro calculado pela calibração
const double rs_r0_ratio = 20; /// valor tomado do datasheet do TGS2620, Rs/Ro = 20 para sensor medindo ppm do ar
const int r0_address = 10; /// endereço de memória de r0

///-----VARIÁVEIS E CONSTANTES UTILIZADAS NO CÓDIGO DO DISPLAY----////

const int button_up = 0;  /// declara o botão up/enter no pino 0
const int button_down = 6;    /// declara o botão down/clear no pino 6
unsigned long last;  //guarda valor de millis() para controle do debounce dos botões
unsigned long end_adjust; // variável de tempo usada para controlar o fim da fase de seleção de ppm no ajuste
const long interval = 100; // intervalo para debounce dos botões
LiquidCrystal lcd(8, 9, 10, 11, 12, 13); //// declaração dos pinos do LCD
const int relay_pin = 2; // pino que aciona relê
const int adj_pin = 7; // pino do led de ajuste
const int setP_address = 0; // endereço da eeprom usado para guardar o ultimo ajuste de ppm

//// Variáveis de controle ////

int setPoint, input;
double v_tgs, r_tgs;
int tgs = 1, resist = 0, v_ctr = 0;

/// Função com mensagem inicial do display ///
void inicio() {

  lcd.setCursor(0, 0);
  lcd.print("Banasil V1");
  lcd.setCursor(0, 1);
  lcd.print("Gerador de Etileno");
  delay(5000);
}

void print_menu(){

  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("Iniciar maquina");
  lcd.setCursor(1,1);
  lcd.print("Calibrar");
  lcd.setCursor(1,2);
  lcd.print("Tempo de ciclo");
}

int confirma_opcao(int op){

  int select_cycle = 0;

  if(op == 1){
    lcd.clear();
    lcd.print("Iniciar maquina?");
    lcd.setCursor(1,1);
    lcd.print("Up para confirmar");
    lcd.setCursor(1,2);
    lcd.print("Down para retornar");
  }

  else if(op == 2){
    lcd.clear();
    lcd.print("Calibrar maquina?");
    lcd.setCursor(1,1);
    lcd.print("Up para confirmar");
    lcd.setCursor(1,2);
    lcd.print("Down para retornar");
  }

  else{
    lcd.clear();
    lcd.print("Selecionar ciclo");
    lcd.setCursor(0,1);
    lcd.print("(Default: 12 horas)");
    lcd.setCursor(1,2);
    lcd.print("12 horas");
    lcd.setCursor(1,3);
    lcd.print("6 horas");
  }

  while(1){

    if(op == 1){
      if(digitalRead(button_up)==LOW && digitalRead(button_down)==HIGH){
        delay(200);
        print_menu();
        return 0;
      }

      else if(digitalRead(button_up)==HIGH && digitalRead(button_down)==LOW){
        delay(200);
        return 1;
      }

      else{
        
      }
    }
    
    else if(op == 2){
      if(digitalRead(button_up)==LOW && digitalRead(button_down)==HIGH){
        delay(200);
        print_menu();
        return 0;
      }

      else if(digitalRead(button_up)==HIGH && digitalRead(button_down)==LOW){
        delay(200);
        TGS_calibration();
        print_menu();
        return 0;
      }

      else{
        
      }
    }

    else{
      if(select_cycle == 0){
        lcd.setCursor(0,2);
        lcd.write(byte(0));

        if(digitalRead(button_up)==LOW && digitalRead(button_down)==HIGH){
          delay(200);
          lcd.setCursor(0,2);
          lcd.write(" ");
          select_cycle = 1;
        }

        else if(digitalRead(button_up)==HIGH && digitalRead(button_down)==LOW){
          delay(200);
          cycle = cycle_12;
          print_menu();
          return 0;
        }

        else{
          
        }
      }
      
      else{
        lcd.setCursor(0,3);
        lcd.write(byte(0));

        if(digitalRead(button_up)==LOW && digitalRead(button_down)==HIGH){
          delay(200);
          lcd.setCursor(0,3);
          lcd.write(" ");
          select_cycle = 0;
        }

        else if(digitalRead(button_up)==HIGH && digitalRead(button_down)==LOW){
          delay(200);
          cycle = cycle_6;
          print_menu();
          return 0;
        }

        else{
          
        }
      }
    }
  }
}

void config_valve(){

  int opt = 1;
  int tmp = 1;

  lcd.clear();
  lcd.print("Configurar Valvula");
  lcd.setCursor(1,1);
  lcd.print("Pulso Curto");
  lcd.setCursor(1,2);
  lcd.print("Pulso Longo");

  while(1){

    if(opt == 1){
      lcd.setCursor(0,1);
      lcd.write(byte(0));

      if(digitalRead(button_up)==HIGH && digitalRead(button_down)==LOW){
        delay(400);
        EEPROM.put(pulse_address, short_pulse);
        break;
      }

      else if(digitalRead(button_up)==LOW && digitalRead(button_down)==HIGH){
        delay(400);
        lcd.setCursor(0,1);
        lcd.print(" ");
        opt = 2;
      }
    }

    else{
      lcd.setCursor(0,2);
      lcd.write(byte(0));

      if(digitalRead(button_up)==HIGH && digitalRead(button_down)==LOW){
        delay(400);
        EEPROM.put(pulse_address, long_pulse);
        break;
      }

      else if(digitalRead(button_up)==LOW && digitalRead(button_down)==HIGH){
        delay(400);
        lcd.setCursor(0,2);
        lcd.print(" ");
        opt = 1;
      }
    }
  }

  lcd.clear();
  lcd.print("Acionar valvula em");
  delay(3000);
  
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("12 segundos");
  lcd.setCursor(1,1);
  lcd.print("24 segundos");
  lcd.setCursor(1,2);
  lcd.print("36 segundos");
  lcd.setCursor(1,3);
  lcd.print("48 segundos");

  while(1){

    if(tmp == 1){
      lcd.setCursor(0,0);
      lcd.write(byte(0));

      if(digitalRead(button_up)==HIGH && digitalRead(button_down)==LOW){
        delay(400);
        EEPROM.put(m_adress, m1);
        break;
      }

      else if(digitalRead(button_up)==LOW && digitalRead(button_down)==HIGH){
        delay(400);
        lcd.setCursor(0,0);
        lcd.print(" ");
        tmp = 2;
      }
    }

    else if(tmp == 2){
      lcd.setCursor(0,1);
      lcd.write(byte(0));

      if(digitalRead(button_up)==HIGH && digitalRead(button_down)==LOW){
        delay(400);
        EEPROM.put(m_adress, m2);
        break;
      }

      else if(digitalRead(button_up)==LOW && digitalRead(button_down)==HIGH){
        delay(400);
        lcd.setCursor(0,1);
        lcd.print(" ");
        tmp = 3;
      }
    }

    else if(tmp == 3){
      lcd.setCursor(0,2);
      lcd.write(byte(0));

      if(digitalRead(button_up)==HIGH && digitalRead(button_down)==LOW){
        delay(400);
        EEPROM.put(m_adress, m3);
        break;
      }

      else if(digitalRead(button_up)==LOW && digitalRead(button_down)==HIGH){
        delay(400);
        lcd.setCursor(0,2);
        lcd.print(" ");
        tmp = 4;
      }
    }

    else{
      lcd.setCursor(0,3);
      lcd.write(byte(0));

      if(digitalRead(button_up)==HIGH && digitalRead(button_down)==LOW){
        delay(400);
        EEPROM.put(m_adress, m4);
        break;
      }

      else if(digitalRead(button_up)==LOW && digitalRead(button_down)==HIGH){
        delay(400);
        lcd.setCursor(0,3);
        lcd.print(" ");
        tmp = 1;
      }
    }
  }

  select_adjust();

  print_menu();
}

void menu(){

  long int temp;
  int opt = 1;
  int ctrl_menu = 0;

  print_menu();

  while(1){

    temp = millis();
    
    if(opt == 1){
      lcd.setCursor(0,0);
      lcd.write(byte(0));

      if(digitalRead(button_up)==LOW && digitalRead(button_down)==HIGH){
        delay(200);
        
        while(digitalRead(button_down)==HIGH){
          if(millis()-temp>=10000){
            config_valve();
          }
        }
        
        lcd.setCursor(0,0);
        lcd.print(" ");
        opt = 2;
      }

      else if(digitalRead(button_up)==HIGH && digitalRead(button_down)==LOW){
        delay(200);
        ctrl_menu = confirma_opcao(1);
        if(ctrl_menu == 1){
          break;
        }
      }

      else{
        
      }
    }
  
    else if(opt == 2){
      lcd.setCursor(0,1);
      lcd.write(byte(0));

      if(digitalRead(button_up)==LOW && digitalRead(button_down)==HIGH){
        delay(200);

        while(digitalRead(button_down)==HIGH){
          if(millis()-temp>=10000){
            config_valve();
          }
        }
        
        lcd.setCursor(0,1);
        lcd.print(" ");
        opt = 3;
      }

      else if(digitalRead(button_up)==HIGH && digitalRead(button_down)==LOW){
        delay(200);
        ctrl_menu = confirma_opcao(2);
      }

      else{
        
      }
    }
  
    else{
      lcd.setCursor(0,2);
      lcd.write(byte(0));

      if(digitalRead(button_up)==LOW && digitalRead(button_down)==HIGH){
        delay(200);

        while(digitalRead(button_down)==HIGH){
          if(millis()-temp>=10000){
            config_valve();
          }
        }
        
        lcd.setCursor(0,2);
        lcd.print(" ");
        opt = 1;
      }

      else if(digitalRead(button_up)==HIGH && digitalRead(button_down)==LOW){
        delay(200);
        ctrl_menu = confirma_opcao(3);
      }

      else{
        
      }
    }
  }
}

void select_adjust(){

  lcd.clear();
  lcd.print("Mudar ajuste?");
  lcd.setCursor(0, 1);
  lcd.print("Ultimo: ");
  lcd.setCursor(8,1);
  EEPROM.get(setP_address, setPoint);
  lcd.print(setPoint);

  while(1){
    
    if(digitalRead(button_up)==HIGH && digitalRead(button_down)==LOW){
      setPoint = ajuste_ppm();
      break;
    }

    else if(digitalRead(button_up)==LOW && digitalRead(button_down)==HIGH){
      delay(1000);
      digitalWrite(adj_pin, HIGH);
      EEPROM.get(setP_address, setPoint);
      break;
    }
  }
  
}

/// Função para escrever o valor de ppm no display ///
void print_ppm(int ppm) {

  int value = ppm;
  
  /// Escreve valores entre 0 e 9 no display //
  if (value >= 0 && value < 10) {
    lcd.setCursor(6, 2);
    lcd.print(" ");
    lcd.setCursor(7, 2);
    lcd.print(" ");
    lcd.setCursor(8, 2);
    lcd.print(value);
    lcd.setCursor(10, 2);
    lcd.print("ppm");
  }

  /// Escreve valores entre 10 e 99 no display //
  else if (value < 100 && value >= 10) {
    lcd.setCursor(6, 2);
    lcd.print(" ");
    lcd.setCursor(7, 2);
    lcd.print(value);
    lcd.setCursor(10, 2);
    lcd.print("ppm");
  }

  /// Escreve valores entre 100 e 999 no display //
  else if (value >= 100 && value<1000) {
    lcd.setCursor(6, 2);
    lcd.print(value);
    lcd.setCursor(10, 2);
    lcd.print("ppm");
  }

  /// Escreve valores>=1000 no display (obs: controlador lê até 1000 ppm, portanto este código só irá escrever 1000 ppm no display)//
  else{
    lcd.setCursor(5, 2);
    lcd.print(value);
    lcd.setCursor(10, 2);
    lcd.print("ppm");
  }
}

/// Função para confirmação do valor de ppm ajustado pelo usuário //
int confirma_ppm(int ppm) {

  // Recebe o valor de ppm e escreve no display, perguntando se o ajuste será confirmado ou não //
  int value = ppm;
  lcd.clear();
  lcd.print("Confirmar Ajuste");
  print_ppm(value);

  while (1) {

    /// Se acionado botão up/enter, ajuste é confirmado e controlador começa a operar //
    if (digitalRead(button_up) == HIGH && digitalRead(button_down) == LOW) {
      lcd.clear();
      lcd.print("Ajuste Confirmado!");
      EEPROM.put(setP_address, value);
      digitalWrite(adj_pin, HIGH);
      print_ppm(value);
      delay(3000);
      return 1;
    }

    /// Se acionado botão down/clear, volta para a seleção de ppm ///
    else if (digitalRead(button_up) == LOW && digitalRead(button_down) == HIGH) {
      delay(200);
      lcd.clear();
      lcd.print("Selecionar ppm");
      print_ppm(value);
      return 0;
    }

  }
}

// Função principal da fase de ajuste de ppm //
int ajuste_ppm() {

  int ppm = 0;
  int ctr;

  digitalWrite(adj_pin, LOW);

  lcd.clear();
  lcd.print("Selecionar ppm");
  print_ppm(ppm);

  end_adjust = millis();

  while (1) {

    // Se botões up e down não forem acionados após 4 segundos, entra para fase de confirmação //
    if (digitalRead(button_up) == LOW && digitalRead(button_down) == LOW) {
      last = millis();
      if (millis() - end_adjust >= 4000) {
        ctr = confirma_ppm(ppm);
        if(ctr == 1){
          return ppm;
        }
        else{
          end_adjust = millis();
        }
      }
    }

    // Se apenas o botão up é acionado, incrementa ppm e escreve na tela do LCD //
    else if (digitalRead(button_up) == HIGH && digitalRead(button_down) == LOW) {
      if (millis() - last >= interval) {
        ppm++;
        print_ppm(ppm);
        end_adjust = millis();
        last = millis();
      }
      if (ppm > 1000) {   //// Tratamento de Erro no caso de ppm>1000 ////
        lcd.clear();
        lcd.print("Fora da faixa de ajuste");
        delay(2500);
        lcd.clear();
        lcd.print("Selecionar ppm");
        ppm = 1000;
        print_ppm(ppm);
        end_adjust = millis();
        last = millis();
      }
    }

    // Se apenas o botão down é acionado, decrementa ppm e escreve na tela do LCD //
    else if (digitalRead(button_up) == LOW && digitalRead(button_down) == HIGH) {
      if (millis() - last >= interval) {
        ppm--;
        print_ppm(ppm);
        end_adjust = millis();
        last = millis();
      }
      if (ppm < 0) {     //// tratamento de erro no caso de tentar selecionar ppm<0 ////
        lcd.clear();
        lcd.print("Erro! PPM negativo!");
        delay(2500);
        lcd.clear();
        lcd.print("Selecionar ppm");
        ppm = 0;
        print_ppm(ppm);
        end_adjust = millis();
        last = millis();
      }
    }

    /// caso ambos os botões sejam acionados, controlador não irá tomar ação ///
    else{
      end_adjust = millis();
      last = millis();
    }

    
  }

}

/// Função que faz a conversão do valor da resistência do sensor para ppm ////
int convert_to_ppm(double rs){

  double ppm;
  double r = rs;

  ppm = pow(10, ((log10(r/r0_calc)-1.93)/(-0.818)));
  
  return (int) ppm;
}

/// Função que faz a leitura a tensão sobre rl (tensão obtida pelo sensor) ////
double sensor_read(){

  double read_value;

  read_value = (vcc/1023.0)*analogRead(A5);

  return read_value;
}

/// Função que faz o cálculo da resistência do sensor com base na tensão vrl ///
double rs_calc(double sensor_voltage){
  
  double rs;
  double v = sensor_voltage;
  
  rs = ((vcc-v)/v)*rl;

  return rs;
}

/// Função que faz a calibração do sensor, obtendo o r0 (resistência do sensor em 300 ppm de etanol) ///
void TGS_calibration(){
  
  double rs=0;
  double v_read;

  lcd.clear();
  lcd.print("Calibrando...");

  delay(180000);
  
  for(int i=0; i<CALIBRATION_SAMPLES; i++){
    v_read = sensor_read();
    rs+=rs_calc(v_read);
    lcd.setCursor(4,1);
    lcd.print("v=");
    lcd.setCursor(6,1);
    lcd.print((String)v_read);
    lcd.setCursor(4,2);
    lcd.print("rs=");
    lcd.setCursor(7,2);
    lcd.print((String)rs);
    delay(CALIBRATION_SAMPLE_TIME);
  }

  rs = rs/CALIBRATION_SAMPLES;
  r0_calc = rs/rs_r0_ratio;

  EEPROM.put(r0_address, r0_calc);

  lcd.clear();
  lcd.print("r0=");
  lcd.setCursor(4,0);
  lcd.print(String(r0_calc));
  delay(3000);
}

void operation_print(int value){
  
  lcd.clear();
  lcd.print("Operando");
  lcd.setCursor(2,2);
  lcd.print("Setpoint=");
  lcd.setCursor(2,3);
  lcd.print("Input=");
  lcd.setCursor(11,2);
  lcd.print(setPoint);
  lcd.setCursor(8,3);
  lcd.print(value);
  delay(200);

  if(value>1000){
    lcd.clear();
    lcd.print("Valor maior que 1000");
    delay(200);
  }
}

int read_input(){

  int ppm;
  double v;
  double rs;

  v = sensor_read();
  rs = rs_calc(v);
  ppm = convert_to_ppm(rs);

  return ppm;
}

void print_values(){

  lcd.setCursor(6,0);
  lcd.print(String(input)+" ");
  
  lcd.setCursor(8,2);
  lcd.print(":");
  lcd.setCursor(11,2);
  lcd.print(":");

  if(horas>=10){
    lcd.setCursor(6,2);
    lcd.print(horas);
  }

  if(horas<10){
    lcd.setCursor(6,2);
    lcd.print("0");
    lcd.setCursor(7,2);
    lcd.print(horas);
  }

  if(minutos>=10){
    lcd.setCursor(9,2);
    lcd.print(minutos);
  }

  if(minutos<10){
    lcd.setCursor(9,2);
    lcd.print("0");
    lcd.setCursor(10,2);
    lcd.print(minutos);
  }
  
  if(segundos>=10){
    lcd.setCursor(12,2);
    lcd.print(segundos);
  }

  if(segundos<10){
    lcd.setCursor(12,2);
    lcd.print("0");
    lcd.setCursor(13,2);
    lcd.print(segundos);
  }

  lcd.setCursor(0,3);
  lcd.print(vpt);
}

void read_pt100(){

  vpt = analogRead(A1);
}

void rising(){

 long int acionar;

   v_ctr++;

     if(v_ctr == t_off){
    
      acionar = millis()+t_on;
    
       while(acionar>millis()){
        digitalWrite(valvula, HIGH);
       }
    
       v_ctr = 0;  
       digitalWrite(valvula, LOW);
     }
     else if(v_ctr>t_off){
      v_ctr = 0;
     } 
}

void falling(){

 long int acionar;

   v_ctr++;

   
     if(v_ctr == t_off){
      
        acionar = millis()+t_on;
      
         while(acionar>millis()){
          digitalWrite(valvula, HIGH);
         }
      
         v_ctr = 0;  
         digitalWrite(valvula, LOW);
      }
     else if(v_ctr>t_off){
        v_ctr = 0;
     } 
   
   
}

void get_time(){

  elapsed_time += 1;
  EEPROM.put(elapsed_address, elapsed_time);

  tempo = cycle - elapsed_time;

  horas = tempo/3600;
  minutos = (tempo%3600)/60;
  segundos = (tempo%3600)%60;

  current+=1000;
}

int check_cycle(){
  
  EEPROM.get(ctrl_address, cycle_ctrl);

  if(cycle_ctrl==1){
    EEPROM.get(cycle_address, cycle);
    EEPROM.get(elapsed_address, elapsed_time);
    return 1;
  }

  else{
    return 0;
  }
}

void warm_pt(){

  lcd.clear();
  lcd.print("Preparando resist.");

  read_pt100();

  if(vpt<197){
    digitalWrite(resistencia, HIGH);
    while(1){
      read_pt100();
      lcd.setCursor(0,1);
      lcd.print(vpt);
      if(vpt>=197){
        resist = 0;
        break;
      }
    }
  }

  else{
    digitalWrite(resistencia, LOW);
    while(1){
      read_pt100();
      lcd.setCursor(0,1);
      lcd.print(vpt);
      if(vpt<=197){
        resist = 1;
        break;
      }
    }
  }

  
}

void lcd_print(){

  lcd.clear();
  lcd.print("ppm =");
  lcd.setCursor(0,1);
  lcd.print("Calibrado em:");
  lcd.setCursor(13,1);
  lcd.print(String(setPoint)+" ppm");
}

void setup() {
  Serial.begin(9600);
  pinMode(A1, INPUT);
  pinMode(A5, INPUT);
  pinMode(button_up, INPUT);
  pinMode(button_down, INPUT);
  pinMode(relay_pin, OUTPUT);
  pinMode(adj_pin, OUTPUT);
  pinMode(resistencia, OUTPUT);
  pinMode(valvula, OUTPUT);
  pinMode(pt_pin, OUTPUT);
  digitalWrite(pt_pin, HIGH);
  lcd.begin(20, 4);
  lcd.createChar(0,sqr);
  inicio();
  on = check_cycle();
}

void loop() {

  if(on == 0){
    EEPROM.put(ctrl_address, 0);
    EEPROM.put(elapsed_address, 0);
    menu();
    EEPROM.put(cycle_address, cycle);
  }
  
  EEPROM.get(setP_address, setPoint);
  EEPROM.get(pulse_address, t_on);
  EEPROM.get(m_adress, t_off);
  EEPROM.get(r0_address, r0_calc);
  t_off = t_off/1000;
  EEPROM.put(ctrl_address, 1);
  warm_pt();
  lcd_print();
  current = millis();

  while(1){
    
    if(cycle>(elapsed_time/1000)){

      input = read_input();
      read_pt100();
  
      if(digitalRead(button_down)==HIGH){
        digitalWrite(resistencia, LOW);
        digitalWrite(valvula, LOW);
        on = 0;
        elapsed_time = 0;
        EEPROM.put(ctrl_address, on);
        EEPROM.put(elapsed_address, elapsed_time);
        break;
      }
      
      if(millis()-current>=1000){

        get_time();

        if(tgs==1){
          
          if(input<=setPoint+5){
            
            if(resist == 1){

              valv_delay++;
              
              digitalWrite(resistencia, HIGH);
              
              if(valv_delay>=85){
                valv_delay = 0;
                resist = 0;
              }

              if(valv_delay >= 25){
                rising();
              }
            }

            else{

              digitalWrite(resistencia, LOW);
              valv_delay++;

              if(valv_delay>=130){
                valv_delay = 0;
                resist = 1;
              }

              falling();
            }
          }

          else{
            tgs = 0;
            digitalWrite(resistencia, LOW);
            digitalWrite(valvula, LOW);
          }
        }

        else{
          if(setPoint-5>input){
            tgs = 1; 
          }
        }
                
        print_values();
      }
    }
  
    else{
      digitalWrite(pt_pin, LOW);
      digitalWrite(resistencia, LOW);
      digitalWrite(valvula, LOW);
      EEPROM.put(ctrl_address, 0);
      EEPROM.put(elapsed_address, 0);

      lcd.clear();
      lcd.print("Ciclo finalizado");
      
      while(1){
        
      }
    }
  }
 }
