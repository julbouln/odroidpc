
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/dma.h>

#include <libopencm3/stm32/crs.h>
#include <libopencm3/stm32/syscfg.h>

#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/pwr.h>

#include "keyboard.h"
#include "usb.h"

#define KBD_O_PORT GPIOA

#define KBD_O1 GPIO0
#define KBD_O2 GPIO1
#define KBD_O3 GPIO2
#define KBD_O4 GPIO3
#define KBD_O5 GPIO4
#define KBD_O6 GPIO5
#define KBD_O7 GPIO6
#define KBD_O8 GPIO7
#define KBD_O9 GPIO8
#define KBD_O10 GPIO9
#define KBD_O11 GPIO10
#define KBD_O12 GPIO13
#define KBD_O13 GPIO14
#define KBD_O14 GPIO0 // PF0
#define KBD_O15 GPIO1 // PF1


#define KBD_I1 GPIO0
#define KBD_I2 GPIO1
#define KBD_I3 GPIO3
#define KBD_I4 GPIO4
#define KBD_I5 GPIO5
#define KBD_I6 GPIO6
#define KBD_I7 GPIO7

volatile uint32_t system_millis;

/* Called when systick fires */
void sys_tick_handler(void)
{
	system_millis++;
}

/* sleep for delay milliseconds */
static void delay(uint32_t delay)
{
	uint32_t wake = system_millis + delay;
	while (wake > system_millis);
}

/* Set up a timer to create 1mS ticks. */
static void systick_setup(void)
{
	/* clock rate / 1000 to get 1mS interrupt rate */
	systick_set_reload(48000);
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
	systick_counter_enable();
	/* this done last */
	systick_interrupt_enable();
}

static void gpio_setup(void)
{
	// keyboard output
	gpio_mode_setup(KBD_O_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, KBD_O1 | KBD_O2 | KBD_O3 | KBD_O4 | KBD_O5 | KBD_O6 | KBD_O7 | KBD_O8);
	gpio_mode_setup(KBD_O_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, KBD_O9 | KBD_O10 | KBD_O11 | KBD_O12 | KBD_O13);

	gpio_mode_setup(GPIOF, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, KBD_O14 | KBD_O15);

	// keyboard input
	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, KBD_I1 | KBD_I2 | KBD_I3 | KBD_I4 | KBD_I5 | KBD_I6 | KBD_I7);
//	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, KBD_I8);

	gpio_clear(GPIOB, KBD_I1 | KBD_I2 | KBD_I3 | KBD_I4 | KBD_I5 | KBD_I6 | KBD_I7);
}

#define MATRIX_ROW 7
#define MATRIX_COL 15

#if 0
uint8_t kbd_matrix[MATRIX_ROW][MATRIX_COL] = {
	{KEYPAD_AT, KEY_SLASH, KEY_MINUS, KEY_SPACE, KEYPAD_LEFT_BRACKET, KEYPAD_RIGHT_BRACKET, KEY_ERROR_UNDEFINED /* Fn */, KEY_ERROR_UNDEFINED},
	{KEY_B, KEY_N, KEY_M, KEY_COMMA, KEY_PERIOD, KEYPAD_COLON, KEY_ERROR_UNDEFINED/* ~ */, KEY_RIGHT_SHIFT},
	{KEY_ERROR_UNDEFINED/* .. */, KEY_ERROR_UNDEFINED/* ^ */, KEY_ENTER, KEY_LEFT_SHIFT, KEY_Z, KEY_X, KEY_C, KEY_V},
	{KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K, KEY_L},
	{KEY_O, KEY_P, KEY_ERROR_UNDEFINED/* ` */, KEY_ERROR_UNDEFINED/* ` */, KEY_BACKSPACE, KEY_ERROR_UNDEFINED/* ? */, KEY_CAPS_LOCK, KEY_A},
	{KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I},
	{KEY_8, KEY_9, KEY_0, KEY_ERROR_UNDEFINED/* " */, KEY_ERROR_UNDEFINED/* ç */, KEY_BACKSPACE, KEY_ERROR_UNDEFINED/* opt */, KEY_TAB},
	{KEY_ERROR_UNDEFINED /*EUR SYM*/, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7}
};
#endif

#define KEY_NC KEY_ERROR_UNDEFINED

uint8_t kbd_matrix[MATRIX_ROW][MATRIX_COL] = {
	{KEY_NC, KEY_EQUALS, KEY_BACKSPACE, KEY_NC, KEY_0, KEY_8, KEY_7, KEY_5, KEY_3, KEY_2, KEY_1, KEY_NC, KEY_NC, KEY_ESCAPE, KEY_NC}, // 16
	{KEY_NUMBER /* * µ */,KEY_RIGHT_BRACE /* $ */, KEY_MINUS /* ° ) ] */, KEY_LEFT_BRACE /* ^ */, KEY_DELETE, KEY_9, KEY_Y, KEY_6, KEY_4, KEY_E, KEY_Q /* a */, KEY_NC, KEY_NC, KEY_TAB, KEY_NC}, // 17
	{KEY_NC, KEY_ENTER, KEY_SEMICOLON /* m */, KEY_P, KEY_I, KEY_U, KEY_NC, KEY_T, KEY_R, KEY_S, KEY_W /* z */, KEY_NC, KEY_NC, KEY_CAPS_LOCK, KEY_NC},	// 19
	{KEY_NC, KEY_QUOTE /* % */, KEY_SLASH /* ! */, KEY_O, KEY_K, KEY_J, KEY_H, KEY_G, KEY_F, KEY_D, KEY_A /* q */, KEY_LEFT_SHIFT, KEY_NC, KEY_NC, KEY_NC}, // 20
	{KEY_NC, KEY_NC, KEY_UP, KEY_L, KEY_M /* ? */, KEY_N, KEY_B, KEY_V, KEY_C, KEY_X, KEY_Z /* w */, KEY_RIGHT_SHIFT, KEY_NC, KEY_NC, KEY_NC}, // 22
	{KEY_RIGHT, KEY_NC, KEY_NC, KEY_PERIOD /* / : */, KEYPAD_BACKSLASH /* < > */, KEY_NC,KEY_NC,KEY_NC,KEY_NC,KEY_NC,KEY_NC,KEY_NC,KEY_NC,KEY_NC, KEY_RIGHT_ALT}, // 23
	{KEY_DOWN, KEY_LEFT, KEY_BACKQUOTE /* 2 */, KEY_COMMA /* . ; */, KEY_NC, KEY_SPACE, KEY_NC,KEY_NC,KEY_NC,KEY_NC,KEY_NC,KEY_NC,KEY_CONTROL, KEY_NC /* Fn */, KEY_LEFT_ALT} // 23
};

