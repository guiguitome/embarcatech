#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "ssd1306_font.h"


//pinos do joystick
#define VRY 27         
#define ADC_CH_Y 1

//pinos do LED
#define LED_R 13
#define LED_G 11

//pino do buzzer
#define BUZZER_PIN 21

//configurações de PWM
#define DIVIDER_PWM 16.0f
#define PERIOD 4095

//inicializa o PWM no pino do buzzer                               
void pwm_init_buzzer(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pin);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0f); 
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(pin, 0); 
}

//ativa e configura o buzzer com som de alarme
void buzzer_alarm(uint pin) {
    uint slice_num = pwm_gpio_to_slice_num(pin);
    uint32_t clock_freq = clock_get_hz(clk_sys);
    uint32_t top = clock_freq / 2000 - 1; 

    pwm_set_wrap(slice_num, top);
    pwm_set_gpio_level(pin, top / 2); 

    sleep_ms(400); 
    pwm_set_gpio_level(pin, 0); 
    sleep_ms(100); 
}

int main() {
    stdio_init_all();

    //incializa o I2C do display OLED
    i2c_init(i2c_default, SSD1306_I2C_CLK * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);


    //inicializa ADC nos pinos do joystick
    adc_init();
    adc_gpio_init(VRY);

    //inicializa PWM do buzzer
    pwm_init_buzzer(BUZZER_PIN);

    //PWM para o  LED vermelho
    gpio_set_function(LED_R, GPIO_FUNC_PWM);
    uint slice_r = pwm_gpio_to_slice_num(LED_R);
    pwm_set_clkdiv(slice_r, DIVIDER_PWM);
    pwm_set_wrap(slice_r, PERIOD);
    pwm_set_gpio_level(LED_R, 0);
    pwm_set_enabled(slice_r, true);

    //PWM para o LED verde
    gpio_set_function(LED_G, GPIO_FUNC_PWM);
    uint slice_g = pwm_gpio_to_slice_num(LED_G);
    pwm_set_clkdiv(slice_g, DIVIDER_PWM);
    pwm_set_wrap(slice_g, PERIOD);
    pwm_set_gpio_level(LED_G, 0);
    pwm_set_enabled(slice_g, true);


    //configurando o display 
    uint8_t buf[SSD1306_BUF_LEN];
    struct render_area frame_area = {0, SSD1306_WIDTH - 1, 0, SSD1306_NUM_PAGES - 1};
    calc_render_area_buflen(&frame_area);

    while (true) {

        //lê eixo Y
        adc_select_input(ADC_CH_Y);
        sleep_us(5);
        uint16_t yVal = adc_read();

        //distância do centro em cada eixo do joystick
        float fracY = (float)abs((int)yVal - 2048) / 2048.0f;

        //transição do verde para o vermelho e vice-versa no LED
        uint16_t red   = (uint16_t)(fracY * PERIOD);
        float greenF   = (1.0f - fracY) * (1.0f - fracY);             
        uint16_t green = (uint16_t)(greenF * PERIOD);                 

        //ativando o PWM do LED
        pwm_set_gpio_level(LED_R, red);
        pwm_set_gpio_level(LED_G, green);

        //apagando a mensagem anterior do display
        memset(buf, 0, SSD1306_BUF_LEN);
        
        if (yVal >= 700 && yVal < 3100){
            WriteString(buf, 5, 8, "Status:");
            WriteString(buf, 5, 16, "OK.");
        } else if (yVal >= 3100 && yVal < 3900){
            WriteString(buf, 5, 8, "Status:");
            WriteString(buf, 5, 16, "Acima.");
        } else if (yVal >= 3900) {
            WriteString(buf, 5, 8, "Status:");
            WriteString(buf, 5, 16, "PERIGO!");
            buzzer_alarm(BUZZER_PIN);
        } else if (yVal > 200 && yVal < 700) {
            WriteString(buf, 5, 8, "Status:");
            WriteString(buf, 5, 16, "Abaixo.");
        } else if (yVal <= 200){
            WriteString(buf, 5, 8, "Status:");
            WriteString(buf, 5, 16, "Muito Baixo!");
            buzzer_alarm(BUZZER_PIN);
        } else {
            pwm_set_gpio_level(BUZZER_PIN, 0);
        }
        
        //atualizando o display
        render(buf, &frame_area);

        //"Desativando por um segundo"
        sleep_ms(1000);
    }
    return 0;
}
