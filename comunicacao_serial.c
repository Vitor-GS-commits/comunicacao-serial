#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "pico/time.h"
#include <math.h>
#include "libs/leds.h"
#include "hardware/pio.h"
#include "hardware/adc.h"
#include "pico/bootrom.h"
#include "hardware/clocks.h"
#include "pio_matrix.pio.h"
#include "libs/animations.h"

// Definições do display
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
// Definições de pinos leds e botões
#define led_red 13
#define led_blue 12
#define led_green 11
#define button_A 5
uint const button_B = 6; 


// variáveis globais
float intensity = 0.3f;
PIO pio;
uint sm;
int contador = 0; //contador de botões
static volatile uint32_t last_time = 0; // Armazena o tempo do último evento (em microssegundos)
ssd1306_t ssd; //inicializa a estrutura do display
bool estado_lg = false;
bool estado_lb = false;

volatile bool callback_ativado = false;  // Variável global para marcar a ativação

//Declaração da função de interrupção
static void gpio_irq_handler(uint gpio, uint32_t events);


int main()
{
    stdio_init_all();

    // inicialização dos leds
    gpio_init(led_red);
    gpio_set_dir(led_red, GPIO_OUT);
    gpio_init(led_blue);
    gpio_set_dir(led_blue, GPIO_OUT);
    gpio_init(led_green);
    gpio_set_dir(led_green, GPIO_OUT);

    // inicialização dos botões de interrupção
    gpio_init(button_A);
    gpio_set_dir(button_A, GPIO_IN);
    gpio_pull_up(button_A);
    gpio_init(button_B);
    gpio_set_dir(button_B, GPIO_IN);
    gpio_pull_up(button_B);

    pio = pio0;
    // configurações da PIO
    printf("iniciando a transmissão PIO \n");
    uint offset = pio_add_program(pio, &pio_matrix_program);
    sm = pio_claim_unused_sm(pio, true);
    pio_matrix_program_init(pio, sm, offset, LED_PIN);

    // Inicializa o I2C a 400kHz
    i2c_init(I2C_PORT, 400 * 1000);

    // Define os pinos SDA e SCL
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // GPIO 14
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // GPIO 15
    gpio_pull_up(I2C_SDA); // SDA é pull-up
    //    ssd1306_t ssd; //inicializa a estrutura do display
    ssd1306_t ssd; //inicializa a estrutura do display
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); //inicializa o display
    ssd1306_config(&ssd); //configura o display
    ssd1306_send_data(&ssd); //envia os dados para o display

    //limpa o display
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    bool cor = true;
    //define caractere
    char c;
    // lista de numeros
    char numeros[10] = {'0','1','2','3','4','5','6','7','8','9'};

    //interrupção
    gpio_set_irq_enabled_with_callback(button_A, GPIO_IRQ_EDGE_RISE, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(button_B, GPIO_IRQ_EDGE_RISE, true, &gpio_irq_handler);

    while (true) {
        cor = !cor;
        int j = 0;

        //atualiza o display com animação
        ssd1306_fill(&ssd, !cor); //limpa o display
        ssd1306_rect(&ssd, 3, 3, 122, 58, cor, !cor);//desenha um retângulo, posição em y, posição em x, prenchimento horizontal, preenchimento vertical
        
        
        for (size_t i = 0; i < 16; i++)
        {
            
            scanf("%c", &c); // Lê um caractere
            printf("Caractere digitado: %c\n", c);
            //verifica se um numero foi digitado
            for (size_t k = 0; k < 10; k++)
            {
                if (c == numeros[k])
                {
                    desenha_numero(k, pio, sm, intensity);
                }
            }
            
            ssd1306_draw_char(&ssd, c, 8+j, 10);
            ssd1306_send_data(&ssd);
            j = j + 7; //intervalo de 7 pixels entre os caracteres para próximmo caractere
            sleep_ms(100);
        }
        
        sleep_ms(500);
    }
}

void gpio_irq_handler(uint gpio, uint32_t events){
    // Obtém o tempo atual em microssegundos
    uint32_t current_time = to_us_since_boot(get_absolute_time());
    // Verifica se passou tempo suficiente desde o último evento
    if (current_time - last_time > 200000) // 200 ms de debouncing
    {
        last_time = current_time; // Atualiza o tempo do último evento
        ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); //inicializa o display
        ssd1306_config(&ssd); //configura o display
        ssd1306_send_data(&ssd); //envia os dados para o display

        //limpa o display
        ssd1306_fill(&ssd, false);
        //condição para verificar qual botão foi pressionado
            if(gpio == button_A){
                gpio_put(led_green, !estado_lg);
                gpio_put(led_blue, false);
                estado_lg = !estado_lg;
                estado_lb = false;
                printf("Led verde = %d\n", estado_lg);
                ssd1306_draw_string(&ssd, "Botao A verde", 5, 30); // Desenha uma string
                ssd1306_send_data(&ssd);
            }
            if(gpio == button_B){
                gpio_put(led_blue, !estado_lb);
                gpio_put(led_green, false);
                estado_lb = !estado_lb;
                estado_lg = false;
                printf("Led azul = %d\n", estado_lb);
                ssd1306_draw_string(&ssd, "Botao B azul", 5, 30); // Desenha uma string
                ssd1306_send_data(&ssd);
            }
    }
    
}