void kbd_input_parse(uint8_t in, uint8_t * mat) {
	delay(10);
	uint16_t read = gpio_port_read(KBD_O_PORT);
//	uint16_t read = gpio_get(KBD_O_PORT, KBD_O1 | KBD_O2 | KBD_O3 | KBD_O4 | KBD_O5 | KBD_O6 | KBD_O7 | KBD_O8);
	mat[0] = (read >> 0) & 0b1;
	mat[1] = (read >> 1) & 0b1;
	mat[2] = (read >> 2) & 0b1;
	mat[3] = (read >> 3) & 0b1;
	mat[4] = (read >> 4) & 0b1;
	mat[5] = (read >> 5) & 0b1;
	mat[6] = (read >> 6) & 0b1;
	mat[7] = (read >> 7) & 0b1;
	mat[8] = (read >> 8) & 0b1;
	mat[9] = (read >> 9) & 0b1;
	mat[10] = (read >> 10) & 0b1;
	// PA11 & PA12 USB
	mat[11] = (read >> 13) & 0b1;
	mat[12] = (read >> 14) & 0b1;
	read=gpio_port_read(GPIOF);
	// PF0 PF1
	mat[13] = (read >> 0) & 0b1;
	mat[14] = (read >> 1) & 0b1;


#if 0
	char str[18];
	str[0]=in +'0';
	str[1]=':';
	int i;
	for(i=2;i<17;i++) {
		if(mat[i-2])
			str[i]='u';
		else
			str[i]='d';
	}
	str[17]='\n';
	usb_send_serial_data(str,18);
#endif

}

void parse_keyboard() {
	int i, j;
	uint8_t matrix[MATRIX_ROW][MATRIX_COL];

	gpio_set(GPIOB, KBD_I1);
	kbd_input_parse(1,matrix[0]);
	gpio_clear(GPIOB, KBD_I1);

	gpio_set(GPIOB, KBD_I2);
	kbd_input_parse(2,matrix[1]);
	gpio_clear(GPIOB, KBD_I2);

	gpio_set(GPIOB, KBD_I3);
	kbd_input_parse(3,matrix[2]);
	gpio_clear(GPIOB, KBD_I3);

	gpio_set(GPIOB, KBD_I4);
	kbd_input_parse(4,matrix[3]);
	gpio_clear(GPIOB, KBD_I4);

	gpio_set(GPIOB, KBD_I5);
	kbd_input_parse(5,matrix[4]);
	gpio_clear(GPIOB, KBD_I5);

	gpio_set(GPIOB, KBD_I6);
	kbd_input_parse(6,matrix[5]);
	gpio_clear(GPIOB, KBD_I6);

	gpio_set(GPIOB, KBD_I7);
	kbd_input_parse(7,matrix[6]);
	gpio_clear(GPIOB, KBD_I7);
/*
	gpio_set(GPIOA, KBD_I8);
	kbd_input_parse(matrix[7]);
	gpio_clear(GPIOA, KBD_I8);
*/
	for (i = 0; i < MATRIX_ROW; i++) {
		for (j = 0; j < MATRIX_COL; j++) {
			if (matrix[i][j]) {
				usb_keyboard_key_down(kbd_matrix[i][j]);
			} else {
				usb_keyboard_key_up(kbd_matrix[i][j]);
			}
		}
	}

}

#define __WFI() __asm__("wfi")
#define __WFE() __asm__("wfe")


void rcc_init() {
#ifdef CUSTOM_BOARD
	// crystal-less ? 
	rcc_clock_setup_in_hsi48_out_48mhz();
	rcc_periph_clock_enable(RCC_SYSCFG_COMP);
	SYSCFG_CFGR1 |= SYSCFG_CFGR1_PA11_PA12_RMP;
	crs_autotrim_usb_enable();
	rcc_set_usbclk_source(RCC_HSI48);
#else
	rcc_clock_setup_in_hsi_out_48mhz();
#endif

	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_GPIOF);

}

void sys_init() {

	rcc_init();
	gpio_setup();
	systick_setup();
	usb_setup();
}

void pow_init() {
	SCB_SCR &= ~SCB_SCR_SLEEPDEEP;
	SCB_SCR |= SCB_SCR_SLEEPONEXIT;
}

int main(void)
{
	sys_init();
//	delay(100);

#ifdef USB_USE_INT
//	pow_init();
#endif

	while (1) {
		parse_keyboard();
		usb_send_keys_if_changed();
#ifdef USB_USE_INT
//		__WFI();

#else
		usb_poll();
#endif
	}

	return 0;
}

